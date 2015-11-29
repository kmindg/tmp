/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2009 EMC Corporation.
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
#ifndef __BasicNotificationVolume__
#define __BasicNotificationVolume__

//++
// File Name:
//      BasicNotificationVolume.h
//
// Description:
//  Declaration of the BasicNotificationVolume class.
//
// Revision History:
//
// 2-Nov-09: Tom Rivera -- Created
//--
#include "BasicVolumeDriver/BasicVolume.h"
#include "RedirectorIRPInterface.h"
#include "BasicVolumeDriver/BasicVolumeState.h"

// A BasicNotificationVolume extends a BasicVolume by providing service for IRPs which
// register for notification. It also holds the current copy of the volume state.
//
// The sub-class is expected to provide the volume state to this class, by passing an
// instance of BasicVolumeState to UpdateVolumeStateReacquireLock().  It is this class which
// will orchestrate IRP handling and change notification.  The subclass may do other
// things, e.g., IOCTLs may be overridden, but this is not necessary.
// 
// Subclasses may choose to intercept FLARE_TRESPASS... IOCTL handling on arrival,
// completion, or both, while still maintaining the notification support in this class.
// The overriding of IOCTL_FLARE_GET_VOLUME_STATE, would typically be for the purpose of
// ensuring that UpdateVolumeStateReacquireLock() was called at least once.
// 
// This class services IOCTL_FLARE_GET_VOLUME_STATE by intercepting then delegating this
// IOCTL to the BasicVolume, where the BasicVolume will upcall to this class for each
// specific volume state field, in order to fill out the FLARE_VOLUME_STATE structure.  A
// sub-class of BasicNotificationVolume can override the IOCTL arrival, collect then
// provide the volume state via UpdateVolumeStateReacquireLock(), and then finally call
// BasicNotificationVolume::..IOCTL_FLARE_GET_VOLUME_STATE.
//
// This class will completely service:
//   IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION
//   IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION
//
// It will record the notification information in IOCTL_FLARE_TRESPASS_EXECUTE and clear
// the same info for IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS.
//
// Subclasses which want to be involved in trespass IOCTLs should normally override the
// CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS... functions, giving the subclass
// control after the notification work has been done. Such subclasses should call
// BasicNotificationVolume::CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS.. when they
// are done. An similar override of trespass IOCTL arrival is acceptable, if necessary.
class BasicNotificationVolume :
    public BasicVolume,
    public VolumeEventNotification 
{
public:

    // Constructor
    BasicNotificationVolume(BasicVolumeParams * volumeParams, BasicVolumeDriver * driver);

 

    // Sends notification of volume state changes back to client.
    // @param newerParams - the latest copy of the voloume state parameters.
    // @param forceNotify - trigger a notification even if newerParams are not different
    //                      than the older parameters.
    // Returns:
    //  - TRUE if client notifications are complete. 
    //  - FALSE if client notify is running but not yet completed.
    // Note: Notification can be triggered prior to  the size being known.
    //       In those cases the volumeSize may not be known and is left at zero, under the
    //       assumption that the client will only use the value presented if
    //       VolumeEventSizeChanged is set.
    bool UpdateVolumeStateReacquireLock(const BasicVolumeState &  newerParams,  bool forceNotify = false);

    // The default handling is for BasicVolume to fill in the parameters via calling the
    // ...ForVolumeStateReport functions, and complete the request. If the subclass
    // wishes, it can override this function, change the volume state via
    // UpdateVolumeStateReacquireLock, then call this function to complete the processing.
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_GET_VOLUME_STATE(PBasicIoRequest  Irp) {
        BasicVolume::IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_GET_VOLUME_STATE( Irp);
    }


    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE returns TRUE if this driver it
    // attemping to prevent TRESPASS requests.  Only meaningful when queried on SP that
    // last received an IOCTL_FLARE_TRESPASS_EXECUTE.
    virtual TRESPASS_DISABLE_REASONS GetTrespassDisableReasonsForVolumeStateReport() const { return mCurrentVolumeState.GetTrespassDisabledReasons(); }

    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE which provides the values to
    // report on GET_VOLUME_STATE.
    virtual VOLUME_ATTRIBUTE_BITS GetVolumeAttributesForVolumeStateReport() const { return GetVolumeAttributes(); }

    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE and IOCTL_DISK_GET_GEOMETRY
    // returns true if this driver believes that the volume is ready for I/O (or doesn't
    // know).
    virtual bool GetReadyForVolumeStateReport() const { return IsReady(); }
    
    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE and IOCTL_DISK_GET_GEOMETRY
    // which returns the current size of the volume.
    virtual DiskSectorCount LogicalUnitSize()  { return GetCurrentVolumeSize(); }

    virtual const BasicVolumeState & GetCurrentState() const { return mCurrentVolumeState; }


protected:

    // Implementation of pure virtual functions as defined in RedirectorIRPInterface.h
    virtual bool IsEventSignaled(VolumeEventSet events) const;
    virtual void AcknowledgeNotification() { /* Do nothing. */ }
    virtual ReservationAccess DetermineCurrentAccess(
        bool thisSpWantsSCSI2Reservation, bool & hasReservation);
    virtual ULONGLONG GetCurrentVolumeSize();
    virtual bool IsReady() const;
    virtual VOLUME_ATTRIBUTE_BITS GetVolumeAttributes() const;
    virtual VOID GetCurrentOwner(SPID & spid, bool & lastOwnershipChangeExplicit) const;


    // If the sub-class overrides any of the next 4 function functions, it should call
    // them when it has completed its operation. The same applies to overriding the
    // BasicVolume IOCTL_FLARE_TRESPASS... functions.

    // Override base class. Establish the notification callback, if any, then call
    // CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE
    virtual void IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(PBasicIoRequest Irp);

    // Override base class. Remove the notification callback, if any, then call
    // CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS
    virtual void IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(PBasicIoRequest Irp);

    //
    // The default implementation in BasicVolume is to call these functions 
    // 
    // If the sub-class overrides, it should call this version of the function to trigger
    // a call to the the top version of Complete...TRESPASS_EXECUTE(), whch should
    // eventually call this classes version. Gets control near the end of TRESPASS_EXECUTE
    // processing.  Must complete the Irp.
    virtual VOID CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(PBasicIoRequest Irp) {
        BasicVolume::CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(Irp);
    }

    // Gets control near the start of the service of TRESPASS OWNERSHIP LOSS.   If the
    // sub-class overrides, it should call this version of the function to trigger a call
    // to the the top version of Complete...TRESPASS_EXECUTE(), whch should eventually
    // call this classes version.
    virtual void CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(PBasicIoRequest Irp)
    {
        BasicVolume::CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(Irp);
    }


private:
    // Manages canceling of client IRP and triggers notification cleanup.
    static void NotificationRegistrationIrpCancelRoutine(PVOID Unused, PBasicIoRequest Irp);

    // Services this notification IRP completely, holding it until the next notification,
    // then completing it.  May complete immediately if a notification event recently
    // occured.  IRPs are cancelable.
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION(
        PBasicIoRequest  Irp);

    // Services this notification IRP completely.  IRPs are cancelable.
    virtual void IRP_MJ_DEVICE_CONTROL_IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION(PBasicIoRequest Irp);


    // List of registration IRPs received from clients above.
    ListOfCancelableBasicIoRequests mWaitingNotificationRegistrationIrps;

    // The state of the volume during the current/last notification.
    BasicVolumeState mCurrentVolumeState;

    // Used to temporarily store volume information passed along with the client volume
    // notify request sent from below.
    BasicVolumeState mNextVolumeState;

    // This is set whenever we have a notify initiated from a secondary source in the
    // driver. Allows us to trigger a second round of notifications up to the clients
    // above.
    bool mNotificationPending;

    // Saved on TRESPASS EXECUTE from above. Called during local notification. Cleared on
    // Ownership Loss.
    VOID (*mStateChangeNotify)(PVOID context);

    // Saved on TRESPASS EXECUTE from above, parameter to above.
    PVOID mStateChangeNotifyContext;

    // This is set when starting the notify, and until it is cleared any OWNERSHIP LOSS
    // IOCTL will need to be held until the notify completes.
    bool mNotifyInProgress;

    // Stores OWNERSHIP LOSS IOCTL in the event that a notify was in progress when it was
    // received.
    PBasicIoRequest mStateChangeNotifyPendingOwnershipLossIrp;
};

#endif //__BasicNotificationVolume__
