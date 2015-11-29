//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
/*!
 * @file EmcPAL_MemoryAlloc.h
 * @addtogroup emcpal_memory
 * @{
 *
 * @brief
 *      This file contains Memory allocation/free routines and defines.
 *
 *      See CSX documentation for a discussion of utility memory vs. I/O memory.
 *      Lookaside lists are always allocated as utility memory.
 *
 * @version
 *      2/18/2011 M.Hamel - Created.
 *
 */

#ifndef _EMCPAL_MEMORYALLOC_H_
#define _EMCPAL_MEMORYALLOC_H_

#include "EmcPAL.h"

#ifdef __cplusplus
extern "C" {
#endif

//++
// End Includes
//--

//**********************************************************************
//
// Utility Memory Structures, Data types, etc.
//
    
/*! @brief Pool type for EmcPAL Utility memory allocation APIs, meaningful only on Windows */
/*! @note this enumerated type is designed to shadow the DDK POOL_TYPE enum 
 * so it should not be altered
 */
#ifdef EMCPAL_CASE_WK
#ifdef EMCPAL_CASE_NTDDK_EXPOSE
typedef int EMCPAL_POOL_TYPE;
#else
typedef POOL_TYPE EMCPAL_POOL_TYPE;
#endif
#define EmcpalNonPagedPool NonPagedPool
#define EmcpalPagedPool PagedPool
#define EmcpalNonPagedPoolMustSucceed NonPagedPoolMustSucceed
#define EmcpalDontUseThisType DontUseThisType
#define EmcpalNonPagedPoolCacheAligned NonPagedPoolCacheAligned
#define EmcpalPagedPoolCacheAligned PagedPoolCacheAligned
#define EmcpalNonPagedPoolCacheAlignedMustS NonPagedPoolCacheAlignedMustS
#define EmcpalMaxPoolType MaxPoolType
#else
typedef enum
{
    EmcpalNonPagedPool,
    EmcpalPagedPool,
    EmcpalNonPagedPoolMustSucceed,
    EmcpalDontUseThisType,
    EmcpalNonPagedPoolCacheAligned,
    EmcpalPagedPoolCacheAligned,
    EmcpalNonPagedPoolCacheAlignedMustS,
    EmcpalMaxPoolType
} EMCPAL_POOL_TYPE;
#endif

#ifdef EMCPAL_CASE_WK
#ifdef EMCPAL_CASE_NTDDK_EXPOSE
typedef void *(*PEMCPAL_ALLOC_FUNCTION) (EMCPAL_POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
typedef void  (*PEMCPAL_FREE_FUNCTION) (void *Buffer);
#else
typedef PALLOCATE_FUNCTION PEMCPAL_ALLOC_FUNCTION;
typedef PFREE_FUNCTION PEMCPAL_FREE_FUNCTION;
#endif
#else
typedef void *(*PEMCPAL_ALLOC_FUNCTION) (EMCPAL_POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
typedef void  (*PEMCPAL_FREE_FUNCTION) (void *Buffer);
#endif

/*! @brief EmcPAL lookaside struct
 *  @note this structure is opaque to callers; they allocate it but do not fill it in
 */
typedef struct {
    csx_nuint_t Tag;										/*!< Lookaside tag */
    PEMCPAL_ALLOC_FUNCTION AllocFunction; /*!< Optional caller-specified function to be called to allocate memory if list is exhausted */
    PEMCPAL_FREE_FUNCTION FreeFunction; /*!< Optional caller-specified function to be called to free memory if list is exhausted */
#ifdef EMCPAL_USE_CSX_MEMORY_ALLOC
    csx_p_lookaside_list_t LookasideList;					/*!< Lookaside list */
#else
    NPAGED_LOOKASIDE_LIST LookasideList;					/*!< Lookaside list */
#endif
} EMCPAL_LOOKASIDE, *PEMCPAL_LOOKASIDE;						/*!< Ptr to EmcPAL lookaside struct */

/***********************************************************************
 *
 * @brief
 * Lookaside lists
 *
 * Callers must:
 * <ol>
 * <li> Allocate an EMCPAL_LOOKASIDE structure (this structure is opaque to callers; do NOT touch any of the fields inside this structure)</li>
 * <li> Call EmcpalLookasideListCreate to initialize/create the list</li>
 * <li> If caller wishes to specify their own allocation/free functions, they 
 *     must be prototypes of the form EMCPAL_ALLOC_FUNCTION and EMCPAL_FREE_FUNCTION</li>
 * <li> Then EmcpalLookasideAllocate and EmcpalLookasideFree can be called repeatedly to alloc/free from the list</li>
 * <li> EmcpalLookasideListDelete can be used to tear down the list</li>
 * </ol>
 */
/*! @brief Create a Lookaside list
 *  @param pPalClient Pointer to client object
 *  @param pLookaside Pointer to caller-allocated Lookaside structure
 *  @param AllocFunction Optional pointer to caller-provided allocation function
 *  @param FreeFunction Optional pointer to caller-provided free function
 *  @param SizeOfEachElementInBytes Size of each allocation element in the lookaside list; 
 *		   callers who then call EmcpalLookasideAllocate() will obtain a pointer to a buffer of this size
 *  @param Depth Suggested depth of list (number of allocation elements to keep on hand). This is currently ignored
 *  @param Tag Four character pool tag
 *  @return none
 */
EMCPAL_API void EmcpalLookasideListCreate(PEMCPAL_CLIENT pPalClient,
                                          PEMCPAL_LOOKASIDE pLookaside,
                                          PEMCPAL_ALLOC_FUNCTION AllocFunction,
                                          PEMCPAL_FREE_FUNCTION FreeFunction,
			                              SIZE_T SizeOfEachElementInBytes,
			                              UINT_16 Depth,
			                              UINT_32 Tag);

/*! @brief Returns a pointer to a nonpaged entry from the given lookaside list, or 
 *         it returns a pointer to a newly allocated nonpaged entry
 *  @param pLookaside Pointer to the lookaside-list structure, which the caller already initialized
 *  @return Pointer to an lookaside entry if one can be allocated. Otherwise, it returns NULL
 */
EMCPAL_API void * EmcpalLookasideAllocate(PEMCPAL_LOOKASIDE pLookaside);

/*! @brief Routine returns a nonpaged entry to the given lookaside list or to nonpaged pool
 *  @param pLookaside Pointer to the lookaside-list structure, which the caller already initialized
 *  @param Buffer Pointer to the entry to be freed
 *  @return none
 */
EMCPAL_API void EmcpalLookasideFree(PEMCPAL_LOOKASIDE pLookaside, void *Buffer);

/*! @brief Delete a nonpaged lookaside list
 *  @param pLookaside Pointer to the lookaside-list structure, which the caller already initialized
 *  @return none
 */
EMCPAL_API void EmcpalLookasideListDelete(PEMCPAL_LOOKASIDE pLookaside);

//**********************************************************************
//
// Utility Memory allocation primitives
//
//


/*! @brief Allocates a buffer of utility memory
 *         See CSX documentation for a discussion of utility memory vs. I/O memory
 *  @param PoolType Specifies the type of pool memory to allocate
 *  @param NumberOfBytes Specifies the number of bytes to allocate
 *  @param Tag Specifies the pool tag for the allocated memory
 *  @return Pointer to allocated memory or NULL if memory could not be allocated
 */
EMCPAL_API void* EmcpalAllocateUtilityPoolWithTag(EMCPAL_POOL_TYPE PoolType, SIZE_T NumberOfBytes, UINT_32 Tag);

EMCPAL_API void EmcpalFreeUtilityPoolWithTag(void* Buffer, UINT_32 Tag);


/*
 * For now, I'm not supporting NonPagedPoolCacheAligned in the normal routines.
 * This is only used in a few places, and I'd rather just do a special wrapper
 * for now, for future identification purposes, until CSX supports this
 * functionality.
 *
 * See \services\cmiscd\src\cmiscd_init.c - I think this needs to use I/O Memory, not Utility Memory
 *
 * Other users, not including utility functions that translate pool type into an ASCII string for printing:
 * flare\ntbe\ntbetest.c (not being ported)
 * FSL\FSLDisk\ (not being ported)
 * ntmirror (not being ported, and I think normal nonpaged pool would be ok))
 * \tools\cpdcd\cpdcd\cpdcd_object.c (listed as "questionable" for being ported, and I think normal nonpaged pool would be ok)
 */
/*!
 * @brief
 *   Special-purpose function for a Windows-specific allocation use case.
 *
 * @note
 *   Not truly portable on CSX. Avoid using this API in new code.
 */
EMCPAL_API void* EmcpalAllocateUtilityPoolWithTagCacheAligned(
					SIZE_T NumberOfBytes,	/*!< Number of bytes to allocate */
					UINT_32 Tag			/*!< Four character pool tag */
                    );
/*!
 *  @brief
 *       Allocates a buffer of utility memory.
 *       See CSX documentation for a discussion of utility memory vs. I/O memory.
 * 
 *  @deprecated
 *	Please use the EmcpalAllocateUtilityPoolWithTag() function in new code 
 *
 *  @pre
 *	Missing detail
 *
 *  @post
 *  Missing detail
 *
 * @note Non-tagged version. Usage of this API is discouraged; please use the tagged APIs when writing new code.
 *
 *  @return
 *        Pointer to allocated memory or NULL if memory could not be allocated.
 *       
 *  @version
 *       2/10/2011  M. Hamel Created.
 */
EMCPAL_API void* EmcpalAllocateUtilityPool(EMCPAL_POOL_TYPE PoolType, SIZE_T NumberOfBytes);

/*!
 *  @brief
 *       Frees a buffer of utility memory.
 *       Note that it is legal to use this function to free memory allocated by
 *       EmcpalAllocateUtilityPool() or EmcpalAllocateUtilityPoolWithTag().
 * 
 *  @note
 *       See CSX documentation for a discussion of utility memory vs. I/O memory.
 * 
 *  @pre
 *	Missing detail
 *
 *  @post
 *  Missing detail
 *
 *  @return
 *        Nothing.
 *       
 *  @version
 *       2/10/2011  M. Hamel Created.
 */
EMCPAL_API void EmcpalFreeUtilityPool(void *Buffer);

//**********************************************************************
//
// I/O Memory Structures, Data types, etc.
//
//
/*!
 *  @brief
 *       Enumerated type designed to shadow the Windows options for allocating "contigous memory"
 */
typedef enum
{
    EMCPAL_IOMEM_GENERIC = 0,
    EMCPAL_IOMEM_CONTIGUOUS, 
    EMCPAL_IOMEM_CONTIGUOUS_SPECIFY_CACHE,
    EMCPAL_IOMEM_CACHE_ALIGNED
} EMCPAL_IOMEM_TYPE;

/*!
 *  @brief
 *       Tracking structure for EmcPAL I/O Memory allocations
 *  @note
 *       this structure is opaque to callers; they allocate it but do not fill it in
 */
typedef struct
{
    EMCPAL_IOMEM_TYPE Type;								/*!< IO-memory type */
    csx_virt_address_t VirtualStartingAddress;			/*!< Virtual starting address */
    csx_phys_length_t NumberOfBytes;					/*!< Number of bytes */
#ifdef EMCPAL_USE_CSX_IOMEMORY
    csx_phys_address_t PhysicalStartingAddress;			/*!< Physical starting address */
    csx_p_io_mem_t pCsxMemoryObject;					/*!< Memory object */
#endif
} EMCPAL_IOMEMORY_OBJECT, *PEMCPAL_IOMEMORY_OBJECT;		/*!< Ptr to EmcPAL IO-memory object */

//**********************************************************************
//
// I/O Memory allocation primitives
//
//

/*! @brief Allocates a buffer of IO Memory.
 *  @note IMPORTANT NOTE: This API can NOT be called at elevated IRQL, such as Dpc/Dispatch or interrupt level,
 *        due to the requirements introduced by the CSX IO Memory API.
 *
 *      IO Memory is required if you need to obtain the physical address
 *      of the memory. For example, if you are using the memory to communicate 
 *      directly with hardware, you need to use IO memory.
 *
 *      See CSX documentation for a deeper discussion of utility memory vs. I/O memory.
 *      In general, IO memory can be obtained using csx_p_io_mem_... APIs or
 *      by using the csx_p_bufman_... APIs.
 * 
 *  @param pPalClient CSX client pointer
 *  @param pMemoryObject Opaque tracking structure that must be allocated by the caller and then 
 *         freed after the memory buffer is freed 
 *  @param Type An EMCPAL_MEM_TYPE, ignored on CSX, but used on Windows for fidelity with pre-existing code
 *  @param NumberOfBytes Number of bytes to allocate
 *  @param WinTag A standard four byte Windows memory tag, ignored on CSX
 *  @param pDescriptionString Text string to identify this allocation; ignored for non-CSX
 *  @param Cached TRUE/FALSE memory cacheable attribute
 *  @param RestrictToBelow4GB TRUE/FALSE to restrict memory to physical memory below 4GB
 *  @return Virtual address of allocated memory buffer, or NULL if unsuccessful.
 */

EMCPAL_API void* EmcpalAllocateIoMemory(PEMCPAL_CLIENT pPalClient, 
                                        PEMCPAL_IOMEMORY_OBJECT pMemoryObject,  
                                        EMCPAL_IOMEM_TYPE Type, 
                                        SIZE_T NumberOfBytes, 
                                        UINT_32 WinTag,
                                        char *pDescriptionString,
                                        BOOL Cached, 
                                        BOOL RestrictToBelow4GB);

/*! @brief Frees a buffer of IO Memory allocated by a previous call to EmcpalAllocateIoMemory()
 *  @param pMemoryObject Opaque tracking structure that was passed to EmcpalAllocateIoMemory when this 
 *         buffer was allocated. The caller is responsible for freeing this structure after this function returns
 *  @param BufferToFree Pointer to the buffer to be freed (virtual address)
 *  @return none
 */
EMCPAL_API void EmcpalFreeIoMemory(PEMCPAL_IOMEMORY_OBJECT pMemoryObject, 
                                   void * BufferToFree);

/*! @brief Obtains physical address for a given I/O Memory virtual address.
 *  @param pMemoryObject Opaque tracking structure that was passed to EmcpalAllocateIoMemory when this buffer was allocated
 *  @param pVirtualAddress Virtual address; must reside inside a buffer allocated by EmcpalAllocateIoMemory
 *  @return Physical address of IO memory
 */
EMCPAL_API csx_phys_address_t EmcpalGetIoMemoryPhysicalAddress(PEMCPAL_IOMEMORY_OBJECT pMemoryObject,
                                                     void *pVirtualAddress);

//**********************************************************************
//
// Physical Memory translation primitives
//
//
/*! @brief Poutine returns the physical address corresponding to a valid nonpaged virtual address
 *         This api panics for invalid physical address
 *  @param pVirtualAddress
 *  @return Physical address
 */
EMCPAL_API csx_phys_address_t EmcpalFindPhysicalAddress(void *pVirtualAddress);

//**********************************************************************
//
// Physical Memory translation primitives
//
//
/*! @brief Poutine returns the physical address corresponding to a valid nonpaged virtual address
 *         This api does not panic for invalid physical address
 *  @param pVirtualAddress
 *  @return Physical address
 */
EMCPAL_API csx_phys_address_t EmcpalFindPhysicalAddressMaybe(void *pVirtualAddress);

//**********************************************************************
//
// Convenience APIs for getting a small piece of PCI I/O capable memory
//

/*! @brief Allocates a buffer of IO Memory.
 *  @note IMPORTANT NOTE: This API can NOT be called at elevated IRQL, such as Dpc/Dispatch or interrupt level,
 *        due to the requirements introduced by the CSX IO Memory API.
 *
 *      IO Memory is required if you need to obtain the physical address
 *      of the memory. For example, if you are using the memory to communicate 
 *      directly with hardware, you need to use IO memory.
 *
 *      See CSX documentation for a deeper discussion of utility memory vs. I/O memory.
 *      In general, IO memory can be obtained using csx_p_io_mem_... APIs or
 *      by using the csx_p_bufman_... APIs.
 * 
 *  @param pPalClient CSX client pointer
 *  @param NumberOfBytes Number of bytes to allocate
 */
EMCPAL_STATUS
EmcpalIoBufferCreateBufmanPool(csx_module_context_t csxModuleContext, csx_bigsize_t Size);

/*! @brief Allocates a buffer of IO Memory.
 *  @note IMPORTANT NOTE: This API can NOT be called at elevated IRQL, such as Dpc/Dispatch or interrupt level,
 *        due to the requirements introduced by the CSX IO Memory API.
 *
 *      IO Memory is required if you need to obtain the physical address
 *      of the memory. For example, if you are using the memory to communicate 
 *      directly with hardware, you need to use IO memory.
 *
 *      See CSX documentation for a deeper discussion of utility memory vs. I/O memory.
 *      In general, IO memory can be obtained using csx_p_io_mem_... APIs or
 *      by using the csx_p_bufman_... APIs.
 * 
 *  @param pPalClient CSX client pointer
 *  @param NumberOfBytes Number of bytes to allocate
 */
EMCPAL_STATUS
EmcpalIoBufferDestroyBufmanPool(csx_module_context_t csxModuleContext);

EMCPAL_API 
csx_pvoid_t
EmcpalIoBufferAllocateAlways(csx_size_t Size);

EMCPAL_API 
csx_pvoid_t
EmcpalIoBufferAllocateMaybe(csx_size_t Size);

EMCPAL_API 
csx_void_t
EmcpalIoBufferFree(csx_pvoid_t IoBuffer);

EMCPAL_API
csx_pvoid_t
EmcpalContiguousMemoryAllocateAlways(csx_size_t Size, csx_bool_t RestrictToBelow4GB, csx_bool_t BeNonCached);

#define EMCPAL_CONTIGUOUS_MEMORY_BE_FROM_BELOW_4GB CSX_TRUE
#define EMCPAL_CONTIGUOUS_MEMORY_BE_FROM_ANYWHERE  CSX_FALSE
#define EMCPAL_CONTIGUOUS_MEMORY_BE_NONCACHED      CSX_TRUE
#define EMCPAL_CONTIGUOUS_MEMORY_BE_CACHED         CSX_FALSE
EMCPAL_API
csx_pvoid_t
EmcpalContiguousMemoryAllocateMaybe(csx_size_t Size, csx_bool_t RestrictToBelow4GB, csx_bool_t BeNonCached);

EMCPAL_API
csx_void_t
EmcpalContiguousMemoryFree(csx_pvoid_t VirtualAddress);

CSX_STATIC_INLINE
csx_bool_t
EmcpalIsAddressValid(csx_pvoid_t VirtualAddress)
{
#if defined(ALAMOSA_WINDOWS_ENV) && (!defined(UMODE_ENV)) && (!defined(SIMMODE_ENV))
    return MmIsAddressValid(VirtualAddress);
#else
    CSX_UNREFERENCED_PARAMETER(VirtualAddress);
    return CSX_TRUE;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this API is not really viable or useful in cases other than the Windows kernel case */
}

/* @} end emcpal_memory */

#ifdef __cplusplus
}
#endif


#endif /* _EMCPAL_MEMORY_ALLOC_H_ */
