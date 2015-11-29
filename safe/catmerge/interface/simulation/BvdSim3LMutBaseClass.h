//***********************************************************************
//
//  Copyright (c) 2015 EMC Corporation.
//  All rights reserved.
//
//  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
//  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
//  BE KEPT STRICTLY CONFIDENTIAL.
//
//  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
//  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR THE PURPOSE OF
//  CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION MAY RESULT.
//
//***********************************************************************

/***************************************************************************
 *                               BvdSim3LMutBaseClass.cpp
 ***************************************************************************
 *
 * DESCRIPTION:  Implementation of the BvdSim 3Legged base testing class
 *               All 3Legged Unit tests need to inherit from this class
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/21/2010   Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _BVDSIM3LMUTBASECLASS_
#define _BVDSIM3LMUTBASECLASS_
#include "csx_ext.h"
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include <stdlib.h>
# include <stdio.h>
# include "generic_types.h"
# include "string.h"
# include "simulation/BvdSimMutProgramBaseClass.h"
# include "simulation/BvdSimMutBaseClass.h"
# include "simulation/DiskIdentity.h"
# include "simulation/VirtualFlareDriver.h"
# include "simulation/ConcurrentIoGenerator.h"
# include "simulation/ConcurrentIOStreamConfig.h"
# include "simulation/ArrayDirector.h"
# include "simulation/BvdSim_Registry.h"
# include "simulation/managed_malloc.h"
# include "simulation/BvdSim_nvram.h"
# include "simulation/BvdSim_cmm.h"
# include "simulation/AutoInsertFactoryStackEntry.h"
# include "simulation/VirtualFlareStackEntry.h"
# include "simulation/BasicDriverEntryPoint_DriverReferenceAdapter.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/BvdSimSP.h"
# include "simulation/BvdSimDefaultSystemConfigurator.h"
# include "simulation/simulation_dynamic_scope.h"
# include "simulation/PersistentNvRamMemory.h"
# include "simulation/PersistentCMMMemory.h"
# include "MemoryPersistStruct.h"
# include "DiskSectorCount.h"
# include "K10Plx.h"


# define SECTOR_PER_PAGE 16
# define PAGES(n) ((n) * SECTOR_PER_PAGE * BYTES_PER_SECTOR)
# define PAGES520(n) ((n) * SECTOR_PER_PAGE * (BYTES_PER_SECTOR + 8))
# define PAGES_(n) ((n) * SECTOR_PER_PAGE)
# define SECTORS(n) ((n) * BYTES_PER_SECTOR)
# define SECTOR_ADDRESS(n) ((n) * BYTES_PER_SECTOR)

#define MUT_ARGUMENTS                 "-mutargs"

// Sometimes you may run into a situation where you need to create a library and
// one or some of the modules that you are compiling require BvdSim3LMutBaseClass.h.
// What you may find is that when you link with this library you encounter a linker
// error message indicating multiple references to “BVD_Trace”.
// In order to eliminate this linker problem, define NO_BVD_TRACE in your library
// modules before the “#include” of BvdSim3LMutBaseClass.h and “BVD_Trace” will be defined as an extern.
// This will cause your linker issue to go away.
#if !defined(NO_BVD_TRACE)
void  BvdSim_Trace(const mut_logging_t level, const char *format, ...)
{
    va_list args;

    va_start( args, format );     /* Initialize variable arguments. */
    KvTracep(format, args);
}
#else
extern void  BvdSim_Trace(const mut_logging_t level, const char *format, ...);
#endif


class AbstractPersistentMemoryControl {
public:
    virtual void setPersistenceMemoryExitStatus(UINT_32 status) = 0;
};

// Class for logging from within a simulated SP.
class TestLogger {
public:
    static void testLogMsg(mut_logging_t logLevel, char *format, ...) __attribute__((format(__printf_func__,2,3)))
    {
        va_list argptr;
        va_start(argptr, format);

        char formatBuffer[1024], msgBuffer[1024];

        csx_p_strcpy(formatBuffer, SPName());
        csx_p_strcat(formatBuffer, ": ");
        csx_p_strcat(formatBuffer, format);

        FF_ASSERT_NO_THIS(csx_p_strlen(formatBuffer) + csx_p_strlen(format) < 1023);  // leave space for string terminator

        csx_p_vsprintf(msgBuffer, formatBuffer, argptr);
        va_end(argptr);
        BvdSim_Trace(logLevel, msgBuffer);
    }

private:
    static char *SPName()  {
        if(BvdSim_Registry::getSpId() == 0) {
            return "SPA";
        }
        else {
            return "SPB";
        }
    }
};


