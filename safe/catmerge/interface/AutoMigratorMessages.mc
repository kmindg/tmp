;//******************************************************************************
;//      COMPANY CONFIDENTIAL:
;//      Copyright (C) EMC Corporation, 2011
;//      All rights reserved.
;//      Licensed material - Property of EMC Corporation.
;//******************************************************************************
;
;//**********************************************************************
;//.++
;// FILE NAME:
;//      K10AutoMigratorMessages.mc
;//
;// DESCRIPTION:
;//      This file defines AutoMigrator's Status Codes and messages. 
;// 
;// REMARKS:
;//      This file is shared by the client interface, internal components, 
;//      Daemon, and CLI.
;//      
;//      Component          | Start of Range  | End of Range  | # of Messages
;//      -------------------+-----------------+---------------+--------------
;//      Client Interface   | 0x716Cx000      | 0x716Cx0FF    | 256 
;//      Migration Manager  | 0x716Cx100      | 0x716Cx1FF    | 256
;//      LUN Manager        | 0x716Cx200      | 0x716Cx2FF    | 256
;//      Lock Manager       | 0x716Cx300      | 0x716Cx3FF    | 256
;//      Database Manager   | 0x716Cx400      | 0x716Cx4FF    | 256
;//      Database Cache     | 0x716Cx500      | 0x716Cx5FF    | 256
;//      Job Manager        | 0x716Cx600      | 0x716Cx6FF    | 256
;//      Daemon             | 0x716Cx700      | 0x716Cx7FF    | 256
;//      Test Interface     | 0x716Cx800      | 0x716Cx8FF    | 256
;//      Misc               | 0x716Cx900      | 0x716Cx9FF    | 256       
;//
;//.--                                                                    
;//**********************************************************************
;#ifndef AUTO_MIGRATOR_MESSAGES_H
;#define AUTO_MIGRATOR_MESSAGES_H

MessageIdTypedef = HRESULT

FacilityNames   = ( System         = 0x0
                    RpcRuntime     = 0x2:FACILITY_RPC_RUNTIME
                    RpcStubs       = 0x3:FACILITY_RPC_STUBS
                    Io             = 0x4:FACILITY_IO_ERROR_CODE
                    AutoMigrator   = 0x16C:FACILITY_AUTO_MIGRATOR
                  )

SeverityNames   = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                    Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                    Warning       = 0x2 : STATUS_SEVERITY_WARNING
                    Error         = 0x3 : STATUS_SEVERITY_ERROR
                  )

