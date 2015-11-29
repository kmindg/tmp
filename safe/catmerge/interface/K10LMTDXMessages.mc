;//++
;// Copyright (C) EMC Corporation, 2007-2015
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            K10LMTDXMessages.mc
;//
;// Description:     This file contains the message catalogue for LM TDX messages.
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
;#ifndef __K10_LM_TDX_MESSAGES__
;#define __K10_LM_TDX_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_CODE
                  LMTDX         = 0x173 : FACILITY_LMTDX
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
;// Introduced In: PSI7
;//
;// Usage: Template
;//
;// Severity: Informational
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0x41730000
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_INFO_TEMPLATE_NOT_USED
Language     = English
Only for Info Template
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: Driver init started
;//
;// Generated value should be: 0x41730001
;//
MessageId    = 0x0001
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_INFO_DRIVER_INIT_STARTED_NOT_USED
Language     = English
TDX driver initialization started.
Compiled: %2
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Received request to create a full copy session 
;//
;// Generated value should be: 0x41730002
;//
MessageId    = 0x0002
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_CREATE_FULL_COPY_RECEIVED_NOT_USED
Language     = English
Received request to create a Full Copy Session. Src %2 Dest %3 Len %4
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: A full copy session was successfully created
;//
;// Generated value should be: 0x41730003
;//
MessageId    = 0x0003
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_CREATE_FULL_COPY_COMPLETED_NOT_USED
Language     = English
Successfully created Full Copy Session %2. Src %3 Dest %4 Len %5.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Received request to create a diff copy session
;//
;// Generated value should be: 0x41730004
;//
MessageId    = 0x0004
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_CREATE_DIFF_COPY_RECEIVED_NOT_USED
Language     = English
Received request to create a Diff Copy Session. Src %2 Dest %3 Comp %4 Len %5
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Copy session created
;//
;// Generated value should be: 0x41730005
;//
MessageId    = 0x0005
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_CREATE_DIFF_COPY_COMPLETED_NOT_USED
Language     = English
Successfully created Diff Copy Session %2. Src %3 Dest %4 Comp %5 Len %6.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Received request to start a copy session
;//
;// Generated value should be: 0x41730006
;//
MessageId    = 0x0006
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_START_SESSION_RECEIVED_NOT_USED
Language     = English
Starting Copy Session %2.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Copy session started successfully
;//
;// Generated value should be: 0x41730007
;//
MessageId    = 0x0007
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_START_SESSION_COMPLETED_NOT_USED
Language     = English
Copy Session %2 started successfully.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Received request to finalize session
;//
;// Generated value should be: 0x41730008
;//
MessageId    = 0x0008
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_FINALIZE_SESSION_RECEIVED_NOT_USED
Language     = English
Received request to Finalize Copy Session %2.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Finalize session completed successfully.
;//
;// Generated value should be: 0x41730009
;//
MessageId    = 0x0009
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_FINALIZE_SESSION_COMPLETED_NOT_USED

Language     = English
Finalize Copy Session %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Received request to destroy copy session.
;//
;// Generated value should be: 0x4173000a
;//
MessageId    = 0x000a
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_DESTROY_SESSION_RECEIVED_NOT_USED
Language     = English
Destroying Copy Session %2.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Copy session destroyed successfully
;//
;// Generated value should be: 0x4173000b
;//
MessageId    = 0x000b
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_INFO_DESTROY_SESSION_COMPLETED_NOT_USED
Language     = English
Destroy Copy Session %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Copy operation completed succesfully
;//
;// Generated value should be: 0x4173000c
;//
MessageId    = 0x000c
Severity     = Informational
Facility     = LMTDX
SymbolicName = TDX_COPY_PROG_INFO_COPY_COMPLETED_NOT_USED
Language     = English
Copy Session %2 completed successfully.
.

