//***************************************************************************
// Copyright (C) EMC Corporation 2012-2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      PmpInterface.h
//
// Contents:
//      Defines the PmpInterface class.
//
//      This class provides a C++ interface to the PMP library (which is written in C)
//
//      PMP provides memory persistence in the case of powerfails on system with no
//      backup power supply.
// Revision History:
//--
//

#ifndef __PmpInterface_h__
#define __PmpInterface_h__

#include "EmcPAL_Status.h"
#include "EmcPAL_Misc.h"
#include "libpmp.h"
#include "spid_types.h"
#include "spid_kernel_interface.h"
#if defined(SIMMODE_ENV)
#include "mut.h"
#include "lxpmp.h"
#include "simulation/SimEmcPALWrappers.h"
#include "EmcPAL_DriverShell.h"
#include "simulation/SimPmpPartition.h"
#include "simulation/CacheSize_option.h"
#include "simulation/BvdSimMutBaseClass.h"
#include "simulation/spid_simulation_interface.h"

PAbstractDriverReference get_Cache_DriverReference();

#endif // SIMMODE_ENV    
#include "cmm_types.h"
#include "MemoryPersistStruct.h"

#ifdef C4_INTEGRATED 

#ifdef SIMMODE_ENV
CSX_MOD_IMPORT
void setPmpPartitionSizeBytes(unsigned long long pmpPartitionSize);
#endif

