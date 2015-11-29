;//++
;// Copyright (C) EMC Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;#ifndef SEP_MESSAGES_H
;#define SEP_MESSAGES_H
;
;//
;//++
;// File:            SEPMessages.h (MC)
;//
;// Description:     This file defines SEP (storage extent package) Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  SEP_PACKAGE_xxx and SEP_PACKAGE_ADMIN_xxx.
;//
;// Notes:
;//
;//  Information Status Codes Range:  0x0000-0x3FFF
;//  Warning Status Codes Range:      0x4000-0x7FFF
;//  Error Status Codes Range:        0x8000-0xBFFF
;//  Error Status Codes Range:        0x8000-0xBFFF
;//  Critical Status Codes Range:     0xC000-0xFFFF
;//
;// History:         25-June-10       Pradnya Patankar     Created
;//--
;//
;//
;#include "k10defs.h"
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( SEP = 0x168 : FACILITY_SEP_PACKAGE )

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
Facility	= SEP
SymbolicName	= SEP_INFO_GENERIC
Language	= English
Generic Information.
.
;
;#define SEP_ADMIN_INFO_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_GENERIC)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x0001
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LOAD_VERSION
Language	= English
Driver compiled on %2 at %3.
.
;
;#define SEP_ADMIN_INFO_LOAD_VERSION                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LOAD_VERSION)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x0002
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED
Language	= English
Sector Reconstructed RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_INFO_RAID_HOST_SECTOR_RECONSTRUCTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED)

;//-----------------------------------------------------------------------------

MessageId	= 0x0003
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
Language	= English
Parity Sector Reconstructed RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED)

;//-----------------------------------------------------------------------------

MessageId	= 0x0004
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA
Language	= English
Sector Reconstructed with new data RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA)

;//-----------------------------------------------------------------------------


MessageId	= 0x0005
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_PARITY_WRITE_LOG_FLUSH_ABANDONED
Language	= English
Write log flush abandoned for RG: %2 Pos bitmap: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7 Slot: %8 Time: %9 %10
.
;
;#define SEP_ADMIN_INFO_RAID_PARITY_WRITE_LOG_FLUSH_ABANDONED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_PARITY_WRITE_LOG_FLUSH_ABANDONED)

;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;//  Space left open for additional generic info messages 
;//-----------------------------------------------------------------------------


;//-------Available/unused Messaged Id 0x0101 and 0x0104 -------------------------------

;//-----------------------------------------------------------------------------
MessageId	= 0x0100
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_STARTED
Language	= English
Rebuild for RAID group %2 (obj %3) position %4 to disk %5_%6_%7 started 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_STARTED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_STARTED)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x0102
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_COMPLETED
Language	= English
Rebuild of all LUNs in RAID group %2 (obj %3) position %4 to disk %5_%6_%7 finished 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_COMPLETED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_COMPLETED)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x0103
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_STARTED
Language	= English
Copy from disk %2_%3_%4 to disk %5_%6_%7 started
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_RAID_GROUP_COPY_STARTED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0105
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_COMPLETED
Language	= English
Copy of all LUNs/all RAID groups from disk %2_%3_%4 to disk %5_%6_%7 finished 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_RAID_GROUP_COPY_COMPLETED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0106
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LUN_OBJECT_LUN_ZEROING_STARTED
Language	= English
Zeroing of Flare LUN %2 (obj %3) started 
.
;
;#define SEP_ADMIN_INFO_FBE_LUN_OBJECT_LUN_ZEROING_STARTED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LUN_OBJECT_LUN_ZEROING_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0107
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED
Language	= English
Zeroing of Flare LUN %2 (obj 0x%3) finished 
.
;
;#define SEP_ADMIN_INFO_FBE_LUN_OBJECT_LUN_ZEROING_COMPLETED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0108
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED
Language	= English
Zeroing of disk %2_%3_%4(obj %5)  started 
.
;
;#define SEP_ADMIN_INFO_FBE_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0109
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED
Language	= English
Zeroing of disk %2_%3_%4(obj %5)  finished 
.
;
;#define SEP_ADMIN_INFO_FBE_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED)


