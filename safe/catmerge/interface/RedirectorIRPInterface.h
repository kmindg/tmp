/************************************************************************
*
*  Copyright (c) 2002, 2005-2006 EMC Corporation.
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


#ifndef __RedirectorIRPInterface_h__
#define __RedirectorIRPInterface_h__

//++
// File Name: RedirectorIRPInterface.h
//    
//
// Contents:
//      Defines the IOCTLs and classes of the RedirectorIRPInterface interface.  The SCSI3
//      reservation support is tentative, and needs more careful elaboration.
//

// Revision History:
//--
# include "SinglyLinkedList.h"
# include "ScsiTargGlobals.h"
# include "scsitarghostports.h"
# include "ScsiTargInitiators.h"
# include "Reservation_I_T_nexus.h"
# include "VolumeEventSet.h"

// Define the various device type values.  Note that values used by Microsoft Corporation
// are in the range 0-32767, and 32768-65535 are reserved for use by customers.  (Quoted
// from "/ddk/inc/devioctl.h".)
#define FILE_DEVICE_REDIRECTOR          0x80CD  // unique in devioctl.h


// IRP_MJ_DEVICE_CONTROL -
//      IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION 
//
// Register for a volume event callback with the redirector volume.  This IRP is directed
// at the local redirector volume, rather than the SP that is owner of the volume. The IRP
// sent to this volume will never complete normally.  The IRP will only complete when
// cancelled, and then never while there is an outstanding callback.  If the IRP is
// cancelled, so is the callback registration. Such callbacks occur in the context of the
// redirector, possibly at DISPATCH_LEVEL, and must not block.
//
// Return Status:
//    STATUS_CANCELLED - this is the only legal status.  Other statuses indicate that this
//                       IOCTL is not understood.
//
// Input: class IoctlInRedirectorRegisterForVolumeEventNotification (see below)
// Output: none.
const ULONG IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION =   
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x800, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);

// IRP_MJ_DEVICE_CONTROL 
//     - IOCTL_REDIRECTOR_GENERATE_VOLUME_EVENTS 
// Cause volume events to be broadcast to all volume events registrants.
//
// Return Status:
//    STATUS_SUCCESS
//
// Input: IoctlInRedirectorGenerateVolumeEvents  (see below)

//      
// Output: none.
const ULONG  IOCTL_REDIRECTOR_GENERATE_VOLUME_EVENTS        =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x801, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);


// IRP_MJ_DEVICE_CONTROL 
//     - IOCTL_REDIRECTOR_CHANGE_RESERVATION 
//
// Change the reservation state for the specific redirector volume this IRP is directed
// to.
//
// Return Status:
//    NTSUCCESS() - The reservation state was changed, and a volume event notification to
//    that effect was broadcast to volume event notification registrants. !NTSUCCESS() -
//    The reservation state was not changed
//    
//
// Input: IoctlInRedirectorChangeReservation (see below)
// Output: none.
const ULONG IOCTL_REDIRECTOR_CHANGE_RESERVATION     =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x802, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);

// IRP_MJ_DEVICE_CONTROL 
//     - IOCTL_REDIRECTOR_QUIESCE
//
// Sets the RedirectorVolume's ConsumedVolume so that new IRPs will be held, and closes
// the consumed volume.  Held IRPs may be cancelled.
//
// Return Status:
//    TBD 
//    
//
// Input: none.
// Output: none.
const ULONG IOCTL_REDIRECTOR_QUIESCE        =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x803, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);

// IRP_MJ_DEVICE_CONTROL - IOCTL_REDIRECTOR_SET_CONSUMED_DEVICE_NAME
//
// Replaces the device name used for the consumed device.  Should be used on when device
// is quiesced.
//
// Return Status:
//    TBD 
//    
// Input: NULL terminated ascii string, which is the device object name of the device
//        to open, replacing the previous name (e.g.,
//        IOCTL_AIDVD_INSERT_DISK_VOLUME_PARAMS::LowerDeviceName).
// Output: none.
const ULONG IOCTL_REDIRECTOR_SET_CONSUMED_DEVICE_NAME       =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x804, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);

// IRP_MJ_DEVICE_CONTROL - IOCTL_REDIRECTOR_UNQUIESCE
//
// Undoes IOCTL_REDIRECTOR_QUIESCE.  Opens the consumed device, submits any waiting IRPs.
//
// Return Status:
//    success - the device was unquiesced and opened
//    failure - the device was unquiesced but the open failed
//    
// Input: none.
// Output: none.
const ULONG IOCTL_REDIRECTOR_UNQUIESCE      =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x805, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);

// IRP_MJ_DEVICE_CONTROL - IOCTL_REDIRECTOR_GET_OWNERSHIP 
//
// Returns the SP that is the current owner for the volume's gang.
//
// Return Status:
//    NTSUCCESS() 
//    
//
// Input: None. 
// Output: SPID - the SPID which should be the new owner.
const ULONG IOCTL_REDIRECTOR_GET_OWNERSHIP  =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x806, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);

// IRP_MJ_DEVICE_CONTROL - IOCTL_REDIRECTOR_SET_OWNERSHIP 
//
// Sets which SP should be owner for the volume's gang.
//
// Return Status:
//    NTSUCCESS() - The desired volume is now the owner.  !NTSUCCESS() - The
//    transition of ownership is not possible, or the IOCTL was not understood.
//    
//
// Input: RedirectorSetOwnershipParams - the SPID of the desired owner plus other parameters. 
// Output: none.
const ULONG IOCTL_REDIRECTOR_SET_OWNERSHIP  =
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_REDIRECTOR, 0x807, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS);


// Input data for IOCTL_REDIRECTOR_SET_OWNERSHIP.
struct RedirectorSetOwnershipParams {
    // The SP which is desired to be owner.
    SPID                    DesiredOwnerSPId;

    // For each reason (i.e., driver) which may be asking for ownership locking to 
    // be suppressed, this field has a corresponding bit.  If this bit is clear, then
    // ownership should not be moved from the owner if the owner is asserting this lock
    // reason.  If this bit is set, ignore that ownership lock reason.
    //
    // Therfore, if this field is 0, then all ownership locks are obeyed, if ~0, ownership 
    // locking will occur regardless.
    ULONG TrespassDisableReasonsToIgnore:16;

    // Informs IoRedirector the reason for issuing SET_OWNER. IoRedirector will use this value 
    // to set the lastTrespassType. Currently only SET_OWNERSHIP_DUE_TO_SHUTDOWN is used by IoRedirector, 
    // the rest of the #defines are place holder.

    ULONG TrespassReason:16;

    // Spare
    ULONG spare; 

};

// The VolumeState class provides an interface to query the current state of the volume.
class VolumeState  {
public:

    // Defines the kind of access allowed based on reservations held.
    enum ReservationAccess { 
        NoAccess = 0, 
        Read = 1,
        Write = 2,
        ReadWrite = 3 
    };

    // Determine what access the current SP is allowed for the volume.
    //
    // @param thisSpWantsSCSI2Reservation  - this SP would like to have the reservation
    // @param hasReservation - returned as TRUE if this SP has the reservation.
    //
    // Returns: The current access rights for all ITLs on this SP have for this volume.
    //
    // The result of this call may change on these events:
    //   - VolumeEventReservationChange
    virtual ReservationAccess  DetermineCurrentAccess(bool thisSpWantsSCSI2Reservation, bool & hasReservation) = 0;

    // Returns the current size of the volume in bytes.
    //
    // The result of this call may change on these events:
    //   - VolumeEventSizeChanged
    virtual ULONGLONG   GetCurrentVolumeSize() = 0;

    // Returns whether the volume is currently Ready.
    // 
    // The result of this call may change on these events:
    //   - VolumeEventReadyStateChanged
    virtual bool        IsReady() const = 0;

    // Returns the current volume attributes.
    // 
    //The result of this call may change on these events:
    //   - VolumeEventAttributesChanged
    //   - VolumeEventOwnerChange
    virtual VOLUME_ATTRIBUTE_BITS        GetVolumeAttributes() const = 0;

    // Queries for information about the current gang owner.
    // @param spid - returns the current owner of the gang.
    // @param lastOwnershipChangeExplicit - returned as true if the last ownership change
    //                                      was due to an explicit trespass, false if done
    //                                      implicitly due to load.
    // 
    // The result of this call may change on these events:
    //   - VolumeEventOwnerChange
    virtual VOID        GetCurrentOwner(SPID & spid, bool & lastOwnershipChangeExplicit) const = 0;
  
    // Try to make this SP the owner.
    virtual VOID TryToAcquireOwnership() {};
};


// A VolumeEventNotification describes one or more events which have just occurred on a
// volume. It is assumed that when notified that there are VolumeEvents, the function
// being notified will query for each type of event it is interested in whether the event
// has been signaled.
//
// VolumeEventNotification always contain the VolumeState at the time of the event
// notification.
class VolumeEventNotification : public VolumeState {
public:



    // Query as to whether any of the specified events was just signaled.
    //
    // @param events - the events the query is about.
    //
    // Returns: true if any event was signalled since the last notification,
    //      false otherwise.
    virtual bool    IsEventSignaled(VolumeEventSet events) const = 0;

    // If a VolumeEventNotification is not immediately acknowledged (i.e.,
    // NotificationCallback() did not return true), then it must be explicitly
    // acknowledged later. by calling this function.
    //
    // This function may be called when IRQL <= DISPATCH_LEVEL.
    virtual void AcknowledgeNotification() = 0;

    // VolumeEventNotification is passed as a token.  The holder of the token can queue
    // this object.
    VolumeEventNotification *                    mVolumeEventNotificationLink;

};
    
// VolumeEventNotificationRegistrationRequest defines an interface for notifications as an
// abstract base class. A pointer to a VolumeEventNotificationRegistrationRequest is
// passed in with a IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION. Drivers that
// implement VolumeEventNotifications will hold onto the IRP containing this object until
// that IRP is cancelled.  The IRP will never complete if the NotificationCallback() was
// called and that call has not been acknowledged.  Therefore there can never be a call to
// NotificationCallback() in progress unless the IRP is outstanding.
//
// When VolumeEvents occur, the member function NotificationCallback() will be called,
// providing a signal that the event has occurred.  A VolumeEvent can either be completely
// handled in NotificationCallback(), or the completion of handling the volume event can
// be deferred, blocking new volume event notifications.
// 
class VolumeEventNotificationRegistrationRequest  {
public:
    // Query as to which events the requestor is interested in.  For a given parameter
    // value, the result of this function should not change while a registration is
    // outstanding. The requestor must allow notifications where none of events it is
    // "InterestedIn"  were signaled.
    //
    // This function may be called when IRQL <= DISPATCH_LEVEL.
    //
    // @param eventSet - the set of events being queried about.
    //
    // Return Value: true if the requestor is soliciting any event in the the set, false
    //    if no events in the specified set are being solicited.
    virtual bool InterestedIn(VolumeEventSet eventSet) const = 0;

    virtual ~VolumeEventNotificationRegistrationRequest() {}


    // This callback will be made when any of the events are signaled on the volume. If
    // this->InterestedIn(VolumeEventNotification::InitialNotification) == true, then a
    // notification will occur immediately upon registration to allow evaluation of the
    // current state.
    //
    // The implementation of NotificationCallback() must acknowledge this call by either:
    //   1) returning true
    //   2) returning false, and calling notification->AcknowledgeNotification() from a
    //      different (non-recursive) context.
    //
    // @param notification - describes the events that have occurred. 
    //
    // This function may be called when IRQL <= DISPATCH_LEVEL.  This function should not
    // block the calling thread.
    //
    // Note: NotificationCommit() will be called once after this call has been made to
    // indicate that NotificationCallback() has been called on every SP.
    //
    // Return Value: 
    //    true - all work for this notification is completed. Equivalent to calling
    //           notification->AcknowledgeNotification() without the recursion risk.
    //    false - notification->AcknowledgeNotification() will be call at a later time.
    //            notification->mLink is for use by the current holder of notification.
    //         
    //            
    virtual bool NotificationCallback(VolumeEventNotification * notification) = 0;

    // This callback will be made once for every NotificationCallback(), after 
    // NotificationCallback() has been called on every SP. This allows the receiver to implement
    // transactional semantics to avoid the appearance of state regression. 
    virtual void NotificationCommit(void) = 0;
};

// The data passed in with an IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION IRP
struct IoctlInRedirectorRegisterForVolumeEventNotification {
    // A System Virtual Address, usable by all system threads.  This indicates the object
    // to notify on on events.
    // Note: registration for volume events is SP local.
    VolumeEventNotificationRegistrationRequest * mObjectToNotify;
};




// The data passed in with an IOCTL_REDIRECTOR_GENERATE_VOLUME_EVENTS IRP
struct IoctlInRedirectorGenerateVolumeEvents {

    // The set of volume events to generate a notification about.
    //      0 - indicates that layered driver ownership must be established
    //          and any pending events notified.
    // If any event in VolumeEventsRequiringLayeredDriverOwnership is set,
    // then layered driver ownership must be established, along with notification
    // of any other pending events.
    VolumeEventSet      mEvents;

    // Spare
    ULONG spare;
};



// The data passed in with an IOCTL_REDIRECTOR_CHANGE_RESERVATION IRP. Note that since
// this is marshaled and transferred to the volume owner, this structure must not contain
// any pointers.
struct IoctlInRedirectorChangeReservation {

    // The SP that this request is being made on behalf of.
    SPID    mRequestingSP;

    // The kind of reservation operation.
    enum ReservationChangeType {
        // A SCSI 2 reserve is for a single ITL.
        SCSI2Reserve=1,

        // A SCSI 2 release is for a single ITL.
        SCSI2Release=2,
    
        // A SCSI2 Force Reserve might not be required.
        SCSI2ForceReserve=3,

        // The ITL has just logged in (needed for group reservations).
        Login=4,

        // The ITL has just logged out. Releases any SCSI 2 reservation.
        // Note: might only called if a reservation was acquired.
        Logout=5,

        // SCSI-3 persistent reservations only need registration and reservation keys.
        // More description should be added when these are actually used.
        SCSI3Register=6,
        SCSI3Reserve=7,
        SCSI3Release=8,
        SCSI3Clear=9,
        SCSI3Preempt=10,
        SCSI3PreemptAndAbort=11,
        SCSI3RegisterAndIgnoreExistingKey=12
    };

    ReservationChangeType   mReservationType;

    ULONG spare; 
# if 0
    // [SPC-3] "The RESERVATION KEY field contains an 8-byte value provided by the
    // application client to the device server to identify the I_T nexus that is the
    // source of the PERSISTENT RESERVE OUT command. The device server shall verify that
    // the contents of the RESERVATION KEY field in a PERSISTENT RESERVE OUT command
    // parameter data matches the registered reservation key for the I_T nexus from which
    // the command was received, except for:
    // a - The REGISTER AND IGNORE EXISTING KEY service action where the RESERVATION KEY
    //     field shall be ignored; and
    // b -  The REGISTER service action for an unregistered I_T nexus where the
    //      RESERVATION KEY field shall contain zero."
    ULONGLONG           mReservationKey;
    
    // The Activate Persist Through Power Loss (APTPL) bit is valid only for the REGISTER
    // service action and the REGISTER AND IGNORE EXISTING KEY service action, and shall
    // be ignored for all other service actions.
    //  After the application client enables the persist through power loss capability the
    //  device server shall preserve the
    // persistent reservation, if any, and all current and future registrations associated
    // with the logical unit to which the REGISTER service action, the REGISTER AND IGNORE
    // EXISTING KEY service action, or REGISTER AND MOVE service action was addressed
    // until an application client disables the persist through power loss capability. The
    // APTPL value from the most recent successfully completed REGISTER service action,
    // REGISTER AND IGNORE EXISTING KEY service action, or REGISTER AND MOVE service
    // action from any application client shall determine the logical unit’s behavior in
    // the event of a power loss.
    bool                mActivatePersistThroughPowerLoss;
    

    // The SERVICE ACTION RESERVATION KEY field contains information needed for the
    // following service actions:
    //   REGISTER, REGISTER AND IGNORE EXISTING KEY, PREEMPT, and PREEMPT AND ABORT.
    ULONGLONG           mServiceActionReservationKey;
# endif
};


SLListDefineListType(VolumeEventNotification, ListOfVolumeEventNotification);
SLListDefineInlineCppListTypeFunctions(VolumeEventNotification, ListOfVolumeEventNotification, mVolumeEventNotificationLink);

#endif  // __RedirectorIRPInterface_h__

