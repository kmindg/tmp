;//++
;// Copyright (C) EMC Corporation, 2011
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            TestControlMessages.mc
;//
;// Description:     This file contains the message catalogue for the
;//                  Test Control mechanism.
;//
;// History:
;//
;//--
;//
;// The CLARiiON standard for how to classify errors according to their range is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;
;#ifndef __TEST_CONTROL_MESSAGES__
;#define __TEST_CONTROL_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( System        = 0x0
                  RpcRuntime    = 0x2   : FACILITY_RPC_RUNTIME
                  RpcStubs      = 0x3   : FACILITY_RPC_STUBS
                  Io            = 0x4   : FACILITY_IO_CODE
                  TestControl   = 0x16E : FACILITY_TEST_CONTROL
                )

SeverityNames = ( Success       = 0x0   : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1   : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2   : STATUS_SEVERITY_WARNING
                  Error         = 0x3   : STATUS_SEVERITY_ERROR
                )



;
;
;//========================
;//======= INFO MSGS ======
;//========================
;
;
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Template
;//
;// Severity: Informational
;//
;// Symptom of Problem:
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = TestControl
SymbolicName = TEST_CONTROL_INFO_TEMPLATE
Language     = English
(template)
.



;//===========================
;//======= WARNING MSGS ======
;//===========================
;
;
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Template
;//
;// Severity: Warning
;//
;// Symptom of Problem:
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = TestControl
SymbolicName = TEST_CONTROL_WARNING_TEMPLATE
Language     = English
(template)
.



;
;
;
;//=========================
;//======= ERROR MSGS ======
;//=========================
;
;
;

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Template
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = 0x8000
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_TEMPLATE
Language     = English
(template)
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_FEATURE_UNSPECIFIED
Language     = English
The feature for an IOTCL_TEST_CONTROL request was not specified.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_OPCODE_UNSPECIFIED
Language     = English
The opcode for an IOTCL_TEST_CONTROL request was not specified.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_UNKNOWN_DRIVER
Language     = English
The driver specified by an IOTCL_TEST_CONTROL request was not found.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_UNKNOWN_WWID
Language     = English
The WWID specified by an IOTCL_TEST_CONTROL request with the TEST_CONTROL_USE_WWID
flag was not found.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_UNKNOWN_OPERATION
Language     = English
The combination of feature and opcode specified by an IOTCL_TEST_CONTROL request
was not understood by the driver to which it was directed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_OPERATION_CONFLICTS
Language     = English
An IOTCL_TEST_CONTROL request conflicted with one or more active requests in
the targeted test control locus and hence could not be processed.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_NOT_A_DRIVER_LOCUS
Language     = English
An IOTCL_TEST_CONTROL request was targeted to a volume-associated locus when
it should have been targeted to a driver-associated locus.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_ERROR_NOT_A_VOLUME_LOCUS
Language     = English
An IOTCL_TEST_CONTROL request was targeted to a driver-associated locus when
it should have been targeted to a volume-associated locus.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_TCTL_ERROR_EXCLUSIVE_MARK_MET_EXISTING
Language     = English
The attempt to set an exclusive Tctl mark failed because it conflicted
with an existing mark.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: Test code only
;//
;// Severity: Error
;//
;// Symptom of Problem:
;//
MessageId    = +1
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_TCTL_ERROR_MARK_MET_EXISTING_EXCLUSIVE
Language     = English
The attempt to set a Tctl mark failed because it conflicted with
an existing exclusive mark.
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
;// Introduced In: R32
;//
;// Usage: Template
;//
;// Severity: Critical
;//
;// Symptom of Problem:
;//
MessageId    = 0xC000
Severity     = Error
Facility     = TestControl
SymbolicName = TEST_CONTROL_CRITICAL_TEMPLATE
Language     = English
(template)
.



;#endif //__TEST_CONTROL_MESSAGES__
