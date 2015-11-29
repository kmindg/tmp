#ifndef _KDBM_H_
#define _KDBM_H_
//=== File Prolog ==============================================================
//--- Copyright ----------------------------------------------------------------
//    Copyright (C) EMC Corporation 2000-2005
//    All rights reserved.
//    Licensed material -- property of EMC Corporation
//
//--- Description --------------------------------------------------------------
//    This header file defines the interface to the Kernel DataBase Manager. Any
//    client of the KDBM Subsystem need include only this file to have access 
//    to the interace of that Kernel DataBase Manager.
// 
//--- Comments --------------------------------------------------------------------
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

#ifdef __cplusplus
extern "C" {
#endif

#include "k10ntddk.h"

#include "k10diskdriveradmin.h"

#ifdef __cplusplus
};
#endif

#include "K10KernelDatabaseManagerMessages.h"

#include "kdbmTypes.h"


//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_Create()
//
// Description:  
// The KDBM_Create routine creates a new Database and Attaches to the new Database.
//
// Arguments: 
//
// PDatabaseHandle - a pointer to a KDBM Database Handle. This value is set in 
// Attach, and must be passed in all subsequent KDBM operations while the 
// Database is Attached.
//
// DatabaseName - A Wide Character string with at most 
// KDBM_DATABASE_MAX_NAME_LEN Wide Characters, excluding the terminating NULL.
// Database Names must be unique on the array.
//
// AppendixBytesToPut - The size of the following Appendix buffer in bytes. 
// This must be at most KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES. An Appendix of 
// KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES bytes is always created - KDBM will 
// zero fill unused bytes. Using a NULL value (and a 0 for the Appendix 
// BytesToPut) zero fills the Appendix.
//
// DatabaseAppendix - The initial value for the Database Appendix.
//
// NumberOfTables - The number of KDBM_TABLE_SPEC structures in the following 
// array.
// 
// TableSpecArray - An array of KDBM_TABLE_SPEC structures, one per table 
// to be initlialized.
//
// PAlloc - An NT PALLOCATE_FUNCTION to be used by KDBM when allocating 
// memory. KDBM will always set the PoolType to NonPaged Pool, with the 
// KDBM_ALLOC_TAG. Client may specify any function (CMM, memmger, etc.) that 
// will allocate non-paged memory.
//
// PFree - An NT PFREE_FUNCTION to be used by KDBM when freeing memory.
//
// RecordCacheSize - This argument is used to calculate the size of a buffer 
// used to transfer record to/from the Physical Databse. It must be at least as large 
// as the largest Record in the Database.
//
// Returns:    
// STATUS_SUCCESS - A Database was created with the specified Tables and 
// Appendix, and has been attached to.
//
// KDBM_WARNING_CREATE_DATABASE_EXISTS - A Database with the specified 
// name exists.
//
// STATUS_INSUFFICIENT_RESOURCES - KDBM could not allocate nondurable memory)r
// esources needed to create the Database.
//
// STATUS_DISK_FULL - KDBM could not allocate a Physical Databse 
// needed to create the Database.
//
// KDBM_WARNING_RECORD_CACHE_TOO_SMALL - the Record Cache Size is too small 
// to transfer a Record in at least one Table in the Database.
//
// KDBM_ERROR_SCHEMA_INSANE - internal KDBM Error
//
// KDBM_ERROR_DATABASE_NOT_DETACHED - KDBM_Create can only be called when 
// the Database is Detached. 
// KDBM_ERROR_DATABASE_APPENDIX_TOO_LARGE - AppendixBytesToPut >  
// KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES.
//
// KDBM_ERROR_CREATE_TOO_MANY_TABLES - NumberOfTables > 
// KDBM_MAX_TABLES_PER_DATABASE
//
// Notes:
// All Table Appendices are zero filled by this operation.
//
// There is an obvious race condition where a Client on each SP attempts to 
// create a new Database. The Client on one SP will succeed, the Client on 
// the other SP will Attach to the created Database. In this case, the call 
// to KDBM_Create on the SP that does not create the Database will return 
// KDBM_WARNING_CREATE_DATABASE_EXISTS. The Client on the SP that actually 
// creates the Database will see a STATUS_SUCCESS success return. Clients may 
// code a "Create or Open" function by looking for that particular Error.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.1
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_Create(KDBM_DATABASE_HANDLE* PHandle, 
                              csx_pchar_t                DatabaseName,
                              UINT32                AppendixSizeInBytes,
                              PVOID                 PDatabaseAppendix,
                              UINT32                NumberOfTables,
                              PKDBM_TABLE_SPEC      TableSpecArray,
                              PEMCPAL_ALLOC_FUNCTION    PAlloc,
                              PEMCPAL_FREE_FUNCTION        PFree,
                              UINT32                CacheSize);

