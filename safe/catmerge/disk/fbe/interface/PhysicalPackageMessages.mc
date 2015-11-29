;//++
;// Copyright (C) EMC Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;#ifndef PHYSICAL_PACKAGE_MESSAGES_H
;#define PHYSICAL_PACKAGE_MESSAGES_H
;
;//
;//++
;// File:            PhysicalPackageMessages.h (MC)
;//
;// Description:     This file defines Physical Package Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  PHYSICAL_PACKAGE_xxx and PHYSICAL_PACKAGE_ADMIN_xxx.
;//
;// Notes:
;//   For simulation add the message string also in file fbe_event_log_esp_msgs.c
;//   under the appropriate function according to message severity.
;//
;//  Information Status Codes Range:  0x0000-0x3FFF
;//  Warning Status Codes Range:      0x4000-0x7FFF
;//  Error Status Codes Range:        0x8000-0xBFFF
;//  Critical Status Codes Range:     0xC000-0xFFFF
;//
;// History:         23-Feb-09       Ashok Tamilarasan     Created
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( PhysicalPackage= 0x11C : FACILITY_PHYSICAL_PACKAGE )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )


;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x0000
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_GENERIC
Language	= English
Generic Information.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0001
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_LOAD_VERSION
Language	= English
Driver compiled for %2.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_LOAD_VERSION                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_LOAD_VERSION)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0002
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_SAS_SENSE
Language	= English
Bus %2 Enclosure %3 Disk %4 CDB Check Condition. SN %5 %6.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_SAS_SENSE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_SAS_SENSE)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0003
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED
Language	= English
Drive firmware download started, TLA %2, new revision %3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_DRIVE_FW_DOWNLOAD_STARTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0004
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED
Language	= English
Drive firmware download completed, TLA %2, new revision %3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_DRIVE_FW_DOWNLOAD_COMPLETED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0005
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED
Language	= English
Drive firmware download aborted by user.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_DRIVE_FW_DOWNLOAD_ABORTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0006
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_PDO_SENSE_DATA
Language	= English
%2 %3 %4.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_PDO_SENSE_DATA                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERRORPHYSICAL_PACKAGE_INFO_PDO_SENSE_DATA)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0007
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_PDO_SET_EOL
Language	= English
%2 PDO has set EOL flag. SN:%3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_PDO_SET_EOL                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_PDO_SET_EOL)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0008
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_PDO_SET_LINK_FAULT
Language	= English
%2 PDO has set Link Fault flag. SN:%3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_PDO_SET_LINK_FAULT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_PDO_SET_LINK_FAULT)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0009
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_PDO_SET_DRIVE_FAULT
Language	= English
%2 PDO has set Drive Fault flag. SN:%3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_PDO_SET_DRIVE_FAULT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_PDO_SET_DRIVE_FAULT)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x000A
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_USER_INITIATED_DRIVE_POWER_CTRL_CMD
Language	= English
User initiated %2 on %3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_USER_INITIATED_DRIVE_POWER_CTRL_CMD                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_USER_INITIATED_DRIVE_POWER_CTRL_CMD)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x000B
Severity	= Informational
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_INFO_HEALTH_CHECK_STATUS
Language	= English
%2 Health Check Status: %3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_INFO_HEALTH_CHECK_STATUS                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_INFO_HEALTH_CHECK_STATUS)
;

;// ----------------------Add all Informational Messages above this ----------------------
;//
;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_WARN_GENERIC
Language	= English
Generic Information.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_WARN_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_WARN_GENERIC)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x4003
Severity	= Warning
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_WARN_SOFT_SCSI_BUS_ERROR
Language	= English
%2 Soft SCSI bus error. DrvErrExtStat:0x%3 %4 %5.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_WARN_SOFT_SCSI_BUS_ERROR                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_WARN_SOFT_SCSI_BUS_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x4004
Severity	= Warning
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_WARN_SOFT_MEDIA_ERROR
Language	= English
%2 Soft media error. DrvErrExtStat:0x%3 %4 %5.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_WARN_SOFT_MEDIA_ERROR                          \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_WARN_SOFT_MEDIA_ERROR)
;


;//--------------------- Add all Warning messages above this -------------------
;//
;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_GENERIC
Language	= English
Generic Information.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_GENERIC)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x8001
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_RECOMMEND_DISK_REPLACEMENT
Language	= English
%2 Recommend disk replacement. DrvErrExtStat:0x%3 %4 %5.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_RECOMMEND_DISK_REPLACEMENT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_RECOMMEND_DISK_REPLACEMENT)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x8003
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_HARD_SCSI_BUS_ERROR
Language	= English
TBD.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_HARD_SCSI_BUS_ERROR                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_HARD_SCSI_BUS_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x8004
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_HARD_MEDIA_ERROR
Language	= English
TBD.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_HARD_MEDIA_ERROR                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_HARD_MEDIA_ERROR)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x8005
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_DRIVE_FW_DOWNLOAD_FAILED
Language	= English
Drive firmware download failed, TLA %2, new revision %3, reason %4.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_DRIVE_FW_DOWNLOAD_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_DRIVE_FW_DOWNLOAD_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x8006
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_PDO_SET_EOL
Language	= English
%2 PDO has set EOL flag. SN:%3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_PDO_SET_EOL                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_PDO_SET_EOL)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x8007
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_ERROR_PDO_SET_DRIVE_KILL
Language	= English
%2 PDO has set Drive Kill flag. SN:%3.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_ERROR_PDO_SET_DRIVE_KILL                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_ERROR_PDO_SET_DRIVE_KILL)
;

;//-------------------- Add all Error Message above this -----------------------
;//-----------------------------------------------------------------------------
;//  Critical Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= PhysicalPackage
SymbolicName	= PHYSICAL_PACKAGE_CRIT_GENERIC
Language	= English
Generic Information.
.
;
;#define PHYSICAL_PACKAGE_ADMIN_CRIT_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(PHYSICAL_PACKAGE_CRIT_GENERIC)
;


;#endif



