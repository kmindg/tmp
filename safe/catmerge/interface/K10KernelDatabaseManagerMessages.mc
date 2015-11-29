;//++
;// Copyright (C) Data General Corporation, 2000
;// All rights reserved.
;// Licensed material -- property of Data General Corporation
;//--
;
;#ifndef K10_KERNEL_DATABASE_MANAGER_MESSAGES_H
;#define K10_KERNEL_DATABASE_MANAGER_MESSAGES_H
;
;//
;//++
;// File:            K10PersistentStorageManagerMessages.h (MC)
;//
;// Description:     This file defines KDBM Status Codes and
;//                  messages. Each Status Code has two forms,
;//                  an internal status and as admin status:
;//                  KDBM_xxx and KDBM_ADMIN_xxx.
;//
;// History:         03-Mar-00       MWagner     Created
;//                  11-Nov-07       Sri         Added KDBM_WARNING_NO_SUCH_DATABASE.
;//--
;#include "k10defs.h" 
;

MessageIdTypedef= EMCPAL_STATUS

FacilityNames   = ( Kdbm= 0x11A : FACILITY_KDBM )

SeverityNames= ( Success       = 0x0 : STATUS_SEVERITY_SUCCESS
                  Informational= 0x1 : STATUS_SEVERITY_INFORMATIONAL
                  Warning      = 0x2 : STATUS_SEVERITY_WARNING
                  Error        = 0x3 : STATUS_SEVERITY_ERROR
                )

;//-----------------------------------------------------------------------------
;//  Info Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x0001
Severity        = Informational
Facility        = Kdbm
SymbolicName    = KDBM_INFO_GENERIC
Language        = English
Generic KDBM Information.
.

;
;#define KDBM_ADMIN_INFO_GENERIC                                                \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_INFO_GENERIC)
;

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: Client is expanding a Table to its current size...
;//
MessageId       = +1
Severity        = Informational
Facility        = Kdbm
SymbolicName    = KDBM_INFO_TABLE_EXPAND_NO_OP
Language        = English
.

;
;#define KDBM_ADMIN_INFO_TABLE_EXPAND_NO_OP                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_INFO_TABLE_EXPAND_NO_OP)
;
;//------------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;//  Warning Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x4000
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_GENERIC
Language        = English
Generic KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_GENERIC                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_GENERIC)
;
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning
;//
;// Symptom of Problem: Client is calling an operation not yet supported by KDBM
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_OPERATION_NOT_SUPPORTED
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_OPERATION_NOT_SUPPORTED                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_OPERATION_NOT_SUPPORTED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning - will BugCheck in DBG builds
;//
;// Symptom of Problem: Client is trying to create a Datbase that already exists
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_CREATE_DATABASE_EXISTS
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_CREATE_DATABASE_EXISTS                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_CREATE_DATABASE_EXISTS)
;
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning 
;//
;// Symptom of Problem: The Client asked KDBM to Verify the Metrics of the Tables
;//                     of the Tables in a Database, and the Metrics did not
;//                     match the stored Metrics.
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_DATABASE_VERIFY_TABLE_METRICS_FAILED
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_DATABASE_VERIFY_TABLE_METRICS_FAILED                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_DATABASE_VERIFY_TABLE_METRICS_FAILED)
;
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning 
;//
;// Symptom of Problem: The Client asked KDBM to Verify a set of Tables in a
;//                     Database against an array of Table Specifications. At
;//                     least one Table did not match its specifcation.
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_DATABASE_VERIFY_TABLES_FAILED
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_DATABASE_VERIFY_TABLES_FAILED                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_DATABASE_VERIFY_TABLES_FAILED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning 
;//
;// Symptom of Problem: The Client asked KDBM to Verify a set of Tables in a
;//                     Database against an array of Table Specifications, and
;//                     also check the Database Appendix. The Tables passed but
;//                     the Appendix failed.
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_DATABASE_VERIFY_TABLES_AND_APPENDIX_FAILED
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_DATABASE_VERIFY_TABLES_AND_APPENDIX_FAILED                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_DATABASE_VERIFY_TABLES_AND_APPENDIX_FAILED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning 
;//
;// Symptom of Problem: The Client asked KDBM to Expand a Table to a smaller size.
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_TABLE_SHRINK_NOT_SUPPORTED
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_TABLE_SHRINK_NOT_SUPPORTED                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_TABLE_SHRINK_NOT_SUPPORTED)

;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning 
;//
;// Symptom of Problem: The Client asked KDBM to Add a Table where one already
;//                     exists.
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_ADD_TABLE_OVERWRITES_EXISTING_TABLE
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_ADD_TABLE_OVERWRITES_EXISTING_TABLE                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_ADD_TABLE_OVERWRITES_EXISTING_TABLE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Warning 
;//
;// Symptom of Problem: The Client asked KDBM to open a non-existent database.
;//
MessageId       = +1
Severity        = Warning
Facility        = Kdbm
SymbolicName    = KDBM_WARNING_NO_SUCH_DATABASE
Language        = English
Internal KDBM Warning.
.

