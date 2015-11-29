#ifndef CMM_TYPES_H
#define CMM_TYPES_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001 - 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file contains definitions for the all the CMM types used
 *     internally.
 *     Functions and structures have forward declarations defined here so that
 *     all structure names are valid when the stucture is actually defined (so
 *     we can make references to other stuctures names without worry.)
 *
 *     The only types not defined (or at least declared) here are those which
 *     are only exported and never used internally.  For those definitions see
 *     cmm.h
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *     14-Jul-2006     64-bit.     Chris Hughes
 *     30-Jul-2007     Control PP  Ian McFadries
 *
 *
 ***************************************************************************/

#include "cmm_generics.h"
#include "environment.h"
#include "flare_sgl.h"

/************************* C O N S T A N T S  ***************************/

/********************
 * EXPORTED CONSTANTS
 *    These constants may be accessed directly by CMM users
 *    all others are for CMM internal use only.
 ********************/

/*************************************************
 * NAME:
 *   CMM_STATUS_*
 *
 * DESCRIPTION:
 *     These #defines provide the values for the CMM_ERROR type.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_STATUS_BASE                     0x42420000

#define CMM_STATUS_SUCCESS                          CMM_STATUS_BASE + 0
#define CMM_STATUS_OBJECT_DOES_NOT_EXIST            CMM_STATUS_BASE + 1
#define CMM_STATUS_REQUEST_CANCELED                 CMM_STATUS_BASE + 2
#define CMM_STATUS_INSUFFICIENT_MEMORY              CMM_STATUS_BASE + 3
#define CMM_STATUS_ILLEGAL_OPERATION                CMM_STATUS_BASE + 4
#define CMM_STATUS_QUOTA_EXHAUSTED                  CMM_STATUS_BASE + 5
#define CMM_STATUS_REQUEST_DEFERRED                 CMM_STATUS_BASE + 6
#define CMM_STATUS_RESOURCE_EXHAUSTED               CMM_STATUS_BASE + 7

#define CMM_STATUS_NO_SUCH_FUNCTION                 CMM_STATUS_BASE + 8
#define CMM_STATUS_ASSERT_FAILED                    CMM_STATUS_BASE + 9
#define CMM_STATUS_MEMORY_CORRUPTION_DETECTED       CMM_STATUS_BASE + 10
#define CMM_STATUS_MEMORY_SIZE_MISMATCH_DETECTED    CMM_STATUS_BASE + 11

#define CMM_ADDRESS_NOT_AVAILABLE	                CMM_STATUS_BASE + 12
#define CMM_STATUS_MEMORY_CONFIG_INVALID            CMM_STATUS_BASE + 13

 /*************************************************
 * NAME:
 *   CMM_MAXIMUM_CLIENTS_PER_POOL
 *
 * DESCRIPTION:
 *   CMM_MAXIMUM_CLIENTS_PER_POOL defines the maximum number of clients that
 *   may be attached to any one pool. Note that this only applies to system
 *   pools.  The number of clients allowed for user created pools is defined
 *   at pool creation time.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_MAXIMUM_CLIENTS_PER_POOL            64


/*************************************************
 * NAME:
 *   CMM_MAXIMUM_MEMORY
 *
 * DESCRIPTION:
 *   CMM_MAXIMUM_MEMORY is used as input when registering as a client of the
 *   CMM to indicate that the client wants to only be restricted on memory
 *   allocation by the total amount of memory in the memory pool for which
 *   the client is registering.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_MAXIMUM_MEMORY  -1


/*
 * The following constants are for CMM internal use only.
 */

//Define the tag that will mark the end of memory
//when guard pages are turned on
#define CMM_MEM_END_TAG "CMM_END"

/*******************
 * INIT CONSTANTS
 *******************/

/*******************
 * CLIENT CONSTANTS
 *******************/

