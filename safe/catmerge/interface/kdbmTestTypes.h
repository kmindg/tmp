//***************************************************************************
// Copyright (C) EMC Corporation 2000-2005
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************

#ifndef _KDBMTEST_TYPES_H_
#define _KDBMTEST_TYPES_H_

//++
// File Name:
//      kdbmTestTypes.h
//
// Contents:
//      Device names, IOCTLs, and IN/OUT buffers to talk 
//      to the kdbmTest Driver. The kdbmTest Driver understands
//      these operations:
//          kdbmTest_CreateDB ()
//          kdbmTest_AttachDB ()
//          kdbmTest_DetachDB ()
//          kdbmTest_DeleteDB ()
//          kdbmTest_VerifyDB ()
//          kdbmTest_PutDBAppendix ()
//          kdbmTest_GetDBAppendix ()
//          kdbmTest_AddTable ()
//          kdbmTest_ExpandTable ()
//          kdbmTest_PutTableAppendix ()
//          kdbmTest_GetTableAppendix ()
//          kdbmTest_GetTableMetrics ()
//          kdbmTest_GetNumberOfTablesInUse ()
//          kdbmTest_GetTableName ()
//          kdbmTest_PutRecord ()
//          kdbmTest_GetRecord ()
//          kdbmTest_StartTransaction ()
//          kdbmTest_DiscardTransaction ()
//          kdbmTest_CommitTransaction ()
//
// Constants:
//      KDBMTEST_DRIVER_NAME
//      KDBMTEST_DRIVER_NT_DEVICE_NAME
//      KDBMTEST_DRIVER_WIN32_DEVICE_NAME   
//      KDBMTEST_DRIVER_DOS_DEVICE_NAME 
//      KDBMTEST_FILE_DEVICE
//      KDBMTEST_IOCTL_INDEX_BASE
//      KDBMTEST_CURRENT_API_VERSION
//
// Macros:
//      KDBMTEST_BUILD_IOCTL
//
// Enums:
//      KDBMTEST_OPERATION_INDEX
//
// IOCTLs:
//      IOCTL_KDBMTEST_CREATE_DB
//      IOCTL_KDBMTEST_ATTACH_DB
//      IOCTL_KDBMTEST_DETACH_DB
//      IOCTL_KDBMTEST_DELETE_DB
//      IOCTL_KDBMTEST_VERIFY_DB
//      IOCTL_KDBMTEST_PUT_DB_APPENDIX
//      IOCTL_KDBMTEST_GET_DB_APPENDIX
//      IOCTL_KDBMTEST_ADD_TABLE
//      IOCTL_KDBMTEST_EXPAND_TABLE
//      IOCTL_KDBMTEST_PUT_TABLE_APPENDIX
//      IOCTL_KDBMTEST_GET_TABLE_APPENDIX
//      IOCTL_KDBMTEST_GET_TABLE_METRICS
//      IOCTL_KDBMTEST_GET_TABLE_NAME
//      IOCTL_KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE
//      IOCTL_KDBMTEST_PUT_RECORD
//      IOCTL_KDBMTEST_GET_RECORD
//      IOCTL_KDBMTEST_START_TRANSACTION
//      IOCTL_KDBMTEST_DISCARD_TRANSACTION
//      IOCTL_KDBMTEST_COMMIT_TRANSACTION
//      IOCTL_KDBMTEST_PANIC
//
// Types:
//      KDBMTEST_CREATE_DB_IN_BUFFER
//      KDBMTEST_CREATE_DB_OUT_BUFFER
//      KDBMTEST_ATTACH_DB_IN_BUFFER
//      KDBMTEST_ATTACH_DB_OUT_BUFFER
//      KDBMTEST_DETACH_DB_IN_BUFFER
//      KDBMTEST_DELETE_DB_IN_BUFFER
//      KDBMTEST_VERIFY_DB_IN_BUFFER
//      KDBMTEST_PUT_DB_APPENDIX_IN_BUFFER
//      KDBMTEST_GET_DB_APPENDIX_IN_BUFFER
//      KDBMTEST_GET_DB_APPENDIX_OUT_BUFFER
//      KDBMTEST_ADD_TABLE_IN_BUFFER
//      KDBMTEST_EXPAND_TABLE_IN_BUFFER
//      KDBMTEST_PUT_TABLE_APPENDIX_IN_BUFFER
//      KDBMTEST_GET_TABLE_APPENDIX_IN_BUFFER
//      KDBMTEST_GET_TABLE_APPENDIX_OUT_BUFFER
//      KDBMTEST_GET_TABLE_METRICS_IN_BUFFER
//      KDBMTEST_GET_TABLE_METRICS_OUT_BUFFER
//      KDBMTEST_GET_TABLE_NAME_IN_BUFFER
//      KDBMTEST_GET_TABLE_NAME_OUT_BUFFER
//      KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_IN_BUFFER
//      KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_OUT_BUFFER
//      KDBMTEST_PUT_RECORD_IN_BUFFER
//      KDBMTEST_GET_RECORD_IN_BUFFER
//      KDBMTEST_GET_RECORD_OUT_BUFFER
//      KDBMTEST_START_TRANSACTION_IN_BUFFER
//      KDBMTEST_DISCARD_TRANSACTION_IN_BUFFER
//      KDBMTEST_COMMIT_TRANSACTION_IN_BUFFER
//      KDBMTEST_PANIC_IN_BUFFER
//
// Revision History:
//      16-Mar-06   Sri      Created.
//
//--