;
;
;//========================
;//======= WARNING MSGS ======
;//===========================
;
;
;


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Template
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0x81734000
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = LMTDX
SymbolicName = TDX_WARNING_TEMPLATE_NOT_USED
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
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:An internal error occurred.  
;//
;// Generated value should be: 0xC1738000
;//
MessageId    = 0x8000
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_INTERNAL
Language     = English
An internal error occurred. Please contact the service provider.
.


;// --------------------------------------------------
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the specified source does not exist.
;//
;// Generated value should be: 0xC1738001
;//
MessageId    = 0x8001
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_INVALID_SRC_ID  
Language     = English
The specified session source does not exist. Please specify a valid source ID and retry the command.
.


;// --------------------------------------------------
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the specified destination does not exist.
;//
;// Generated value should be: 0xC1738002
;//
MessageId    = 0x8002
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_INVALID_DEST_ID  
Language     = English
The specified session destination does not exist. Please specify a valid destination ID and retry the command.
.


;// --------------------------------------------------
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the specified base (for comparison) does not exist.
;//
;// Generated value should be: 0xC1738003
;//
MessageId    = 0x8003
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_INVALID_COMP_ID  
Language     = English
The specified session base (for comparison) does not exist. Please specify a valid ID and retry the command.
.

;// --------------------------------------------------
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the specified session src and base (for comparison) are not in the same family (container FS)  
;//
;// Generated value should be: 0xC1738004
;//
MessageId    = 0x8004
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_SRC_COMP_INCOMPATIBLE
Language     = English
The specified session source and base (for comparison) are not in the same family. Please retry the command with source and base in the same family.
.

;// --------------------------------------------------
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the specified session source and base (for comparison) are not the same size.
;//
;// Generated value should be: 0xC1738005
;//
MessageId    = 0x8005
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_SRC_COMP_INCOMPATIBLE_SIZE
Language     = English
The specified session source and base (for comparison) are not the same size. Please retry the command with source and base of same size.
.