class PmpInterface {
public:
    PmpInterface() : mDeviceCapacityInMB(0) {
#if defined(SIMMODE_ENV)
        libpmpLog("PmpInterface::PmpInterface PMP Initializing....\n");
        Cache_Size_option *cs = Cache_Size_option::getCliSingleton();
        SPID spid;

        CSX_MAYBE_UNUSED EMCPAL_STATUS status = spidSimGetSpid(&spid);
        FF_ASSERT(status == EMCPAL_STATUS_SUCCESS);


        BvdSimMutBaseClass *system = get_Cache_DriverReference()->getSystem();
        SPID_HW_TYPE hwType;
        bool changePMPSize = false;
        UINT_64 partitionSize = 0;

        if(cs->get() != 0) {
            partitionSize = ((UINT_64) cs->get() * (UINT_64) (1024*1024));
            // If the user specifies a cache size that is too small then
            // the PMP Partition will get too small and will fail initialization
            // therefore we add in the pmp partition overhead so that we will not fail.
            if(cs->get() < 200) {
                partitionSize += PMP_PARTITION_OVERHEAD;
            }
            changePMPSize = true;
        } else {
            if(system->getTestCapability(BvdSim_Capability_SPA_LargeCMMMemory) ||
              (system->getTestCapability(BvdSim_Capability_1GB_CMMMemory))) {
                libpmpLog("PmpInterface::growing to 1GB cache.....\n");
                partitionSize = (UINT_64) (1024*1024*1024) + PMP_PARTITION_OVERHEAD;
                cs->_set(1024 + (ULONG) (PMP_PARTITION_SIZE/(UINT_64)(1024*1024)));
                changePMPSize = true;
            }
        }
        // See if the test requires a Smaller Vault.  If so we have to determine how
        // we will proceed.
        if(ccrGetPlatformType(&hwType)) {
            if (system && system->getTestCapability(BvdSim_Capability_Smaller_Vault)) {
                libpmpLog("Requesting smaller Vault\n");
                switch(hwType) {
                    case SPID_HYPERION_1_HW_TYPE: //         = 0x3B,
                    case SPID_HYPERION_2_HW_TYPE: //         = 0x3C,
                    case SPID_HYPERION_3_HW_TYPE: //         = 0x3D,
                        libpmpLog("Running on Hyperion which vaults to PMP\n");
                        // We are on a HYPERION class machine which vaults to the PMP
                        // drive.  The user has also requested that we use a smaller
                        // vault for this test, so we want to make sure the Vault is
                        // sized appropriately.   Note, PMP is not happy if the vault
                        // is less then 2GB+Fudge, so we we will only do this is the
                        // specified Cache size > 2GB.
                        if(cs->get() >= 256) {
                            //partitionSize = PMP_PARTITION_SIZE_DEFAULT;
                            partitionSize = (UINT_64) ((cs->get()-100) * 1024 * 1024); // + PMP_PARTITION_OVERHEAD;
                            libpmpLog("Reducing Cache Size %lldMB PmpPartitionSize %lldMB\n", (UINT_64) cs->get(),(UINT_64) partitionSize/ (UINT_64) (1024*1024));
                            changePMPSize = true;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        if(changePMPSize) {
            libpmpLog("Requesting PMP Partition Size change to %lldMB\n", (UINT_64)partitionSize/(UINT_64)(1024 * 1024));
            setPmpPartitionSizeBytes(partitionSize);
        }

        // Each SP must open and initialize their own file.
        if(spid.engine == 0) {
            mPartitionA = new SimPmpPartition(PMP_SPA);
            mPartitionB = NULL;
        } else {
            mPartitionB = new SimPmpPartition(PMP_SPB);
            mPartitionA = NULL;
        }
#endif // SIMMODE_ENV

        mHandle = LIBPMP_INVALID_HANDLE();
        // No need to check return status, the validness of mHandle will suffice
        open(libpmp_client_mcc);

        // See if the platform is Virtual  If it is, we will just
        // exit
        SPID_PLATFORM_INFO platformInfo;

        spidGetHwTypeEx(&platformInfo);
        if(platformInfo.isVirtual) {
            return;
        }
        getDeviceCapacityInMB(&mDeviceCapacityInMB);
    }


    ~PmpInterface() {
        close();
#if defined(SIMMODE_ENV)
        delete mPartitionA;
        delete mPartitionB;
#endif // SIMMODE_ENV    
    }

    //
    // addSgl
    //
    // Description:             Present a block of memory to be protected by PMP.
    //                          Upon successful return, the memory will be
    //                          protected by PMP.
    //
    // Input:       
    //   data:                  Pointer to the start of physical memory to
    //                          be protected.
    //   size:                  Length of the memory to be protected.
    //
    // Output:      
    //   EMCPAL_STATUS_SUCCESS: The operation was successful.  On failure the
    //                          method asserts.
    //
    EMCPAL_STATUS addSgl(CMM_MEMORY_ADDRESS data, CMM_MEMORY_SIZE size)
    {
        libpmp_status_t pmpStatus = libpmp_status_success;
        uint32_t entryNumberNotUsed;

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            pmpStatus = libpmpAddSgl(mHandle, (uint64_t)size, (uint64_t)data, &entryNumberNotUsed);
        } else {
            pmpStatus = libpmp_status_not_initialized;
        }
        libpmpLog("(%s) PmpInterface::addSgl: %s", clientIdToName(mId), libpmpErrorString(pmpStatus));
        if(LIBPMP_SUCCESS(pmpStatus)) {
            return EMCPAL_STATUS_SUCCESS;
        } else {
            return EMCPAL_STATUS_UNSUCCESSFUL;
        }
    }

    //
    //  getDeviceCapacityInMB
    //
    //
    // Description:
    //  This function will return the amount of memory (in MB) that lxPMP can protect
    //  through a powerfail.
    //
    //  N.B. This number reflects the total memory that can be protected (for all
    //  PMP clients).
    //
    //
    // Arguments:
    //  handle              Handle returned by libpmpRegister
    //
    // Return Value:
    //  libpmp_status_t
    //  capacity            Populated with PMP capacity in MB.
    //
    EMCPAL_STATUS getDeviceCapacityInMB(uint32_t* deviceCapacityInMB)
    {
        libpmp_status_t pmpStatus;

        if(mDeviceCapacityInMB == 0) {

            pmpStatus =  libpmpGetDeviceCapacity( mHandle, deviceCapacityInMB );

            if(!LIBPMP_SUCCESS(pmpStatus)) {
                libpmpLog("(%s) PmpInterface::libpmpGetDeviceCapacity: %s", clientIdToName(mId), libpmpErrorString(pmpStatus));
                *deviceCapacityInMB = 0;
                return EMCPAL_STATUS_UNSUCCESSFUL;
            }

            mDeviceCapacityInMB = *deviceCapacityInMB;
        } else {
            *deviceCapacityInMB = mDeviceCapacityInMB;
        }

        return(EMCPAL_STATUS_SUCCESS);
    }

    //
    // getSglSize
    //
    // Description:             Get the size of the current SGL if present.
    //
    // Input:
    //   usage  - address to receive clients usage of the PMP partition
    //
    // Output:
    //   EMCPAL_STATUS_SUCCESS: The operation was successful.  On failure the
    //                          method asserts.
    //
    EMCPAL_STATUS getSglSize(uint64_t* usage)
    {
        libpmp_status_t pmpStatus;

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            pmpStatus = libpmpGetClientUsage(mHandle, usage);

            libpmpLog("(%s) PmpInterface::getSglSize: %s", clientIdToName(mId), libpmpErrorString(pmpStatus));
            CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
            return EMCPAL_STATUS_SUCCESS;
        }
        return(EMCPAL_STATUS_UNSUCCESSFUL);
    }
    //
    // removeSglAll
    //
    // Description:             Stop protecting all memory that was added
    //                          to pmp with addSgl.
    //
    // Input:                   None.
    //
    // Output:
    //   EMCPAL_STATUS_SUCCESS: The operation was successful.  On failure the
    //                          method asserts.
    //
    EMCPAL_STATUS removeSglAll(void)
    {
        libpmp_status_t pmpStatus;

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            pmpStatus = libpmpDelSglAll(mHandle);
            libpmpLog("(%s) PmpInterface::removeSglAll: %s", clientIdToName(mId), libpmpErrorString(pmpStatus));
            CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
        }
        return(EMCPAL_STATUS_SUCCESS);    
    }

    //
    // RestoredMemory
    //
    // Description: Determine if PMP restored memory from the PMP partition.
    //              And, if it did, if that memory is valid.
    //
    // Input:   None
    //
    // Output:
    //   true:  Memory restored by PMP with no errors.
    //   false: Memory not restored, was restored with errors or PMP is not
    //          supported on this platform.
    //
    bool RestoredMemory(void) 
    {
        uint32_t operflags;
        uint16_t appflags;
        libpmp_status_t status;
        bool rc;

        if(!LIBPMP_HANDLE_IS_VALID(mHandle)) {
            // PMP not supported on this platform.
            return true;
        }

        status = libpmpGetAndClearUserFlags(mHandle, 0, &operflags, &appflags);
        if(!LIBPMP_SUCCESS(status)) {
            libpmpLog("PmpInterface::RestoredMemory(%s): Unable to read PMP flags: %s",
                    clientIdToName(mId), libpmpErrorString(status));
            return false;
        }
        libpmpLog("PmpInterface::RestoredMemory(%s): operflags:0x%x. appflags:0x%x", 
                  clientIdToName(mId), operflags, appflags);
        if(LIBPMP_USER_FLAG_IS_ERROR(operflags)) {
            libpmpLog("PmpInterface::RestoredMemory(%s): Error (0x%x)", clientIdToName(mId), operflags);
            return false;
        }
        if((operflags & libpmpFlagImageRestored)) {
            if(operflags & libpmpFlagImageValid) {
              libpmpLog("PmpInterface::RestoredMemory(%s): Memory successfully restored.", 
                          clientIdToName(mId));
                rc = true;
            } else {
              libpmpLog("PmpInterface::RestoredMemory(%s): Restored but not valid", clientIdToName(mId));
                rc = false;
            }
        } else {
            libpmpLog("PmpInterface::RestoredMemory(%s): Not restored", clientIdToName(mId));
            rc = false;
        }
        // ?!? Also need to check signature and validate the drive was filled on
        // this chassis.
        return rc;
    }

    //
    // disable
    //
    // Description:   Disable PMP for this client.  This method will assert if
    //                it encounters any errors.
    //
    // Input:         None
    //
    // Output:        true  if PMP was enabled before this method was invoked.
    //                false if PMP was disabled before this method was invoked.
    //
    bool disable(void)
    {
        libpmp_status_t pmpStatus;
        bool enabled = false;

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            // Determine if PMP for MCC is enabled.
            pmpStatus = libpmpClientIsEnabled(mHandle, (csx_bool_t *)&enabled);
            CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
	
            if(enabled) {
                pmpStatus = libpmpClientDisable(mHandle);
                CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
            }
        }
        return enabled;
    }

