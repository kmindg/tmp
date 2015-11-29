;//++
;// Copyright (C) EMC Corporation, 2007-2015
;// All rights reserved.
;// Licensed material -- property of EMC Corporation                   
;//--
;//
;//++
;// File:            K10TDXMessages.mc
;//
;// Description:     This file contains the message catalogue for TDX messages.
;//
;// History:
;//
;//--
;//************************************************************************
;//  GENERAL GUIDELINES AND STATUS FORMATS FOR TDX STATUS
;//************************************************************************
;// 1. Non-action specific warning or error that does not associate with 
;//    client actions
;//    a. This has the form of TDX-severity-reason,  
;//                      where severity means either INFO, WARNING or ERROR
;// 2. Action Specific warning or error
;//    a. This has the form of TDX-severity-action-reason, it is 
;//       used to identify a more specific error condition such as fail to 
;//       finalize a session due to the session is still in active state
;//       e.g.  TDX_ERROR_CREATE_SESSION_INVALID_SESSION_ID, 
;//             TDX_ERROR_DESTROY_SESSION_IN_FINALIZING_STATE,
;//             TDX_ERROR_FINALIZE_SESSION_IN_DESTROYING_STATE
;// 3. All non-TDX errors from other subsystems should be propagated up ;with
;//    minimal error mapping if appropriate, such as I/O errors from MLU.
;// 4. Error codes should not to be reused. Another word, a specific error 
;//    code can only be used once in Tdx, any error must be unique in Tdx 
;//    and with an unique reason
;// 5. Tdx error code value assignment is not specific to any Tdx components.
;//    We do not need to reserve a range of number for a specific component,
;//    the next available error code value is consumed as new error is 
;//    defined. 
;//
;//
;// The standard for how to classify errors according to their range is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;
;#ifndef __K10_TDX_MESSAGES__
;#define __K10_TDX_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_CODE
                  TDX           = 0x175 : FACILITY_TDX
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
;// Generated value should be: 0x41750000
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_TEMPLATE
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
;// Generated value should be: 0x41750001
;//
MessageId    = 0x0001
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_DRIVER_INIT_STARTED
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
;// Generated value should be: 0x41750002
;//
MessageId    = 0x0002
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_CREATE_FULL_COPY_RECEIVED
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
;// Generated value should be: 0x41750003
;//
MessageId    = 0x0003
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_CREATE_FULL_COPY_COMPLETED
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
;// Generated value should be: 0x41750004
;//
MessageId    = 0x0004
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_CREATE_DIFF_COPY_RECEIVED
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
;// Generated value should be: 0x41750005
;//
MessageId    = 0x0005
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_CREATE_DIFF_COPY_COMPLETED
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
;// Generated value should be: 0x41750006
;//
MessageId    = 0x0006
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_START_SESSION_RECEIVED
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
;// Generated value should be: 0x41750007
;//
MessageId    = 0x0007
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_START_SESSION_COMPLETED
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
;// Generated value should be: 0x41750008
;//
MessageId    = 0x0008
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_FINALIZE_SESSION_RECEIVED
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
;// Generated value should be: 0x41750009
;//
MessageId    = 0x0009
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_FINALIZE_SESSION_COMPLETED
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
;// Generated value should be: 0x4175000a
;//
MessageId    = 0x000a
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_DESTROY_SESSION_RECEIVED
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
;// Generated value should be: 0x4175000b
;//
MessageId    = 0x000b
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_DESTROY_SESSION_COMPLETED
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
;// Generated value should be: 0x4175000c
;//
MessageId    = 0x000c
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_SESSION_COPY_COMPLETED
Language     = English
Copy Session %2 completed successfully.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Received request to stop a copy session
;//
;// Generated value should be: 0x4175000d
;//
MessageId    = 0x000d
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_STOP_SESSION_RECEIVED
Language     = English
Stopping Copy Session %2.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Copy session stopped successfully
;//
;// Generated value should be: 0x4175000e
;//
MessageId    = 0x000e
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_STOP_SESSION_COMPLETED
Language     = English
Copy Session %2 stopped successfully.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Event Log
;//
;// Severity: Informational
;//
;// Symptom of Problem: Send transfer complete to indicate stopping session.
;//
;// Generated value should be: 0x4175000f
;//
MessageId    = 0x000f
Severity     = Informational
Facility     = TDX
SymbolicName = TDX_INFO_STOP_SESSION_SEND_TRANSFER_COMPLETE
Language     = English
Sending transfer complete to stop session %2.
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
;// Generated value should be: 0x81754000
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = TDX
SymbolicName = TDX_WARNING_TEMPLATE
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
;// Generated value should be: 0xC1758000
;//
MessageId    = 0x8000
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_INTERNAL
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
;// Generated value should be: 0xC1758001
;//
MessageId    = 0x8001
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_INVALID_SRC_ID
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
;// Generated value should be: 0xC1758002
;//
MessageId    = 0x8002
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_INVALID_DEST_ID
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
;// Generated value should be: 0xC1758003
;//
MessageId    = 0x8003
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_INVALID_COMP_ID
Language     = English
The specified session base (for comparison) does not exist. Please specify a valid ID and retry the command.
.