;// --------------------------------------------------
;// Introduced In: PSI7                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the specified session source is larger than the destination.
;//
;// Generated value should be: 0xC1738006
;//
MessageId    = 0x8006
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_SRC_DEST_INCOMPATIBLE_SIZE 
Language     = English
The specified session source is larger than the destination. Please retry the command with source size smaller than or equal to the destination size.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because session ID passed in is invalid.
;//
;// Generated value should be: 0xC1738007
;//
MessageId    = 0x8007
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_INVALID_SESSION_ID
Language     = English
Session ID passed in is invalid. Please retry the command with a valid session ID.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the session could not be found.
;//
;// Generated value should be: 0xC1738008
;//
MessageId    = 0x8008
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_SESSION_NOT_FOUND
Language     = English
Session not found. Please retry the command with the ID of an existing session.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because NDU is in progress.
;//
;// Generated value should be: 0xC1738009
;//
MessageId    = 0x8009
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_NDU_IN_PROGRESS
Language     = English
Array upgrade is in progress. Please retry the command once the upgrade completes.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because destination is not reachable.
;//
;// Generated value should be: 0xC173800A
;//
MessageId    = 0x800A
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_DESTINATION_NOT_REACHABLE
Language     = English
Destination is not reachable. Please make sure destination is accessible and retry the command.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because source is offline
;//
;// Generated value should be: 0xC173800B
;//
MessageId    = 0x800B
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_SRC_OFFLINE
Language     = English
The operation cannot proceed because the source is offline. 
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because destination is offline
;//
;// Generated value should be: 0xC173800C
;//
MessageId    = 0x800C
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_DEST_OFFLINE
Language     = English
The operation cannot proceed because the destination is offline.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the base (for comparison) is offline
;//
;// Generated value should be: 0xC173800D
;//
MessageId    = 0x800D
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_COMP_OFFLINE
Language     = English
The operation cannot proceed because the base (for comparison) is offline.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because it is not supported currently.
;//
;// Generated value should be: 0xC173800E
;//
MessageId    = 0x800E
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_CURRENTLY_NOT_SUPPORTED
Language     = English
Requested operation is not supported currently.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because destroy is not allowed when session is active.
;//
;// Generated value should be: 0xC173800F
;//
MessageId    = 0x800F
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_DESTROY_NOT_ALLOWED_SESSION_ACTIVE
Language     = English
Destroy is not allowed when session is active.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The command failed because the session is not in finalize-required state
;//
;// Generated value should be: 0xC1738010
;//
MessageId    = 0x8010
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_FINALIZE_SESSION_NOT_ALLOWED
Language     = English
Requested operation is not allowed because the session is not in finalize-required state.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: Temporary error code for exception testing.
;//
;// Generated value should be: 0xC1738011
;//
MessageId    = 0x8011
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_SESSION_INTERNAL_ERROR
Language     = English
This is an temporary error code.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: Start of a session that is already running.
;//
;// Generated value should be: 0xC1738012
;//
MessageId    = 0x8012
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_START_SESSION_ALREADY_RUNNING
Language     = English
You cannot start a session that is already running.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: Attempt to use an event that is already in use.
;//
;// Generated value should be: 0xC1738013
;//
MessageId    = 0x8013
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_EVENT_IN_USE
Language     = English
The event is already in use.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The TDX Thread Mgr could not attach
;// to the BasicPerCpuThreadPriorityMonitorProxy
;//
;// Generated value should be: 0xC1738014
;//
MessageId    = 0x8014
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_THREAD_MGR_COULD_NOT_ATTACH_TO_PROXY
Language     = English
The TDX Thread Mgr could not attach to the BasicPerCpuThreadPriorityMonitorProxy
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The TDX Thread Mgr could not attach
;// to the BasicPerCpuThreadPriorityMonitorProxy
;//
;// Generated value should be: 0xC1738015
;//
MessageId    = 0x8015
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_THREAD_MGR_COULD_NOT_INITIALIZE_WORKER_THREAD
Language     = English
The TDX Thread Mgr could not initialize a Worker Thread
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The TDX DM Task could not create
;// a DM request during initialization
;//
;// Generated value should be: 0xC1738016
;//
MessageId    = 0x8016
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_TASK_COULD_NOT_CREATE_DM_REQUEST
Language     = English
The TDX DM Task could not create a DM request
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Destroy Session operation could not be performed because
;// the session is already in the destroying state
;//
;// Generated value should be: 0xC1738017
;//
MessageId    = 0x8017
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_DESTROY_NOT_ALLOWED_SESSION_ALREADY_DESTROYING
Language     = English
TDX Destroy Session operation is not allowed because the session is already in the destroying state.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Finalize Session operation not could be performed because
;// the session is already in the finalizing state
;//
;// Generated value should be: 0xC1738018
;//
MessageId    = 0x8018
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_FINALIZE_NOT_ALLOWED_SESSION_ALREADY_FINALIZING
Language     = English
TDX Finalize Session operation is not allowed because the session is already in the finalizing state.
.


