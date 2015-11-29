;//++
;// Copyright (C) EMC Corporation, 2003
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;//++
;// File:            K10IoMarshalMessages.h (MC)
;//
;// Description:     This file contains IoMarshal bug check codes.
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
;//                  21-Mar-2003       V Nagapraveen     Added code review changes
;//                  11-Mar-2003       V Nagapraveen     Created
;//--
;
;#ifndef __K10_IOMARSHAL_MESSAGES__
;#define __K10_IOMARSHAL_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( IoMarshal           = 0x12C         : FACILITY_IOMARSHAL )


SeverityNames = (
                  Success       = 0x0           : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1           : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2           : STATUS_SEVERITY_WARNING
                  Error         = 0x3           : STATUS_SEVERITY_ERROR
                )


MessageId    = 0x8000
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_INVALID_IRP
Language     = English
The client has sent a NULL IRP pointer as an argument while arresting I/O. 
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_QUEUE_NOT_INITIALIZED
Language     = English
An operation is being performed on an un initialized I/O marshal Queue
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_ARREST_QUEUE_NOT_EMPTY
Language     = English
An attempt to clean up the I/O Marshal queue is made while there are still some
I/O' s in the arrest queue
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_INSPECT_FOUND_NEGATIVE_DEPTH
Language     = English
The I/O Marshal depth was found to be negative when we inspected an I/O.
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_DUPLICATE_EXTRICATE
Language     = English
The I/O Marshal depth went negative.  This means that the client called
extricate too many times.
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_ARREST_ALREADY_ARRESTED
Language     = English
An attempt was made to arrest an already arrested queue.
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_RELEASE_NOT_ARRESTED
Language     = English
An attempt was made to release a queue that wasn't arrested.
.

MessageId    = +1
Severity     = Error
Facility     = IoMarshal
SymbolicName = K10_IOMARSHAL_DRAIN_NOT_ARRESTED
Language     = English
An attempt was made to drain a queue that wasn't arrested.
.

MessageIdTypedef = EMCPAL_STATUS

;
;#endif // __K10_IOMARSHAL_MESSAGES__
;