#include "k10diskdriveradmin.h"
#include "EmcPAL_Ioctl.h"

//++
// Include files
//--

//
// NOTE: 
//      kdbm.h must be included for kernel space.
//      kdbmTypes.h must be included for user space.
//

//++
//.End Include Files
//--

//++
// Constants
//--

//++
// Constant:
//      KDBMTEST_DRIVER_NAME
//
// Description:
//      The "full" name of kdbmTest
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_DRIVER_NAME    "kdbmTest"
//.End

//++
// Constant:
//      KDBMTEST_DRIVER_NT_DEVICE_NAME
//
// Description:
//      The kdbmTest Driver NT Device name
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_DRIVER_NT_DEVICE_NAME    "\\Device\\" KDBMTEST_DRIVER_NAME 
//.End                                                                        

//++
// Constant:
//      KDBMTEST_DRIVER_WIN32_DEVICE_NAME   
//
// Description:
//      The kdbmTest Driver Win32 Device name
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_DRIVER_WIN32_DEVICE_NAME   "\\\\.\\" KDBMTEST_DRIVER_NAME
//.End                                                                        

//++
// Constant:
//      KDBMTEST_DRIVER_DOS_DEVICE_NAME 
//
// Description:
//      The kdbmTest Driver DOS Device name
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_DRIVER_DOS_DEVICE_NAME    "\\DosDevices\\" KDBMTEST_DRIVER_NAME
//.End                                                                        

//++
// Constant:
//      KDBMTEST_FILE_DEVICE
//
// Description:
//      DEVICE_TYPE for the kdbmTest Driver.
//
//      Note that values used by Microsoft Corp are in the range 0-32767, and
//      32768-65535 are reserved for use by customers.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_FILE_DEVICE             39000
//.End

//++
// Constant:
//      KDBMTEST_IOCTL_INDEX_BASE
//
// Description:
//      IOCTL index values for the kdbmTestDriver.
//
//      Note that function codes 0-2047 are reserved for Microsoft Corp, and
//      2048-4095 are reserved for customers.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_IOCTL_INDEX_BASE        3900
//.End

//++
// Constant:
//      KDBMTEST_API_VERSION_ONE
//
// Description:
//      The version of the kdbmTest IOCTL interface structures.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//      
//--
#define KDBMTEST_API_VERSION_ONE        0x0001
//.End

//++
// Constant:
//      KDBMTEST_CURRENT_API_VERSION
//
// Description:
//      The version of the kdbmTest IOCTL interface structures.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//      
//--
#define KDBMTEST_CURRENT_API_VERSION        KDBMTEST_API_VERSION_ONE
//.End

//++
//.End Constants
//--

//++
// Macros
//--

//++
// Macro:
//      KDBMTEST_BUILD_IOCTL
//
// Description:
//      Builds a kdbmTest Ioctl code
//
//      winioctl.h or ntddk.h must be included before kdbmTestIUserInterface.h
//      to pick up the CTL_CODE() macro.
//
//      Note that function codes 0-2047 are reserved for Microsoft 
//      Corporation, and 2048-4095 are reserved for customers.
//
// Arguments:
//      ULONG   Ioctl_Value
//
// Return Value
//      A kdbmTest Ioctl Code
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define KDBMTEST_BUILD_IOCTL( IoctlValue )                       (\
        EMCPAL_IOCTL_CTL_CODE (KDBMTEST_FILE_DEVICE,                          \
                  ((IoctlValue) + KDBMTEST_IOCTL_INDEX_BASE),    \
                  EMCPAL_IOCTL_METHOD_BUFFERED,                               \
                  EMCPAL_IOCTL_FILE_ANY_ACCESS)                               \
                                                                 )
