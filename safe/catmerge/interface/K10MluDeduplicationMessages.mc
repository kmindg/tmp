;//++
;// Copyright (C) EMC Corporation, 2009
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            K10MluDeduplicationMessages.mc
;//
;// Description:     This file contains the message catalogue for
;//                  the MLU Deduplication feature.
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
;// The MLU Deduplication feature further classifies its range by breaking up
;// each facility into sub-categories for each MLU Deduplication component.
;// The second nibble represents the component.  We are not using the entire 
;// range of the second nibble so we can expand if the future if necessary.
;//
;// +--------------------------+---------+-----+
;// | Component                |  Range  | Cnt |
;// +--------------------------+---------+-----+
;// | Deduplication Manager    | 0xk0mn  | 256 |
;// | Deduplication Server     | 0xk1mn  | 256 | Kernel Dedup Components
;// | Unused				   | 0xk2mn  | 256 |
;// | Unused			   	   | 0xk3mn  | 256 |
;// +--------------------------+---------+-----+
;// | Use-Space Dedup ComonentDeduplication Director   | 0xk4mn- |     | User-Space Dedup Components
;// |   Components as needed   | 0xkfmn  |     |   (Defined in director file)
;// +--------------------------+---------+-----+
;//
;// Where:
;// 'k' is the CLARiiON error code classification.  Note that we only use
;// the top two bits.  So there are two bits left for future expansion.
;//
;//  'mn' is the Manager specific error code.
;//
;//  *** NOTE : tabs in this file are set to every 4 spaces. ***
;//
;
;#ifndef __K10_MLU_DEDUPLICATION_MESSAGES__
;#define __K10_MLU_DEDUPLICATION_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System           = 0x0
                  RpcRuntime       = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs         = 0x3   : FACILITY_RPC_STUBS
                  Io               = 0x4   : FACILITY_IO_ERROR_CODE
                  MluDeduplication = 0x16B : FACILITY_MLUDEDUPLICATION
                )

SeverityNames = ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2 : STATUS_SEVERITY_WARNING
                  Error         = 0x3 : STATUS_SEVERITY_ERROR
                )


;//==============================================================
;//========== START DEDUPLICATION MANAGER SUCCESS MSGS ==========
;//==============================================================
;//
;// Deduplication Manager "Success" messages.
;//
MessageId    = 0x0000
Severity     = Success
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_STATUS_SUCCESS
Language     = English
All is well.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Success
;//
;// Symptom of Problem:
;//
;// Description: Digest iteration has reached the end of the domain 
;//
MessageId    = +1
Severity     = Success
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_SUCCESS_END_OF_ITERATION
Language     = English
Digest iteration reached the end of the domain
.

;//==============================================================
;//========== END DEDUPLICATION MANAGER SUCCESS MSGS ============
;//==============================================================
;//

;//==============================================================
;//======= START DEDUPLICATION MANAGER INFORMATION MSGS =======
;//==============================================================
;//
;// Deduplication Manager "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code only
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//
;//MessageId    = +1
;//Severity     = Informational
;//Facility     = MluDeduplication
;//SymbolicName = DEDUPLICATION_MGR_INFO_TBD1
;//Language     = English
;//Internal Information Only.
;//.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//
;//MessageId    = +1
;//Severity     = Informational
;//Facility     = MluDeduplication
;//SymbolicName = DEDUPLICATION_MGR_INFO_TBD2
;//Language     = English
;//Internal information only.
;//.

;//==============================================================
;//======= END DEDUPLICATION MANAGER INFORMATION MSGS =========
;//==============================================================
;//

;//==============================================================
;//======= START DEDUPLICATION SERVER INFORMATION MSGS ============
;//==============================================================
;//
;// Deduplication Server "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
;// Description: TBD 
;//
;//MessageId    = 0x0100
;//Severity     = Informational
;//Facility     = MluDeduplication
;//SymbolicName = DEDUPLICATION_SERVER_INFO_TBD1
;//Language     = English
;//TBD description
;//.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: 
;//
;// Description:
;//
;//MessageId    = +1
;//Severity     = Informational
;//Facility     = MluDeduplication
;//SymbolicName = DEDUPLICATION_SERVER_INFO_TBD2
;//Language     = English
;//Internal information only.
;//.

;//==============================================================
;//======= END DEDUPLICATION SERVER INFORMATION MSGS ============
;//==============================================================
;//


;//==============================================================
;//========= START DEDUPLICATION MANAGER WARNING MSGS ===========
;//==============================================================
;//
;// Deduplication Manager "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Returned to Director (As Reason Code).
;//
;// Severity: Warning
;//
;// Symptom of Problem: 
;//
;// Description:
;//     
MessageId    = 0x4000
Severity     = Warning
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_MANAGER_WARNING_DOMAIN_OFFLINE
Language     = English
The deduplication domain is currently OFFLINE.
.

;//==============================================================
;//========= END DEDUPLICATION MANAGER WARNING MSGS =============
;//==============================================================
;//


;//==============================================================
;//========= START DEDUPLICATION SERVER WARNING MSGS ============
;//==============================================================
;//

;//==============================================================
;//========== END DEDUPLICATION SERVER WARNING MSGS =============
;//==============================================================
;//


