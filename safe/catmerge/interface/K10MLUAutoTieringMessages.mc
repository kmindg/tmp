;//++
;// Copyright (C) EMC Corporation, 2009
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            K10MLUAutoTieringMessages.mc
;//
;// Description:     This file contains the message catalog for
;//                  the MLU portion of the Auto Tiering functionality.
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
;#ifndef __K10_MLU_AUTO_TIERING_MESSAGES__
;#define __K10_MLU_AUTO_TIERING_MESSAGES__
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

;//========================
;//======= INFO MSGS ======
;//========================
;//
;//

;//=========================================
;//======= START MLU Driver INFO MSGS ======
;//=========================================
;//
;// MLU Driver "Information" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: Template
;//
MessageId    = 0x0000
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_INFO_TEMPLATE
Language     = English
Only for template
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Template
;//
MessageId    = +1
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_INFO_TEMPLATE_2
Language     = English
Second template to show how autoincrement works in MessageId.
.

;//=========================================
;//======= END MLU Driver INFO MSGS ========
;//=========================================
;//
;//==========================================
;//======= START Tier Admin INFO MSGS =======
;//==========================================
;//
;// Tier Admin "Information" messages.
;//


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Informational
;//
;// Symptom of Problem: Tier Admin Info template
;//
MessageId    = 0x0200
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TIER_ADMIN_INFO_TEMPLATE
Language     = English
Not used. Info Template to show how messageId is used to move to next 'component'
.

;//==========================================
;//======= END Tier Admin INFO MSGS ========
;//==========================================

;//===========================
;//======= WARNING MSGS ======
;//===========================
;//
;//

;//============================================
;//======= START MLU Driver WARNING MSGS ======
;//============================================
;//
;// MLU Driver "Warning" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
MessageId    = 0x4000
Severity     = Warning
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_WARNING_TEMPLATE
Language     = English
Only for template
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Template
;//
MessageId    = +1
Severity     = Warning
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_WARNING_TEMPLATE_2
Language     = English
Second template to show how autoincrement works in MessageId.
.

;//============================================
;//======= END MLU Driver WARNING MSGS ========
;//============================================
;//

;//=============================================
;//======= START Tier Admin WARNING MSGS =======
;//=============================================
;//
;// Tier Admin "Warning" messages.
;//


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Warning
;//
;// Symptom of Problem: Tier Admin Warning template
;//
MessageId    = 0x4200
Severity     = Warning
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TIER_ADMIN_WARNING_TEMPLATE
Language     = English
Not used. Warning Template to show how messageId is used to move to next 'component'
.

;//============================================
;//======= END Tier Admin WARNING MSGS ========
;//============================================

;//=========================
;//======= ERROR MSGS ======
;//=========================
;//
;//

;//==========================================
;//======= START MLU Driver ERROR MSGS ======
;//==========================================
;//
;// MLU Driver "Error" messages.
;//

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts are not available
;//     because slice accounting was not enabled for this FLU.
;//
MessageId    = 0x8000
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_NO_COUNTS_NOT_ENABLED
Language     = English
Slice activity counts are not available because slice accounting was
not enabled for this FLU.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts are not available
;//     because insufficient memory was available.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_NO_COUNTS_INSUFFICIENT_MEMORY
Language     = English
Slice activity counts are not available because insufficient memory
was available.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts are not available
;//     because of an internal problem with conversion preparation.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_NO_COUNTS_CONVERSION_PROBLEM
Language     = English
Slice activity counts are not available because of an internal problem
with conversion preparation.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts cannot be set because
;//     the given slice layout is incorrect.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_INCORRECT_SLICE_LAYOUT
Language     = English
Slice activity counts cannot be set because the given slice layout is
incorrect.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts cannot be set because
;//     the given file system ownership for a slice is incorrect.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_INCORRECT_FILE_SYSTEM_OWNER
Language     = English
Slice activity counts cannot be set because the given file system
ownership for a slice is incorrect.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts are not available because
;//     the FLU was not (able to be) registered (with the slice accountant).
;//
MessageId    = 0x8005
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_NO_COUNTS_UNREGISTERED_FLU
Language     = English
Slice activity counts are not available because of a problem with this FLU.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts are not available because
;//     the FLU slice layout was missing or invalid.
;//
MessageId    = 0x8006
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_NO_COUNTS_INVALID_SLICE_LAYOUT
Language     = English
Slice activity counts are not available because of an internal problem with the slice layout for this FLU.
.

;// --------------------------------------------------
;// Introduced In: R32
;//
;// Usage: IOCTL error code
;//
;// Severity: Error
;//
;// Symptom of Problem: Slice activity counts are not available because
;//     they were not requested.
;//
MessageId    = 0x8007
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_MLU_ERROR_NO_COUNTS_NOT_REQUESTED
Language     = English
Slice activity counts are not available because they were not requested.
.

;//==========================================
;//======= END MLU Driver ERROR MSGS ========
;//==========================================
;//

;//===========================================
;//======= START Tier Admin ERROR MSGS =======
;//===========================================
;//
;// Tier Admin "Error" messages.
;//


;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Tier Admin Error template
;//
MessageId    = 0x8200
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TIER_ADMIN_ERROR_TEMPLATE
Language     = English
Not used. Error Template to show how messageId is used to move to next 'component'
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: Tier Admin version mismatch.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TIER_ADMIN_VERSION_MISMATCH
Language     = English
An internal error occurred. Tier Admin and MLU driver Version mismatched.
Please contact your service provider.
Internal information only. Can not get Base entry from ToC file.
.

;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage: For Event Log Only
;//
;// Severity: Error
;//
;// Symptom of Problem: IOCTL output buffer length doesn't match bytes returned.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TIER_ADMIN_IOCTL_BUFFER_LENGTH_MISMATCH
Language     = English
An internal error occurred. IOCTL output buffer length doesn't match bytes returned from MLU driver 
Please contact your service provider.
Internal information only. Can not get Base entry from ToC file.
.

;//==========================================
;//======= END Tier Admin ERROR MSGS ========
;//==========================================


;#endif //__K10_MLU_AUTO_TIERING_MESSAGES__
