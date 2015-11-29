#ifndef __InterlockedLIFOList__
#define __InterlockedLIFOList__

//***************************************************************************
// Copyright (C) EMC Corporation 2000-2008
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      InterlockedLIFOList.h
//
// Contents:
//      Macros to creat and manipulate FIFO queues that
//      are SMP safe with multiple producers and
//      a single consumer.  External locking is required
//      for multiple consumers.
//      There are no IRQL restrictions on any of the calls.
//      This queueing mechanism is optimized for the
//      case where the producers are executing on one CPU,
//      and the consumer on another, but there are no
//      restrictions on CPUS.
//
//      These lists are type-safe, requiring a new list
//      type for each type of element.  The link field
//      is embedded in the element.
//
//
// Usage:
//      0) Select the correct type of list:
//          - DoublyLinked if items are randomly removed from the list
//          - LIFO if items are typically removed from the head
//          - InterlockedLIFO for inter-thread FIFO queuing, 
//              (n producer, single consumer )
//      1) Define an embedded link field in each element, eg.
//          typedef struct FOO FOO, *PFOO;
//          struct FOO { PFOO  flink;  ...}
//         There are no restrictions on link field placement.
//          Items that go on two lists at the same time will
//          need more link fields.
//      2) Define a new list type using ILIFOListDefineListType, e.g.
//          ILIFOListDefineListType(FOO, ListOfFOO);
//
//      3) Declare an instance of the List.
//
//      4) Initialize the list:
//          C++ : Constructor does this for you.
//          C   : Use DLListInitialize()
//
//      5) Specify link field when adding/removing elements from the list.
//
//      6) External locking is required with more than one reader.
//         Multiple writers and one reader are correctly synchronized.
//  
// Notes:
//      The works fine for X86, but other CPUs might add restrictions.
//
//      Cache Line Movement:
//         The producer dirties the list head and the element added.
//         The consumer will dirty the list head about twice per
//         wakeup, and each of the elements.
//
//         Under light load, the list head will change CPUs twice
//         per element, as will the element, or 4 cache line moves
//         per element.
//
//         Under heavy load, the list head movement will be amortized,
//         and the cache line movement will approach 2 moves per element.
//
//      Serializing instructions:
//         Under light load, 2 per element.
//
//         Under heavy load, approaches one per element.
//
//  ***********************************************************************
//  *** NOTE: There is a unit test program: TestInterlockedLIFOList
//  ***********************************************************************
//
// Revision History:
//  20-Dec-2001   Harvey    Created.
//--

# include "InlineInterlocked.h"

# define _CacheLineSize 16

// 
// Create new type of list.
//
//    @param elementType - The type of elements on the list
//    @param listType - the new type name to declare.
//
// Fields:
//    sharedHead - pointer shared between producer and consumer. Values:
//         0 - shared list is empty and consumer is not draining "consumerHead"
//         1 - shared list is empty, but consumer is draining "consumerHead"
//         otherwise - linked list tail (note there is no head for this list)
//    pad - want to force consumerHead onto its own
//          cache line to avoid cache line contention.
//    consumerHead - Only used by the single consumer, list of items
//         pulled from shared tail, but not yet processed.
# ifdef __cplusplus
# define ILIFOListDefineListType( elementType, listType)    \
    typedef struct listType listType, * P##listType;        \
    struct listType {                                       \
        elementType * volatile sharedHead;                  \
        listType() : sharedHead(NULL) {}                    \
        bool AddHead(elementType * element);                \
        elementType * RemoveHead();                         \
    };
# else
# define ILIFOListDefineListType( elementType, listType)    \
    typedef struct listType listType, * P##listType;        \
    struct listType {                                       \
        CSX_ALIGN_N(128) elementType * volatile sharedHead;                  \
        CSX_ALIGN_N(128) ULONG pad;                   \
    };
# endif

# define ILIFOListDefineInlineCppListTypeFunctions( elementType, listType, link)	\
      static inline bool listType::AddHead(elementType * element)               \
      {                                                                  \
			bool result;                                                  \
                                                                     \
			ILIFOListAddHead(elementType, *this, element, link, result);       \
                                                                     \
			return result;                                                   \
      }                                                                  \
      static inline elementType * listType::RemoveHead()                        \
      {                                                                  \
        elementType * result;                                            \
                                                                         \
		ILIFOListRemoveHead(elementType, *this, result, link);               \
        return result;                                                   \
      }                                                                  \

// Declare or define Enqueue and Dequeue functions for a specific list type.
// 
# define ILIFOListDeclareListTypeFunctions(elementType, listType)    \
    extern BOOLEAN listType##Enqueue(listType *, elementType *);     \
    extern elementType * listType##Dequeue(listType *);              \
    extern void listType##Initialize(listType * list);        

