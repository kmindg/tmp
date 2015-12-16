/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               FctSimUtil.h
 ***************************************************************************
 *
 * DESCRIPTION:  Utilities for managing Fast Cache for tests both inside and
 *              outside of Fast Cache structure
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/04/2012  CHH Initial Version
 *
 **************************************************************************/

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "simulation/BvdSimMutBaseClass.h"

#include "BasicLib/BasicConsumedDevice.h"
#include "../FabricArray/Fct/interface/FctAdminTypes.h"
#include "../FabricArray/Fct/interface/FctSimRAP.h"
#include "Cache/CacheDriverParams.h"
#include "simulation/IOSectorManagement.h"
#include "../FabricArray/Fct/interface/FctTypes.h"
#include "../FabricArray/Fct/interface/FctSmDefines.h"

#define FCT_SIMUTIL_MAX_LUN_NUMBER_DEFAULT 255

static const char TestEfdCacheDeviceValidWwn[4] = {0x60, 0x06, 0x01, 0x60};

class FctSimUtil {
public:
    FctSimUtil(): mpBaseClass(NULL),
                  mStartCacheLun(FCT_SIMUTIL_MAX_LUN_NUMBER_DEFAULT),
                  mCacheLunSectorSize(FCT_DEFAULT_SECTOR_SIZE),
                  mNonHEDevices(false)
    {
    }

    static const UINT_32 FCT_DEFAULT_SECTOR_SIZE = FIBRE_SECTORSIZE;

    void Initialize(BvdSimMutBaseClass * pBaseClass)
    {
        mpBaseClass = pBaseClass;
        
        mDevice.SetDeviceObjectName("FctFactory");
        EMCPAL_STATUS Status = mDevice.OpenConsumedDevice();
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        mRequest = BasicIoRequest::Allocate(mDevice.GetStackSize());
    }

    // CreateFastCache(CacheDeviceCnt, NonHEDevices) - is expected to be used
    // to create a Fast Cache for all tests, unless the test is trying to test
    // different sector sizes or LUN numbering or expecting the create to fail.
    // Default is one caching high-endurance device.
    //
    void CreateFastCache(int CacheDeviceCnt = 1, bool NonHEDevices = false)
    {
        FF_ASSERT(0 != CacheDeviceCnt);
        mNonHEDevices = NonHEDevices;
        SetupCache(CacheDeviceCnt, FCT_VALID);
        FF_ASSERT(EnableCache());
    }
    void CreateFastCache(unsigned int CacheDeviceCnt, UINT_32 SectorSize,
                         unsigned int StartLunNumber)
    {
        FF_ASSERT((SECTORSIZE == SectorSize) ||
                  (FIBRE_SECTORSIZE == SectorSize));
        FF_ASSERT((FCT_SIMUTIL_MAX_LUN_NUMBER_DEFAULT >= StartLunNumber) &&
                  ((4 * CacheDeviceCnt) <= StartLunNumber));

        mCacheLunSectorSize = SectorSize;
        mStartCacheLun = StartLunNumber;
        CreateFastCache(CacheDeviceCnt);
    }
    bool CreateFastCacheFailOk(unsigned int CacheDeviceCnt)
    {
        FF_ASSERT(0 != CacheDeviceCnt);
        SetupCache(CacheDeviceCnt, FCT_VALID);
        return (EnableCache());
    }

    // DestroyFastCache
    //
    void DestroyFastCache()
    {
        EFDCacheDriverParams DriverParams;

        DisableCache();

        GetDriverParams(&DriverParams);

        for (int i = 0; i < FCT_MAX_CACHE_DEVICES; i++)
        {
            if (DriverParams.CachingObjects[i].State != FCT_INVALID)
            {
                DeleteCacheDevice(i);
            }
        }
    }

    // ExpanFastCacheNoWait - is expected to only be used when needing to have
    // other actions performed (e.g., take the device offline) while trying to
    // expand a Fast Cache. This routine expands the Fast Cache by 1 device.
    //
    void ExpandFastCacheNoWait(unsigned int StartLunNum = 0)
    {
        if (0 != StartLunNum)
        {
            mStartCacheLun = StartLunNum;
        }
        SetupCache(1, FCT_EXPAND);
    }
    void ExpandFastCache(int CacheDeviceCnt, bool ExpectFail = false)
    {
        EFDCacheDriverStatus DriverStatus;
        EFDCacheDriverParams DriverParams;

        for (int cnt = 0; cnt < CacheDeviceCnt; cnt++)
        {
            bool wait_done = false;
            bool failed = false;

            ExpandFastCacheNoWait();
            GetDriverParams(&DriverParams);

            // Total max wait of two minutes.
            for (int i = 0; i < 240 && !wait_done; i++)
            {
                bool check_devices = true;

                GetDriverStatus(&DriverStatus);

                for (int j = 0; j < FCT_MAX_CACHE_DEVICES && check_devices; j++)
                {
                    if (DriverParams.CachingObjects[j].State != FCT_INVALID)
                    {
                        FctDeviceReportedStates state;
                        state = DriverStatus.DeviceStatus[j].State;

                        if (FCT_EXPANSION_FAILED == state)
                        {
                            failed = true;
                        }
                        else if (FCT_DEVICE_DEGRADED == state || FCT_DEVICE_OK == state)
                        {
                            check_devices = true ;
                        }
                        else 
                        {
                            check_devices = false;
                        }
                    }
                }
                if (check_devices)
                {
                    wait_done = true;
                }
                else
                {
                    // Pause for half a second each time through the loop to
                    // give other threads a chance to run.
                    EmcpalThreadSleep(500);
                }
            }
            if (wait_done)
            {
                FF_ASSERT((ExpectFail) ? failed : ! failed);
            }
            else
            {
                FF_ASSERT(0); // Did not reach terminal state in allotted time.
            }
        }
    }

    // ShrinkFastCache - will remove the specified device from the list of
    // "active/usable" Fast Cache devices.
    //
    // Note: when this routine returns, although the device will no longer be
    // "active", the device will still be in the list of cache devices; to
    // remove it completely, the DeleteCacheDevice routine should be used.
    //
    void ShrinkFastCache(unsigned short DeviceIndex,
                         unsigned short timeoutInSec = 120)
    {
        ShrinkFastCacheNoWait(DeviceIndex);
        WaitForShrink(DeviceIndex, timeoutInSec);
    }
    // SrinkFastCacheNoWait - is similar to ShrinkFastCache except the routine
    // does not wait for the Fct driver to fully process the "shrink". Thereby,
    // allowing the caller to perform other actions in parallel with the shrink
    // processing.
    //
    void ShrinkFastCacheNoWait(unsigned short DeviceIndex)
    {
        EFDCacheDriverParams DriverParams;
        
        FF_ASSERT(DeviceIndex < FCT_MAX_CACHE_DEVICES);

        GetDriverParams(&DriverParams);
        FF_ASSERT(DriverParams.CachingObjects[DeviceIndex].State == FCT_VALID);
        DriverParams.CachingObjects[DeviceIndex].State = FCT_SHRINK;
        SetDriverParams(&DriverParams);
    }
    void WaitForShrink(unsigned short deviceId,
                       unsigned short timeoutInSec = 600)
    {
        EFDCacheDriverStatus DriverStatus;
        bool printShrinking = true;
        int timeoutLoopCount;

        timeoutLoopCount = (int)timeoutInSec * 2; // pause half second in loop

        for (int i = 0; i < timeoutLoopCount; i++)
        {
            GetDriverStatus(&DriverStatus); 

            if ((DriverStatus.DeviceStatus[deviceId].State == FCT_SHRINKING) &&
                (printShrinking))
            {    
                KvPrint("FCT_SHRINKING TEST: device state become FCT_SHRINKING");
                printShrinking = false;
            }
            if (DriverStatus.DeviceStatus[deviceId].State == FCT_SHRINK_DONE)
            {    
                KvPrint("FCT_SHRINKING TEST: device state become FCT_SHRINK_DONE");
                return;
            }
            // Pause for half a second each time through to give other threads
            // a chance.
            EmcpalThreadSleep(500);
        }
        FF_ASSERT(0); // Did not reach terminal state in allotted time.
    }