;//-----------------------------------------------------------------------------
MessageId	= 0x010A
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_USER_RW_BCKGRND_VERIFY_STARTED
Language	= English
User initiated verify for Flare LUN %2 (obj %3) started 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_USER_RW_BCKGRND_VERIFY_STARTED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_USER_RW_BCKGRND_VERIFY_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x010B
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_USER_RO_BCKGRND_VERIFY_STARTED
Language	= English
User initiated read only verify for Flare LUN %2 (obj 0x%3) started 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_USER_RO_BCKGRND_VERIFY_STARTED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_USER_RO_BCKGRND_VERIFY_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x010E
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STARTED
Language	= English
rebuild logging started on disk position %2 in RG %3 (obj %4)
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_DISK_RB_LOGGING_STARTED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x010F
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STOPPED
Language	= English
rebuild logging stopped on position %2 in RG %3 (obj %4)
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_DISK_RB_LOGGING_STOPPED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STOPPED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0110
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED
Language	= English
Permanent Spare Swap-in operation is initiated.Permanent disk:%2_%3_%4,Failed disk:%5_%6_%7
.
;
;#define SEP_ADIMN_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0111
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN
Language	= English
Permanent spare swapped-in.Disk %2_%3_%4,serial num:%5 is permanently replacing disk %6_%7_%8,serial num:%9 
.
;
;#define SEP_ADMIN_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0112
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED
Language	= English
Proactive Spare Swap-in operation is initiated.Permanent disk:%2_%3_%4,Failed disk:%5_%6_%7
.
;
;#define SEP_ADMIN_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0113
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN
Language	= English
Proactive spare swapped-in.Disk %2_%3_%4,serial num:%5 is permanently replacing disk %6_%7_%8,serial num:%9
.
;
;#define SEP_ADMIN_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0114
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_DRIVE_SWAP_OUT
Language	= English
Copy operation swapped out disk:%2_%3_%4
.
;
;#define SEP_ADMIN_INFO_SPARE_DRIVE_SWAP_OUT\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_DRIVE_SWAP_OUT)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0115
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_COPY_OPERATION_COMPLETED
Language	= English
Copy operation completed.Permanent disk:%2_%3_%4,Original disk:%5_%6_%7
.
;
;#define SEP_ADMIN_INFO_SPARE_COPY_OPERATION_COMPLETED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_COPY_OPERATION_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0116
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SNIFF_VERIFY_ENABLED
Language	= English
Sniff verify enabled on obj 0x%2 by user.Disk Info:%3_%4_%5
.
;
;#define SEP_ADMIN_SNIFF_VERIFY_ENABLED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SNIFF_VERIFY_ENABLED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0117
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SNIFF_VERIFY_DISABLED
Language	= English
Sniff verify disabled on obj 0x%2 by user.Disk Info:%3_%4_%5
.
;
;#define SEP_ADMIN_SNIFF_VERIFY_DISABLED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SNIFF_VERIFY_DISABLED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0118
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_USER_COPY_INITIATED
Language	= English
User copy is initiated.Destination disk:%2_%3_%4,Original disk:%5_%6_%7
.
;
;#define SEP_ADMIN_INFO_SPARE_USER_COPY_INITIATED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_USER_COPY_INITIATED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0119
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LUN_CREATED
Language	= English
LUN created with obj id 0x%2 and Flare LUN %3
.
;
;#define SEP_ADMIN_LUN_CREATED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LUN_CREATED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x011A
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LUN_DESTROYED
Language	= English
LUN destroyed with Flare LUN %2.
.
;
;#define SEP_ADMIN_INFO_LUN_DESTROYED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LUN_DESTROYED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x011B
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED
Language	= English
LU automatic verify for Flare LUN %2 (obj 0x%3) started 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x011C
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE
Language	= English
LU automatic verify for Flare LUN %2 (obj 0x%3) complete 
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE)
;

;//----------------------------------------------------SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED-------------------------
MessageId	= 0x011D
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN
Language	= English
User copy swapped-in.Disk %2_%3_%4,serial num:%5 is permanently replacing disk %6_%7_%8,serial num:%9 
.
;
;#define SEP_ADMIN_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x011E
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_USER_BCKGRND_VERIFY_COMPLETED
Language	= English
User initiated verify for Flare LUN %2 (obj 0x%3) finished 
.
;
;#define SEP_ADMIN_INFO_USER_BCKGRND_VERIFY_COMPLETED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_USER_BCKGRND_VERIFY_COMPLETED)

