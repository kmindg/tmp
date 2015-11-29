#ifndef __ListsOfIRPs__
#define __ListsOfIRPs__

//***************************************************************************
// Copyright (C) EMC Corporation 2000-2002
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      ListsOfIRPs.h
//
// Contents:
//   ListOfIRPs
//      Macros to creat and manipulate FIFO queues of IRPs that
//      External locking is required.
//          VOID ListOfIRPsInitialize(PListOfIRPs list) - constructor
//          PEMCPAL_IRP ListOfIRPsDequeue(PListOfIRPs list) - remove from head
//          VOID ListOfIRPsEnqueue(PListOfIRPs list, PEMCPAL_IRP Irp) - add to tail
//          VOID ListOfIRPsRequeue(PListOfIRPs list, PEMCPAL_IRP Irp) - add to head
//			BOOLEAN ListOfIRPsRemove(PListOfIRPs list, PEMCPAL_IRP Irp) - remove from list, 
//                              returns TRUE if found/removed
//
//   InterlockedListOfIRPs
//      Macros to creat and manipulate FIFO queues of IRPs that
//      are SMP safe with multiple producers and
//      a single consumer.  External locking is required
//      for multiple consumers.
//      There are no IRQL restrictions on any of the calls.
//      This queueing mechanism is optimized for the
//      case where the producers are executing on one CPU,
//      and the consumer on another, but there are no
//      restrictions on CPUS.
//          VOID InterlockedListOfIRPsInitialize(PInterlockedListOfIRPs list)
//              - Constructor
//          PEMCPAL_IRP InterlockedListOfIRPsDequeue(PInterlockedListOfIRPs list)
//              - remove from head (caller must single thread dequeue/requeue/extractCancelledIRPs)
//          BOOLEAN InterlockedListOfIRPsEnqueue(PInterlockedListOfIRPs list, PEMCPAL_IRP Irp)
//              - add to tail (by any thread)
//          VOID InterlockedListOfIRPsRequeue(PInterlockedListOfIRPs list, PEMCPAL_IRP Irp)
//              - add to head (caller must single thread dequeue/requeue/extractCancelledIRPs)
//          VOID InterlockedListOfIRPsExtractCancelledIRPs(PInterlockedListOfIRPs list,
//                                                             PListOfIRPs cancelledIRPs)
//              - remove all cancelled IRPS from 'list'  (caller must 
//                  single thread dequeue/requeue/extractCancelledIRPs)
//
//
// These lists are type-safe.  The link field is embedded in the element.
//
//
// Usage:
//      1) Initialize the list:
//          C++ : Constructor does this for you.
//          C   : Use DLListInitialize()
//
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
//
// Revision History:
//   4-Jan-2001   Harvey    Created.
//--
// CX_PREP_TODO FIXME

# include "InterlockedFIFOList.h"
# include "SinglyLinkedList.h"
# include "EmcPAL_Irp.h"

// Create a list of EMCPAL_IRP_LIST_ENTRY also.
IFIFOListDefineListType( EMCPAL_IRP_LIST_ENTRY, InterlockedListOfLIST_ENTRY);
SLListDefineListType( EMCPAL_IRP_LIST_ENTRY, ListOfLIST_ENTRY);
	
IFIFOListDefineInlineListTypeFunctions(EMCPAL_IRP_LIST_ENTRY, InterlockedListOfLIST_ENTRY, EMCPAL_IRP_LIST_ENTRY_LINK)

typedef struct InterlockedListOfIRPs InterlockedListOfIRPs, * PInterlockedListOfIRPs;

struct InterlockedListOfIRPs {
    InterlockedListOfLIST_ENTRY list;
};

struct ListOfIRPs {
    ListOfLIST_ENTRY list;
};

typedef struct ListOfIRPs ListOfIRPs, * PListOfIRPs;

// Constructor for the list
static __forceinline VOID ListOfIRPsInitialize(PListOfIRPs list)
{
    SLListInitialize(list->list);
}

// Remove item at head.  NULL return means list empty.
static __forceinline PEMCPAL_IRP ListOfIRPsDequeue(PListOfIRPs list)
{
    PEMCPAL_IRP_LIST_ENTRY			WorkItem;

    SLListRemoveHead(list->list, WorkItem, EMCPAL_IRP_LIST_ENTRY_LINK);
   
    if (!WorkItem) return NULL;
    
    return CSX_CONTAINING_STRUCTURE(WorkItem, EMCPAL_IRP, EMCPAL_IRP_LIST_ENTRY_FIELD);
}

// Add to the tail of the list.
static __forceinline VOID ListOfIRPsEnqueue(PListOfIRPs list, PEMCPAL_IRP Irp)
{
    SLListAddTail(list->list, &Irp->EMCPAL_IRP_LIST_ENTRY_FIELD, EMCPAL_IRP_LIST_ENTRY_LINK);
} 
 
static __forceinline BOOLEAN ListOfIRPsIsEmpty(const ListOfIRPs * list)
{
    return SLListIsEmpty(list->list);
}  

// Add to the head of the list.
static __forceinline VOID ListOfIRPsRequeue(PListOfIRPs list, PEMCPAL_IRP Irp)
{
    SLListAddHead(list->list, &Irp->EMCPAL_IRP_LIST_ENTRY_FIELD, EMCPAL_IRP_LIST_ENTRY_LINK);
}

// Remove a specific IRP from the list and return TRUE.  Return FALSE
// if the Irp was not on the list.
static __forceinline BOOLEAN ListOfIRPsRemove(PListOfIRPs list, PEMCPAL_IRP Irp)
{
	PEMCPAL_IRP_LIST_ENTRY	element;

	SLListExtractMatch(EMCPAL_IRP_LIST_ENTRY, list->list, element, EMCPAL_IRP_LIST_ENTRY_LINK, &Irp->EMCPAL_IRP_LIST_ENTRY_FIELD == element);

	return element != NULL;
}
			
