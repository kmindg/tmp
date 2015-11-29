//**********************************************************************
//      Copyright (C) EMC Corporation, 2013
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//      MemMgr.h
//
// DESCRIPTION:
//      Functions for tracking pool usage in a driver and detecting
//      some kinds of buffer over- and under-runs and enforcing memory 
//      limits.
//
// REVISION HISTORY:
//      21-Jan-13  CBailey  Added LD DataPool interfaces and minor reformatting
//      12-Nov-07   NSeela  Modified MemMgrOnLongbow declaration to extern.
//                          MemMgrOnLongbow is defined in MemMgr.c
//      04-Mar-05  kagarwal Added interfaces for querying memory usage stats.
//      13-Feb-02  pmcgrath Added MemMgrGetCurrentUsage to help detect
//                          memory leaks in layered drivers.
//      01-Dec-01  nfaridi  Added MemMgrGetControlPoolClientID() proto
//      18-Oct-01  nfaridi  Removed commented out function header for
//                          MemMgrGetDataPoolClientID()
//                          Removed function header for 
//                          MemMgrSetIfLongbow().
//                          Removed MEMMGR_ON_LOGBOW.
//      23-May-01  nfaridi  Added CMM Interface changes to MemMgr.
//      29-Jan-01  nfaridi  Modified and reformated for MemManager.
//      14-Mar-00  PTM      Updated file for enforcing memory limits 
//                          for NPP allocations, also reformatted file.
//      ---------           Based on the source created by Mark 
//                          Russinovich, http://www.sysinternals.com.
//.--                                                                    
//**********************************************************************

#ifndef _MEMMGR_H_
#define _MEMMGR_H_

#include "cmm.h"
#include "flare_sgl.h"
#include "EmcPAL.h"


//**********************************************************************
//.++
// CONSTANT:
//      MEMMGR_MAX_MEM
//
// DESCRIPTION:
//      Used as input when allcoating a memory pool to indicate that
//      the client wants to only be restricted on memory allocation
//      by the total amount of memory in memory pool from which the
//      allocation is being requested.
//
// REVISION HISTORY:
//      23-May-01  nfaridi      Created.
//.--
//**********************************************************************

#define MEMMGR_MAX_MEM                                -1

//**********************************************************************
//.++
// GLOBAL VARIABLES:
//      MemMgrOnLongbow
//
// DESCRIPTION:
//      This variable is used to findout if the system is running on 
//      Longbow hardware or not. On Longbow, it is set to TRUE and on 
//      Chamapult, it is set to FALSE.
//
// REVISION HISTORY:
//      09-Jul-01  nfaridi      Changed the type to boolean.
//      23-May-01  nfaridi      Created.
//.--
//**********************************************************************

extern BOOLEAN MemMgrOnLongbow;


//**********************************************************************
//.++
// TYPE:
//      MEMMGR_MODE
//
// DESCRIPTION:
//      Modes that MemMgr can function in. 
//
//
// MEMBERS:
//      MemMgrNormalMode        Both shared and local values are tracked
//      MemMgrMemTrackMode      Only local values are tracked
//      MemMgrDualMode          Depends on the pool type which values 
//                              are updated, local, global or both
// REMARKS
//      This type and its usage will be removed once the MemMgr is 
//      completely developed.
//
// REVISION HISTORY:
//      30-Jan-01  nfaridi      Created.
//.--
//**********************************************************************
typedef enum _MEMMGR_MODE
{
    MemMgrNormalMode,
    MemMgrMemTrackMode, 
    MemMgrDualMode

} MEMMGR_MODE;

