#ifndef __InterlockedFIFOList__
#define __InterlockedFIFOList__

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
//      InterlockedFIFOList.h
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
//          - SinglyLinked if items are typically removed from the head
//          - InterlockedFIFO for inter-thread FIFO queuing, 
//              (n producer, single consumer )
//          - InterlockedLIFO for inter-thread LIFO queuing, 
//              (n producer, single consumer )
//      1) Define an embedded link field in each element, eg.
//          typedef struct FOO FOO, *PFOO;
//          struct FOO { PFOO  flink;  ...}
//         There are no restrictions on link field placement.
//          Items that go on two lists at the same time will
//          need more link fields.
//      2) Define a new list type using IFIFOListDefineListType, e.g.
//          IFIFOListDefineListType(FOO, ListOfFOO);
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
//  *** NOTE: There is a unit test program: TestInterlockedFIFOList
//  ***********************************************************************
//
// Revision History:
//  20-Dec-2001   Harvey    Created.
//  03-Jul-2006   GSchaffer 64-bit pointers requires 128-bit interlocked
//--

# include "InlineInterlocked.h"
# include "K10Assert.h"

# define P4CacheLineSize 64

// 
// IFIFOListDefineListType Create new type of list.
//
// Arguments:
//    elementType - The type of elements on the list
//    listType - the new type name to declare.
//
// Fields:
//    shared - traditional singly linked list head/tail,
//        but used both by the producer and consumer.
//        head == NULL => shared list is empty, consumer is draining
//        tail == NULL => List is empty and drained.
//    pad1 - want to force shared onto its own
//          cache line to avoid cache line contention.
//    consumer - Traditional singly linked list that
//          only the consumer can use.

# define IFIFOListDefineListType( elementType, listType)\
    typedef struct listType listType, * P##listType;    \
    union elementType##IFIFOHeader {                    \
        struct CSX_ALIGN_N(16) {                    \
        elementType * volatile head;                    \
        elementType * volatile tail;                    \
      };                                                \
      PVOIDPVOID headAndTail;                           \
    };                                                  \
    struct listType {                                   \
        CSX_ALIGN_N(128) union elementType##IFIFOHeader shared;   \
        CSX_ALIGN_N(128) union elementType##IFIFOHeader consumer; \
                                                        \
        CSX_ALIGN_N(128) ULONG pad;               \
        IFIFOListDefineListTypeConstructor(listType)    \
    };

// C++ lists are constructed empty. 
# ifdef __cplusplus
#   define IFIFOListDefineListTypeConstructor(listType) \
        listType() {                                    \
            consumer.head = NULL;                       \
            consumer.tail = NULL;                       \
            shared.head = NULL;                         \
            shared.tail = NULL;                         \
            pad = 0;                         \
        }
# else                                                
# define IFIFOListDefineListTypeConstructor(listType)    
#endif

// Initialize a list
//
//  list - the list to initialize to empty.
# define IFIFOListInitialize( list)                     \
{                                                       \
    EMCKE_LIST_ALIGNED(&((list).shared.headAndTail));   \
    EMCKE_LIST_ALIGNED(&((list).consumer.headAndTail)); \
    (list).consumer.head = NULL;                        \
    (list).consumer.tail = NULL;                        \
    (list).shared.head = NULL;                          \
    (list).shared.tail = NULL;                          \
}

