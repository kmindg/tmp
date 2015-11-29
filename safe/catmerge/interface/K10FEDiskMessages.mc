;//******************************************************************************
;//      COMPANY CONFIDENTIAL:
;//      Copyright (C) EMC Corporation, 2001
;//      All rights reserved.
;//      Licensed material - Property of EMC Corporation.
;//******************************************************************************
;
;//**********************************************************************
;//.++
;// FILE NAME:
;//      K10FEDiskMessages.mc
;//
;// DESCRIPTION:
;//
;//     This file defines FEDisk's Status Codes and messages. Each Status
;//     Code has two forms, an internal status and an admin status:
;//     FEDISK_xxx and FEDISK_ADMIN_xxx.
;//
;//     Many of these messages are strictly information-only, and
;//     used only during the "Verbose Verify" option.
;//
;// CONTENTS:
;//
;//
;// REVISION HISTORY:
;//      03-Jul-07 LVidal   Several status codes were split fo fit 
;//                         KLogWriteEntry parameter's buffer size. 
;//                         The second part of the status code has a _MORE_INFO suffix.
;//      26-Jul-04 DScott   Add FEDISK_STATUS_PORT_LUN_ID_INVALID_FOR_EMC_ARRAY
;//      08-Apr-04 MHolt    Log message for inacessible Clariion LUN (104712).
;//      03-Mar-04 DScott   Add FEDISK_STATUS_SOFT_SCSI_ERROR_OCCURRED,
;//                         FEDISK_STATUS_HARD_SCSI_ERROR_OCCURRED,
;//                         FEDISK_STATUS_SOFT_IO_ERROR_OCCURRED,
;//                         FEDISK_STATUS_HARD_IO_ERROR_OCCURRED,
;//                         FEDISK_STATUS_DEVICE_REMOVED,
;//                         FEDISK_STATUS_IO_TIMEOUT,
;//                         FEDISK_STATUS_NO_PATH_TO_DEVICE,
;//                         FEDISK_STATUS_DEVICE_RECOVERY_IN_PROGRESS, and
;//                         FEDISK_STATUS_SOFT_ERRS_ABOVE_THRESHOLD.
;//      29-Apr-03 MHolt    Add FEDISK_STATUS_DEVICE_REMOVE_IN_PROGRESS (86411)
;//      19-Mar-03 MHolt    Added Reservation Conflict (85173).
;//      27-Feb-03 MHolt    Created
;//
;//.--
;//**********************************************************************
;#ifndef FEDISK_MESSAGES_H
;#define FEDISK_MESSAGES_H

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( System       = 0x0
                    RpcRuntime   = 0x2:FACILITY_RPC_RUNTIME
                    RpcStubs     = 0x3:FACILITY_RPC_STUBS
                    Io           = 0x4:FACILITY_IO_ERROR_CODE
                    SANCopy      = 0x12B:FACILITY_FEDISK
                  )

SeverityNames   = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                    Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                    Warning       = 0x2 : STATUS_SEVERITY_WARNING
                    Error         = 0x3 : STATUS_SEVERITY_ERROR
                  )

;//-----------------------------------------------------------------------------
;//  Informational Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x0001
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SEARCHING_FROM_LOCAL_PORT
Language        = English
SANCopy Disk Driver: Searching for Lun WWN %2:%3 via local SP-%4 Port %5
.

;
;#define FEDISK_ADMIN_STATUS_SEARCHING_FROM_LOCAL_PORT \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SEARCHING_FROM_LOCAL_PORT)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LOCAL_PORT_ADDR
Language        = English
SANCopy Disk Driver: Searching via Local Port address %2 (log 1 of 2).
.

;
;#define FEDISK_ADMIN_STATUS_LOCAL_PORT_ADDR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_LOCAL_PORT_ADDR)
;


;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SEARCHING_REMOTE_PATH
Language        = English
SANCopy Disk Driver: Searching target %2 (log 1 of 2).
.

;
;#define FEDISK_ADMIN_STATUS_SEARCHING_REMOTE_PATH \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SEARCHING_REMOTE_PATH)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LUN_MISMATCH_FOUND
Language        = English
SANCopy Disk Driver: WWN %2 %3 ( Target Lun %4) does NOT match (log 1 of 2).
.

