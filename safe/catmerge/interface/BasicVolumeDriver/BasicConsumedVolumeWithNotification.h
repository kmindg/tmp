/************************************************************************
*
*  Copyright (c) 2002, 2005-2006,2009 EMC Corporation.
*  All rights reserved.
*
*  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
*  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
*  BE KEPT STRICTLY CONFIDENTIAL.
*
*  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
*  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
*  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
*  MAY RESULT.
*
************************************************************************/

#ifndef __BasicConsumedVolumeWithNotification__
#define __BasicConsumedVolumeWithNotification__

//++
// File Name:
//      BasicConsumedVolumeWithNotification.h
//
// Contents:
//      Defines the BasicConsumedVolumeWithNotification class along with related 
//      classes and datatypes
//
// Revision History:
//      09/07/2010 - Ported from RedirectorConsumedVolume.h
//--

# include "BasicVolumeDriver/BasicVolumeDriver.h"
# include "BasicVolumeDriver/ConsumedVolume.h"
# include "BasicVolumeDriver/BasicVolumeState.h"
# include "BasicLib/BasicTimer.h"
# include "BasicLib/BasicDeferredProcedureCall.h" 
# include "BasicLib/BasicWaiter.h"
# include "BasicLib/BasicLockedObject.h"

# define MAX_CLOSE_CONSUMED_VOLUME_DELAY (100000) //100 seconds

// Type that determines what kind of registration should be made to
// drivers below for capacity and ready state changes.
enum CapacityChangeRegistrationType {
    // Do not register.
    CapChangeRegNone = 0,

    // Register only on owner.
    CapChangeRegOwner = 1,

    // Register on owner and non-owner
    CapChangeRegBoth = 2
};