;
;//-----------------------------------------------------------------------------
MessageId	= 0x011F
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_USER_RO_BCKGRND_VERIFY_COMPLETED
Language	= English
User initiated read only verify for Flare LUN %2 (obj 0x%3) finished 
.
;
;#define SEP_ADMIN_INFO_USER_RO_BCKGRND_VERIFY_COMPLETED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_USER_RO_BCKGRND_VERIFY_COMPLETED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0120
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED
Language	= English
Permanent spare request for %2_%3_%4,serial num:%5 denied. RAID Group unconsumed, broken or non-redundant
.
;
;#define SEP_ADMIN_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0121
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED
Language	= English
Proactive copy request for %2_%3_%4,serial num:%5 denied. RAID Group unconsumed, degraded or non-redundant
.
;
;#define SEP_ADMIN_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0122
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_USER_COPY_REQUEST_DENIED
Language	= English
User copy request for %2_%3_%4,serial num:%5 denied. RAID Group unconsumed, degraded or non-redundant
.
;
;#define SEP_ADMIN_INFO_SPARE_USER_COPY_REQUEST_DENIED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_USER_COPY_REQUEST_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0123
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PROACTIVE_COPY_NO_LONGER_REQUIRED
Language	= English
Disk %2_%3_%4,serial num:%5. No longer requires proactive copy
.
;
;#define SEP_ADMIN_INFO_SPARE_PROACTIVE_COPY_NO_LONGER_REQUIRED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PROACTIVE_COPY_NO_LONGER_REQUIRED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0124
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_PERMANENT_SPARE_NO_LONGER_REQUIRED
Language	= English
Disk %2_%3_%4,serial num:%5 has been re-inserted.  Permanent spare not required
.
;
;#define SEP_ADMIN_INFO_SPARE_PERMANENT_SPARE_NO_LONGER_REQUIRED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_PERMANENT_SPARE_NO_LONGER_REQUIRED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0125
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR
Language	= English
Expected Coherency Error RG: %2 Pos: %3 LBA: %4 Blocks: %5 Err info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_INFO_SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x0126
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN
Language	= English
User copy - spare swapped-in.Disk %2_%3_%4,serial num:%5 is permanently replacing disk %6_%7_%8,serial num:%9
.
;
;#define SEP_ADMIN_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0127
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_GROUP_CREATED
Language	= English
RAID Group created with obj id 0x%2 and RG number %3
.
;
;#define SEP_ADMIN_RAID_GROUP_CREATED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_GROUP_CREATED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0128
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_GROUP_DESTROYED
Language	= English
RAID Group destroyed with obj id 0x%2 and RG number %3
.
;
;#define SEP_ADMIN_RAID_GROUP_DESTROYED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_GROUP_DESTROYED)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x0129
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LUN_CREATED_FBE_CLI
Language	= English
LUN (object ID 0x%2) is create with FbeCli.exe with status %3
.
;
;#define SEP_ADMIN_LUN_CREATED_FBE_CLI\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LUN_CREATED_FBE_CLI)
;
;//-----------------------------------------------------------------------------
MessageId	= 0x012A
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_LUN_DESTROYED_FBE_CLI
Language	= English
LUN (object ID 0x%2) is destroyed with FbeCli.exe with status %3
.
;
;#define SEP_ADMIN_LUN_DESTROYED_FBE_CLI\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_LUN_DESTROYED_FBE_CLI)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x012B
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_DISABLED
Language	= English
Error Thresholds for RAID group %2 (obj %3) position mask %4 have been disabled
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_DISABLED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_DISABLED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x012C
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_RESTORED
Language	= English
Error Thresholds for RAID group %2 (obj %3) position mask %4 have been restored
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_RESTORED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_RESTORED)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x012D
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_DISABLED
Language	= English
Proactive spare RAID group %2 (obj %3) position %4 error thresholds for position mask %5 have been disabled
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_DISABLED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_DISABLED)
;