    // DeleteCacheDevice - deletes a Fast Cache device
    //
    void DeleteCacheDevice(unsigned short DeviceIndex)
    {
        K10_LU_ID Dir_A_Key;
        K10_LU_ID Dir_B_Key;
        K10_LU_ID Data_A_Key;
        K10_LU_ID Data_B_Key;
        EFDCacheDriverStatus DriverStatus;
        EFDCacheDriverParams DriverParams;
        EFDC_CACHING_OBJECT *DeviceObject;
        
        GetDriverStatus(&DriverStatus);
        GetDriverParams(&DriverParams);
        DeviceObject = &DriverParams.CachingObjects[DeviceIndex];

        // Save away which LUNs are being used, so they can be removed later
        Dir_A_Key = DeviceObject->Dir_A_Key;
        Dir_B_Key = DeviceObject->Dir_B_Key;
        Data_A_Key = DeviceObject->Data_A_Key;
        Data_B_Key = DeviceObject->Data_B_Key;

        DeviceObject->State = FCT_INVALID;

        // If we are disabled, removing a device means the memory allocation
        // state becomes invalid.  If we are remaining enabled then we are
        // removing a failed expansion or a completed shrink so the memory
        // state needs to remain intact.

        if (DriverStatus.InternalState == EFDC_CACHE_STATE_DISABLED)
        {
            DriverParams.MemAllocState = EFDC_CACHE_NO_MEM_ALLOC;
        }

        DeviceObject->Dir_A_Key = K10_WWID_Zero();
        SetDriverParams(&DriverParams);

        DeviceObject->Dir_B_Key = K10_WWID_Zero(); 
        SetDriverParams(&DriverParams);
                
        DeviceObject->Data_A_Key = K10_WWID_Zero(); 
        SetDriverParams(&DriverParams);

        DeviceObject->Data_B_Key = K10_WWID_Zero();
        SetDriverParams(&DriverParams);

        // Final step to complete the deletion is to zero out the WWNs
        DeviceObject->Dir_A_WWN = K10_WWID_Zero(); 
        DeviceObject->Dir_B_WWN = K10_WWID_Zero(); 
        DeviceObject->Data_A_WWN = K10_WWID_Zero(); 
        DeviceObject->Data_B_WWN = K10_WWID_Zero(); 
        SetDriverParams(&DriverParams);

        WaitForCacheDeviceDeleted(DeviceIndex);
    }


    void SetCacheDeviceDegraded(unsigned short DevIndex,
                                VFLT_SPIdentity whichSp = VFLT_BothSP)
    {
        ChangeCacheDeviceState(DevIndex, FctHw_DEGRADED, true, whichSp);
    }

    void ClearCacheDeviceDegraded(unsigned short DevIndex,
                                  VFLT_SPIdentity whichSp = VFLT_BothSP)
    {
        ChangeCacheDeviceState(DevIndex, FctHw_DEGRADED, false, whichSp);
    }

    void SetCacheDeviceBroken(unsigned short DevIndex,
                          VFLT_SPIdentity whichSp = VFLT_BothSP)
    {
        ChangeCacheDeviceState(DevIndex, FctHw_BROKEN, true, whichSp);
    }

    void ClearCacheDeviceBroken(unsigned short DevIndex,
                                VFLT_SPIdentity whichSp = VFLT_BothSP)
    {
        ChangeCacheDeviceState(DevIndex, FctHw_BROKEN, false, whichSp);
    }

    ULONGLONG GetTrashedPagesCount()
    {
        PVOID pFunc = getSharedSymbol("Fct", "get_Fct_Cache_PagesTrashed_Cnt");
        ULONGLONG (*dr_lookup)() = (ULONGLONG (*)())pFunc;
        if (dr_lookup != NULL)
        {
            return (*dr_lookup)();
        }
        mut_printf(MUT_LOG_HIGH, "GetTrashedPagesCount symbol not found");
        FF_ASSERT(0);
        return 0;
    }

protected:

