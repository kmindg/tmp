;//++
;// Copyright (C) Data General Corporation, 2000,2003
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_NVRAM_LIB_MESSAGES_H
;#define K10_NVRAM_LIB_MESSAGES_H
;
;//
;//++
;// File:            K10NvRamLibMessages.h (MC)
;//
;// Description:     This file defines NVRAM Library Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  NVRAMLIB_xxx and NVRAMLIB_ADMIN_xxx.
;//
;// History:         09-Nov-00       MWagner     Created
;//                  02-Apr-03       CJHughes    Corrected duplicate
;//                                              FacilityNames
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( NvRamLib= 0x13e : FACILITY_NVRAMLIB )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )

;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x0001
Severity	= Informational
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_INFO_GENERIC
Language	= English
Generic NVRAMLIB Information.
.

;
;#define NVRAMLIB_ADMIN_INFO_GENERIC                                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_INFO_GENERIC)
;


;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_GENERIC
Language	= English
Generic NVRAMLIB Warning.
.

;
;#define NVRAMLIB_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_DEVICE_NOT_FOUND
Language	= English
The NVRAM device was not found on the scanned PCI buses.
.

;
;#define NVRAMLIB_ADMIN_WARNING_DEVICE_NOT_FOUND                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_DEVICE_NOT_FOUND)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_VENDOR_ID_NOT_FOUND_IN_REGISTRY
Language	= English
The NVRAM device Vendor Indentifier could not be found in the Registry.
.

;
;#define NVRAMLIB_ADMIN_WARNING_VENDOR_ID_NOT_FOUND_IN_REGISTRY                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_VENDOR_ID_NOT_FOUND_IN_REGISTRY)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_DEVICE_ID_NOT_FOUND_IN_REGISTRY
Language	= English
The NVRAM device Device Indentifier could not be found in the Registry.
.

;
;#define NVRAMLIB_ADMIN_WARNING_DEVICE_ID_NOT_FOUND_IN_REGISTRY                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_DEVICE_ID_NOT_FOUND_IN_REGISTRY)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_BASE_REGISTER_INDEX_NOT_FOUND_IN_REGISTRY
Language	= English
The NVRAM device Base Register Index could not be found in the Registry.
.

;
;#define NVRAMLIB_ADMIN_WARNING_BASE_REGISTER_INDEX_NOT_FOUND_IN_REGISTRY                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_BASE_REGISTER_INDEX_NOT_FOUND_IN_REGISTRY)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_COULD_NOT_MAP_PHYSICAL_ADDRESS
Language	= English
The device could not be mapped.
.

;
;#define NVRAMLIB_ADMIN_WARNING_COULD_NOT_MAP_PHYSICAL_ADDRESS                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_COULD_NOT_MAP_PHYSICAL_ADDRESS)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_WARNING_READWRITE_UNKNOWN_OP
Language	= English
The Library was asked to perform an unknown operation.
.

;
;#define NVRAMLIB_ADMIN_WARNING_READWRITE_UNKNOWN_OP                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_WARNING_READWRITE_UNKNOWN_OP)
;


;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_ERROR_GENERIC
Language	= English
Generic NVRAMLIB Error Code.
.

;
;#define NVRAMLIB_ADMIN_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_ERROR_GENERIC)
;

;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= NvRamLib
SymbolicName	= NVRAMLIB_CRITICAL_ERROR_GENERIC
Language	= English
Generic NVRAMLIB Critical Error Code.
.

;
;#define NVRAMLIB_ADMIN_CRITICAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMLIB_CRITICAL_ERROR_GENERIC)
;

;#endif
