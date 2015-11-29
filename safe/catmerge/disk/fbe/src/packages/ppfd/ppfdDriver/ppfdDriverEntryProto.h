#ifndef PPFD_DRIVERENTRY_PROTO_H
#define PPFD_DRIVERENTRY_PROTO_H

/***************************************************************************
  Copyright (C)  EMC Corporation 2008-2009
  All rights reserved.
  Licensed material - property of EMC Corporation. 
**************************************************************************/

/***************************************************************************
 ppfdDriverEntryProto.h
 ****************************************************************************

 File Description:
  Header file for internal function prototypes for ppfdDriverEntry.c

 Author:
  Eric Quintin
  
 Revision History:

***************************************************************************/
#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "ppfdDeviceExtension.h"        // for PPFD_DEVICE_EXTENSION 

BACKEND_TYPE ppfdDetectBackendType( void );
BOOL AttachToScsiPort( PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject, PEMCPAL_DEVICE_OBJECT ppfdPortDeviceObject );
BOOL IsScsiPortBE0( PEMCPAL_DEVICE_OBJECT ScsiPortDeviceObject, BOOL *pbIsBEZero );
EMCPAL_STATUS DispatchAny(IN PEMCPAL_DEVICE_OBJECT fido, IN PEMCPAL_IRP Irp);
EMCPAL_STATUS ppfdDrvInitializeDeviceExtension( IN PEMCPAL_DEVICE_OBJECT PDeviceObject );
EMCPAL_STATUS ppfdAddDevice(PEMCPAL_NATIVE_DRIVER_OBJECT driverObject, PEMCPAL_DEVICE_OBJECT physicalDeviceObject);
void ppfdInitGlobalData( PEMCPAL_NATIVE_DRIVER_OBJECT PDriverObject );

//
// Physical package interface functions 
//
BOOL ppfdPhysicalPackageInterfaceInit( void );
#endif //PPFD_DRIVERENTRY_PROTO_H