class BvdSim3LMutBaseClass: public BvdSimMutBaseClass{
public:
    BvdSim3LMutBaseClass(PAbstractDriverReference driverConfig = NULL)
    : BvdSimMutBaseClass(driverConfig){
    }

    /*
     * Provide implementations of CAbstractTest StartUP() & TearDown()
     * that invoke standard BvdSimButBaseClass StartUp()/TearDown()
     */
    void StartUp() {
        BvdSimMutBaseClass::StartUp();
    }
    void TearDown() {
       BvdSimMutBaseClass::TearDown();
    }

};


class BvdSim3LIntegrationTestBaseClass: public BvdSimMutIntegrationTestBaseClass {
public:
    BvdSim3LIntegrationTestBaseClass(PAbstractDriverReference driverConfig = NULL, UINT32 sectorSize = BE_BYTES_PER_BLOCK)
    : BvdSimMutIntegrationTestBaseClass(driverConfig),
      mSectorSize(sectorSize)
    {
        /*
         * The majority of 3 Legged tests need to configure
         * a simulation environment that:
         *    Configures/boots an SP with the LR, the PSM stack 
         *    and KDBM and the SyncTransaction libraries
         *    Configures tests to delete all system LUNS during startup/teardown
         *    This is typically done in your driver (see MCC Cache_DriverReference.cpp)
         */
        setTestCapability(BvdSim_Capability_Simulation, TRUE);
        setTestCapability(BvdSim_Capability_BootSystem, TRUE);
        setTestCapability(BvdSim_Capability_SkipTearDown, TRUE);
        setTestCapability(BvdSim_Capability_Delete_FS_On_Startup, useExistingSystemDevices == NULL || !useExistingSystemDevices->isSet());
        setTestCapability(BvdSim_Capability_Delete_FS_On_TearDown, leaveSystemDevices == NULL || !leaveSystemDevices->isSet());
        setTestCapability(BvdSim_Capability_PersistentConfiguration, TRUE);
        setTestCapability(BvdSim_Capability_PersistentDeviceCreation, TRUE);
    }

    void AddExtraParams(BvdSimRegistry_Value *param) {
        extraParams.addParameter(param);
    };

    // Delete any persistent disks that could be left hanging around.
    void DeletePersistentDisks(const char * testCaseName, UINT_32 firstVolumeIndex, UINT_32 volumeCount = 1)
    {
        char fileName[256];
        csx_status_e status;

        for(UINT_32 volumeIndex = firstVolumeIndex; volumeIndex < firstVolumeIndex + volumeCount; volumeIndex++)
        {
            csx_rt_print_sprintf(fileName,"VirtualFlare%d",volumeIndex);
            status = csx_p_file_delete(fileName);
            if(status != CSX_STATUS_SUCCESS) {
                BvdSim_Trace(MUT_LOG_HIGH, "%s: deleteFile(%s) lastError(0x%x)\n", testCaseName, fileName, (DWORD)status);
            }
        }
    }

protected:
    BvdSimExtraParameters extraParams;
    UINT32 mSectorSize;
};

class BvdSimSimpleDriverLoadBaseClass: public BvdSim3LIntegrationTestBaseClass {
public:

    BvdSimSimpleDriverLoadBaseClass(PAbstractDriverReference driverConfig = NULL, UINT32 sectorSize = BE_BYTES_PER_BLOCK)
    : BvdSim3LIntegrationTestBaseClass(driverConfig, sectorSize) {}
};

class BvdSimConcurrentConfigParams {
public:
# define SECT_PER_THD_ENV_STRING ("SECTORS_PER_THREAD")
# define DEFAULT_SECTORS_PER_THREAD (512)
# define LONG_TEST_SECTORS_PER_THREAD (122880)
# define TEST_ITERATIONS_STRING ("TEST_ITERATIONS")
# define DEFAULT_TEST_ITERATIONS (200)
    