    // Enable PMP for MCC.
    //
    // enable
    //
    // Description:   Enable PMP for this client.  This method will assert
    //                if it encounters any errors.
    //
    // Input:         None
    //
    // Output:        None
    //
    void enable(void)
    {
        libpmp_status_t pmpStatus;
        csx_bool_t enabled;

        libpmpLog("PmpInterface::PmpInterface enable....\n");

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            pmpStatus = libpmpClientIsEnabled(mHandle, &enabled);
            CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
    
            if(!enabled) {
                // Make sure that the SGL is installed before allowing an enable.
                csx_bool_t installed = false;
                pmpStatus = libpmpIsSglInstalled(mHandle, &installed);
                CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
                CSX_ASSERT_H_RDC(installed == true);

                pmpStatus = libpmpClientEnable(mHandle);
                CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
            }
        }
    }

    //
    // shutdown
    //
    // Description:   Remove all SGL's from PMP and close.
    //
    // Input:         None.
    //
    // Output:        None.  On failure, the methods called by this method
    //                may assert.
    //
    void shutdown(void)
    {
        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            EMCPAL_STATUS status;

            // Remove the old SGL, we are protecting anything.
            // When we need to protect something we will add
            // a new SGL.
            status = removeSglAll();
            CSX_ASSERT_H_RDC(status == EMCPAL_STATUS_SUCCESS);
            close();
        }
    }

    //
    // SavePersistentMemory
    //
    // Description:   Have lxpmp dump persistent memory.
    //
    // Input:         None.
    //
    // Output:        true:  success
    //                false: failure
    //
    bool SavePersistentMemory();

    //
    // rearm
    //
    // Description:   Prepare lxpmp to do another protection cycle.
    //
    // Input:         None.
    //
    // Output:        true:  success
    //                false: failure
    //
    bool rearm(void)
    {
        libpmp_status_t status;

        if(!LIBPMP_HANDLE_IS_VALID(mHandle)) {
            return false;
        }

        libpmpLog("PmpInterface::PmpInterface rearm....\n");

        // Need to clear error bits and _RESTORED and _VALID, easier to
        // zero out all flags.
        status = libpmpClearUserFlags( mHandle );
        if (!LIBPMP_SUCCESS(status)) {
            libpmpLog("(%s) rearm: Unable to clear user flags: %s",clientIdToName(mId),
                      libpmpErrorString(status));
            return false;
        }
        enable();
        libpmpLog("(%s) Successfully rearmed", clientIdToName(mId));
        return true;
    }

    //
    // suppported
    //
    // Description:   Indicate whether PMP is supported on this platform.
    //
    // Input:         None.
    //
    // Output:        true:  PMP is supported
    //                false: PMP is not supported
    //
    bool supported(void)
    {
        return mPmpSupported;
    }

    //
    // requestMpDisable
    //
    // Description:   Allow memory persistence to be disabled.  During shutdown MP will be disabled
    //                if all PMP users approve.
    //
    // Input:         None.
    //
    // Output:        true:  success
    //                false: failure
    //
    bool approveMpDisable(void)
    {
        bool   returnStatus = false;

        libpmpLog("(%s) PmpInterface::approveMpDisable", clientIdToName(mId));
        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            libpmp_status_t pmpStatus;
            
            pmpStatus = libpmpGlobalSetMpApproveDisableRequest(mHandle);
            if(LIBPMP_SUCCESS(pmpStatus)) {
                libpmpLog("(%s) PmpInterface::approveMpDisable: success", 
                          clientIdToName(mId));
                returnStatus = true;
            } else {
                libpmpLog("(%s) PmpInterface::approveMpDisable: failed: %s", 
                          clientIdToName(mId), libpmpErrorString(pmpStatus));
            }
        }
        return returnStatus;
    }

    //
    // PersistStatus
    //
    // Description:   Return the status of Memory Persistence from the PMP partion.
    //                This is the original value of the status, found by lxpmp in the 
    //                bootflash.
    //
    // Input:         None.
    //
    // Output:        Value originally in MP header "PersistStatus" (sptool -mp header)
    //                -1 if PMP is not supported on this platform.
    //
    uint32_t PersistStatus(void)
    {
        uint32_t mpStatus = (uint32_t) -1;
        libpmp_status_t pmpStatus;

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            pmpStatus = libpmp_get_mp_status(mHandle, &mpStatus);
            CSX_ASSERT_H_RDC(LIBPMP_SUCCESS(pmpStatus));
        } 
        libpmpLog("(%s) PmpInterface::PersistStatus: 0x%x",
                  clientIdToName(mId), mpStatus);
        return mpStatus;
    }

    //
    // PmpErrors
    //
    // Description:   Determine if lxpmp has reported any errors.
    //
    // Input:         None.
    //
    // Output:        true:  lxpmp has reported errors.
    //                false: lxpmp has not reported errors.
    //
    bool PmpErrors(void)
    {
        uint32_t operflags;
        uint16_t appflags;
        libpmp_status_t status;

        if(!LIBPMP_HANDLE_IS_VALID(mHandle)) {
            // PMP not supported on this platform.
            return false;
        }

        status = libpmpGetAndClearUserFlags(mHandle, 0, &operflags, &appflags);
        if(!LIBPMP_SUCCESS(status)) {
          libpmpLog("PmpInterface::PmpErrors(%s): Unable to read PMP flags: %s", 
                    clientIdToName(mId), libpmpErrorString(status));
          return true;
        }
        libpmpLog("PmpInterface::PmpErrors(%s): operflags:0x%x. appflags:0x%x", 
                  clientIdToName(mId), operflags, appflags);
        if(LIBPMP_USER_FLAG_IS_ERROR(operflags)) {
            return true;
        } else {
            return false;
        }
    }

