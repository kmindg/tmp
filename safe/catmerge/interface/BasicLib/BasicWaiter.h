//***************************************************************************
// Copyright (C) EMC Corporation 2002, 2011
//
// Licensed material -- property of EMC Corporation
//**************************************************************************/

#ifndef __BasicWaiter_h__
#define __BasicWaiter_h__

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicWaiter.h
//
// Contents:
//      Defines the BasicWaiter class. 
//
// Revision History:
//--

#include "BasicLib/BasicWaiterQueuingClass.h"
#include "SinglyLinkedList.h"


// An object "is a" BasicWaiter if it can wait to be signaled on a ListOfBasicWaiter. A
// typical protocol a BasicWaiter is passed to an object which has a ListOfBasicWaiter.
// The list is protected by a locking protocol in the object.  The object may enque the
// BasicWaiter on the tail of the ListOfBasicWaiter because of some object state which
// prevents the request from being serviced immediately.
// 
// At some later point, the object state changes, while the lock is held, and the object
// either:
// - removes the first item on the list, e.g., "BasicWaiter * x = mWaitList.RemoveHead();"
// - Move the list to a local list, e.g., "ListOfBasicWaiter localList = mWaitList;" Then
//   releases the lock. Then either:
// - "x->Signaled(b);"
// - "localList.Signaled(b)" to inform the waiters that the wait was satified.
// - Thundering herd problem/solution: When we are maintaining list of waiters which are
//   waiting on some limited resouces than when that resouce becomes available we just
//   signal all the waiters. In this case it may be possible that only few waiters can
//   procced with available resources and rest might end up again in waiting list. To
//   handle this problem we should first check if the waiter can proceed with the
//   available resources or not by calling SignalWouldBeProductive(). If this function
//   returns true then only we should signal the waiter. -
class BasicWaiter {
public:

    // Must be implemented by the sub-class.  Informs the subclass that the event being
    // waited for was satified.
    // @param status - indicates success or failure of the operation
    // WARNING: The assumption is that this function is called without locks held.
    virtual void Signaled(EMCPAL_STATUS status) = 0;

    // Returns the number of resources needed by the waiter.  The meaning of this, if any,
    // depends on the type of resource being waited for.
    virtual UINT_64 ResourcesNeeded() const { return 1; }

    static const UINT_32 NoCPUPreference = (UINT_32)-1;

    // Returns the CPU # of the desired CPU, or NoCPUPerference indicating that any CPU will do.
    virtual UINT_32   PreferredCPU() const { return  NoCPUPreference; }

    // Returns the status that the subclass has, defaults to success.
    virtual EMCPAL_STATUS   GetStatus () const { return  EMCPAL_STATUS_SUCCESS; }

    virtual BasicWaiterQueuingClass GetQueuingClass() const { return QC_DefaultGroup; }

private:
    // For list of BasicWaiters
    friend struct BasicListOfBasicWaiterNoConstructor;

    BasicWaiter *   mWaitListLink;

   
};

// Extends BasicWaiter with a public member that can hold an NTStatus.  Enables queueing
// Signaled() calls.
class BasicWaiterWithStatus : public BasicWaiter {
public:
    BasicWaiterWithStatus(EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS) : mStatus(status) { }
    virtual EMCPAL_STATUS   GetStatus ()  const CSX_CPP_OVERRIDE { return  mStatus; }

    EMCPAL_STATUS    mStatus;
};

// Extends BasicWaiterWithStatus with a public member that can hold a PVOID.  Enables queueing
// Signaled() calls.
class BasicWaiterWithContext : public BasicWaiterWithStatus {
public:
    BasicWaiterWithContext() : mContext(NULL) {}

    void*   mContext;
};

SLListDefineListType(BasicWaiter, BasicListOfBasicWaiter);
SLListDefineInlineCppListTypeFunctions(BasicWaiter, BasicListOfBasicWaiter, mWaitListLink);

// See BasicWaiter.
class ListOfBasicWaiter : public BasicListOfBasicWaiter {
public:
    // Dequeues the entire list, calling BasicWait::Signaled for each item on the list.
    // @param success - true means the operation was sucessful,  false the operation was
    //                  unsuccessful.
    // WARNING: The assumption is that this function is called without locks held.  The
    // typical protocol is to copy from a list protected by a lock to a list on the stack,
    // release the lock, then call "Signaled" on the list on the stack.
    void Signaled(EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS);

    // Moves all the items on the from list to this list, preserving items already on this
    // list. The caller is responsible locking both lists during this call.
    VOID MoveList(ListOfBasicWaiter &  from) {
        for(;;) {
            BasicWaiter * waiter = from.RemoveHead();
            if (!waiter) break;
            AddTail(waiter);
        }
    }
    // Moves up to maxToMove items on the from list to this list, preserving items already
    // on this list. The caller is responsible locking both lists during this call.
    // @param from - The source list.
    // @param maxToMove - Move no more than this many items from the list.
    // Returns - The actual number of items removed which can be less than maxToMove
    //           if the list contains less than maxToMove items.
    UINT64 MoveList(ListOfBasicWaiter &  from, UINT64 maxToMove) {
        UINT64 numberMoved;

        for(numberMoved = 0; numberMoved < maxToMove; numberMoved++) {
            BasicWaiter * waiter = from.RemoveHead();
            if (!waiter) break;
            AddTail(waiter);
        }
        return numberMoved;
    }
};

inline void ListOfBasicWaiter::Signaled(EMCPAL_STATUS status) {
    for(;;) {
        BasicWaiter * waiter = RemoveHead();
        if (!waiter) break;
        waiter->Signaled(status);
    }
}

#endif  // __BasicWaiter_h__