    void RegisterCachingLuns(int numCacheSets)
    {
        int i = 0;

        // Check for valid input
        FF_ASSERT(numCacheSets <= FCT_MAX_CACHE_DEVICES);

        for (i = 0; i < numCacheSets; i++)
        {
            SetupCachingLun(mStartCacheLun--, FCT_SIMMODE_DATA_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            SetupCachingLun(mStartCacheLun--, FCT_SIMMODE_DATA_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            SetupCachingLun(mStartCacheLun--, FCT_SIMMODE_DIR_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            SetupCachingLun(mStartCacheLun--, FCT_SIMMODE_DIR_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
        }
    }

private:

    // SetupCache - makes ioctl calls to the driver to configure a Fast Cache.
    // The flow of the routine is intended to mimic how the "Control Path" does
    // the configuration.  See K10EFDCacheCompleteObjects::GetCreateWorklist in
    // K10EFDCacheObjects.cpp
    void SetupCache(int numCacheSets, FctDeviceConfigStates stateType)
    {
        int i, j;
        EFDCacheDriverParams DriverParams;

        // Check for valid input
        FF_ASSERT(numCacheSets <= FCT_MAX_CACHE_DEVICES);
        
        GetDriverParams(&DriverParams);

        if (mNonHEDevices) {
            DriverParams.CacheConfigInfo |= EFDC_CACHE_NON_HE_DEVICES;
        } else {
            DriverParams.CacheConfigInfo &= ~EFDC_CACHE_NON_HE_DEVICES;
        }

        for (i = 0; i < numCacheSets; i++)
        {
            for (j = 0; j < FCT_MAX_CACHE_DEVICES; j++)
            {
                if (DriverParams.CachingObjects[j].State == FCT_INVALID)
                {break;}
            }
            // First operation sets all the Real WWNs.
            memcpy(&DriverParams.CachingObjects[j].Dir_A_WWN, TestEfdCacheDeviceValidWwn, 4); 
            memcpy(&DriverParams.CachingObjects[j].Dir_B_WWN, TestEfdCacheDeviceValidWwn, 4); 
            memcpy(&DriverParams.CachingObjects[j].Data_A_WWN, TestEfdCacheDeviceValidWwn, 4); 
            memcpy(&DriverParams.CachingObjects[j].Data_B_WWN, TestEfdCacheDeviceValidWwn, 4); 
            DriverParams.CachingObjects[j].Data_A_Key = K10_WWID_Zero();
            DriverParams.CachingObjects[j].Data_B_Key = K10_WWID_Zero();
            DriverParams.CachingObjects[j].Dir_A_Key = K10_WWID_Zero();
            DriverParams.CachingObjects[j].Dir_B_Key = K10_WWID_Zero();
            SetDriverParams(&DriverParams);

            // Each adminm processed 'Consume' operation sends in the Key we
            // use for the specified caching LUN (four total). With the last
            // last one, the slot's state gets set to the desired value.

            SetupCachingLun(mStartCacheLun, FCT_SIMMODE_DATA_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            GenerateKeyFromLun(mStartCacheLun,
                               DriverParams.CachingObjects[j].Data_A_Key);
            mStartCacheLun--;
            SetDriverParams(&DriverParams);

            SetupCachingLun(mStartCacheLun, FCT_SIMMODE_DATA_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            GenerateKeyFromLun(mStartCacheLun,
                               DriverParams.CachingObjects[j].Data_B_Key);
            mStartCacheLun--;
            SetDriverParams(&DriverParams);

            SetupCachingLun(mStartCacheLun, FCT_SIMMODE_DIR_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            GenerateKeyFromLun(mStartCacheLun,
                               DriverParams.CachingObjects[j].Dir_A_Key);
            mStartCacheLun--;
            SetDriverParams(&DriverParams);

            SetupCachingLun(mStartCacheLun, FCT_SIMMODE_DIR_LUN_SIZE_SECTORS,
                            mCacheLunSectorSize);
            GenerateKeyFromLun(mStartCacheLun,
                               DriverParams.CachingObjects[j].Dir_B_Key); 
            mStartCacheLun--;
            DriverParams.CachingObjects[j].State = stateType;
            SetDriverParams(&DriverParams);
        }
    }
    void SetupCachingLun(int Lun, unsigned long Size, int SectorSize)
    {
        mpBaseClass->registerDiskConfiguration(Lun, new DiskIdentity(PersistentDisks_DiskTypeSSD, Size, SectorSize));
        mpBaseClass->requestDeviceCreation(Lun, 0, PersistentDisks_DiskTypeSSD);
    }

    // ChangeCacheDeviceState - changes the HW state for all the LUNs associated
    // with a Fast Cache device.
    // Inputs:
    //    devIndex - which Fast Cache device to change (as identified by its
    //               index into the array of devices maintained by the driver
    //    state - the HW state (e.g., broken, degraded) to change  
    //    setState - indicates whether (true) to set the state or (false) clear
    //               the state
    //    whichSP - indicates which SP(s) are to see the state change
    //
    void ChangeCacheDeviceState(unsigned short devIndex, enum FctHwState state,
                                bool setState, VFLT_SPIdentity whichSp)
    {
        ULONG Lun;
        EFDCacheDriverParams DriverParams;
        void (*changeFunc)(UINT_32, VFLT_SPIdentity);

        FF_ASSERT(devIndex < FCT_MAX_CACHE_DEVICES);

        if (FctHw_BROKEN == state)
            if (setState)
                changeFunc = &VolumeDataTable_MarkVolumeNotReady;
            else
                changeFunc = &VolumeDataTable_MarkVolumeReady;
        else if (FctHw_DEGRADED== state)
            if (setState)
                changeFunc = &VolumeDataTable_MarkVolumeDegraded;
            else
                changeFunc = &VolumeDataTable_MarkVolumeNotDegraded;
         else
            FF_ASSERT(0); // bad caller

        GetDriverParams(&DriverParams);

        Lun = GetLunFromKey(&DriverParams.CachingObjects[devIndex].Data_A_Key);
        (*changeFunc)(Lun, whichSp);

        EmcpalThreadSleep(500);
        Lun = GetLunFromKey(&DriverParams.CachingObjects[devIndex].Data_B_Key);
        (*changeFunc)(Lun, whichSp);

        EmcpalThreadSleep(500);
        Lun = GetLunFromKey(&DriverParams.CachingObjects[devIndex].Dir_A_Key);
        (*changeFunc)(Lun, whichSp);

        EmcpalThreadSleep(500);
        Lun = GetLunFromKey(&DriverParams.CachingObjects[devIndex].Dir_B_Key);
        (*changeFunc)(Lun, whichSp);

        // Delay 0.5 second to allow for change notification to be received
        EmcpalThreadSleep(500);
    }

    
protected:

    bool EnableCache()
    {
        EMCPAL_STATUS Status;
        int i = 0;
        EFDCacheDriverStatus DriverStatus;
        EFDCacheDriverParams DriverParams;
        FctAction   Action;
        
        // Set the configured state and mode
        GetDriverParams(&DriverParams);
        
        DriverParams.ConfiguredState = EFDC_CACHE_STATE_ENABLED;
        DriverParams.ConfiguredMode = EFDC_CACHE_MODE_READ_WRITE;
        
        SetDriverParams(&DriverParams);

        // Enable the cache
        Action.ActionKind = BasicDriverAction::ActionThisSP;
        Action.Op = FCT_ACTION_ENABLE;
        
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_PERFORM_DRIVER_ACTION, 
            &Action, sizeof(FctAction), NULL, 0);

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        // Wait for cache to come enabled
        do
        {
            GetDriverStatus(&DriverStatus);

            // Check if cache is disabled faulted 
            if (DriverStatus.InternalState == EFDC_CACHE_STATE_DISABLED_FAULTED)
            {
                mut_printf(MUT_LOG_HIGH,"Fast Cache is disabled faulted, failed to go enabled.\n");
                return(false);
            }

            EmcpalThreadSleep(500);  // Pause for half a second each time through to give other threads a chance
            i++;

        }while ((DriverStatus.InternalState != EFDC_CACHE_STATE_ENABLED) &&
                (i < 240));  // Only wait for two minutes

        FF_ASSERT(i < 240);  // This number may need to be modified if large caches take too long
        return(true);
    }

    void WaitForCacheEnabled()
    {
        WaitForCacheState(EFDC_CACHE_STATE_ENABLED);
    }

    void WaitForCacheDegraded()
    {
        WaitForCacheState(EFDC_CACHE_STATE_ENABLED_DEGRADED);
    }

    void WaitForCacheState(EFDC_CACHE_STATE DesiredState)
    {
        int i = 0;
        EFDCacheDriverStatus DriverStatus;

        // Wait for cache to come enabled
        do
        {
            GetDriverStatus(&DriverStatus);
            EmcpalThreadSleep(500);  // Pause for half a second each time through to give other threads a chance
            i++;

        }while ((DriverStatus.InternalState != DesiredState) &&
                (i < 240));  // Only wait for two minutes

        FF_ASSERT(i < 240);  // This number may need to be modified if large caches take too long

    }   

    void WaitForCacheClean()
    {
        int i = 0;
        EFDCacheDriverStatus DriverStatus;


        // Wait for cache to go clean
        do
        {
            GetDriverStatus(&DriverStatus);
            if (i==0)
            {
                mut_printf(MUT_LOG_HIGH, "Start waiting for Fast Cache to go clean, total element %d, dirty elements %d\n", (int)DriverStatus.TotalElements, (int)DriverStatus.DirtyElements);
            }
            EmcpalThreadSleep(500);  // Pause for half a second each time through to give other threads a chance
            i++;

            mut_printf(MUT_LOG_HIGH, "Waiting for Fast Cache to go clean, pass %d, dirty elements %d\n", i, (int)DriverStatus.DirtyElements);
        } while ((DriverStatus.DirtyElements != 0) && (i < 1200));  // Quit waiting after ten minutes

        FF_ASSERT(i < 1200); // This number may need to be modified if cache cleaning takes too long
    }

    void WaitForCachePromotes(ULONG Lun, ULONGLONG cnt)
    {
        int i = 0;
        ULONGLONG TotalPromoted; 

        // Wait for cache to promote pages. Pause for half a second each time
        // through to give other threads a chance. Quit waiting after 2 minutes.
        // This 2 min may need to be modified if cache promotes take too long.

        for (i=0; i < 240; i++)
        {
            TotalPromoted = GetPromoteCount(Lun);

            if (TotalPromoted >= cnt) break;

            mut_printf(MUT_LOG_HIGH, "Waiting for Fast Cache to promote %llu for lun %lu, pass %d, total promoted %llu\n", (unsigned long long)cnt, (unsigned long)Lun, i, (unsigned long long)TotalPromoted);

            EmcpalThreadSleep(500); 
        }
        FF_ASSERT(i < 240);
        mut_printf(MUT_LOG_HIGH, "WaitForCachePromotes: Fast Cache promoted %llu for lun %lu\n", (unsigned long long)TotalPromoted, (unsigned long)Lun);
    }

    void WaitForCacheDirty(ULONG Lun, ULONG cnt)
    {
        int i = 0;
        ULONGLONG TotalDirtied; 
        FctVolumeStats VolStats;

        // Wait for cache to dirty pages
        do
        {
            VolStats = GetVolumeStat(Lun);

            TotalDirtied = VolStats.mStatsNumDirtyElements + 
                           VolStats.mStatsNumIdleCleaned +
                           VolStats.mStatsNumIdleFlushed;
            if (i==0)
            {
                mut_printf(MUT_LOG_HIGH, "Start waiting for Fast Cache to dirty, total dirtied %ld\n", (long)TotalDirtied);
            }
            EmcpalThreadSleep(500);  // Pause for half a second each time through to give other threads a chance
            i++;

            mut_printf(MUT_LOG_HIGH, "Waiting for Fast Cache to dirty, pass %d, total dirtied %ld\n", i, (long)TotalDirtied);
        } while ((TotalDirtied< cnt) && (i < 120));  // Quit waiting after 2 minutes

        FF_ASSERT(i < 120); // This number may need to be modified if cache dirties take too long
    }

    void ForceDirectoryUpdateFailure(FctTrackerType type = FctTrackerTypeDm)
    {
        FctAction Action;

        // Force directory update failure in FCT for testing
        Action.ActionKind = BasicDriverAction::ActionThisSP;
        Action.Op = FCT_ACTION_FORCE_DIR_UPDATE_FAILURE;
        Action.type.DirUpdateFailure.trackerType = type;

        PerformDriverAction(&Action);
    }

    void EnableWritePinPageFailure()
    {
        // Enable Write Pin Page failures in FCT for testing
        PerformLocalDriverAction(FCT_ACTION_ENABLE_WRITE_PIN_PAGE_FAILURE);
    }

    void DisableWritePinPageFailure()
    {
        // Disable Write Pin Page failures in FCT for testing
        PerformLocalDriverAction(FCT_ACTION_DISABLE_WRITE_PIN_PAGE_FAILURE);
    }

    EMCPAL_STATUS CheckPagePromoted(ULONG Volume, DiskSectorCount Lba)
    {
        EMCPAL_STATUS Status;
        FctAction Action;

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_CHECK_PAGE_PROMOTED;
        Action.type.TestLunAndBlock.Volume = Volume;
        Action.type.TestLunAndBlock.Lba = Lba;

        PerformDriverAction(&Action, &Status);
        return Status;
    }

    EMCPAL_STATUS CheckPageNotPromoted(ULONG Volume, DiskSectorCount Lba)
    {
        EMCPAL_STATUS Status;
        FctAction Action;

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_CHECK_PAGE_NOT_PROMOTED;
        Action.type.TestLunAndBlock.Volume = Volume;
        Action.type.TestLunAndBlock.Lba = Lba;

        PerformDriverAction(&Action, &Status);
        return Status;
    }

    void DisableDynamicOverProvisioning()
    {
        FctAction Action;

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_ENABLE_DISABLE_DYNAMIC_OVERPROVISIONING;
        Action.type.DynamicOverProvisioning.disable = 1;

        PerformDriverAction(&Action);
    }

    void SetDeviceWearInfo(UINT_8 devIndex, ULONG eolPECount,
                           ULONG currentPECount, ULONG currentPOH)
    {
        FctAction Action;

        FF_ASSERT(devIndex < FCT_MAX_CACHE_DEVICES);

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_SET_DEVICE_WEAR_INFO;
        Action.type.DeviceWearInfo.deviceId = devIndex;
        Action.type.DeviceWearInfo.devEolPECount = eolPECount;
        Action.type.DeviceWearInfo.devPECount = currentPECount;
        Action.type.DeviceWearInfo.devPOH = currentPOH;

        PerformDriverAction(&Action);
    }

    void DisablePriorityPromotes()
    {
        PerformDriverActionAllSPs(FCT_ACTION_DISABLE_PRIORITY_PROMOTES);
    }

    void EnablePriorityPromotes()
    {
        PerformDriverActionAllSPs(FCT_ACTION_ENABLE_PRIORITY_PROMOTES);
    }

    void PerformDriverAction(FctAction * pAction,
                             EMCPAL_STATUS * pStatus = NULL)
    {
        EMCPAL_STATUS Status;

        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0,
                                IOCTL_BVD_PERFORM_DRIVER_ACTION, 
                                pAction, sizeof(FctAction), NULL, 0);
        
        Status = mDevice.SyncCallDriver(mRequest);

        if (pStatus)
        {
            // Caller wants to handle the status.
            *pStatus = Status;
        }
        else
        {
            FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
        }
    }

    void PerformDriverActionAllSPs(FCT_ACTION_CODE op,
                                   EMCPAL_STATUS * pStatus = NULL)
    {
        FctAction Action;

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = op;
        PerformDriverAction(&Action, pStatus);
    }

    void PerformLocalDriverAction(FCT_ACTION_CODE op,
                                  EMCPAL_STATUS * pStatus = NULL)
    {
        FctAction Action;
        
        Action.ActionKind = BasicDriverAction::ActionThisSP;
        Action.Op = op;
        PerformDriverAction(&Action, pStatus);
    }

    void DisableIdleCleaner()
    {
        PerformLocalDriverAction(FCT_ACTION_DISABLE_IDLE_CLEANER);
    }

    void EnableIdleCleaner()
    {
        PerformLocalDriverAction(FCT_ACTION_ENABLE_IDLE_CLEANER);
    }

    void EnableForceDoNotFlush()
    {        
        PerformLocalDriverAction(FCT_ACTION_ENABLE_FORCE_DO_NOT_FLUSH);
    }

    void DisableForceDoNotFlush()
    {        
        PerformLocalDriverAction(FCT_ACTION_DISABLE_FORCE_DO_NOT_FLUSH);
    }

    void EnableSuspendDeviceResize(UINT_8 suspendFlags,
                                   INT_16 devID = -1,
                                   INT_32 elementIndex = -1)
    {        
        FctAction Action;

        FF_ASSERT((devID < FCT_MAX_CACHE_DEVICES) && (devID >= -1));
        FF_ASSERT((elementIndex >= -1));

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_ENABLE_DISABLE_SUSPEND_DEVICE_RESIZE;
        Action.type.SuspendDeviceResize.enableSuspend = 1;
        Action.type.SuspendDeviceResize.suspendFlags = suspendFlags;
        Action.type.SuspendDeviceResize.deviceId = devID;
        Action.type.SuspendDeviceResize.elementIndex = elementIndex;

        PerformDriverAction(&Action);
    }

    void DisableSuspendDeviceResize()
    {        
        FctAction Action;

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_ENABLE_DISABLE_SUSPEND_DEVICE_RESIZE;
        Action.type.SuspendDeviceResize.enableSuspend = 0;
        Action.type.SuspendDeviceResize.suspendFlags = 0; // unused
        Action.type.SuspendDeviceResize.deviceId = 0; // unused
        Action.type.SuspendDeviceResize.elementIndex = 0; // unused

        PerformDriverAction(&Action);
    }

    void ValidateAllCacheDeviceUnusedElements()
    {        
        PerformLocalDriverAction(FCT_ACTION_VALIDATE_UNUSED_DEVICE_ELEMENTS);
    }

    void ForceDirWriteFailure()
    {
        PerformLocalDriverAction(FCT_ACTION_FORCE_DIR_WRITE_FAILURE);
    }


    void EnableReadCorruptData()
    {        
        PerformLocalDriverAction(FCT_ACTION_ENABLE_READ_CORRUPT_DATA);
    }

    void DisableReadCorruptData()
    {        
        PerformLocalDriverAction(FCT_ACTION_DISABLE_READ_CORRUPT_DATA);
    }

    void DisableCache(EMCPAL_STATUS * pStatus = NULL)
    {
        int i = 0;
        EFDCacheDriverStatus DriverStatus;
        EFDCacheDriverParams DriverParams;
        
        // Set the configured state
        GetDriverParams(&DriverParams);
        
        DriverParams.ConfiguredState = EFDC_CACHE_STATE_DISABLED;
        DriverParams.MemAllocState = EFDC_CACHE_NO_MEM_ALLOC;
        
        SetDriverParams(&DriverParams, pStatus);

        if (pStatus && !EMCPAL_IS_SUCCESS(*pStatus))
        {
            // We've gotten bad status and the caller wants to handle
            return;
        }

        // Disable the cache
        PerformLocalDriverAction(FCT_ACTION_DISABLE, pStatus);
        
        if (pStatus && !EMCPAL_IS_SUCCESS(*pStatus))
        {
            // We've gotten bad status and the caller wants to handle it
            return;
        }

        // Wait for cache to come disabled
        do
        {
            GetDriverStatus(&DriverStatus);
            EmcpalThreadSleep(500);  // Pause for half a second each time through to give other threads a chance
            i++;

        }while ((DriverStatus.InternalState != EFDC_CACHE_STATE_DISABLED) &&
                (i < 1200));  // Quit waiting after ten minutes

        FF_ASSERT(i < 1200); // This number may need to be modified if cache flushes take too long

        if (pStatus)
        {
            *pStatus = EMCPAL_STATUS_SUCCESS;
        }

    }

    void Shutdown()
    {
        EMCPAL_STATUS Status;

        // Use a local request shutdownRequest to let a shutdown IOCTL be outstanding with another
        // IOCTL concurrently.  This allows for testing of a shutdown while other type requests are in progress.
        BasicIoRequest * shutdownRequest = BasicIoRequest::Allocate(mDevice.GetStackSize());

        shutdownRequest ->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        shutdownRequest ->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_SHUTDOWN_WARNING); 
        Status = mDevice.SyncCallDriver(shutdownRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        shutdownRequest ->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        shutdownRequest ->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_SHUTDOWN); 
        Status = mDevice.SyncCallDriver(shutdownRequest);
        mut_printf(MUT_LOG_HIGH, "BVD shutdown completed, status 0x%x\n", Status);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        shutdownRequest->Free();
    }

    void EnableShutdownTest()
    {        
        PerformLocalDriverAction(FCT_ACTION_ENABLE_SHUTDOWN_TEST);
    }

    void CleanoutAnyPriorCache()
    {
        // TODO: 255 is an arbitrary number, see SetupCache which constructs luns starting at 255
        int Lun = FCT_SIMUTIL_MAX_LUN_NUMBER_DEFAULT;  // Start with LUN 255 and work down

        for (int i = 0; i < FCT_MAX_CACHE_DEVICES; i++)
        {
            for (int j = 0; j < 4; j++)  // four LUNs in each caching set
            {
                RemoveLunFile(Lun--);
            }
        }
        /*
         * Also delete the Fct Data repository LUN
         */
        mpBaseClass->deleteDeviceFromFS("FDR");

    }
    
    void WaitForCacheDeviceDeleted(int deviceId)
    {
        EFDCacheDriverStatus DriverStatus;
        do
        {
            GetDriverStatus(&DriverStatus); 

            if (DriverStatus.DeviceStatus[deviceId].State == FCT_NO_DEVICE)
            {    
                KvPrint("FCT_SHRINKING TEST: device state removed");
                break;
            }
            EmcpalThreadSleep(500);  // Pause for half a second each time through to give other threads a chance
        }while (true);  // Loop until the cache device: deviceId is deleted
    }
    void SetCacheStateOnLun(ULONG Lun, EFDC_VOL_STATE state)
    {
        EMCPAL_STATUS Status;
        K10EFDCacheVolumeParams VolParams;

        GenerateKeyFromLun(Lun, VolParams.LunWwn);

        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_GET_VOLUME_PARAMS, 
            &VolParams, sizeof(VolumeIdentifier), &VolParams, sizeof(K10EFDCacheVolumeParams));

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        FF_ASSERT((state == EFDC_VOL_STATE_ENABLED) || (state == EFDC_VOL_STATE_DISABLED))
        VolParams.ConfiguredState = state;
        VolParams.SeqNo++;
        
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_SET_VOLUME_PARAMS, 
            &VolParams, sizeof(K10EFDCacheVolumeParams), NULL, 0);

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
    }

    void EnableCacheOnLun(ULONG Lun)
    {
        SetCacheStateOnLun(Lun, EFDC_VOL_STATE_ENABLED);        
    }

    void DisableCacheOnLun(ULONG Lun)
    {
       SetCacheStateOnLun(Lun, EFDC_VOL_STATE_DISABLED);    
    }

    void GetDriverStatus(EFDCacheDriverStatus * pStatus)
    {
        EMCPAL_STATUS Status;
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0,
                                IOCTL_BVD_QUERY_VOLUME_DRIVER_STATUS, 
                                NULL, 0, pStatus, sizeof(EFDCacheDriverStatus));
        Status = (mDevice.SyncCallDriver(mRequest));

        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
    }

    void GetDriverParams(EFDCacheDriverParams * pParams)
    {
        EMCPAL_STATUS Status;
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0,
                                IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS, 
                                NULL, 0, pParams, sizeof(EFDCacheDriverParams));

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
    }
    