// Contents of "from" are moved to "to".  "from" end up empty.
# define IFIFOListMove( from, to)	                    \
{                                                       \
    EMCKE_LIST_ALIGNED(&((from).shared.headAndTail));   \
    EMCKE_LIST_ALIGNED(&((from).consumer.headAndTail)); \
    EMCKE_LIST_ALIGNED(&((to).shared.headAndTail));   \
    EMCKE_LIST_ALIGNED(&((to).consumer.headAndTail)); \
                                                        \
    (to).consumer.head = (from).consumer.head;          \
    (to).consumer.tail = (from).consumer.tail;          \
    (to).shared.head = (from).shared.head;              \
    (to).shared.tail = (from).shared.tail;              \
														\
    (from).consumer.head = NULL;                        \
    (from).consumer.tail = NULL;                        \
    (from).shared.head = NULL;							\
    (from).shared.tail = NULL;							\
}

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
//   Traditional add to tail implementation, but with CMPXCHG to make it atomic, and
//   a dependence on the Remove code following protocol to make appending to an existing
//   list work.
//  
# define IFIFOListAddTail(elementType, list, element, link, wasDrained)                       \
{                                                                                             \
  union elementType##IFIFOHeader sharedCopy;                                                  \
  union elementType##IFIFOHeader newShared;                                                   \
                                                                                              \
  EMCKE_LIST_ALIGNED(&((list).shared.headAndTail));                                           \
  EMCKE_LIST_ALIGNED(&((list).consumer.headAndTail));                                         \
  EMCKE_PTR_ALIGNED(&((element)->link));                                                      \
  (element)->link = NULL;                                                                     \
                                                                                              \
  for(;;){     /* Until the compare and exchange succeeds */                                  \
    sharedCopy.headAndTail = (list).shared.headAndTail;                                                               \
                                                                                              \
    if (sharedCopy.head) {   /* List is not empty, append */                                  \
      newShared.head = sharedCopy.head;                                                       \
      newShared.tail = element;                                                               \
      /* Update the tail pointer */                                                           \
      if (InterlockedCompareExchangePVOIDPVOID(                                               \
        &(list).shared.headAndTail, &newShared.headAndTail, &sharedCopy.headAndTail)) {       \
        PVOID oldLink;                                                                        \
                                                                                              \
        /* at this point, the tail change is visible to other producers and the consumer   */ \
        /* but the old tail pointer is not yet pointing to the new tail.                   */ \
        /* Other producers don't care about the old tail.  The consumer, if it sees a NULL */ \
        /* next pointer on an element that is not the tail will set (via cmpxhcg) the next */ \
        /* pointer in that element to "1", and leave that element at the head of its queue */ \
        /* returning NULL.  So in order to get the correct value for "wasDrained" we must  */ \
        /* atomically set this field.                                                      */ \
        oldLink = InlineInterlockedExchangePVOID((PVOID *)&sharedCopy.tail->link, element);   \
        DBG_ASSERT((ULONG_PTR)oldLink <= 1);                                                  \
        wasDrained = (oldLink == (PVOID)1);                                                   \
        break;                                                                                \
      }                                                                                       \
    }                                                                                         \
    else {    /* List was empty, create single element */                                     \
      newShared.head = element;                                                               \
      newShared.tail = element;                                                               \
      if (InterlockedCompareExchangePVOIDPVOID(                                               \
        &(list).shared.headAndTail, &newShared.headAndTail, &sharedCopy.headAndTail)) {       \
                                                                                              \
        /* The consumer empties the list, setting the head pointer to NULL, then after it */  \
        /* has completely processed the list, it sets the tail pointer to NULL.           */  \
        wasDrained = (sharedCopy.tail == NULL);                                               \
        break;                                                                                \
      }                                                                                       \
    }                                                                                         \
  }                                                                                           \
}