/*************************************************
 * NAME:
 *   CMM_MAXIMUM_CLIENTS
 *
 * DESCRIPTION:
 *   CMM_MAXIMUM_CLIENTS specifies the maximum number of clients supported by the
 *   CMM.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_MAXIMUM_CLIENTS     \
    (CMM_NUMBER_OF_SYSTEM_POOLS *       \
                CMM_MAXIMUM_CLIENTS_PER_POOL)


/*****************************
 * MEMORY DESCRIPTOR CONSTANTS
 *****************************/

/*************************************************
 * NAME:
 *   CMM_CONTROL_MEMORY_SGL
 *
 * DESCRIPTION:
 *   CMM_CONTROL_MEMORY_SGL is a pointer to an SGL which describes the range(s) of
 *   memory which make up the CONTROL memory nonpaged pool.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_CONTROL_MEMORY_SGL  cmmPoolControlMemorySGL

/*************************************************
 * NAME:
 *   CMM_CONTROL_MEMORY_NCP_SGL
 *
 * DESCRIPTION:
 *   CMM_CONTROL_MEMORY_NCP_SGL is a pointer to an SGL which describes 
 *   the range(s) of memory which make up the CONTROL memory non-contiguous 
 *   pool.
 *
 *  HISTORY
 *     30-Jul-2007     Created.   Ian McFadries
 *
 *************************************************/
#define CMM_CONTROL_MEMORY_NCP_SGL  cmmPoolControlMemoryNonContiguousPoolSGL


/*************************************************
 * NAME:
 *   CMM_DATA_MEMORY_SGL
 *
 * DESCRIPTION:
 *   CMM_DATA_MEMORY_SGL is a pointer to an SGL which describes the range(s) of
 *   memory which make up the DATA memory pool.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_DATA_MEMORY_SGL cmmPoolDataMemorySGL

/*************************************************
 * NAME:
 *   CMM_SEP_DATA_MEMORY_SGL
 *
 * DESCRIPTION:
 *   CMM_SEP_DATA_MEMORY_SGL is a pointer to an SGL which describes the range(s) of
 *   memory which make up the SEP DATA memory pool.
 *
 *  HISTORY
 *     20-Oct-2011     Created.   -Matt Ferson
 *
 *************************************************/
#define CMM_SEP_DATA_MEMORY_SGL cmmPoolSEPDataMemorySGL

/*************************************************
 * NAME:
 *   CMM_LAYERED_DATA_MEMORY_SGL
 *
 * DESCRIPTION:
 *   CMM_LAYERED_DATA_MEMORY_SGL is a pointer to an SGL which describes the range(s) of
 *   memory which make up the Layered DATA memory pool.
 *
 *  HISTORY
 *     16-Jan-2013     Created.   - Joe Ash
 *
 *************************************************/
#define CMM_LAYERED_DATA_MEMORY_SGL cmmPoolLayeredDataMemorySGL

/*************************************************
 * NAME:
 *   CMM_LOCAL_ONLY_DATA_MEMORY_SGL
 *
 * DESCRIPTION:
 *   CMM_LOCAL_ONLY_DATA_MEMORY_SGL is a pointer to an SGL which describes the range(s) of
 *   memory which make up the DATA memory pool that can only be accessed locally.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_LOCAL_ONLY_DATA_MEMORY_SGL  cmmPoolLocalOnlyDataMemorySGL

/*************************************************
 * NAME:
 *   CMM_MAXIMUM_MEMORY_SUPPORTED
 *
 * DESCRIPTION:
 *   CMM_MAXIMUM_MEMORY_SUPPORTED specifies the maximum physical memory
 *   space supported by  the CMM.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_MAXIMUM_MEMORY_SUPPORTED        \
                        ((UINT_64)128 * GIGABYTE)

/*************************************************
 * NAME:
 *   CMM_MAXIMUM_NONCONTIGUOUSPOOL_SZ
 *
 * DESCRIPTION:
 *   CMM_MAXIMUM_NONCONTIGUOUSPOOL_SZ specifies the maximum 
 *   non-contiguous pool memory size supported by the CMM.
 *
 *  HISTORY
 *     30-Jul-2007     Created.   Ian McFadries
 *
 *************************************************/
#define CMM_MAXIMUM_NONCONTIGUOUSPOOL_SZ        \
                        ((UINT_64)24 * GIGABYTE)