// Call IoCompleteRequest() for all IRPs on the list...
static __forceinline VOID ListOfIRPsForAllCallIoCompleteRequest(PListOfIRPs list)
{
    for(;;) {
        PEMCPAL_IRP irp = ListOfIRPsDequeue(list);  
        if (!irp) { break; }

        EmcpalIrpCompleteRequest(irp);
    }
}  

static __forceinline VOID ListOfIRPsMove(PListOfIRPs from, PListOfIRPs to)
{
    SLListMove(from->list, to->list);
}


// Constructor for the list
static __forceinline VOID InterlockedListOfIRPsInitialize(PInterlockedListOfIRPs list)
{
    InterlockedListOfLIST_ENTRYInitialize(&list->list);
}

// Check if list is empty
static __forceinline BOOLEAN InterlockedListOfIRPsIsEmpty(PInterlockedListOfIRPs list)
{
    return InterlockedListOfLIST_ENTRYIsEmpty(&list->list);
}

// Only one consumer may dequeue
static __forceinline PEMCPAL_IRP InterlockedListOfIRPsDequeue(PInterlockedListOfIRPs list)
{
    PEMCPAL_IRP_LIST_ENTRY			WorkItem;
    WorkItem = InterlockedListOfLIST_ENTRYDequeue(	&list->list);
    
    if (!WorkItem) return NULL;
    
    return CSX_CONTAINING_STRUCTURE(WorkItem, EMCPAL_IRP, EMCPAL_IRP_LIST_ENTRY_FIELD);
}

// Many producers supported
static __forceinline BOOLEAN InterlockedListOfIRPsEnqueue(PInterlockedListOfIRPs list, PEMCPAL_IRP Irp)
{
    return InterlockedListOfLIST_ENTRYEnqueue(&list->list, &Irp->EMCPAL_IRP_LIST_ENTRY_FIELD);
}

// Add back an item just dequeued.  Must be done by single consumer.
static __forceinline VOID InterlockedListOfIRPsRequeue(PInterlockedListOfIRPs list, PEMCPAL_IRP Irp)
{
    IFIFOListRequeueRemovedElement(list->list, &Irp->EMCPAL_IRP_LIST_ENTRY_FIELD, EMCPAL_IRP_LIST_ENTRY_LINK);
}

// Extract all cancelled IRPs.  Must be done by single consumer.
// 
//   IRPs on "list" that are marked cancelled are appended to 'cancelledIRPs'
static __forceinline VOID InterlockedListOfIRPsExtractCancelledIRPs(PInterlockedListOfIRPs list,
                                                       PListOfIRPs cancelledIRPs)
{
    PEMCPAL_IRP irp;
    ListOfIRPs outstanding;
    
    ListOfIRPsInitialize(&outstanding);

    // Empty 'list' completely.
    for(;;) {
        irp = InterlockedListOfIRPsDequeue(list);  
        if (!irp) break;

        if (EmcpalIrpIsCancelInProgress(irp)) {
            ListOfIRPsEnqueue(cancelledIRPs, irp);
        }
        else {
            // Add the item at the head of the queue,
            // so that 'outstanding' is in reverse order.
            ListOfIRPsRequeue(&outstanding, irp);
        }

    }

    // Move the ones not cancelled back to 'list'.  
    // "Requeueing" adds at the head, and preserves order.
    for(;;) {
        irp = ListOfIRPsDequeue(&outstanding);  
        if (!irp) break;
        
        // The 'requeue' reverses the order a second time
        // to get back to the original order.
        InterlockedListOfIRPsRequeue(list, irp);
    }

}


// Extract all cancelled IRPs.  Must be done by single consumer or under protection of lock.
// 
//   IRPs on "list" that are marked cancelled are appended to 'cancelledIRPs'
static __inline VOID ListOfIRPsExtractCancelledIRPs(PListOfIRPs list,
                                             PListOfIRPs cancelledIRPs)
{
    PEMCPAL_IRP irp;
    ListOfIRPs outstanding;
    
    ListOfIRPsInitialize(&outstanding);

    // Empty 'list' completely.
    for(;;) {
        irp = ListOfIRPsDequeue(list);  
        if (!irp) break;

        if (EmcpalIrpIsCancelInProgress(irp)) {
            ListOfIRPsEnqueue(cancelledIRPs, irp);
        }
        else {
            // Add the item at the head of the queue,
            // so that 'outstanding' is in reverse order.
            ListOfIRPsRequeue(&outstanding, irp);
        }

    }

    // Move the ones not cancelled back to 'list'.  
    // "Requeueing" adds at the head, and preserves order.
    for(;;) {
        irp = ListOfIRPsDequeue(&outstanding);  
        if (!irp) break;
        
        // The 'requeue' reverses the order a second time
        // to get back to the original order.
        ListOfIRPsRequeue(list, irp);
    }
    return;
}
// END ListOfIRPsExtractCancelledIRPs

// Extract all IRPs.  Must be done by single consumer or under protection of lock.
// 
//   IRPs on "list" that are marked cancelled are appended to 'cancelledIRPs'
static __inline VOID ListOfIRPsExtractIRPs(PListOfIRPs list,
                                    PListOfIRPs cancelledIRPs)
{
    PEMCPAL_IRP irp;
    
    // Empty 'list' completely.
    for(;;) {
        irp = ListOfIRPsDequeue(list);  
        if (!irp) break;

        ListOfIRPsEnqueue(cancelledIRPs, irp);
    }
    return;
}
// END ListOfIRPsExtractIRPs

# endif