;
;#define KDBM_ADMIN_WARNING_NO_SUCH_DATABASE                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_WARNING_NO_SUCH_DATABASE)
;
;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;//  Error Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0x8000
Severity        = Error
Facility        = Kdbm
SymbolicName    = KDBM_ERROR_GENERIC
Language        = English
Generic KDBM Error Code.
.

;
;#define KDBM_ADMIN_ERROR_GENERIC                                               \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_GENERIC)
;                              
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a client has passed an
;//                     KDBM an invalid Database Handle.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_HANDLE_NOT_FOUND
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_ERROR_HANDLE_NOT_FOUND \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_HANDLE_NOT_FOUND)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - KDBM has discovered a 
;//                     Schema that doesn't make sense.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_SCHEMA_INSANE
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_ERROR_SCHEMA_INSANE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_SCHEMA_INSANE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Rockies
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - KDBM has discovered a 
;//                     Storage version that cannot be processed by the
;//                     current version of KDBM.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_STORAGE_UNKNOWN_VERSION
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_STORAGE_UNKNOWN_VERSION \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_STORAGE_UNKNOWN_VERSION)
;

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - KDBM has discovered a 
;//                     Schema that has the wrong version, and KDBM cannot
;//                     parse it.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_SCHEMA_UNKNOWN_VERSION
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_SCHEMA_UNKNOWN_VERSION \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_SCHEMA_UNKNOWN_VERSION)
;

;//-----------------------------------------------------------------------------
;
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - KDBM has discovered a 
;//                     that the Record Cache Size is not large enough
;//                     to hold some Record in some Table in the Database.
;// 
;//                     Note that if this conidtion is discoverd in Attach, 
;//                     it is a WARNING. If it's discovered  in Start Transaction,
;//                     it's an error.
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_RECORD_CACHE_TOO_SMALL
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_RECORD_CACHE_TOO_SMALL \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_RECORD_CACHE_TOO_SMALL)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client has called
;//                     a function that reequires the Database to be
;//                     DETACHED when it's not in that State.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_NOT_DETACHED
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_NOT_DETACHED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_NOT_DETACHED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client has called
;//                     a function that reequires the Database to be
;//                     ATTACHED when it's not in that State.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_NOT_ATTACHED
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_NOT_ATTACHED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_NOT_ATTACHED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - the Database was Tainted
;//                     by an earlier put error.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_TAINTED
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_TAINTED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_TAINTED)
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a Transcation exceeded its
;//                     specified time, and the Physical Database was closed.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_TRANSACTION_EXPIRED
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_TRANSACTION_EXPIRED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_TRANSACTION_EXPIRED)
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client has called
;//                     a function that reequires the Database to be
;//                     TRANSACTING when it's not in that State.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_NOT_TRANSACTING
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_NOT_TRANSACTING \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_NOT_TRANSACTING)
;
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client is
;//                     trying to manipulate a Table that does not exist
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_TABLE_DOES_NOT_EXIST
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_TABLE_DOES_NOT_EXIST \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_TABLE_DOES_NOT_EXIST)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client is
;//                     trying to manipulate a Table that can not
;//                     possibly exist
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_TABLE_INDEX_OUT_OF_RANGE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client is
;//                     trying to manipulate a Record that is beyond
;//                     the end of a Table
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_RECORD_INDEX_OUT_OF_RANGE
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_RECORD_INDEX_OUT_OF_RANGE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_RECORD_INDEX_OUT_OF_RANGE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client is
;//                     trying to write more Data to a Record than the
;//                     Record can hold
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATA_SIZE_LARGER_THAN_RECORD_SIZE
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATA_SIZE_LARGER_THAN_RECORD_SIZE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATA_SIZE_LARGER_THAN_RECORD_SIZE)
;
;//-----------------------------------------------------------------------------
;
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client has called
;//                     an operation that would create a or grow a Database 
;//                     larger than is permitted.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_TOO_LARGE
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_TOO_LARGE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_TOO_LARGE)
;
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client is
;//                     trying to read/write too much Data from/to
;//                     the Database Appendix.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_DATABASE_APPENDIX_TOO_LARGE
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_DATABASE_APPENDIX_TOO_LARGE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_DATABASE_APPENDIX_TOO_LARGE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error
;//
;// Symptom of Problem: This is an internal error - a KDBM Client is
;//                     trying to read/write too much Data from/to
;//                     the Table Appendix.
;// 
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_ERROR_TABLE_APPENDIX_TOO_LARGE
Language        = English
KDBM internal error.
.
;
;#define KDBM_ADMIN_ERROR_TABLE_APPENDIX_TOO_LARGE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_TABLE_APPENDIX_TOO_LARGE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error 
;//
;// Symptom of Problem: The Client asked KDBM to update a DB to new KDBM 
;//                     Version though a KDBM_PutCompatibilityMode(),
;//                     and KDNM cannot convert the exsting DB to the new 
;//                     Version.
;//
MessageId       = +1
Severity        = Error
Facility        = Kdbm
SymbolicName    = KDBM_ERROR_PUT_STORAGE_VERSION_MISMATCH
Language        = English
Internal KDBM Error.
.