    BvdSimConcurrentConfigParams() {
        mStartBlk = 0;
        mSectorsPerThd = DEFAULT_SECTORS_PER_THREAD;
        mIterations = 200;
        
        // Get sectors per thread environment variable so that tests can be configured
        // without re-compiling.
        mSectorsPerThd = MyGetEnvironmentVariable(SECT_PER_THD_ENV_STRING, DEFAULT_SECTORS_PER_THREAD);
        
        // Handle special cases of 0 = default and 1 = standard long run.
        if (mSectorsPerThd == 0) { 
            KvPrint("Using default sectors per thread of %d instead of 0", DEFAULT_SECTORS_PER_THREAD);
            mSectorsPerThd = DEFAULT_SECTORS_PER_THREAD;
        } else if (mSectorsPerThd == 1) {
            KvPrint("Using long test sectors per thread of %d instead of 1", LONG_TEST_SECTORS_PER_THREAD);
            mSectorsPerThd = LONG_TEST_SECTORS_PER_THREAD;
        }
        
        // Get test iterations environment variable.
        mIterations = MyGetEnvironmentVariable(TEST_ITERATIONS_STRING, DEFAULT_TEST_ITERATIONS);
        
        // Handle special case of 0 = default
        if (mIterations == 0) {
            mIterations = DEFAULT_TEST_ITERATIONS;
        }
        
    };
    BvdSimConcurrentConfigParams(ULONG startBlk, ULONG sectorsPerThd, ULONG iterations) :
    mStartBlk(startBlk), mSectorsPerThd(sectorsPerThd), mIterations(iterations) {};
    
    ULONG GetStartingBlk() const { return mStartBlk; };
    ULONG GetSectorsPerThread() const { return mSectorsPerThd; };
    ULONG GetIterations() const { return mIterations; };
    
private:
    
    ULONG MyGetEnvironmentVariable(const char *envStr, ULONG defaultVal) {
        FF_ASSERT(envStr);
        
        char *envStrVal = getenv(envStr);
        ULONG value = defaultVal;
        if (envStrVal == NULL) {
            KvPrint("Unable to get environment variable: %s, using default of %d",
                       envStr, defaultVal);
        }else {
            KvPrint("Using environment variable value of %s", envStrVal);
            value = atoi(envStrVal);
        }
        
        KvPrint("Returning value of %d\n", value);
        return value;
    }
    
    ULONG mStartBlk;
    ULONG mSectorsPerThd;
    ULONG mIterations;
};


class BvdSimConcurrentIoBaseClass:  public BvdSimSimpleDriverLoadBaseClass, BvdSimMutConcurrentIoLoad {
public:
    BvdSimConcurrentIoBaseClass(PAbstractDriverReference driverConfig = NULL, UINT_32 sectorSize = BE_BYTES_PER_BLOCK)
    : BvdSimSimpleDriverLoadBaseClass(driverConfig, sectorSize), BvdSimMutConcurrentIoLoad(driverConfig) {}

    virtual void Test() {
        BvdSimMutConcurrentIoLoad::Test();
    }
};


class BvdSimSystemTestMasterSPBaseClass: public BvdSim3LIntegrationTestBaseClass {
public:

    BvdSimSystemTestMasterSPBaseClass(PAbstractDriverReference driverConfig = NULL)
    : BvdSim3LIntegrationTestBaseClass(driverConfig) {
        /*
         * BvdSimIntegrationTestBaseClass sets the standard sytem simulation flags required for Single SP simulations
         * Add flag BvdSim_Capability_LaunchPeer so that the SPB process will be managed by this process
         */
        setTestCapability(BvdSim_Capability_LaunchPeer, TRUE);
    }
};

class BvdSimSystemTestMasterSPConcurrentIoBaseClass: public BvdSimSystemTestMasterSPBaseClass, public BvdSimMutConcurrentIoLoad {
public:
    BvdSimSystemTestMasterSPConcurrentIoBaseClass(PAbstractDriverReference driverConfig = NULL)
    : BvdSimSystemTestMasterSPBaseClass(driverConfig), BvdSimMutConcurrentIoLoad(driverConfig) { }

    virtual void Test() {
        BvdSimMutConcurrentIoLoad::Test();
    }

};

class BvdSimSPAOnlyMasterSystemBaseClass: public BvdSimSystemTestMasterSPBaseClass {
public:
    BvdSimSPAOnlyMasterSystemBaseClass(PAbstractDriverReference driverConfig = NULL)
    : BvdSimSystemTestMasterSPBaseClass(driverConfig) {
        /*
         * BvdSimSPAOnlyMasterSystemTestBaseClass is identical to BvdSimSystemTestMasterSPBaseClass
         * except that BvdSim_Capability_LaunchPeer is not set
         */
        setTestCapability(BvdSim_Capability_LaunchPeer, FALSE);
    }
};

class BvdSimSystemTestPeerSPBaseClass: public BvdSimSimplePeerSystemMutBaseClass {
public:

    BvdSimSystemTestPeerSPBaseClass(PAbstractDriverReference driverConfig = NULL)
    : BvdSimSimplePeerSystemMutBaseClass(driverConfig) {
        setTestCapability(BvdSim_Capability_SkipTearDown, TRUE);
        setTestCapability(BvdSim_Capability_Delete_FS_On_Startup, false);
        setTestCapability(BvdSim_Capability_Delete_FS_On_TearDown, false);
        setTestCapability(BvdSim_Capability_PersistentConfiguration, TRUE);
        setTestCapability(BvdSim_Capability_PersistentDeviceCreation, TRUE);
    }

