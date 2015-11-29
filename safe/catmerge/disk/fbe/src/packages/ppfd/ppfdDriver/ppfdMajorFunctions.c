/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation. * All rights reserved.


 File Name:
      ppfdMajorFunctions.c

 Contents:

 Exported:
      ppfdDrvOpen()
      ppfdDrvClose()
      ppfdDrvUnload()

 Revision History:
      09/10/08    Eric Quintin Created (started with Nvram driver to create a shell driver)

***************************************************************************/

//
// Include Files
//

#include "ppfdMajorFunctions.h"
#include "ppfdDeviceExtension.h"
#include "ppfdMisc.h"
#include "ppfdKtraceProto.h"
#include "k10ntddk.h"
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"



/*******************************************************************************************

 Function:
    NTSTATUS ppfdDrvOpen( PDEVICE_OBJECT PDeviceObject, PIRP PIrp )
      
 Description:
    Just completes the IRP_MJ_CREATE with status = STATUS_SUCCESS.

 Arguments:
    PDEVICE_OBJECT DeviceObject
    PIRP Irp 

 Return Value:
      STATUS_SUCCESS:  

 Revision History:

*******************************************************************************************/

EMCPAL_STATUS
ppfdDrvOpen( PEMCPAL_DEVICE_OBJECT PDeviceObject, PEMCPAL_IRP pIrp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;

    if( NULL == PDeviceObject || pIrp == NULL )
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: ppfdDrvOpen Invalid Parameter!\n" );
        return( EMCPAL_STATUS_UNSUCCESSFUL );
    }

    //
    // Just complete the IRP
    //
    EmcpalExtIrpStatusFieldSet(pIrp, EMCPAL_STATUS_SUCCESS);
    EmcpalExtIrpInformationFieldSet(pIrp, 0);

    EmcpalIrpCompleteRequest(pIrp);
    return ( ntStatus );
}
// .End ppfdDrvOpen


/*******************************************************************************************
 Function:
    NTSTATUS ppfdDrvClose( IN PDEVICE_OBJECT PDeviceObject, IN PIRP PIrp )

 Description:
    Just completes the IRP_MJ_CLOSE with status = STATUS_SUCCESS.

 Arguments:
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp

 Return Value:
    STATUS_SUCCESS:

 Revision History:

 *******************************************************************************************/

EMCPAL_STATUS
ppfdDrvClose( PEMCPAL_DEVICE_OBJECT PDeviceObject, PEMCPAL_IRP pIrp )
{
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;

    if( pIrp == NULL )
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: ppfdDrvClose Invalid Parameter!\n" );
        return( EMCPAL_STATUS_UNSUCCESSFUL );
    }

    //
    // Just complete the IRP
    //

    EmcpalExtIrpStatusFieldSet(pIrp, ntStatus);
    EmcpalExtIrpInformationFieldSet(pIrp, 0);

    EmcpalIrpCompleteRequest(pIrp);
    return( ntStatus );
}
// .End ppfdDrvClose

/*******************************************************************************************

 Function:
    NTSTATUS ppfdDrvUnload( IN PEMCPAL_DRIVER pPalDriverObject )

 Description:

 Arguments:
      PalDriverObject

 Return Value:
      ntStatus

 Revision History:

*******************************************************************************************/
EMCPAL_STATUS
ppfdDrvUnload( IN PEMCPAL_DRIVER pPalDriverObject )
{
    PEMCPAL_DEVICE_OBJECT PDeviceObject = NULL;
    PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject;
    EMCPAL_STATUS ntStatus = EMCPAL_STATUS_SUCCESS;

    PDriverObject = (PEMCPAL_NATIVE_DRIVER_OBJECT)EmcpalDriverGetNativeDriverObject(pPalDriverObject);

    if( PDriverObject == NULL )
    {
        PPFD_TRACE( PPFD_TRACE_FLAG_INFO, TRC_K_STD, "PPFD: ppfdDrvUnload Invalid Driverobject!\n" );
        return( EMCPAL_STATUS_UNSUCCESSFUL );
    }

    // Technically this driver should never be unloaded, so we do not have to implement anything
    // here.  However, for test purposes it might be convenient to be able to start/stop the driver so
    // the device object will be deleted to allow stopping/restarting the driver.  This will not work
    // if the system was booted through PPFD..only can unload if booting in debug configuration where
    // we boot from an external SATA drive and start PPFD manually.
    
    PPFD_TRACE( PPFD_TRACE_FLAG_OPT, TRC_K_STD, "PPFD: ppfdDrvUnload\n" );

    //
    // Find our Device Object and Device Extension
    //
    PDeviceObject = EmcpalExtDeviceListFirstGet(PDriverObject);

    //
    // Delete our device object
    //
    EmcpalExtDeviceDestroy(PDeviceObject);
    return ntStatus;

}
// .End ppfdDrvUnload


// End ppfdMajorFunctions.c