//**********************************************************************
//.++
// TYPE:
//      MEMMGR_MEM_POOL_TYPE
//
// DESCRIPTION:
//      Type of pool to use with CMM. 
//
//
// MEMBERS:
//      MEMMGR_FEATURE_POOL     Use feature pool, shared among feature
//                              drivers only.
//      MEMMGR_NON_FEATURE_POOL Only local values are tracked
//      MEMMGR_NONPAGED_FEATURE_POOL:
//      MEMMGR_LAYERED_DATA_POOL
//
// REMARKS
//      This type and its usage will be removed once the MemMgr is 
//      completely developed.
//
// REVISION HISTORY:
//      18-Nov-01  nfaridi      Created.
//      18-Jan-13  cbailey      Added MEMMGR_LAYERED_DATA_POOL
//.--
//**********************************************************************
typedef enum _MEMMGR_MEM_POOL_TYPE
{
    MEMMGR_FEATURE_POOL         = 0x0,
    MEMMGR_NON_FEATURE_POOL,
    MEMMGR_NONPAGED_FEATURE_POOL,
    MEMMGR_LAYERED_DATA_POOL
    
} MEMMGR_MEM_POOL_TYPE;

//**********************************************************************
//.++
// TYPE:
//      MEMMGR_POOL_TYPE
//
// DESCRIPTION:
//      Pool Types used in MemMgr. They are a multiple of NT Pool Types
//      and carries some more information about the mode of MemMgr and
//      type of memory requested, buffer or control.
//
// REMARKS
//      This type and its usage will be removed once the MemMgr is
//      completely developed.
//
// REVISION HISTORY:
//      30-Jan-01  nfaridi      Created.
//.--
//**********************************************************************
typedef enum _MEMMGR_POOL_TYPE
{
    NonPagedPoolBuffer = EmcpalMaxPoolType + 1,
    NonPagedPoolCacheAlignedBuffer,
    PagedPoolBuffer,
    PagedPoolCacheAlignedBuffer,
    NonPagedPoolControl,
    NonPagedPoolCacheAlignedControl,
    PagedPoolControl,
    PagedPoolCacheAlignedControl,
    NonPagedPoolBufferUnconditional,
    NonPagedPoolCacheAlignedBufferUnconditional,
    NonPagedPoolMustSucceedBufferUnconditional,
    NonPagedPoolCacheAlignedMustSBufferUnconditional,
    PagedPoolBufferUnconditional,
    PagedPoolCacheAlignedBufferUnconditional,
    NonPagedPoolControlUnconditional,
    NonPagedPoolCacheAlignedControlUnconditional,
    NonPagedPoolMustSucceedControlUnconditional,
    NonPagedPoolCacheAlignedMustSControlUnconditional,
    PagedPoolControlUnconditional,
    PagedPoolCacheAlignedControlUnconditional,
    MemTrkNonPagedPoolBuffer,
    MemTrkNonPagedPoolCacheAlignedBuffer,
    MemTrkPagedPoolBuffer,
    MemTrkPagedPoolCacheAlignedBuffer,
    MemTrkNonPagedPoolControl,
    MemTrkNonPagedPoolCacheAlignedControl,
    MemTrkPagedPoolControl,
    MemTrkPagedPoolCacheAlignedControl,
    MemTrkNonPagedPoolBufferUnconditional,
    MemTrkNonPagedPoolCacheAlignedBufferUnconditional,
    MemTrkNonPagedPoolMustSucceedBufferUnconditional,
    MemTrkNonPagedPoolCacheAlignedMustSBufferUnconditional,
    MemTrkPagedPoolBufferUnconditional,
    MemTrkPagedPoolCacheAlignedBufferUnconditional,
    MemTrkNonPagedPoolControlUnconditional,
    MemTrkNonPagedPoolCacheAlignedControlUnconditional,
    MemTrkNonPagedPoolMustSucceedControlUnconditional,
    MemTrkNonPagedPoolCacheAlignedMustSControlUnconditional,
    MemTrkPagedPoolControlUnconditional,
    MemTrkPagedPoolCacheAlignedControlUnconditional,
    MemMgrMaxPoolType

} MEMMGR_POOL_TYPE;



//**********************************************************************
//.++
// TYPE:
//      MEMMGR_MEMORY_SIZE
//
// DESCRIPTION:
//      MEMMGR_MEMORY_SIZE abstracts the data type used by CMM to return 
//      memory size.
//
// REMARKS
//      None
//      
// REVISION HISTORY:
//      28-Feb-05  kagarwal Created.
//.--
//**********************************************************************
typedef ULONG64  MEMMGR_MEMORY_SIZE ;