private:  
    //
    // open
    //
    // Description: Initializes PMP and registers the caller with PMP.
    //
    // Input:
    //   id:                         Identification of the caller.
    //
    // Output:
    //   EMCPAL_STATUS_SUCCESS:      The operation was successful.
    //   EMCPAL_STATUS_UNSUCCESSFUL: Some step in the operation failed, the data cannot
    //                               be protected by PMP.
    //
    EMCPAL_STATUS open(libpmp_client_id id) 
    {
        EMCPAL_STATUS   returnStatus = EMCPAL_STATUS_SUCCESS;

        libpmpLog("(%s) PmpInterface::open", clientIdToName(id));
        if(! LIBPMP_HANDLE_IS_VALID(mHandle)) {
            libpmp_status_t pmpStatus;

            // Initialize PMP
            pmpStatus = libpmpInit( &mHandle, id, clientIdToName(id) );
            if (libpmp_status_pmp_not_supported == pmpStatus) {
                libpmpLog("(%s) PmpInterface::open PMP not supported", clientIdToName(id));
                mPmpSupported = false;
                returnStatus = EMCPAL_STATUS_SUCCESS;
            } else if (!LIBPMP_SUCCESS(pmpStatus)) {
                // Some other error
                libpmpLog("(%s) PmpInterface::open Init failed: %s", clientIdToName(id),
                          libpmpErrorString(pmpStatus));
                mPmpSupported = false;
                returnStatus = EMCPAL_STATUS_UNSUCCESSFUL;
            } else {
                // PMP Initialization worked, now register
                mPmpSupported = true;
                pmpStatus = libpmpRegister( mHandle, LXPMP_APP_REVISION );
                if (!LIBPMP_SUCCESS(pmpStatus)) {
                    returnStatus = EMCPAL_STATUS_UNSUCCESSFUL;
                    libpmpLog("(%s) PmpInterface::open Register failed: %s", clientIdToName(id),
                              libpmpErrorString(pmpStatus));
                } else {
                    returnStatus = EMCPAL_STATUS_SUCCESS;
                    mId = id;
                }
            }
        }
        libpmpLog("(%s) PmpInterface::open returning %s", clientIdToName(id), 
                  csx_p_cvt_csx_status_to_string(returnStatus));
        return returnStatus;
    }

    //
    // close
    //
    // Description: Close interface to PMP.  After this call PMP will not protect
    //              any data for this client
    //
    // Input:       None
    //
    // Output:      None
    //
    void close()
    {
        libpmpLog("(%s) PmpInterface::close", clientIdToName(mId));

        if(LIBPMP_HANDLE_IS_VALID(mHandle)) {
            mHandle      = LIBPMP_INVALID_HANDLE();
            mId          = libpmp_client_max;
        }
    }

    libpmp_handle_t        mHandle;
    libpmp_client_id       mId;
    bool                   mPmpSupported;
    const char            *clientIdToName(libpmp_client_id);
    uint32_t              mDeviceCapacityInMB;
