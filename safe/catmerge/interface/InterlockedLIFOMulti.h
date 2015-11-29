#ifndef __InterlockedLIFOMulti__
#define __InterlockedLIFOMulti__

//***************************************************************************
// Copyright (C) EMC Corporation 2000-2005
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      InterlockedLIFOMulti.h
//
// Contents:
//      Macros to creat and manipulate LIFO queues (free-list)
//      are SMP safe with multiple producers and consumers.
//      See InterlockedLIFOList.h for single consumer.
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
//          - InterlockedLIFO for inter-thread LIFO queuing,
//              (n producer, single consumer )
//          - InterlockedLIFOMulti for n producer, n consumer.
//      1) Define an embedded link field in each element, eg.
//          typedef struct FOO FOO, *PFOO;
//          struct FOO { PFOO  flink;  ...}
//         There are no restrictions on link field placement.
//          Items that go on two lists at the same time will
//          need more link fields.
//      2) Define a new list type using ILIFOMultiDefineListType, e.g.
//          ILIFOMultiDefineListType(FOO, ListOfFOO);
//
//      3) Declare an instance of the List.
//
//      4) Initialize the list:
//          C++ : Constructor does this for you.
//          C   : Use DLListInitialize()
//
//      5) Specify link field when adding/removing elements from the list.
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
//  *** NOTE: There is a unit test program: TestInterlockedLIFOMulti
//  ***********************************************************************
//
// Revision History:
//  30-Apr-2004   GSchaffer. From InterlockedLifoList.h (20-Dec-2001  DHarvey)
//--

# include "InlineInterlocked.h"

# define _CacheLineSize 16

// In order to allow multiple consumers (& producers) must add sequence number.
// But quadword read is not atomic.  Could use memory barrier instruction after
// reading both head & seq, and before reading next 'link' to protect against
// read reordering.  Alternatively by storing the same USHORT sequence number
// in each DWORD allows detecting where write happened between DWORD reads.

// ILIFOFMP_SPLITLIST
//  union to allow referencing 64-bit value of split sequence and head pointer
//    pair - value of split seq and head pointer.
//    seq0 and seq1 in separate DWORDS - must match.
//    splitHead is the sharedHead pointer, split across DWORDs.
// Note: Can't use #pragma pack in #define below.

#pragma pack(1)
typedef union CSX_ALIGN_DOUBLE_NATIVE ILIFOMP_SPLITLIST_U {
    PVOIDPVOID pair;
    struct {
        ULONG32 seq0;
        PVOID  splitHead;
        ULONG32 seq1;
    } s;
} ILIFOMP_SPLITLIST;
#pragma pack()

//
// Create new type of list.
//
// Arguments:
//    elementType - The type of elements on the list
//    listType - the new type name to declare.
//
// Fields:
//
# ifdef __cplusplus
# define ILIFOMultiDefineListType( elementType, listType)   \
    typedef struct listType listType, * P##listType;        \
    struct listType {                                       \
        ILIFOMP_SPLITLIST splitList;                    \
    };
# else
# define ILIFOMultiDefineListType( elementType, listType)   \
    typedef struct listType listType, * P##listType;    \
    struct listType {                                   \
        ILIFOMP_SPLITLIST splitList;                    \
    };
# endif

// Declare or define Enqueue and Dequeue functions for a specific list type.
//
# define ILIFOMultiDeclareListTypeFunctions(elementType, listType) \
    extern BOOLEAN listType##Enqueue(listType *, elementType *);\
    extern elementType * listType##Dequeue(listType *);         \
    extern void listType##Initialize(listType * list);


# define ILIFOMultiDefineListTypeFunctions(elementType, listType, link) \
BOOLEAN listType##Enqueue(listType * list, elementType * element)    \
{                                                                    \
    BOOLEAN result = FALSE;                                          \
                                                                     \
    ILIFOMultiAddHead(elementType, *list, element, link, result);    \
                                                                     \
    return result;                                                   \
}                                                                    \
                                                                     \
elementType * listType##Dequeue(listType * list)                     \
{                                                                    \
    elementType * result = NULL;                                     \
                                                                     \
    ILIFOMultiRemoveHead(elementType, *list, result, link);          \
    return result;                                                   \
}                                                                    \
BOOLEAN listType##IsEmpty(listType * list)                           \
{                                                                    \
    return ILIFOMultiIsEmpty(*list);                                 \
}                                                                    \
void listType##Initialize(listType * list)                           \
{                                                                    \
    ILIFOMultiInitialize(*list);                                     \
}

