/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation.


 File Name:
    ppfdDiskDeviceCreate.c

 Contents:
    The "PPFD Disk device Objects" are created by the functions in this file.  
    PPFD disk devices are created separately for ASIDC and NtMirror. The NtMirror
    disk devices get created via PnP mechanism when ppfdAddDevice() gets called.
    The PPFD disk devices used/claimed by ASIDC get created during driver entry by
    calling ppfdCreateDiskDeviceObjects().  

 Internal:

 Exported:
    BOOL ppfdIsPnPDiskDeviceObject( PDEVICE_OBJECT deviceObject )
    BOOL ppfdIsDiskDeviceObject( PDEVICE_OBJECT deviceObject )
    NTSTATUS ppfdAddDevice(PDRIVER_OBJECT driverObject, PDEVICE_OBJECT physicalDeviceObject )
    NTSTATUS ppfdCreateDiskDeviceObjects( PDRIVER_OBJECT  PDriverObject )


 Revision History:
   
***************************************************************************/
//
// Include Files
//
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "K10MiscUtil.h"
#include "ppfdDeviceExtension.h"
#include "ppfdMajorFunctions.h"
#include "EmcPrefast.h"
//#include <ntstrsafe.h>
#include "ppfdMisc.h"
#include "ppfdDriverEntryProto.h"
#include "ppfdKtraceProto.h"
#include "cpd_interface.h"
#include "ppfd_panic.h"

EMCPAL_STATUS ppfdGetScsiAddress (IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject, IN SCSI_ADDRESS *pScsiAddress);


//
//Imported functions
//
BOOL ppfdPhysicalPackageAddPPFDFidoToMap( PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject, UCHAR DiskNum, BOOL bPnPDisk );


/*******************************************************************************************

 Function:
    NTSTATUS	ppfdAddDevice(PDRIVER_OBJECT driverObject, PDEVICE_OBJECT physicalDeviceObject)

 Description:
    The logic in this AddDevice() function is very similar to the SfdAddDevice() function in the
    Sata Filter Driver for AX.  This will get called by Pnp manager at startup for each of the 4 
    "boot" drives.  Creates a "PPFD disk device object" (or FiDO - Filter Device Object) and attaches it to 
    the PDO passed to this AddDevice() function.  It will also set the device extension flag 
    PPFD_PNP_DISK_DEVICE_OBJECT so the dispatch  code can identify it as a disk device object.

 Arguments:
    driverObject - Pointer to driver object created by system.
    physicalDeviceObject - Pointer to device object used to send requests to the port driver.  We want 
                to create our FiDO (Filter Device Object) and attach to this Pdo.
 
 Return Value:
    NTSTATUS

 Revision History:

*******************************************************************************************/

