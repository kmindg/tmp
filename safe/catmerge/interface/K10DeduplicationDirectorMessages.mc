;//++
;// Copyright (C) EMC Corporation, 2009
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            K10DeduplicationDirectorMessages.mc
;//
;// Description:     This file contains the message catalog for
;//                  the Deduplication Director portion of Deduplication functionality.
;//
;// History:
;//
;//--
;//
;// The CLARiiON standard for how to classify errors according to their range
;// is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;// The Deduplication feature further classifies its range by breaking up each CLARiiON
;// category into a sub-category.  The
;// second nibble represents the sub-category.  We are not using
;// the entire range of the second nibble so we can expand if the future if
;// necessary.  Each component below is represented in a different message
;// catalog file and associated .dll 
;//
;// +--------------------------+---------+-----+
;// | Component                |  Range  | Cnt |
;// +--------------------------+---------+-----+
;// | Deduplication Manager    |  0xk0mn | 256 |
;// | Deduplication Server     |  0xk1mn | 256 |
;// | Deduplication Director   |  0xk4mn | 256 |
;// | Dedup Admin              |  0xk5mn | 256 |
;// +--------------------------+---------+-----+
;// | Unused                   |  0xk6mn |     |
;// | Unused                   |  0xk7mn |     |
;// | Unused                   |  0xk8mn |     |
;// | Unused                   |  0xk9mn |     |
;// | Unused                   |  0xkamn |     |
;// | Unused                   |  0xkbmn |     |
;// | Unused                   |  0xkcmn |     |
;// | Unused                   |  0xkdmn |     |
;// | Unused                   |  0xkemn |     |
;// | Unused                   |  0xkfmn |     |
;// +--------------------------+---------+-----+
;//
;// Where:
;// 'k' is the CLARiiON error code classification.  Note that we only use
;// the top two bits.  So there are two bits left for future expansion.
;//
;//  'mn' is the component specific error code.
;//
;//  *** NOTE : tabs in this file are set to every 4 spaces. ***
;//
;
;#ifndef __K10_DEDUPLICATION_DIRECTOR_MESSAGES__
;#define __K10_DEDUPLICATION_DIRECTOR_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        	= 0x0
                  RpcRuntime    	= 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      	= 0x3   : FACILITY_RPC_STUBS
                  Io            	= 0x4   : FACILITY_IO_ERROR_CODE
                  Deduplication 	= 0x16A : FACILITY_DEDUPLICATION
                )

SeverityNames = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2 : STATUS_SEVERITY_WARNING
                  Error         = 0x3 : STATUS_SEVERITY_ERROR
                )


MessageId    = 0x0000
Severity     = Success
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_STATUS_SUCCESS
Language     = English
The operation completed successfully.
.

;//========================
;//======= INFO MSGS ======
;//========================
;//
;//

;//============================================
;//======= START DEDUPLICATION DIRECTOR INFO MSGS ======
;//============================================
;//
;// Director "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: 32
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Deduplication Director Summary
;//
MessageId    = 0x400
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_INFO_SUMMARY
Language     = English
Deduplication completed for Storage Pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup has been enabled for a LUN
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_DEDUP_ENABLED
Language     = English
Deduplication has been enabled on LUN %1, %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup has been disabled for a LUN
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_DEDUP_DISABLED
Language     = English
Deduplication has been disabled on LUN %1, %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup on LUN migration job is awaiting to start
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_ENABLE_MIGRATION_TO_START
Language     = English
LUN %1 is awaiting auto migration into deduplication container %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup on LUN migration job has started
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_ENABLE_MIGRATION_STARTED
Language     = English
LUN %1 is auto migrating into deduplication container  %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup on LUN migration job has completed.
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_ENABLE_MIGRATION_COMPLETE
Language     = English
LUN %1 has completed auto migrating into deduplication container  %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup off LUN migration job is awaiting to start
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_DISABLE_MIGRATION_TO_START
Language     = English
LUN %1 is awaiting auto migration out of deduplication container %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup on LUN migration job has started
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_DISABLE_MIGRATION_STARTED
Language     = English
LUN %1 is auto migrating out of deduplication container  %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: Dedup on LUN migration job has completed
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_DME_DISABLE_MIGRATION_COMPLETE
Language     = English
LUN %1 has completed auto migrating out of deduplication container  %2.
.

