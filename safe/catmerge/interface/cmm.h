#ifndef CMM_H
#define CMM_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file defines the functional interface to CMM.
 *   
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *     30-Jul-2007     Control PP  Ian McFadries
 *
 *
 ***************************************************************************/

#include "cmm_generics.h"
#include "cmm_types.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
#include "specl_types.h"

CSX_CDECLS_BEGIN

#if !defined(UMODE_ENV)
#ifdef _CMM_
#define CMMAPI CSX_MOD_EXPORT
#else
#define CMMAPI CSX_MOD_IMPORT
#endif
#else
#define CMMAPI
#endif
/*************************  E X P O R T E D   I N T E R F A C E   F U N C T I O N S  ***************************/

CMMAPI CMM_ERROR cmmCancelDeferredRequest (
                    CMM_CLIENT_ID   clientId,
                    CMM_REQUEST     *request);

CMMAPI void cmmCancelDeferredRequestWithCallback (
                    CMM_CLIENT_ID   clientId,
                    CMM_REQUEST     *request);

CMMAPI CMM_ERROR cmmCreatePool (
                    CMM_CLIENT_ID           clientIdMetaData,
                    CMM_CLIENT_ID           clientIdPoolMemory,
                    CMM_MEMORY_SIZE         allocationUnitSize,
                    UINT_32                 numberOfClients,
                    CMM_MEMORY_SIZE         sizeOfPool,
                    CMM_ALLOC_METHOD        allocMethod,
                    CMM_POOL_ID             *poolIdPtr);

CMMAPI CMM_ERROR cmmDeregisterClient (
                    CMM_CLIENT_ID   clientId);

CMMAPI CMM_ERROR    cmmDestroyPool (
                    CMM_CLIENT_ID   clientIdMetaData,
                    CMM_CLIENT_ID   clientIdPoolMemory,
                    CMM_POOL_ID     poolId);

CMMAPI CMM_ERROR    cmmFreeMemorySGL (CMM_CLIENT_ID clientId,
                    PCMM_SGL            memorySGL);

CMMAPI CMM_ERROR    cmmFreeMemoryVA (   CMM_CLIENT_ID   clientId,
                    PVOID           memoryVA);

CMMAPI CMM_MEMORY_SIZE
                    cmmGetMemorySize (CMM_CLIENT_ID   clientId,
                                      PVOID           memoryVA);

CMMAPI  CMM_MEMORY_SIZE cmmGetFreeMemoryInPool (CMM_POOL_ID poolId);

CMMAPI  CMM_MEMORY_SIZE cmmGetClientAllocatedMemory (CMM_CLIENT_ID  clientId);

CMMAPI  CMM_MEMORY_SIZE cmmGetMaxContiguousMemoryInPool (
                            CMM_POOL_ID     poolId);

CMMAPI  CMM_MEMORY_SIZE cmmGetFreeMemoryNumberOfEntries (
                            CMM_POOL_ID     poolId);

CMMAPI CMM_POOL_ID  cmmGetLongTermControlPoolID (void);
CMMAPI CMM_POOL_ID  cmmGetLongTermControlPagedPoolID (void);
CMMAPI CMM_POOL_ID  cmmGetLongTermDataPoolID (void);
CMMAPI CMM_POOL_ID  cmmGetLocalOnlyDataPoolID (void);
CMMAPI CMM_POOL_ID  cmmGetSEPDataPoolID (void);
CMMAPI CMM_POOL_ID  cmmGetLayeredDataPoolID (void);


CMMAPI void cmmGetMemoryAndCallbackSGL (
            CMM_REQUEST             *request);

CMMAPI CMM_ERROR    cmmGetMemoryOrDeferSGL (
            CMM_REQUEST             *request,
            PCMM_SGL                    *memorySGL);

CMMAPI CMM_ERROR    cmmGetMemorySGL (
            CMM_CLIENT_ID           clientId,
            CMM_MEMORY_SIZE             numberOfBytes,
            PCMM_SGL                    *memorySGL);

CMMAPI CMM_ERROR    cmmGetSpecificMemorySGL (
            CMM_CLIENT_ID           clientId,
            PCMM_SGL                    memorySGL);

CMMAPI CMM_ERROR    cmmGetMemoryVA (
            CMM_CLIENT_ID           clientId,
            CMM_MEMORY_SIZE         numberOfBytes,
            PVOID                   *memoryVA);

CMMAPI SGL_PADDR    cmmGetPhysAddrFromVA (PVOID memoryVA);

CMMAPI PCMM_SGL cmmGetPoolSGL (
                        CMM_POOL_ID             poolId);

