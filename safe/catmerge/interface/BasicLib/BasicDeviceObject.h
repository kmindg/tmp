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

#ifndef __BasicDeviceObject__
#define __BasicDeviceObject__
//++
// File Name:
//      BasicDeviceObject.h
//
// Contents:
//  Defines the BasicDeviceObject class.
//
//
// Revision History:
//--

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "BasicLib/BasicIoRequest.h"
# include "BasicLib/ConfigDatabase.h"
# include "CmiUpperInterface.h"
# include "BasicLib/BasicDriver.h"
# include "BasicLib/BasicThreadWaitEvent.h"
# include "k10defs.h"
# include "K10SpinLock.h"
    
typedef VOID (*DRIVER_UNLOAD_FUNCTION)();


//  A BasicDeviceObject is an abstract base class that contains Dispatch functions that
//  match Windows Driver Model dispatch functions, except that these dispatch functions
//  are on the BasicDeviceObject rather than in the driver.
class BasicDeviceObject {
public:
    BasicDeviceObject();

    enum DeviceType {
        TypeDefault = 1,
        TypeDisk = 2
    };

    // Create the device object which will not be accessible to other layers. Assumes the 
    // device name(s) have been set. To make the device object accessible to other layers
    // we have to call ExposeDevice().
    virtual EMCPAL_STATUS CreateDevice(DeviceType type = TypeDefault, bool bIsDirectIo = true);

    // Make the device object accessible to other layers. We will create device object if
    // it is not yet created.
    virtual EMCPAL_STATUS ExposeDevice(DeviceType type = TypeDefault, bool bIsDirectIo = true);

    // Handle an arriving IRP_MJ_CREATE.  The IRP must be completed synchronously.
    // @param Irp - an IRP_MJ_CREATE that has just arrived in the driver.
    // Returns: The status returned in the IRP
    virtual EMCPAL_STATUS DispatchIRP_MJ_CREATE(PBasicIoRequest  Irp) = 0;

    // Handle an arriving IRP_MJ_CLOSE
    // @param Irp - an IRP_MJ_CLOSE that has just arrived in the driver.
    virtual EMCPAL_STATUS DispatchIRP_MJ_CLOSE( PBasicIoRequest  Irp) = 0;

    // Handle an arriving IRP_MJ_CLEANUP
    // @param Irp - an IRP_MJ_CLEANUP that has just arrived in the driver.
    virtual EMCPAL_STATUS DispatchIRP_MJ_CLEANUP( PBasicIoRequest  Irp) = 0;

    // Handle an arriving IRP_MJ_DEVICE_CONTROL
    // @param Irp - an IRP_MJ_DEVICE_CONTROL that has just arrived in the driver.
    virtual EMCPAL_STATUS DispatchIRP_MJ_DEVICE_CONTROL( PBasicIoRequest  Irp) = 0;

    // Handle an arriving IRP_MJ_INTERNAL_DEVICE_CONTROL
    // @param Irp - an IRP_MJ_INTERNAL_DEVICE_CONTROL that has just arrived in the driver.
    virtual EMCPAL_STATUS DispatchIRP_MJ_INTERNAL_DEVICE_CONTROL( PBasicIoRequest  Irp) = 0;

    // Handle an arriving IRP_MJ_READ
    // @param Irp - an IRP_MJ_READ that has just arrived in the driver.
    virtual EMCPAL_STATUS DispatchIRP_MJ_READ(PBasicIoRequest  Irp) = 0;

    // Handle an arriving IRP_MJ_WRITE
    // @param Irp - an IRP_MJ_WRITE that has just arrived in the driver.
    virtual EMCPAL_STATUS DispatchIRP_MJ_WRITE(PBasicIoRequest  Irp) = 0;

    // A BasicDeviceObject has the capability to queue arriving to a thread, so the IRP is
    // serviced in the context of that thread.  See BasicDeviceObjectIrpDispatcherThread.
    // If the device queues the IRP to a BasicDeviceObjectIrpDispatcherThread in the
    // DispatchMethods above, the thread will call this function to do the real IRP
    // dispatch.
    //  Therefore subclasses that queue IRPs to threads must implement this method.
    //     
    // @param Irp - the IRP to dispatch
    virtual VOID DispatchQueuedIRP(PBasicIoRequest  Irp) { FF_ASSERT(false); }