// BasicConsumedVolumeWithNotification allows a DEVICE_OBJECT exported by a lower layer to 
// be opened, and provides an interface for sending IRPs.
// The BasicConsumedVolumeWithNotification class will register with the device below for
// notifications of changes to size and ready state, depending upon the value it is
// constructed with. This notification is a reliable indication of the state of the
// local paths, calling BasicConsumedVolumeWithNotification::VolumeStateChanged() 
// whenever the state changes.
class BasicConsumedVolumeWithNotification : public ConsumedVolume, private BasicTimerElement, private BasicDeferredProcedureCall, 
    public BasicLockedObject, public BasicObjectTracer
{
public:

    // Constructor
    //
    // @param vol - the volume this consumed volume is embedded in.
    // @param notificationParams - Parameters passed by subclass which determine behavior of this class.
    //                  
    BasicConsumedVolumeWithNotification(BasicVolumeDriver * driver);

    virtual ~BasicConsumedVolumeWithNotification();

    // Completes BasicConsumedVolumeWithNotification initialization.
    // Returns: true if initialization successful, false if out of resources.
    bool Initialize();


    // Overrides the base class.  Opens the device object whose name was previosly
    // specified via ConsumedVolume::SetDeviceObjectName(). Must execute at PASSIVE_LEVEL.
    //
    // @param capabilitiesQuery - If open is successful, issue CAPABILITIES_QUERY if true,
    //                            otherwise mark as capabilities unknown.
    // @param raidInfoQuery     - If open is successful, issue GET_RAID_INFO
    //                            if true, otherwise set stripe size to -1
    //
    // Returns: success if the device below was successfully attached to.
    virtual EMCPAL_STATUS OpenConsumedVolume(bool capabilitiesQuery = true,
                                bool raidInfo = false);

    // Close the consumed device.
    //
    // Must execute at PASSIVE_LEVEL
    virtual VOID CloseConsumedVolume();

    // Sends an IOCTL_FLARE_TRESPASS_EXECUTE to the consumed volume.
    //
    // @param waiter - waiter to be signaled when the IRP completes.
    VOID SendTrespassExecute(BasicWaiter * waiter);

    // Sends an IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS to the consumed volume. 
    //
    // @param handler - the function on the GANG to be called when the IRP completes.
    VOID SendOwnershipLoss(BasicWaiter * waiter);

    // Trigger this volume to update the volume state asynchonrously.  This asynchronous
    // operation will be complete when IsVolumeStateStableLockHeld() returns true.
    //
    // @param waiter - waiter to be signaled when the IRP completes.
    VOID InitiateRefreshOfVolumeStateReacquireLock(BasicWaiter * waiter = NULL);

    // Returns false if there has been a notification of volume state change (or
    // InitiateRefreshOfVolumeStateReacquireLock()) and the state is not yet retrieved.
    // Returns TRUE otherwise.
    bool IsVolumeStateStableLockHeld() const; 

    // Changes the settings for the power saving policies
    // @param params - the new value of the parameters.
    void SetPowerSavingPolicyReleaseLock(PFLARE_POWER_SAVING_POLICY params);

    // Replaces the timeout value set on construction.  May be done only prior to
    // any timer being started.
    //
    // @param timeOut - The number of seconds that an element will sit on the mElements
    //                  list until it's TimerTriggered() function will be called.
    //
    static VOID SetBasicFixedTimerInterval(ULONG timeOut);

    // Handles processing the volume state change detected by driver below.
    // Pure virtual, subclass must implement.
    virtual VOID VolumeStateChanged(const BasicVolumeState & state) = 0;

    // Default Timeout for warning on Trespass of ConsumedVolume  
    static const int DEFAULT_CONSUMED_VOLUME_TRESPASS_TIMEOUT_WARNING = 10;

    // Default Timeout for crash on Trespass of ConsumedVolume  
    static const int DEFAULT_CONSUMED_VOLUME_TRESPASS_TIMEOUT_CRASH = 250;

    // Default wait time to Cancel GETVOLINFO when Trespass of ConsumedVolume Timesout  
    static const int DEFAULT_CONSUMED_VOLUME_TRESPASS_TIMEOUT_CANCELGETVOLINFO = 230;

    // Returns the number of seconds a trespass to be in-flight before warning messages are logged.
    virtual UINT_32 GetConsumedVolumeTrespassTimeoutWarning() const { return DEFAULT_CONSUMED_VOLUME_TRESPASS_TIMEOUT_WARNING; }

    // Returns the number of seconds a trespass to be in-flight before the SP will crash.  Long
    // trespass will indicate that there is alrady a host DU.   The crash will capture diagnosis information
    // an may allow automated recovery.
    virtual UINT_32 GetConsumedVolumeTrespassTimeoutCrash() const { return DEFAULT_CONSUMED_VOLUME_TRESPASS_TIMEOUT_CRASH; }

    // Returns the number of seconds a GET_VOLUME_STATE to be in-flight before it should be cancelled.  Cancellation
    // attempts to limit the impact of a bug for a particular volume.   In addition, cancel timeouts may trigger the 
    // offending component to detect the failure, and do a better recovery (e.g., crash the at-fault SP).
    //
    virtual UINT_32 GetConsumedVolumeTrespassTimeoutCancelGetVolInfo() const { return DEFAULT_CONSUMED_VOLUME_TRESPASS_TIMEOUT_CANCELGETVOLINFO; }

    // Should this volume register for volume state change notifications on 0, 1, or 2 SPs?
    virtual CapacityChangeRegistrationType GetCapacityChangeRegistrationType() const { return CapChangeRegBoth; }    

    // Report the current state of the volume.
    VOID GetCurrentVolumeState(BasicVolumeState & state) {
        AcquireSpinLock();
        state = mVolumeState;
        ReleaseSpinLock();
    }

    BasicVolumeDriver   *   GetDriver() const {return mDriver;}

private:
    // Save state of consumed volume.
    BasicVolumeState mVolumeState;

    // Describes whether mIrp is not in use, in use, or in the process of being cancelled.
    // 
    enum IrpState {
        // There is no outstanding IRP.  
        IrpIdle, 

        // We have kicked the DPC to start the registration, but it has not yet run.
        IrpSendingWait,

        // A registration IRP has been issued that has not completed or been cancelled.
        IrpWaiting,

        // There is an outstanding IRP that will finish shortly and initiate a poll. If an
        // IoCancelIrp() sequence completed, but the IRP did not yet complete, then state
        // become IrpActive.
        IrpActive,

        // There is an outstanding IRP that will finish shortly returning the current
        // value of the volume state.
        IrpPolling,

        // IoCancelIrp is about to be, or in the process of being, called. IoCancelIrp
        // must be called with no locks held, and we must ensure that the IRP cannot be
        // freed or reused until IoCancelIrp() returns.  Next transtion will be to
        // IrpCancelComplete or IrpActive.
        IrpCancelling, 

        // An IRP that was in the IrpCancelling state is moved to this state on IRP
        // completion. There must be some context that has released locks and is in the
        // process of executing an IoCancelIrp() call for this state to be reached.  That
        // context is expected to deal with IRP completion.
        IrpCancelComplete,

    };

    // Defines the trespass state of this volume.   The state transition occurs when the
    // relevant IOCTL_FLARE_TRESPASS_* is issued.
    enum CurrentState {
        // The consumed device is closed, and there are no registrations.
        Closed,

        // The consumed device is open, and the this SP is not owner, i.e., the device was
        // never owner or a IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS was last sent to the
        // device.
        NotOwned,

        // The consumed device is open, and the this SP is owner, i.e., a
        // IOCTL_FLARE_TRESPASS_EXECUTE was last sent to the consumed device.
        Owned
    };

    // Overrides the base class. An DEVICE_CONTROL which just completed causes this
    // routine to be called.  This routine determines its action based on the current
    // state.
    //
    // @param Irp - the IRP that just completed.  
    VOID DeviceControlCompletion(IN PBasicIoRequest pIrp);

    // Internal implementation for DeviceControlCompletion
    // @param Irp - the IRP that just completed.
    VOID DeviceControlCompletionReleaseLock(IN PBasicIoRequest pIrp);
 
    // Send a GET_VOLUME_STATE to determine current ready state and size. The Irp
    // completion may cause a IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION to be sent. May
    // do nothing depending upon state and configuration.
    //
    // waiter - a waiter to signal after volume state is reported.
    void InitiateGetOfReadyStateReleaseLock(BasicWaiter * waiter = NULL);

    // Prepare to send a GET FLARE VOLUME STATE IOCTL to the lower driver, preceeding that
    // with a SET POWER SAVING POLICY if one it needed.
    VOID PreparePowerPolicyOrGetVolInfoLockHeld();

    // Callback function registered with devices below on TRESPASS_EXECUTE. This will be
    // called if there is a change in the consumed device's ready state or size.
    // Note: This can only be called after a TRESPASS_EXECUTE has been issued and before
    // the OWNERSHIP_LOSS completes.
    //
    // Call member function of the same name. 
    //
    // @param pvthis - The RedirectorConsumedVolume being notified.
    static VOID  StateChangeNotifyHandler(PVOID pvthis);

    // Called when some lower device's state may have changed, and we need to go query
    // those devices (via Get Volume State) to see if the size or ready state changed.
    VOID  StateChangeNotifyHandler();

    // We pre-allocate a single IRP in Initialize() for use in sending trespass IOCTLs,
    // GET IOCTLs, and REGISTRATION IOCTLS to the devices below.
    PBasicIoRequest                       mIrp;

    // Defines the trespass state this volume.   The state transition occurs when the
    // relevant IOCTL_FLARE_TRESPASS_* is issued.
    CurrentState       mCurrentState;


    // The parameters communicated down the stack that control drive power consumption.
    FLARE_POWER_SAVING_POLICY           mPowerSavingPolicy;

    // Set to true if the power saving policies have changed, and the new parameters have
    // not been sent to the driver below.  Note: An ownership loss changes the parameters
    // implicitly to dispabled. We also send the power-policy along with a
    // TRESPASS_EXECUTE.
    bool                               mPowerSavingPolicyChanged;

    // This function is called if spIrpTimer determines that the consumed volume is not
    // making forward progress. At the time of the call, the element has been removed from
    // the timer's list, and is no longer associated with a timer.
    //
    // This callback is made with no locks held.  It is legal for this callback to add the
    // timer back to the clients callback list.
    //
    // @param whichTimer - must be spIrpTimer.
    // @param seconds - the number of seconds since the element was added to the timer's
    //                  queue.
    VOID TimerTriggeredNoLocksHeld(BasicFixedRateTimer * whichTimer, ULONG seconds);
    
    // Move to the state desired, by issuing the IOCTL specified. This initiates an
    // asynchronous process.
    //
    // @param desired - the state to move to.
    // 
    // mNextTrespassIoctl should just have been set.
    //       - IOCTL_FLARE_TRESPASS_EXECUTE transtions from NotOwned -> Owned.
    //       - IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS transtions from Owned -> NotOwned.
    VOID TransitionOwnerStateReleaseLock();

    // Member function to send an IRP to a device from a separate DPC. This DPC issues
    // IoCallDriver for the IRP.
    // @param DeviceObject - the consumed device object
    // @param Irp - the IRP to send to that device.
    VOID DeferredProcedureCallHandler();

    // Returns a string specifying the current state name.
    char * CurrentStateName() 
    {
        switch(mCurrentState) 
        {
        case Closed: return "Closed";
        case NotOwned: return "NotOwned";
        case Owned: return "Owned";
        default: return "???";
        }
    }


    // Describes whether mIrp is not in use, in use, or in the process of being cancelled.
    IrpState         mIrpState;

    // The IOCTL code of the IRP that is currently in flight.  Not valid if no IRP is in
    // flight.
    ULONG           mIrpIoControlCode;
    
    // If non-zero, the IOCTL code to issue when the IRP next completes.
    ULONG           mNextTrespassIoctl;

    // The tick count at the time the IRP was sent, or the cancel time for Registration
    // IRPs.
    EMCPAL_TIME_MSECS   mIrpIssuedTickCount;

    // Used to handle the race condition where an IRP is cancelled just as the timer goes
    // off.
    PBasicIoRequest            mIrpToCompleteOnTimer;

    // True if the timer has been started.
    bool            mTimerActive;
    
    // There is one consumed volume IRP timer object per driver.
    static BasicFixedRateTimer * spIrpTimer;

    // Spinlock used for instantiation and destruction of static IrpTimer
    static K10SpinLock sSpinlock;

    // CreationCount for instantiation and destruction of static IrpTimer
    static unsigned int sCreationCount;

    // Pointer to Driver which is consuming volumes.   
    BasicVolumeDriver *          mDriver;

    // How to register.
    CapacityChangeRegistrationType  mRegType;

    // If there is a IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION outstanding, causes it
    // to complete, so that the state machine can take some new action. The action on the
    // cancel completing is dependent upon state, e.g.,
    //    - issue GET VOLUME STATE and update the ready state/size on completion.
    //    - Complete a CloseConsumedVolume().
    //    - Issue a trespass IOCTL.
    VOID CancelNotificationRegistrationReleaseLock();

 
    // Get ready to send a pending TRESPASS IOCTL, if any, otherwise get ready to send a
    // IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION.
    //
    // Returns - true if an Irp is ready to send (mIrp)
    //         - NULL if no IOCTL should be sent now.  
    bool PrepareNextIoctlLockHeld();

    // Returns a string specifying the current state name.    
    char * IrpStateName() 
    {
        switch(mIrpState) 
        {
        case IrpIdle: return "IrpIdle";
        case IrpWaiting: return "IrpWaiting";
        case IrpSendingWait: return "IrpSendingWait";
        case IrpActive: return "IrpActive";
        case IrpPolling: return "IrpPolling";
        case IrpCancelling: return "IrpCancelling";
        case IrpCancelComplete: return "IrpCancelComplete";
        default: return "???";
        }
    }

    // Memory used for various IOCTLs.
    union u {

        // For IOCTL_FLARE_GET_VOLUME_STATE
        FLARE_VOLUME_STATE                 mFlareVolumeInfo;

        // A TRESPASS_EXECUTE needs a small structure, which is simply allocated here. 
        FLARE_TRESPASS_EXECUTE             mTrespassExecuteParams; 

        // We copy the parameters here on a set so that they are not changed by other
        // operations while the IOCTL is outstanding.
        FLARE_POWER_SAVING_POLICY          mPowerSavingPolicy;
    } u;

    // Waiting for completion of TrespassExecute Ioctl 
    ListOfBasicWaiter mListOfTrespassExecuteWaiters;

    // Waiting for completion of OwnershipLoss Ioctl
    ListOfBasicWaiter mListOfOwnershipLossWaiters;

    // Waiting for the next volume state.
    ListOfBasicWaiter mListOfVolumeStateWaiters;
};

#endif  // __BasicConsumedVolumeWithNotification__