;// --------------------------------------------------
;// Introduced In: PSIQ                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The start session command failed because the specified session is not found  
;//
;// Generated value should be: 0xC1758004
;//
MessageId    = 0x8004
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_NOT_FOUND
Language     = English
The request to start a session failed because the session is not found.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage: Return to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The specified FilterType is not within the range of supported filter types
;//
;// Generated value should be: 0xC1758005
;//
MessageId    = 0x8005
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_POLL_SESSION_FILTER_TYPE_OUT_OF_RANGE
Language     = English
The poll session command failed because the specified FilterType is not within the range of supported filter types.
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
;// Generated value should be: 0xC1758006
;//
MessageId    = 0x8006
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_SRC_DEST_INCOMPATIBLE_SIZE
Language     = English
The specified session source is larger than the destination. Please retry the command with source size smaller than or equal to the destination size.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: There is failure allocating memory for class instantiation
;//
;// Generated value should be: 0xC1758007
;//
MessageId    = 0x8007
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_NO_MEMORY_FOR_CLASS_INSTANTIATION
Language     = English
There is an internal failure allocating memory during class instatiation.
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
;// Generated value should be: 0xC1758008
;//
MessageId    = 0x8008
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SEGMENT_BITMAP_VALID_BIT_OVERRUN
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
;// Generated value should be: 0xC1758009
;//
MessageId    = 0x8009
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_INVAID_SGL_PARAMETER_COMBINATION
Language     = English
The specified Segment Sgl parameter combination is invalid.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The destroy command failed because the specified session is not found
;//
;// Generated value should be: 0xC175800A
;//
MessageId    = 0x800A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESTROY_SESSION_NOT_FOUND
Language     = English
The request to destroy a session failed because the session is not found.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The IOCTL input buffer size is less than the required size
;//
;// Generated value should be: 0xC175800B
;//
MessageId    = 0x800B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_INPUT_BUFFER_TOO_SMALL
Language     = English
The input buffer size for the requested IOCTL is less than the required size.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: The IOCTL output buffer size is less than the required size
;//
;// Generated value should be: 0xC175800C
;//
MessageId    = 0x800C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_OUTPUT_BUFFER_TOO_SMALL
Language     = English
The output buffer size for the requested IOCTL is less than the required size.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem:The finalize command failed because the specified session is not found
;//
;// Generated value should be: 0xC175800D
;//
MessageId    = 0x800D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_NOT_FOUND
Language     = English
The request to finalize a session failed because the session is not found.
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
;// Generated value should be: 0xC175800E
;//
MessageId    = 0x800E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CURRENTLY_NOT_SUPPORTED
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
;// Generated value should be: 0xC175800F
;//
MessageId    = 0x800F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESTROY_SESSION_ACTIVE
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
;// Generated value should be: 0xC1758010
;//
MessageId    = 0x8010
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_NOT_IN_FINALIZE_REQUIRED_STATE
Language     = English
Requested operation is not allowed because the session is not in finalize-required state.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage: Returned to Tdx client 
;//
;// Severity: Error
;//
;// Symptom of Problem: The command to destroy session cannot proceed because the specified ClientType does not match 
;//                     the ClientType associated with the TDX Session.
;//
;// Generated value should be: 0xC1758011
;//
MessageId    = 0x8011
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESTROY_SESSION_CLIENT_TYPE_MISMATCH
Language     = English
The command to destroy session cannot proceed because the specified ClientType does not match the ClientType associated with the TDX Session.
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
;// Generated value should be: 0xC1758012
;//
MessageId    = 0x8012
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_ALREADY_RUNNING
Language     = English
You cannot start a session that is already running.
.