    // Support delete via base class.
    virtual ~BasicDeviceObject();

     // Returns the WDM name in 8 bit characters
    const CHAR * AsciiName() const
    {
        return mDevNameTail;
    }

    bool DeviceNameAscii(char* Str, ULONG Size) const
    {
        if (Size < K10_DEVICE_ID_LEN)
        {
            strncpy(Str, mDevName, Size);
            return TRUE;
        }
        return FALSE;
    }

    
    // Informs the BasicVolume of its WDM device name, creates a WDM symbolic link name if
    // necessary.
    //
    // If overridden by the subclass (unusual), the subclass must call this.
    // 
    // @param DeviceName - pointer to a constant string containing the name of the object which cannot change during the life of
    //                     the object.
    // @param DeviceLinkName - pointer to a constant string containing an alternate name of the object which cannot change during the life of
    //                     the object.
    //
    // Return Value:
    //     STATUS_SUCCESS <other> error (resource allocation failure or name conflict)
    virtual EMCPAL_STATUS SetDeviceName(const CHAR * DeviceName, const CHAR * DeviceLinkName);

    // Returns the current names
    virtual VOID GetDeviceName(CHAR DeviceName[K10_DEVICE_ID_LEN], CHAR  DeviceLinkName[K10_DEVICE_ID_LEN]);
    
    // Returns the WDM device object associated with this volume.
    PEMCPAL_DEVICE_OBJECT      DeviceObject() const { return mDeviceObject; }

  

    // Called to adjust DEVICE_OBJECT::StackSize higher to allow IRPs to be forwarded to
    // consumed volumes.  If called multiple times on the BasicVolume, the maximum of all
    // values will be used.
    //
    // @param consumedVolumeStackSize - the IRP stack size of a consumed volume. 
    //                                  DEVICE_OBJECT::StackSize will be set to
    //                                  consumedVolumeStackSize+1
    //      
    VOID AdjustIrpStackSize(ULONG consumedVolumeStackSize);   
	
    static EMCPAL_STATUS DriverEntry(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, DRIVER_UNLOAD_FUNCTION unload,
        BasicDriver* gDriver, const PCHAR  RegistryPath);

	
    static EMCPAL_STATUS DriverEntry(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, DRIVER_UNLOAD_FUNCTION unload,
        BasicDriver* gDriver, EMCPAL_PUNICODE_STRING  RegistryPath);

    static VOID SetDispatchRoutines(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject);    

    static PEMCPAL_NATIVE_DRIVER_OBJECT GetDriverObject();

    ULONG GetAdjustIrpStackSize() { return mConsumedVolumeStackSize; }    

    // Not for use outside of BasicDeviceObject
    EMCPAL_STATUS InternalDispatchIRP_MJ_CREATELockHeld(PBasicIoRequest Irp);
    EMCPAL_STATUS InternalDispatchIRP_MJ_CLOSE(PBasicIoRequest Irp);

private:
    // The device name in ascii.
    CHAR                mDevName[K10_DEVICE_ID_LEN];

    // The symbolic link name, or "".
    CHAR                mDevLinkName[K10_DEVICE_ID_LEN];

    // Tail of device name
    CHAR   *             mDevNameTail;
    
    // The device object that this volume is associated with. Note that
    // mDeviceObject->DeviceExtension == this
    PEMCPAL_DEVICE_OBJECT      mDeviceObject;

    //Store the stack size
    ULONG mConsumedVolumeStackSize;

    // The number of IRP_MJ_CREATEs recieved which are outstanding,
    // or which returned STATUS_SUCCESS and have not yet been followed
    // with an IRP_MJ_CLOSE.
    ULONG                  mNumDeviceObjectOpens;

    // An event to wait on until mNumOpens is decremented.
    BasicThreadWaitEvent   mWaitForClose;

};

typedef BasicDeviceObject * PBasicDeviceObject;


#endif  // __BasicDeviceObject__