CMMAPI PVOID  cmmGetContigCntrlPoolVirtAddrFromPhysAddr (CMM_MEMORY_ADDRESS physAddr);

CMMAPI CMM_MEMORY_POOL_TYPE cmmGetPoolType (
                        CMM_POOL_ID     poolId);

CMMAPI CMM_POOL_ID cmmGetMemoryPool(CMM_MEMORY_ADDRESS PhysAddress);

CMMAPI CMM_MEMORY_SIZE  cmmGetTotalMemoryInPool (
                        CMM_POOL_ID     poolId);

CMMAPI CMM_MEMORY_SIZE  cmmGetTotalPhysicalMemory(void);

CMMAPI CMM_ERROR cmmInitializeRequestSGL (
                        CMM_CLIENT_ID       clientId,
                        CMM_MEMORY_SIZE     numberOfBytes,
                        CMM_CALLBACK_SGL        callbackSGL,
                        CMM_REQUEST         *requestPtr);

CMMAPI CMM_ERROR cmmMapMemorySVA (
                CMM_MEMORY_ADDRESS           PhysAddress,
                CMM_MEMORY_SIZE     PhysLength,
                 PVOID           *virtualAddress,
                csx_p_io_map_t      *pio_map_obj);

CMMAPI CMM_ERROR cmmMapMemoryUVA (
                SGL_PADDR           PhysAddress,
                SGL_LENGTH          PhysLength,
                PVOID           *virtualAddress);

CMMAPI CMM_ERROR cmmQueryCmm (CMM_POOL_ID   poolIdArray[],
                 UINT_32        poolIdArrayEntries,
                 UINT_32        *numberOfPools);

CMMAPI CMM_ERROR cmmRegisterClient (
            CMM_POOL_ID             poolId,
            CMM_MEMORY_RETURN_REQUEST_CALLBACK  freeCallback,
            CMM_MEMORY_SIZE         minimumAllocSize,
            CMM_MEMORY_SIZE         maximumAllocSize,
            TEXT                    *clientName,
            CMM_CLIENT_ID           *clientIdPtr);

CMMAPI CMM_ERROR cmmUnMapMemorySVA (csx_p_io_map_t *pio_map_obj);

#ifdef ALAMOSA_WINDOWS_ENV
CMMAPI CMM_ERROR cmmUnMapMemoryUVA (
                    PVOID           virtualAddress);
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */

CMMAPI CMM_ERROR  cmmGetPoolAddressRange(
                            CMM_POOL_ID     poolId,
                            PVOID           *startAddress,
                            PVOID           *endAddress);


CMMAPI void cmmSecondaryDumpData(PVOID Address, ULONG Length, char *name);

/*************************  I N T E R N A L   F U N C T I O N S  ***************************/

// For now these are only internal functions, they provide services not currently needed
// and a level of detail we don't want to expose at this time.  They may be exported
// at a later date.

IMPORT CMM_ERROR    cmmCreatePoolFromMemDesc (
                        PVOID                       metaDataMemory,
                        CMM_MEMORY_SIZE             allocationUnitSize,
                        UINT_32                     numberOfClientDescriptors,
                        CMM_MEMORY_SIZE             totalPoolSize,
                        CMM_ALLOC_METHOD            allocMethod,
                        struct cmmMemoryDescriptor  *memoryToManage,
                        CMM_POOL_ID                 *poolIdPtr);

IMPORT CMM_ERROR    cmmDestroyPoolNoFree (CMM_POOL_ID   poolId);

IMPORT CMM_ERROR    cmmGetMemory(
                        CMM_CLIENT_ID               clientId,
                        CMM_MEMORY_SIZE             numberOfBytes,
                        CMM_MEMORY_ADDRESS          physAddressRequested,
                        struct cmmMemoryDescriptor  **memoryDescPtrPtr);

IMPORT void         cmmGetMemoryAndCallbackVA (
                        CMM_REQUEST             *request);

IMPORT CMM_ERROR    cmmGetMemoryOrDeferVA (
                        CMM_REQUEST             *request,
                        PVOID                   *memoryVA);

IMPORT CMM_MEMORY_SIZE  cmmGetPoolAllocSize (
                        CMM_POOL_ID     poolId);

IMPORT  CMM_ERROR   cmmInitializeRequestVA (
                            CMM_CLIENT_ID       clientId,
                            CMM_MEMORY_SIZE     numberOfBytes,
                            CMM_CALLBACK_VA     callbackVA,
                            CMM_REQUEST         *requestPtr);

CSX_CDECLS_END

#endif      /* ifndef CMM_H */