;// Severity: Informational
;//
;// Symptom of Problem: User has paused deduplication for a pool
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_POOL_PAUSED
Language     = English
An administrator has paused deduplication for pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: User has resumed deduplication for a pool
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_POOL_RESUMED
Language     = English
An administrator has resumed deduplication for pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: User has select operation is successful with delay in execution 
;//
MessageId    = +1
Severity     = Informational
Facility     = Deduplication 
SymbolicName = DEDUPLICATION_DIRECTOR_SUCCESS_WITH_DELAY
Language     = English
User operation is successful with delay in execution.
.

;//============================================
;//======= END DEDUPLICATION DIRECTOR INFO MSGS ========
;//============================================
;//


;//===============================================
;//======= START DEDUPLICATION DIRECTOR WARNING MSGS ======
;//===============================================
;//
;// Director"Warning" messages.
;//

;// Severity: Warning
;//
;// Symptom of Problem: Deduplication migration pool full (out of space)
;//
MessageId    = 0x4400 
Severity     = Warning
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_WARNING_DEDUPLICATION_POOL_FULL
Language     = English
Not enough free space in storage pool to complete enabling or disabling deduplication. Please add disks to storage pool %1.
.


;// Usage: Event Log 
;//
;// Severity: Warning
;//
;// Symptom of Problem: Deduplication (disable dedup) not enough free space to start migration
;//
MessageId    = +1
Severity     = Warning
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_WARNING_ENABLE_DD_NOT_ENOUGH_FREE_SPACE
Language     = English
Not enough free space in storage pool to enable deduplication. Please add disks to storage pool %1.
.


;// Usage: Event Log 
;//
;// Severity: Warning
;//
;// Symptom of Problem: Deduplication (disable dedup) not enough free space to start migration
;//
MessageId    = +1
Severity     = Warning
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_WARNING_DISABLE_DD_NOT_ENOUGH_FREE_SPACE
Language     = English
Not enough free space in storage pool to disable deduplication. Please add disks to storage pool %1.
.

;//
;// Severity: Warning
;//
;// Symptom of Problem: DirectorWarning Storage Registry - used as return
;//
MessageId    = +1
Severity     = Warning
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_WARNING_REGISTRY
Language     = English
Unable to access registry key %1 for deduplication director service.
.

;//
;// Severity: Warning
;//
;// Symptom of Problem: DirectorWarning Storage Registry - used as return
;//
MessageId    = +1
Severity     = Warning
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_DME_MIGRATION_PAUSED
Language     = English
The System has paused enabling/disabling deduplication for LUN %1, %2.
.

;//
;// Severity: Warning
;//
;// Symptom of Problem: DirectorWarning Storage Registry - used as return
;//
MessageId    = +1
Severity     = Warning
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_DME_MIGRATION_RESUMED
Language     = English
The System has resumed enabling/disabling deduplication for LUN %1, %2.
.



;//===============================================
;//======= END DEDUPLICATION DIRECTOR WARNING MSGS ========
;//===============================================
;//


;//=============================================
;//======= START DEDUPLICATION DIRECTOR ERROR MSGS ======
;//=============================================
;//
;// DEDUPLICATION DIRECTOR "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: 32
;//
;// Usage: For Event Log and for Navi use
;//