;//-----------------------------------------------------------------------------
;// Client Interface Messages
;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = 0x0000
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_AUTOSTART_FAILED
Language        = English
Internal information only. AutoStart of the job failed with status.
.
;#define AM_AUTOSTART_FAILED \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_AUTOSTART_FAILED)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_AUTOSTART_FAILED_NDU_IN_PROGRESS
Language        = English
Internal information only. AutoStart of the job failed due to an NDU in progress.
.
;#define AM_AUTOSTART_FAILED_NDU_IN_PROGRESS \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_AUTOSTART_FAILED_NDU_IN_PROGRESS)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_AUTOSTART_FAILED_COMMIT_NEEDED
Language        = English
Internal information only. AutoStart of the job failed because a bundle commit is required.
.
;#define AM_AUTOSTART_FAILED_COMMIT_NEEDED \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_AUTOSTART_FAILED_COMMIT_NEEDED)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED
Language        = English
Internal information only. Creation of the destination LUN failed with status.
.
;#define AM_DEST_CREATE_FAILED \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_OUT_OF_SPACE
Language        = English
Internal information only. Creation of the destination LUN failed due to lack of free space.
.
;#define AM_DEST_CREATE_FAILED_OUT_OF_SPACE \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_OUT_OF_SPACE)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_INTERNAL_ERROR
Language        = English
Internal information only. An unexpected internal error occurred.
.
;#define AM_INTERNAL_ERROR \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_INTERNAL_ERROR)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_INVALID_OPERATION
Language        = English
Internal information only. The requested operation is invalid.
.
;#define AM_INVALID_OPERATION \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_INVALID_OPERATION)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_JOB_NOT_FOUND
Language        = English
Internal information only. The specified job could not be found.
.
;#define AM_JOB_NOT_FOUND \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_JOB_NOT_FOUND)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_LUN_IN_USE
Language        = English
Internal information only. The specified LUN is already in use.
.
;#define AM_LUN_IN_USE \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_LUN_IN_USE)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED
Language        = English
Internal information only. The requested operation failed.
.
;#define AM_OPERATION_FAILED \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED_COMMIT_NEEDED
Language        = English
Internal information only. The requested operation failed because a bundle commit is needed.
.
;#define AM_OPERATION_FAILED_COMMIT_NEEDED \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED_COMMIT_NEEDED)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED_NDU_IN_PROGRESS
Language        = English
Internal information only. The requested operation failed because an NDU is in progress.
.
;#define AM_OPERATION_FAILED_NDU_IN_PROGRESS \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED_NDU_IN_PROGRESS)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED_SYNCHRONIZED
Language        = English
Internal information only. The requested operation failed because the job is synchronized.
.
;#define AM_OPERATION_FAILED_SYNCHRONIZED \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED_SYNCHRONIZED)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_LOW_SPACE
Language        = English
Internal information only. Creation of the destination LUN failed because of the low space threshold.
.
;#define AM_DEST_CREATE_FAILED_LOW_SPACE\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_LOW_SPACE)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_HARVESTING
Language        = English
Internal information only. Creation of the destination LUN failed because of the auto-delete pool full threshold.
.
;#define AM_DEST_CREATE_FAILED_HARVESTING\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_HARVESTING)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_LOW_SPACE_HARVESTING
Language        = English
Internal information only. Creation of the destination LUN failed because of both the low space and the auto-delete thresholds.
.
;#define AM_DEST_CREATE_FAILED_LOW_SPACE_HARVESTING\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_LOW_SPACE_HARVESTING)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_MAX_LUNS
Language        = English
Internal information only. Creation of the destination LUN failed because the maximum LUNs have been created.
.
;#define AM_DEST_CREATE_FAILED_MAX_LUNS\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_MAX_LUNS)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_POOL_NOT_READY
Language        = English
Internal information only. Creation of the destination LUN failed because the storage pool is not ready.
.
;#define AM_DEST_CREATE_FAILED_POOL_NOT_READY\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_POOL_NOT_READY)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_COMMIT_NEEDED
Language        = English
Internal information only. Creation of the destination LUN failed because a bundle commit is required.
.
;#define AM_DEST_CREATE_FAILED_COMMIT_NEEDED\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_COMMIT_NEEDED)


;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED_SOURCE_NOT_READY
Language        = English
Internal information only. The requested operation failed because the source LUN was not ready.
.
;#define AM_OPERATION_FAILED_SOURCE_NOT_READY\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED_SOURCE_NOT_READY)


;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED_DEST_NOT_READY
Language        = English
Internal information only. The requested operation failed because the destination LUN was not ready.
.
;#define AM_OPERATION_FAILED_DEST_NOT_READY\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED_DEST_NOT_READY)


;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_AUTOSTART_FAILED_SOURCE_NOT_READY
Language        = English
Internal information only. AutoStart of the job failed because the source LUN was not ready.
.
;#define AM_AUTOSTART_FAILED_SOURCE_NOT_READY\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_AUTOSTART_FAILED_SOURCE_NOT_READY)


;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_AUTOSTART_FAILED_DEST_NOT_READY
Language        = English
Internal information only. AutoStart of the job failed because the destination LUN was not ready.
.
;#define AM_AUTOSTART_FAILED_DEST_NOT_READY\
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_AUTOSTART_FAILED_DEST_NOT_READY)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_AUTOSTART_FAILED_NICE_NAME_EXISTS
Language        = English
Internal information only. AutoStart of the job failed because the pool LUN exists with same nice name.
.
;#define AM_AUTOSTART_FAILED_NICE_NAME_EXISTS \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_AUTOSTART_FAILED_NICE_NAME_EXISTS)

;//-----------------------------------------------------------------------------
;// Introduced: Release 32
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_OPERATION_FAILED_NICE_NAME_EXISTS
Language        = English
Internal information only. The requested operation failed because the pool LUN exists with same nice name.
.
;#define AM_OPERATION_FAILED_NICE_NAME_EXISTS \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_OPERATION_FAILED_NICE_NAME_EXISTS)

;//-----------------------------------------------------------------------------
;// Introduced: Release 33
;// Usage:      Internal Use Only
;// Severity:   Error
;//
MessageId       = +1
Severity        = Error
Facility        = AutoMigrator
SymbolicName    = AM_KERNEL_DEST_CREATE_FAILED_INVALID_PREFERRED_PATH
Language        = English
Internal information only. Creation of the destination LUN failed due to invalid preferred path.
.
;#define AM_DEST_CREATE_FAILED_INVALID_PREFERRED_PATH \
;        MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(AM_KERNEL_DEST_CREATE_FAILED_INVALID_PREFERRED_PATH)

;#endif

