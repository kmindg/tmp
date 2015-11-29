/************************************************************************
*
*  Copyright (c) 2009 EMC Corporation.
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

#ifndef __BasicConsumedDevice__
#define __BasicConsumedDevice__
//++
// File Name:
//      BasicConsumedDevice.h
//
// Contents:
//  Defines the BasicConsumedDevice class.
//
//
// Revision History:
//--

# include "BasicLib/BasicIoRequest.h"
# include "k10defs.h"
# include "BasicLib/BasicBinarySemaphore.h"
#if !defined (I_AM_DEBUG_EXTENSION)
# include "BasicLib/BasicThreadWaitEvent.h"
#endif
# include "EmcPAL_Irp.h"



//  A BasicConsumedDevice is an class which hides both WDM and CSX "external device" functionality
class BasicConsumedDevice {
public:
    // Constructor.  
    BasicConsumedDevice();

    // Destructor.
    virtual ~BasicConsumedDevice();

    // Set the DeviceObject name.
    //
    // @param name - the device object name of the device that will be consumed.
    // Lenght of the device object name should be less then K10_DEVICE_ID_LEN.
    void SetDeviceObjectName(const CHAR * name); 

    //Not abolutely necessary for native nt kernel, but required for csx
    void SetClient(PEMCPAL_CLIENT client)
    {
        mClient = client;
    }

    // Returns the device object name set via SetDeviceObjectName
    const CHAR* GetDeviceObjectName() const {
        return mDeviceObjectName;
    }


    // Returns true if the device is currently opened.
    bool IsOpen() const {
        return mDeviceObject != NULL;
    }


    // Returns the number of I/O stack locations needed for IRPs sent to this device. The
    // device must be open to make this call.
    CCHAR GetStackSize() const;

    // Return the underlying device object pointer for this consumed volume. Returns NULL
    // if the ConsumedDevice is not open.
    PEMCPAL_DEVICE_OBJECT GetDeviceObject () const {
        return mDeviceObject;
    }

    PEMCPAL_FILE_OBJECT GetFileObject () const {
        return mFileObject;
    }

    // Open the consumed device.
    EMCPAL_STATUS OpenConsumedDevice();


    // Close the consumed device.
    virtual VOID CloseConsumedDevice();

    //returns desired method for READ/WRITE device calls
    // \retval EMCPAL_IRP_TRANSFER_BUFFERED 
    // \retval EMCPAL_IRP_TRANSFER_DIRECT 
    // \retval EMCPAL_IRP_TRANSFER_USER if not any above
    // \retval EMCPAL_IRP_TRANSFER_AGNOSTIC for CSX to CSX I/O
    EMCPAL_IRP_TRANSFER_METHOD GetRWMethod();


    //Send the request down. This will be asynchronous operation.
    // NOTE: This routine is deprecated and should not be used.
    EMCPAL_STATUS CallDriverDeprecated(PBasicIoRequest  Irp);

    //Send the request down. This will be asynchronous operation.
    VOID CallDriver(PBasicIoRequest  Irp);

    // Send the request down synchronously. It will not return to 
    // the caller until IRP completes.
    EMCPAL_STATUS SyncCallDriver(PBasicIoRequest  Irp);         

    // Send the request down synchronously. It will panic on timeout.
    EMCPAL_STATUS SyncCallDriverPanicOnTimeout(PBasicIoRequest  Irp, ULONG timeoutSecond);

private:
    // What device should we open/have we opened.
    CHAR mDeviceObjectName[K10_DEVICE_ID_LEN];

    // The device object pointer of the underlying device.  NULL if closed.
    PEMCPAL_DEVICE_OBJECT mDeviceObject;

    // The file object pointer of the underlying device.  NULL if closed.
    PEMCPAL_FILE_OBJECT   mFileObject;

    PEMCPAL_CLIENT mClient;
};

#endif  // __BasicConsumedDevice__