;// --------------------------------------------------
;// Introduced In: PSIQ
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The command to finalize session cannot proceed because the specified ClientType does not match 
;//                     the ClientType associated with the TDX Session.
;//
;// Generated value should be: 0xC1758013
;//
MessageId    = 0x8013
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_CLIENT_TYPE_MISMATCH
Language     = English
The command to finalize session cannot proceed because the specified ClientType does not match the ClientType associated with the TDX Session.
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
;// Generated value should be: 0xC1758014
;//
MessageId    = 0x8014
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_THREAD_MGR_COULD_NOT_ATTACH_TO_PROXY
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
;// Generated value should be: 0xC1758015
;//
MessageId    = 0x8015
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_WORKER_THREAD_COULD_NOT_BE_INITIALIZED
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
;// Generated value should be: 0xC1758016
;//
MessageId    = 0x8016
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DM_REQUEST_COULD_NOT_BE_CREATED
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
;// Generated value should be: 0xC1758017
;//
MessageId    = 0x8017
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESTROY_SESSION_ALREADY_DESTROYING
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
;// Generated value should be: 0xC1758018
;//
MessageId    = 0x8018
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_ALREADY_FINALIZING
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
;// Generated value should be: 0xC1758019
;//
MessageId    = 0x8019
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_ALREADY_COMPLETED
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
;// Generated value should be: 0xC175801A
;//
MessageId    = 0x801A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_NOT_READY
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
;// Generated value should be: 0xC175801B
;//
MessageId    = 0x801B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CREATE_SESSION_SRC_AND_DEST_SAME
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
;// Generated value should be: 0xC175801C
;//
MessageId    = 0x801C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CREATE_FULL_COPY_FAILED
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
;// Generated value should be: 0xC175801D
;//
MessageId    = 0x801D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CREATE_DIFF_COPY_FAILED
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
;// Generated value should be: 0xC175801E
;//
MessageId    = 0x801E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_FAILED
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
;// Generated value should be: 0xC175801F
;//
MessageId    = 0x801F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_FAILED
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
;// Generated value should be: 0xC1758020
;//
MessageId    = 0x8020
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESTROY_SESSION_FAILED
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
;// Generated value should be: 0xC1758021
;//
MessageId    = 0x8021
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_COPY_FAILED
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
;// Generated value should be: 0xC1758022
;//
MessageId    = 0x8022
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CREATE_SESSION_INVALID_TRANSFER_LENGTH
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
;// Generated value should be: 0xC1758023
;//
MessageId    = 0x8023
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_POLL_SESSION_UNSUPPORTED_FILTER_TYPE
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
;// Symptom of Problem: The command to start session cannot proceed because the specified ClientType does not match 
;//                     the ClientType associated with the TDX Session.
;//
;// Generated value should be: 0xC1758024
;//
MessageId    = 0x8024
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_CLIENT_TYPE_MISMATCH
Language     = English
The command to start session cannot proceed because the specified ClientType does not match the ClientType associated with the TDX Session.
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
;// Generated value should be: 0xC1758025
;//
MessageId    = 0x8025
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CLIENT_TYPE_INVALID
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
;// Generated value should be: 0xC1758026
;//
MessageId    = 0x8026
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_API_VERSION_INCOMPATIBLE
Language     = English
The client specified version is not compatible with current API version.
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
;// Generated value should be: 0xC1758027
;//
MessageId    = 0x8027
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_CREATE_SESSION_INVALID_STARTING_OFFSET
Language     = English
The specified starting offset value is less than 0.
.