;//==============================================================
;//========= START DEDUPLICATION MANAGER ERROR MSGS =============
;//==============================================================
;//
;// Deduplication Manager "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description: Close was issued for active session.
;//
MessageId    = 0x8000
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_MANAGER_ERROR_SESSION_STILL_ACTIVE
Language     = English
There is an existing deduplication session for the Deduplication Domain which the client is attempting to open a session for.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description: Open session request failed and session was closed.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_MANAGER_ERROR_EMPTY_SESSION_CLOSED
Language     = English
The deduplication client attempted to open a session for a domain and it was not able to add any VUs to the session.  The empty session was closed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description: Invalid Domain specified.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_MANAGER_ERROR_INVALID_DOMAIN
Language     = English
The deduplication client specified an invalid domain.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:  Internal Error
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_MANAGER_ERROR_INTERNAL_ERROR
Language     = English
The deduplication manager encountered an internal error.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Error Code Only
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description:  Domain is not in a releasable state
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_MANAGER_ERROR_UNRELEASABLE_DOMAIN
Language     = English
The deduplication manager cannot release the specified domain.
.

;//==============================================================
;//========= END DEDUPLICATION MANAGER ERROR MSGS ===============
;//==============================================================
;//


;//==============================================================
;//========= START DEDUPLICATION SERVER ERROR MSGS ==============
;//==============================================================
;//
;// Deduplication Server "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description:
;//
MessageId    = 0x8100
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_SESSION_STILL_ACTIVE
Language     = English
The deduplication server was asked to close a deduplication session that is still active.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Invalid input parameter
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_INVALID_PARAMETER
Language     = English
There is an invalid input parameter in the deduplication request
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Invalid chunk ID parameter
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_INVALID_CHUNKID
Language     = English
The input chunk Id is invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: unsupported functionality
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_NOT_SUPPORTED
Language     = English
unsupported functionality
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: supported but not implemented functionality.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_NOT_IMPLEMENTED
Language     = English
functionality is supported, but not implemented yet.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: deduplicaton server resources manager deson't have enough resources
;// to handle the request
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_INSUFFICIENT_RESOURCES
Language     = English
deduplicaton server resources manager deson't have enough resources
to handle the request
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: an internal error happened in the deduplication server
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_INTERNAL_ERROR
Language     = English
an internal error happened in the deduplication server
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: 
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_COUNT_EXCEED_LIMIT
Language     = English
count exceeds the internal limit
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: two chunks that are to be deduped are not idential. 
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_NOT_IDENTICAL_CHUNKS
Language     = English
two chunks are not idential.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: deduplication request has been stopped in the deduplication server 
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_REQUEST_STOPPED
Language     = English
deduplication request has been stopped
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: deduplication request has been paused in the deduplication server 
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_REQUEST_PAUSED
Language     = English
deduplication request has been paused
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: deduplication request has been resumed in the deduplication server 
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_REQUEST_RESUMED
Language     = English
deduplication request has been resumed
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT Application Log
;//
;// Severity: Error
;//
;// Symptom of Problem: 
;//
;// Description: Cannot find DDS VU configuration
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_VU_CONFIG_NOT_FOUND
Language     = English
Cannot find the deduplication server VU configuration record. 
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: DDS VU configure record is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_BAD_VU_CONFIG
Language     = English
Invalid deduplication server VU configuration record.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: DDS VU configuration has unexpected IO flag.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_VU_UNEXPECTED_IOFLAG
Language     = English
Deduplication server VU configuration record has unexpected IO flag.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: DDS domain is in unexpected state
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_DOMAIN_UNEXPECTED_STATE
Language     = English
Deduplication server domain record has unexpected state.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: DDS domain still have VU quiesced.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_DOMAIN_HAS_VU_QUIESCED
Language     = English
Deduplication server domain has quiesced VU.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: DDS request is invalid.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_INVALID_REQUEST
Language     = English
Deduplication server request is invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Deduplication work item is invalid
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_BAD_WORK_ITEM
Language     = English
Deduplication work item is invalid.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Deduplication session is in stopped state
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_SESSION_STATE_STOPPED
Language     = English
Deduplication session is stopped.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Deduplication session is in invalid state
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_SESSION_STATE_INVALID
Language     = English
Deduplication session is in invalid state.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Deduplication session is in init state
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_SESSION_STATE_INIT
Language     = English
Deduplication session is in init state.
.


;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Deduplication work item is paused.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_WORK_ITEM_PAUSED
Language     = English
Deduplication work item is paused.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Deduplication work item cannot continue.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_WORK_ITEM_CANNOT_CONTINUE
Language     = English
Deduplication work item cannot continue.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: Reached maximum deduplication request allowed per dedup domain .
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_REACH_MAX_REQUEST_PER_DEDUP_DOMAIN
Language     = English
Reached maximum deduplication request allowed per dedup domain .
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Windows NT System Log
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
;// Description: The deduplication server was asked to close a non-existent session.
;//
MessageId    = +1
Severity     = Error
Facility     = MluDeduplication
SymbolicName = DEDUPLICATION_SERVER_ERROR_INVALID_SESSION
Language     = English
The deduplication server was asked to close a non-existent session.
.

;//==============================================================
;//========== END DEDUPLICATION SERVER ERROR MSGS ===============
;//==============================================================
;//


;//==============================================================
;//========= START DEDUPLICATION MANAGER CRITICAL MSGS ==========
;//==============================================================
;//
;// Deduplication Manager "Critical" messages.
;//

;//==============================================================
;//========= END DEDUPLICATION MANAGER CRITICAL MSGS ============
;//==============================================================
;//



;//==============================================================
;//========= START DEDUPLICATION SERVER CRITICAL MSGS ===========
;//==============================================================
;//
;// Deduplication Server "Critical" messages.
;//

;//==============================================================
;//========= END DEDUPLICATION SERVER CRITICAL MSGS =============
;//==============================================================
;//

;#endif // __K10_MLU_DEDUPLICATION_MESSAGES__
