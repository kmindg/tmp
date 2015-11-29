;//++
;// Copyright (C) EMC Corporation, 2009
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            K10PolicyEngineAutoTieringMessages.mc
;//
;// Description:     This file contains the message catalog for
;//                  the Polcy Engine portion of Auto Tiering functionality.
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
;// The AT feature further classifies its range by breaking up each CLARiiON
;// category into a sub-category.  The
;// second nibble represents the sub-category.  We are not using
;// the entire range of the second nibble so we can expand if the future if
;// necessary.  Each component below is represented in a different message
;// catalog file and associated .dll (actually, the MLUDriver and Tier Admin
;// components share a single .mc and .dll).
;//
;// +--------------------------+---------+-----+
;// | Component                |  Range  | Cnt |
;// +--------------------------+---------+-----+
;// | MLUDriver                |  0xk0mn | 512 |
;// | Tier Admin               |  0xk2mn | 512 |
;// | Policy Engine            |  0xk4mn | 512 |
;// | Tier Definition Library  |  0xk6mn | 512 |
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
;#ifndef __K10_PE_AUTO_TIERING_MESSAGES__
;#define __K10_PE_AUTO_TIERING_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_ERROR_CODE
                  AutoTiering   = 0x166 : FACILITY_AUTO_TIERING
                )

SeverityNames = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2 : STATUS_SEVERITY_WARNING
                  Error         = 0x3 : STATUS_SEVERITY_ERROR
                )


MessageId    = 0x0400
Severity     = Success
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_STATUS_SUCCESS
Language     = English
The operation completed successfully.
.

;//========================
;//======= INFO MSGS ======
;//========================
;//
;//

;//============================================
;//======= START Policy Engine INFO MSGS ======
;//============================================
;//
;// Policy Engine "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: 30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Policy Engine Relocation Summary
;//
MessageId    = 0x401
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_INFO_RELOCATION_SUMMARY
Language     = English
Relocation completed for Storage Pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: Policy Engine Relocation Stops successfully.
;//
MessageId    = +1
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_INFO_RELOCATION_STOP_OK
Language     = English
Relocation is stopped for Storage Pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: Policy Engine Relocation starts successfully.
;//
MessageId    = +1
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_INFO_RELOCATION_START_OK
Language     = English
Relocation is started for Storage Pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: Policy Engine Relocation is paused.
;//
MessageId    = +1
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_INFO_RELOCATION_PAUSED
Language     = English
Relocation is paused for Storage Pool %1.
.

;// Severity: Informational
;//
;// Symptom of Problem: Policy Engine Relocation is resumed.
;//
MessageId    = +1
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_INFO_RELOCATION_RESUMED
Language     = English
Relocation is resumed for Storage Pool %1.
.

;//============================================
;//======= END Policy Engine INFO MSGS ========
;//============================================
;//


;//===============================================
;//======= START Policy Engine WARNING MSGS ======
;//===============================================
;//
;// Policy Engine "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: 30
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Policy Engine Warning Storage
;//
MessageId    = 0x4400
Severity     = Warning
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_WARNING_STORAGE
Language     = English
Unable to access Auto-Tiering service persistent storage for Storage Pool %1.
.


;//
;// Severity: Warning
;//
;// Symptom of Problem: Policy Engine Warning Storage Registry
;//
MessageId    = +1
Severity     = Warning
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_WARNING_REGISTRY
Language     = English
Unable to access registry key %1 for Auto-Tiering service.
.


;//===============================================
;//======= END Policy Engine WARNING MSGS ========
;//===============================================
;//


;//=============================================
;//======= START Policy Engine ERROR MSGS ======
;//=============================================
;//
;// Policy Engine "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: 30
;//
;// Usage: For Event Log and for Navi use
;//


;// For Navi error
;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error version mismatch
;//
MessageId    = 0x8400
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_VERSION_MISMATCH
Language     = English
Version mismatch. FLARE-Operating-Environment must be committed to complete this operation.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error feature is not enabled.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_NOT_ENABLED
Language     = English
Auto-Tiering feature is not enabled. Install Auto-Tiering Enabler.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error relocation is running.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_RELOCATION_RUNNING
Language     = English
A relocation task is already active for the storage pool. Retry after the current task is complete.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error invalid argument.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_INVALID_ARG
Language     = English
Invalid parameter(s) for function.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error Policy engine is degraded.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_DEGRADED
Language     = English
Auto-Tiering service is not accessible at the moment, retry later.
.

;// For event Log errors
;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error configuration discovery
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_CONFIG_DISCOVERY
Language     = English
An internal error occurred. Unable to do configuration pool discovery for Auto-Tiering service - %1. Check the status of the Storage Pool(s).
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error statistics collection 
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_STATS_COLLECTION
Language     = English
An internal error occurred. Statistics collection failed for Storage Pool %1. Check the health of peer SP.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error exception
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_EXCEPTION
Language     = English
An internal error occurred in Auto-Tiering service. %1
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine Error task failure
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_TASK_FAILURE
Language     = English
Task execution failure in Auto-Tiering service. Retry the task.
.

;//
;// Severity: Error
;//
;// Symptom of Problem: Policy Engine does not have enough informaion to perform the operation
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_PE_ERROR_POOL_NOT_ENOUGH_DATA_FOR_OPERATION
Language     = English
Auto-Tiering service does not have sufficient usage statistics for the storage pool. The storage pool is either added or deleted recently. Wait an hour and retry.
.
;//=============================================
;//======= END Policy Engine ERROR MSGS ========
;//=============================================
;//


;#endif //__K10_PE_AUTO_TIERING_MESSAGES__