;
;#define FEDISK_ADMIN_STATUS_LUN_MISMATCH_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_LUN_MISMATCH_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LUN_MATCH_FOUND
Language        = English
SANCopy Disk Driver: MATCHING WWN %2 %3 found at Lun %4 (log 1 of 2).
.

;
;#define FEDISK_ADMIN_STATUS_LUN_MATCH_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_LUN_MATCH_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_ACCESSIBLE_MATCH_FOUND
Language        = English
SANCopy Disk Driver: SUCCESS: %2 ACCESSIBLE at Lun %3 (log 1 of 2).
.

;
;#define FEDISK_ADMIN_STATUS_ACCESSIBLE_MATCH_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_ACCESSIBLE_MATCH_FOUND)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LUN_SIZE
Language        = English
SANCopy Disk Driver: Additional Lun info: Block Count %2   Block Size %3
.

;
;#define FEDISK_ADMIN_STATUS_LUN_SIZE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_LUN_SIZE)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_DGC_PUSH_FAILED
Language        = English
SANCopy Disk Driver: Push failed to target CLARiiON at address %2  model %3, status = %4.
INTERNAL USE ONLY
.

;
;#define FEDISK_ADMIN_STATUS_DGC_PUSH_FAILED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_DGC_PUSH_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_INQUIRY_FAILED
Language        = English
SANCopy Disk Driver: SCSI Inquiry failed on address %2 (log 1 of 2).
.

;
;#define FEDISK_ADMIN_STATUS_INQUIRY_FAILED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_INQUIRY_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_UNSUPPORTED_DEVICE
Language        = English
SANCopy Disk Driver: Unsupported device at address %2, LUN %3, DeviceType %4, DeviceTypeQualifier %5
via local SP-%6 Port %7
.

;
;#define FEDISK_ADMIN_STATUS_UNSUPPORTED_DEVICE\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_UNSUPPORTED_DEVICE)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_DEVICE_NOT_READY
Language        = English
SANCopy Disk Driver: Lun may be owned by peer sp. Check Lun status & SP ownership on target system.
Device not ready: Address %2, Lun %3, cmd %4, status %5
.

;
;#define FEDISK_ADMIN_STATUS_DEVICE_NOT_READY\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_DEVICE_NOT_READY)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_CANT_GET_VPD
Language        = English
SANCopy Disk Driver: Can't get VPD (Vital Product Inquiry Data) from Lun %2, address %3,
via local SP-%4 Port %5  :  status = %6
.

;
;#define FEDISK_ADMIN_STATUS_CANT_GET_VPD\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_CANT_GET_VPD)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SCSI_STATUS_SENSE_INFO
Language        = English
SANCopy Disk Driver: SCSI info: ScsiStat %2, SK %3, ASC/Q %4 %5, SrbStat: %6, info: %7,  FRUcode/SKSV: %8
.

;
;#define FEDISK_ADMIN_STATUS_ADDITIONAL_ERROR_INFO\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_ADDITIONAL_ERROR_INFO)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_NOT_FOUND_ON_THIS_PATH
Language        = English
SANCopy Disk Driver: Lun %2 %3 not found on target at address %4 via local SP-%5 Port %6
.

;
;#define FEDISK_ADMIN_STATUS_NOT_FOUND_ON_THIS_PATH\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_NOT_FOUND_ON_THIS_PATH)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SEARCH_FAILED
Language        = English
SANCopy Disk Driver: Search failed : could not find or access %2 : status = %3
.