    void SetDriverParams(EFDCacheDriverParams * pParams,
                         EMCPAL_STATUS * pStatus = NULL)
    {
        EMCPAL_STATUS Status;
        pParams->SeqNo++;
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0,
                                IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS, 
                                pParams, sizeof(EFDCacheDriverParams), NULL, 0);

        Status = mDevice.SyncCallDriver(mRequest);

        // If pStatus is not NULL, the caller wants to handle the status rather
        // than us assert the IOCTL was "successful".
        if (pStatus)
        {
            *pStatus = Status;
            // If not successful, need to put the SeqNo back so the next call
            // to this routine using the same pParams has the correct value.
            if (EMCPAL_STATUS_SUCCESS != Status) pParams->SeqNo--;
            return;
        }
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
    }

    void WaitForPromoteDelay()
    {EmcutilSleep(10000);}

    bool PromotePage(ULONG Lun, LBA_T Lba, bool IgnoreFailures=false, PROMOTE_FAIL_TYPE FailType=PROMOTE_FAIL_NO_FORCED_FAIL)
    {
        EMCPAL_STATUS Status;
        FctPromoteReq Req;
        
        Req.Lun = Lun;
        Req.Lba = Lba;
        Req.FailType = FailType;

        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_FCT_PROMOTE_PAGE, 
                                &Req, sizeof(FctPromoteReq), NULL, 0);

        Status = mDevice.SyncCallDriver(mRequest);

        if ((!IgnoreFailures) && (EMCPAL_STATUS_SUCCESS != Status))
        {
            mut_printf(MUT_LOG_HIGH, "PromotePage: Bad Status %lx\n", (unsigned long)Status);
            FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
        }

        return ((Status == EMCPAL_STATUS_SUCCESS)?true:false);
    }

    void PromotePagesToCacheAndWait(DiskSectorCount startLba, ULONG lun, ULONG numPages, ULONG totalPages)
    {
        DiskSectorCount promoteLba = startLba;

        for (int i = 0; i < numPages; i++, promoteLba += FctPageSizeInSectors)
        {
            PromotePage(lun, promoteLba);

            // Delay when promote queue nears its max
            // so it can have time to drain.   
            if (i == 60)
            {
                EmcpalThreadSleep(2000);
            }
        }
        WaitForCachePromotes(lun, totalPages);
    }

    ULONGLONG GetPromoteCount(ULONG Lun)
    {
        FctVolumeStats VolStats = GetVolumeStat(Lun);

        return (VolStats.mStatsNumPromotedThisSP +
                VolStats.mStatsNumIdleFlushed +
                VolStats.mStatsNumPromotesFailed +
                VolStats.mStatsNumPromotesRejected);
    }
    //
    // PromotePages - will promote a contiguous set of pages for a given LUN
    // via the driver IOCTL to promote a page.  The routine waits for the
    // requested number pages to be promoted or the specified number of seconds.
    //
    void PromotePages(ULONG Lun, LBA_T StartLba, ULONG NumPages,
                      LONG WaitTimeInSec = -1)
    {
        LBA_T NextLba = StartLba;
        ULONGLONG TotalPromoted = GetPromoteCount(Lun);

        mut_printf(MUT_LOG_HIGH, "PromotePages: StartLba %llu, NumPages %lu, Wait %ld, initial promote count %llu for LUN %lu\n", (unsigned long long)StartLba, (unsigned long)NumPages, (long)WaitTimeInSec, (unsigned long long)TotalPromoted, (unsigned long)Lun);

        for (int i = 0; i < NumPages; i++)
        {
            PromotePage(Lun, NextLba);
            TotalPromoted++;

            if (i % FCT_MAX_PROMOTES_PER_SP == 0)
            {
                // wait for promote queue to drain
                WaitForCachePromotes(Lun, TotalPromoted);
            }
            NextLba += FctPageSizeInSectors;
        }
        if (WaitTimeInSec > 0)
        {
            EmcpalThreadSleep(WaitTimeInSec * 1000);
        }
        else if (WaitTimeInSec < 0)
        {
            WaitForCachePromotes(Lun, TotalPromoted);
        }
    }
    //
    // PromotePagesByWriting - will promote a contiguous set of pages for a
    // given LUN by writing each page 3 times. The routine waits for the
    // requested number of pages to be promoted or the specified number of
    // seconds.
    //
    void PromotePagesByWriting(ULONG Lun, BvdSimFileIo_File *pFile,
                               LBA_T StartLba, ULONG NumPages,
                               LONG WaitTimeInSec = -1)
    {
        ULONGLONG TotalPromoted = GetPromoteCount(Lun);

        mut_printf(MUT_LOG_HIGH, "PromotePagesByWriting: StartLba %llu, NumPages %lu, Wait %ld, initial promote count %llu for LUN %lu\n", (unsigned long long)StartLba, (unsigned long)NumPages, (long)WaitTimeInSec, (unsigned long long)TotalPromoted, (unsigned long)Lun);

        for (int j = 0; j < 3; j ++)
        {
            WritePages(pFile, StartLba, NumPages);
        }
        if (WaitTimeInSec > 0)
        {
            EmcpalThreadSleep(WaitTimeInSec * 1000);
        }
        else if (WaitTimeInSec < 0)
        {
            WaitForCachePromotes(Lun, TotalPromoted);
        }
    }
    //
    // WritePages - will write a contiguous set of pages for a given LUN
    // (represented by the associated simulated LUN file).
    //
    void WritePages(BvdSimFileIo_File *pFile, LBA_T StartLba, ULONG NumPages) 
    {
        LBA_T NextLba = StartLba;

        for (int i = 0; i < NumPages; i++)
        {
            mpBaseClass->validateBlockDCAWriteBlockDCAReadSectors(pFile,
                                                          FctPageSizeInSectors,
                                                          NextLba);
            NextLba += FctPageSizeInSectors;
        }
    }
    void WritePagesSGL(BvdSimFileIo_File *pFile, LBA_T StartLba, ULONG NumPages) 
    {
        LBA_T NextLba = StartLba;
        for (int i = 0; i < NumPages; i++)
        {
            mpBaseClass->validateSGLWriteSGLReadSectors(pFile,
                                                        FctPageSizeInSectors,
                                                        NextLba, 1);
            NextLba += FctPageSizeInSectors;
        }
    }

    bool PinPage(ULONG Lun, LBA_T Lba, PVOID *pinPageHandle, bool IgnoreFailures=false)
    {
        EMCPAL_STATUS Status;
        IoctlCacheReadAndPinInfo info;
        BasicConsumedDevice SPCacheDevice;
            
        SPCacheDevice.SetDeviceObjectName("SPCacheFactory");
        Status = SPCacheDevice.OpenConsumedDevice();
        if (EMCPAL_STATUS_SUCCESS != Status)
        {
            mut_printf(MUT_LOG_HIGH, "Failed to open SPCacheFactory, status 0x%x\n", Status); 
            return false;
        }

        info.Request.LunIndex.Index32 = 0x27;
        info.Request.LunIndex.Index64 = 0x100000000000000;
             
        info.Request.StartLba = Lba;
        info.Request.BlockCnt = FctPageSizeInSectors;
        info.Request.Action = PIN_READ_ONLY; // just pick a pin type don't really care...
        info.Request.ClientProvidedSgl = NULL;
        info.Request.ClientProvidedSglMaxEntries = 0;
        info.Request.ExtendedAction = PIN_EXTENDED_NO_OPERATION;

        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_CACHE_READ_AND_PIN_DATA, 
                                &info.Request, sizeof(info.Request), &info.Response, sizeof(info.Response));

        Status = SPCacheDevice.SyncCallDriver(mRequest);

        if (!IgnoreFailures)
        {
            FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
        }

        *pinPageHandle = info.Response.Opaque0;

        SPCacheDevice.CloseConsumedDevice();

        return ((Status == EMCPAL_STATUS_SUCCESS)?true:false);
    }

    bool UnPinPage(PVOID pinPageHandle, bool IgnoreFailures=false)
    {
        EMCPAL_STATUS Status;
        IoctlCacheUnpinInfo info;
        BasicConsumedDevice SPCacheDevice;
            
        SPCacheDevice.SetDeviceObjectName("SPCacheFactory");
        Status = SPCacheDevice.OpenConsumedDevice();
        if (EMCPAL_STATUS_SUCCESS != Status)
        {
            mut_printf(MUT_LOG_HIGH, "Failed to open SPCacheFactory, status 0x%x\n", Status); 
            return false;
        }

        info.Opaque0 = pinPageHandle;
        info.Action = UNPIN_HAS_BEEN_WRITTEN; 

        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_CACHE_UNPIN_DATA, 
                                &info, sizeof(info), NULL, 0);

        Status = SPCacheDevice.SyncCallDriver(mRequest);

        if (!IgnoreFailures)
        {
            FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
        }

        SPCacheDevice.CloseConsumedDevice();

        return ((Status == EMCPAL_STATUS_SUCCESS)?true:false);
    }

    LONGLONG GetIdleFlushedNum(ULONG Lun)
    {
        FctVolumeStats VolStats = GetVolumeStat(Lun);
        return VolStats.mStatsNumIdleFlushed;
    }

    void DestroyLun(ULONG Lun)
    {
        EMCPAL_STATUS Status;
        K10_WWID luId;
        GenerateKeyFromLun(Lun, luId);

        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_DESTROY_VOLUME,
                                     &luId, sizeof(K10_LU_ID), NULL, 0);

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
    }

    void DisableSPCacheWriteOrPretech(bool disableWriteCache = true, bool disableAutoPrefetch = true)
    {
        BasicConsumedDevice Device;  // factory device
        BasicIoRequest *    Request;   // General purpose IRP
        CacheDriverParams DriverParams;

        //Open the cache driver's factory device
        Device.SetDeviceObjectName("SPCacheFactory");
        EMCPAL_STATUS Status = Device.OpenConsumedDevice();
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        Request = BasicIoRequest::Allocate(Device.GetStackSize());

        // Get current parameters from driver
        Request->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        Request->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS, 
                                NULL, 0,  &DriverParams, sizeof(CacheDriverParams));

        Status = Device.SyncCallDriver(Request);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        // Modify desired items
        DriverParams.setWriteCacheDisable(disableWriteCache);
        DriverParams.setAutoPrefetchDisable(disableAutoPrefetch);

        // Send modified parameters back to driver
        Request->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        Request->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS, 
            &DriverParams, sizeof(CacheDriverParams), NULL, 0);

        Status = Device.SyncCallDriver(Request);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        Device.CloseConsumedDevice();
    }

    void DisableSPCacheWriteOrPrefetch(bool disableWriteCache = true,
                                       bool disableAutoPrefetch = true)
    {
        BasicConsumedDevice Device;  // factory device
        BasicIoRequest *    Request;   // General purpose IRP
        CacheDriverParams DriverParams;

        //Open the cache driver's factory device
        Device.SetDeviceObjectName("SPCacheFactory");
        EMCPAL_STATUS Status = Device.OpenConsumedDevice();
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        Request = BasicIoRequest::Allocate(Device.GetStackSize());

        // Get current parameters from driver
        Request->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        Request->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS,
                                NULL, 0,  &DriverParams, sizeof(CacheDriverParams));

        Status = Device.SyncCallDriver(Request);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        // Modify desired items
        DriverParams.setWriteCacheDisable(disableWriteCache);
        DriverParams.setAutoPrefetchDisable(disableAutoPrefetch);

        // Send modified parameters back to driver
        Request->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        Request->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS,
            &DriverParams, sizeof(CacheDriverParams), NULL, 0);

        Status = Device.SyncCallDriver(Request);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        Device.CloseConsumedDevice();

    }

    FctVolumeStats GetVolumeStat(ULONG Lun)
    {
        union {
            K10EFDCacheVolumeStats VolumeStats;
            K10_WWID               LunWwn;
        } stats;
        
        EMCPAL_STATUS Status;
        
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);           
        GenerateKeyFromLun(Lun, stats.LunWwn);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_FCT_GET_VOL_STATS, 
                &stats, sizeof(stats.LunWwn), &stats, sizeof(stats.VolumeStats));
        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);
        
        return stats.VolumeStats.Stats;
    }

    // DisplayAllVolumeStats displays the stats of all VirtualFlare volumes.
    // Inputs:
    //     startingLun - The starting index of VirtualFlare volume.
    //     numOfDevices - The number of VirtualFlare volumes.
    void DisplayAllVolumeStats(int startingLun = 1, int numOfDevices = 1)
    {
        EMCPAL_STATUS Status;
        int i = 0, lun = startingLun;
        K10_WWID *wwid;
        K10EFDCacheVolumeStats *CacheVolStats;
        char buffer[sizeof (K10EFDCacheVolumeStats)];
        wwid = (K10_WWID*)buffer;
        CacheVolStats = (K10EFDCacheVolumeStats*)buffer;

        // Check for valid input
        FF_ASSERT((startingLun + numOfDevices - 1) <= FCT_MAX_HOST_ACCESSIBLE_LUNS);
        
        for (i = 0; i < numOfDevices; i ++)
        {
            memset(buffer, '\0', sizeof (K10EFDCacheVolumeStats));
            GenerateKeyFromLun(lun, *wwid);
            mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
            mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_FCT_GET_VOL_STATS, 
                wwid, sizeof(K10_WWID),  CacheVolStats, sizeof(K10EFDCacheVolumeStats));

            Status = mDevice.SyncCallDriver(mRequest);
            if (EMCPAL_STATUS_SUCCESS == Status)
            {
                KvPrint("Display status for Lun %d\n", lun);
                DisplayVolumeStats(CacheVolStats);
            }

            lun ++;
        }        
    }

    void GetCacheDeviceUnmapInfo(USHORT devId, FLARE_ZERO_FILL_INFO * pZFInfo)
    {
        EMCPAL_STATUS Status;
        char buffer[sizeof (FLARE_ZERO_FILL_INFO)];
        USHORT *pReq = (USHORT *)buffer;
        FLARE_ZERO_FILL_INFO *pResp = (FLARE_ZERO_FILL_INFO *)buffer;

        // Check for valid input
        FF_ASSERT(devId < FCT_MAX_CACHE_DEVICES);

        memset(buffer, '\0', sizeof (FLARE_ZERO_FILL_INFO));
        *pReq = devId;
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0,
                                IOCTL_FCT_GET_CACHE_DEVICE_ZEROFILL_INFO, 
                                pReq, sizeof(USHORT), pResp,
                                sizeof(FLARE_ZERO_FILL_INFO));
        Status = mDevice.SyncCallDriver(mRequest);
        if (EMCPAL_STATUS_SUCCESS == Status)
        {
            *pZFInfo = *pResp;
        }
        else
        {
            memset(pZFInfo, '\0', sizeof (FLARE_ZERO_FILL_INFO));
        }
    }

    void GetPagesPromotedOnDevice(ULONG devId, ULONG& promoted)
    {
        EMCPAL_STATUS Status;
        FctPagesPromotedReq * pReq;
        FctPagesPromotedResp * pResp;
        char buffer[sizeof (FctPagesPromotedReq)];
        pReq = (FctPagesPromotedReq *)buffer;
        pResp = (FctPagesPromotedResp *)buffer;

        // Check for valid input
        FF_ASSERT(devId < FCT_MAX_CACHE_DEVICES);
        
        memset(buffer, '\0', sizeof (FctPagesPromotedReq));
        pReq->DeviceIndex = devId;
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_FCT_CACHE_DEVICE_PROMOTED_PAGES, 
            pReq, sizeof(FctPagesPromotedReq),  pResp, sizeof(FctPagesPromotedResp));

        Status = mDevice.SyncCallDriver(mRequest);
        if (EMCPAL_STATUS_SUCCESS == Status)
        {
            promoted = pResp->PagesPromoted;
        }

    }

    void GetPagesPromotedOnAllDevices()
    {
        EMCPAL_STATUS Status;
        FctPagesPromotedOnDevices response;
        
        memset(&response, '\0', sizeof (response));
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_FCT_GET_PROMOTED_PAGES_ON_ALL_DEVICES, 
            NULL, 0,  &response, sizeof(response));

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT (EMCPAL_STATUS_SUCCESS == Status)
        if (EMCPAL_STATUS_SUCCESS == Status)
        {
            for (USHORT devId = 0; devId < FCT_MAX_CACHE_DEVICES; devId ++)
            {
                if (response.PagesPromoted[devId] != (ULONG)-1)
                {
                    printf("Device %02d: %d pages are promoted.\n", devId, response.PagesPromoted[devId]);
                }
            }
        }

    }

    void EnableExpansionDebug(ULONG errEnabler = 0x1)
    {
        EMCPAL_STATUS Status;
        FctEnableExpansionDebugReq Req;

        memset(&Req, '\0', sizeof (FctEnableExpansionDebugReq));
        Req.DebugEnabler = errEnabler;
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_FCT_ENABLE_EXPANSION_DEBUG, 
            &Req, sizeof(FctEnableExpansionDebugReq),  NULL, 0);

        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT (EMCPAL_STATUS_SUCCESS == Status)
    }

    void ZeroFillTest(bool tarnish, bool pendOnLock, bool peerContactLost)
    {
        EMCPAL_STATUS Status;
        FctAction   Action;
        
        Action.ActionKind = BasicDriverAction::ActionThisSP;
        Action.Op = FCT_ACTION_ZERO_FILL_TEST;
        Action.type.ZeroFill.tarnish = tarnish;
        Action.type.ZeroFill.pendOnLock = pendOnLock;
        Action.type.ZeroFill.peerContactLost = peerContactLost;
        
        mRequest->ReinitializeRequest(EMCPAL_STATUS_SUCCESS);
        mRequest->SetupTopIoctl(BasicIoRequest::CODE_IOCTL, 0, 0, IOCTL_BVD_PERFORM_DRIVER_ACTION, 
                                &Action, sizeof(FctAction), NULL, 0);
        
        Status = mDevice.SyncCallDriver(mRequest);
        FF_ASSERT(EMCPAL_STATUS_SUCCESS == Status);

        FF_ASSERT(EMCPAL_STATUS_SUCCESS == mDevice.SyncCallDriver(mRequest));

         return;
    }

    void ForceUnmapResponseDelay(UINT_8 devId, ULONG secs)
    {
        FctAction Action;

        Action.ActionKind = BasicDriverAction::ActionAllSPs;
        Action.Op = FCT_ACTION_ENABLE_UNMAP_RESPONSE_DELAY;
        Action.type.UnmapResponseDelayTime.devId = devId;
        Action.type.UnmapResponseDelayTime.tmSeconds = secs;

        PerformDriverAction(&Action);
    }

