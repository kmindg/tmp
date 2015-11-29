//   cbfs_malloc.hxx
//
//  Copyright (C) 2008, All Rights Reserved, by
//  EMC Corporation, Hopkinton, Mass.
//
//  This software is furnished under a license and may be used and copied
//  only  in  accordance  with  the  terms  of such  license and with the
//  inclusion of the above copyright notice. This software or  any  other
//  copies thereof may not be provided or otherwise made available to any
//  other person. No title to and ownership of  the  software  is  hereby
//  transferred.
//
//  The information in this software is subject to change without  notice
//  and  should  not be  construed  as  a commitment by EMC Corporation.
//
//  EMC Corporation assumes no responsibility for the use or  reliability
//  of its software on equipment which is not supplied by EMC Corporation.
//
//
//
//  FACILITY:
//
//
//  ABSTRACT:
//
//
//  AUTHORS:
//
//
//  CREATION DATE:
//
//
//  MODIFICATION HISTORY:
//


#ifndef MEM_CACHE_H
#define MEM_CACHE_H


typedef 
PVOID ( *MEM_CACHE_ALLOCATION_FUNCTION ) (
    IN SIZE_T                                    NumBytes 
);


typedef 
VOID ( *MEM_CACHE_FREE_FUNCTION ) (
    IN PVOID                                    pBuffer 
);


//
//  Initialization / shutdown routines.
// 

EMCPAL_STATUS
MemCacheInit( 
    IN ULONG                                    MaxMemoryToManage,
    IN ULONG                                    MinMemorySizeToManage,
    IN ULONG                                    MaxMemorySizeToManage,
    IN MEM_CACHE_ALLOCATION_FUNCTION            pMemoryAllocator,
    IN MEM_CACHE_FREE_FUNCTION                  pFreeAllocator
);


void
MemCacheShutdown( void );


//
// Stationary memory routines.
// 

PVOID
MemCacheAllocate( 
    ULONG                                       BytesToAllocate
);


void
MemCacheFree(
    PVOID                                       pBufferToFree
);


//
//  Discretionary memory routines.
// 

EMCPAL_STATUS
MemCacheAllocateDiscretionaryMemory(
    IN  PVOID                                   MemBrokerClientHandle,
    IN  ULONG32                                 BytesToAllocate,
    OUT PVOID                                   *pVirtualAdd
);

EMCPAL_STATUS
MemCacheFreeDiscretionaryMemory(
    IN PVOID                                    MemBrokerClientHandle,
    IN PVOID                                    pBufferToFree
);

ULONG64
MemCacheAdjustSuggestedDiscUsageAmount(
    IN ULONG64                                  CurrentDiscretionaryAllocations,
    IN ULONG64                                  SuggestedDiscretionaryAllocations
);


//
//  Utility routines.
// 

void
MemCacheDumpStatistics( void );

#endif // MEM_CACHE_H