;//-----------------------------------------------------------------------------
MessageId	= 0x012E
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_RESTORED
Language	= English
Proactive spare RAID group %2 (obj %3) position %4 error thresholds for position mask %5 have been restored
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_RESTORED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_RESTORED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x012F
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_DISABLED
Language	= English
Proactive spare Resiliency has been disabled persist %d
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_DISABLED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_DISABLED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0130
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_ENABLED
Language	= English
Proactive spare Resiliency has been enabled persist %d
.
;
;#define SEP_ADMIN_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_ENABLED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_ENABLED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0131
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SYSTEM_DISK_END_OF_LIFE_CLEAR
Language	= English
Disk in slot 0_0_%2 EOL state has been cleared, serial num:%3 
.
;
;#define SEP_ADMIN_INFO_SYSTEM_DISK_END_OF_LIFE_CLEAR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SYSTEM_DISK_END_OF_LIFE_CLEAR)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0132
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_USER_DISK_END_OF_LIFE_CLEAR
Language	= English
Disk in slot %2_%3_%4 EOL state has been cleared, serial num:%5 
.
;
;#define SEP_ADMIN_INFO_USER_DISK_END_OF_LIFE_CLEAR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_USER_DISK_END_OF_LIFE_CLEAR)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0133
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR
Language	= English
Disk in slot 0_0_%2 Drive Fault has been cleared, serial num:%3 
.
;
;#define SEP_ADMIN_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x0134
Severity	= Informational
Facility	= SEP
SymbolicName	= SEP_INFO_USER_DISK_DRIVE_FAULT_CLEAR
Language	= English
Disk in slot %2_%3_%4 Drive Fault state has been cleared, serial num:%5  
.
;
;#define SEP_ADMIN_INFO_USER_DISK_DRIVE_FAULT_CLEAR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_INFO_USER_DISK_DRIVE_FAULT_CLEAR)
;

;// ----------------------Add all Informational Messages above this ----------------------
;//
;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x4000
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_GENERIC
Language	= English
Generic Information.
.
;
;#define SEP_ADMIN_WARN_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x4001
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_SPARE_NO_SPARES_AVAILABLE
Language	= English
There are no spares in the system to replace failed disk:%2_%3_%4,serial num:%5
.
;
;#define SEP_ADMIN_WARN_SPARE_NO_SPARES_AVAILABLE\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_SPARE_NO_SPARES_AVAILABLE)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x4002
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE
Language	= English
There is no suitable spare to replace failed disk:%2_%3_%4,serial num:%5
.
;
;#define SEP_ADMIN_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x4003
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_SYSTEM_DISK_NEED_OPERATION
Language	= English
Disk in slot 0_0_%2 is %3 maynot be consumed. Use fbecli can force it online if it could not be online. 
.
;
;#define SEP_ADMIN_WARN_SYSTEM_DISK_NEED_OPERATION                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_SYSTEM_DISK_NEED_OPERATION)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x4004
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_SYSTEM_DISK_NEED_REPLACEMENT
Language	= English
Disk in slot 0_0_%2 needs to be replaced, serial num:%3 
.
;
;#define SEP_ADMIN_WARN_SYSTEM_DISK_NEED_REPLACEMENT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_SYSTEM_DISK_NEED_REPLACEMENT)

;//-----------------------------------------------------------------------------
MessageId	= 0x4005
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_USER_DISK_END_OF_LIFE
Language	= English
Disk in slot %2_%3_%4 is in EOL state, serial num:%5 
.
;
;#define SEP_ADMIN_WARN_USER_DISK_END_OF_LIFE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_USER_DISK_END_OF_LIFE)

;//-----------------------------------------------------------------------------
MessageId	= 0x4006
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_SYSTEM_DISK_DRIVE_FAULT
Language	= English
Disk in slot 0_0_%2 is drive fault and needs to be replaced, serial num:%3 
.
;
;#define SEP_ADMIN_WARN_SYSTEM_DISK_DRIVE_FAULT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_SYSTEM_DISK_DRIVE_FAULT)

;//-----------------------------------------------------------------------------
MessageId	= 0x4007
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_USER_DISK_DRIVE_FAULT
Language	= English
Disk in slot %2_%3_%4 is in Drive Fault state, serial num:%5 
.
;
;#define SEP_ADMIN_WARN_USER_DISK_DRIVE_FAULT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_USER_DISK_DRIVE_FAULT)

