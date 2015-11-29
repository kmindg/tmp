;//++
;// Copyright (C) EMC Corporation, 2006 - 2007
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;
;//++
;// File:            K10TransportManagerMessages.h (MC)
;//
;// Description:     This file contains K10 Transport manager  codes.
;//
;// The CLARiiON standard for how to classify errors according to their range
;// is as follows:
;//
;// "Information"       0x0000-0x3FFF
;// "Warning"           0x4000-0x7FFF
;// "Error"             0x8000-0xBFFF
;// "Critical"          0xC000-0xFFFF
;//
;//  Author(s):     
;//
;// History:
;//                 
;//--
;
;#ifndef __K10TRANSPORT_MANAGER_MESSAGES__
;#define __K10TRANSPORT_MANAGER_MESSAGES__
;

MessageIdTypedef = EMCPAL_STATUS


FacilityNames = ( K10TransportManager           = 0x161         : K10TRANSPORT_MANAGER )


SeverityNames = (
                  Success       = 0x0           : STATUS_SEVERITY_SUCCESS
                  Informational = 0x1           : STATUS_SEVERITY_INFORMATIONAL
                  Warning       = 0x2           : STATUS_SEVERITY_WARNING
                  Error         = 0x3           : STATUS_SEVERITY_ERROR
                )


;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Severity: Error
;//
;// Usage: Returned to K10TM clients
;//
MessageId    = 0x8000
Severity     = Error
Facility     = K10TransportManager
SymbolicName = K10TM_REGISTRATION_FAILED
Language     = English
.

;
;#define K10TM_ADMIN_STATUS_REGISTRATION_FAILED         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10TM_REGISTRATION_FAILED)

;//------------------------------------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Severity: Error
;//
;// Usage: Returned to K10TM clients
;//
MessageId    = +1
Severity     = Error
Facility     = K10TransportManager
SymbolicName = K10TM_PRE_INITIALIZATION_FAILED
Language     = English
.

;
;#define K10TM_ADMIN_STATUS_PRE_INITIALIZATION_FAILED        \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(K10TM_PRE_INITIALIZATION_FAILED)

;//------------------------------------------------------------------------------------------------------------
MessageIdTypedef = EMCPAL_STATUS

;
;#endif // __K10TRANSPORT_MANAGER_MESSAGES__
;