;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Internal use only
;//
;// Severity: Error
;//
;// Symptom of Problem: Collection is inconsistent across both SPs.
;//
;// Generated value should be: 0xC1758028
;//
MessageId    = 0x8028
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_COLLECTION_INCONSISTENT
Language     = English
The collection is inconsistent across both SPs.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to revive a session
;//
;// Generated value should be: 0xC1758029
;//
MessageId    = 0x8029
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_REVIVE_SESSION_FAILED
Language     = English
Failed to revive a Session. Primary Key %2 Type %3 Session Key %4 Error %5.
.


;// --------------------------------------------------
;// Introduced In: PSI8                                   
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem: The stop session command failed because the specified session is not found  
;//
;// Generated value should be: 0xC175802A
;//
MessageId    = 0x802A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_NOT_FOUND
Language     = English
The request to stop a session failed because the session %2 is not found.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Returned to Tdx client
;//
;// Severity: Error
;//
;// Symptom of Problem: The command to stop session cannot proceed because the specified ClientType does not match 
;//                     the ClientType associated with the TDX Session.
;//
;// Generated value should be: 0xC175802B
;//
MessageId    = 0x802B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_CLIENT_TYPE_MISMATCH
Language     = English
The command to stop session %2 cannot proceed because the specified ClientType %3 does not match the ClientType associated with the TDX Session %4.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Event Log
;//
;// Severity: Error
;//
;// Symptom of Problem: Failed to stop a copy session
;//
;// Generated value should be: 0xC175802C
;//
MessageId    = 0x802C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_FAILED
Language     = English
Failed to stop Copy Session %2. Error %3.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Destroy Session operation could not be performed because
;// the session is in the stopping state
;//
;// Generated value should be: 0xC175802D
;//
MessageId    = 0x802D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESTROY_SESSION_STOPPING
Language     = English
TDX Destroy Session operation is not allowed because the session is in stopping state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Stop Session operation could not be performed because
;// the session is in the destroying state
;//
;// Generated value should be: 0xC175802E
;//
MessageId    = 0x802E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_DESTROYING
Language     = English
TDX Stop Session operation is not allowed because the session is in destroying state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Stop Session operation could not be performed because
;// the session is in the completed state
;//
;// Generated value should be: 0xC175802F
;//
MessageId    = 0x802F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_COMPLETED
Language     = English
TDX Stop Session operation is not allowed because the session is in completed state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Stop Session operation could not be performed because
;// the session is in already in stopping state
;//
;// Generated value should be: 0xC1758030
;//
MessageId    = 0x8030
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_ALREADY_STOPPING
Language     = English
TDX Stop Session operation is not allowed because the session is in already in stopping state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Stop Session operation could not be performed because
;// the session is in the ready state
;//
;// Generated value should be: 0xC1758031
;//
MessageId    = 0x8031
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_READY
Language     = English
TDX Stop Session operation is not allowed because the session is in ready state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Stop Session operation could not be performed because
;// the session is in an invalid state
;//
;// Generated value should be: 0xC1758032
;//
MessageId    = 0x8032
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_NOT_SUPPORTED
Language     = English
TDX Stop Session operation is not allowed because the session is in an invalid state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage: Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Start Session operation could not be performed because
;// the session is in stopping state
;//
;// Generated value should be: 0xC1758033
;//
MessageId    = 0x8033
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_START_SESSION_STOPPING
Language     = English
TDX Stop Session operation is not allowed because the session is in stopping state.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: TDX Finalize Session operation not could be performed because
;// the session is stopping.
;//
;// Generated value should be: 0xC1758034
;//
MessageId    = 0x8034
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FINALIZE_SESSION_STOPPING
Language     = English
TDX Finalize Session operation is not allowed because the session is stopping.
.

;// --------------------------------------------------
;// Introduced In: PSI8                                   
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem: The stop session command failed because the specified session is not active  
;//
;// Generated value should be: 0xC1758035
;//
MessageId    = 0x8035
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_STOP_SESSION_NOT_ACTIVE
Language     = English
The request to stop a session failed because the session %2 is not active.
.