//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_CreateZeroFill()
//
// Description:  
// The KDBM_Create Zero Fill routine creates a new Database, forcibly zero fills the Database,
// and Attaches to the new Database
//
// Arguments: 
// See KDBM_Create()
//
// Returns:    
// See KDBM_Create()
//
// Notes:
// PSM - the underlying storage mechanism - uses a "lazy" zeroing algorithm that only zero fills
// from the offset of the last write in a Data Area to the offset of a new write. This algorithm
// can lead to "spiky" performance as the first Put Record operation may cause a zero fill of MBs
// of data. 
//
// This routines moves the cost of that zeroing "up front". It is expected to be used when the client 
// prefers to assume the cost of zeroing at the time of creation  - as opposed to assuming that cost at
// the time of modification (e.g., a KDBM_PutRecord() to a high offset in the Database).
//
// KDBM Fuctional Specification: Section 5.5.22
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_CreateZeroFill(KDBM_DATABASE_HANDLE* PHandle, 
                              csx_pchar_t           DatabaseName,
                              UINT32                AppendixSizeInBytes,
                              PVOID                 PDatabaseAppendix,
                              UINT32                NumberOfTables,
                              PKDBM_TABLE_SPEC      TableSpecArray,
                              PEMCPAL_ALLOC_FUNCTION    PAlloc,
                              PEMCPAL_FREE_FUNCTION        PFree,
                              UINT32                CacheSize);

//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_Attach()
//
// Description:  
// KDBM_Attach allocates the resources needed to access an existing Database.
//
// Arguments:
// PDatabaseHandle - See KDBM_Create()
// DatabaseName - See KDBM_Create()
// PAlloc - See KDBM_Create()
// PFree - See KDBM_Create()
// RecordCacheSize - See KDBM_Create()
//
// Returns:    
// STATUS_SUCCESS - KDBM has allocated sufficient resources for any subsequent 
// operation on the Database.
//
// KDBM_WARNING_NO_SUCH_DATABASE - A Database with the specified name does not 
// exist.
//
// KDBM_ERROR_SCHEMA_INSANE - the Database Schema is invalid. KDBM will 
// "fsck" the Schema and return this Error if the Schema is ill-formed.
//
// STATUS_INSUFFICIENT_RESOURCES - KDBM could not allocate resources needed 
// to access the Database.
//
// KDBM_WARNING_RECORD_CACHE_TOO_SMALL - the Record Cache Size is too small 
// to transfer a Record in at least one Table in the Database.
//
// KDBM_ERROR_DATABASE_NOT_DETACHED - KDBM_Attach can only be called when 
// the Database Detached. 
//
// Notes:
// Clients are expected and encouraged to Attach to a DB in Drive Entry, and 
// to remain Attached for the life of the system.
//
// The Record Cache Size must be at least as large as the largest Record in the 
// DB for the duration of the Attach. If the Record Cache Size is several 
// times the size of the largest Record, there will be a significant 
// performance increase for sequential operations - KDBM will attempt to 
// coalesce data transfer to/from the Physical Database. In general, the number 
// of calls to the Physical Database for a set of sequential operations will be 
// as follows: Number of Calls to Backing Store =
//
//  Number of Sequential Operations × Record Size / Record Cache Size
//
// If a Client reads a set of Records in Drive Entry, it may be worthwhile for 
// the Client to set a large Record Cache Size during Driver Entry. After all
// of the Record are read, the Client could Detach and re-Attach with a 
// smaller Buffer Size for the life of the system. 
//
// If a Client is adding a new Table to an existing Database, and that Table 
// has a larger Re-cord Size than any existing Table in the Database, the 
// Client must ensure that both the Local and the Peer SP (the SP not 
// processing the Add Table operation) has an Record Cache Size large enough 
// to accommodate the new larger Record Size (See KDBM_StartTranscation()).
//
// KDBM Fuctional Specification: Section 5.5.2
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_Attach(KDBM_DATABASE_HANDLE* PHandle, 
                              csx_pchar_t                DatabaseName,
                              PEMCPAL_ALLOC_FUNCTION    PAlloc,
                              PEMCPAL_FREE_FUNCTION        PFree,
                              const UINT32           CacheSize);