private:
    void GenerateKeyFromLun(ULONG Lun, K10_WWID & Key)
    {
        Key = K10_WWID_Zero();
        *(ULONG *) (VOID *) &Key.node = 1;  
        *(ULONG *) (VOID *) &Key.nPort = Lun;
    }

    ULONG GetLunFromKey(VolumeIdentifier * Key)
    {return (*(ULONG *) (VOID *) &Key->nPort);}

    void DestroyDeviceAndRemoveLun(K10_LU_ID * pKey)
    {
        int Lun = GetLunFromKey(pKey);
        mpBaseClass->requestDeviceDestruction(Lun);
        RemoveLunFile(Lun);
    }

    void RemoveLunFile(int Lun)
    {
        char filename[64];
        sprintf(filename, "VirtualFlare%d", Lun);
        mpBaseClass->deleteDeviceFromFS(filename);
    }

    void DisplayVolumeStats(const K10EFDCacheVolumeStats *CacheVolStats)
    {
        KvPrint("NumPromoted:%lld, "
                "NumDirtyElements:%lld, "
                "NumReads:%lld, "
                "NumBlocksRead:%lld, "
                "NumWrites:%lld, "
                "NumBlocksWritten:%lld, "
                "NumReadErrors:%lld, "
                "NumWriteErrors:%lld, "
                "NumFlashReads:%lld, "
                "NumFlashBlocksRead:%lld, "
                "NumFlashWrites:%lld, "
                "NumFlashBlocksWritten:%lld, "
                "NumHddReads:%lld, "
                "NumHddBlocksRead:%lld, "
                "NumHddWrites:%lld, "
                "NumHddBlocksWritten:%lld, "
                "NumPromoted:%lld, "
                "NumPromotesAborted:%lld, "
                "NumPromotesFailed:%lld, "
                "NumPromotesRejected:%lld, "
                "NumBlocksPromoted:%lld, "
                "NumCleaned:%lld, "
                "NumBlocksCleaned:%lld, "
                "NumCleanEvicted:%lld, "
                "NumCleanBlocksEvicted:%lld, "
                "NumDirtyEvicted:%lld, "
                "NumDirtyBlocksEvicted:%lld, "
                "NumWriteThrus:%lld, "
                "NumBlocksWrittenThrough:%lld, "
                "NumReadHits:%lld, "
                "NumReadMisses:%lld, "
                "NumDirtyReadHits:%lld, "
                "NumWriteHits:%lld, "
                "NumWriteMisses:%lld, "
                "NumDirectoryUpdates:%lld, "
                "NumDmRequests:%lld, "
                "NumDmRejects:%lld, "
                "NumDmFwds:%lld, "
                "NumSyncOps:%lld, "
                "NumIdleFlushesAborted:%lld, "
                "NumLockCollisions:%lld, "
                "NumPendingTrackerCoreReassignments: %lld,"
                "NumFlashBypassedReads:%lld, "
                "NumDcaReadRedirectFailed:%lld, "
                "NumDcaWriteRedirectFailed:%lld, "
                "NumDeviceCleaned:%lld, "
                "NumDeviceBlocksCleaned:%lld, "
                "NumDeviceFlushLockCollisionsLocal:%lld, "
                "NumDeviceFlushLockCollisionsPeer:%lld, "
                "\n",
            CacheVolStats->Stats.mStatsNumPromotedThisSP - (CacheVolStats->Stats.mStatsNumIdleFlushed + CacheVolStats->Stats.mStatsNumInvalidated),
            (long long)CacheVolStats->Stats.mStatsNumDirtyElements,
            (long long)CacheVolStats->Stats.mStatsNumReads,
            (long long)CacheVolStats->Stats.mStatsNumBlocksRead,
            (long long)CacheVolStats->Stats.mStatsNumWrites,
            (long long)CacheVolStats->Stats.mStatsNumBlocksWritten,
            (long long)CacheVolStats->Stats.mStatsNumReadErrors,
            (long long)CacheVolStats->Stats.mStatsNumWriteErrors,
            (long long)CacheVolStats->Stats.mStatsNumFlashReads,
            (long long)CacheVolStats->Stats.mStatsNumFlashBlocksRead,
            (long long)CacheVolStats->Stats.mStatsNumFlashWrites,
            (long long)CacheVolStats->Stats.mStatsNumFlashBlocksWritten,
            (long long)CacheVolStats->Stats.mStatsNumHddReads,
            (long long)CacheVolStats->Stats.mStatsNumHddBlocksRead,
            (long long)CacheVolStats->Stats.mStatsNumHddWrites,
            (long long)CacheVolStats->Stats.mStatsNumHddBlocksWritten,
            (long long)CacheVolStats->Stats.mStatsNumPromoted,
            (long long)CacheVolStats->Stats.mStatsNumPromotesAborted,
            (long long)CacheVolStats->Stats.mStatsNumPromotesFailed,
            (long long)CacheVolStats->Stats.mStatsNumPromotesRejected,
            (long long)CacheVolStats->Stats.mStatsNumBlocksPromoted,
            (long long)CacheVolStats->Stats.mStatsNumCleaned,
            (long long)CacheVolStats->Stats.mStatsNumBlocksCleaned,
            (long long)CacheVolStats->Stats.mStatsNumCleanEvicted,
            (long long)CacheVolStats->Stats.mStatsNumCleanBlocksEvicted,
            (long long)CacheVolStats->Stats.mStatsNumDirtyEvicted,
            (long long)CacheVolStats->Stats.mStatsNumDirtyBlocksEvicted,
            (long long)CacheVolStats->Stats.mStatsNumWriteThrus,
            (long long)CacheVolStats->Stats.mStatsNumBlocksWrittenThrough,
            (long long)CacheVolStats->Stats.mStatsNumReadHits,
            (long long)CacheVolStats->Stats.mStatsNumReadMisses,
            (long long)CacheVolStats->Stats.mStatsNumDirtyReadHits,
            (long long)CacheVolStats->Stats.mStatsNumWriteHits,
            (long long)CacheVolStats->Stats.mStatsNumWriteMisses,
            (long long)CacheVolStats->Stats.mStatsNumDirectoryUpdates,
            (long long)CacheVolStats->Stats.mStatsNumSyncOps,
            (long long)CacheVolStats->Stats.mStatsNumIdleFlushesAborted,
            (long long)CacheVolStats->Stats.mStatsNumLockCollisions,
            (long long)CacheVolStats->Stats.mStatsNumFlashBypassedReads,
            (long long)CacheVolStats->Stats.mStatsNumDcaReadRedirectFailed,
            (long long)CacheVolStats->Stats.mStatsNumDcaWriteRedirectFailed,
            (long long)CacheVolStats->Stats.mStatsNumDeviceCleaned,
            (long long)CacheVolStats->Stats.mStatsNumDeviceBlocksCleaned,
            (long long)CacheVolStats->Stats.mStatsNumDeviceFlushLockCollisionsLocal,
            (long long)CacheVolStats->Stats.mStatsNumDeviceFlushLockCollisionsPeer, 
            (long long)0,
            (long long)0, 
            (long long)0,
            (long long)0);
    }

