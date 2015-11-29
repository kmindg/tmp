/************************************************************************
 *
 *  Copyright (C) EMC Corporation, 2000 - 2011.
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

#ifndef __BasicVolume__
#define __BasicVolume__

//++
// File Name:
//      BasicVolume.h
//
// Contents:
//      Defines the BasicVolume abstract base class, along with the HashListOfBasicVolume
//      class and the VolumeIdentifier type.
//
// Revision History:
//--
#include "cppcmm.h"
#include "cppshdriver.h"


extern "C" {
#include "ktrace.h"
};
# include "K10AnsiString.h"
# include "k10defs.h"
# include "K10SpinLock.h"
# include "flare_export_ioctls.h"
# include "BasicLib/ConfigDatabase.h"

# include "K10HashTable.h"
# include "CmiUpperInterface.h"
# include "InboundMessage.h"
# include "VolumeMultiplexor.h"
# include "DiskSectorCount.h"
# include "BasicLib/BasicObjectTracer.h"
# include "BasicLib/BasicDeviceObject.h"
# include "BasicLib/BasicIoRequest.h"
# include "BasicLib/BasicBinarySemaphore.h"
# include "BasicVolumeDriver/BasicVolumeParams.h"

// ENHANCE: Csx is supposed to have this define.
#if !defined(CSX_CPP_OVERRIDE) 
#define OVERRIDE override
# else
#define OVERRIDE CSX_CPP_OVERRIDE
# endif

class BasicVolume;
class BasicVolumeDriver;

// A small integer used to indicate the position of the volume in an array.
typedef UINT_32 VolumeIndex;



// BasicVolume is an abstract base class.  A BasicVolume contains the generic methods
// attributes needed for a volume exported by a K10 "layered driver".
//
// There is a 1:1 relationship between an instance of a BasicVolume and a WDM
// DEVICE_OBJECT.  When an instance of a subclass of BasicVolume is created, a
// DEVICE_OBJECT is also created, and DEVICE_OBJECT::DeviceExtension is used as the memory
// for BasicVolume and its subclass.
//
// Creation follows the following sequence:
//  - The WDM DEVICE_OBJECT is created
//  - The constructor for the subclass of BasicVolume is called, passing parameters. That
//    constructor calls the BasicVolume constructor.
//     
//  - The volume is added to the BasicVolume hash table.
//  - Symbolic link names, if any, are added via WDM calls.
//  - [booting only] DriverEntryAfterConstructingDevices()
//  - InitializeVolumeReleaseLock()
//  - The DO_DEVICE_INITIALIZING flag in the device object is cleared, allowing I/O.
// 
// WDM IRPs are received by the driver for a particular DEVICE_OBJECT, and these are
// dispatched to the associated BasicVolume via the BasicDeviceObject class. The subclass
// can choose to:
//   - Allow BasicVolume to completely service the IRP
//   - Allow BasicVolume to partially service the IRP, but provide details by overriding
//     some specific functions.
//   - Completely service the IRP by overriding the IRP handler.
//   - Override the IRP handler, but then call BasicVolume functions from that handler.
//   - Queue the IRP to a BasicDeviceObjectIrpDispatcherThread in the IRP handler, and
//     override BasicDeviceObject::DispatchQueuedIRP().  There could be one thread per
//     volume, one thread per driver, one thread per MajorFunction, etc., as appropriate
//     for the particular driver.
//
// The BasicVolume will, unless overridden by the sub-class, handle every IRP.  "Handling"
// may mean "return with error status".
//
// A BasicVolume handles IRPs arriving from upper drivers, but it has no concept of device
// objects that might logically be below its DEVICE_OBJECT in the device stack.  It is the
// sub-class's full responsibility to implement such a driver.
//
// There is a SpinLock per Volume that is used as part of the implementation of
// BasicVolume that is available for use by the subclass.
//      
class BasicVolume : public BasicDeviceObject, public BasicObjectTracer {

public:

    // Create a BasicVolume, where the following is true:  this ==
    // DeviceObject->DeviceExtension.
    // 
    // Must be called in the sub-class's constructor, and passed the same arguments.
    // @param volumeParams - a pointer to parameters describing the persisten volume
    //                       state. The lifetime of the memory for those parameters must
    //                       be at least as long as the volume itself.
    // @param driver - The driver that this volume is part of.
    BasicVolume(BasicVolumeParams * volumeParams, BasicVolumeDriver * driver);

    //   Called as part of device creation, but before the DEVICE_OBJECT::Flags
    //   DO_DEVICE_INITIALIZING bit is cleared.  The volume does not exist prior to this
    //   function returning or VolumeNowExists() is called.
    //
    //   No messages or IRPs may arrive on this volume until it "exists".  It is therefore
    //   important that this function does nothing to announce the volume's existance
    //   unless it previously calls VolumeNowExists(). For example, it should not send any
    //   CMI messages which might stimulate a response from another SP.
    //
    // The subclass must implement this function. 
    //
    // The BasicVolume spinlock is held when called, and the implementation of this
    // function must release the lock in all code paths.
    //
    // Return Value:
    //     STATUS_SUCCESS <other> error (resource allocation failure )    
    virtual EMCPAL_STATUS InitializeVolumeReleaseLock() = 0;

    // Get the Open/Close semaphore.  The calling thread may block, therefore it must be
    // at PASSIVE_LEVEL. The Open/Close semaphore is used to serialize volume open and
    // closes..
    VOID AcquireOpenCloseSemaphore();

    // Release the Open/Close semaphore.  
    VOID ReleaseOpenCloseSemaphore();


public:   // These are internal BVD interfaces, not meant to be called externally.  

    // Determine if these parameters are acceptable during an update trasnaction.  This
    // will check with the sub-class via EventValidateVolumeParamsForUpdateLockHeld().
    // NOTE: there is no guarantee that there will be futher calls, even if true is
    // returned.
    // @param volumeParams - a sub-class of BasicVolumeParams, defined for the specific driver.
    // @param paramsLen - the number of bytes in the subclass of basic volume params. 
    // Returns: true - validation success, false - validation failed, abort transaction.
    bool ValidateVolumeParamsForUpdateLockHeld(BasicVolumeParams * volumeParams, ULONG paramsLen);

    // The volume parameters have been changed.  This will notify the sub-class via
    // EventVolumeParamsUpdatedReleaseLock().
    void UpdateVolumeParamsReleaseLock(BasicVolumeParams * volumeParams, ULONG paramsLen);

    // During the intention phase of the transaction, this method is called to prepare for
    // deletion. Queries the sub-class via:  EventPrepareVolumeForDelete()
    // NOTE: there is no guarantee that there will be futher calls, even if true is
    // returned.  If the volume is deleted, notification will be via the destructor.
    // Returns: true - validation success, false - validation failed, abort transaction.
    bool PrepareVolumeForDelete();

    // Verify that the driver action params are acceptable to BVD. Calls
    // EventValidateVolumeParamsForCreate() which the sub-class can override.
    // @param IsInitialingSP - is this the SP on which the transaction arrived
    // @param DriverAction - a sub-class of BasicDriverAction, defined for the specific
    //                       driver.
    // @param newLen - the number of bytes in the subclass of BasicDriverAction
    // Returns: true if the parameters are acceptable, false otherwise.
    bool VolumeActionValidateReleaseLock(BOOLEAN IsInitialingSP, const BasicVolumeAction * VolAction, ULONG Len);

    // Routine to handle PERFORM_VOLUME_ACTION IOCTL Takes the action specified on the
    // driver. Calls EventPerformVolumeActionReleaseLock() on the specific volume. which
    // the sub-class can override.
    //
    // WARNING: This call occurs prior to the persistence of any parameters, allowing this function
    // to indicate that persistence is required.  Care should be taken to ensure that externally 
    // visible actions occur in UpdateVolumeParamsReleaseLock rather than here, because if there is a crash,
    // the transaction may abort, and the BasicVolumeParams will revert to the state prior to the transaction.
    //
    // @param IsInitialingSP - is this the SP on which the transaction arrived
    // @param VolumeAction - a sub-class of BasicVolumeAction, defined for the specific
    //                       driver.  Contains the WWN of the volume to perform the action
    //                       on.
    // @param Len - The number of bytes passed in for the subclass of VolumeAction
    // @params volumeParamsBuffer - Memory to use to optionally store a new copy of the volume params. See return value.
    // Returns: 
    //    0 - No volume params update.
    //   >0 - Indicates a copy of the volume parameters has been copied to volumeParamsBuffer, and that
    //        UpdateVolumeParamsReleaseLock() should be called after these parameters have been stored.
    ULONG PerformVolumeActionReleaseLock(BOOLEAN IsInitiatingSP, BasicVolumeAction * VolumeAction, ULONG Len, 
        BasicVolumeParams * volumeParamsBuffer);

    // Fill in the status for this volume, then query the sub-class via
    // EventQueryVolumeStatusReleaseLock()
    EMCPAL_STATUS QueryVolumeStatus(BasicVolumeStatus * ReturnedVolumeStatus,
                                              ULONG MaxVolumeParamsLen, ULONG & OutputLen);
    
    // Mark the volume as delete pending, to prevent further opens. The open/close
    // semaphore must be held.
    void MarkDeletePendingSemHeld();

   
public:

    //////////////// This section contains IRP handlers and their modifiers.  ////////////
    // BasicVolume has an implementation for each IRP handler.  That IRP handler may have
    // modifiers which call back into the subclass.

    // 
    //     IRP_MJ_CREATE Handler
    // 
    //    Called only at PASSIVE_LEVEL per WDM rules.  
    //
    //    Typically, the sub-class will not override this, but implement
    //    IRP_MJ_CREATE_ModifierFirstOpen instead.
    //
    // @param Irp - the IRP requesting the create.
    //
    //    Default implementation, maintains the count of the current number of opens of
    //    the device object. The default handler will call IRP_MJ_CREATE_ModifierFirstOpen
    //    when the open count transitions from 0 to 1. Completes the IRP when done.
    // Return Value: Status of volume creation.
    virtual EMCPAL_STATUS     IRP_MJ_CREATE_Handler(PBasicIoRequest  Irp);

protected:
    // 
    //     IRP_MJ_CREATE Modifier
    // 
    //    Called only at PASSIVE_LEVEL when the open count for this device transitions
    //    from 0 to 1.
    virtual EMCPAL_STATUS IRP_MJ_CREATE_ModifierFirstOpen() = 0;


    /// Copies data from member variables to BasicVolumeStatus structure
    /// Derived class can override this function to copy it's own parameters
    //
    // @param status - BasicVolumeStatus struct to copy parameters
    //
    virtual void CopyVolumeStatus(BasicVolumeStatus& status) const;

public:    
    // 
    //     IRP_MJ_CLEANUP
    //  
    //    Default implementation simply completes the IRP, so the sub-class may override.
    //     
    // @param Irp - the IRP requesting the IRP_MJ_CLEANUP.
    // 
    virtual VOID IRP_MJ_CLEANUP_Handler(PBasicIoRequest  Irp);

    // 
    //     IRP_MJ_CLOSE Handler
    // 
    //    Called only at PASSIVE_LEVEL per WDM rules.
    //
    //    Typically, the sub-class will not override this, but implement
    //    IRP_MJ_CLOSE_ModifierLastClose instead.
    //
    // @param Irp - the IRP requesting the create.
    //
    //    Default implementation, maintains the count of the current number of opens of
    //    the device object. The default handler will call IRP_MJ_CLOSE_ModifierLastClose
    //    when then open count transitions from 1 to 0. Completes the IRP when done.
    virtual VOID IRP_MJ_CLOSE_Handler(PBasicIoRequest  Irp);

protected:
    // 
    //     IRP_MJ_CLOSE Modifier
    // 
    //    Called only at PASSIVE_LEVEL when the create count for this device transitions
    //    from 1 to 0.
    virtual VOID IRP_MJ_CLOSE_ModifierLastClose() = 0;

public:
    // 
    //     IRP_MJ_READ_Handler Handler
    //
    // The default implementation completes the IRP with an error. Typically the sub-class
    // will override this function.
    // 
    // @param Irp - the IRP to be handled.
    virtual VOID IRP_MJ_READ_Handler(PBasicIoRequest  Irp) {
        Irp->CompleteRequest( EMCPAL_STATUS_NOT_IMPLEMENTED);
    }
    
    // 
    //     IRP_MJ_WRITE_Handler Handler
    //
    // The default implementation completes the IRP with an error. Typically the sub-class
    // will override this function.
    //
    // @param Irp - the IRP to be handled.
    virtual VOID IRP_MJ_WRITE_Handler(PBasicIoRequest  Irp) {
        Irp->CompleteRequest( EMCPAL_STATUS_NOT_IMPLEMENTED);
    }
    

    // IRP_MJ_INTERNAL_DEVICE_CONTROL handlers. 


    // Handles an arriving IRP_MJ_INTERNAL_DEVICE_CONTROL.
    //
    // The driver's handler for IRP_MJ_INTERNAL_DEVICE_CONTROL cannot be overridden, and
    // dispatches the BasicVolume's IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler().
    //
    // The default implementation of IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler() calls
    // IoMarkIrpPending and then dispatches to the IRP_MJ_INTERNAL_DEVICE_CONTROL_*
    // functions, using IO_STACK_LOCATION::MinorFunction to dispatch.
    // IRP_MJ_INTERNAL_DEVICE_CONTROL_Other is the catchall function for unrecognized
    // IO_STACK_LOCATION::MinorFunction's.
    //
    // A volume that wants to trap an IOCTL not currently dispatched by this function
    // should override this function, handle that IOCTL, but call
    // BasicVolume::IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler for all other IOCTLs.  This
    // allows the default function to add cases without changing the sub-class's behavior.
    //
    // @param Irp - The IRP being dispatched.
    virtual EMCPAL_STATUS IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler(PBasicIoRequest  Irp);

    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_DCA_READ
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DCA_READ(PBasicIoRequest  Irp)
    {
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }
    
    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_DCA_WRITE
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // The sub-class must override this function if it supports DCA WRITE.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DCA_WRITE(PBasicIoRequest  Irp){
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }

    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_SGL_READ
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_SGL_READ(PBasicIoRequest  Irp)
    {
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }
    
    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_SGL_WRITE
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // The sub-class must override this function if it supports SGL WRITE.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_SGL_WRITE(PBasicIoRequest  Irp){
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }

    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_DM_SOURCE
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DM_SOURCE(PBasicIoRequest  Irp)
    {
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }
    
    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_DM_DESTINATION
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // The sub-class must override this function if it supports DCA WRITE.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_DM_DESTINATION(PBasicIoRequest  Irp){
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }

    // Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when IO_STACK_LOCATION::MinorFunction
    // == FLARE_NOTIFY_EXTENT_CHANGE
    //
    // The default implementation calls IRP_MJ_INTERNAL_DEVICE_CONTROL_Other. 
    //
    // The sub-class must override this function if it supports receiving
    // a notify extent change notification.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_FLARE_NOTIFY_EXTENT_CHANGE(PBasicIoRequest  Irp){
        IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(Irp);
    }

    //     Handler for IRP_MJ_INTERNAL_DEVICE_CONTROL, when
    //     IO_STACK_LOCATION::MinorFunction != {FLARE_DCA_{READ,WRITE}
    //
    // The default implementation completes the IRP with an error.
    //
    // The subclass must override this function if it wants to do anything else with
    // IRP_MJ_INTERNAL_DEVICE_CONTROL IRPs not specified above. However, it should not
    // have specific handling of IOCTLs not currently dispatched by the default handler
    // IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler.  Instead, IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler()
    // should be overridden, those IOCTLs handled there, and
    // BasicVolume::IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler() should be called for other
    // IOCTLs.  This allows the base class to add support for new IOCTLs without
    // disturbing the behavior of this subclass.
    // 
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_INTERNAL_DEVICE_CONTROL_Other(PBasicIoRequest  Irp) { 
        Irp->CompleteRequest( EMCPAL_STATUS_NOT_IMPLEMENTED); 
    }


    // IRP_MJ_DEVICE_CONTROL handlers. 


    // Handles an arriving IRP_MJ_DEVICE_CONTROL.
    //
    // The driver's handler for IRP_MJ_DEVICE_CONTROL cannot be overridden, and dispatches
    // the BasicVolume's IRP_MJ_DEVICE_CONTROL_Handler().
    //
    // The default implementation of IRP_MJ_DEVICE_CONTROL_Handler() calls
    // IoMarkIrpPending and then dispatches to the IRP_MJ_DEVICE_CONTROL_* functions,
    // using IO_STACK_LOCATION::Parameters.DeviceIoControl.IoControlCode to determine
    // where to dispatch to. IRP_MJ_DEVICE_CONTROL_Other is the catchall function for
    // unrecognized IoControlCodes.
    //
    // A driver that wants to trap an IOCTL not currently dispatched by this function
    // should override this function, handle that IOCTL, but call
    // BasicVolume::IRP_MJ_DEVICE_CONTROL_Handler for all other IOCTLs.  This allows the
    // default function to add cases without changing the sub-class's behavior.
    //
    // @param Irp - The IRP being dispatched.
    virtual EMCPAL_STATUS IRP_MJ_DEVICE_CONTROL_Handler(PBasicIoRequest  Irp);

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_FLARE_TRESPASS_EXECUTE.
    //
    // A subclass can override the IRP arrive, getting control of the IRP on arrival, or
    // it can override the Complete handler, getting control a bit later.  This difference
    // allows intermediate classes, such as BasicNotificationVolume to service the 
    // operation on arrival, then allows its sub-class to continue processing of it.
    //
    // In general, if the sub-class overrides, it should call the specific instance of the
    // function it overrode when it is done (e.g., BasicNotificationVolume::...).
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(PBasicIoRequest  Irp) {
        CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(Irp);
    }
    virtual VOID CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_EXECUTE(PBasicIoRequest Irp) {
        Irp->CompleteRequest(EMCPAL_STATUS_SUCCESS);
    }


    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS.
    //
    // The default handler completes the IRP immediately, with STATUS_SUCCESS.
    //
    // In general, if the sub-class overrides, it should call the specific instance of the
    // function it overrode when it is done (e.g., BasicNotificationVolume::...).
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual void IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(PBasicIoRequest Irp)
    {
        CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(Irp);
    }
        
    virtual void CompleteIRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS(PBasicIoRequest Irp){
        Irp->CompleteRequest(EMCPAL_STATUS_SUCCESS);
    }

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_FLARE_CAPABILITIES_QUERY.
    //
    // The default handler completes the IRP immediately, after filling in the answer
    // based on the result of the modifiers SupportsDcaRead() and SupportsDcaWrite().  An
    // error status will be returned if the output buffer is not large enough.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_CAPABILITIES_QUERY(PBasicIoRequest  Irp);


    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_DISK_GET_DRIVE_GEOMETRY.
    // 
    // The default handler completes the IRP immediately with success. An error status
    // will be returned if the output buffer is not large enough. Default Handlers use
    // modifiers LogicalUnitSize(), MediaType(), and BytesPerSector() to calculate answer.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_DISK_GET_DRIVE_GEOMETRY(PBasicIoRequest  Irp);

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_FLARE_GET_VOLUME_STATE.
    //
    // The default handler completes the IRP immediately with success. An error status
    // will be returned if the output buffer is not large enough. Default Handlers use
    // modifiers LogicalUnitSize(), MediaType(), and BytesPerSector() to calculate answer.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_GET_VOLUME_STATE(PBasicIoRequest  Irp);

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_DISK_IS_WRITABLE.
    // 
    // The default handler completes the IRP immediately with success. An error status
    // will be returned if the output buffer is not large enough. Default Handlers use
    // modifier IsVolumeWritable() to calculate answer.
    //
    // The sub-class may override this function.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_DISK_IS_WRITABLE(PBasicIoRequest  Irp);

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals IOCTL_DISK_VERIFY.
    //
    // The default handler completes the IRP immediately with success. An error status
    // will be returned if the input buffer is not large enough.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_DISK_VERIFY(PBasicIoRequest  Irp);

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals IOCTL_FLARE_MODIFY_CAPACITY
    //
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_MODIFY_CAPACITY(PBasicIoRequest  Irp) {
        IRP_MJ_DEVICE_CONTROL_Other(Irp);
    }

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED
    //
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED(PBasicIoRequest  Irp) {
        IRP_MJ_DEVICE_CONTROL_Other(Irp);
    }

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals IOCTL_FLARE_ZERO_FILL
    //
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_ZERO_FILL(PBasicIoRequest  Irp) {
        IRP_MJ_DEVICE_CONTROL_Other(Irp);
    }

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_FLARE_GET_RAID_INFO.
    //
    // The default handler completes the IRP immediately with success. An error status
    // will be returned if the output buffer is not large enough. Default Handlers use
    // modifiers RaidInfo to calculate answer.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_GET_RAID_INFO(PBasicIoRequest  Irp);

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION.
    //
    // The default handler completes the IRP immediately with an error.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION(
        PBasicIoRequest  Irp) 
    {
        Irp->CompleteRequest( EMCPAL_STATUS_NOT_IMPLEMENTED); 
    }

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals
    // IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION.
    //
    // The default handler completes the IRP immediately with an error.
    //
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual void IRP_MJ_DEVICE_CONTROL_IOCTL_REDIRECTOR_REGISTER_FOR_VOLUME_EVENT_NOTIFICATION(PBasicIoRequest Irp)
    {
        Irp->CompleteRequest( EMCPAL_STATUS_NOT_IMPLEMENTED); 
    }

    // Services IRP_MJ_DEVICE_CONTROL IRPs whose IoControlCode equals none of the above. 
    //
    // The default handler completes the IRP immediately with an error.
    //
    // To follow the normal protocol of forwarding this request to the next driver, the
    // subclass must override this method.
    // 
    // The subclass must override this function if it wants to do anything else with
    // IRP_MJ_INTERNAL_DEVICE_CONTROL IRPs not specified above. However, it should not
    // have specific handling of IOCTLs not currently dispatched by the default handler
    // IRP_MJ_INTERNAL_DEVICE_CONTROL.  Instead, IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler()
    // should be overridden, those IOCTLs handled there, and
    // BasicVolume::IRP_MJ_INTERNAL_DEVICE_CONTROL_Handler() should be called for other
    // IOCTLs.  This allows the base class to add support for new IOCTLs without
    // disturbing the behavior of this subclass.
    //
    // @param Irp - the IRP to be handled (already marked Pending).
    virtual VOID IRP_MJ_DEVICE_CONTROL_Other(PBasicIoRequest  Irp) { 
        Irp->CompleteRequest( EMCPAL_STATUS_NOT_IMPLEMENTED); 
    }


    // Get a unique number for this volume. This value must not change across the lifetime of the volume.
    // Returns: the volume index.  
    VolumeIndex GetVolumeIndex() const { return mVolumeIndex; }



protected:  ///////////  Modifier functions for default IRP handlers ///////

    // Modifier functions are called by default IRP handlers, and can be overridden by the
    // subclass without requiring the entire subclass to handle all IRPs.


    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE returns TRUE if this driver it
    // attemping to prevent TRESPASS requests.  Only meaningful when queried on the owner.
    virtual TRESPASS_DISABLE_REASONS GetTrespassDisableReasonsForVolumeStateReport() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE returns 0 if this driver does
    // not believe the volume is degraded.  Otherwise, bits indicating the reason for the
    // degredation should be set.
    virtual VOLUME_ATTRIBUTE_BITS GetVolumeAttributesForVolumeStateReport() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_VOLUME_STATE and IOCTL_DISK_GET_GEOMETRY
    // returns true if this driver believes that the volume is ready for I/O (or doesn't
    // know).
    virtual bool GetReadyForVolumeStateReport() const { return true; }
    
    // Modifier function for IOCTL_FLARE_CAPABILITIES_QUERY returns true if the volume
    // should report support for DCA READ, default false.
    virtual bool SupportsDcaRead() const { return false; }

    // Modifier function for IOCTL_FLARE_CAPABILITIES_QUERY returns true if the volume
    // should report support for DCA WRITE, default false.
    virtual bool SupportsDcaWrite() const { return false; }

    // Modifier function for IOCTL_FLARE_CAPABILITIES_QUERY returns true if the volume
    // should supports setting of special bits in the IO_STACK_LOCATION::Flags, default
    // false.
    virtual bool SupportsCOVflags() const { return false; }

    // Modifier function for IOCTL_FLARE_CAPABILITIES_QUERY returns true if the volume
    // supports zero-fill, setting the zero-fill capability bits in the
    // IO_STACK_LOCATION::Flags, default false.
    virtual bool SupportsZeroFill() const { return false; }

    // Modifier function for IOCTL_FLARE_CAPABILITIES_QUERY returns true if the volume
    // supports sparse allocation, setting the sparse allocation bits in the
    // IO_STACK_LOCATION::Flags, default false.
    virtual bool SupportsSparseAllocation() const { return false; }


    // Modifier function so several IOCTLs.  Returns true if the volume is not write
    // protected, default true.
    virtual bool IsVolumeWritable() const { return true; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the RAID type to report,
    // default FLARE_RAID_TYPE_INDIVID.
    virtual UCHAR RaidInfoType() const { return FLARE_RAID_TYPE_INDIVID; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the MajorRev to report,
    // default 0.
    virtual UCHAR RaidInfoMajorRev() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the MinorRev to report,
    // default 0.
    virtual UCHAR RaidInfoMinorRev() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the number of disks per
    // stripe, default 1.
    virtual ULONG RaidInfoDisksPerStripe() const { return 1; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the number of sectors per
    // stripe to report, default 1.
    virtual ULONG RaidInfoSectorsPerStripe() const { return 1; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the LunCharacteristic,
    // default 0.
    virtual UCHAR RaidInfoLunCharacteristics() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the number of bytes in
    // each element to report, default 1.
    virtual ULONG RaidInfoBytesPerElement() const { return 1; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the number of stripes to
    // report, default 0.
    virtual ULONG RaidInfoStripeCount() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the RAID group ID to
    // report, default 0.
    virtual ULONG RaidInfoGroupId() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the User defined name for
    // the volume to report.
    virtual VOID  RaidInfoGetUserDefinedName(K10_LOGICAL_ID buffer, ULONG maxLen);

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the Bind time to report,
    // default 0.
    virtual ULONG RaidInfoBindTime() const { return 0; }

    // Modifier function for IOCTL_FLARE_GET_RAID_INFO. Returns the FLU number to report,
    // default 0.
    virtual ULONG RaidInfoLun() const { return 0; }

    // Modifier function for GEOMETRY IOCTLS. Returns the media type to report, default
    // FixedMedia.
    virtual MEDIA_TYPE MediaType() const { return FixedMedia; }

    // Returns the number of sectors in the volume.
    virtual DiskSectorCount LogicalUnitSize()  { return mLunSize; }
    
    // How many bytes per sector are being reported, default 512.
    virtual ULONG BytesPerSector() const { return 512; }

public:
    ///////////  Subclass event notifications ////////////////////////
    // Event notifications are implemented as no-op virtual functions, so that BasicVolume
    // takes no actions, but the subclass need not implement them.

    // An SP other that the peer SP has lost connectivity.
    //
    // If the subclass overrides this function, it must release the volume lock before
    // returning.
    //
    // @param mux - the mux the contact lost is being reported on (it will be reported on
    //              all muxes)
    // @param spID - identifies the SP that can no longer be reached.
    virtual VOID ContactLostReleaseLock(VolumeMultiplexor * mux, CMI_SP_ID spId) {
        ReleaseSpinLock();
    };

    // A message arrived from an SP for this volume. Note that the subclass may
    // differentiate between partner SP and other SP messages by overriding
    // PeerMessageReceivedReleaseLock below.
    //
    // If the subclass overrides this function, it must release the volume lock before
    // returning.
    //
    // @param mux - the mux the message arrived on
    // @param message - the message that just arrived.
    //
    // Returns: 
    //   true - delayed Release; message is held waiting further service.
    //   false - Message processing is complete.
    virtual bool MessageReceivedReleaseLock(VolumeMultiplexor * mux, InboundMessage * message) { 
        ReleaseSpinLock();
        return false; 
    };

    // The peer SP has failed and is now going offline.  
    //
    // Note: We cannot be sure that all other drivers or volumes have been notified of
    // this event until a message that we have sent or will send in this callback gets a
    // send error.  We should not take action in this callback that affects other drivers
    // or volumes and assumes that those other drivers/volumes have been notified of the
    // SP failure.  Instead, this callback may send a message to the peer SP that is
    // guaranteed to fail, but only after all drivers and volumes have been notified of
    // this event. Therefore the send error event is a reliable trigger for certain
    // actions.
    //
    // @param mux - the mux the contact lost was reported on (there will be one call per
    //              mux)
    //
    // If the subclass overrides this function, it must release the volume lock before
    // returning.
    virtual VOID PeerContactLostReleaseLock(VolumeMultiplexor * mux) 
    {
        ReleaseSpinLock();
    }

    // A message arrived from another SP for this volume.
    // @param mux - the mux the message arrived on
    // @param message - the message that just arrived.
    //
    // If the subclass overrides this function, it must release the volume lock before
    // returning.
    //
    // Returns: 
    //   true - delayed Release; message is held waiting further service.
    //   false - Message processing is complete.
    virtual bool PeerMessageReceivedReleaseLock(VolumeMultiplexor * mux, InboundMessage * message) {
        return MessageReceivedReleaseLock(mux, message);
    }

    // A CMI message is arriving that has a fixed data portion, and CMID is asking for the
    // physical location into which to place the data from any SP.
    //
    // Note: the CMID protocol may re-request the same transfer multiple times, so it is
    // illegal to change any state within this call.
    //
    // If SPs are sending fixed data to this driver, this function must be overridden by
    // the subclass.  The default action is to PANIC if called.
    //
    // This function may be due to messages from any SP.  If an SP is not allowed to send
    // fixed data transfers, the implementation of this function must direct the data to a
    // bit-bucket.
    //
    // @param mux - the mux the message arrived on
    // @param pDMAAddrEventData - see CmiUpperInterface.h
    virtual VOID DMAAddrNeededReleaseLock(const VolumeMultiplexor * mux,
        PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA pDMAAddrEventData);

    // Convenience function for derived drivers to call when there are Filter parameters
    // to be validated
    bool ValidateFilterParamsForUpdateLockHeld(BasicVolumeParams * volumeParams, ULONG paramsLen);

    // An IOCTL_BVD_SET_VOLUME_PARAMS is being processed for this object.  Overriding
    // gives the specific driver the opportunity to decide if the proposed parameters are
    // acceptable.
    // WARNING: there is no notification if the transaction is aborted, so no state
    // change should be made in this function which depends on the transaction
    // committing.
    // @param volumeParams - the parameters which are being proposed. The base class has
    //                       been already validated.  This is a pointer to the base
    //                       structure which can be static_cast<> to the drivers actual
    //                       parameters.
    // @param paramsLen - the number of bytes in the structure .
    virtual bool EventValidateVolumeParamsForUpdateLockHeld(BasicVolumeParams * volumeParams, ULONG paramsLen) {      
        return true;
    }

    // An IOCTL_BVD_DESTROY_VOLUME is being processed for this object.  Overriding gives
    // the specific driver the opportunity to do any processing it needs prior to the
    // actual delete of the object. No volume specific locks are held when this is called
    // but the locking around the SyncTransaction that is managing the delete operation is
    // still in place and will hold off any other attempts to remove the volume (only a
    // configuration operation coming through as a SyncTransaction will delete the volume
    // object).
    // WARNING: there is no notification if the transaction is aborted, so no state change
    // should be made in this function which depends on the transaction committing.
    virtual bool EventPrepareVolumeForDelete() {
        return true;
    }

    // An IOCTL_BVD_SET_VOLUME_PARAMS has been accepted for this object, and the
    // underlying volume parameters have been changed and the lock has not been released
    // since the change. This function should be overridden if the driver has any action
    // to take on the change of parameters.
    // @param volumeParams - the parameters which have been updated.  This is a pointer to
    //                       the base structure which can be static_cast<> to the drivers
    //                       actual parameters.
    // @param paramsLen - the number of bytes in the structure accepted.

    virtual void EventVolumeParamsUpdatedReleaseLock(BasicVolumeParams * volumeParams, ULONG paramsLen) {
        ReleaseSpinLock();
        return;
    }

    //Derived class shall override this function and fills in volume specific status
    //information in addition
    //to what basicVolume provides.
    virtual EMCPAL_STATUS EventQueryVolumeStatusReleaseLock(BasicVolumeStatus * ReturnedVolumeStatus,
                                                   ULONG MaxVolumeParamsLen, ULONG & OutputLen) {
        ReleaseSpinLock();
        return EMCPAL_STATUS_SUCCESS;
    }
    
    //Derived class shall override this function and fills in volume specific statistics
    //information in addition
    //to what basicVolume provides.
    virtual EMCPAL_STATUS EventGetVolumeStatisticsReleaseLock(BasicVolumeStatistics * ReturnedVolumeStatistics,
                                                         ULONG MaxVolumeParamsLen, ULONG & OutputLen) {
        ReleaseSpinLock();
        return EMCPAL_STATUS_SUCCESS;
    }

    //Derived class shall override this function and clear the volume specific statistics
    //information in addition to what basicVolume provides.
    virtual VOID EventClearVolumeStatisticsReleaseLock() {
        ClearBasicVolumeStatistics();
        ReleaseSpinLock();
    }
                                                       
    virtual bool EventVolumeActionValidateReleaseLock(const BasicVolumeAction * bvdAction, ULONG newParamLen)
    {
          ReleaseSpinLock();
          //Derived class shall override this function and perform volume specific
          //operation.
          return false;
    }

    //Derived class shall override this function and perform volume specific action
    
    // Routine to handle PERFORM_VOLUME_ACTION IOCTL Takes the action specified on the
    // driver. Calls EventPerformVolumeActionReleaseLock() on the specific volume. which
    // the sub-class can override.  
    //
    // WARNING: This call occurs prior to the persistence of any parameters, allowing this function
    // to indicate that persistence is required.  Care should be taken to ensure that externally 
    // visible actions occur in EventVolumeParamsUpdatedReleaseLock rather than here, because if there is a crash,
    // the transaction may abort, and the BasicVolumeParams will revert to the state prior to the transaction.
    //
    // @param VolumeAction - a sub-class of BasicVolumeAction, defined for the specific
    //                       driver.  Contains the WWN of the volume to perform the action
    //                       on.
    // @param Len - The number of bytes passed in for the subclass of VolumeAction
    // @params newVolumeParams - A buffer initialized to the current state of BasicVolume params.
    //                           The buffer may be updated, and the updated values will be used 
    //                           in the subsequent call to UpdateVolumeParamsReleaseLock().
    virtual VOID EventPerformVolumeActionReleaseLock(BasicVolumeAction * VolumeAction, ULONG Len,
        BasicVolumeParams * newVolumeParams) {

        //Derived class shall override this function and perform volume specific operation.
        ReleaseSpinLock();
        return;
    }
    virtual ~BasicVolume();

public:

    // Returns true if the device object is currently opened.
    bool IsOpen() const { return mOpenCount > 0; }

	// Returns mOpenCount
    ULONG GetOpenCount() const { return mOpenCount;}
	
    // Acquire the spin lock associated with the volume.
    void AcquireSpinLock() { mLock.Acquire();}

    // Acquires the volume spin lock then releases another lock.
    // @param lockToRelease - the spin lock to release after getting the volume spin lock.
    //
    // Note: careful ordering is required to avoid deadlocks.
    void AcquireReleaseSpinLock(K10SpinLock & lockToRelease) { 
        mLock.AcquireRelease(lockToRelease);
    }

    // Release the spin lock associated with the volume.
    void ReleaseSpinLock() { mLock.Release();}

    // Log debugging information about an arriving IRP to ktrace
    // @param level - The trace level that identifies the information
    // @param format - the ktrace format string, must have "%p %s %s" format
    //                 specifications, for IRP pointer, Major/Minor constant strings.
    // @param Irp - the IRP itself.
    VOID TraceIrp(BasicTraceLevel level, const char * format, PBasicIoRequest  Irp) const;

    // Returns driver's pointer.    
    BasicVolumeDriver* GetDriver();
    
    // For user internally within BVD at volume creation time.
    // @param howCreated - How was the volume created
    // @param UniqueVolumeIndex - the unique value for this volume.
    void SetHowCreated( BasicVolumeCreationMethod howCreated, VolumeIndex UniqueVolumeIndex)  {  
        mHowCreated = howCreated;

        mVolumeIndex = UniqueVolumeIndex;

        // This can be made persistent later also.
        mVolumeParametersArePersistent = (BvdVolumeCreate_PeristenceIoctl == howCreated);
    }

    void SetVolumeCreationCount(ULONGLONG CreationCount) {
        mVolumeCreateCount = CreationCount;
    }

    // Called during volume creation to allow for auto-insertion of devices above.  This
    // creates volumes   
    virtual EMCPAL_STATUS AutoInsertAboveAndExposeDevice();

    // Some drivers, notably the redirectors, require that a volume's device object
    // be created before calling InitializeVolumeReleaseLock() rather than during the
    // AutoInsertAboveAndExposeDevice processing.  Those drivers must override this 
    // function and return true so that the BVD CreateVolume method will do just that.
    virtual bool RequireDeviceObjectCreationBeforeInitialization() {return false;}
  
    // Internal logging function, could be overridden to change the trace prefix, etc.
    //
    // @param ring - the ktrace ring buffer to log to
    // @param format - the printf format string
    // @param args - the format arguments.
    virtual VOID TraceRing(KTRACE_ring_id_T ring, const char * format, va_list args)  const __attribute__ ((format(__printf_func__, 3, 0)));

    virtual KTRACE_ring_id_T  DetermineTraceBuffer(BasicTraceLevel level) const;
 

protected:
    // Clears basic volume statistics.
    VOID ClearBasicVolumeStatistics();

    /// returns size of VolumeStatus structure used by this volume
    /// Derived class can override this function to return another size of they are using different structure
    //
    virtual ULONG GetVolumeStatusSize() const
    {
         return sizeof(BasicVolumeStatus);
    }

    // Logical number of sectors in the volume.
    DiskSectorCount     mLunSize;

    // The memory for the parameters is allocated when the volume is allocated, and the
    // pointer is passed in via the constructor.
    BasicVolumeParams * const mVolumeParams;         

    // What Driver is this volume part of?
    BasicVolumeDriver * mDriver;

    // Protects the volume.
    K10SpinLock         mLock;


private:
    // For serializing open/close
    BasicBinarySemaphore     mOpenCloseSemaphore;

    enum {    
        // How long to wait on the Open/Close semaphore before bug-checking? We want to
        // bug check on the wait event first, since it is probably closer to the root of
        // the problem.
        BasicVolumeOpenCloseSemaphoreBugCheckSeconds = 4*60
    };
    // The number of outstanding opens. Protected by mOpenCloseSemaphore.
    ULONG               mOpenCount;

    // We are committed to deleting this volume. Protected by mOpenCloseSemaphore.
    bool                mDeletePending;


    // The timestamp of last modification
    ULONG               mTimeStamp;

    // Unique key for the volume.
    VolumeIndex         mVolumeIndex;

    // How was this volume created on this SP?
    BasicVolumeCreationMethod  mHowCreated;

    // Will this volume be re-created on reboot by BVD persistence?
    bool                       mVolumeParametersArePersistent;


    // Copy the Driver change count to each volume
    ULONGLONG                 mVolumeChangeCount;	

    // To indicate when the volume was created. Is 0 for a volume created at boot time
    // Is equal to driver change count at the time of creation for a volume created at runtime
    ULONGLONG mVolumeCreateCount; 

    // If we're auto inserting we may get a different "Key" from the derived
    // driver.  We will need that when we go to delete the volume but by the
    // time the base destructor runs the derived volume class is long gone.
    // As a result we can't query the derived volume for the Key again.  We'll
    // save a copy of it here to use at delete time
    VolumeIdentifier        mAutoInsertKey;

    // Handle an arriving IRP_MJ_CREATE.  The IRP must be completed synchronously.
    // @param Irp - an IRP_MJ_CREATE that has just arrived in the driver.
    // Returns: The status returned in the IRP
    EMCPAL_STATUS DispatchIRP_MJ_CREATE(PBasicIoRequest  Irp);

    // Handle an arriving IRP_MJ_CLOSE (override for BasicDeviceObject function)
    // @param Irp - The IRP being dispatched.
    EMCPAL_STATUS DispatchIRP_MJ_CLOSE( PBasicIoRequest  Irp);

    // Handle an arriving IRP_MJ_CLEANUP (override for BasicDeviceObject function)
    // @param Irp - The IRP being dispatched.
    EMCPAL_STATUS DispatchIRP_MJ_CLEANUP( PBasicIoRequest  Irp);

    // Handle an arriving IRP_MJ_DEVICE_CONTROL (override for BasicDeviceObject function)
    // @param Irp - The IRP being dispatched.
    EMCPAL_STATUS DispatchIRP_MJ_DEVICE_CONTROL( PBasicIoRequest  Irp);

    // Handle an arriving IRP_MJ_INTERNAL_DEVICE_CONTROL (override for BasicDeviceObject
    // function)
    // @param Irp - The IRP being dispatched.
    EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL( PBasicIoRequest  Irp);

    // Handle an arriving IRP_MJ_READ (override for BasicDeviceObject function)
    // @param Irp - The IRP being dispatched.
    EMCPAL_STATUS DispatchIRP_MJ_READ(PBasicIoRequest  Irp);

    // Handle an arriving IRP_MJ_WRITE (override for BasicDeviceObject function)
    // @param Irp - The IRP being dispatched.
    EMCPAL_STATUS DispatchIRP_MJ_WRITE(PBasicIoRequest  Irp);

    // Called when auto-inserting a driver above to 
    // determine what key to use on the auto-insert creation, that is, what to use for
    // BasicVolumeParams::LunWwn.  Defaults to using the same key as this volume.
    // Returns: the LunWwn to use for all auto-insert devices created above this device.
    virtual VolumeIdentifier GetAutoInsertKey() { return mVolumeParams->LunWwn; }

public:

    // What is the unique key for this volume?
    const  VolumeIdentifier &   Key() const { return mVolumeParams->LunWwn; }

    VOID ChangeLunWwnDriverLockHeld(VolumeIdentifier wwn) {
        mVolumeParams->LunWwn = wwn;
    }

    // Get readable access to all volume parameters.
    const BasicVolumeParams * GetVolumeParams() const { return mVolumeParams; }

    // How was this volume created on this SP?
    BasicVolumeCreationMethod HowCreated() const { return mHowCreated; }

    // Will this volume be re-created on reboot by BVD persistence?
    // NOTE: transitions to true at transaction commit time.
    bool VolumeParametersArePersistent() const { return mVolumeParametersArePersistent; }

    // Get the Driver Change Count from the driver
    inline ULONGLONG  GetVolumeChangedCount() const {return mVolumeChangeCount ; }

    // Get the Volume Creation Count 
    ULONGLONG GetVolumeCreationCount() const { return mVolumeCreateCount; }

    // This must be called whenever the state may have  changed if that IOCTLs
    // would report different results than previously, causing notification of this
    // change elsewhere.
    VOID  VolumeParametersStatusOrExistenceChangedNoLocksHeld();
    VOID  VolumeParametersStatusOrExistanceChangedReleaseDriverLock();

    // A BasicVolume is part of a hash table of volumes, and the hash table class uses
    // mHashNext in its implementation.
    BasicVolume *       mHashNext;

#if defined(SIMMODE_ENV)
    void * GetVolumeSharedMemory() { return mVolumeSharedMemory; };
    void   SetVolumeSharedMemory(void* sharedMemory) { mVolumeSharedMemory = sharedMemory; };

private:
    void* mVolumeSharedMemory;       // Volume Shared Memory;
#endif
};

// From an IRP, determine which volume the IRP is current referencing
inline BasicVolume * IrpToBasicVolume(PBasicIoRequest  Irp)
{
    return (BasicVolume *)Irp->GetCurrentDeviceObject();
}


// A HashListOfBasicVolume is a collection of BasicVolumes where the key is the
// VolumeIdentifier.  The cost per hash table entry is one pointer. We want short hash
// chains, so this value should be similar to the volume count.
const int BasicVolumeHashTableSize = 2048;
typedef K10HashTable<BasicVolume, VolumeIdentifier, BasicVolumeHashTableSize, &BasicVolume::mHashNext> HashListOfBasicVolume;

SLListDefineListType(BasicVolume, FreeListOfBasicVolume);
SLListDefineInlineCppListTypeFunctions(BasicVolume, FreeListOfBasicVolume, mHashNext);

#endif  // __BasicVolume__