// .End

//++
//.End Macros
//--

//++
// Enums
//--

//++
// Enum:
//      KDBMTEST_OPERATION_INDEX
//
// Description:
//      The operations implemented by kdbmTest
//
// Members:
//      KDBMTEST_CREATE_DB                          = 1
//      KDBMTEST_ATTACH_DB                          = 2
//      KDBMTEST_DETACH_DB                          = 3
//      KDBMTEST_DELETE_DB                          = 4
//      KDBMTEST_VERIFY_DB                          = 5
//      KDBMTEST_PUT_DB_APPENDIX                    = 6
//      KDBMTEST_GET_DB_APPENDIX                    = 7
//      KDBMTEST_ADD_TABLE                          = 8
//      KDBMTEST_EXPAND_TABLE                       = 9
//      KDBMTEST_PUT_TABLE_APPENDIX                 = 10
//      KDBMTEST_GET_TABLE_APPENDIX                 = 11
//      KDBMTEST_GET_TABLE_METRICS                  = 12
//      KDBMTEST_GET_TABLE_NAME                     = 13
//      KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE        = 14
//      KDBMTEST_PUT_RECORD                         = 15
//      KDBMTEST_GET_RECORD                         = 16
//      KDBMTEST_START_TRANSACTION                  = 17
//      KDBMTEST_DISCARD_TRANSACTION                = 18
//      KDBMTEST_COMMIT_TRANSACTION                 = 19
//      KDBMTEST_PANIC                              = 20
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
typedef enum _KDBMTEST_OPERATION_INDEX
{
    KDBMTEST_CREATE_DB                          = 1,
    KDBMTEST_ATTACH_DB                          = 2,
    KDBMTEST_DETACH_DB                          = 3,
    KDBMTEST_DELETE_DB                          = 4,
    KDBMTEST_VERIFY_DB                          = 5,
    KDBMTEST_PUT_DB_APPENDIX                    = 6,
    KDBMTEST_GET_DB_APPENDIX                    = 7,
    KDBMTEST_ADD_TABLE                          = 8,
    KDBMTEST_EXPAND_TABLE                       = 9,
    KDBMTEST_PUT_TABLE_APPENDIX                 = 10,
    KDBMTEST_GET_TABLE_APPENDIX                 = 11,
    KDBMTEST_GET_TABLE_METRICS                  = 12,
    KDBMTEST_GET_TABLE_NAME                     = 13,
    KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE        = 14,
    KDBMTEST_PUT_RECORD                         = 15,
    KDBMTEST_GET_RECORD                         = 16,
    KDBMTEST_START_TRANSACTION                  = 17,
    KDBMTEST_DISCARD_TRANSACTION                = 18,
    KDBMTEST_COMMIT_TRANSACTION                 = 19,
    KDBMTEST_PANIC                              = 20,
    KDBMTEST_PUT_COMPATIBILITY_MODE             = 22,
    KDBMTEST_GET_COMPATIBILITY_MODE             = 21

    //
    // Add new operations here, not in the middle of the list.
    //

} KDBMTEST_OPERATION_INDEX, *PKDBMTEST_OPERATION_INDEX;
//.End                                                                        

//++
//.End Enums
//--

//++
// IOCTLs
//--

