;//++
;// Copyright (C) EMC Corporation, 2009
;// All rights reserved.
;// Licensed material -- property of EMC Corporation
;//--
;//
;//++
;// File:            K10TDLAutoTieringMessages.mc
;//
;// Description:     This file contains the message catalog for
;//                  the Tier Definition Library portion of the Auto Tiering functionality.
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
;#ifndef __K10_TDL_AUTO_TIERING_MESSAGES__
;#define __K10_TDL_AUTO_TIERING_MESSAGES__
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


;//======================================================
;//======= START Tier Definition Library INFO MSGS ======
;//======================================================
;//
;// Tier Definition Library "Information" messages.
;//

;//
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Informational
;//
;// Symptom of Problem: TDL Info Template
;//
MessageId    = 0x0600
Severity     = Informational
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TDL_INFO_TEMPLATE
Language     = English
Not used - template for TDL info message
.

;//======================================================
;//======= END Tier Definition Library INFO MSGS ========
;//======================================================
;//


;//===========================
;//======= WARNING MSGS ======
;//===========================
;//
;//
;//=========================================================
;//======= START Tier Definition Library WARNING MSGS ======
;//=========================================================
;//
;// Tier Definition Library "Warning" messages.
;//

;//
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Warning
;//
;// Symptom of Problem: TDL Info Template
;//
MessageId    = 0x4600
Severity     = Warning
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TDL_WARNING_TEMPLATE
Language     = English
Not used - template for TDL Warning message
.

;//=========================================================
;//======= END Tier Definition Library WARNING MSGS ========
;//=========================================================
;//


;//=========================
;//======= ERROR MSGS ======
;//=========================
;//
;//

;//==========================================
;//======= END Tier Admin ERROR MSGS ========
;//==========================================

;//=======================================================
;//======= START Tier Definition Library ERROR MSGS ======
;//=======================================================
;//
;// Tier Definition Library "Error" messages.
;//

;//
;// --------------------------------------------------
;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Error
;//
;// Symptom of Problem: A client of the TDL has passed in
;// an invalid drive type that doesn't fall in the range 
;// of drive types from the list in drive_type.h
;//
MessageId    = 0x8600
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TDL_INVALID_DRIVE_TYPE
Language     = English
The drive type does not exist in the list of supported drive types.
.

;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Error
;//
;// Symptom of Problem: A client of the TDL has passed in
;// an invalid raid type that doesn't fall in the range 
;// of raid types from the list in raid_type.h
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TDL_INVALID_RAID_TYPE
Language     = English
The RAID type does not exist in the list of supported RAID types.
.

;// Introduced In: R30
;//
;// Usage:
;//
;// Severity: Error
;//
;// Symptom of Problem: A client of the TDL has passed in
;// an invalid index into the list of tier names.
;//
MessageId    = +1
Severity     = Error
Facility     = AutoTiering
SymbolicName = AUTO_TIERING_TDL_INVALID_TIER_INDEX
Language     = English
Invalid coarse tier index.
.


;//=======================================================
;//======= END Tier Definition Library ERROR MSGS ========
;//=======================================================
;//

;#endif //__K10_TDL_AUTO_TIERING_MESSAGES__