;// --------------------------------------------------
;// Introduced In: PSI8                                   
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem: Client attempts to create more than the maximum number of sessions. 
;//
;// Generated value should be: 0xC1758036
;//
MessageId    = 0x8036
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_MAX_NUMBER_OF_SESSIONS_EXIST
Language     = English
The request to create a session failed because the maximum number of sessions already exist.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: Client attempts to create multiple sessions with same destination.
;//
;// Generated value should be: 0xC1758037
;//
MessageId    = 0x8037
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_MULTIPLE_SESSIONS_WITH_SAME_DESTINATION
Language     = English
The request to create a session failed because there is already another session with the same destination.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: Client attempts to create session with a source which is same as destination of an existing session.
;//
;// Generated value should be: 0xC1758038
;//
MessageId    = 0x8038
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SPECIFIED_SOURCE_IS_ALREADY_A_DESTINATION
Language     = English
The request to create a session failed because there is already another session with destination same as source in the current request.
.

;// --------------------------------------------------
;// Introduced In: PSI8
;//
;// Usage:Return to TDX Client
;//
;// Severity: Error
;//
;// Symptom of Problem: Client attempts to create session with a destination which is same as source of an existing session.
;//
;// Generated value should be: 0xC1758039
;//
MessageId    = 0x8039
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SPECIFIED_DESTINATION_IS_ALREADY_A_SOURCE
Language     = English
The request to create a session failed because there is already another session with source same as destination in the current request.
.

    
;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encoding the session client context to GPB format.
;//
;// Generated value should be: 0xC175803A
;//
MessageId    = 0x803A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_CLIENT_CONTEXT_ENCODE
Language     = English
An error occurred while encoding the session client context into protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error decoding the session client context from GPB format.
;//
;// Generated value should be: 0xC175803B
;//
MessageId    = 0x803B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_CLIENT_CONTEXT_DECODE
Language     = English
An error encountered while decoding the session client context from protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encoding the session core properties to GPB format.
;//
;// Generated value should be: 0xC175803C
;//
MessageId    = 0x803C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_CORE_PROP_ENCODE
Language     = English
An error occurred while encoding the session core properties into protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error decoding the session core properties from GPB format.
;//
;// Generated value should be: 0xC175803D
;//
MessageId    = 0x803D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_CORE_PROP_DECODE
Language     = English
An error encountered while decoding the session core properties from protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encoding the session pageable properties to GPB format.
;//
;// Generated value should be: 0xC175803E
;//
MessageId    = 0x803E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_PAGEABLE_PROP_ENCODE
Language     = English
An error occurred while encoding the session pageable properties into protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error decoding the session pageable properties from GPB format.
;//
;// Generated value should be: 0xC175803F
;//
MessageId    = 0x803F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_PAGEABLE_PROP_DECODE
Language     = English
An error encountered while decoding the session pageable properties from protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encoding the session descriptor to GPB format.
;//
;// Generated value should be: 0xC1758040
;//
MessageId    = 0x8040
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_DESCRIPTOR_ENCODE
Language     = English
An error occurred while encoding the session descriptor into protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error decoding the session descriptor from GPB format.
;//
;// Generated value should be: 0xC1758041
;//
MessageId    = 0x8041
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_SESSION_DESCRIPTOR_DECODE
Language     = English
An error encountered while decoding the session descriptor from protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Client Context missing from the data read from depot during deserialize.
;//
;// Generated value should be: 0xC1758042
;//
MessageId    = 0x8042
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CLIENT_CONTEXT_MISSING
Language     = English
Client Context is not present in the data read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Session type missing from the data read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758043
;//
MessageId    = 0x8043
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_MISSING_SESSION_TYPE
Language     = English
Session type is not present in the core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid Session type read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758044
;//
MessageId    = 0x8044
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_INVALID_SESSION_TYPE
Language     = English
Invalid Session type found in core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Source ID missing from the data read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758045
;//
MessageId    = 0x8045
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_MISSING_SOURCE_ID
Language     = English
Source ID is not present in the core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Compare ID missing from the data read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758046
;//
MessageId    = 0x8046
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_MISSING_COMPARE_ID
Language     = English
Compare ID is not present in the core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Destination ID missing from the data read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758047
;//
MessageId    = 0x8047
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_MISSING_DESTINATION_ID
Language     = English
Destination ID is not present in the core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Client type missing from the data read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758048
;//
MessageId    = 0x8048
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_MISSING_CLIENT_TYPE
Language     = English
Client type is not present in the core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid Client type read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC1758049
;//
MessageId    = 0x8049
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_INVALID_CLIENT_TYPE
Language     = English
Invalid Client type found in core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Session state missing from the data read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC175804A
;//
MessageId    = 0x804A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_MISSING_SESSION_STATE
Language     = English
Session state is not present in the core properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid Session state read from depot during core prop deserialize.
;//
;// Generated value should be: 0xC175804B
;//
MessageId    = 0x804B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_CORE_PROP_INVALID_SESSION_STATE
Language     = English
Invalid Session state found in core properties read from depot.
.

    
;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Session type missing from the data read from depot during session descriptor deserialize.
;//
;// Generated value should be: 0xC175804C
;//
MessageId    = 0x804C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_DESC_MISSING_SESSION_TYPE
Language     = English
Session type is not present in the session descriptor read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Invalid Session type read from depot during session descriptor deserialize.
;//
;// Generated value should be: 0xC175804D
;//
MessageId    = 0x804D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_DESC_INVALID_SESSION_TYPE
Language     = English
Invalid Session type found in session descriptor read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Session ID missing from the data read from depot during session descriptor deserialize.
;//
;// Generated value should be: 0xC175804E
;//
MessageId    = 0x804E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_DESC_MISSING_SESSION_ID
Language     = English
Session ID is not present in the session descriptor read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Array ID missing from the data read from depot during session descriptor deserialize.
;//
;// Generated value should be: 0xC175804F
;//
MessageId    = 0x804F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_DESC_MISSING_ARRAY_ID
Language     = English
Array ID is not present in the session descriptor read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Session ID serialnumber missing from the data read from depot during session descriptor deserialize.
;//
;// Generated value should be: 0xC1758050
;//
MessageId    = 0x8050
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_DESC_MISSING_SERIALNUMBER
Language     = English
Session ID serialnumber is not present in the session descriptor read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Depot key missing from the data read from depot during session descriptor deserialize.
;//
;// Generated value should be: 0xC1758051
;//
MessageId    = 0x8051
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_DESC_MISSING_DEPOT_KEY
Language     = English
Depot key is not present in the session descriptor read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: SourceStartOffsetInBytes missing from the data read from depot during pageable properties deserialize.
;//
;// Generated value should be: 0xC1758052
;//
MessageId    = 0x8052
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_PAGEABLE_PROP_MISSING_SOURCE_START_OFFSET
Language     = English
SourceStartOffsetInBytes is not present in the session pageable properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: DataTransferLengthInBytes missing from the data read from depot during session pageable properties deserialize.
;//
;// Generated value should be: 0xC1758053
;//
MessageId    = 0x8053
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_PAGEABLE_PROP_MISSING_DATA_TRANSFER_LENGTH
Language     = English
DataTransferLengthInBytes is not present in the session pageable properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: TotalBytesTransferred missing from the data read from depot during session pageable properties deserialize.
;//
;// Generated value should be: 0xC1758054
;//
MessageId    = 0x8054
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_PAGEABLE_PROP_MISSING_TOTAL_BYTES_TRANSFERRED
Language     = English
TotalBytesTransferred is not present in the session pageable properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: SessionCheckpoint missing from the data read from depot during session pageable properties deserialize.
;//
;// Generated value should be: 0xC1758055
;//
MessageId    = 0x8055
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_SESSION_PAGEABLE_PROP_MISSING_CHECKPOINT
Language     = English
SessionCheckpoint is not present in the session pageable properties read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error decoding the peer message header from GPB format.
;//
;// Generated value should be: 0xC1758056
;//
MessageId    = 0x8056
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_PEER_COMMUNICATOR_NANO_PB_MESSAGE_HEADER_DECODE 
Language     = English
An error encountered while decoding the peer message header from protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message starts with a "magic cookie" to prevent non GPB messages
;//                     from being decoded as valid but incorrect GPB message. The message being decoded is
;//                     missing its magic cookie.
;//
;// Generated value should be: 0xC1758057
;//
MessageId    = 0x8057
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_MAGIC_COOKIE
Language     = English
The decoded peer message header is missing its magic cookie.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message starts with a "magic cookie" to prevent non GPB messages
;//                     from being decoded as valid but incorrect GPB message. The message being has
;//                     an invalid cookie.
;//
;// Generated value should be: 0xC1758058
;//
MessageId    = 0x8058
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_MAGIC_COOKIE
Language     = English
The decoded peer message header has an invalid magic cookie.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message type. 
;//                     The message being decoded is missing its message type.
;//
;// Generated value should be: 0xC1758059
;//
MessageId    = 0x8059
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_MESSAGE_TYPE
Language     = English
The decoded peer message header is missing its message type.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message type. 
;//                     The message being decoded has in invalid message type.
;//
;// Generated value should be: 0xC175805A
;//
MessageId    = 0x805A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_MESSAGE_TYPE
Language     = English
The decoded peer message header has an invalid message type.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message size. 
;//                     The message being decoded is missing its message size.
;//
;// Generated value should be: 0xC175805B
;//
MessageId    = 0x805B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_MESSAGE_SIZE
Language     = English
The decoded peer message header is missing its message size.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message size. 
;//                     The message being decoded has in invalid message size.
;//
;// Generated value should be: 0xC175805C
;//
MessageId    = 0x805C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_MESSAGE_SIZE
Language     = English
The decoded peer message header has an invalid message size.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message sequence number. 
;//                     The message being decoded is missing its message sequence number.
;//
;// Generated value should be: 0xC175805D
;//
MessageId    = 0x805D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_SEQUENCE_NUMBER
Language     = English
The decoded peer message header is missing its message sequence number.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message sequence number. 
;//                     The message being decoded has in invalid message sequence number.
;//
;// Generated value should be: 0xC175805E
;//
MessageId    = 0x805E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_SEQUENCE_NUMBER
Language     = English
The decoded peer message header has an invalid message sequence number.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the is RPC flag. 
;//                     The message being decoded is missing its is RPC flag.
;//
;// Generated value should be: 0xC175805F
;//
MessageId    = 0x805F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_IS_RPC
Language     = English
The decoded peer message header is missing its is RPC flag.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the is RPC flag. 
;//                     The message being decoded has in invalid is RPC flag.
;//
;// Generated value should be: 0xC1758060
;//
MessageId    = 0x8060
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_IS_RPC
Language     = English
The decoded peer message header has an invalid is RPC flag.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the instantiation id of the sender. 
;//                     The message being decoded is missing its instantiation id of the sender.
;//
;// Generated value should be: 0xC1758061
;//
MessageId    = 0x8061
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_INSTANTIATION_ID
Language     = English
The decoded peer message header is missing its instantiation id of the sender.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the instantiation id of the sender. 
;//                     The message being decoded has in invalid instantiation id of the sender.
;//
;// Generated value should be: 0xC1758062
;//
MessageId    = 0x8062
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_INSTANTIATION_ID
Language     = English
The decoded peer message header has an invalid instantiation id of the sender.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message status. 
;//                     The message being decoded is missing its message status.
;//
;// Generated value should be: 0xC1758063
;//
MessageId    = 0x8063
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_MISSING_MESSAGE_STATUS
Language     = English
The decoded peer message header is missing its message status.
.


