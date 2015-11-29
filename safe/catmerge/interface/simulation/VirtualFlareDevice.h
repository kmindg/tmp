/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualFlareDevice.h
 ***************************************************************************
 *
 * DESCRIPTION: VirtualFlare Device API definitions
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/13/2009  Martin Buckley Initial Version
 *    10/09/2015  Liu Yousheng Add VFD Statistics
 *
 **************************************************************************/
#ifndef __VIRTUALFLAREDEVICE
#define __VIRTUALFLAREDEVICE

# include "k10ntddk.h"
# include "EmcPAL_DriverShell.h"
# include "generic_types.h"
# include "simulation/VirtualFlareDriver.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/DiskGeometry.h"
# include "BasicLib/BasicIoRequest.h"
# include "BasicLib/BasicThread.h"
# include "flare_ioctls.h"
# include "processorCount.h"
# include "simulation/VirtualFlareDeviceStatistics.h"
# include "simulation/simulation_dynamic_scope.h"

#if defined(VFDRIVER_EXPORT)
#define VFDRIVERPUBLIC CSX_MOD_EXPORT 
#else
#define VFDRIVERPUBLIC CSX_MOD_IMPORT 
#endif

#define COMPILE_TIME_ASSERT(e) CSX_ASSERT_AT_COMPILE_TIME(e)

// Maximum number of CPUs platform will support (Matches MCC limitation)
static const int VFDMaxCPUs = MAX_CORES;

// Prototype for Virtual Flare Device Raid Information callback
// Parameters are:
// Inputs:
//  Lun - Lun identifier
//  Flu - Flare Lun identifier
//  info - address of the FLARE_RAID_INFO that was set by the VFD.  Caller can modify the input information
//  identity - disk identity data for the disk associated with this volume.
//  context - user selectable value.
//
// Returns:
//   true  - callback has changed the input raid information
//   false - no information was changed in the callback
typedef bool (*Register_Raid_Information_Callback)(ULONG Lun, ULONG Flu, FLARE_RAID_INFO *info, const PDiskIdentity identity, void* context);

typedef struct VirtualFlareDeviceData_s
{
    volatile LONG openCount;
    char deviceName[64];
    char fileName[64];
    UINT_32 index;
    PIOTaskId taskId;
    DiskIdentity *identity;
    DiskGeometry *geometry;
} VirtualFlareDeviceData_t;

class VirtualFlareVolumeParams : public FilterDriverVolumeParams 
{
public:
    char deviceName[64];
    char fileName[64];
    UINT_32 index;
    DiskIdentity_t  identityData;
};

/*
 * When included in a C++ module, include the description of the VirtualDiskClass
 */
#ifdef __cplusplus

// Number of requests that we will process sequentially out of the que3ue before
// reording a request if it is not High Priority
#define VFD_REORDER_REQUEST_COUNT 3

class VirtualFlareDeviceProcessingThread : public BasicRealTimeThread {
public:
    VirtualFlareDeviceProcessingThread();
    ~VirtualFlareDeviceProcessingThread();

    void shutdownProcessing();
    EMCPAL_STATUS QueueRequest(PBasicIoRequest  Irp, KEY_PRIORITY prority = KEY_PRIORITY_DEFAULT);

    UINT_32 CurrentQueueDepth() const { return mCurrentQueueDepth; }

    bool Initialize();

 protected:
    void ThreadExecute(bool timedOut);

    // Arriving IRPs are queued here
    InterlockedListOfBasicIoRequests   mWaitingIrps;
    InterlockedListOfBasicIoRequests   mWaitingHPIrps;

    INT_32 DetermineIrpQueueCost(PBasicIoRequest Irp);

private:
    VOID DispatchQueuedIRP(PBasicIoRequest  Irp, INT_32 queueDepthDecrement);
    UINT_32 mReorderCount;
    UINT_32 mHiPriorityIoCount;

    UINT_32 mCurrentQueueDepth;
};