;//-----------------------------------------------------------------------------
MessageId	= 0x4008
Severity	= Warning
Facility	= SEP
SymbolicName	= SEP_WARN_TWO_OF_THE_FIRST_THREE_SYSTEM_DRIVES_FAILED
Language	= English
Two of the first three system drives are failed at booting time. 
.
;
;#define SEP_ADMIN_WARN_TWO_OF_THE_FIRST_THREE_SYSTEM_DRIVES_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_WARN_TWO_OF_THE_FIRST_THREE_SYSTEM_DRIVES_FAILED)

;//--------------------- Add all Warning messages above this -------------------
;//
;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0x8000
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_GENERIC
Language	= English
Generic Information.
.
;
;#define SEP_ADMIN_ERROR_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_GENERIC)
;
;//-----------------------------------------------------------------------------

MessageId	= 0x8001
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED
Language	= English
Data Sector Invalidated RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED)

;//-----------------------------------------------------------------------------

MessageId	= 0x8002
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR
Language	= English
Uncorrectable Data Sector RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR)

;//-----------------------------------------------------------------------------

MessageId	= 0x8003
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR
Language	= English
Uncorrectable Parity Sector RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR)

;//-----------------------------------------------------------------------------
MessageId	= 0x8007
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED
Language	= English
Parity Invalidated RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8008
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR
Language	= English
Uncorrectable Sector RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR)

;//-----------------------------------------------------------------------------
MessageId	= 0x800A
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR
Language	= English
The System Drives are out of order. Disk0 should be in slot%2, disk1 in slot%3, disk2 in slot%4, disk3 in slot%5
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x800B
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR
Language	= English
System integrity broken:0_0_0: %2, 0_0_1: %3, 0_0_2: %4, 0_0_3: %5.
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR                                 \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x800C
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_CHASSIS_MISMATCHED_ERROR
Language	= English
Chassis is mismatched with system drives. If you want to replace chassis, please set the Chassis Replacement Flag.
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_CHASSIS_MISMATCH_ERROR                                   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_CHASSIS_MISMATCHED_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x800D
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR
Language	= English
An unexpected error(%2) occurred when attempting to replace failed disk:%3_%4_%5
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_UNEXPECTED_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x800E
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS
Language	= English
FLARE LUN %2 (Obj %3) encountered unexpected errors. 
.
;
;#define SEP_ADMIN_ERROR_CODE_LUN_FAILED_FOR_UNEXPECTED_ERRORS                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS)

;
;//-----------------------------------------------------------------------------
MessageId	= 0x800F
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_NEWER_CONFIG_ON_DISK
Language	= English
Database configuration entry size(size: 0x%2) loaded from disk is larger than the entry size(size: 0x%3) in software
.
;
;#define SEP_ADMIN_ERROR_CODE_NEWER_CONFIG_ON_DISK                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_NEWER_CONFIG_ON_DISK)

;//-----------------------------------------------------------------------------
MessageId	= 0x8010
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER
Language	= English
Database config(size: 0x%2) received from Peer is larger than the config(size: 0x%3) in software
.
;
;#define SEP_ADMIN_ERROR_CODE_NEWER_CONFIG_FROM_PEER                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER)

;//-----------------------------------------------------------------------------
MessageId	= 0x8011
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_EMV_MISMATCH_WITH_PEER
Language	= English
Local expected memory value (0x%2) mismatches with Peer Shared expected memory value(0x%3) 
.
;
;#define SEP_ADMIN_ERROR_CODE_EMV_MISMATCH_WITH_PEER                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_EMV_MISMATCH_WITH_PEER)

;//-----------------------------------------------------------------------------
MessageId	= 0x8012
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_MEMORY_UPGRADE_FAIL
Language	= English
After upgrade, the memory(0x%2) is neither the source memory size(0x%3) nor the target memory size(0x%4)
.
;
;#define SEP_ADMIN_ERROR_CODE_MEMORY_UPDATE_FAIL                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_MEMORY_UPDATE_FAIL)

