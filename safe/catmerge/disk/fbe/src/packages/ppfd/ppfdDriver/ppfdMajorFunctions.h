/***************************************************************************
 Copyright (C)  EMC Corporation 2008-2009
 All rights reserved.
 Licensed material - property of EMC Corporation. * All rights reserved.


 File Name:
    ppfdMajorFunctions.h

 Contents:
    Function prototypes for ppfdMajorFunctions.c    

 ***************************************************************************/

#ifndef _PPFD_DRV_MAJOR_FUNCTIONS_H_
#define _PPFD_DRV_MAJOR_FUNCTIONS_H_

//
// Include Files
//

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"



extern EMCPAL_STATUS
ppfdDrvOpen(
           IN PEMCPAL_DEVICE_OBJECT pDeviceObject,
           IN PEMCPAL_IRP PIrp
           );

extern EMCPAL_STATUS
ppfdDrvClose(
            IN PEMCPAL_DEVICE_OBJECT pDeviceObject,
            IN PEMCPAL_IRP PIrp );


extern EMCPAL_STATUS
ppfdDrvUnload(
             IN PEMCPAL_DRIVER pPalDriverObject
             );
#endif // _PPFD_DRV_MAJOR_FUNCTIONS_H_

// End ppfdMajorFunctions.h