# define ILIFOMultiDefineInlineListTypeFunctions(elementType, listType, link) \
__forceinline BOOLEAN listType##Enqueue(listType * list, elementType * element)    \
{                                                                    \
    BOOLEAN result = FALSE;                                          \
                                                                     \
    ILIFOMultiAddHead(elementType, *list, element, link, result);    \
                                                                     \
    return result;                                                   \
}                                                                    \
                                                                     \
__forceinline elementType * listType##Dequeue(listType * list)       \
{                                                                    \
    elementType * result = NULL;                                     \
                                                                     \
    ILIFOMultiRemoveHead(elementType, *list, result, link);          \
    return result;                                                   \
}                                                                    \
__forceinline BOOLEAN listType##IsEmpty(listType * list)             \
{                                                                    \
    return ILIFOMultiIsEmpty(*list);                                 \
}                                                                    \
__forceinline void listType##Initialize(listType * list)             \
{                                                                    \
    ILIFOMultiInitialize(*list);                                     \
}




// Initialize a list
//
//  list - the list to initialize to empty.
# define ILIFOMultiInitialize(list)  { (list).splitList.s.seq0  = 0, (list).splitList.s.splitHead = NULL; (list).splitList.s.seq1 = 0; }

// Does the list have anything on it?
//  Possible to return FALSE while list transitioning to empty.
//  Thus Dequeue() could return NULL even if previous call to IsEmpty() was FALSE.
//
# define ILIFOMultiIsEmpty(list)    ((list).splitList.s.splitHead == NULL)


//
// Add an element to a list.  Can be multiple simultaneous
//    calls on different CPUs.
//
// Arguments:
//   elementType - the type of element being added
//   list -         the list to add to
//   element     - the element to add
//   link        - the field name of the link field embedded in the element to be used.
//   wasDrained -      (BOOLEAN) returned as FALSE if waking up the consumer
//                  is unnecessary, because the consumer has not seen an empty queue
//                  since the last wakeup.  TRUE indicates that the
//                  consumer has drained the queue and seen it empty.
//
// Implementation:
//   Standard "insert at head" implementation, but we use Compare and Swap
//   to change the list head.  The CAS will fail only if the list head
//   has changed in the several instructions since it was read.
//
//   Note that splitHead will point to the last element inserted.
//
# define ILIFOMultiAddHead(elementType, list, element, link, wasDrained)    \
{                                                                           \
    elementType *  h = NULL;                                                \
    ILIFOMP_SPLITLIST copy = {0};                                           \
    ILIFOMP_SPLITLIST update = {0};                                         \
    do {                                                                    \
        copy.s.seq0 = (list).splitList.s.seq0;                              \
        copy.s.splitHead = (list).splitList.s.splitHead;                    \
        copy.s.seq1 = (list).splitList.s.seq1;                              \
        if (copy.s.seq0 != copy.s.seq1) {                                   \
           asm_pause();                                                     \
           continue;                                                        \
        }                                                                   \
        h = (elementType *) copy.s.splitHead;                               \
        (element)->link = h;                                                \
        update.s.seq0 = update.s.seq1 = copy.s.seq0;                        \
        update.s.splitHead = (element);                                     \
    } while (!InterlockedCompareExchangePVOIDPVOID(                         \
              &(list).splitList.pair, &update.pair, &copy.pair));           \
    wasDrained = (h == NULL);                                               \
}

//
// Removes the some element from a list.  Can be multiple simultaneous
//    calls on different CPUs.
//
// Arguments:
//   elementType - the type of element being removed
//   list        -         the list to remove from
//   element     - the element removed, NULL if empty
//   link        - the field name of the link field embedded in the element to be used.
//
// Implementation:
//   This LIFO implementation is optimizing cache locality assuming is usage
//   is as a free list.
//
//
//
# define ILIFOMultiRemoveHead(elementType, list, element, link)             \
{                                                                           \
	ILIFOMP_SPLITLIST copy = {0};                                           \
	ILIFOMP_SPLITLIST update = {0};                                         \
    do {                                                                    \
        copy.s.seq0 = (list).splitList.s.seq0;                              \
        copy.s.splitHead = (list).splitList.s.splitHead;                    \
        copy.s.seq1 = (list).splitList.s.seq1;                              \
        if (copy.s.seq0 != copy.s.seq1) {                                   \
           asm_pause();                                                     \
           continue;                                                        \
        }                                                                   \
        (element) = (elementType *) copy.s.splitHead;                       \
        if (element == NULL) break;                                         \
        update.s.seq0 = update.s.seq1 = copy.s.seq0+1;                      \
        update.s.splitHead = (element)->link;                               \
    } while (!InterlockedCompareExchangePVOIDPVOID(                         \
              &(list).splitList.pair, &update.pair, &copy.pair));           \
}



# endif