;//-----------------------------------------------------------------------------
MessageId	= 0x8013
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SYSTEM_DB_HEADER_CORRUPTED
Language	= English
The system DB header is corrupted.
.
;
;#define SEP_ADMIN_ERROR_CODE_SYSTEM_DB_HEADER_CORRUPTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SYSTEM_DB_HEADER_CORRUPTED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8014
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED
Language	= English
The system db content is corrupted.
.
;
;#define SEP_ADMIN_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED                              \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8015
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SYSTEM_DB_HEADER_RW_ERROR
Language	= English
Read/Write system DB header fail.
.
;
;#define SEP_ADMIN_ERROR_CODE_SYSTEM_DB_HEADER_RW_ERROR                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SYSTEM_DB_HEADER_RW_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x8016
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SYSTEM_DB_CONTENT_RW_ERROR
Language	= English
Read/Write system db content fail.
.
;
;#define SEP_ADMIN_ERROR_CODE_SYSTEM_DB_CONTENT_RW_ERROR                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_RW_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0x8017
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_TOO_SMALL_DB_DRIVE
Language	= English
%2 Database drive(s) is too small to create the RAID defined in PSL. Use a drive equal to other system drives.
.
;
;#define SEP_ADMIN_ERROR_CODE_TOO_SMALL_DB_DRIVE                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_TOO_SMALL_DB_DRIVE)

;//-----------------------------------------------------------------------------
MessageId	= 0x8018
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED
Language	= English
Copy operation failed because source disk:%2_%3_%4 was removed
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8019
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED
Language	= English
Copy operation failed because destination disk:%2_%3_%4 was removed.
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED)

;//-----------------------------------------------------------------------------
MessageId	= 0x801A
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_UNEXPECTED_COMMAND
Language	= English
Unexpected command(%2) for spare operation to disk:%3_%4_%5
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_UNEXPECTED_COMMAND                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_UNEXPECTED_COMMAND)

;//-----------------------------------------------------------------------------
MessageId	= 0x801B
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_SOURCE_DRIVE_INVALID
Language	= English
The source disk:%2_%3_%4 is not part of a RAID group. Make sure you chose the correct source disk.
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_SOURCE_DRIVE_INVALID                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_SOURCE_DRIVE_INVALID)

;//-----------------------------------------------------------------------------
MessageId	= 0x801C
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_SOURCE_IN_UNEXPECTED_STATE
Language	= English
Source disk:%2_%3_%4 is in an unexpected state. Remove and re-insert the source disk
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_SOURCE_IN_UNEXPECTED_STATE                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_SOURCE_IN_UNEXPECTED_STATE)

;//-----------------------------------------------------------------------------
MessageId	= 0x801D
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_DRIVE_VALIDATION_FAILED
Language	= English
Validation of replacement disk for original disk:%2_%3_%4 failed. Choose a different destination disk
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_DRIVE_VALIDATION_FAILED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_DRIVE_VALIDATION_FAILED)

;//-----------------------------------------------------------------------------
MessageId	= 0x801E
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_DESTINATION_INCOMPATIBLE_TIER
Language	= English
Destination disk:%2_%3_%4 is not compatible with original disk:%5_%6_%7. Choose a different destination disk
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_DESTINATION_INCOMPATIBLE_TIER                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_DESTINATION_INCOMPATIBLE_TIER)

;//-----------------------------------------------------------------------------
MessageId	= 0x801F
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS
Language	= English
A sparing or copy operation is already in progress on disk:%2_%3_%4. Wait until copy is completed
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS)

;//-----------------------------------------------------------------------------
MessageId	= 0x8020
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_UNCONSUMED
Language	= English
Policy does not allow a sparing or copy operation to to an empty RAID group disk:%2_%3_%4
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_RAID_GROUP_IS_UNCONSUMED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_UNCONSUMED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8021
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT
Language	= English
Policy does not allow a sparing or copy operation to a non-redundant RAID group disk:%2_%3_%4
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT)

;//-----------------------------------------------------------------------------
MessageId	= 0x8022
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED
Language	= English
Policy does not allow copy operations to a degraded raid group disk:%2_%3_%4
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_RAID_GROUP_DEGRADED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8023
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_BLOCK_SIZE_MISMATCH
Language	= English
Destination disk:%2_%3_%4 has a different block size from original:%5_%6_%7. Choose a compatible destination disk
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_BLOCK_SIZE_MISMATCH                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_BLOCK_SIZE_MISMATCH)

;//-----------------------------------------------------------------------------
MessageId	= 0x8024
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY
Language	= English
Destination disk:%2_%3_%4 has less capacity than original:%5_%6_%7
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY)

