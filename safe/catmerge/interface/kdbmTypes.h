#ifndef _KDBM_TYPES_H_
#define _KDBM_TYPES_H_
//=== File Prolog ==============================================================
//--- Copyright ----------------------------------------------------------------
//    Copyright (C) EMC Corporation 2000-2005
//    All rights reserved.
//    Licensed material -- property of EMC Corporation
//
//--- Contents -----------------------------------------------------------------
// KDBM_CURRENT_VERSION
// KDBM_DATABASE_VERIFY_BREADTH
// KDBM_DATABASE_VERIFY_BREADTH_TABLE_METRICS
// KDBM_DATABASE_VERIFY_BREADTH_TABLES
// KDBM_DATABASE_VERIFY_BREADTH_TABLES_AND_DB_APPENDIX
// KDBM_DATABASE_NAME_LEN_IN_CHARS
// KDBM_TABLE_NAME_LEN_IN_CHARS
// KDBM_MAX_DATABASE_SIZE_IN_BYTES
// KDBM_MAX_TABLES_PER_DATABASE
// KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES
// KDBM_TABLE_APPENDIX_SIZE_IN_BYTES
// KDBM_TRANSACTION_LIFESPAN_IMMORTAL
// KDBM_ALLOC_TAG
// KDBM_DATABASE_HANDLE
// KDBM_TABLE_SPEC
//
//--- Description --------------------------------------------------------------
//    This header file defines the data structures used in the Kernel 
//    DataBase Manager.  Do not include this file directly in kernel space
//    programs; include kdbm.h instead.  This file must have only those data
//    types that are common to both kernel and user space.
// 
//--- Comments -----------------------------------------------------------------
//    Anything relevant about this class, including document references, 
//    assumptions, constraints, restrictions, abnormal termination conditions, 
//    etc. 
//
//--- Development History ------------------------------------------------------
//    1/31/2006        MWagner        Created
//
//
//--- Warning ------------------------------------------------------------------
//    This source code follows the Goddard Space Flight Center "Software and 
//    Automations Systems Branch C++ Style Guide, Version 2.0" (DSTL-96-011).
//
//    In addition to that Guide, this source code relaxes the naming formats as
//    follows (see Section 2.3.1):
//    Types             : As alternative for Types (but not Classes or 
//                        Functions), Capitalize every letter, and separate 
//                        words with under underscores. 
//                        FOO_BAR_HEAD fbh;
//
//    In addition to that Guide, this source code restricts the naming formats
//    as follows (see Section 2.3.1):
//    Method Parameters : Capitalize the first letter of each Word 
//                        (PascalStyle). If a local variable would have the 
//                        same name as a method parameter, prepend "lcl" to 
//                        the local variable.
//                        foo (BAR_TYPE Bar)
//                        {
//                            BAR_TYPE lclBar;
//                        }
//
//    In addition to that Guide, this source code restricts Type Conversions 
//    and Casting see Section 5.5):
//    Casting           : Casts must use C++ casts (in order of preference): 
//                        <static_cast>, <dynamic_cast> , <reinterpret_cast>, 
//                        or <const_cast>. 
//
//    Please follow these guidelines when modifying this source code.
//
//=== End File Prolog ==========================================================

#include "csx_ext.h"

//------------------------------------------------------------------------------
// HACK ALERT!!!
// To make this file work in both user and kernel spaces, MAXULONG has to be
// redefined, as user space somehow does not find it.
//
//------------------------------------------------------------------------------
#ifndef MAXULONG
#define MAXULONG    0xffffffff
#endif

//#define KDBM_TRACE_INTERFACE
//#define KDBM_TRACE_CONSERVATOR
//#define KDBM_TRACE_CACHE
//#define KDBM_TRACE_SCRIB
//#define KDBM_TRACE_VERIFY