;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Finalize Session operation not could be performed because
;// the session has already been completed.
;//
;// Generated value should be: 0xC1738019
;//
MessageId    = 0x8019
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_FINALIZE_NOT_ALLOWED_SESSION_ALREADY_COMPLETED
Language     = English
TDX Finalize Session operation is not allowed because the session has already completed.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: Start of a session that is not in the ready state
;//
;// Generated value should be: 0xC173801A
;//
MessageId    = 0x801A
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_API_ERROR_START_NOT_ALLOWED_SESSION_NOT_READY
Language     = English
The specified session cannot be started because it is not in the ready state.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: Source ID and Destination ID are the same.
;//
;// Generated value should be: 0xC173801B
;//
MessageId    = 0x801B
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_CREATE_SESSION_SRC_AND_DEST_SAME_NOT_USED
Language     = English
The specified session source is the same as its destination. Please specify different ID for source or destination and retry the command.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to create a full copy session
;//
;// Generated value should be: 0xC173801C
;//
MessageId    = 0x801C
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_ERROR_CREATE_FULL_COPY_FAILED
Language     = English
Failed to create a Full Copy Session. Src %2 Dest %3 Len %4 Error %5.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to create a diff copy session
;//
;// Generated value should be: 0xC173801D
;//
MessageId    = 0x801D
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_ERROR_CREATE_DIFF_COPY_FAILED
Language     = English
Failed to create a Diff Copy Session. Src %2 Dest %3 Comp %4 Len %5 Error %6.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to start a copy session
;//
;// Generated value should be: 0xC173801E
;//
MessageId    = 0x801E
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_ERROR_START_SESSION_FAILED
Language     = English
Failed to start Copy Session %2. Error %3.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to finalize a copy session
;//
;// Generated value should be: 0xC173801F
;//
MessageId    = 0x801F
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_ERROR_FINALIZE_SESSION_FAILED
Language     = English
Failed to Finalize Copy Session %2. Error %3.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to destroy a copy session
;//
;// Generated value should be: 0xC1738020
;//
MessageId    = 0x8020
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_SESSION_MGR_ERROR_DESTROY_SESSION_FAILED
Language     = English
Failed to Destroy Copy Session %2. Error %3.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: The copy operation has failed.
;//
;// Generated value should be: 0xC1738021
;//
MessageId    = 0x8021
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_COPY_PROG_ERROR_COPY_FAILED
Language     = English
Copy session %2 has failed. Error %3.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified DataTransferLengthInBytes value is <= 0.
;//
;// Generated value should be: 0xC1738022
;//
MessageId    = 0x8022
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_CREATE_SESSION_INVALID_TRANSFER_LENGTH_NOT_USED
Language     = English
The specified DataTransferLengthInBytes value is <= 0.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified FilterType is not supported
;//
;// Generated value should be: 0xC1738023
;//
MessageId    = 0x8023
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_POLL_INVALID_FILTER_TYPE
Language     = English
The specified FilterType is not supported.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified ClientType does not match the ClientType associated with the TDX Session.
;//
;// Generated value should be: 0xC1738024
;//
MessageId    = 0x8024
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_CLIENT_TYPE_MISMATCH 
Language     = English
The specified ClientType does not match the ClientType associated with the TDX Session.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified ClientType is not a valid value.
;//
;// Generated value should be: 0xC1738025
;//
MessageId    = 0x8025
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_CLIENT_TYPE_INVALID_NOT_USED
Language     = English
The specified ClientType is not a valid value.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: API caller is using different version value than 
;//                     api library version.
;//
;// Generated value should be: 0xC1738026
;//
MessageId    = 0x8026
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_API_VERSION_INCOMPATIBLE_NOT_USED
Language     = English
The client specified version is not compatible with current API version.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The number of valid bits is greater than expected
;//
;// Generated value should be: 0xC1738027
;//
MessageId    = 0x8027
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_SEGMENT_BITMAP_VALID_BIT_OVERRUN_NOT_USED
Language     = English
The Segment's bitmap has more valid bits than expected.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The combination of the Sgl parameters are not valid
;//
;// Generated value should be: 0xC1738028
;//
MessageId    = 0x8028
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_SEGMENT_INVAID_SGL_PARAMETER_COMBINATION_NOT_USED
Language     = English
The specified Segment Sgl parameter combination is invalid.
.

;// --------------------------------------------------
;// Introduced In: PSI7
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified SourceStartOffsetInBytes value is less than 0.
;//
;// Generated value should be: 0xC1738029
;//
MessageId    = 0x8029
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_ERROR_CREATE_SESSION_INVALID_STARTING_OFFSET_NOT_USED
Language     = English
The specified starting offset value is less than 0.
.

;
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
;// Introduced In: PSI7
;//
;// Usage: Template
;//
;// Severity: Error
;//
;// Symptom of Problem: Template
;//
;// Generated value should be: 0xC173C000
;//
MessageId    = 0xC000
Severity     = Error
Facility     = LMTDX
SymbolicName = TDX_CRITICAL_TEMPLATE_NOT_USED
Language     = English
Only for critical template
.


;#endif //__K10_TDX_MESSAGES__