EMCPAL_STATUS ppfdAddDevice(PEMCPAL_NATIVE_DRIVER_OBJECT driverObject, PEMCPAL_DEVICE_OBJECT physicalDeviceObject)
{
	EMCPAL_STATUS ntStatus;
	PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject;
	PPFD_DISK_DEVICE_EXTENSION	*DiskDeviceExtension;
    SCSI_ADDRESS ScsiAddress;
    ULONG       LoopId;

    // The pnpDiskNumber will increment each time AddDevice() is called.  We will just associate the physicaldeviceObject
    // passed to us with disk drives 0,1,2,3,4 
    

    PPFD_TRACE(PPFD_TRACE_FLAG_INFO, TRC_K_START, "PPFD: ppfdAddDevice \n");

    // Create disk device object
	ntStatus = EmcpalExtDeviceCreate(driverObject,
                              sizeof(PPFD_DISK_DEVICE_EXTENSION),
							  NULL,			// No device name
							  EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN,
							  0,            // No device
				                            // characteristics
					          FALSE,        // Non-exclusive access
						      &ppfdDiskDeviceObject);

	if (!EMCPAL_IS_SUCCESS(ntStatus))
	{
		PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdAddDevice [ERROR] Unable to create Disk Device Object!\n" );
        panic( PPFD_FAILED_ADD_DEVICE, ntStatus );
        return( ntStatus );
	}

  
	DiskDeviceExtension = (PPFD_DISK_DEVICE_EXTENSION *) EmcpalDeviceGetExtension(ppfdDiskDeviceObject);
    DiskDeviceExtension->CommonDevExt.Size = sizeof(PPFD_DISK_DEVICE_EXTENSION);

    //
    // Set flag in device extension indicating that this is a PNP disk device object.  This flag will be
    // checked later to identify if the IRP is targeted to a PNP disk device object
    //
    DiskDeviceExtension->CommonDevExt.DeviceFlags = PPFD_PNP_DISK_DEVICE_OBJECT;
	DiskDeviceExtension->physicalDeviceObject = physicalDeviceObject;

    ntStatus = ppfdGetScsiAddress(physicalDeviceObject,&ScsiAddress);
    if (!EMCPAL_IS_SUCCESS(ntStatus)){
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdAddDevice [ERROR] Unable to get Scsi address for device.!\n" );
        EmcpalExtDeviceDestroy(ppfdDiskDeviceObject);

        panic( PPFD_FAILED_ADD_DEVICE, ntStatus );

        return( EMCPAL_STATUS_UNSUCCESSFUL );
    }

    //Format of the address is 4,0 4,1 4,2 4,3 or 4,4
    LoopId = (ULONG)CPD_GET_LOGIN_CONTEXT_FROM_PATH_TARGET_ULONG(ScsiAddress.PathId, ScsiAddress.TargetId);

    DiskDeviceExtension->targetDiskDeviceObject = EmcpalExtDeviceAttachCreate(ppfdDiskDeviceObject, physicalDeviceObject);
	if (DiskDeviceExtension->targetDiskDeviceObject == NULL)
	{
        PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdAddDevice [ERROR] Unable to Attach Disk Device Object!\n" );
        EmcpalExtDeviceDestroy(ppfdDiskDeviceObject);
        
        panic( PPFD_FAILED_ADD_DEVICE, LoopId );

        return( EMCPAL_STATUS_UNSUCCESSFUL );
	}

    //
    // If we are booting from a SAS/SATA backend, then add this disk device object to our map
    //
    if( ppfdDetectBackendType() != FC_BACKEND )
    {
        //ASSERT(ScsiAddress.PathId == 4);
        if( ScsiAddress.TargetId < TOTALDRIVES )
        {
            ppfdPhysicalPackageAddPPFDFidoToMap( ppfdDiskDeviceObject, ScsiAddress.TargetId, TRUE );            
        }
    }

	EmcpalExtDeviceIoFlagsCopy(ppfdDiskDeviceObject, DiskDeviceExtension->targetDiskDeviceObject);
								/* line auto-joined with above */

	EmcpalExtDeviceEnable(ppfdDiskDeviceObject);
    
	// Return successfully.
	return (EMCPAL_STATUS_SUCCESS);
} // end ppfdAddDevice


/*******************************************************************************************

 Function:
    BOOL ppfdCreateDiskDeviceObjects( PDEVICE_OBJECT  PDriverObject )

 Description:
    Creates 5 "PPFD Disk Device Objects" and adds each disk object to our map (in ppfdPhysicalPackage file).
    These PPFD disk device objects are returned to ASIDC when it "claims" a disk (via SRB_FUNCTION_CLAIM_DEVICE). 
    ASIDC will then use these "PPFD disk device objects" for sending I/O to the disks. PPFD will translate the read/write
    into a call to the FBE API to the PhysicalPackage.

 Arguments:
    pDriverObject - The Driver Object Pointer

 Return Value:
    NTSTATUS  STATUS_SUCESS if all 5 disk devices cerated successfully
    
 Revision History:

*******************************************************************************************/