//++
// Constants:
//      IOCTL_KDBMTEST_CREATE_DB
//      IOCTL_KDBMTEST_ATTACH_DB
//      IOCTL_KDBMTEST_DETACH_DB
//      IOCTL_KDBMTEST_DELETE_DB
//      IOCTL_KDBMTEST_VERIFY_DB
//      IOCTL_KDBMTEST_PUT_DB_APPENDIX
//      IOCTL_KDBMTEST_GET_DB_APPENDIX
//      IOCTL_KDBMTEST_ADD_TABLE
//      IOCTL_KDBMTEST_EXPAND_TABLE
//      IOCTL_KDBMTEST_PUT_TABLE_APPENDIX
//      IOCTL_KDBMTEST_GET_TABLE_APPENDIX
//      IOCTL_KDBMTEST_GET_TABLE_METRICS
//      IOCTL_KDBMTEST_GET_TABLE_NAME
//      IOCTL_KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE
//      IOCTL_KDBMTEST_PUT_RECORD
//      IOCTL_KDBMTEST_GET_RECORD
//      IOCTL_KDBMTEST_START_TRANSACTION
//      IOCTL_KDBMTEST_DISCARD_TRANSACTION
//      IOCTL_KDBMTEST_COMMIT_TRANSACTION
//      IOCTL_KDBMTEST_PANIC
//
// Revision History:
//      16-Mar-06   Sri     Created.
//
//--
#define IOCTL_KDBMTEST_CREATE_DB                          \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_CREATE_DB)
#define IOCTL_KDBMTEST_ATTACH_DB                          \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_ATTACH_DB)
#define IOCTL_KDBMTEST_DETACH_DB                          \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_DETACH_DB)
#define IOCTL_KDBMTEST_DELETE_DB                          \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_DELETE_DB)
#define IOCTL_KDBMTEST_VERIFY_DB                          \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_VERIFY_DB)
#define IOCTL_KDBMTEST_PUT_DB_APPENDIX                    \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_PUT_DB_APPENDIX)
#define IOCTL_KDBMTEST_GET_DB_APPENDIX                    \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_DB_APPENDIX)
#define IOCTL_KDBMTEST_ADD_TABLE                          \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_ADD_TABLE)
#define IOCTL_KDBMTEST_EXPAND_TABLE                       \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_EXPAND_TABLE)
#define IOCTL_KDBMTEST_PUT_TABLE_APPENDIX                 \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_PUT_TABLE_APPENDIX)
#define IOCTL_KDBMTEST_GET_TABLE_APPENDIX                 \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_TABLE_APPENDIX)
#define IOCTL_KDBMTEST_GET_TABLE_METRICS                  \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_TABLE_METRICS)
#define IOCTL_KDBMTEST_GET_TABLE_NAME                     \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_TABLE_NAME)
#define IOCTL_KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE        \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE)
#define IOCTL_KDBMTEST_PUT_RECORD                         \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_PUT_RECORD)
#define IOCTL_KDBMTEST_GET_RECORD                         \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_RECORD)
#define IOCTL_KDBMTEST_START_TRANSACTION                  \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_START_TRANSACTION)
#define IOCTL_KDBMTEST_DISCARD_TRANSACTION                \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_DISCARD_TRANSACTION)
#define IOCTL_KDBMTEST_COMMIT_TRANSACTION                 \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_COMMIT_TRANSACTION)
#define IOCTL_KDBMTEST_PANIC                 \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_PANIC)
#define IOCTL_KDBMTEST_GET_COMPATIBILITY_MODE                 \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_GET_COMPATIBILITY_MODE)
#define IOCTL_KDBMTEST_PUT_COMPATIBILITY_MODE                 \
    KDBMTEST_BUILD_IOCTL (KDBMTEST_PUT_COMPATIBILITY_MODE)

//++
//.End IOCTLS
//--

//++
// Types
//--