/*************************************************
 * NAME:
 *   MAXIMUM_MDL_BUFFER_SZ
 *
 * DESCRIPTION:
 *   MAXIMUM_MDL_BUFFER_SZ Specifies the length in bytes of the 
 *   buffer the MDL is to describe
 *
 *  HISTORY
 *     30-Jul-2007     Created.   Ian McFadries
 *
 *************************************************/
#define MAXIMUM_MDL_BUFFER_SZ        \
             ROUND_DOWN(((PAGE_SIZE * (65535 - sizeof(EMCPAL_MDL)) / sizeof(ULONG_PTR))-1), MEGABYTE)


/*************************************************
 * NAME:
 *   CMM_MEMORY_BLOCKING_SIZE
 *
 * DESCRIPTION:
 *   CMM_MEMORY_BLOCKING_SIZE specifies the granularity at which
 *   physical memory is divided.  It is selected so that both
 *   512 and 520 byte blocks divide into it with no remainder
 *   and to provide appropriate byte alignment.
 *
 *  Note:
 *   The base CMM blocksize is loosely coupled with the basic EMM
 *   blocksize within Flaredrv as the EMM is by far the largest
 *   consumer of CMM blocks. Hence, granularity based on EMM's
 *   best fit (512*520*4). PTolvanen
 *
 *  HISTORY
 *     26-Apr-2001     Created.            -Matt Yellen
 *     12-Jun-2002     For X1 hardware use a smaller memory blocking size
 *                     so we don't loose as much memory from rounding.  -Matt Yellen
 *
 *************************************************/
#define CMM_MEMORY_BLOCKING_SIZE    (512*520*4)


/*************************************************
 * NAME:
 *   CMM_DATA_MEMORY_BLOCKING_SIZE
 *
 * DESCRIPTION:
 *   CMM_DATA_MEMORY_BLOCKING_SIZE specifies the granularity at which
 *   physical memory is divided.  It is selected so that windows is
 *   able to use large pages to map in the memory, which must be
 *   aligned to 4MB 
 *
 *  HISTORY
 *     16-Feb-2012     Created              -Joe Ash
 *
 *************************************************/
#define CMM_DATA_MEMORY_BLOCKING_SIZE   (4 * MEGABYTE)

/*************************************************
 * NAME:
 *   CMM_USER_POOL_MEMORY_BLOCKING_SIZE
 *
 * DESCRIPTION:
 *   User pools can set thier own allocation unit size (as opposed to system pool
 *   which are initialized to the CMM_MEMORY_BLOCKING_SIZE).  However the allocation
 *   unit size is forced to be a multiple of the CMM_USER_POOL_MEMORY_BLOCKING_SIZE
 *   in order to assure cache line alignment
 *
 *  HISTORY
 *     26-Apr-2001     Created.            -Matt Yellen
 *
 *************************************************/
#define CMM_USER_POOL_MEMORY_BLOCKING_SIZE  64