// Removes the earliest element from a list.  The caller must ensure that
//    there is only one simultaneous call to this IFIFOListRemove per list.
//
//    The suppressWakeup output of IFIFOListAddHead assumes that the consumer, once
//    awoken, will continue to call this function until it gets a NULL element.
//
// Arguments:
//   elementType - the type of element being removed
//   list        -         the list to remove from
//   element     - the element removed, NULL if empty
//   link        - the field name of the link field embedded in the element to be used.
//
// Implementation:
//   If the consumer's private list is empty, we move the shared list to it, leaving the shared
//   list marked as draining.  If we return a NULL, the state of the shared queue goes to 
//   drained.
//
//   Special code if we find a NULL next element pointer on a element that is not the tail.
//   This indicates that the producer is in the middle of appending an element, and we cannot
//   take the prior element off.  The producer will be notified that the queue is logically
//   drained at this point, and the consumer needs to be signaled again.                                                                
//  
# define IFIFOListRemoveHead(elementType, list, element, link)                                \
{                                                                                             \
  EMCKE_LIST_ALIGNED(&((list).shared.headAndTail));                                           \
  EMCKE_LIST_ALIGNED(&((list).consumer.headAndTail));                                         \
  EMCKE_PTR_ALIGNED(&(((elementType*)0)->link));                                              \
  for(;;) {                                                                                   \
    element = (list).consumer.head;                                                           \
                                                                                              \
    if (element) { /* Our local list has something on it */                                   \
      elementType * next;                                                                     \
                                                                                              \
      if ( element == (list).consumer.tail) {                                                 \
        DBG_ASSERT(element->link == NULL);                                                    \
        (list).consumer.head = NULL;                                                          \
        break;                                                                                \
      }                                                                                       \
      next = element->link;                                                                   \
      if ( (ULONG_PTR)next > 1) {                                                             \
        (list).consumer.head = next;                                                          \
                                                                                              \
        /* The consumer will reference this soon, so get it into cache */                     \
        InlineCachePrefetchPVOID(next);                                                       \
        break;                                                                                \
      }                                                                                       \
      else {                                                                                  \
        /* This is the unlikely race condition where  */                                      \
        /* an element is in the process of being added after 'element'.*/                     \
        /* Mark that we've drained the queue and return empty.*/                              \
        if ((ULONG_PTR)next != 1) { /* If not already marked */                               \
          next = (elementType *)InterlockedCompareExchangePVOID(                              \
                                                    (PVOID *)&element->link, (PVOID)1, NULL); \
          if (next != NULL) {                                                                 \
            continue; /* Field just changed, retry */                                         \
          }                                                                                   \
        }                                                                                     \
        /* We tell the client that the queue is empty, when in fact there is one committed */ \
        /* element and others in the process of being added. */                               \
        /* Note that this can cause the consumer to see an empty queue after a wakeup, but */ \
        /* it will never cause a double wakeup. */                                            \
        element = NULL;                                                                       \
        break;                                                                                \
      }                                                                                       \
    }                                                                                         \
    else {  /* move all elements on the shared list to the local list */                      \
      union elementType##IFIFOHeader sharedCopy, newShared;                                   \
      EMCKE_LIST_ALIGNED(&(newShared.headAndTail));                                           \
                                                                                              \
      sharedCopy.headAndTail = (list).shared.headAndTail;                                     \
                                                                                              \
      if (sharedCopy.head) {  /* shared is not empty => make empty/not drained */             \
        /* We know we want this element immediately, so started the fetch before we stall */  \
        /* the CPU on the serialized instruction "cmpxchg8b". */                              \
        InlineCachePrefetchPVOID(sharedCopy.head);                                            \
                                                                                              \
        newShared.tail = sharedCopy.tail;                                                     \
        newShared.head = NULL;   /* mark as empty, but not drained */                         \
        if (InterlockedCompareExchangePVOIDPVOID(                                             \
          &(list).shared.headAndTail, &newShared.headAndTail, &sharedCopy.headAndTail)) {     \
          (list).consumer.headAndTail = sharedCopy.headAndTail;                               \
        }                                                                                     \
      }                                                                                       \
      else { /* Shared is empty */                                                            \
        if (sharedCopy.tail) { /* If not marked as drained, do so now */                      \
          newShared.tail = NULL;                                                              \
          newShared.head = NULL;                                                              \
          if(! InterlockedCompareExchangePVOIDPVOID(                                          \
            &(list).shared.headAndTail, &newShared.headAndTail, &sharedCopy.headAndTail)) {   \
            continue;  /* list changed, start over */                                         \
          }                                                                                   \
        }                                                                                     \
        break; /* Returns NULL */                                                             \
      }                                                                                       \
    }                                                                                         \
  } /* for */                                                                                 \
}


// Define Enqueue and Dequeue functions for a specific list type.

# define IFIFOListDefineInlineListTypeFunctions(elementType, listType, link) \
static __forceinline BOOLEAN listType##Enqueue(listType * list, elementType * element)    \
{                                                                    \
    BOOLEAN result;                                                  \
                                                                     \
    IFIFOListAddTail(elementType, *list, element, link, result);     \
                                                                     \
    return result;                                                   \
}                                                                    \
                                                                     \
static __forceinline elementType * listType##Dequeue(listType * list)       \
{                                                                    \
    elementType * result;                                            \
                                                                     \
    IFIFOListRemoveHead(elementType, *list, result, link);           \
    return result;                                                   \
}                                                                    \
                                                                     \
static __forceinline BOOLEAN listType##IsEmpty(listType * list)             \
{                                                                    \
    return IFIFOListIsEmpty(*list);                                  \
}                                                                    \
static __forceinline void listType##Initialize(listType * list)             \
{                                                                    \
    IFIFOListInitialize(*list);                                      \
}

// Does the list have anything on it?  
//
// This is only accurate if issued by the consumer or when the consumer is not running.
# define IFIFOListIsEmpty(list) ((list).consumer.head == NULL && (list).shared.head == NULL)

//
// Returns an item that was just removed to the head of the queue.
//  the earliest element from a list.  The caller must ensure that
//    there is only one simultaneous call to this and to IFIFOListRemove per list.
//
// Arguments:
//   list        - the list removed from
//   element     - the element to return
//   link        - the field name of the link field embedded in the element to be used.
//

//  
# define IFIFOListRequeueRemovedElement(list, element, link)                \
{                                                                           \
    if ( (list).consumer.head == NULL) { (list).consumer.tail = (element); }\
    (element)->link = (list).consumer.head;                                 \
    (list).consumer.head = (element);                                       \
} 


# endif