//------------------------------------------------------------------------------
// Enumerations
//------------------------------------------------------------------------------
//--- Enumeration Prolog -------------------------------------------------------
// Name: KDBM_DATABASE_VERIFY_BREADTH
//
// Description:
// This enumeration is used to specify the breadth of information verified in
// KDBM_Verify,
//
// Note that each successive value is a superset of the previous value "Tables"
// includes "Table Metrics" and "Tables and DB Appendix" includes Tables. 
//
// Members:
// KDBM_DATABASE_VERIFY_BREADTH_TABLE_METRICS - for each specified Table, the
// size and number of Record in that table will be verified.
// KDBM_DATABASE_VERIFY_BREADTH_TABLES - for each specified Table, the full
// (metrics and name) Table Specification will be verified against the stored
// Data-base Table. 
// KDBM_DATABASE_VERIFY_BREADTH_TABLES_AND_DB_APPENDIX - the full Table
// Specification (metrics and name) will be verified against the stored 
// Database Table, and the Database Appendix will be verified against the 
// stored Database Appendix.
//
//--- End Enumeration Prolog ---------------------------------------------------

typedef enum _KDBM_DATABASE_VERIFY_BREADTH
{
    KDBM_DATABASE_VERIFY_BREADTH_INVALID = -1,
    KDBM_DATABASE_VERIFY_BREADTH_TABLE_METRICS,
    KDBM_DATABASE_VERIFY_BREADTH_TABLES,
    KDBM_DATABASE_VERIFY_BREADTH_TABLES_AND_DB_APPENDIX,
    KDBM_DATABASE_VERIFY_BREADTH_TABLES_MAX
} KDBM_DATABASE_VERIFY_BREADTH;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
// --- Constant Prolog ---------------------------------------------------------

// Name: KDBM_DATABASE_NAME_LEN_IN_CHARS 
//
// Description:
// The maximum length of a Database Name in WCHARS (excluding the terminating 
// NULL).
//
// Comments:
// This length allows KDBM to prepend "KDBM_" to the beginning of the Database 
// Name and not exceed the PSM Data Area Name Length. This prefix is only a 
// mnemonic for analysis.
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_DATABASE_NAME_LEN_IN_CHARS  = 42;

#else

    #define KDBM_DATABASE_NAME_LEN_IN_CHARS  42 

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_TABLE_NAME_LEN_IN_CHARS 
//
// Description
// The maximum length of a Table Name in WCHARS (excluding the terminating 
// NULL).
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_TABLE_NAME_LEN_IN_CHARS  = 31;

#else

#define KDBM_TABLE_NAME_LEN_IN_CHARS        31

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_MAX_DATABASE_SIZE_IN_BYTES 
//
// Description:
// The largest Database supported by KDBM.
//
// Comments:
// This is one less than MAXULONG to allow MAXULONG to be used as an 
// invalid size;
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_MAX_DATABASE_SIZE_IN_BYTES = (MAXULONG - 1);

#else

    #define KDBM_MAX_DATABASE_SIZE_IN_BYTES (MAXULONG - 1)

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_MAX_TABLES_PER_DATABASE 
//
// Description:
// The maximum number of Tables in a KDBM Database
//
// Comments:
// Each Table Entry in the Schema uses 16 bytes. 31 Tables (and a header) will 
// all fit into the first Sector of the Database.
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_MAX_TABLES_PER_DATABASE = 127;

#else

    #define KDBM_MAX_TABLES_PER_DATABASE 127

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES 
//
// Description:
// The size of the Appendix KDBM stores for each Database.
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES = 8;

#else

    #define KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES  8

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_TABLE_APPENDIX_SIZE_IN_BYTES 
//
// Description:
// The size of the Appendix KDBM stores for each Table.
//
// Comments:
// The Table Appendix and the Table Name fit exactly into one Sector. Since 
// the Schema occupies exactly one Sector, the Table Metadata follows the 
// Schema, this impliees that each Record of Table Metadata will also be 
// Sector aligned.
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_TABLE_METADATA_SIZE          = 512;
const UINT32 KDBM_TABLE_NAME_SIZE              = (KDBM_TABLE_NAME_LEN_IN_CHARS + 1) * 
                                                sizeof(NTWCHAR);