/*************************************************
 * NAME:
 *   CMM_NUMBER_OF_MEMORY_DESCRIPTORS
 *
 * DESCRIPTION:
 *   CMM_NUMBER_OF_MEMORY_DESCRIPTORS specifies the number of memory
 *   descriptors to be statically allocated in CMM.   Its value is a function of the maximum
 *   memory size supported by the CMM and the CMM memory blocking size.
 *   With the values outlined in this document, the total size statically allocated to memory
 *   descriptors by  the CMM would be approximately 362 KB which represents 0.004% of a
 *   memory space of 8 GB.  This number would go down if the maximum memory space
 *   supported by the CMM were reduced; it would go up if the memory blocking size were
 *   reduced.
 *   Note  that CMM_NUMBER_OF_MEMORY_DESCRIPTORS represents the absolute
 *   maximum number of memory descriptors that could ever be required in a totally memory
 *   fragmented system where individual blocks of the memory blocking size are allocated.  It is
 *   expected that clients of the CMM will allocate relatively large numbers of memory blocks and
 *   so most memory descriptors are not expected to actually be used.
 *   Initial implementation of the CMM will use a base address ordered queuing model for
 *   memory descriptors.  If indeed the system functions such that a limited number of  memory
 *   descriptors are ever  in use, such queuing is probably sufficient.  However, in the extreme
 *   such a queuing model is poor as queue lengths become too great for efficient searching.
 *   Consequently plans should  include the potential for modifications to the method of tracking
 *   memory descriptors.  Such modifications would require  a modest amount more memory and
 *   would be solely contained within the CMM.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_NUMBER_OF_MEMORY_DESCRIPTORS        \
    (CMM_MAXIMUM_MEMORY_SUPPORTED / CMM_MEMORY_BLOCKING_SIZE)


 /*****************************
 * POOL CONSTANTS
 *****************************/


 /*************************************************
 * NAME:
 *   CMM_IN_USE_POOLS
 *
 * DESCRIPTION:
 *   CMM_IN_USE_POOLS is defined as the CMM's global in use pool queue.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_IN_USE_POOLS    cmmPools.inuse

 /*************************************************
 * NAME:
 *   CMM_MAX_POOL_SGL_LENGTH
 *
 * DESCRIPTION:
 *   CMM_MAX_POOL_SGL_LENGTH specifies the maximum length of an SGL which is used
 *   describe a CMM pool.  This includes one entry for NULL terminiation.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_MAX_POOL_SGL_LENGTH     16

/*************************************************
 * NAME:
 *   CMM_NUMBER_OF_SYSTEM_POOL_COMMONS
 *
 * DESCRIPTION:
 *   CMM_NUMBER_OF_SYSTEM_POOL_COMMONS specifies the number of system level
 *   pool common structures provided by the CMM.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
#define CMM_NUMBER_OF_SYSTEM_POOL_COMMONS   \
                CMM_NUMBER_OF_SYSTEM_POOLS

/***********************  F O R W A R D   D E C L A R A T I O N S  ********************************/

/*
 * Forward declarations for all structures and function pointer types are put here.
 * This is so we can be sure that once cmm_types.h is included that the entire CMM
 * name space is defined and can be used even though the actual structure or function definition
 * might be defined after the name is used (which can happen with circular references).
 */


/*
 * Client types
 */
struct cmmClientDescriptor;

/*
 * Memory Descriptor Types
 */
struct cmmMemoryDescriptor;

/*
 * Request Types
 */
struct cmmRequest;


/**********************  T Y P E S  **************************************************************/

/*************************************************
 * NAME:
 *   CMM_MEMORY_ADDRESS
 *
 * DESCRIPTION:
 *   CMM_MEMORY_ADDRESS is a generic address type which CMM uses internally to track
 *   memory in each of its pools.  Depending on the particular pool it may be a physical address, a
 *   virtual address, or in theory something completely different.  Regardless it is the responsibility of
 *   CMM to convert this into something we export (virtual address or SGL).
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *     24-Dec-2001     Convert to 64 bit.    -Matt Yellen
 *
 *************************************************/
typedef UINT_64 CMM_MEMORY_ADDRESS;

/*************************************************
 * NAME:
 *   CMM_MEMORY_SIZE
 *
 * DESCRIPTION:
 *   CMM_MEMORY_SIZE specifies an amount of memory managed by CMM..
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *     24-Dec-2001     Convert to 64 bit.    -Matt Yellen
 *
 *************************************************/
typedef UINT_64 CMM_MEMORY_SIZE;

/*************************************************
 * NAME:
 *   CMM_SGL
 *
 * DESCRIPTION:
 *   A CMM_SGL is the SGL type used internally by CMM.  CMM will handle all
 *   the necessary conversions to the exported FLARE SGL.
 *
 *  HISTORY
 *     24-Dec-2001     Created.   -Matt Yellen
 *     01-Mar-2003     Packed structure to save 4 bytes.
 *************************************************/
#pragma pack(4)
typedef struct _CMM_SGL
{
    CMM_MEMORY_SIZE MemSegLength;
    CMM_MEMORY_ADDRESS MemSegAddress;
#define CMM_MEMORY_ADDRESS_NULL 0
} CMM_SGL, *PCMM_SGL;
#pragma pack()