//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_verify()
//
// Description:  
// Description of the method written for a user or maintainer of the method
//
// Arguments:
// Handle - returned from KDBM_Create().
// AppendixBytesToVerify - Size of the Appendix Buffer in bytes.
// DatabaseAppendix - See KDBM_Create ().
// NumberOfTables - See KDBM_Create ().
// TableSpecArray - See KDBM_Create ().
// Breadth - See KDBM_DATABASE_VERIFY_BREADTH 
//
// Returns:    
// STATUS_SUCCESS - The Database matched the specification.
//
// KDBM_WARNING_VERIFY_DATABASE_TABLE_METRICS_FAILED - At least one of the 
// Tables in the Database has a different number of Records or a different 
// size of Record from its specification.
//
// KDBM_WARNING_VERIFY_DATABASE_TABLES_FAILED - All of the specified Tables 
// had the correct metrics, but at least one did not match its full 
// specification.
//
// KDBM_WARNING_VERIFY_DATABASE_TABLES_AND_APPENDIX_FAILED - All of the 
// specified Tables matched the specifications, but the specified Database 
// Appendix did not match the stored Database Appendix.
//
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_Verify can only be called when 
// the Database is Attached.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or un-initialized.
//
// KDBM_ERROR_DATABASE_TAINTED - the Database was "tainted" by an earlier 
// Put failure, and the Transaction has not been Aborted.
//
// KDBM Fuctional Specification: Section 5.5.3
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_Verify( KDBM_DATABASE_HANDLE        Handle,
                             UINT32                         AppendixBytesToVerify,
                             PVOID                         DatabaseAppendix,
                             UINT32                         NumberOfTables,
                             PKDBM_TABLE_SPEC              TableSpecArray,
                             KDBM_DATABASE_VERIFY_BREADTH  Breadth);


//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_Detach()
//
// Description:  
// KDBM_Detach() releases all resources allocated in KDBM_Attach().
//
// Arguments:
// DatabaseHandle - returned from KDBM_Create().
//
// Returns:    
// STATUS_SUCCESS - The Database was successfully Detached.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_Detach can only be called when 
// the Database is Attached.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.4
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_Detach(KDBM_DATABASE_HANDLE Handle);

//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_Create()
//
// Description:  
// KDBM_Delete destroys a Database, including freeing a Backing Store.
//
// Arguments: 
//
// DatabaseName - see KDBM_Create().
//
// PAlloc - see KDBM_Create().
//
// PFree - see KDBM Create()
//
// Returns:    
// STATUS_SUCCESS - The Database was deleted.
//
// KDBM_WARNING_NO_SUCH_DATABASE - A Database with the specified 
// name does not exist.
//
// STATUS_INSUFFICIENT_RESOURCES - KDBM could not allocate memory resources 
// needed to delete the Database.
//
// KDBM_ERROR_DATABASE_NOT_DETACHED - KDBM_Create can only be called when 
// the Database is Detached. 
// KDBM_ERROR_DATABASE_APPENDIX_TOO_LARGE - AppendixBytesToPut >  
// KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES.
//
// Notes:
// When a Database is Deleted, all records stored in the Database are 
// permanently de-stroyed. Re-creating the Database will not recover 
// those records.
//
// Some resources must be allocated to Delete the Database. KDBM either 
// needs to require a prior Attach or PAlloc and PFree functions as 
// arguments to this call.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.5
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_Delete( csx_pchar_t                DatabaseName,
                               PEMCPAL_ALLOC_FUNCTION    PAlloc,
                               PEMCPAL_FREE_FUNCTION        PFree);