//**********************************************************************
//.++
// START of the functions that will be removed once the 
// MemMgr developement is complete.

PVOID
MemMgrAllocatePoolWithTag(   IN MEMMGR_POOL_TYPE        PoolType,
                             IN ULONG                   NumberOfBytes,
                             IN ULONG                   Tag );
PVOID
MemMgrAllocatePool(          IN MEMMGR_POOL_TYPE        PoolType,
                             IN ULONG                   NumberOfBytes );
VOID
MemMgrFreePool(              IN PVOID                   pBuffer );


EMCPAL_STATUS
MemMgrMaxAllowed(            IN ULONG                   MaxMemoryAllowed );

EMCPAL_STATUS
MemMgrMaxUnused(             IN PULONG                  pNumberOfBytes );

EMCPAL_STATUS
MemMgrGetCurrentUsage(       IN PULONG                  pNumberOfBytes );

//  The following are clients of MemMgr and the parameters they use
//  to initialize MemMgr.
//
//      MPS:        '1spM', 0xa0, 0xa1, 0xf0
//      SnapCopy:   'rDcS', 0xb0, 0xb1, 0xf1
//      RMD:        '1dmR', 0xc0, 0xc1, 0xf2
//      DLS:        'MslD', 0xd0, 0xd1, 0xf3
//      DlsDriver:  'DslD', 0xe0, 0xe1, 0xf4
//      CopyMgr:    'DmpC', 0xf0, 0xf1, 0xf5 

VOID
MemMgrInit( IN ULONG                        Tag,
            IN UCHAR                        Allocate,
            IN UCHAR                        AllocateWithTag,
            IN UCHAR                        Free,
            IN MEMMGR_MODE                  Mode );

// END of the functions that will be removed once the 
// MemMgr developement is complete.
//.--
//**********************************************************************

EMCPAL_STATUS
MemMgrInitLongbowGlobals(    IN ULONG                   Tag,
                             IN UCHAR                   Allocate,
                             IN UCHAR                   AllocateWithTag,
                             IN UCHAR                   Free );
VOID
MemMgrSetEmcpalClientPtr (   IN  PEMCPAL_CLIENT         inEmcpalClientPtr);

VOID
MemMgrCheckForCorruption(    IN PVOID                   pBuffer );

VOID
MemMgrPrintStats(            IN PCHAR                   pText);

EMCPAL_STATUS
MemMgrIncrementCurrentUsage( IN ULONG                   NumberOfBytes,
                             IN MEMMGR_POOL_TYPE        PoolType );

EMCPAL_STATUS
MemMgrDecrementCurrentUsage( ULONG                      NumberOfBytes,
                             IN MEMMGR_POOL_TYPE        PoolType );

// This interface is used to setup all of the control pool types
EMCPAL_STATUS
MemMgrControlPoolInit(IN MEMMGR_MEMORY_SIZE MinBytesToUse,
                      IN MEMMGR_MEMORY_SIZE MaxBytesToUse,
                      IN PCHAR pClientName,
                      IN MEMMGR_MEM_POOL_TYPE MemPoolType );

//********************************************************
//
// Control Pool Interfaces:
// These interfaces implement access to the LD control pool
//
//********************************************************

CMM_CLIENT_ID
MemMgrGetControlPoolClientID(VOID);

// Called to unregister as a client of the LD control pool
VOID
MemMgrControlPoolDeregisterClient(VOID);

// Called to allocate memory from the LD control pool
PVOID
MemMgrAllocateControlPool(IN SIZE_T NumBytes);

// Called to free memory from the LD control pool
VOID
MemMgrFreeControlPool(IN PVOID pVirtualAdd);

// Called to size of the LD control pool
EMCPAL_STATUS 
MemMgrControlPoolGetPoolSize(OUT MEMMGR_MEMORY_SIZE * pSizeInBytes);

// Called to get the amount of free memory in the LD control pool
EMCPAL_STATUS
MemMgrControlPoolGetSizeOfAvailableMemory(OUT MEMMGR_MEMORY_SIZE *pAvailableMemoryInBytes);