/*************************************************
 * NAME:
 *   CMM_ERROR
 *
 * DESCRIPTION:
 *   The CMM_ERROR is the CMM error type.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef UINT_32 CMM_ERROR;

/*************************************************
 * NAME: 
 *   CMM_DESCRIPTOR_HASH_ENTRY
 *
 * DESCRIPTION:
 *   The CMM_DESCRIPTOR_HASH_ENTRY is a single entry in a hash stored in each pool.  It is 
 *   used to convert virtual or physical addresses (depending on the pool type) to the associated 
 *   memory ID.
 *   
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef struct cmmMemoryDescriptor *CMM_DESCRIPTOR_HASH_ENTRY;

/*************************************************
 * NAME: 
 *   CMM_CALLBACK_SGL
 *
 * DESCRIPTION:
 *   The CMM_CALLBACK specifies the interface type for a callback function provided by clients of the 
 *   CMM for deferred requests for memory for which an SGL is returned.
 *   
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef	void	(*CMM_CALLBACK_SGL) (
				CMM_ERROR		    cmmError,
				PSGL			    allocatedMemorySGL,
				struct cmmRequest   *request);

/*************************************************
 * NAME: 
 *   CMM_CALLBACK_VA
 *
 * DESCRIPTION:
 *   The CMM_CALLBACK_VA specifies the interface type for a callback function provided by clients of 
 *   the CMM for requests for memory for which a virtual address is returned.
 *   
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef	void	(*CMM_CALLBACK_VA) (
				CMM_ERROR		    cmmError,
				PVOID			    allocatedMemoryVA,
				struct cmmRequest   *request);

/*************************************************
 * NAME:
 *   CMM_MEMORY_POOL_TERM
 *
 * DESCRIPTION:
 *   CMM_MEMORY_POOL_TERM identifies the "term of usage" of the memory by the client.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef enum cmmPoolTerm
{
    CMM_TERM_LONG,
    CMM_TERM_SHORT
} CMM_MEMORY_POOL_TERM;

/*************************************************
 * NAME:
 *   CMM_MEMORY_POOL_TYPE
 *
 * DESCRIPTION:
 *   The CMM_MEMORY_POOL_TYPE identifies the  type of the memory pool.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *     30-Jul-2007     Control PP  Ian McFadries
 *
 *************************************************/
typedef enum cmmMemoryPoolType
{
    CMM_TYPE_CONTROL = 0,
    CMM_TYPE_CONTROL_NCP,
    CMM_TYPE_DATA,
    CMM_TYPE_DATA_LOCAL_ONLY,
    CMM_TYPE_SEP_DATA,
    CMM_TYPE_LAYERED_DATA,
    CMM_NUMBER_OF_SYSTEM_POOLS
} CMM_MEMORY_POOL_TYPE;

/*************************************************
 * NAME:
 *   CMM_TYPE_EQUALS_CONTROL_POOL_TYPE
 *
 * DESCRIPTION:
 *   The CMM_TYPE_EQUALS_CONTROL_POOL_TYPE macro checks if 
 *   an arg is equal to any CMM control pool type
 *
 *  HISTORY
 *     30-Jul-2007     Created.   Ian McFadries
 *
 *************************************************/
#define CMM_TYPE_EQUALS_CONTROL_POOL_TYPE(a)    \
    (((a) == CMM_TYPE_CONTROL)      ||          \
     ((a) == CMM_TYPE_CONTROL_NCP)  ||          \
     ((a) == CMM_TYPE_LAYERED_DATA))

/*************************************************
 * NAME:
 *   CMM_TYPE_NOT_EQUAL_TO_CONTROL_POOL_TYPE
 *
 * DESCRIPTION:
 *   The CMM_TYPE_NOT_EQUAL_TO_CONTROL_POOL_TYPE macro checks that 
 *   an arg is NOT equal to any CMM control pool type
 *
 *  HISTORY
 *     30-Jul-2007     Created.   Ian McFadries
 *
 *************************************************/