#if defined(SIMMODE_ENV)
    SimPmpPartition        *mPartitionA;
    SimPmpPartition        *mPartitionB;
#endif // SIMMODE_ENV    
};

#ifdef SIMMODE_ENV
/*
 * This class is used to run the lxpmp.exe program.  It's only used by the MCC
 * simulation code.
 */
class LxPmpInterface {

public:
    LxPmpInterface() {
        SPID            sp_info;
        EMCPAL_STATUS   status;

        status = spidGetSpid(&sp_info);
        CSX_ASSERT_H_RDC(EMCPAL_IS_SUCCESS(status));

        if( EMCPAL_IS_SUCCESS(status) ) {
            mSPid = sp_info.engine == 0 ? SP_A : SP_B;
        } else {
            mSPid = SP_INVALID;
        }
    }


    void dumpLxPmp(const char *message)
    {
        char *argv_a_dump[] = {
            "lxpmp", "-F", PMP_A_FILENAME, "-f", "-D"
        };
        char *argv_b_dump[] = {
            "lxpmp", "-F", PMP_B_FILENAME, "-f", "-D"
        };
    
        libpmpLog("%s", message);
        if( mSPid == SP_B ) {
            (void) lxpmp_main( sizeof(argv_b_dump)/sizeof(char *), argv_b_dump );
        } else {
            (void) lxpmp_main( sizeof(argv_a_dump)/sizeof(char *), argv_a_dump );
        }
    }
  
