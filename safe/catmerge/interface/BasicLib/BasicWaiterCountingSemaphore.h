#ifndef __BasicWaiterCountingSemaphore_h__
#define __BasicWaiterCountingSemaphore_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedCountingSemaphore.h
//
// Contents:
//      Defines the BasicWaiterCountingSemaphore class. 
//
// Revision History:
//--


# include "csx_ext_p_core.h"


// Provides a counting semaphore  that a BasicWaiter may wait on until the count is sufficient
// to grant its request.  Requests that wait are serviced in  FIFO order.
//
// If a BasicWaiter is signaled, that indicates that the count was granted
// to that BasicWaiter, and the BasicWaiter can immediately act in that context, including eventually
// calling ReleaseCountAndPossiblySignal() recursively.  The recursion is broken in this class by
// pushing recursion attempts to the outermost call to ReleaseCountAndPossiblySignal(), transforming possible
// recurion to iteration.
template <class Type>
class BasicWaiterCountingSemaphore : protected BasicLockedObject 
{
public:
    BasicWaiterCountingSemaphore(Type initial = 0) : mCounter(initial) {}

    // Releases the count, signaling any waiting items.  However, if this signaling
    // would be recursive, the signalling will occur in the outermost recursion level,
    // to avoid blowing off the end of the stack.
    // @param numRelease      - the amount to add.
    void ReleaseCountAndPossiblySignal(Type numRelease=1)
    {
        ListOfBasicWaiter wakeup;

        AcquireSpinLock();
        mCounter += numRelease;
     
        for(;;) {
            for(;;) {
                BasicWaiter * waiter = mWaiters.RemoveHead();
                if (!waiter) { break; }

                Type count = waiter->ResourcesNeeded();

                if(!mWakeupActive && count <= mCounter) {

                    // Break recursion.
                    mWakeupActive = true;
                    wakeup.AddTail(waiter);
                    mCounter -= count;
                }
                else {
                    mWaiters.AddHead(waiter);
                    break;
                }
            }

            ReleaseSpinLock();

            if(wakeup.IsEmpty())
                return;

            // Drain the wakeup list.
            wakeup.Signaled();
            
            AcquireSpinLock();
            mWakeupActive = false;   
        }
        ReleaseSpinLock();
    }

    // Requests a certain count, acquiring
    // @param element - the element # to subtract from.
    // @param by - the amount to subtract.
    // Returns: true if the resulting count is zero, false otherwise
    bool RequestCount( BasicWaiter * waiter)
    {
        AcquireSpinLock();
       
        bool result = RequestCountLockHeld(waiter);

        ReleaseSpinLock();
        return result;
    }

    bool RequestCountLockHeld( BasicWaiter * waiter)
    {
        bool    result = true;

        Type CountNeeded = waiter->ResourcesNeeded();

        // Don't allow small counts to stave large ones.
        if (mWaiters.IsEmpty() && mCounter >= CountNeeded) {
            mCounter -= CountNeeded;
        }
        else {
            if (waiter) mWaiters.AddTail(waiter);
            result = false;
        }

        return result;
    }

  
private:
    Type       mCounter;
    bool       mWakeupActive;  
    ListOfBasicWaiter  mWaiters;

};





#endif  // __BasicWaiterCountingSemaphore_h__