// structure used to contain Device (Disk) I/O response trigger counts and delays.
typedef struct _DeviceIOResponses {
    _DeviceIOResponses(UINT_32 Trigger,UINT_32 Delay) : TriggerCount(Trigger), DelayValue(Delay) {};
    _DeviceIOResponses() : TriggerCount(0), DelayValue(0) {};
    _DeviceIOResponses& operator=(const _DeviceIOResponses &rhs) {
        TriggerCount = rhs.TriggerCount;
        DelayValue = rhs.DelayValue;
        return *this;
    }
    // Number of I/Os to complete with introducing a delay
    UINT_32   TriggerCount;
    // Amount to delay in microseconds when TriggerCount is reached.
    UINT_32   DelayValue;
} DeviceIOResponses, *PDeviceIOResponses;



class VirtualFlareVolume : public BasicNotificationVolume {
public:
    VirtualFlareVolume(VirtualFlareVolumeParams * volumeParams, BasicVolumeDriver * driver);

    ~VirtualFlareVolume();
    
    static UINT_32 MAX_Q_DEPTH_PER_DISK;

    static UINT_32 MAX_VAULT_Q_DEPTH_PER_DISK;

    VFDRIVERPUBLIC static void SetMaxQueueDepth(UINT_32 depth);

    VFDRIVERPUBLIC static UINT_32 GetMaxQueueDepth();

    // Queue the incoming IRP to the thread on the thread handling those
    // requests.
    void QueueRequestToIOThread(PBasicIoRequest  Irp, KEY_PRIORITY priority = KEY_PRIORITY_DEFAULT);

    VFDRIVERPUBLIC EMCPAL_STATUS InitializeVolumeReleaseLock();
    