;//-----------------------------------------------------------------------------
MessageId	= 0x8025
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DESTROYED
Language	= English
The spare or copy operation to disk:%2_%3_%4 failed because the RAID group no longer exists
.
;
;#define SEP_ADMIN_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DESTROYED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DESTROYED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8026
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED
Language	= English
The spare or copy operation to disk:%2_%3_%4 was denied by RAID group. Check if RAID group is degraded.
.
;
;#define SEP_ADMIN_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED)

;//-----------------------------------------------------------------------------
MessageId	= 0x8027
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_DESTINATION_OFFSET_MISMATCH
Language	= English
Destination disk:%2_%3_%4 bind offset is greater than original:%5_%6_%7. Choose another destination disk
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_DESTINATION_OFFSET_MISMATCH                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_DESTINATION_OFFSET_MISMATCH)

;//-----------------------------------------------------------------------------
MessageId	= 0x8028
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT
Language	= English
The bind user disk with PVD id:%2 in system slot:%3_%4_%5. Please pull out it and return to user drive slot.
.
;

;#define SEP_ADMIN_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT)

;//-----------------------------------------------------------------------------
MessageId	= 0x8029
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT
Language	= English
The system disk (with PVD id:%2) is in the wrong slot:%3_%4_%5. It should be in:%6_%7_%8
.
;

;#define SEP_ADMIN_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT                            \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT)


;//-----------------------------------------------------------------------------
MessageId	= 0x802A
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS
Language	= English
Policy does not allow a new copy to a RAID group with a copy in progress disk:%2_%3_%4
.
;
;#define SEP_ADMIN_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS                     \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS)

;//-----------------------------------------------------------------------------
MessageId	= 0x802B
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN
Language	= English
Policy does not allow a sparing or copy operation to a broken RAID group disk:%2_%3_%4
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN)


;//-----------------------------------------------------------------------------
MessageId	= 0x802C
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY
Language	= English
The copy object is not in the Ready state for disk:%2_%3_%4
.
;
;#define SEP_ADMIN_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY)

;//-----------------------------------------------------------------------------
MessageId	= 0x802D
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED
Language	= English
Attempt to abort copy operation to disk %2_%3_%4 from disk %5_%6_%7 denied 
.
;
;#define SEP_ADMIN_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x802E
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED
Language	= English
Currently disk %2_%3_%4 is degraded.  Retry operation later 
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED	\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED)
;

;//-----------------------------------------------------------------------------
MessageId	= 0x802F
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_INVALID_DESTINATION_DRIVE
Language	= English
Copy operation failed because destination disk:%2_%3_%4 cannot be found.
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_INVALID_DESTINATION_DRIVE                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_INVALID_DESTINATION_DRIVE)

;//-----------------------------------------------------------------------------
MessageId	= 0x8030
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY
Language	= English
Copy operation failed because destination disk:%2_%3_%4 is faulted
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY)

;//-----------------------------------------------------------------------------
MessageId	= 0x8031
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_IN_USE
Language	= English
Copy operation failed because destination disk:%2_%3_%4 is part of a RAID group
.
;
;#define SEP_ADMIN_ERROR_CODE_COPY_DESTINATION_DRIVE_IN_USE                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_IN_USE)

;//-----------------------------------------------------------------------------
MessageId	= 0x8032
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_DESTINATION_INCOMPATIBLE_RAID_GROUP_OFFSET
Language	= English
Copy operation failed because destination disk:%2_%3_%4 has an incompatible bind offset
.
;
;#define SEP_ADMIN_ERROR_CODE_DESTINATION_INCOMPATIBLE_RAID_GROUP_OFFSET                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_DESTINATION_INCOMPATIBLE_RAID_GROUP_OFFSET)

;//-----------------------------------------------------------------------------
MessageId	= 0x8033
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS
Language	= English
usermodifiedWwnSeedFlag is set through unisphere, meantime, chassis_replacement_movement flag is set through fbecli.
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS)

