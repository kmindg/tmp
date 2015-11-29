/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimSyncTransaction.h
 ***************************************************************************
 *
 * DESCRIPTION:  BVvdSim KDBM & SynchTrasactor function remapper
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/16/2011  MBuckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMSYNCTRANSACTION_H__
#define __BVDSIMSYNCTRANSACTION_H__

#if defined(SIMMODE_ENV)
#include "kdbm.h"
#include "SyncTransaction.h"

/*
 * used to configure usage of the Mock(false) or real SyncTransactor(true)
 * default behavior configures the Mock SyncTransactor
 */
EXTERN_C void BvdSim_set_SyncTransaction_enable(bool b); 

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_Attach(KDBM_DATABASE_HANDLE* PHandle, 
                              csx_pchar_t                DatabaseName,
                              PEMCPAL_ALLOC_FUNCTION    PAlloc,
                              PEMCPAL_FREE_FUNCTION        PFree,
                              const UINT32           CacheSize);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_Detach(KDBM_DATABASE_HANDLE Handle);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_Delete( csx_pchar_t                DatabaseName,
                               PEMCPAL_ALLOC_FUNCTION    PAlloc,
                               PEMCPAL_FREE_FUNCTION        PFree);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_AddTable(KDBM_DATABASE_HANDLE Handle, 
                                     UINT32               TableIndex,
                                     PKDBM_TABLE_SPEC     PTableSpec);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_PutRecord(KDBM_DATABASE_HANDLE DatabaseHandle,
                                 UINT32                TableIndex,
                                 UINT32                RecordIndex,
                                 UINT32                DataSizeInBytes,
                                 PVOID                PData);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_GetRecord(KDBM_DATABASE_HANDLE DatabaseHandle,
                                 UINT32                TableIndex,
                                 UINT32                RecordIndex,
                                 UINT32                DataSizeInBytes,
                                 PVOID                PData);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_StartTransaction(KDBM_DATABASE_HANDLE Handle,
                                        UINT32                TransactionLifespanInMinutes);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_CommitTransaction(KDBM_DATABASE_HANDLE Handle);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_GetDBAppendix(KDBM_DATABASE_HANDLE Handle, 
                                     UINT32               AppendixSizeInBytes,
                                     PVOID                PAppendix);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_PutDBAppendix(KDBM_DATABASE_HANDLE Handle, 
                                     UINT32               AppendixSizeInBytes,
                                     PVOID                PAppendix);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_PutTableAppendix(KDBM_DATABASE_HANDLE Handle,
                                        UINT32        TableIndex,
                                        UINT32        AppendixSizeInBytes,
                                        PVOID         PAppendix);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_GetTableAppendix(KDBM_DATABASE_HANDLE Handle,
                                        UINT32        TableIndex,
                                        UINT32        AppendixSizeInBytes,
                                        PVOID        PAppendix,
                                        PUINT32       PNumberOfValidBytes);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_Create(KDBM_DATABASE_HANDLE* PHandle, 
                              csx_pchar_t                DatabaseName,
                              UINT32                AppendixSizeInBytes,
                              PVOID                 PDatabaseAppendix,
                              UINT32                NumberOfTables,
                              PKDBM_TABLE_SPEC      TableSpecArray,
                              PEMCPAL_ALLOC_FUNCTION    PAlloc,
                              PEMCPAL_FREE_FUNCTION        PFree,
                              UINT32                CacheSize);

EXTERN_C EMCPAL_STATUS BvdSim_KDBM_AbortTransaction(KDBM_DATABASE_HANDLE Handle);

#define KDBM_Attach(a,b,c,d,e)  BvdSim_KDBM_Attach(a, b, c, d, e)

#define KDBM_Detach(a) BvdSim_KDBM_Detach(a)

#define KDBM_Delete(a,b,c) BvdSim_KDBM_Delete(a, b, c)

#define KDBM_AddTable(a, b, c) BvdSim_KDBM_AddTable(a, b, c)

#define KDBM_PutRecord(a, b, c, d, e) BvdSim_KDBM_PutRecord(a, b, c, d, e)

#define KDBM_GetRecord(a, b, c, d, e) BvdSim_KDBM_GetRecord(a, b, c, d, e)

#define KDBM_StartTransaction(a, b) BvdSim_KDBM_StartTransaction(a, b)

#define KDBM_CommitTransaction(a) BvdSim_KDBM_CommitTransaction(a)

#define KDBM_GetDBAppendix(a, b, c) BvdSim_KDBM_GetDBAppendix(a, b, c)

#define KDBM_PutDBAppendix(a, b, c) BvdSim_KDBM_PutDBAppendix(a, b, c)

#define KDBM_PutTableAppendix(a, b, c, d) BvdSim_KDBM_PutTableAppendix(a, b, c, d)

#define KDBM_GetTableAppendix(a, b, c, d, e) BvdSim_KDBM_GetTableAppendix(a, b, c, d, e)

#define KDBM_Create(a,b,c,d,e,f,g,h,i) BvdSim_KDBM_Create(a,b,c,d,e,f,g,h,i)

#define KDBM_AbortTransaction(a) BvdSim_KDBM_AbortTransaction(a)


EXTERN_C EMCPAL_STATUS BvdSim_SyncTransactionInitialize(TRANSACTION_DESCRIPTION * pTransDescArray,
                                   ULONG domainName,
                                   csx_pchar_t  mailboxName,
                                   ULONG mailboxNameLength,
                                   CHAR * syncLockName,
                                   TransactionGetTransNumber GetTransactionNumber,
                                   PVOID pContext
                                   );

EXTERN_C EMCPAL_STATUS BvdSim_DoSyncTransaction(ULONG transKey, CHAR * Params, ULONG ParamsLen, VOID * Context);

EXTERN_C VOID BvdSim_SyncTransactionCleanup();

EXTERN_C ULONG BvdSim_SyncGetCurrentTransactionNumber();

EXTERN_C TRANSACTION_DESCRIPTION * BvdSim_FindTransaction(ULONG transKey);



#define SyncTransactionInitialize(a,b,c,d,e,f,g) BvdSim_SyncTransactionInitialize(a,b,c,d,e,f,g)

#define DoSyncTransaction(a,b,c,d) BvdSim_DoSyncTransaction(a,b,c,d)

#define SyncTransactionCleanup() BvdSim_SyncTransactionCleanup()

#define SyncGetCurrentTransactionNumber() BvdSim_SyncGetCurrentTransactionNumber()

#define FindTransaction(a) BvdSim_FindTransaction(a)



#endif
#endif