    VFDRIVERPUBLIC EMCPAL_STATUS IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC EMCPAL_STATUS IRP_MJ_DEVICE_CONTROL_Handler(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_READ_Handler(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_WRITE_Handler(PBasicIoRequest  Irp);
    
    VFDRIVERPUBLIC virtual EMCPAL_STATUS IRP_MJ_CREATE_ModifierFirstOpen();

    VFDRIVERPUBLIC void IRP_MJ_CLOSE_ModifierLastClose();

    VFDRIVERPUBLIC VOID VolumeDataTableNotificationHandlerReAcquireLock(VirtualFlareLogicalTable * logicalTable);

    VFDRIVERPUBLIC VOID VolumePowerSavingNotificationHandlerReAcquireLock();

    VFDRIVERPUBLIC VOID IRP_MJ_DEVICE_CONTROL_Other(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(PBasicIoRequest Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(PBasicIoRequest Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_SGL_READ(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_SGL_WRITE(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DCA_WRITE(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DCA_READ(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_CAPABILITIES_QUERY(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_GET_VOLUME_STATE(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_GET_RAID_INFO(PBasicIoRequest  Irp);

    VFDRIVERPUBLIC VirtualFlareVolumeParams *getDiskData() const; 

    VFDRIVERPUBLIC PDiskIdentity getDiskIdentity() const;

    VFDRIVERPUBLIC UINT_32 getRaidGroupId() const;

    VFDRIVERPUBLIC PEMCPAL_DEVICE_OBJECT getDeviceObject() const;

    VFDRIVERPUBLIC         void Perform_MJ_READ(PEMCPAL_DEVICE_OBJECT  DeviceObject, PBasicIoRequest Irp);    
    VFDRIVERPUBLIC         void Perform_MJ_WRITE(PEMCPAL_DEVICE_OBJECT  DeviceObject, PBasicIoRequest Irp);
    
    VFDRIVERPUBLIC         EMCPAL_STATUS Perform_MJ_DEVICE_CONTROL(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp, UINT_32 *information);

    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_CLEANUP(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp) = 0;

    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_SGL_READ(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp, ULONG32  *information);
    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_SGL_WRITE(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp, ULONG32  *information);
    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DCA_READ(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp, ULONG32  *information);
    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DCA_WRITE(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp, ULONG32  *information);
    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DM_SOURCE(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp);
    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DM_DESTINATION(PEMCPAL_DEVICE_OBJECT DeviceObject, PBasicIoRequest Irp);

    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_READ(PEMCPAL_DEVICE_OBJECT   DeviceObject, PBasicIoRequest Irp, ULONG32  *information)  = 0;
    VFDRIVERPUBLIC virtual EMCPAL_STATUS DispatchIRP_MJ_WRITE(PEMCPAL_DEVICE_OBJECT  DeviceObject, PBasicIoRequest Irp, ULONG32 *information)  = 0;

    VFDRIVERPUBLIC virtual EMCPAL_STATUS PerformDeviceSpecific_MJ_DEVICE_CONTROL(PEMCPAL_DEVICE_OBJECT  DeviceObject, PBasicIoRequest Irp, UINT_32 *information) = 0;

    VFDRIVERPUBLIC static VOID  DCAReadCompletion(EMCPAL_STATUS status, PVOID context);
    
    VFDRIVERPUBLIC static VOID  DCAWriteCompletion(EMCPAL_STATUS status, PVOID context);

    VFDRIVERPUBLIC void SetIoCompletionCallback(Register_Io_Notification_Callback callback, void * context) {
        mIoCompletionCallback = callback;
        mIoCompletionCallbackContext = context;
    }

    VFDRIVERPUBLIC void SetIsDeviceReadyCallback(Register_Is_Device_Ready_Callback callback,void* context) {
        mCallerIndicatesDeviceIsReady = callback;
        mCallerIndicatesDeviceIsReadyContext = context;
    }

    VFDRIVERPUBLIC void SetIoMonitor(VirtualFlareIOMonitor *monitor) {
        mIoMonitor = monitor;
    }

    VFDRIVERPUBLIC Register_Io_Notification_Callback GetIoCompletionCallback() const {
        return mIoCompletionCallback;
    }

    VFDRIVERPUBLIC bool GetDeviceStatus() const {
        return mReady;
    }

    VFDRIVERPUBLIC void  setIOAllowed(bool allowed) {
        mIOAllowed = allowed;
    }
    
    VFDRIVERPUBLIC void setVFDThrottling(bool throttling);
    
    VFDRIVERPUBLIC bool getIOAllowed() const {
        return mIOAllowed;
    }

    VFDRIVERPUBLIC void  setSlowIO(UINT_32 delay) {
        mSlowIO = true;
        mIODelay = delay;
    }
    
    VFDRIVERPUBLIC bool getSlowIO() const {
        return mSlowIO;
    }

    VFDRIVERPUBLIC UINT_32 getIODelay() const {
        if(mSlowIO) {
            return mIODelay;
        }
        
        AbstractDisk_type_t  type = getDiskIdentity()->getDiskType();
        return getDeviceIOResponse(type);
    }

    VFDRIVERPUBLIC bool isSpunDown() const {
        return mIsSpunDown;
    }

    // Set alerted and quote exceeded intervals for this device.  If set, then the number "success" IRP's
    // will be completed with STATUS_SUCCESS, then "alerted" IRP's with STATUS_ALERTED.
    // The pattern continues until the parameters are reset.
    // @param alerted - The number of consecutive STATUS_ALERTED completions.
    // @param success - The number of consecutive STATUS_SUCCESS completions.
    // @param quota - The number of consecutive STATUS_QUOTA_EXCEEDED completions.
    VFDRIVERPUBLIC void setDeviceAlertedInterval(UINT_32 alerted, UINT_32 success, UINT_32 quota) {
        mStatusAlertedInterval = alerted;
        mStatusSuccessInterval = success;
        mStatusQuotaExceededInterval = quota;
    }

    // Set this volume quota limit.  It is usually set assuming 5 volumes in a raid
    // group, which isn't always the case for some tests.   This just makes
    // setting up some testing situations easier.
    // @param limit - queue limit for this volume.
    VFDRIVERPUBLIC void setVolumeIoQuotaLimit(UINT_32 limit) {
        if(limit != 0) {
            mVolumeIoQuotaLimit = limit;
        }
    }
    // Get the current queue depth for this device.
    // Note: this is a dynamic value not protected by any locks.
    VFDRIVERPUBLIC UINT_32 CurrentQueueDepth() const {
        return mProcessingThread.CurrentQueueDepth();
    }

    void performReadSectorNotification(UINT_64 LBA, UINT_8 *buffer);
    void performReadNotification(UINT_64 LBA, UINT_32 noLba, UINT_8 *buffer);
    void performWriteSectorNotification(UINT_64 LBA, UINT_8 *buffer);
    void performWriteNotification(UINT_64 LBA, UINT_32 noLba, UINT_8 *buffer);

    // Number of I/Os to process before sending alerts;
    UINT_32 mIoCount;

    // Increments the status success counter and indicates what status to return
    // for an I/O.
    // Returns:
    //   true  - I/O should return STATUS_ALERTED
    //   false - I/O should return STATUS_SUCCESS
    bool shouldReturnStatusAlerted();
    
    // For a non-zero quota exceeded interval, determine if the status returned
    // for an I/O should be STATUS_QUOTA_EXCEEDED or not.
    // Returns:
    //   true  - I/O should be returned with STATUS_QUOTA_EXCEEDED
    //   false - I/O should be returned with a different status.
    bool shouldReturnStatusQuotaExceeded();

    VFDRIVERPUBLIC bool SendVolumeStateChangeNotification();
    static void ActiveNotificationIrpCancelRoutine(IN PVOID Unused, IN PBasicIoRequest Irp);

    // Set the device Io responses for ALL the volumes that are created by this driver.
    VFDRIVERPUBLIC static void setDeviceIOResponse(DeviceIOResponses ssdResponse, DeviceIOResponses fibreResponse,
            DeviceIOResponses sasResponse, DeviceIOResponses ataResponse, bool enableSpinForDiskDelay = false);

    // Set the device Io responses for this volume.  This overrides the global settings
    VFDRIVERPUBLIC void setVolumeDeviceIOResponse(DeviceIOResponses ssdResponse, DeviceIOResponses fibreResponse,
            DeviceIOResponses sasResponse, DeviceIOResponses ataResponse, bool enableSpinForDiskDelay = false);
    
    // Reset the DeviceIoResponses for this volume back to the global ones set for the driver
    VFDRIVERPUBLIC void resetVolumeDeviceIOResponse();

    VFDRIVERPUBLIC UINT_32 getDeviceIOResponse(AbstractDisk_type_t type) const;
    VFDRIVERPUBLIC UINT_32 getDeviceIOTriggerCount(AbstractDisk_type_t type);

    // Registers a callback with the Virtual Flare Volume class.
    // Returns:
    //   true  - indicates that caller callback has been successfully registered.   There can only
    //           ever be one registered callback
    //   false - callback previously registered
    static bool registerRaidQueryCallback(Register_Raid_Information_Callback callbackAddr, void* context);

    // Unregisters a callback with the Virtual Flare Volume class.
    // Returns:
    //   true  - indicates that caller callback has been successfully unregistered.
    //   false - callback was not unregistered
    static bool unRegisterRaidQueryCallback(Register_Raid_Information_Callback callbackAddr);
    
    bool IsUsingSpinForIODelay() {
        return mUseSpinForDiskDelay;
    }

    // Update the Volumes' Statistics
    // @params Irp - io request
    // @params status - status to be returned
    void UpdateStatistics(PBasicIoRequest Irp, EMCPAL_STATUS status);

    // GetStats - sum Volume stats and return
    // @param stats - this structure is filled in with the sum across all cores.
    SOURCE_SYMBOL_SCOPE void GetStats( VirtualFlareVolumeStatistics &stats );

    // Overridden from BasicVolume.  Service for IOCTL_BVD_CLEAR_VOLUME_STATISTICS
    VOID EventClearVolumeStatisticsReleaseLock() OVERRIDE {
        ClearStatsLockHeld();
        BasicVolume::EventClearVolumeStatisticsReleaseLock();
    }

    // Clear Volume statistics.  The volume lock is not actually necessary, since the
    // stats are partitioned and protected by their own per core locks, but the caller is
    // expected to have the lock held.
    SOURCE_SYMBOL_SCOPE void ClearStatsLockHeld();

    // GetStats - Get Statistics object for the executing core.
    inline VirtualFlareVolumeProxyStatisticsLocked *GetThisCoreStats(ULONG id = csx_p_get_processor_id());

    // This function overrides the base class and fills in volume specific statistics
    // information in addition to what basicVolume provides.
    // @params ReturnedVolumeStatistics - the status structure to fill in.
    // @params MaxVolumeStatsLen - the maximum # bytes to be written into
    // ReturnedVolumeStatistics
    // @params OutputLen - the actual number of bytes 
    // Returns:  
    //   STATUS_SUCCESS - the data was returned
    //   STATUS_BUFFER_TOO_SMALL - MaxVolumeStatsLen was inadequate.
    EMCPAL_STATUS EventGetVolumeStatisticsReleaseLock(BasicVolumeStatistics * ReturnedVolumeStatistics,
                                                 ULONG MaxVolumeStatsLen, ULONG & OutputLen) OVERRIDE;
protected:
    // Notify the driver who has initiated the particular request if it has registered with us.
    inline void NotifyCallerIfRequired(const PEMCPAL_IRP irp, EMCPAL_STATUS status) {
        if(mIoCompletionCallback) {
            mIoCompletionCallback(irp, status, mIoCompletionCallbackContext);
        }
    }
    
    // Return the Volumes' Raid Information
    //
    // @params info - pointer to FLARE_RAID_INFO buffer which will receive returned information
    void getRGInfo(FLARE_RAID_INFO *info);

    inline bool CallerIndicatesDevicesIsReady() {
        if(mCallerIndicatesDeviceIsReady) {
            return mCallerIndicatesDeviceIsReady(mCallerIndicatesDeviceIsReadyContext);
        }
        return true;
    }
    PEMCPAL_DEVICE_OBJECT      mDeviceObject;


    PIOTaskId mIOTask;               // default/specified IO task manager
    PIOTaskId mIOTask512;            // IO task manager for 512Byte sectors (MJ_READ/MJ_WRITE)
    PDiskIdentity mDiskIdentity;     // description of disk

private:
    VirtualFlareDriver * GetVirtualFlareDriver() {
        return (VirtualFlareDriver*)GetDriver();
    }

    virtual BOOL writeToDisk(UINT_8 *buffer, ULONG deviceLength, INT_64 sectorAddress, csx_status_e& errStatus) = 0;
    virtual BOOL readFromDisk(UINT_8 *buffer, ULONG deviceLength, INT_64 sectorAddress, csx_status_e& errStatus) = 0;

    virtual BOOL writeToDisk512(UINT_8 *buffer, ULONG deviceLength, INT_64 sectorAddress, csx_status_e& errStatus) = 0;
    virtual BOOL readFromDisk512(UINT_8 *buffer, ULONG deviceLength, INT_64 sectorAddress, csx_status_e& errStatus) = 0;

    // Check that the SGL type (and optionally the IOTask) is appropriate for the current volume.
    virtual void ValidateSglSectorSizes(PFLARE_SGL_INFO SglInfo, PIOTaskId IOTask = NULL);

    VirtualFlareVolumeParams mVolumeParams; // parameters provided when volume was created

    DeviceIOResponses   mLocalDeviceIOResponse[DiskTypeLAST];
    bool                mLocalUseSpinForDiskDelay;
    DeviceIOResponses*  mPDeviceIoResponses;
    bool*               mPUseSpinForDiskDelay;

    // Flag for ready state
    bool mReady;

    // Indicates whether volume is degraded or not.
    bool mIsDegraded;

    // Indicates whether path is not preferred for this volume.
    bool mIsPathNotPreferred;

    // Indicates whether volume is spun down.
    bool mIsSpunDown;
    
    // Indicates whether volume makes throttling decisions
    bool mVFDThrottlingEnabled;
    
    // Indicates that the volume alerted MCC of congestion
    bool mReportedAlerted;
    
    // Indicates that the volume alerted MCC of congestion
    bool mReportedQuotaExceeded;
    
    // If mVFDThrottlingEnabled is true then this is the number
    // of I/Os that that the volume can handle before throttling begins
    UINT_32 mVolumeIoQuotaLimit;
    
    // VFD I/O processing thread(s)
    VirtualFlareDeviceProcessingThread mProcessingThread;
    
    // Callback routine which we call before completing the IRP.
    Register_Io_Notification_Callback mIoCompletionCallback;
    void * mIoCompletionCallbackContext;

    // Callback routine which we call before processing an IRP to
    // ask caller if device is ready.   Used to test intermittent underlying
    // disk issues;
    Register_Is_Device_Ready_Callback mCallerIndicatesDeviceIsReady;
    void* mCallerIndicatesDeviceIsReadyContext;

    // IO monitor delegate
    VirtualFlareIOMonitor *mIoMonitor;

    /*
     * True when test is forcing a particular delay on IO completion
     */
    bool mSlowIO;

    /*
     * TRUE we use spin to wait, false we use slee pt wait;
    */
    static bool mUseSpinForDiskDelay;

    /*
     * When false Read/Write operations will cause an exception
     */
    bool mIOAllowed;

    /*
     * Test specific IO completion delay, in milliseconds
     * Only used when mSlowIO is true
     */
    UINT_32 mIODelay;

    /*
     * The following three variables having to do with return status
     * are for specialized testing of back-end queue depth management.
     */

    // Number of consecutive I/O's to be return STATUS_SUCCESS.
    UINT_32 mStatusSuccessInterval;

    // Number of consecutive I/O's to return STATUS_ALERTED.  When 0, all
    // I/O's return STATUS_SUCCESS, regardless of mStatusSuccessInterval.
    UINT_32 mStatusAlertedInterval;
    
    // Number of consecutive I/Os' to return STATUS_QUOTA_EXCEEDED. When 0,
    // all I/O's are returned with STATUS_SUCCESS or STATUS_ALERTED based on
    // those intervals.
    UINT_32 mStatusQuotaExceededInterval;

    // Counter for keeping track of counts of requests returning
    // STATUS_SUCCESS or STATUS_ALERTED or STATUS_QUOTA_EXCEEDED.
    UINT_32 mIOCounter;

    // Buffer used by driver to perform a zero operation
    static const ULONG  mZeroBufferSize = 1040*64;
    unsigned char mZeroBuffer[mZeroBufferSize];

    // If non-zero contains the address of a callback that is called whenever
    // a virtual flare volume is queried for Raid Information.   This gives the
    // caller the ability to change the information for testing purposes.
    static Register_Raid_Information_Callback  mRaidInformationCallback;
    static void* mRaidInformationCallbackContext;

    // Per core Statistics of this volume which has its own lock.
    VirtualFlareVolumeProxyStatisticsLocked *mStats;// [gPlatformCPUs]

    // Sectors Per Stripe of the Raid Group
    UINT_32 mSectorsPerStripe;
};

// VirtuaFlareVolume_register_Raid_Information_Callback allows a test application the ability of registering a callback that will be
// called whenever information about a raid group is required by a caller of the VFD.   The callback has the ability of modifying the returned
// information.   It is the callbacks responsibility to return PROPER DATA, in other words the data must be valid for a Raid Group.
extern bool VFDRIVERPUBLIC VirtuaFlareVolume_register_Raid_Information_Callback(Register_Raid_Information_Callback callback, void* context);

// This allows a caller who registered for the raid information callback to unregister.
extern bool VFDRIVERPUBLIC VirtuaFlareVolume_unregister_Raid_Information_Callback(Register_Raid_Information_Callback callback);


typedef struct VirtualFlareDevice_DCA_Context_s
{
    VOID * MemoryAllocationPointer;
    PEMCPAL_IRP Irp;
    VirtualFlareVolume *disk;
    EMCPAL_STATUS status;
} VirtualFlareDevice_DCA_Context_t;

typedef struct VirtualFlareDevice_DCA_Tracker_s
{
    FLARE_DCA_ARG dcaArg;
    VirtualFlareDevice_DCA_Context_t DCA_Context;
    VSGL sgList[2];
} VirtualFlareDevice_DCA_Tracker_t;

#endif
#endif // __VIRTUALFLAREDEVICE