# define ILIFOListDefineListTypeFunctions(elementType, listType, link) \
BOOLEAN listType##Enqueue(listType * list, elementType * element)    \
{                                                                    \
    BOOLEAN result;                                                  \
                                                                     \
    ILIFOListAddHead(elementType, *list, element, link, result);     \
                                                                     \
    return result;                                                   \
}                                                                    \
                                                                     \
elementType * listType##Dequeue(listType * list)                     \
{                                                                    \
    elementType * result;                                            \
                                                                     \
    ILIFOListRemoveHead(elementType, *list, result, link);                 \
    return result;                                                   \
}                                                                    \
BOOLEAN listType##IsEmpty(listType * list)                           \
{                                                                    \
    return ILIFOListIsEmpty(*list);                                  \
}                                                                    \
void listType##Initialize(listType * list)                           \
{                                                                    \
    ILIFOListInitialize(*list);                                      \
}

# define ILIFOListDefineInlineListTypeFunctions(elementType, listType, link) \
static __forceinline BOOLEAN listType##Enqueue(listType * list, elementType * element) \
{                                                                    \
    BOOLEAN result;                                                  \
                                                                     \
    ILIFOListAddHead(elementType, *list, element, link, result);     \
                                                                     \
    return result;                                                   \
}                                                                    \
                                                                     \
static __forceinline elementType * listType##Dequeue(listType * list)       \
{                                                                    \
    elementType * result;                                            \
                                                                     \
    ILIFOListRemoveHead(elementType, *list, result, link);           \
    return result;                                                   \
}                                                                    \
static __forceinline BOOLEAN listType##IsEmpty(listType * list)             \
{                                                                    \
    return ILIFOListIsEmpty(*list);                                  \
}                                                                    \
static __forceinline void listType##Initialize(listType * list)             \
{                                                                    \
    ILIFOListInitialize(*list);                                      \
}


// Initialize a list
//
//  @param list - the list to initialize to empty.
# define ILIFOListInitialize( list)                     \
{                                                       \
    EMCKE_PTR_ALIGNED(&((list).sharedHead));            \
    (list).sharedHead = NULL;                           \
}


// Does the list have anything on it?  
//
// This is only accurate if issued by the consumer or when the consumer is not running.
# define ILIFOListIsEmpty(list) ((list).sharedHead == NULL)


//
// Add an element to a list.  Can be multiple simultaneous
//    calls on different CPUs.
//
//  @param  elementType - the type of element being added
//  @param  list -         the list to add to
//  @param  element     - the element to add
//  @param  link        - the field name of the link field embedded in the element to be used.
//  @param  wasDrained -      (BOOLEAN) returned as FALSE if waking up the consumer
//                  is unnecessary, because the consumer has not seen an empty queue
//                  since the last wakeup.  TRUE indicates that the 
//                  consumer has drained the queue and seen it empty.
//
// Implementation:
//   Standard "insert at head" implementation, but we use Compare and Swap
//   to change the list head.  The CAS will fail only if the list head
//   has changed in the several instructions since it was read.
//
//   Note that sharedHead will point to the last element inserted.
//
//   sharedHead == 0 or 1  indicates an empty list, and the tail
//     of the list may have either value.
//  
# define ILIFOListAddHead(elementType, list, element, link, wasDrained)     \
{                                                                           \
    elementType *  h;                                                       \
    EMCKE_PTR_ALIGNED(&((list).sharedHead));                                \
    EMCKE_PTR_ALIGNED(&((element)->link));                                  \
    do {                                                                    \
        h = (list).sharedHead;                                              \
        (element)->link = h;                                                \
    } while (h != InterlockedCompareExchangePVOID((PVOID *)&(list).sharedHead, element, h)); \
    wasDrained = (h == NULL);                                               \
}

//
// Removes the some element from a list.  The caller must ensure that
//    there is only one simultaneous call to this ILIFOListRemove per list.
//
//  @param  elementType - the type of element being removed
//  @param  list        -         the list to remove from
//  @param  element     - the element removed, NULL if empty
//  @param  link        - the field name of the link field embedded in the element to be used.
//
// Implementation:
//   This LIFO implementation is optimizing cache locality assuming is usage
//   is as a free list.
//   
//
//  
# define ILIFOListRemoveHead(elementType, list, element, link)          \
{                                                                       \
    PVOID next;                                                         \
    EMCKE_PTR_ALIGNED(&((list).sharedHead));                            \
    EMCKE_PTR_ALIGNED(&(((elementType*)0)->link));                      \
    do {                                                                \
        element = (list).sharedHead;                                    \
        if (element == NULL) break;                                     \
        next = element->link;                                           \
    } while (element != InterlockedCompareExchangePVOID(                \
                        (PVOID *)&(list).sharedHead, next, element));   \
}



# endif