//++
// Type:
//      KDBMTEST_CREATE_DB_IN_BUFFER
//
// Description:
//      The input buffer for a CreateDB() operation, used in conjuction with
//      a IOCTL_KDBMTEST_CREATE_DB control code.
//
//      DatabaseName     - This is a pointer to an array of wide characters
//                         that hold the name of the database to be created.
//
//      AppendixSize     - The number of bytes to be written in the database
//                         appendix.
//
//      DatabaseAppendix - A pointer to the data to be written in the database
//                         appendix.
//
//      NumberOfTables   - The number of tables specified in the TABLE_SPEC
//                         array.
//
//      TableSpecArray   - An array of structures that define each table to be
//                         created in the database.
//
//      CacheSize        - Size (in bytes) of the in-memory cache to be 
//                         allocated by KDBM for this database.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_CREATE_DB_IN_BUFFER
{
    ULONG                     RevisionID;
    CHAR                      DatabaseName [KDBM_DATABASE_NAME_LEN_IN_CHARS];
    ULONG                     AppendixSizeInBytes;
    CHAR                      DatabaseAppendix [KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES];
    ULONG                     NumberOfTables;
    KDBM_TABLE_SPEC           TableSpecArray [KDBM_MAX_TABLES_PER_DATABASE];
    ULONG                     CacheSize;

} KDBMTEST_CREATE_DB_IN_BUFFER, *PKDBMTEST_CREATE_DB_IN_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_CREATE_DB_OUT_BUFFER
//
// Description:
//      The output buffer to send the result of a CreateDB() operation to
//      the user.
//
//      DatabaseHandle   - an opaque handle to the KDBM database created, to be
//                         used in subsequent KDBM operations.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_CREATE_DB_OUT_BUFFER
{
    KDBM_DATABASE_HANDLE      DatabaseHandle;

} KDBMTEST_CREATE_DB_OUT_BUFFER, *PKDBMTEST_CREATE_DB_OUT_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_ATTACH_DB_IN_BUFFER
//
// Description:
//      The input buffer for an AttachDB() operation, used in conjuction with
//      a IOCTL_KDBMTEST_ATTACH_DB control code.
//
//      DatabaseName     - This is a pointer to an array of wide characters
//                         that hold the name of an existing database to
//                         attach to.
//
//      CacheSize        - Size (in bytes) of the in-memory cache to be 
//                         allocated by KDBM for this database.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_ATTACH_DB_IN_BUFFER
{
    ULONG                     RevisionID;
    CHAR                      DatabaseName [KDBM_DATABASE_NAME_LEN_IN_CHARS];
    ULONG                     CacheSize;

} KDBMTEST_ATTACH_DB_IN_BUFFER, *PKDBMTEST_ATTACH_DB_IN_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_ATTACH_DB_OUT_BUFFER
//
// Description:
//      The output buffer to send the result of a AttachDB() operation to
//      the user.
//
//      DatabaseHandle   - an opaque handle to the KDBM database, to be
//                         used in subsequent KDBM operations.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_ATTACH_DB_OUT_BUFFER
{
    KDBM_DATABASE_HANDLE      DatabaseHandle;

} KDBMTEST_ATTACH_DB_OUT_BUFFER, *PKDBMTEST_ATTACH_DB_OUT_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_DETACH_DB_IN_BUFFER
//
// Description:
//      The input buffer for an DetachDB() operation, used in conjuction with
//      a IOCTL_KDBMTEST_DETACH_DB control code.
//
//      DatabaseHandle   - an opaque handle to the KDBM database to detach from.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_DETACH_DB_IN_BUFFER
{
    ULONG                     RevisionID;
    KDBM_DATABASE_HANDLE      DatabaseHandle;

} KDBMTEST_DETACH_DB_IN_BUFFER, *PKDBMTEST_DETACH_DB_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_DELETE_DB_IN_BUFFER
//
// Description:
//      The input buffer for an DeleteDB() operation, used in conjuction with
//      a IOCTL_KDBMTEST_DELETE_DB control code.
//
//      DatabaseName     - This is a pointer to an array of wide characters
//                         that hold the name of an existing database to
//                         delete.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_DELETE_DB_IN_BUFFER
{
    ULONG               RevisionID;
    CHAR                DatabaseName [KDBM_DATABASE_NAME_LEN_IN_CHARS];

} KDBMTEST_DELETE_DB_IN_BUFFER, *PKDBMTEST_DELETE_DB_IN_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_VERIFY_DB_IN_BUFFER
//
// Description:
//      The input buffer for a VerifyDB() operation, used in conjuction with
//      a IOCTL_KDBMTEST_VERIFY_DB control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      AppendixSize     - The number of bytes in the database appendix to
//                         verify by comparing to user supplied database 
//                         appendix data.
//
//      DatabaseAppendix - A pointer to the data to be compared to the database
//                         appendix.
//
//      NumberOfTables   - The number of tables specified in the TABLE_SPEC
//                         array.
//
//      TableSpecArray   - An array of structures that define each table to be
//                         verified in the database.
//
//      Breadth          - A value used to specify the breadth of information
//                         to be verified in the database.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_VERIFY_DB_IN_BUFFER
{
    ULONG                           RevisionID;
    KDBM_DATABASE_HANDLE            DatabaseHandle;
    ULONG                           AppendixSizeInBytes;
    CHAR                            DatabaseAppendix [KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES];
    ULONG                           NumberOfTables;
    KDBM_TABLE_SPEC                 TableSpecArray [KDBM_MAX_TABLES_PER_DATABASE];
    KDBM_DATABASE_VERIFY_BREADTH    Breadth;

} KDBMTEST_VERIFY_DB_IN_BUFFER, *PKDBMTEST_VERIFY_DB_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_PUT_DB_APPENDIX_IN_BUFFER
//
// Description:
//      The input buffer for a PutDBAppendix() operation, used in conjuction
//       with a IOCTL_KDBMTEST_PUT_DB_APPENDIX control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      AppendixSize     - The number of bytes to write to the database
//                         appendix.
//
//      DatabaseAppendix - A pointer to the data to be written to the database
//                         appendix.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_PUT_DB_APPENDIX_IN_BUFFER
{
    ULONG                     RevisionID;
    KDBM_DATABASE_HANDLE      DatabaseHandle;
    ULONG                     AppendixSizeInBytes;
    CHAR                      DatabaseAppendix [KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES];

} KDBMTEST_PUT_DB_APPENDIX_IN_BUFFER, *PKDBMTEST_PUT_DB_APPENDIX_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_DB_APPENDIX_IN_BUFFER
//
// Description:
//      The input buffer for a GetDBAppendix() operation, used in conjuction
//       with a IOCTL_KDBMTEST_GET_DB_APPENDIX control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      AppendixSize     - The number of bytes to read from the database
//                         appendix.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_DB_APPENDIX_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 AppendixSizeInBytes;

} KDBMTEST_GET_DB_APPENDIX_IN_BUFFER, *PKDBMTEST_GET_DB_APPENDIX_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_DB_APPENDIX_OUT_BUFFER
//
// Description:
//      The output buffer for a GetDBAppendix() operation, used to return data
//      to the user.
//
//      AppendixSize     - The number of valid bytes stored in the database
//                         appendix previously by the user.
//
//      AppendixData     - A buffer to hold the data read from the database
//                         appendix.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_DB_APPENDIX_OUT_BUFFER
{
    CHAR     AppendixData [1];

} KDBMTEST_GET_DB_APPENDIX_OUT_BUFFER, *PKDBMTEST_GET_DB_APPENDIX_OUT_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_ADD_TABLE_IN_BUFFER
//
// Description:
//      The input buffer for a AddTable() operation, used in conjuction with
//      a IOCTL_KDBMTEST_ADD_TABLE control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The index of the table to be added to the database.
//
//      TableSpec        - A description of the table to be added.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_ADD_TABLE_IN_BUFFER
{
    ULONG                     RevisionID;
    KDBM_DATABASE_HANDLE      DatabaseHandle;
    ULONG                     TableIndex;
    KDBM_TABLE_SPEC           TableSpec;

} KDBMTEST_ADD_TABLE_IN_BUFFER, *PKDBMTEST_ADD_TABLE_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_EXPAND_TABLE_IN_BUFFER
//
// Description:
//      The input buffer for a ExpandTable() operation, used in conjuction with
//      a IOCTL_KDBMTEST_EXPAND_TABLE control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The index of the table to be expanded.
//
//      TotalNumberOfRecords - The number of records in the expanded table.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_EXPAND_TABLE_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;
    ULONG                 TotalNumberOfRecords;

} KDBMTEST_EXPAND_TABLE_IN_BUFFER, *PKDBMTEST_EXPAND_TABLE_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_PUT_TABLE_APPENDIX_IN_BUFFER
//
// Description:
//      The input buffer for a PutTableAppendix() operation, used in conjuction
//      with a IOCTL_KDBMTEST_PUT_TABLE_APPENDIX control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The index of the table in the database.
//
//      AppendixSize     - The number of bytes to write into the table 
//                         appendix.
//
//      TableAppendix    - A pointer containing the data to be written to the
//                         table appendix.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_PUT_TABLE_APPENDIX_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;
    ULONG                 AppendixSizeInBytes;
    CHAR                  TableAppendix [KDBM_TABLE_APPENDIX_SIZE_IN_BYTES];

} KDBMTEST_PUT_TABLE_APPENDIX_IN_BUFFER, *PKDBMTEST_PUT_TABLE_APPENDIX_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_TABLE_APPENDIX_IN_BUFFER
//
// Description:
//      The input buffer for a GetTableAppendix() operation, used in conjuction
//      with a IOCTL_KDBMTEST_GET_TABLE_APPENDIX control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The index of the table in the database.
//
//      AppendixSize     - The number of bytes to read from the table 
//                         appendix.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_TABLE_APPENDIX_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;
    ULONG                 AppendixSizeInBytes;

} KDBMTEST_GET_TABLE_APPENDIX_IN_BUFFER, *PKDBMTEST_GET_TABLE_APPENDIX_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_TABLE_APPENDIX_OUT_BUFFER
//
// Description:
//      The outputbuffer for a GetTableAppendix() operation, used to return
//      the table appendix to the user.
//
//      AppendixSize      - The number of valid bytes stored in the table
//                          appendix previously by the user.
//
//      TableAppendixData - A buffer to hold the data read from the table
//                          appendix.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_TABLE_APPENDIX_OUT_BUFFER
{
    ULONG    AppendixSizeInBytes;
    CHAR     TableAppendixData [1];

} KDBMTEST_GET_TABLE_APPENDIX_OUT_BUFFER, *PKDBMTEST_GET_TABLE_APPENDIX_OUT_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_TABLE_METRICS_IN_BUFFER
//
// Description:
//      The input buffer for a GetTableMetrics() operation, used in conjuction
//      with a IOCTL_KDBMTEST_GET_TABLE_METRICS control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The index of the table whose information is sought.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_TABLE_METRICS_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;

} KDBMTEST_GET_TABLE_METRICS_IN_BUFFER, *PKDBMTEST_GET_TABLE_METRICS_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_TABLE_METRICS_OUT_BUFFER
//
// Description:
//      The output buffer for a GetTableMetrics() operation, used to send the
//      table information back to the user.
//
//      NumberOfRecords  - The number of records in the table.
//
//      RecordSize       - The size of each record in the table.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_TABLE_METRICS_OUT_BUFFER
{
    ULONG        NumberOfRecords;
    ULONG        RecordSize;

} KDBMTEST_GET_TABLE_METRICS_OUT_BUFFER, *PKDBMTEST_GET_TABLE_METRICS_OUT_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_TABLE_NAME_IN_BUFFER
//
// Description:
//      The input buffer for a GetTableName() operation, used in conjuction
//      with a IOCTL_KDBMTEST_GET_TABLE_NAME control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The index of the table whose name is sought.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_TABLE_NAME_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;

} KDBMTEST_GET_TABLE_NAME_IN_BUFFER, *PKDBMTEST_GET_TABLE_NAME_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_TABLE_NAME_OUT_BUFFER
//
// Description:
//      The output buffer for a GetTableName() operation, used to send the
//      table name back to the user.
//
//      TableName  - An array of wide characters that hold the name of the
//                   requested table.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_TABLE_NAME_OUT_BUFFER
{
    CHAR      TableName [KDBM_TABLE_NAME_LEN_IN_CHARS];

} KDBMTEST_GET_TABLE_NAME_OUT_BUFFER, *PKDBMTEST_GET_TABLE_NAME_OUT_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_IN_BUFFER
//
// Description:
//      The input buffer for a GetNumberOfTablesInUse() operation, used in 
//      conjuction with a IOCTL_KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE 
//      control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;

} KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_IN_BUFFER, *PKDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_OUT_BUFFER
//
// Description:
//      The output buffer for a GetNumberOfTablesInUse() operation, used to 
//      the number of tables used by the database back to the user.
//
//      NumberOfTables - The number of tables in use in the database.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_OUT_BUFFER
{
    ULONG        NumberOfTablesInUse;

} KDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_OUT_BUFFER, *PKDBMTEST_GET_NUMBER_OF_TABLES_IN_USE_OUT_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_PUT_RECORD_IN_BUFFER
//
// Description:
//      The input buffer for a PutRecord() operation, used in conjuction
//       with a IOCTL_KDBMTEST_PUT_RECORD control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The table index to which the record belongs.
//
//      RecordIndex      - The record index within the table.
//
//      DataSize         - The number of bytes to write to the record.
//
//      Data             - The data to write to the record.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_PUT_RECORD_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;
    ULONG                 RecordIndex;
    ULONG                 RecordSizeInBytes;
    CHAR                  RecordData [1];

} KDBMTEST_PUT_RECORD_IN_BUFFER, *PKDBMTEST_PUT_RECORD_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_GET_RECORD_IN_BUFFER
//
// Description:
//      The input buffer for a GetRecord() operation, used in conjuction
//       with a IOCTL_KDBMTEST_GET_RECORD control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TableIndex       - The table index to which the record belongs.
//
//      RecordIndex      - The record index within the table.
//
//      DataSize         - The number of bytes to read from the record.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_RECORD_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TableIndex;
    ULONG                 RecordIndex;
    ULONG                 RecordSizeInBytes;

} KDBMTEST_GET_RECORD_IN_BUFFER, *PKDBMTEST_GET_RECORD_IN_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_GET_RECORD_OUT_BUFFER
//
// Description:
//      The output buffer for a GetRecord() operation, used to send the 
//      record data back to the user.
//
//      Data    - A buffer into which the record has been read.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_RECORD_OUT_BUFFER
{
    CHAR     RecordData [1];

} KDBMTEST_GET_RECORD_OUT_BUFFER, *PKDBMTEST_GET_RECORD_OUT_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_START_TRANSACTION_IN_BUFFER
//
// Description:
//      The input buffer for a StartTransaction() operation, used in conjuction
//       with a IOCTL_KDBMTEST_START_TRANSACTION control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
//      TimeoutInMinutes - The expected maximum duration of the transaction
//                         in minutes.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_START_TRANSACTION_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;
    ULONG                 TimeoutInMinutes;

} KDBMTEST_START_TRANSACTION_IN_BUFFER, *PKDBMTEST_START_TRANSACTION_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_DISCARD_TRANSACTION_IN_BUFFER
//
// Description:
//      The input buffer for a DiscardTransaction() operation, used in 
//      conjuction with a IOCTL_KDBMTEST_DISCARD_TRANSACTION control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_DISCARD_TRANSACTION_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;

} KDBMTEST_DISCARD_TRANSACTION_IN_BUFFER, *PKDBMTEST_DISCARD_TRANSACTION_IN_BUFFER;
//.End