;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Each encoded Tdx Peer Message header includes the message status. 
;//                     The message being decoded has in invalid message status.
;//
;// Generated value should be: 0xC1758064
;//
MessageId    = 0x8064
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_PEER_MESSAGE_INVALID_MESSAGE_STATUS
Language     = English
The decoded peer message header has an invalid message status.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encoding the peer message header to GPB format.
;//
;// Generated value should be: 0xC1758065
;//
MessageId    = 0x8065
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_PEER_COMMUNICATOR_NANO_PB_MESSAGE_HEADER_ENCODE 
Language     = English
An error occurred while encoding the peer message header into protocol buffer format.
.
;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error encoding the global table to GPB format.
;//
;// Generated value should be: 0xC1758066
;//
MessageId    = 0x8066
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_GLOBAL_TABLE_ENCODE
Language     = English
An error occurred while encoding the global into protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Error decoding the global table from GPB format.
;//
;// Generated value should be: 0xC1758067
;//
MessageId    = 0x8067
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_GLOBAL_TABLE_DECODE
Language     = English
An error encountered while decoding the global table from protocol buffer format.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The information whether the global table is initialized is missing 
;//						from the data read from depot during global table  deserialize.
;//
;// Generated value should be: 0xC1758068
;//
MessageId    = 0x8068
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_ISINITIALIZED
Language     = English
The information whether the global table is initialized is not present in the global table read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The SerialNumber is missing from the data read from depot during global table deserialize.
;//
;// Generated value should be: 0xC1758069
;//
MessageId    = 0x8069
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_SERIAL_NUMBER
Language     = English
The SerialNumber is not present in the global table read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The KeyBitmap is missing from the data read from depot during global table deserialize.
;//
;// Generated value should be: 0xC175806A
;//
MessageId    = 0x806A
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_KEYBITMAP
Language     = English
The KeyBitmap is not present in the global table read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The KeyBitmap's NumberOfBits is missing from the data read from depot during global table deserialize.
;//
;// Generated value should be: 0xC175806B
;//
MessageId    = 0x806B
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_KEYBITMAP_NUMBEROFBITS
Language     = English
The KeyBitmap's number of bits is not present in the global table read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The KeyBitmap's bits set is missing from the data read from depot during global table deserialize.
;//
;// Generated value should be: 0xC175806C
;//
MessageId    = 0x806C
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_KEYBITMAP_BITS_SET
Language     = English
The KeyBitmap's bits set is not present in the global table read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The KeyBitmap's bits is missing from the data read from depot during global table deserialize.
;//
;// Generated value should be: 0xC175806D
;//
MessageId    = 0x806D
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_KEYBITMAP_BITS
Language     = English
The KeyBitmap's bits is not present in the global table read from depot.
.

