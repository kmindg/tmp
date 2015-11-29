#ifndef __BasicIoRequestPartitionedResourceFreelist_h__
#define __BasicIoRequestPartitionedResourceFreelist_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2014
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicIoRequestPartitionedResourceFreelist.h
//
// Contents:
//      Defines the BasicIoRequestPartitionedResourceFreelist class. 
//
// Revision History:
//--

# include "BasicLib/BasicLockedFreeList.h"
# include "BasicLib/LockedListOfCancelableBasicIoRequest.h"
# include "csx_ext_p_core.h"

// Provides a free list mechanism that is distributed over multiple cores, and allows
// BasicIoRequests to queue for those resources.  Each CPU # has affinity to one of the
// internal free lists, and adds all items to that list.  Allocations will try that list
// first on allocation, but on failure it will allocate from other lists, and wait if
// nothing is available.
//
// The Free function may call things which call the AllocateAndExecute() function recursively, but this recursion is limited (bound) to one level.
//
// NOTE: Handles, but is not optimized for, the cases where there the request waits for a resource to be freed.
// @param Type - the type of the item in the free list.  Must have a method:  VOID Start(BasicIoRequest*);
// @param ListOfType - must provide single element AddHead/RemoveHead methods.
template <class Type, class ListOfType>
class BasicIoRequestPartitionedResourceFreelist : protected BasicPartitionedFreeList<Type, ListOfType> {
public:

    BasicIoRequestPartitionedResourceFreelist() : mActive(false) {}

    // Returns the number of items currently free. Expensive because of cache misses if
    // other cores are using this.  Data is stale before returning. 
    //
    // ULONGLONG GetNumFree() const;  << Inherited.

    // Returns the number of bytes that this class dynamically allocates
    // when it is instantiated.
    static ULONG GetAllocationSizeBytes() {
        return BasicPartitionedFreeList<Type, ListOfType>::GetAllocationSizeBytes();
    }

    // Frees a resource. This may call element->Start() with new Irp(), but the recursion is bounded to a depth of 2.
    // NOTE: if there was a use case for freeing multiple items, then a new iterface that
    // supported that would be more efficient than one at a time.
    // @param element - the resource to free.
    void Free(Type * element) {

        // Free it on local queue.
        BasicPartitionedFreeList<Type, ListOfType>::AddHead(element);

        // Read global queue, cheap if there are never waiters.
        if (!mWaiters.IsEmptyNoLockHeld()) {

            // If there are waiters, handle them
            ProvideResourcesToWaiters();
        }
    }


    // Allocates one item, calling the item's "Start" method with the irp, either
    // immediately, or when one is available.
    // @param irp - the IRP to signal when the allocation completes.
    // @param param1 - an optional pointer that is stored and retrived.
    // @param highPriority - Identifies priority of this request. 
    void AllocateAndStart(BasicIoRequest * irp, PVOID param1=NULL, bool highPriority = false) {
        Type * element = BasicPartitionedFreeList<Type, ListOfType>::RemoveHead();

        if(element) {
            element->Start(irp, param1);
        }
        else {
            // Queue the IRP
            ProvideResourcesToWaiters(irp, param1, highPriority);
        }

    }


    Type * RemoveHead() { return BasicPartitionedFreeList<Type, ListOfType>::RemoveHead(); }


private:

    // If both resources and requests are on the lists, then allocate the resource.
    // @param irp - an request to add to the freelist, if not NULL.
    // @param param1 - an optional pointer that is stored and retrived.
    // @param highPriority - Identifies priority of this request. If it is high priority
    //                  and if we will not get free request then we will enqueue it to the
    //                  head of the waiter list otherwise at tail.
    VOID ProvideResourcesToWaiters(BasicIoRequest * irp = NULL, PVOID param1=NULL, bool highPriority = false)
    {

        // Single lock w/ cache line and lock contention if there are waiters....
        mWaiters.AcquireSpinLock();

        // If it is high priority request then add it to the head of the list other wise to the tail.
        if (irp) {
            if(highPriority) {
                if (mWaiters.AddHeadLockHeld(irp, param1)) { irp = NULL; }
            }
            else {
                if (mWaiters.AddTailLockHeld(irp, param1)) { irp = NULL; }
            }
        }

        // mActive is used to convert recursion to iteration.  If someone else is draining
        // the waiters queue, allow them to keep doing so.

        while (!mActive && !mWaiters.IsEmptyLockHeld()) {
            Type * element = BasicPartitionedFreeList<Type, ListOfType>::RemoveHead();
            if (element) {
                PVOID optionalParam;

                BasicIoRequest * irp = mWaiters.RemoveHeadLockHeld(&optionalParam);
                DBG_ASSERT(irp);
                mActive = true;
                mWaiters.ReleaseSpinLock();
                element->Start(irp, optionalParam);
                mWaiters.AcquireSpinLock();
                mActive = false;
            }
            else {
                // No elements, give up.
                break;
            }
        }

        mWaiters.ReleaseSpinLock();

        // Was the IRP cancelled, and could not be added to the wait list?
        if (irp) { irp->CompleteRequestCompletionStatusAlreadySet(); }
    }

    // requests that are waiting for I/O requests.
    LockedListOfCancelableBasicIoRequest  mWaiters;

    // Recursion breaker.  Protected by mWaiters lock.
    bool                                   mActive;
};




#endif  // __BasicIoRequestPartitionedResourceFreelist_h__