#define CMM_TYPE_NOT_EQUAL_TO_CONTROL_POOL_TYPE(a)  \
    (((a) != CMM_TYPE_CONTROL)      &&              \
     ((a) != CMM_TYPE_CONTROL_NCP)  &&              \
     ((a) != CMM_TYPE_LAYERED_DATA))

/*************************************************
 * NAME:
 *   CMM_TYPE_EQUALS_DATA_POOL_TYPE
 *
 * DESCRIPTION:
 *   The CMM_TYPE_EQUALS_DATA_POOL_TYPE macro checks if 
 *   an arg is equal to any CMM Data pool type
 *
 *  HISTORY
 *     01-Feb-2013  Created.    Joe Ash
 *
 *************************************************/
#define CMM_TYPE_EQUALS_DATA_POOL_TYPE(a)   \
    (((a) == CMM_TYPE_DATA)             ||  \
     ((a) == CMM_TYPE_DATA_LOCAL_ONLY)  ||  \
     ((a) == CMM_TYPE_SEP_DATA))

/*************************************************
 * NAME:
 *   CMM_ALLOC_METHOD
 *
 * DESCRIPTION:
 *   The CMM_ALLOC_METHOD is a bit mask that defines various
 *   attribute which control how memory is allocated from this pool.
 *
 *   CMM_METHOD_DEFAULT - Contiguous memory allocations and static descriptor allocations.
 *                        This cannot be used with any other option.
 *
 *   CMM_METHOD_DYNAMIC - memory descriptors are allocated from the memory which is being
 *   being managed.  Control memory only.
 *
 *   CMM_METHOD_DISCONTIGUOUS - this pool supports discontiguous memory allocations.  Memory
 *   returned may be physically discontiguous (but will always be virtually contiguous).
 *   Control memory only.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *     29-Apr-2004     Changed to a bitmask to add discontigous memory support.   -Matt Yellen
 *     06-Sep-2005     Removed "discontiguous" bit (unimplemented, unused).
 *                     Added bit for "guard pages".         -Matt Yellen
 *
 *************************************************/
typedef BITS_32 CMM_ALLOC_METHOD;

#define CMM_METHOD_DEFAULT  0       //Static descriptors and contiguous memory
#define CMM_METHOD_STATIC   0       //Same as above - depricated.
#define CMM_METHOD_DYNAMIC  BIT0        //Dynamic descriptors
#define CMM_METHOD_GUARD_PAGES BIT1     //Memory mapped with guard pages
#define CMM_METHOD_PATTERN_FILL_UNUSED BIT2     //Fill unused memory with a pattern


/********************************* E M B E D D E D   S T R U C T U R E S ******************************/

/*
 * These structures are embedded directly in other stuctures therefore
 * we need to be sure they are fully defined first.  The only way to do
 * that is to define them here.
 */

/*
 * Queues
 */

/*************************************************
 * NAME:
 *   CMM_POOL_QUEUE
 *
 * DESCRIPTION:
 *   CMM_POOL_QUEUE provides the definition for pool queues.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef CMM_DEFINE_QUEUE_LINKS (cmmPoolDescriptor,
                                CMM_POOL_DESCRIPTOR_LINKS);

typedef CMM_DEFINE_QUEUE (cmmPoolDescriptor,
                     CMM_POOL_QUEUE);

/*************************************************
 * NAME:
 *   CMM_POOLS
 *
 * DESCRIPTION:
 *   The CMM_POOLS structure provides the definition for the base structure
 *   in memory for accessing the memory pools.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef struct cmmPools
{
    CMM_POOL_QUEUE inuse;
} CMM_POOLS;

/*************************************************
 * NAME:
 *   CMM_CLIENT_QUEUE
 *
 * DESCRIPTION:
 *   CMM_CLIENT_QUEUE provides the definition for client queues.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef CMM_DEFINE_QUEUE_LINKS (cmmClientDescriptor,
                                CMM_CLIENT_DESCRIPTOR_LINKS);

typedef CMM_DEFINE_QUEUE (cmmClientDescriptor,
                     CMM_CLIENT_QUEUE);

/*************************************************
 * NAME:
 *   CMM_MEMORY_DESCRIPTOR_QUEUE
 *
 * DESCRIPTION:
 *   CMM_MEMORY_DESCRIPTOR_QUEUE provides the definition for memory descriptor
 *   queues.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef CMM_DEFINE_QUEUE_LINKS (cmmMemoryDescriptor,
                                CMM_MEMORY_DESCRIPTOR_LINKS);

typedef CMM_DEFINE_QUEUE (cmmMemoryDescriptor,
                     CMM_MEMORY_DESCRIPTOR_QUEUE);

/*************************************************
 * NAME:
 *   CMM_REQUEST_QUEUE
 *
 * DESCRIPTION:
 *   CMM_REQUEST_QUEUE provides the definition for request queues.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef CMM_DEFINE_QUEUE_LINKS (cmmRequest,
                                CMM_REQUEST_LINKS);

typedef CMM_DEFINE_QUEUE (cmmRequest,
                     CMM_REQUEST_QUEUE);

/*
 * Pools
 */