;
;#define FEDISK_ADMIN_STATUS_SEARCH_FAILED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SEARCH_FAILED)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_REPORT_LUNS_FAILED
Language        = English
SANCopy Disk Driver: Can't get ReportLun data from address %2, Lun %3 (log 1 of 2). 
.
;
;#define FEDISK_ADMIN_STATUS_REPORT_LUNS_FAILED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_REPORT_LUNS_FAILED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SOFT_SCSI_ERROR_OCCURRED
Language        = English
SANCopy Disk Driver: A non-fatal SCSI error occurred on target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_SOFT_SCSI_ERROR_OCCURRED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SOFT_SCSI_ERROR_OCCURRED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_HARD_SCSI_ERROR_OCCURRED
Language        = English
SANCopy Disk Driver: A fatal SCSI error occurred on target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_HARD_SCSI_ERROR_OCCURRED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_HARD_SCSI_ERROR_OCCURRED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SOFT_IO_ERROR_OCCURRED
Language        = English
SANCopy Disk Driver: A non-fatal I/O error occurred on target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_SOFT_IO_ERROR_OCCURRED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SOFT_IO_ERROR_OCCURRED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_HARD_IO_ERROR_OCCURRED
Language        = English
SANCopy Disk Driver: A fatal I/O error occurred on target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_HARD_IO_ERROR_OCCURRED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_HARD_IO_ERROR_OCCURRED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_IO_TIMEOUT
Language        = English
SANCopy Disk Driver: Fatal error -- retries exhausted for an I/O request issued to target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_IO_TIMEOUT\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_IO_TIMEOUT)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_NO_PATH_TO_DEVICE
Language        = English
SANCopy Disk Driver: Fatal error -- no path to target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_NO_PATH_TO_DEVICE\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_NO_PATH_TO_DEVICE)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_DEVICE_RECOVERY_IN_PROGRESS
Language        = English
SANCopy Disk Driver: Non-fatal error -- an I/O request was sent to a device, target %2, that is in the process of recovering from a link-level event.  The I/O request will be retried (log 1 of 2).
.
;
;#define FEDISK_ADMIN_DEVICE_RECOVERY_IN_PROGRESS\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_DEVICE_RECOVERY_IN_PROGRESS)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 16
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SOFT_ERRS_ABOVE_THRESHOLD
Language        = English
SANCopy Disk Driver: Non-fatal SCSI errors above reporting threshold for target %2 (log 1 of 2).
.
;
;#define FEDISK_ADMIN_STATUS_SCSI_ERRS_ABOVE_THRESHOLD\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SCSI_ERRS_ABOVE_THRESHOLD)
;

;//-----------------------------------------------------------------------------
;// Introduced In: Release 14
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//    This message will be sent to the event log only when FEDisk encounters
;//    the 05/25/01 SK/ASC/ASCQ on request issued to a device on the FAR port.
;//
;//    When an MirrorView/A secondary image starts to synchronize,
;//    the sync operation may fail if there is not enough free space
;//    in the Reserved Lun Pool on the secondary array because the
;//    Snap LUN on the secondary cannot be activated.
;//
;//    Although certain SCSI commmands isssued to the unactivated target Snap
;//    LUN will succeed (Inquiry, TestUnitReady, etc.) , Reads & Writes
;//    will fail with Illegal Request, Snap LUN Not Activated (05/25/01).
;//
;//    Therefore, a "Verify" will succeed, and the session will start,
;//    but IOs will eventually fail, causing the session to fail with
;//    "Unable to Locate Device". The intent is for the operator to be
;//    able to relate this error message to the lack of Free Space on
;//    the secondary.
;//
;//    Also note that this situation could occur in the context of issuing
;//    IO to a real Snap LUN, but that situation is considered unlikely
;//    since the session would either be writing to a Snap LUN, or reading
;//    from a Snap LUN that is running out of Snap cache space. The former
;//    is a "bad idea", and the latter is covered by the "Source Failed"
;//    return code, so it's OK for us to mention MV/A in this message.

MessageId       = +1
Severity        = Informational
Facility        = SanCopy
SymbolicName    = FEDISK_STATUS_ILLEGAL_REQUEST_UNACTIVATED_SNAP_LUN
Language        = English
FEDisk: Unable to locate device. This error may cause a MirrorView/A secondary fracture. Check Reserved LUN Pool Free Space on MirrorView/A secondary array.
.

;
;#define FEDISK_ADMIN_STATUS_ILLEGAL_REQUEST_UNACTIVATED_SNAP_LUN\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_ILLEGAL_REQUEST_UNACTIVATED_SNAP_LUN)
;