//--- Method Prolog ------------------------------------------------------------
// Name:   KDBM_PutDBAppendix()
//
// Description:  
// 
// KDBM_PutDBAppendix stores non-Record Client data in a Database. 
// KDBM_PutDBAppendix does not understand or interpret this data, but 
// will return this data in call to KDBM_GetDBAppendix.
//
// Arguments:
// DatabaseHandle      - returned from KDBM_Create()
// AppendixSizeInBytes - See KDBM_Create()
// PAppendix           - See KDBM_Create()
//
// Returns:    
// STATUS_SUCCESS - The Appendix was stored
//
// KDBM_ERROR_DATABASE_TAINTED - a previous Put has failed. An attempt 
// to Commit this Transaction will return an Error.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_PutDBAppendix can only be 
// called when the Database is Transacting.
//
//KDBM_ERROR_DATABASE_APPENDIX_TOO_LARGE - >  AppendixSizeInBytes KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES
//
// Notes:  
// The Database Appendix information in the Database is intended to be 
// used to store Client revision numbers, etc. The size of the DB Appendix 
// is limited by the desire to store the Appendix and the Schema in a 
// single disk sector (512 bytes).
// 
// KDBM Fuctional Specification: Section 5.5.6
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_PutDBAppendix(KDBM_DATABASE_HANDLE Handle, 
                                     UINT32                AppendixSizeInBytes,
                                     PVOID                PAppendix);

//--- Method Prolog ------------------------------------------------------------
// Name:   KDBM_GetDBAppendix()
//
// Description:  
// KDBM_GetDBAppendix() retrieves Client data stored previously by 
// KDBM_Create() or KDBM_PutDBAppendix().
//
// Arguments:
// DatabaseHandle      - returned from KDBM_Create()
// AppendixSizeInBytes - Number of Bytes to retrieve from the Database
// PAppendix           - Buffer for retrieved bytes.
//
// Returns:    
// STATUS_SUCCESS - The Appendix was retrieved.
//
// KDBM_ERROR_DATABASE_TAINTED - a previous Put has failed. An attempt 
// to Commit this Transaction will return an Error.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_GetDBAppendix can only be 
// called when the Database is Transacting.
//
//KDBM_ERROR_DATABASE_APPENDIX_TOO_LARGE - >  AppendixSizeInBytes KDBM_DATABASE_APPENDIX_SIZE_IN_BYTES
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.7
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetDBAppendix(KDBM_DATABASE_HANDLE Handle, 
                                     UINT32               AppendixSizeInBytes,
                                     PVOID                PAppendix);

//--- Method Prolog ------------------------------------------------------------
// Name:   KDBM_AddTable()
//
// Description:  
// KDBM_AddTable() adds a new Table to an existing Database.
//
// Arguments:
// DatabaseHandle      - returned from KDBM_Create()
// TableIndex          - Which Table?
// PTableSpec          - Table Specification
//
// Returns:    
// STATUS_SUCCESS - The Table was added.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_AddTable can only be 
// called when the Database is Attached.
//
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex >= KDBM_MAX_TABLES_PER_DATABASE. 
// 
// KDBM_ERROR_ADD_TABLE_OVERWRITES_EXISTING_TABLE - a Table already exists at 
// the specified index
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.8
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_AddTable(KDBM_DATABASE_HANDLE Handle, 
                                     UINT32               TableIndex,
                                     PKDBM_TABLE_SPEC     PTableSpec);

//--- Method Prolog ------------------------------------------------------------
// Name:   KDBM_GetNumberOfTablesInUse()
//
// Description:  
// KDBM_GetNumberOfTablesInUse() returns the number of Tables currently in use
// in the Database
//
// Arguments:
// DatabaseHandle      - returned from KDBM_Create()
// PNumerOfTables      - number of Tables currently configured in the Database
//
// Returns:    
// STATUS_SUCCESS -  the number of Tables was calculated correcty
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_GetNumberOfTablesInUse() can only be 
// called when the Database is Transacting.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.9
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetNumberOfTablesInUse(KDBM_DATABASE_HANDLE Handle, 
                                              PUINT32              PNumberOfTables);