protected:
    BasicConsumedDevice mDevice;  // factory device
    BasicIoRequest *    mRequest;   // General purpose IRP
    BvdSimMutBaseClass * mpBaseClass;

private:

    // mStartCacheLun records the next caching LUN number to be allocated.
    unsigned int mStartCacheLun;

    // mCacheLunSectorSize indicates the sector size when allocating LUNs
    unsigned int mCacheLunSectorSize;

    // mNonHEDevices indicates the endurance rating of the Fast Cache devices
    bool mNonHEDevices;

    friend class RebootSPAServiceRunner;
    friend class RebootSPBServiceRunner;
    friend class ConcurrentIOSPAServiceRunner;
    friend class ConcurrentIOSPBServiceRunner;
    friend class DestroyLunSPAServiceRunner;
    friend class DestroyLunSPBServiceRunner;
    friend class RebootSPWithIoSPAServiceRunner;
    friend class RebootSPWithIoSPBServiceRunner;
    friend class DisableRenableCacheSPAServiceRunner;
    friend class DisableRenableCacheSPBServiceRunner;
    friend class BrokenCacheLunSPAServiceRunner;
    friend class BrokenCacheLunSPBServiceRunner;
    friend class DegradedCacheLunSPAServiceRunner;
    friend class DegradedCacheLunSPBServiceRunner;
    friend class FctExpansionDualSPSPAServiceRunner;
    friend class FctExpansionDualSPSPBServiceRunner;
    friend class FctMulExpansionDualSPSPAServiceRunner;
    friend class FctMulExpansionDualSPSPBServiceRunner;
    friend class FctMulExpansionShrinkDualSPSPAServiceRunner;
    friend class FctMulExpansionShrinkDualSPSPBServiceRunner;
    friend class FctMulExpansionShrinkWithIoSPAServiceRunner;
    friend class FctMulExpansionShrinkWithIoSPBServiceRunner;
    friend class FctExpandingCacheBrokenDualSPSPAServiceRunner;
    friend class FctExpandingCacheBrokenDualSPSPBServiceRunner;
    friend class FctExpansionCacheBrokenDualSPSPAServiceRunner;
    friend class FctExpansionCacheBrokenDualSPSPBServiceRunner;
    friend class FctExpansionCacheBrokenRebootSPAServiceRunner;
    friend class FctExpansionCacheBrokenRebootSPBServiceRunner;
    friend class FctExpansionCacheDegradedDualSPSPAServiceRunner;
    friend class FctExpansionCacheDegradedDualSPSPBServiceRunner;
    friend class FctExpansionCacheDegradedRebootSPAServiceRunner;
    friend class FctExpansionCacheDegradedRebootSPBServiceRunner;
    friend class FctExpandingCacheDegradedDualSPSPAServiceRunner;
    friend class FctExpandingCacheDegradedDualSPSPBServiceRunner;
    friend class FctExpansionRebootDualSPSPAServiceRunner;
    friend class FctExpansionRebootDualSPSPBServiceRunner;
    friend class FctExpansionFailureRebootDualSPSPAServiceRunner;
    friend class FctExpansionFailureRebootDualSPSPBServiceRunner;
    friend class FctShrinkDualSPSPBServiceRunner;
    friend class FctShrinkDualSPSPAServiceRunner;
    friend class FctShrinkDualSPDeviceDegradedSPAServiceRunner;
    friend class FctShrinkDualSPDeviceDegradedSPBServiceRunner;
    friend class FctShrinkDualSPCacheDegradedSPAServiceRunner;
    friend class FctShrinkDualSPCacheDegradedSPBServiceRunner;
    friend class FctShrinkDualSPDeviceBrokenSPAServiceRunner;
    friend class FctShrinkDualSPDeviceBrokenSPBServiceRunner;
    friend class FctShrinkDualSPCacheBrokenSPAServiceRunner;
    friend class FctShrinkDualSPCacheBrokenSPBServiceRunner;
    friend class FctExpansionInitDevFailureDualSPSPAServiceRunner;
    friend class FctExpansionInitDevFailureDualSPSPBServiceRunner;
    friend class FctExpansionMemAllocFailureSPAServiceRunner;
    friend class FctExpansionMemAllocFailureSPBServiceRunner;
    friend class FctExpansionDirSetupFailureSPAServiceRunner;
    friend class FctExpansionDirSetupFailureSPBServiceRunner;
    friend class FctExpansionRebootBothSPSPAServiceRunner;
    friend class FctExpansionRebootBothSPSPBServiceRunner;
    friend class FctExpansionRebootSPSPAServiceRunner;
    friend class FctExpansionRebootSPSPBServiceRunner;
    friend class FctBrokenRebootSPAServiceRunner;
    friend class FctBrokenRebootSPBServiceRunner;
    friend class FctShrinkRebootSingleSPSPBServiceRunner;
    friend class FctShrinkRebootSingleSPSPAServiceRunner;
    friend class FctShrinkRebootSingleSPCacheBrokenSPBServiceRunner;
    friend class FctShrinkRebootSingleSPCacheBrokenSPAServiceRunner;
    friend class FctShrinkRebootSingleSPDeviceBrokenSPBServiceRunner;
    friend class FctShrinkRebootSingleSPDeviceBrokenSPAServiceRunner;
    friend class FctShrinkRebootSingleSPCacheDegradedSPBServiceRunner;
    friend class FctShrinkRebootSingleSPCacheDegradedSPAServiceRunner;
    friend class FctShrinkRebootSingleSPDeviceDegradedSPBServiceRunner;
    friend class FctShrinkRebootSingleSPDeviceDegradedSPAServiceRunner;
    friend class FctShrinkRebootDualSPSPAServiceRunner;
    friend class FctShrinkRebootDualSPSPBServiceRunner;
    friend class FctShrinkRebootDualSPCacheBrokenSPBServiceRunner;
    friend class FctShrinkRebootDualSPCacheBrokenSPAServiceRunner;
    friend class FctShrinkRebootDualSPDeviceBrokenSPAServiceRunner;
    friend class FctShrinkRebootDualSPDeviceBrokenSPBServiceRunner;
    friend class FctShrinkRebootDualSPCacheDegradedSPAServiceRunner;
    friend class FctShrinkRebootDualSPCacheDegradedSPBServiceRunner;
    friend class FctShrinkRebootDualSPDeviceDegradedSPAServiceRunner;
    friend class FctShrinkRebootDualSPDeviceDegradedSPBServiceRunner;
    friend class FctShrinkDualSPWithUserLunOfflineSPBServiceRunner;
    friend class FctShrinkDualSPWithUserLunOfflineSPAServiceRunner;
    friend class FctShrinkMulDeviceDualSPSPBServiceRunner;
    friend class FctShrinkMulDeviceDualSPSPAServiceRunner;
    friend class FctShrinkMulDeviceDualSPDegradedSPAServiceRunner;
    friend class FctShrinkMulDeviceDualSPDegradedSPBServiceRunner;
    friend class FctShrinkMulDeviceDualSPBrokenSPAServiceRunner;
    friend class FctShrinkMulDeviceDualSPBrokenSPBServiceRunner;
    friend class FctShrinkMulDeviceRebootDualSPSPAServiceRunner;
    friend class FctShrinkMulDeviceRebootDualSPSPBServiceRunner;
    friend class FctShrinkMulDeviceRebootSingleSPSPAServiceRunner;
    friend class FctShrinkMulDeviceRebootSingleSPSPBServiceRunner;
    friend class FctFlushSPAServiceRunner;
    friend class FctFlushSPBServiceRunner;
    friend class FctExpandShrinkDualSPSPAServiceRunner;
    friend class FctExpandShrinkDualSPSPBServiceRunner;
    friend class FctExpandShrinkRebootSingleSPSPAServiceRunner;
    friend class FctExpandShrinkRebootSingleSPSPBServiceRunner;
    friend class FctExpandShrinkRebootDualSPSPAServiceRunner;
    friend class FctExpandShrinkRebootDualSPSPBServiceRunner;
    friend class CreateDestroyCacheSPAServiceRunner;
    friend class CreateDestroyCacheSPBServiceRunner;
    friend class ZeroFillSPAServiceRunner;
    friend class ZeroFillSPBServiceRunner;
};
