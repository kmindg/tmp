;//++
;// Copyright (C) EMC Corporation, 2007-2014
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            K10MluReplicationMessages.mc
;//
;// Description:     This file contains the message catalogue for Replication messages from LO.
;//
;// History:
;//
;//--
;//
;// The standard for how to classify errors according to their range is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;
;#ifndef __K10_MLU_REPLICATION_MESSAGES__
;#define __K10_MLU_REPLICATION_MESSAGES__
;
;#if defined(CSX_BV_LOCATION_KERNEL_SPACE) && defined(CSX_BV_ENVIRONMENT_WINDOWS)
;#define MLU_MSG_ID_ASSERT(_expr_) C_ASSERT((_expr_));  
;#else
;#define MLU_MSG_ID_ASSERT(_expr_)
;#endif 


MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_CODE
                  MluReplication= 0x172 : FACILITY_MLU_REPLICATION
                )

SeverityNames = ( Success       = 0x0   : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1   : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2   : STATUS_SEVERITY_WARNING
                  Error         = 0x3   : STATUS_SEVERITY_ERROR
                )
;
;
;
;
;//========================
;//======= INFO MSGS ======
;//========================
;
;
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Template
;//
;// Severity: Informational
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0x41720000
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_INFO_TEMPLATE
Language     = English
Only for Info Template
.
;
;
;
;
;//========================
;//======= WARNING MSGS ======
;//===========================
;
;
;


;// --------------------------------------------------
;// Introduced In: R__
;//
;// Usage: Template
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0x81724000
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_WARNING_TEMPLATE
Language     = English
Only for warning template
.
;
;
;
;
;//========================
;//======= ERROR MSGS ======
;//=========================
;
;
;


;// --------------------------------------------------
;// Introduced In: R36                                    
;//
;// Usage: Template
;//
;// Severity: Error
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0xC1728000
;//
MessageId    = 0x8000
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_ERROR_TEMPLATE
Language     = English
Only for error template
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Destroy is not allowed since FS is in use by a replication session.
;//
MessageId    = 0x8001
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_DESTROY_ERROR_FS_IN_USE_BY_REPLICATION
Language     = English
Cannot destroy file system while it is in use by replication.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Converting FS type is not allowed since FS is in use by a replication session.
;//
MessageId    = 0x8002
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_CONVERT_FSTYPE_ERROR_FS_IN_USE_BY_REPLICATION
Language     = English
Cannot convert file system type while it is in use by replication.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Expand or Shrink operations are not allowered since FS is in use by a replication session.
;//
MessageId    = 0x8003
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_EXPAND_OR_SHRINK_ERROR_FS_IN_USE_BY_REPLICATION
Language     = English
Cannot expand or shrink file system since it is functioning as secondary in a replication session.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Restoring FS is not allowed since FS is in use by a replication session.
;//
MessageId    = 0x8004
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_RESTORE_ERROR_FS_IN_USE_BY_REPLICATION
Language     = English
Cannot restore file system since it is functioning as secondary in a replication session.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Restoring FS type is not allowed since FS is being used as replication destination.
;//
MessageId    = 0x8005
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_RESTORE_ERROR_FS_IS_REPL_DESTINATION
Language     = English
Cannot restore file system since it is being used as replication destination.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovering FS is not allowed since FS is being used as replication destination.
;//
MessageId    = 0x8006
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_RECOVER_ERROR_FS_IS_REPL_DESTINATION
Language     = English
Cannot perform file system recovery since it is being used as replication destination.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Recovering FS is not allowed since FS is in use by a replication session.
;//
MessageId    = 0x8007
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_RECOVER_ERROR_FS_IN_USE_BY_REPLICATION
Language     = English
Cannot perform file system recovery since it is in use by replication.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Modifying FS is not allowed since FS is in use by a replication session.
;//
MessageId    = 0x8008
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_MODIFY_ERROR_FS_IS_REPL_DESTINATION
Language     = English
Cannot modify file system property since it is already set as replication destination.
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Modifying FS is not allowed since FS is in use by a replication session.
;//
MessageId    = 0x8009
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_MODIFY_ERROR_FS_IN_USE_BY_REPLICATION
Language     = English
Cannot modify file system property since the file system is in use by replication.
.


;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Replication session failed because volume is shrinking.
;//
MessageId    = 0x800a
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_VOLUME_IS_BEING_SHRUNK
Language     = English
The replication session could not be created because the volume to be replicated is shrinking. Wait for the size change to complete and retry the operation.
.

;MLU_MSG_ID_ASSERT((NTSTATUS)0xE172800aL == MLU_REPL_MGR_VOLUME_IS_BEING_SHRUNK);
;

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the number of synchronous replication objects.  Can not create the sync replication session.
;//
MessageId    = 0x800b
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_SYNC_OBJECT_MAX
Language     = English
The replication session could not be created because the max number of synchronous replication objects has been reached .
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the number of asynchronous replication sessions.  Can not create the async replication session.
;//
MessageId    = 0x800c
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_ASYNC_OBJECT_MAX
Language     = English
The replication session could not be created because the max number of asynchronous replication objects has been reached .
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the number of replication sessions for the platform. Can not create the replication session.
;//
MessageId    = 0x800d
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_SESSION_MAX
Language     = English
The replication session could not be created because the max number of replication sessions for the system has been reached .
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the max number of replication sessions for a single resource. Can not create the replication session.
;//
MessageId    = 0x800e
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_SESSION_PER_RESOURCE_MAX
Language     = English
The replication session could not be created because the max number of replication sessions for the resource has been reached .
.

;// --------------------------------------------------
;// Introduced In: R36
;//
;// Usage: Returned to Unisphere
;//
;// Severity: Error
;//
;// Symptom of Problem: Exceeded the number of synchronous Consistency Group replication sessions.  Can not create the sync replication session.
;//
MessageId    = 0x800f
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_SYNC_CG_SESSION_MAX
Language     = English
The replication session could not be created because the max number of synchronous consistency group replication sessions has been reached.
.

;
;
;
;//============================
;//======= CRITICAL MSGS ======
;//============================
;
;
;

;// --------------------------------------------------
;// Introduced In: R34
;//
;// Usage: Template
;//
;// Severity: Error
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0xc172c000
;//
MessageId    = 0xc000
Severity     = Error
Facility     = MluReplication
SymbolicName = MLU_REPL_MGR_CRITICAL_TEMPLATE
Language     = English
Only for critical template
.



;#endif //__K10_MLU_REPLICATION_MESSAGES__
