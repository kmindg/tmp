;//++
;// Copyright (C) EMC Corporation, 2007
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;//++
;// File:            K10RPSplitterMessages.h (MC)
;//
;// Description:     This file contains message catalogue for RPSplitter driver and
;//                  Splitter notification library.
;//
;// The CLARiiON standard for how to classify errors according to their range
;// is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;// History:
;//
;//      06-Sep-2007    NS;       Initial creation.
;//--

;#ifndef __K10_RPSPLITTER_MESSAGES__
;#define __K10_RPSPLITTER_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS

FacilityNames = ( RPSplitter           = 0x12F         : FACILITY_RPSPLITTER )


SeverityNames = (
                  Success       = 0x0           : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1           : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2           : STATUS_SEVERITY_WARNING
                  Error         = 0x3           : STATUS_SEVERITY_ERROR
                )
                

;//
;//     Splitter "Informational" messages
;//

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Event log entry.
;// Severity: Informational
;// Symptom of Problem: A generic place holder to log any informational messages.

MessageId    = 0x0000
Severity     = Informational
Facility     = RPSplitter
SymbolicName = SPLITTER_INFORMATIONAL_TEXT
Language     = English
Informational message : %2
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Event log entry.
;// Severity: Informational
;// Symptom of Problem: Driver information on loading.
;//                     Informational to aid in debugging and understanding
;//                     operations on the array. 


MessageId    = +1
Severity     = Informational
Facility     = RPSplitter
SymbolicName = SPLITTER_INFO_DRIVER_LOAD
Language     = English
RPSplitterDriver (Build %2 %3) is starting...
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Event log entry.
;// Severity: Informational
;// Symptom of Problem: A Splitter volume has changed state.
;//                     Informational to aid in debugging and understanding
;//                     operations on the array. 


MessageId    = +1
Severity     = Informational
Facility     = RPSplitter
SymbolicName = SPLITTER_INFO_VOLUME_CHANGED_MODE
Language     = English
Volume %2:%3 has changed mode from %4 to %5 
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Event log entry.
;// Severity: Informational
;// Symptom of Problem: Notification from a layered driver (CPM or Clones or Rollback) 
;//                     is received for a Splitter volume.
;//                     Informational to aid in debugging and understanding
;//                     operations on the array. 

MessageId    = +1
Severity     = Informational
Facility     = RPSplitter
SymbolicName = SPLITTER_INFO_NOTIFICATION_RECEIVED
Language     = English
Volume %2:%3 has received a notification (Type %4)
.

;//
;//     Splitter "Warning" messages
;//

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Event log entry.
;// Severity: Warning
;// Symptom of Problem: A generic place holder to log any Warning messages.

MessageId    = 0x4000
Severity     = Warning
Facility     = RPSplitter
SymbolicName = SPLITTER_WARNING_TEXT
Language     = English
Warning message : %2
.

;//
;//     Splitter "Error" messages
;//

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Event log entry.
;// Severity: Error
;// Symptom of Problem: A generic place holder to log any error messages.

MessageId    = 0x8000
Severity     = Error
Facility     = RPSplitter
SymbolicName = SPLITTER_ERROR_TEXT
Language     = English
Error message : %2
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if Splitter notification
;//                     library detects that Splitter is not enabled on
;//                     the array.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOT_ENABLED
Language     = English
Internal information only. RecoverPoint Splitter feature is not enabled on the array.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if a client attempts
;//                     to send start/complete notifications to the
;//                     Splitter prior to initializing the notification
;//                     library.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOTIFY_LIBRARY_NOT_INITIALIZED
Language     = English
Internal information only. Attempt to send notification to the Splitter prior to the
notification library initialization.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if the Splitter notification 
;//                     library fails to allocate memory for the input buffer 
;//                     while attempting to send start notification to the
;//                     Splitter driver.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOTIFY_OP_START_INPUT_BUFFER_ALLOC_FAILED
Language     = English
Internal information only. Unable to allocate memory for the Splitter's operation start
notification input buffer.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if the Splitter notification 
;//                     library fails to allocate memory for the output buffer 
;//                     while attempting to send start notification to the
;//                     Splitter driver.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOTIFY_OP_START_OUTPUT_BUFFER_ALLOC_FAILED
Language     = English
Internal information only. Unable to allocate memory for the Splitter's operation start
notification output buffer.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if the Splitter notification 
;//                     library fails to allocate memory for the input buffer 
;//                     while attempting to send complete notification to the
;//                     Splitter driver.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOTIFY_OP_COMPLETE_INPUT_BUFFER_ALLOC_FAILED
Language     = English
Internal information only. Unable to allocate memory for the Splitter's operation complete
notification input buffer.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if the Splitter notification 
;//                     library fails to allocate IRP to send the start
;//                     notification to the Splitter driver.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOTIFY_OP_START_IRP_ALLOC_FAILED
Language     = English
Internal information only. Unable to allocate IRP to send the operation start notification 
to the Splitter.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Internal information only.
;// Severity: Error
;// Symptom of Problem: This error is returned if the Splitter notification 
;//                     library fails to allocate IRP to send the complete
;//                     notification to the Splitter driver.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_SPLITTER_NOTIFY_OP_COMPLETE_IRP_ALLOC_FAILED
Language     = English
Internal information only. Unable to allocate IRP to send the operation complete notification 
to the Splitter.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Error returned to user
;// Severity: Error
;// Symptom of Problem: This error is returned if the user of 
;//                     either CPM, Clones or Rollback attempts 
;//                     an operation that can modify data on a
;//                     Splitter secondary LU.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = STATUS_OPERATION_NOT_ALLOWED_ON_SPLITTER_SECONDARY_LU
Language     = English
Operation not allowed on a RecoverPoint target replication volume.
.

;//------------------------------------------------------------
;// Introduced in Release 26 ( RP Splitter on CLARiiON Arrays )
;// Usage: Panic.
;// Severity: Error
;// Symptom of Problem: Memory Manager was unable to initialize a feature pool
;//                     for the Splitter. The cause is reported in the status code.
;//                     Rebooting the SP is required to correct the problem.
;//                     

MessageId    = +1
Severity     = Error
Facility     = RPSplitter
SymbolicName = SPLITTER_SYS_MEMORY_INIT_FAILED
Language     = English
Memory initialization failed with status = %2.
SP will reboot. Please call your Service Provider.
.

;
;#endif // __K10_RPSPLITTER_MESSAGES__
;