    void runLxPmp(bool booting)
    {
        int status;
        char *argv_a_boot[] = {
            "lxpmp", "--brief", "--file", PMP_A_FILENAME, "--restore"
        };
        char *argv_a_powerloss[] = {
            "lxpmp", "--brief", "--file", PMP_A_FILENAME, "--save"
        };
        char *argv_b_boot[] = {
            "lxpmp", "--brief", "--file", PMP_B_FILENAME, "--restore"
        };
        char *argv_b_powerloss[] = {
            "lxpmp", "--brief", "--file", PMP_B_FILENAME, "--save"
        };
    
        libpmpLog("%s<%d>: %s %s",
                  __FUNCTION__, __LINE__, booting ? "Booting" : "Powerfail",
                  mSPid == SP_A ? "SPA" : "SPB");

        if( booting && SP_B == mSPid ) {
            libpmpLog("%s<%d>: %s %s %s %s %s", __FUNCTION__, __LINE__,
                      argv_b_boot[0],
                      argv_b_boot[1],
                      argv_b_boot[2],
                      argv_b_boot[3],
                      argv_b_boot[4]);
            status = lxpmp_main( sizeof(argv_b_boot)/sizeof(char *), argv_b_boot );
        } else if( booting && SP_A == mSPid ) {
            libpmpLog("%s<%d>: %s %s %s %s %s", __FUNCTION__, __LINE__,
                      argv_a_boot[0],
                      argv_a_boot[1],
                      argv_a_boot[2],
                      argv_a_boot[3],
                      argv_a_boot[4]);
            status = lxpmp_main( sizeof(argv_a_boot)/sizeof(char *), argv_a_boot );
        } else if( !booting && SP_B == mSPid ) {
            libpmpLog("%s<%d>: %s %s %s %s %s", __FUNCTION__, __LINE__,
                      argv_b_powerloss[0],
                      argv_b_powerloss[1],
                      argv_b_powerloss[2],
                      argv_b_powerloss[3],
                      argv_b_powerloss[4]);
            status = lxpmp_main( sizeof(argv_b_powerloss)/sizeof(char *), argv_b_powerloss );
        } else {
            libpmpLog("%s<%d>: %s %s %s %s %s", __FUNCTION__, __LINE__,
                      argv_a_powerloss[0],
                      argv_a_powerloss[1],
                      argv_a_powerloss[2],
                      argv_a_powerloss[3],
                      argv_a_powerloss[4]);
            status = lxpmp_main( sizeof(argv_a_powerloss)/sizeof(char *), argv_a_powerloss );
        }
        return;
    }