/*************************************************
 * NAME:
 *   CMM_POOL_COMMON_DESCRIPTOR
 *
 * DESCRIPTION:
 *   The CMM_POOL_COMMON_DESCRIPTOR is the internal CMM object used to describe the
 *   common aspects of a memory pool.  Whether or not the memory represented by the
 *   CMM_POOL_COMMON_DESCRIPTOR can be mapped into virtual space is determined by the
 *   type stored in the CMM_POOL_COMMON_DESCRIPTOR.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef struct cmmPoolCommonDescriptor
{
    CMM_SGL                         poolMemory[CMM_MAX_POOL_SGL_LENGTH];
    CMM_MEMORY_POOL_TYPE        type;
    CMM_ALLOC_METHOD            allocMethod;

    //This is only set for minimum pools and is a pointer to the orignial descriptor used to
    //create this pool.  It is needed when we need to free this memory when the pool is destroyed.
    struct cmmMemoryDescriptor *managedMemoryDesc;

    CMM_DESCRIPTOR_HASH_ENTRY  *memoryDescHash;

    CMM_MEMORY_SIZE             allocationUnitSize;
    UINT_32                     totalAllocationUnits;
    UINT_32                     freeAllocationUnits;

    struct cmmMemoryDescriptor *maxContiguousFreeMemoryPtr;
    CMM_MEMORY_DESCRIPTOR_QUEUE freeMemDescQueue;
    CMM_MEMORY_DESCRIPTOR_QUEUE freeMemoryQueue;
    CMM_MEMORY_DESCRIPTOR_QUEUE inuseMemoryQueue;

    CMM_REQUEST_QUEUE   deferredRequests;
} CMM_POOL_COMMON_DESCRIPTOR;


/*************************************************
 * NAME:
 *   CMM_POOL_DESCRIPTOR (CMM_POOL_ID)
 *
 * DESCRIPTION:
 *   The CMM_POOL_DESCRIPTOR is the internal CMM object used to describe a memory pool to
 *   the CMM.  A CMM_POOL_DESCRIPTOR defines the semantics of the pool.
 *   CMM_POOL_IDs are converted into CMM_POOL_DESCRIPTORs within CMM.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef struct cmmPoolDescriptor
{
    CMM_POOL_DESCRIPTOR_LINKS links;
    BOOLEAN                 isUserPool;

    CMM_MEMORY_POOL_TERM    termOfUsage;

    CMM_CLIENT_QUEUE        freeClientDescQueue;
    CMM_CLIENT_QUEUE        inuseClientQueue;

    CMM_POOL_COMMON_DESCRIPTOR  *commonDescriptorPtr;
} CMM_POOL_DESCRIPTOR;

/*************************  E X P O R T E D   T Y P E S   A N D   S T R U C T U R E S  ***************************/