;// For Navi error
;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError version mismatch
;//
MessageId    = 0x8400
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_VERSION_MISMATCH
Language     = English
Version mismatch. FLARE-Operating-Environment must be committed to complete this operation.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError feature is not enabled.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_NOT_ENABLED
Language     = English
Deduplication Director feature is not enabled. Install Deduplication Enabler.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError feature is not committed.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_UNCOMMITTED
Language     = English
Deduplication Director feature is not committed. please commit the package.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError invalid argument.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_INVALID_ARG
Language     = English
Invalid parameter(s) for function.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError exception
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_EXCEPTION
Language     = English
An internal error occurred in Deduplication Director service. %1
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Director degraded
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_DEGRADED
Language     = English
Deduplication Director service is degraded. Please check if Deduplication Director service is running.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError task failure
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_TASK_FAILURE
Language     = English
Task execution failure in Deduplication Director service. Retry the task.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError Psm Write error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_INCORRECT_NUMBER_BYTES_WRITTEN_TO_PSM
Language     = English
Deduplication Director incorrect number of bytes written to PSM.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError Psm OpenWrite error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_OPENWRITE_PSM
Language     = English
Deduplication Director error while opening PSM for write.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError Psm OpenRead error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_OPENREAD_PSM
Language     = English
Deduplication Director error while opening PSM for read.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError FSL Storage create error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_FSL_STORAGE_CREATE
Language     = English
Deduplication Service could not create FSL Storage for metadata. Pool ID %1
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError FSL Storage delete error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_FSL_STORAGE_DELETE
Language     = English
Deduplication Service could not delete FSL Storage for metadata. Pool ID %1
.

;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError FSL Storage use error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_FSL_STORAGE_USE
Language     = English
Deduplication Service could not use FSL Storage for metadata. Pool ID %1
.
    
;//
;// Severity: Error
;//
;// Symptom of Problem: DirectorError FSL Storage unuse error.
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_FSL_STORAGE_UNUSE
Language     = English
Deduplication Service could not stop using FSL Storage for metadata. Pool ID %1
.

;// For Navi error
;//
;// Severity: Error
;//
;// Symptom of Problem: Error setting deduplication flag failed
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_DME_NOT_ENOUGH_FREE_SPACE
Language     = English
Not enough free space in storage pool %1 to enable/disable deduplication. Please extend the pool.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication flag is already enabled
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_DME_LUN_DEDUP_ALREADY_ENABLED
Language     = English
Dedup is already enabled for the lun.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: DDeduplication flag is already disabled
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_DME_LUN_DEDUP_ALREADY_DISABLED
Language     = English
Dedup is already disabled for the lun.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication storage Error
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_STORAGE
Language     = English
Dedup is having storage errors. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication meta storage Error
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_META_STORAGE
Language     = English
Dedup is having meta storage errors. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication environment Error
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_ENVIRONMENT
Language     = English
Dedup is having environment errors. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication generic Error
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_GENERIC
Language     = English
Dedup is having generic errors. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error in setting up meta storage for dedup job
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_META_STORAGE_SETUP_FAILURE
Language     = English
Deduplication error in setting up meta storage for dedup job. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error in shutdown meta storage for dedup job
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_META_STORAGE_SHUTDOWN_FAILURE
Language     = English
Deduplication error in shutdown meta storage for dedup job. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error in deleting meta storage for dedup job
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_META_STORAGE_DELETE_FAILURE
Language     = English
Deduplication error in deleting meta storage for dedup job. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error from config
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_CONFIG
Language     = English
Deduplication error from config. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error from Kernel
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_DIRECTOR_ERROR_KERNEL
Language     = English
Deduplication error from Kernel. If problem persist please gather SP Collects and contact your service provider.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error from Kernel
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_AUTOMIGRATION_INTERNAL_ERROR
Language     = English
Enable/disable deduplication on LUN %1 encoutered internal error.    
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error from Kernel
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_AUTOMIGRATION_LUN_NOT_READY
Language     = English
Cannot enable/disable deduplication on the LUN %1 which is not ready.
.

;// For Navi error
;// Severity: Error
;//
;// Symptom of Problem: Deduplication error from Kernel
;//
MessageId    = +1
Severity     = Error
Facility     = Deduplication
SymbolicName = DEDUPLICATION_AUTOMIGRATION_POOL_NOT_READY
Language     = English
Cannot enable/disable deduplication on the LUN from the pool %1 which is not ready.
.


;//=============================================
;//======= END DirectorERROR MSGS ========
;//=============================================
;//

;//==============================================
;//======== Deduplication =======================
;//==============================================

;#endif //__K10_DEDUPLICATION_DIRECTOR_MESSAGES__