EMCPAL_STATUS ppfdCreateDiskDeviceObjects( PEMCPAL_NATIVE_DRIVER_OBJECT  PDriverObject )
{
   	EMCPAL_STATUS ntStatus;
    PEMCPAL_DEVICE_OBJECT ppfdDiskDeviceObject;
    PPFD_DISK_DEVICE_EXTENSION	*ppfdDiskDeviceExtension;
    UCHAR   DiskNum;

    //
    // Create 5 PPFD Disk Device Objects to represent the Boot drives
    // 
    for ( DiskNum=0; DiskNum < TOTALDRIVES; DiskNum++)
    {
        ntStatus = EmcpalExtDeviceCreate( PDriverObject,
                                          sizeof(PPFD_DISK_DEVICE_EXTENSION),
					  NULL,    // No device name
				          EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN,
				          0,       // No devicecharacteristics
					  FALSE,   // Non-exclusive access
				          &ppfdDiskDeviceObject);

        if ( EMCPAL_IS_SUCCESS(ntStatus))
        {           
            ppfdDiskDeviceExtension = (PPFD_DISK_DEVICE_EXTENSION *) EmcpalDeviceGetExtension(ppfdDiskDeviceObject);
            ppfdDiskDeviceExtension->CommonDevExt.Size = sizeof(PPFD_DISK_DEVICE_EXTENSION);
            //
            // Set flag in device extension indicating that this is a disk device object.  
            //
            ppfdDiskDeviceExtension->CommonDevExt.DeviceFlags = PPFD_DISK_DEVICE_OBJECT;

            // Add the ppfd Disk Device Object to our map (this maps Physical Package Disk OBJ ID's
            // to ppfd Disk Device Objects)
            ppfdPhysicalPackageAddPPFDFidoToMap( ppfdDiskDeviceObject, DiskNum, FALSE );
        }
        else
        {
            PPFD_TRACE(PPFD_TRACE_FLAG_ERR, TRC_K_START, "PPFD: ppfdCreateDiskDeviceObjects [ERROR] Unable to Create Disk Device Object for Disk=%d!\n", DiskNum );
            break;
        }
    }

    PPFD_TRACE(PPFD_TRACE_FLAG_OPT, TRC_K_START, "PPFD: ppfdCreateDiskDeviceObjects exit with status=0x%x\n", ntStatus );

    return( ntStatus );
}



/***************************************************************************

 Function:
    BOOL ppfdIsPortDeviceObject(PDEVICE_OBJECT  deviceObject  )

 Description:
    Returns a boolean indicating if the specified object is the "port device object" (PPFDPort0) 
    created during driver entry.  It checks the flag PPFD_PORT_DEVICE_OBJECT in the objects
    DeviceExtension->DeviceFlags to determine this.
    
  Arguments:
    deviceObject - Pointer to the device object to check
  
 Return Value:
    TRUE    => deviceObject is the PPFDPort0 "port device object"
    FALSE   => otherwise
  
 Notes:
   None

 Revision History:
***************************************************************************/

BOOLEAN
ppfdIsPortDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject )
{
    BOOLEAN     IsPortDevice = FALSE;
    PPPFD_COMMON_EXTENSION Extension = EmcpalDeviceGetExtension(deviceObject);

    if (Extension)
        IsPortDevice = ((Extension->DeviceFlags & PPFD_PORT_DEVICE_OBJECT)!= 0);
    
    return IsPortDevice;
}


/***************************************************************************
 Function:
    BOOL ppfdIsPnPDiskDeviceObject(PDEVICE_OBJECT  deviceObject  )

 Description:
    Returns a boolean indicating if the specified object is a "PNP Disk Device Object" which 
    is created in ppfdAddDevice().  It checks the flag PPFD_PNP_DISK_DEVICE_OBJECT in the objects
    DeviceExtension->DeviceFlags to determine this.
 
 Arguments:
    deviceObject

 Return Value:
    TRUE    => deviceObject is a PNP Disk device Object
    FALSE   => otherwise
  
 Notes:
   None

 Revision History:
***************************************************************************/