/*************************************************
 * NAME:
 *   CMM_CLIENT_ID
 *
 * DESCRIPTION:
 *   The CMM_CLIENT_ID is the CMM object which specifies a particular client to the CMM.  The client
 *   is specified by a triple composed of a memory pool identifier, maximum allowable allocated memory
 *   and currently allocated memory.  This allows the CMM to implement a per client memory type
 *   allocation limit mechanism.  Note that it requires those clients of the CMM which use multiple
 *   memory pools to register once per memory pool to be used.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef struct cmmClientDescriptor *     CMM_CLIENT_ID;

/*************************************************
 * NAME:
 *   CMM_RETURNED_MEMORY_CALLBACK
 *
 * DESCRIPTION:
 *   The CMM_RETURNED_MEMORY_CALLBACK specifies the interface type for a callback
 *   function provided by the CMM to clients for return of memory at the CMM’s request.
 *   The CMM_RETURNED_MEMORY_CALLBACK is used by a client to return memory freed
 *   as a consequence of a request from the CMM to the client to free memory.  Memory is freed by
 *   virtual address or SGL.  When memory is freed either freeMemoryVA will be set to a virtual
 *   address or freeMemorySGL will be set to an SGL.  The other pointer is set to NULL.
 *   A client must not free memory as a consequence of a CMM request by calling cmmFreeMemory().
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef void    (*CMM_RETURNED_MEMORY_CALLBACK) (
                    CMM_CLIENT_ID   clientId,
                    PVOID   freeMemoryVA,
                    PSGL    freeMemorySGL);

/*************************************************
 * NAME:
 *   CMM_MEMORY_RETURN_REQUEST_CALLBACK
 *
 * DESCRIPTION:
 *   The CMM_MEMORY_RETURN_REQUEST_CALLBACK specifies the interface type for a callback function
 *   provided by clients of the CMM for freeing memory at the CMM’s request.
 *   The CMM_MEMORY_RETURN_REQUEST_CALLBACK is used by the CMM to notify a client that the client
 *   should relinquish to the CMM what memory it can which was allocated from the backing pool of the
 *   client.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef void    (*CMM_MEMORY_RETURN_REQUEST_CALLBACK) (
            CMM_RETURNED_MEMORY_CALLBACK    callback);

/*************************************************
 * NAME:
 *   CMM_POOL_ID
 *
 * DESCRIPTION:
 *   The CMM_POOL_ID is the  CMM object which provides for access to a memory pool.  The attributes
 *   of the particular memory pool are accessed via macros.
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef CMM_POOL_DESCRIPTOR *       CMM_POOL_ID;

/*************************************************
 * NAME:
 *   CMM_REQUEST
 *
 * DESCRIPTION:
 *   CMM_REQUEST is the object used to describe a request for memory for which the client wishes to provide a
 *   callback function.  The callback function may be called whenever the allocation succeeds, only if the request
 *   was deferred, or when the request was cancelled depending on the particular interface functions the client uses.
 *   The callback specified may be the type that returns a virtual address or SGL (but not both, the other must be
 *   NULL).
 *
 *  HISTORY
 *     26-Apr-2001     Created.   -Matt Yellen
 *
 *************************************************/
typedef struct cmmRequest
{
/**************** CMM Private Fields **********************/
    CMM_REQUEST_LINKS poolLinks;
    CMM_REQUEST_LINKS clientLinks;

    CMM_CALLBACK_VA         callbackVA;
    CMM_CALLBACK_SGL        callbackSGL;

    struct cmmClientDescriptor *clientPtr;
    CMM_POOL_DESCRIPTOR     *poolPtr;

    CMM_REQUEST_QUEUE       *clientQueueHead;       //To check if this request is on a client queue
                                                    //and validate it is the right one.

    CMM_ERROR               result;                 //Store the result once the request has been processed

	CMM_MEMORY_ADDRESS		physAddress;            //Physical address requested for this request.
/*********************************************************/

    CMM_MEMORY_SIZE         numberOfBytes;
    PVOID                   clientPrivate;

} CMM_REQUEST;

//added this #def for debugging DIMS 164633
//CL = Chetan Loke
#if DBG
#define CLDBG 1
#endif

#endif /* CMM_TYPES_H */