// Called to determine the largest piece of virtually contiguous memory in the
// LD control pool
MEMMGR_MEMORY_SIZE
MemMgrControlPoolGetMaxContiguousMemory(VOID);

// Called to determine the number of free allocation units in the LD control
// pool
ULONG64
MemMgrControlPoolGetNumberOfFreeEntriesInPool(VOID);

// Called to determine the allocated size of a piece of memory.
MEMMGR_MEMORY_SIZE
MemMgrControlPoolGetAllocatedMemorySize(PVOID pVirtualAddress);

// Called to determine the range of possible virtual address out of
// the LD control pool
VOID
MemMgrControlPoolGetPoolVirtualAddressRange(OUT PVOID *pStartAddress,
                                            OUT PVOID *pEndAddress);

// Called to determine the amount of memory that a specific client has allocated in the
// LD control pool
EMCPAL_STATUS 
MemMgrControlPoolGetSizeClientAllocatedMemory(OUT MEMMGR_MEMORY_SIZE *pClientAllocatedMemoryInBytes);

//********************************************************
// 
// Data Pool Interfaces
// These interfaces implement access to the LD Data pool
//
//********************************************************
EMCPAL_STATUS
MemMgrDataPoolInit(IN MEMMGR_MEMORY_SIZE MinBytesToUse,
                   IN MEMMGR_MEMORY_SIZE MaxBytesToUse,
                   IN PCHAR pClientName );
CMM_CLIENT_ID
MemMgrDataPoolGetClientID( VOID );

// Called to unregister as a client of the LD data pool
VOID
MemMgrDataPoolDeregisterClient(VOID);

// Called to allocate memory from the LD data pool
PVOID
MemMgrAllocateDataPool(IN MEMMGR_MEMORY_SIZE NumBytes);

// Called to free memory from the LD data pool
EMCPAL_STATUS
MemMgrFreeDataPool(IN PVOID PVa);

// Called to size of the LD data pool
EMCPAL_STATUS 
MemMgrDataPoolGetPoolSize(OUT MEMMGR_MEMORY_SIZE * pSizeInBytes);

// Called to get the amount of free memory in the LD data pool
EMCPAL_STATUS 
MemMgrDataPoolGetSizeOfAvailableMemory(OUT   MEMMGR_MEMORY_SIZE *pAvailableMemoryInBytes);

// Called to determine the largest piece of virtually contiguous memory in the
// LD data pool
MEMMGR_MEMORY_SIZE
MemMgrDataPoolGetMaxContiguousMemory(VOID);

// Called to determine the number of free allocation units in the LD control
// pool
ULONG64
MemMgrDataPoolGetNumberOfFreeEntriesInPool(VOID);

// Called to determine the allocated size of a piece of memory.
MEMMGR_MEMORY_SIZE
MemMgrDataPoolGetAllocatedMemorySize(PVOID pVirtualAddress);

// Called to determine the range of possible virtual address out of
// the LD data pool
VOID
MemMgrDataPoolGetPoolVirtualAddressRange(OUT PVOID *PStartAddress,
                                         OUT PVOID *PEndAddress);

// NonPaged Pool Interfaces
PVOID
MemMgrAllocateNonPagedControlPool(IN ULONG NumBytes );
VOID
MemMgrFreeNonPagedControlPool(IN PVOID pVirtualAdd );

//  We define all of the lookaside list functions to use the native NT versions
//  and encourage the callers to use MemMgrAllocatePool() and MemMgrFreePool() for
//  the allocation and free functions.
#define MemMgrInitializeNPagedLookasideList(_Lookaside, _Allocate, _Free, _Flags, _Size, _Tag, _Depth) \
    EmcpalLookasideListCreate(EmcpalDriverGetCurrentClientObject(), _Lookaside, _Allocate, _Free, _Size, _Depth, _Tag)
#define MemMgrDeleteNPagedLookasideList             EmcpalLookasideListDelete 
#define MemMgrAllocateFromNPagedLookasideList       EmcpalLookasideAllocate 
#define MemMgrFreeToNPagedLookasideList             EmcpalLookasideFree 

#endif // _MEMMGR_H_
