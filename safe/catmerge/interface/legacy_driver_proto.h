/*******************************************************************************
 * Copyright (C) EMC Corporation, 2006
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * File Name:
 *              legacy_driver_proto.h
 *
 * Description: 
 *              This header file contains functions prototypes for legacy functions
 *              which have, not only been deprecated, but have had their function
 *              prototypes removed when compiling under 64-bit. 
 *
 * History:
 *              05-19-06 - Phil leef - created
 ******************************************************************************/

#ifndef _LEGACY_DRIVER_PROTO_H_
#define _LEGACY_DRIVER_PROTO_H_

#include "EmcPAL_String.h"

#if defined(__cplusplus)
extern "C"
{
#endif 

/****************************************
 *** Legacy Function Prototypes. ***
 ****************************************/

#if defined(NO_LEGACY_DRIVERS) && defined(ALAMOSA_WINDOWS_ENV)

/* If NO_LEGACY_DRIVERS is defined, then we need to export those functions, 
 * which have had their prototypes removed in the ntddk.
 *
 * These prototypes were grabbed from the DDK
 * version: 3790.1218
 * path ..ddk\wnet\ntddk.h
 */

DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
NTHALAPI
NTSTATUS
HalAssignSlotResources (
    IN EMCPAL_PUNICODE_STRING RegistryPath,
    IN EMCPAL_PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
NTHALAPI
ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalSetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalGetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
HalMakeBeep(
    IN ULONG Frequency
    );

#endif /* ALAMOSA_WINDOWS_ENV - NTHACK */

#if defined(__cplusplus)
}
#endif 

#endif // _LEGACY_DRIVER_PROTO_H_
