MessageIdTypedef=DWORD
;//
;//     User space messages
;//
;// INFORMATIONAL MSGS
MessageId=0
Severity=Informational
Facility=Application
SymbolicName=UMSG_GENERIC_INFO
Language=English
No text.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=UMSG_GENERIC_INFO_TEXT
Language=English
%1
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=UMSG_GENERIC_INFO_TEXT_TEXT
Language=English
%1 %2
.
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=UMSG_GENERIC_INFO_EXP
Language=English
Exception: %1; File: %2; Line: %3.
.
; // GOVERNOR INFO MESSAGES
;
MessageId=+100
Severity=Informational
Facility=Application
SymbolicName=GOVERNOR_PARAMETERS
Language=English
Governor %1 started with params %2.
.
;//MONITOR INFO MESSAGES
;//
MessageId=+100
Severity=Informational
Facility=Application
SymbolicName=MONITOR_STARTUP_BEGAN
Language=English
K10Monitor process started, executable = %1.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=MONITOR_ALL_PROCESSES_STARTED
Language=English
All processes started, process count = %1.
.
;//
;// WARNING MESSAGES
;
MessageId=0x4000
Severity=Warning
Facility=Application
SymbolicName=UMSG_GENERIC_WARN
Language=English
No text.
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=UMSG_GENERIC_WARN_TEXT
Language=English
%1
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=UMSG_GENERIC_WARN_TEXT_TEXT
Language=English
%1 %2
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=UMSG_GENERIC_WARN_EXP
Language=English
Exception: %1; File: %2; Line: %3.
.
;
;// ERROR MESSAGES
;
MessageId=0x8000
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_ERROR
Language=English
No text.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_ERROR_TEXT
Language=English
%1
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_ERROR_TEXT_TEXT
Language=English
%1 %2
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_ERROR_EXP
Language=English
Exception: %1; File: %2; Line: %3.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_ERROR_EXP_EX
Language=English
Command: %1; Exception: %2; File: %3; Line: %4.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_ERROR_TEXT_TLD
Language=English
Failing Command: %1.
.
; // GOVERNOR ERRORS
;
MessageId=+100
Severity=Error
Facility=Application
SymbolicName=GOVERNOR_ERROR_STARTING_SERVICE
Language=English
No processes were defined to start.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=GOVERNOR_ERROR_SERVICE_DISPATCHER
Language=English
Error %1 returned by service dispatcher.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=GOVERNOR_EXCEPTION
Language=English
Exception caught in the Governor.
.
;//
;// MONITOR ERRORS
;
MessageId=+100
Severity=Error
Facility=Application
SymbolicName=MONITOR_NO_PROCESS_DEFINED
Language=English
No processes were defined to be started.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_PROC_PTR_MEMORY
Language=English
Memory alloc failed for %1 process objects.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_PROC_OBJECT_MEMORY
Language=English
Memory allocation failed for process object #%1.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_HANDLE_MEMORY
Language=English
Memory allocation failed for %1 handles.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_WAIT_ERROR
Language=English
Error in a wait: %1.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_STARTUP_RETRY
Language=English
Timed out waiting for startup event from process %1.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_COMPLETE_TIMEOUT
Language=English
Timed out waiting for process %1 to complete.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_WAIT_UNKNOWN_EVENT
Language=English
An unexpected event (%1) during a wait.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_PROCESS_TERMINATED
Language=English
Process %1 terminated with exit code %2.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_PROCESS_TERMINATED_NO
Language=English
Process %1 terminated no exit code available.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=MONITOR_STARTUP_FAIL
Language=English
Unable to start process %1.
.
;
;// CRITICAL MESSAGES
;
MessageId=0xC000
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_CRITICAL
Language=English
No text.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_CRITICAL_TEXT
Language=English
%1
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_CRITICAL_TEXT_TEXT
Language=English
%1 %2
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=UMSG_GENERIC_CRITICAL_EXP
Language=English
Exception %1; File: %2; Line: %3.
.