//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_ExpandTable
//
// Description:  
// KDBM_Expand_Table adds records to a Table in an existing Database.
//
// Arguments:
// DatabaseHandle       - returned from KDBM_Create
// TableIndex           - Which Table?
// TotalNumberOfRecords - TotalNumberOfRecords - The number of Records in the 
//                        expanded Table. This is not the number of Records 
//                        added to the Table - it is the total number of 
//                        Records in the expanded Table.
//
// Returns:    
// STATUS_SUCCESS - The Table was expanded.
// KDBM_WARNING_TABLE_EXPAND_NO_OP - a Table already has the 
//                    specified NumberOfRecords.
// KDBM_WARNING_TABLE_SHRINK_NOT_SUPPORTED - The Table already contains more 
//                    than the specified number of records.
// STATUS_DISK_FULL - Backing Store could not be allocated for the 
//                   new Records.
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
//                   the specified Handle - the Handle is most likely stale 
//                   or uninitialized. 
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_Expand_Table can only be called 
//                   when the Database is Attached.
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - 
//                   TableIndex >= KDBM_MAX_TABLES_PER_DATABASE. 
// KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
// Notes: 
// 
// KDBM Fuctional Specification: Section 5.5.10
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_ExpandTable(KDBM_DATABASE_HANDLE Handle,
                                   UINT32               TableIndex,
                                   UINT32               TotalNumberOfRecords);

//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_GetTableName()
//
// Description:  
// KDBM_GetTableName() retrieves the name of a particular Table in a Database.
//
// Arguments:
// Handle - returned from KDBM_Create() or KDBM_Attach()
//
// TableIndex     -  Which Table?
//
// PTableName - Client allocated buffer for the Table Name; see KDBM_TABLE_SPEC .
//
// Returns:    
// STATUS_SUCCESS - The Table name was returned.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_GetTableName can only be 
// called when the Database is Transacting.
//
//  KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex >= KDBM_MAX_TABLES_PER_DATABASE. 
//
//  KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
//  KDBM_ERROR_DATABASE_TAINTED - the Database was "tainted" by an earlier 
//  Put failure, and the Transaction has not been Aborted.
//
// Notes:
//
// KDBM Fuctional Specification: Section 5.5.11
//
// 
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetTableName(KDBM_DATABASE_HANDLE    Handle,
                                    UINT32                   TableIndex,
                                    csx_pchar_t                   PTableName);



//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_GetTableMetrics()
//
// Description:  
// KDBM_GetTableMetrics retrieves the size of each Record and number of 
// Records in a particular Table in a Database.
//
// Arguments:
// Handle - returned from KDBM_Create, (§ 5.5.1 above).
//
// TableIndex - Which Table?
//
// PNumberOfRecords - Number of Records in the Table
//
// PSizeOfRecord - Size of each Record in the Table
//
// Returns:    
// STATUS_SUCCESS - The metrics were retrieved.
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_GetTableMetrics() can only 
// be called when the Database is Transacting.
//
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex >= KDBM_MAX_TABLES_PER_DATABASE. 
//
// KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
// KDBM_ERROR_DATABASE_TAINTED - the Database was "tainted" by an earlier 
// Put failure, and the Transaction has not been Aborted.
//
// Notes: 
//
// KDBM Fuctional Specification: Section 5.5.12
// 
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetTableMetrics( KDBM_DATABASE_HANDLE    Handle,
                                      UINT32               TableIndex,
                                      PUINT32          PNumberOfRecords,
                                      PUINT32          PSizeOfRecord);