;//-----------------------------------------------------------------------------
MessageId	= 0x8034
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_NOT_ALL_DRIVES_SET_ICA_FLAGS
Language	= English
Not all drives are set ICA flags. (drive0,drive1,drive2)=>(%2, %3, %4) (1=flags is not valid;2=flags is set; 3=flags is not set).
.
;
;#define SEP_ADMIN_ERROR_CODE_NOT_ALL_DRIVES_SET_ICA_FLAGS                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_NOT_ALL_DRIVES_SET_ICA_FLAGS)

;//-----------------------------------------------------------------------------
MessageId	= 0x8035
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT
Language	= English
RAID type RAID-%2 does not support a drive count of %3
.
;
;#define SEP_ADMIN_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT)

;//-----------------------------------------------------------------------------
MessageId	= 0x8036
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY
Language	= English
Drives selected do not have sufficient capacity for request
.
;
;#define SEP_ADMIN_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY)

;//-----------------------------------------------------------------------------
MessageId	= 0x8037
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CREATE_RAID_GROUP_INVALID_RAID_TYPE
Language	= English
The RAID type: %2 is not a supported RAID type
.
;
;#define SEP_ADMIN_ERROR_CREATE_RAID_GROUP_INVALID_RAID_TYPE                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CREATE_RAID_GROUP_INVALID_RAID_TYPE)

;//-----------------------------------------------------------------------------
MessageId	= 0x8038
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION
Language	= English
Requested RAID type RAID-%2 with drive count %3 is not a valid configuration
.
;
;#define SEP_ADMIN_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION)

;//-----------------------------------------------------------------------------
MessageId	= 0x8039
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES
Language	= English
One or more drives for RAID type RAID-%2 with drive count %3 are not compatible
.
;
;#define SEP_ADMIN_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES)

;//-----------------------------------------------------------------------------
MessageId	= 0x803A
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE
Language	= English
New system drive %2_%3_%4 is incompatible with original one. Please check its capacity or drive type
.
;
;#define SEP_ADMIN_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE)

;//-----------------------------------------------------------------------------
MessageId	= 0x803B
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_RAID_GROUP_METADATA_RECONSTRUCT_REQUIRED
Language	= English
RAID group %2 obj %3 Raid type RAID-%4 metadata reconstruction required
.
;
;#define SEP_ADMIN_ERROR_RAID_GROUP_METADATA_RECONSTRUCT_REQUIRED                             \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_RAID_GROUP_METADATA_RECONSTRUCT_REQUIRED)

;//-----------------------------------------------------------------------------
MessageId	= 0x803C
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT
Language	= English
Two system drives are invalid and at least one of them are in user slot
.
;
;#define SEP_ADMIN_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT     \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT)


;//-------------------- Add all Error Message above this -----------------------
;//-----------------------------------------------------------------------------
;//  Critical Status Codes
;//-----------------------------------------------------------------------------
MessageId	= 0xC000
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_CRIT_GENERIC
Language	= English
Generic Information.
.
;
;#define SEP_ADMIN_CRIT_GENERIC                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_CRIT_GENERIC)
;

;//-----------------------------------------------------------------------------
MessageId	= 0xC001
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR
Language	= English
Checksum Error RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0xC002
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR
Language	= English
Parity Checksum Error RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0xC003
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR
Language	= English
Coherency Error RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0xC004
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR
Language	= English
LBA Stamp Error RAID Group: %2 Position: %3 LBA: %4 Blocks: %5 Error info: %6 Extra info: %7
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR)

;//-----------------------------------------------------------------------------
MessageId	= 0xC005
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY
Language	= English
Encryption Key Incorrect for disk: %2_%3_%4 (object id: %5) SN: %6
.
;
;#define SEP_ADMIN_ERROR_SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY   \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY)

;//-----------------------------------------------------------------------------
MessageId	= 0xC006
Severity	= Error
Facility	= SEP
SymbolicName	= SEP_ERROR_CODE_PROVISION_DRIVE_WEAR_LEVELING_THRESHOLD
Language	= English
Writes exceed writes per day limit for disk: %2_%3_%4 (object id: %5) SN: %6
.
;
;#define SEP_ADMIN_ERROR_CODE_PROVISION_DRIVE_WEAR_LEVELING_THRESHOLD                           \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(SEP_ERROR_CODE_PROVISION_DRIVE_WEAR_LEVELING_THRESHOLD)

;//-----------------------------------------------------------------------------
;#endif