;//-----------------------------------------------------------------------------
;// Introduced in Release 18
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Description: When a "sancopy -verify -log" operation finds the matching Front-End WWN
;//              device. Log the "QFull" parameters for the device. i.e. Log the
;//              Device Extension's "LURequestQueueCurrentSize", "LURequestQueueMaxSize",
;//              "LURetryQueueCurrentSize", and the "LURetryQueueMaxSize".
;//
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LUN_QFULL_PARAMETERS
Language        = English
SANCopy Disk Driver: %2 Request Queue Size= %3 Max= %4, Retry Queue Size= %5 Max= %6
.

;
;#define FEDISK_ADMIN_STATUS_LUN_QFULL_PARAMETERS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_LUN_QFULL_PARAMETERS)
;

;// The following codes are part of another message (look for the same code name 
;// without the _MORE_INFO suffix). These error codes were split in this way to fit the 
;// KLogWriteEntry parameter's buffer size. If you plan to split a code message, 
;// please add it here.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SEARCHING_REMOTE_PATH_MORE_INFO
Language        = English
SANCopy Disk Driver: Searching via local SP-%2 Port %3 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LUN_MISMATCH_FOUND_MORE_INFO
Language        = English
SANCopy Disk Driver: LUN mismatch found at address %2, via Local SP-%3 port %4 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LUN_MATCH_FOUND_MORE_INFO
Language        = English
SANCopy Disk Driver: LUN matching at address %2, via Local SP-%3 port %4 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_ACCESSIBLE_MATCH_FOUND_MORE_INFO
Language        = English
SANCopy Disk Driver: Success access at Address %2, via Local SP-%3 port %4 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_INQUIRY_FAILED_MORE_INFO
Language        = English
SANCopy Disk Driver: SCSI Inquiry failure occured on Lun %3, via local SP-%4 Port %5, status: %6 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_REPORT_LUNS_FAILED_MORE_INFO
Language        = English
SANCopy Disk Driver: Can't get ReportLun data via local SP-%2 Port %3, status: %4 (log 2 of 2). 
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SOFT_SCSI_ERROR_OCCURRED_MORE_INFO
Language        = English
SANCopy Disk Driver: Non-fatal SCSI error additional information: Target LUN %2 (SP %3 port %4):  SCSI status %5, SK/ASC/ASCQ %6/%7/%8.  The I/O request will be retried. (log 2 of 2) 
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_HARD_SCSI_ERROR_OCCURRED_MORE_INFO
Language        = English
SANCopy Disk Driver: Fatal SCSI error additional information: LUN %2 (SP %3 port %4):  SCSI status %5, SK/ASC/ASCQ %6/%7/%8 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SOFT_IO_ERROR_OCCURRED_MORE_INFO
Language        = English
SANCopy Disk Driver: Non-fatal I/O error additional information: LUN %2 (SP %3 port %4):  request status %5 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_HARD_IO_ERROR_OCCURRED_MORE_INFO
Language        = English
SANCopy Disk Driver: Fatal I/O error additional information: LUN %2 (SP %3 port %4): request status %5 (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_IO_TIMEOUT_MORE_INFO
Language        = English
SANCopy Disk Driver: Fatal Error additional information: Retries exhausted on LUN %2 (SP %3 port %4) (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_NO_PATH_TO_DEVICE_MORE_INFO
Language        = English
SANCopy Disk Driver: Fatal error additional information: No path to LUN %2 (SP %3 port %4) (log 2 of 2). 
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_DEVICE_RECOVERY_IN_PROGRESS_MORE_INFO
Language        = English
SANCopy Disk Driver: Non-fatal error additional information LUN %2, (SP %3 port %4) (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SOFT_ERRS_ABOVE_THRESHOLD_MORE_INFO
Language        = English
SANCopy Disk Driver: %2 Non-fatal SCSI errors occurred since last report on LUN %3 (SP %4 port %5) (log 2 of 2).
.

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Informational
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LOCAL_PORT_ADDR_MORE_INFO
Language        = English
SANCopy Disk Driver: Searching via Local SP-%2 Port %3 (log 2 of 2).
.


;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x8000
Severity        = Error
Facility        = SanCopy
SymbolicName    = FEDISK_STATUS_RESERVATION_CONFLICT
Language        = English
Internal information only. Reservation conflict : device could not be accessed.
.

;
;#define FEDISK_ADMIN_STATUS_RESERVATION_CONFLICT \
;           MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_RESERVATION_CONFLICT)
;

;//-----------------------------------------------------------------------------
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_DEVICE_REMOVE_IN_PROGRESS
Language        = English
Internal information only. Device object remove in progress.
.
;
;#define FEDISK_ADMIN_STATUS_DEVICE_REMOVE_IN_PROGRESS \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_DEVICE_REMOVE_IN_PROGRESS)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 18
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_SC_LITE_TARGET_NOT_RUNNING_SANCOPY
Language        = English
Failed to find the source or target LU because the array on which it exists is not running SAN Copy.
.
;
;#define FEDISK_ADMIN_STATUS_SC_LITE_TARGET_NOT_RUNNING_SANCOPY\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_SC_LITE_TARGET_NOT_RUNNING_SANCOPY)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 18
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_PORT_LUN_ID_INVALID_FOR_EMC_ARRAY
Language        = English
SANCopy Disk Driver:  EMC CLARiiON and Symmetrix devices cannot be identified by their fibrechannel port world-wide name and LUN.  Please use system name and LUN or LUN WWN.
.
;
;#define FEDISK_ADMIN_STATUS_PORT_LUN_ID_INVALID_FOR_EMC_ARRAY \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_PORT_LUN_ID_INVALID_FOR_EMC_ARRAY)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 24
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Note: For communication between FEDisk and CPM
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TIMEOUT_EXPIRED
Language        = English
SANCopy Disk Driver: The search for (device:%2) timed out while in progress. Please retry this operation.
.
;
;#define FEDISK_ADMIN_STATUS_TIMEOUT_EXPIRED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TIMEOUT_EXPIRED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 24
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Note: For communication between FEDisk and CPM
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TIMEOUT_WAITING_ON_LOCK
Language        = English
SANCopy Disk Driver: The search for (device:%2) timed out while waiting on another operation. Please retry this operation after some time.
.
;
;#define FEDISK_ADMIN_STATUS_TIMEOUT_WAITING_ON_LOCK \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TIMEOUT_WAITING_ON_LOCK)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 24
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Note: For communication between FEDisk and CPM
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TIMEOUT_BELOW_MINIMUM
Language        = English
Internal information only. SANCopy Disk Driver: A timeout value below the minimum was provided.%2
.
;
;#define FEDISK_ADMIN_STATUS_TIMEOUT_BELOW_MINIMUM \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TIMEOUT_BELOW_MINIMUM)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 24
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Note: a negative value was passed to FEDisk in the timeout field.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TIMEOUT_INVALID
Language        = English
Internal information only. SANCopy Disk Driver: An invalid timeout value was provided. %2
.
;
;#define FEDISK_ADMIN_STATUS_TIMEOUT_INVALID \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TIMEOUT_INVALID)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_ALREADY_LOGGED_IN
Language        = English
A login session to the iSCSI target device is already active.
.
;
;#define FEDISK_ADMIN_STATUS_ALREADY_LOGGED_IN\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_ALREADY_LOGGED_IN)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_REMOTE_AUTHENTICATION_FAILED
Language        = English
Failed to login to the specified connection path; remote authentication failed. Please check the connection set.
.
;
;#define FEDISK_ADMIN_STATUS_REMOTE_AUTHENTICATION_FAILED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_REMOTE_AUTHENTICATION_FAILED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LOCAL_AUTHENTICATION_FAILED
Language        = English
Failed to login to the specified connection path; authentication failed. Please check the connection set.

.
;
;#define FEDISK_ADMIN_STATUS_LOCAL_AUTHENTICATION_FAILED\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_LOCAL_REMOTE_AUTHENTICATION_FAILED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Internal Use
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_NO_REMOTE_PASSWORD_FOUND
Language        = English
Internal Information Only. A connection to the iSCSI target device could not be established; no remote secret found.
.
;
;#define FEDISK_ADMIN_STATUS_NO_REMOTE_PASSWORD_FOUND\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_NO_REMOTE_PASSWORD_FOUND)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_NO_LOCAL_PASSWORD_FOUND
Language        = English
Failed to login to the specified connection path; no secret found. Please check the connection set.
.
;
;#define FEDISK_ADMIN_STATUS_NO_LOCAL_PASSWORD_FOUND\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_NO_LOCAL_PASSWORD_FOUND)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_ZONEDB_FAILURE
Language        = English
Failed to retrieve information from the connection set database. Please retry the operation.
.
;
;#define FEDISK_ADMIN_STATUS_ZONEDB_FAILURE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_ZONEDB_FAILURE)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TARGET_NOT_FOUND
Language        = English
Internal information only. No targets were found for the iSCSI session.
.
;
;#define FEDISK_ADMIN_STATUS_TARGET_NOT_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TARGET_NOT_FOUND)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_LOGIN_DEVICE_ERROR
Language        = English
Failed to login to the specified iSCSI path.
Please check the connectivity and configuration.
.
;
;#define FEDISK_ADMIN_STATUS_LOGIN_DEVICE_ERROR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_LOGIN_DEVICE_ERROR)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TARGET_INFO_ERROR
Language        = English
Internal information only.  Failed to get target information for an iSCSI device (%2).
.
;
;#define FEDISK_ADMIN_STATUS_TARGET_INFO_ERROR \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TARGET_INFO_ERROR)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_INVALID_PORT
Language        = English
Internal information only. An attempt was made to verify an connection path with an invalid port number. 
.
;
;#define FEDISK_ADMIN_STATUS_INVALID_PORT \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_INVALID_PORT)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_PORT_NOT_ISCSI
Language        = English
Internal information only. An attempt was made to verify an connection path on a non-iSCSI port.
.
;
;#define FEDISK_ADMIN_STATUS_PORT_NOT_ISCSI \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_PORT_NOT_ISCSI)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_PATH_NOT_FOUND
Language        = English
Internal information only. An attempt was made to verify an connection path that could not be found in the connection set database.
.
;
;#define FEDISK_ADMIN_STATUS_PATH_NOT_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_PATH_NOT_FOUND)
;

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_WRONG_SPID
Language        = English
Internal information only. An attempt was made to verify an connection path on the wrong SP.
.
;
;#define FEDISK_ADMIN_STATUS_WRONG_SPID \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_WRONG_SPID)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_PATH_TABLE_FULL
Language        = English
Too many Active iSCSI sessions to complete the operation.
Please retry the operation after an iSCSI session completes.
.
;
;#define FEDISK_ADMIN_STATUS_PATH_TABLE_FULL\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_PATH_TABLE_FULL)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Return to Navi
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_ISCSI_PORT_CONFIG
Language        = English
Failed to complete the operation.
Please check that the iSCSI port is properly configured.
.
;
;#define FEDISK_ADMIN_STATUS_ISCSI_PORT_CONFIG\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_ISCSI_PORT_CONFIG)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Note: FEDisk was unable to read through a SCSI reservation bypass.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_BYPASS_RESERVATION_FAILED
Language        = English
Internal information only. SANCopy Disk Driver: SCSI reservation bypass failed.
.
;
;#define FEDISK_ADMIN_STATUS_BYPASS_RESERVATION_FAILED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_BYPASS_RESERVATION_FAILED)
;

;//-----------------------------------------------------------------------------
;// Introduced in: Release 26
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Note: OutstandingRequestCount went negative
;//
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_NEGATIVE_ORC
Language        = English
Internal information only. SANCopy Disk Driver: OutstandingRequestCount went negative! %2 %3 %4 %5 %6 %7
.
;
;#define FEDISK_ADMIN_STATUS_NEGATIVE_ORC\
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_NEGATIVE_ORC)

;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 28
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_STATUS_TARGET_INFO_BUFFER_TOO_SMALL
Language        = English
Internal information only. Buffer to Target information too small, retrying  (%2).
.
;
;#define FEDISK_ADMIN_STATUS_TARGET_INFO_BUFFER_TOO_SMALL \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(FEDISK_STATUS_TARGET_INFO_BUFFER_TOO_SMALL)
;


;//-----------------------------------------------------------------------------
;//-----------------------------------------------------------------------------
;// Introduced in: Release 33
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
MessageId       = +1
Severity        = Error
Facility        = SANCopy
SymbolicName    = FEDISK_INVALID_SCSIPASSTHRU_DATA_LEN
Language        = English
Internal information only. FEDISK: Invalid data length(%2) provided for PASSTHRU.
.


;#endif