    SP_ID                  mSPid;

};
#endif /* SIMMOD_ENV */





#else  /* C4_INTEGRATED */
/* PMP is not used on VNX, simply provide stubs so we can build. */

#include "cmm_types.h"

class PmpInterface {
public:
    PmpInterface() { }

    EMCPAL_STATUS open(libpmp_client_id id) 
    {
        return EMCPAL_STATUS_SUCCESS;
    }

    void close()
    {
    }
    
    EMCPAL_STATUS getSglSize(uint64_t* usage)
    {
        return (usage) ? EMCPAL_STATUS_SUCCESS : EMCPAL_STATUS_UNSUCCESSFUL;
    }
    
    EMCPAL_STATUS addSgl(CMM_MEMORY_ADDRESS data, CMM_MEMORY_SIZE size)
    {
        return(EMCPAL_STATUS_SUCCESS);
    }

    EMCPAL_STATUS getDeviceCapacityInMB(uint32_t* deviceCapacityInMB)
    {
      return(EMCPAL_STATUS_SUCCESS);
    }

    EMCPAL_STATUS removeSglAll(void)
    {
        return EMCPAL_STATUS_SUCCESS; 
    }

    bool RestoredMemory(void) const
    {
        return false;
    }

    bool disable(void)
    {
        return false;
    }

    void enable(void)
    {
    }

    void shutdown(void)
    {
    }

    bool rearm(void)
    {
        return true;
    }

    bool supported(void)
    {
        return false;
    }

    bool requestMpDisable(void)
    {
        return false;
    }

    bool approveMpDisable(void)
    {
        return false;
    }

    uint32_t PersistStatus(void)
    {
        return (uint32_t) -1;
    }

    bool PmpErrors(void)
    {
        return false;
    }

    bool SavePersistentMemory()
    {
      return false;
    }

private:
    libpmp_handle_t        mHandle;
    libpmp_client_id       mId;
    bool                   mPmpSupported;
    const char            *clientIdToName(libpmp_client_id);
};

#ifdef SIMMODE_ENV
class LxPmpInterface {
public:
    LxPmpInterface() {
    }

    void dumpLxPmp(const char *message)
    {
    }
  
    void runLxPmp(bool booting)
    {
    }
};
#endif /* SIMMODE_ENV */

#endif /* C4_INTEGRATED - C4HW - PMP */
#endif /* __PmpInterface_h__ */