//++
// Type:
//      KDBMTEST_COMMIT_TRANSACTION_IN_BUFFER
//
// Description:
//      The input buffer for a CommitTransaction() operation, used in 
//      conjuction with a IOCTL_KDBMTEST_COMMIT_TRANSACTION control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_COMMIT_TRANSACTION_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;

} KDBMTEST_COMMIT_TRANSACTION_IN_BUFFER, *PKDBMTEST_COMMIT_TRANSACTION_IN_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_GET_COMPATIBILITY_MODE_IN_BUFFER
//
// Description:
//      The input buffer for a CommitTransaction() operation, used in 
//      conjuction with a IOCTL_KDBMTEST_GET_COMPATIBILITY_MODE control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_COMPATIBILITY_MODE_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;

} KDBMTEST_GET_COMPATIBILITY_MODE_IN_BUFFER, *PKDBMTEST_GET_COMPATIBILITY_MODE_IN_BUFFER;

//++
// Type:
//      KDBMTEST_GET_COMPATIBILITY_MODE_OUT_BUFFER
//
// Description:
//      The input buffer for a CommitTransaction() operation, used in 
//      conjuction with a IOCTL_KDBMTEST_GET_COMPATIBILITY_MODE control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_GET_COMPATIBILITY_MODE_OUT_BUFFER
{
	K10_DRIVER_COMPATIBILITY_MODE    CompatMode;

} KDBMTEST_GET_COMPATIBILITY_MODE_OUT_BUFFER, *PKDBMTEST_GET_COMPATIBILITY_MODE_OUT_BUFFER;

