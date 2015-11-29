;//++
;// Copyright (C) Data General Corporation, 2000,2003
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_NVRAM_DRIVER_MESSAGES_H
;#define K10_NVRAM_DRIVER_MESSAGES_H
;
;//
;//++
;// File:            K10NvRamDrvMessages.h (MC)
;//
;// Description:     This file defines NVRAM Driver Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  NVRAMDRV_xxx and NVRAMDRV_ADMIN_xxx.
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

FacilityNames   = ( NvRamDrv= 0x13d : FACILITY_NVRAMDRV )

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
Facility	= NvRamDrv
SymbolicName	= NVRAMDRV_INFO_GENERIC
Language	= English
Generic NVRAMDRV Information.
.

;
;#define NVRAMDRV_ADMIN_INFO_GENERIC                                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMDRV_INFO_GENERIC)
;


;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= NvRamDrv
SymbolicName	= NVRAMDRV_WARNING_GENERIC
Language	= English
Generic NVRAMDRV Warning.
.

;
;#define NVRAMDRV_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMDRV_WARNING_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamDrv
SymbolicName	= NVRAMDRV_WARNING_READ_BYTES_TOO_SMALL
Language	= English
The buffer supplied in an IOCTL_K10_NVRAM_READ_BYTES was too small.
.

;
;#define NVRAMDRV_ADMIN_WARNING_READ_BYTES_TOO_SMALL                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMDRV_WARNING_READ_BYTES_TOO_SMALL)
;


;//-----------------------------------------------------------------------------
MessageId	= +1
Severity	= Warning
Facility	= NvRamDrv
SymbolicName	= NVRAMDRV_WARNING_WRITE_BYTES_TOO_SMALL
Language	= English
The buffer supplied in an IOCTL_K10_NVRAM_WRITE_BYTES was too small.
.

;
;#define NVRAMDRV_ADMIN_WARNING_WRITE_BYTES_TOO_SMALL                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMDRV_WARNING_WRITE_BYTES_TOO_SMALL)
;

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= NvRamDrv
SymbolicName	= NVRAMDRV_ERROR_GENERIC
Language	= English
Generic NVRAMDRV Error Code.
.

;
;#define NVRAMDRV_ADMIN_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMDRV_ERROR_GENERIC)
;

;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= NvRamDrv
SymbolicName	= NVRAMDRV_CRITICAL_ERROR_GENERIC
Language	= English
Generic NVRAMDRV Critical Error Code.
.

;
;#define NVRAMDRV_ADMIN_CRITICAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(NVRAMDRV_CRITICAL_ERROR_GENERIC)
;

;#endif