//--- Method Prolog ------------------------------------------------------------
// Name:    KDBM_PutTableAppendix()
//
// Description:
// KDBM_Put_Table_Appendix stores opaque Client data in a Database for 
// a particular Table. KDBM_PutTableAppendix does not understand or 
// interpret the Data, but will return this data in call to 
// KDBM_GetTableAppendix.
//
// Arguments:
// TableIndex          - the Index of the Table whose Appendix we are 
//                       storing.
// DatabaseHandle      - returned from KDBM_Create()
// AppendixSizeInBytes - Number of Bytes to retrieve from the Database
// PAppendix           - Buffer for retrieved bytes.
//
// Returns:    
// STATUS_SUCCESS - The Appendix was stored.
//
// KDBM_ERROR_DATABASE_TAINTED - a previous Put has failed. An attempt to 
// Commit this Transaction will return an Error.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_PutTableAppendix() can only 
// be called when the Database is Transacting.
//
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex >= KDBM_MAX_TABLES_PER_DATABASE. 
//
// KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
// KDBM_ERROR_TABLE_APPENDIX_TOO_LARGE - AppendixSizeInBytes > KDBM_TABLE_APPENDIX_SIZE_IN_BYTES.
//
// Notes:
// KDBM keeps a "high water mark" of the bytes that have been stored in 
// the Table Appendix. This high water mark is returned as the Number of 
// Valid Bytes in KDBM_GetTableAppendix().
//
// KDBM Fuctional Specification: Section 5.5.13
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_PutTableAppendix(KDBM_DATABASE_HANDLE Handle,
                                        UINT32        TableIndex,
                                        UINT32        AppendixSizeInBytes,
                                        PVOID         PAppendix);

//--- Method Prolog ------------------------------------------------------------
// Name:    KDBM_GetTableAppendix()
//
// Description:
// KDBM_GetTableAppendix retrieves data stored by KDBM_PutTableAppendix()
//
// Arguments:
// TableIndex          - the Index of the Table whose Appendix we are 
//                       storing.
// DatabaseHandle      - returned from KDBM_Create()
// AppendixSizeInBytes - Number of Bytes to retrieve from the Database
// PAppendix           - Buffer for retrieved bytes.
// PNumberOfValidBytes - see Notes.
//
// Returns:    
// STATUS_SUCCESS - The Appendix was stored.
//
// KDBM_ERROR_DATABASE_TAINTED - a previous Put has failed. An attempt to 
// Commit this Transaction will return an Error.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_GetTableAppendix() can only 
// be called when the Database is Transacting.
//
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex >= KDBM_MAX_TABLES_PER_DATABASE. 
//
// KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
// KDBM_ERROR_TABLE_APPENDIX_TOO_LARGE - AppendixSizeInBytes > KDBM_TABLE_APPENDIX_SIZE_IN_BYTES.
//
// Notes:
// KDBM keeps a "high water mark" of the bytes that have been stored in 
// the Table Appendix. This high water mark is returned as the Number of 
// Valid Bytes in KDBM_GetTableAppendix().
//
// KDBM Fuctional Specification: Section 5.5.14
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetTableAppendix(KDBM_DATABASE_HANDLE Handle,
                                        UINT32        TableIndex,
                                        UINT32        AppendixSizeInBytes,
                                        PVOID        PAppendix,
                                        PUINT32       PNumberOfValidBytes);

//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_PutRecord()
//
// Description:  
// KDBM_Put_Record stores a Record at a particular index in a Table in a 
// Database
//
// Arguments:
// DatabaseHandle - returned from KDBM_Create()
//
// TableIndex - index of the Table containing the Record to be put.
//
// RecordIndex - The (zero based) Record Index of the Record in the Table.
//
// DataSizeInBytes - the size of the Data to be stored in the Record. If 
// DataSizeInBytes is less than the Table's Record size, the unused bytes 
// the Record will be untouched. If DataSizeInBytes is larger than the 
// Table's Record Size, KDBM return KDBM_ERROR_DATA_SIZE_LARGER_THAN_RECORD_SIZE.
//
// PData - The data to be stored - this buffer should be at >= DataSizeInBytes 
// 
// Returns:    
// STATUS_SUCCESS - The Record was stored.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with the 
// specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_PutRecord() can only be called 
// when the Database is Transacting.
//
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex > 
// KDBM_MAX_TABLES_PER_DATABASE 
//
// KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
// KDBM_ERROR_RECORD_INDEX_OUT_OF_RANGE - the RecordIndex specifies a 
// Record beyond the end of the Table.
//
// KDBM_ERROR_DATA_SIZE_LARGER_THAN_RECORD_SIZE - the Client is trying to 
// store more data than can be stored in the Record.
//
// KDBM_ERROR_TRANSACTION_TAINTED - the Database was "tainted" by an earlier 
// Put failure, and the Transaction has not been Aborted.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.15
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_PutRecord(KDBM_DATABASE_HANDLE DatabaseHandle,
                                 UINT32                TableIndex,
                                 UINT32                RecordIndex,
                                 UINT32                DataSizeInBytes,
                                 PVOID                PData);