//++
// Type:
//      KDBMTEST_PUT_COMPATIBILITY_MODE_IN_BUFFER
//
// Description:
//      The input buffer for a CommitTransaction() operation, used in 
//      conjuction with a IOCTL_KDBMTEST_PUT_COMPATIBILITY_MODE control code.
//
//      DatabaseHandle   - The database handle returned from a previous call
//                         to CreateDB() or AttachDB().
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_PUT_COMPATIBILITY_MODE_IN_BUFFER
{
    ULONG                 RevisionID;
    KDBM_DATABASE_HANDLE  DatabaseHandle;

} KDBMTEST_PUT_COMPATIBILITY_MODE_IN_BUFFER, *PKDBMTEST_PUT_COMPATIBILITY_MODE_IN_BUFFER;
//.End

//++
// Type:
//      KDBMTEST_PANIC_IN_BUFFER
//
// Description:
//      The input buffer for panicking an SP through KDBMTest.  Used in 
//      conjunction with an IOCTL_KDBMTEST_PANIC control code.
//
// Revision History:
//      16-Mar-06   Sri     Created.
//--
typedef struct _KDBMTEST_PANIC_IN_BUFFER
{
    ULONG                 RevisionID;

} KDBMTEST_PANIC_IN_BUFFER, *PKDBMTEST_PANIC_IN_BUFFER;
//.End

//++
//.End Types
//--

#endif // __KDBMTEST_TYPES_H_

//++
//.End file kdbmTestTypes.h