;// --------------------------------------------------
;// Introduced In: PSI9
;//
;// Usage:Internal Use Only
;//
;// Severity: Error
;//
;// Symptom of Problem: The OpaqueKeyBits is missing from the data read from depot during global table deserialize.
;//
;// Generated value should be: 0xC175806E
;//
MessageId    = 0x806E
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_DESERIALIZE_GLOBAL_TABLE_MISSING_OPAQUE_KEYBITS
Language     = English
The OpaqueKeyBits is not present in the global table read from depot.
.
	
;// --------------------------------------------------
;// Introduced In: PSI9                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The update copyprogress command failed because the specified session is not found  
;//
;// Generated value should be: 0xC175806F
;//
MessageId    = 0x806F
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_UPDATE_SESSION_NOT_FOUND
Language     = English
The request to update a session failed because the session was not found.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The fail session command failed because the specified session is not found  
;//
;// Generated value should be: 0xC1758070
;//
MessageId    = 0x8070
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_FAIL_SESSION_NOT_FOUND
Language     = English
The request to fail a session failed because the session was not found.
.

;// --------------------------------------------------
;// Introduced In: PSI9                                    
;//
;// Usage:Return to TDX Client 
;//
;// Severity: Error
;//
;// Symptom of Problem:The attempt to resume copying failed due to an unsafe session state. 
;//
;// Generated value should be: 0xC1758071
;//
MessageId    = 0x8071
Severity     = Error
Facility     = TDX
SymbolicName = TDX_ERROR_RESUME_COPY_UNSAFE_SESSION_STATE
Language     = English
The attempt to resume copying failed due to an unsafe session state.
.
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
;// Generated value should be: 0xC175C000
;//
MessageId    = 0xC000
Severity     = Error
Facility     = TDX
SymbolicName = TDX_CRITICAL_TEMPLATE
Language     = English
Only for critical template
.


;#endif //__K10_TDX_MESSAGES__