//--- Method Prolog ------------------------------------------------------------
// Name:  KDBM_GetRecord()
//
// Description:  
// KDBM_GetRecord() retrieves a Record at a particular index in a Table 
// in a Database
//
// Arguments:
// DatabaseHandle - returned from KDBM_Create().
// TableIndex - see KDBM_PutRecord()
// RecordIndex - see KDBM_PutRecord()
// DataSizeInBytes - see KDBM_PutRecord()
// PData           - see KDBM_PutRecord()
//
// Returns:    
// STATUS_SUCCESS - The Record was retrieved.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with the 
// specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_Get_Record can only be called 
// when the Database is Transacting.
//
// KDBM_ERROR_DATA_SIZE_LARGER_THAN_RECORD_SIZE - the Client is trying to 
// retrieve more data than is stored in the Record.
//
// KDBM_ERROR_TABLE_INDEX_OUT_OF_RANGE - TableIndex >= KDBM_MAX_TABLES_PER_DATABASE
//
// KDBM_ERROR_TABLE_DOES_NOT_EXIST - no Table exists at the specified Index.
//
// KDBM_ERROR_RECORD_INDEX_OUT_OF_RANGE - the Record Index specifies a Record 
// beyond the end of the Table.
//
// KDBM_ERROR_TRANSACTION_TAINTED - the Database was "tainted" by an earlier 
// Put failure, and the Transaction has not been Aborted.
//
// Notes:  Anything relevant about this class, including document references,
// assumptions,constraints, restrictions, abnormal termination conditions, etc.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.16
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetRecord(KDBM_DATABASE_HANDLE DatabaseHandle,
                                 UINT32                TableIndex,
                                 UINT32                RecordIndex,
                                 UINT32                DataSizeInBytes,
                                 PVOID                PData);


//--- Method Prolog ------------------------------------------------------------
// Name:  
//
// Description:  KDBM_StartTransaction()
// KDBM_StartTransaction begins a Database Transaction.
//
// Arguments:
// DatabaseHandle - returned from KDBM_Create().
// 
// TransactionLifespanInMinutes - The expected maximum duration of the 
// Transaction in minutes. If KDBM_Commit_Transaction() or 
// KDBM_AbortTransaction() is not called before this duration expires, 
// then KDBM will discard the transaction.
//
// Returns:    
// STATUS_SUCCESS - The Transaction was started.
//
// KDBM_ERROR_IO_RECORD_CACHE_TOO_SMALL  - if KDBM discovers that is does 
// not have sufficient resources for a Put Record of any Record in the 
// Database at the time of a Start Transaction call, KDBM will Error. The 
// only way that this Error can occur is if a Table was added by the other 
// SP subsequent to the last Attach operation for this Database on this SP. 
// See KDBM_Attach().
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with 
// the specified Handle - the Handle is most likely stale or uninitialized. 
//
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_StartTransaction can only be 
// called when the Database is Attached.
//
// Notes:
// KDBM will serialize Transactions by multiple threads on the Local SP. 
//
// An individual thread must follow a (successful) call to 
// KDBM_StartTransaction() with a call to KDBM_CommitTransaction() or 
// KDBM_AbortTransation() prior to another call to KDBM_StartTransaction().
// If a thread calls KDBM_StartTransaction() twice without an intervening 
// call to one of the operations that terminates a transaction, the thread 
// will hang.
//
// Clients that want to manage their own timers can pass 
// KDBM_TRANSACTION_LIFESPAN_IMMORTAL for the Lifespan.
//
// KDBM Fuctional Specification: Section 5.5.17
//
//--- End Method Prolog --------------------------------------------------------

EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_StartTransaction(KDBM_DATABASE_HANDLE Handle,
                                        UINT32                TransactionLifespanInMinutes);

//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_CommitTransaction()
//
// Description:  
// KDBM_Commit_Transaction commits all modifications since the associated 
// KDBM_Start_Transaction. KDBM_Commit_Transaction terminates the Transaction.
//
// Arguments:
// Handle      Handle returned from a previous KDBM_StartTransaction()
//
// Returns:    
// STATUS_SUCCESS       The Transaction was terminated.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with the 
// specified Handle - the Handle is most likely stale or uninitialized
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_Commit_Transaction can only be 
// called when the Database is Transacting
//
// Notes:  Anything relevant about this class, including document references,
// assumptions,constraints, restrictions, abnormal termination conditions, etc.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.18
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_CommitTransaction(KDBM_DATABASE_HANDLE Handle);

//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_AbortTransaction()
//
// Description:  
// KDBM_Commit_Transaction discards all modifications since the associated 
// KDBM_Start_Transaction. KDBM_Commit_Transaction terminates the Transaction.
//
// Arguments:
// Handle      Handle returned from a previous KDBM_StartTransaction()
//
// Returns:    
// STATUS_SUCCESS       The Transaction was terminated.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with the 
// specified Handle - the Handle is most likely stale or uninitialized
//
// KDBM_ERROR_DATABASE_NOT_TRANSACTING - KDBM_Abort_Transaction can only be 
// called when the Database is Transacting
//
// Notes:  Anything relevant about this class, including document references,
// assumptions,constraints, restrictions, abnormal termination conditions, etc.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.19
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_AbortTransaction(KDBM_DATABASE_HANDLE Handle);

//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_GetCompatibilityMode()
//
// Description:  
// KDBM_GetCompatibilityMode() updates a KDBM Database to the Current Version
// of KDBM
//
// Arguments:
// Handle      Handle returned from a previous KDBM_StartTransaction()
//
// Returns:    
// STATUS_SUCCESS       The Transaction was terminated.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with the 
// specified Handle - the Handle is most likely stale or uninitialized
//
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_Abort_Transaction can only be 
// called when the Database is Attached
//
// Notes:  Anything relevant about this class, including document references,
// assumptions,constraints, restrictions, abnormal termination conditions, etc.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.21.
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_GetCompatibilityMode(KDBM_DATABASE_HANDLE             Handle,
	                                    K10_DRIVER_COMPATIBILITY_MODE*   PCompatMode);

//--- Method Prolog ------------------------------------------------------------
// Name: KDBM_PutCompatibilityMode()
//
// Description:  
// KDBM_PutCompatibilityMode() updates a KDBM Database to the Current Version
// of KDBM
//
// Arguments:
// Handle      Handle returned from a previous KDBM_StartTransaction()
//
// Returns:    
// STATUS_SUCCESS       The Transaction was terminated.
//
// KDBM_ERROR_HANDLE_NOT_FOUND - KDBM found no Database associated with the 
// specified Handle - the Handle is most likely stale or uninitialized
//
// KDBM_ERROR_DATABASE_NOT_ATTACHED - KDBM_Abort_Transaction can only be 
// called when the Database is Attached
//
// KDBM_ERROR_PUT_STORAGE_VERSION_MISMATCH - the Database could not be updated
// to the current version of KDBM
//
// Notes:  Anything relevant about this class, including document references,
// assumptions,constraints, restrictions, abnormal termination conditions, etc.
//
// Notes:
// 
// KDBM Fuctional Specification: Section 5.5.21.
//
//--- End Method Prolog --------------------------------------------------------
EXTERN_C_EXP CSX_MOD_EXPORT
EMCPAL_STATUS KDBM_PutCompatibilityMode(KDBM_DATABASE_HANDLE Handle);

#endif //_KDBM_H_