const UINT32 KDBM_TABLE_APPENDIX_HEADER_SIZE   = sizeof(UINT32);
const UINT32 KDBM_TABLE_APPENDIX_SIZE_IN_BYTES = KDBM_TABLE_METADATA_SIZE - 
                                                (KDBM_TABLE_NAME_SIZE  + KDBM_TABLE_APPENDIX_HEADER_SIZE);

#else

    #define KDBM_TABLE_METADATA_SIZE                  (512)
    #define KDBM_TABLE_NAME_SIZE              ((KDBM_TABLE_NAME_LEN_IN_CHARS + 1) \
                                                                * sizeof(NTWCHAR))
    #define KDBM_TABLE_APPENDIX_HEADER_SIZE   (sizeof(UINT32))
    #define KDBM_TABLE_APPENDIX_SIZE_IN_BYTES (KDBM_TABLE_METADATA_SIZE -          \
                      (KDBM_TABLE_NAME_SIZE  + KDBM_TABLE_APPENDIX_HEADER_SIZE))

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_MIN_RECORD_CACHE_SIZE_IN_BYTES 
//
// Description:
// The minimum Record Cache Size.
// 
// This must be at least 512 bytes - the size of a Schema. 
//
// 64K is a completely arbitrary number
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_MIN_RECORD_CACHE_SIZE_IN_BYTES = (1024 * 64);

#else

    #define KDBM_MIN_RECORD_CACHE_SIZE_IN_BYTES (1024 * 64)

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_TRANSACTION_LIFESPAN_IMMORTAL 
//
// Description:
// Allows a Transaction to live forever.
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_TRANSACTION_LIFESPAN_IMMORTAL = MAXULONG;

#else

    #define KDBM_TRANSACTION_LIFESPAN_IMMORTAL MAXULONG

#endif

// --- Constant Prolog ---------------------------------------------------------
// Name: KDBM_ALLOC_TAG 
//
// Description:
// Memory Tag for Debugging
//
// --- End Constant Prolog -----------------------------------------------------
#ifdef __cplusplus

const UINT32 KDBM_ALLOC_TAG = 'mbdk';

#else

    #define KDBM_ALLOC_TAG  (UINT32) 'mbdk';

#endif

//------------------------------------------------------------------------------
// Structures/Classes
//------------------------------------------------------------------------------
//--- Structure Prolog ---------------------------------------------------------
// Name: KDBM_DATABASE_HANDLE
//
// Description:
//
// An opaque data type returned from KDBM_Attach(), used for all subsequent 
// access to the Attached Database.
//
// Comments:
// KDBM_Attach() will never return a handle with all fields set to zero, so the 
// following initialization will not create a possible valid Database Handle.
//
//      KDBM_DATBASE_HANDLE handle = {0};
//
//--- End Structure Prolog -----------------------------------------------------
typedef struct _KDBM_DATABASE_HANDLE
{
    UINT32  fPrivateData[3];
    csx_u64_t  fWhatAreYouLookingAt;
}KDBM_DATABASE_HANDLE, * PKDBM_DATABASE_HANDLE;

//--- Structure Prolog ---------------------------------------------------------
// Name: KDBM_TABLE_SPEC
//
// Description
// This structure specifies a KDBM Table.
//
// Members:
// RecordSize the size in Bytes of each Record in the Table.
// NumberofRecords the number of Records in the Tables
// TableName a Wide Character string that specifies the name of the Table. 
// This string must be less than or equal to KDBM_TABLE_NAME_LEN_IN_CHARS 
// Wide Characters long.
//
// Comments:
// KDBM stores the Table Name for debugging and tracing purposes only. KDBM 
// operations identify Tables by Table Index only.

//--- End Structure Prolog -----------------------------------------------------
typedef struct _KDBM_TABLE_SPEC
{
    UINT32                           SizeOfRecords;
    UINT32                           NumberOfRecords;
    CHAR                             TableName[KDBM_TABLE_NAME_LEN_IN_CHARS];
}KDBM_TABLE_SPEC, *PKDBM_TABLE_SPEC ;

#endif //_KDBM_TYPES_H_