;
;#define KDBM_ADMIN_ERROR_PUT_STORAGE_VERSION_MISMATCH                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_PUT_STORAGE_VERSION_MISMATCH)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Error 
;//
;// Symptom of Problem: The Client asked KDBM to Add a Table where one already
;//                     exists.
;//
MessageId       = +1
Severity        = Error
Facility        = Kdbm
SymbolicName    = KDBM_ERROR_ADD_TABLE_OVERWRITES_EXISTING_TABLE
Language        = English
Internal KDBM Error.
.

;
;#define KDBM_ADMIN_ERROR_ADD_TABLE_OVERWRITES_EXISTING_TABLE                                      \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_ERROR_ADD_TABLE_OVERWRITES_EXISTING_TABLE)
;
;//-----------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;//  Critical Error Status Codes
;//-----------------------------------------------------------------------------
MessageId       = 0xC000
Severity        = Error
Facility        = Kdbm
SymbolicName    = KDBM_CRITICAL_ERROR_GENERIC
Language        = English
Generic KDBM Critical Error Code.
.

;
;#define KDBM_ADMIN_CRITICAL_ERROR_GENERIC         \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_CRITICAL_ERROR_GENERIC)
;
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - KDBM is releasing a
;//                     Sempahore in error.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_OFFICES_SEMAPHORE_LIMIT_EXCEEDED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_OFFICES_SEMAPHORE_LIMIT_EXCEEDED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_OFFICES_SEMAPHORE_LIMIT_EXCEEDED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - KDBM is releasing a
;//                     Sempahore in error.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_OFFICE_REFERENCE_COUNT_NEGATVE
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_OFFICE_REFERENCE_COUNT_NEGATVE \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_OFFICE_REFERENCE_COUNT_NEGATVE)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - KDBM is releasing a
;//                     Sempahore in error.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_OPERATION_SEMAPHORE_LIMIT_EXCEEDED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_OPERATION_SEMAPHORE_LIMIT_EXCEEDED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_OPERATION_SEMAPHORE_LIMIT_EXCEEDED)
;
;//-----------------------------------------------------------------------------
;
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - KDBM is releasing a
;//                     Sempahore in error.
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_TRANSACTION_SEMAPHORE_LIMIT_EXCEEDED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_TRANSACTION_SEMAPHORE_LIMIT_EXCEEDED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_TRANSACTION_SEMAPHORE_LIMIT_EXCEEDED)
;
;//-----------------------------------------------------------------------------
;
;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_DIRECTOR_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_DIRECTOR_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_DIRECTOR_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_DIRECTOR_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_DIRECTOR_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_DIRECTOR_IMPL_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_CURATOR_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_CURATOR_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_CURATOR_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_CURATOR_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_CURATOR_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_CURATOR_IMPL_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_CONSERVATOR_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_CONSERVATOR_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_CONSERVATOR_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_CONSERVATOR_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_CONSERVATOR_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_CONSERVATOR_IMPL_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_GUARD_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_GUARD_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_GUARD_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_GUARD_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_GUARD_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_GUARD_IMPL_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_SCHEMA_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_SCHEMA_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_SCHEMA_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_SCHEMA_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_SCHEMA_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_SCHEMA_IMPL_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_SCRIBE_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_SCRIBE_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_SCRIBE_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_SCRIBE_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_SCRIBE_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_SCRIBE_IMPL_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_RECORD_CACHE_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_RECORD_CACHE_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_RECORD_CACHE_DTOR_CERTIFIED)
;
;//-----------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Taurus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_RECORD_CACHE_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_RECORD_CACHE_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_RECORD_CACHE_IMPL_DTOR_CERTIFIED)
;
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Zeus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_PICKET_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_PICKET_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_PICKET_DTOR_CERTIFIED)
;
;//------------------------------------------------------------------------------

;//------------------------------------------------------------------------------
;// Introduced In: Zeus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_PICKET_IMPL_DTOR_CERTIFIED
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_PICKET_IMPL_DTOR_CERTIFIED \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_PICKET_IMPL_DTOR_CERTIFIED)
;
;//------------------------------------------------------------------------------

;//-----------------------------------------------------------------------------
;// Introduced In: Zeus
;//
;// Usage:              Internal
;//
;// Severity:           Critical Error (BugCheck)
;//
;// Symptom of Problem: This is an internal error - this BugCheck is only
;//                     called in DBG builds
;//
;//
MessageId       = +1
Severity        = Error
Facility        = KDBM
SymbolicName    = KDBM_BUGCHECK_PICKET_LOCK_CALLBACK_UNEXPECTED_EVENT
Language        = English
KDBM internal error.
.

;
;#define KDBM_ADMIN_BUGCHECK_PICKET_LOCK_CALLBACK_UNEXPECTED_EVENT \
;          MAKE_ADMIN_ERROR_FROM_KERNEL_ERROR(KDBM_BUGCHECK_PICKET_LOCK_CALLBACK_UNEXPECTED_EVENT)
;
;//-----------------------------------------------------------------------------
;#endif