BOOLEAN
ppfdIsPnPDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject )
{
    BOOLEAN     IsPnPDiskDevice = FALSE;
    PPPFD_COMMON_EXTENSION Extension = EmcpalDeviceGetExtension(deviceObject);

    if (Extension)
        IsPnPDiskDevice = ((Extension->DeviceFlags & PPFD_PNP_DISK_DEVICE_OBJECT)!= 0);
    
    return IsPnPDiskDevice;
}


/***************************************************************************
 Function:
    BOOL ppfdIsDiskDeviceObject( PDEVICE_OBJECT deviceObject )

 Description:
    Returns a boolean indicating if the specified object is a "non PNP" disk device object
    The PPFD disk device objects are created during driver entry for use by ASIDC when
    it "claims" a disk device.  It checks the flag PPFD_DISK_DEVICE_OBJECT in the objects
    DeviceExtension->DeviceFlags to determine this.

 Arguments:
    deviceObject

 Return Value:
    TRUE    => It is a disk device object
    FALSE   => otherwise
  
 Notes:
   None

 Revision History:
***************************************************************************/
BOOLEAN
ppfdIsDiskDeviceObject( PEMCPAL_DEVICE_OBJECT deviceObject )
{
    BOOLEAN IsDiskDevice = FALSE;
    PPPFD_COMMON_EXTENSION Extension = EmcpalDeviceGetExtension(deviceObject);

    if (Extension)
        IsDiskDevice = ((Extension->DeviceFlags & PPFD_DISK_DEVICE_OBJECT)!= 0);
    
    return IsDiskDevice;
}


EMCPAL_STATUS 
ppfdGetScsiAddress (IN PEMCPAL_DEVICE_OBJECT pPortDeviceObject,
                    IN SCSI_ADDRESS *pScsiAddress)
{
    PEMCPAL_IRP                  pIrp;
    EMCPAL_IRP_STATUS_BLOCK      IoStatus;
    EMCPAL_RENDEZVOUS_EVENT  Event;
    EMCPAL_STATUS        Status;


    /* Initialize a notification event object to the non-signaled state. It 
     * will be used to signal the request completion.
     */
    EmcpalRendezvousEventCreate(
           EmcpalDriverGetCurrentClientObject(),
           &Event,
           "ppfdGetScsiAddress",
           FALSE);

    /* Allocate and initialize an IRP for an IOCTL request to be sent 
     * to the SCSI port device. The SCSI port drver will free this IRP.
     */
    pIrp = EmcpalExtIrpBuildIoctl (IOCTL_SCSI_GET_ADDRESS,
                                         pPortDeviceObject,
                                         NULL,
                                         0,
                                         pScsiAddress,
                                         sizeof(SCSI_ADDRESS),
                                         FALSE,
                                         &Event,
                                         &IoStatus);
    if (pIrp == NULL)
    {
        /* Couldn't get an IRP.
         */
        EmcpalRendezvousEventDestroy(&Event);
        return (EMCPAL_STATUS_INSUFFICIENT_RESOURCES);
    }

    /* Pass request to port driver and wait for request to complete.
     */
    Status = EmcpalExtIrpSendAsync (pPortDeviceObject, pIrp);

    if (Status == EMCPAL_STATUS_PENDING)
    {
        EmcpalRendezvousEventWait(&Event, EMCPAL_TIMEOUT_INFINITE_WAIT);
        EmcpalRendezvousEventDestroy(&Event);
        return (EmcpalExtIrpStatusBlockStatusFieldGet(&IoStatus));
    }
    
    /* We're back from this synchronous request. The address of this
     * SCSI port device will be returned in the supplied buffer.
     */

    EmcpalRendezvousEventDestroy(&Event);
    return (Status);

}   /* end of ppfdGetScsiAddress() */