    void StartUp() {
        BvdSimSimplePeerSystemMutBaseClass::StartUp();
    }

};

class BvdSimSystemTestPeerSPConcurrentIoBaseClass: public BvdSimSystemTestPeerSPBaseClass, public BvdSimMutConcurrentIoLoad {
public:

    BvdSimSystemTestPeerSPConcurrentIoBaseClass(PAbstractDriverReference driverConfig = NULL)
    : BvdSimSystemTestPeerSPBaseClass(driverConfig), BvdSimMutConcurrentIoLoad(driverConfig) { }

    virtual void Test() {
        BvdSimMutConcurrentIoLoad::Test();
    }
};


class BvdSim3LUnitTestSetup : public BvdSimUnitTestSetup {
public:
    BvdSim3LUnitTestSetup(int argc , char ** argv, char const *name)
                       : BvdSimUnitTestSetup( argc, argv, name) { }

/* CGCG - had to move this to avoid breaking the EmcUTIL Shell mechanism */
#if 0
     void *operator new (size_t size) throw (const char *){
         void * p = malloc(size);
         if (p == 0) throw "allocation failure"; //instead of std::bad_alloc
         return p;
     }
    void operator delete (void *p){
        CacheUnitTestSetup * pc = static_cast<CacheUnitTestSetup*>(p);
        free(p);
    }
#endif
    
    virtual void init_mut() {
        BvdSimUnitTestSetup::init_mut();
        mut_isolate_tests(TRUE); // all 3 Legged tests need to run in their own process
    }
};



class BvdSim3LPeerSPTestSetup : public BvdSimPeerSPTestSetup {
public:
    BvdSim3LPeerSPTestSetup(int argc , char ** argv, char const *name)
                       : BvdSimPeerSPTestSetup(argc, argv, name) { }

    virtual void init_mut() {
        BvdSimPeerSPTestSetup::init_mut();
        mut_isolate_tests(TRUE); // all 3Legged tests need to run in their own process
    }
};


/*
 * All  3Legged stool control processes test classes should
 * inherit from BvdSim3LControlRemoteSystemTestBaseClass
 */
class BvdSim3LControlRemoteSystemTestBaseClass: public BvdSimMutRemoteSystemTestBaseClass {
public:
    BvdSim3LControlRemoteSystemTestBaseClass(PAbstractDriverReference driverReference = NULL)
    : BvdSimMutRemoteSystemTestBaseClass(driverReference) {
        /*
         * To date, existing 3legged tests are choosing
         * to explicitly manage booting SP processes
         */
        setTestCapability(BvdSim_Capability_AutoLaunchSPA, FALSE);
        setTestCapability(BvdSim_Capability_AutoLaunchSPB, FALSE);
    }

    virtual void StartUp() {
        BvdSimMutRemoteSystemTestBaseClass::StartUp();

        /* 
         * grab the Nvram and CMM shared memory.
         * This ensures that the shared memory segments
         * will not be released until the control process exits
         */
        BvdSim_Trace(MUT_LOG_HIGH, "BvdSim3LControlRemoteSystemTestBaseClass: Control leg holds PersistentMemory\n");
        if(getTestCacheSizeBytes()) {
            BvdSim_Trace(MUT_LOG_HIGH, "BvdSim3LControlRemoteSystemTestBaseClass: Control leg requests %d MB Cache\n", (ULONG) (getTestCacheSizeBytes()/(UINT_64) 1024*1024));
            PersistentCMMMemory::configure_CMM(PersistentMemory_Split, getTestCacheSizeBytes());
        }
        else if(getTestCapability(BvdSim_Capability_1GB_CMMMemory)) {
            PersistentCMMMemory::configure_CMM(PersistentMemory_Split, ((UINT_64) 1024 * 1024 * 1024));
            BvdSim_Trace(MUT_LOG_HIGH, "BvdSim3LeControlRemoteSystemTestBaseClass: Control leg requests 1GB Cache\n");
        }

        // Shared memory and nvram need to be initialized during the control leg startup and should not be
        // released until the control leg teardowwn otherwise the SPA/SPB processes will not properly
        // support the Cache memory model.
        PersistentNvRamMemory::getNvRamMemory();
        PersistentCMMMemory::getInstance();
    }


    virtual void Test() = 0;

};

#endif // _BVDSIM3LMUTBASECLASS_
