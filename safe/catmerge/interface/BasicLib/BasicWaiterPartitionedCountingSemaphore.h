#ifndef __BasicWaiterPartitionedCountingSemaphore_h__
#define __BasicWaiterPartitionedCountingSemaphore_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2014
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicWaiterPartitionedCountingSemaphore.h
//
// Contents:
//      Defines the BasicWaiterPartitionedCountingSemaphore class. 
//
// Revision History:
//--


# include "csx_ext_p_core.h"
# include "BasicLib/BasicTimer.h"
# include "BasicLib/BasicWaiterCountingSemaphore.h"

// Provides a counting semaphore  that a BasicWaiter may wait on until the count is sufficient
// to grant its request.   Requests that wait are serviced in global FIFO order.
// To prevent starvation, resources that are released are accumulated until they are sufficient to begin
// servicing waiters.
//
// The counts are distributed across cores to reduce lock contention.  Each CPU# has affinity to a local
// pool of resources, and adds items to that list.  Allocations will try that list first, but on
// failure will allocate from other lists.
//
// If a BasicWaiter is signaled, that indicates that the count was granted
// to that BasicWaiter, and the BasicWaiter can immediately act in that context, including eventually
// calling ReleaseCountAndPossiblySignal() recursively.  The recursion is broken in this class by
// pushing recursion attempts to the outermost call to ReleaseCountAndPossiblySignal(), transforming possible
// recurion to iteration.
// 
// @param Type - the type of counter (ULONG, ULONGLONG)
template <class Type>
class BasicWaiterPartitionedCountingSemaphore {

public:

    BasicWaiterPartitionedCountingSemaphore() : mReserveCount(0), mActive(false), mNumberOfRequestsPended(0)
    {
      csx_p_sys_info_t  sys_info;
      csx_p_get_sys_info(&sys_info);
      mNumCpus = sys_info.os_cpu_count;
      p = new Partition[mNumCpus];
      FF_ASSERT(p);
    }
    
    ~BasicWaiterPartitionedCountingSemaphore() {
        FF_ASSERT(mWaiters.IsEmpty());
        delete p;
    }

    // Returns the number of bytes that this class dynamically allocates
    // when it is instantiated.
    static ULONG GetAllocationSizeBytes() {
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        csx_nuint_t cpuCnt = sys_info.os_cpu_count;
        return (sizeof(Partition) * cpuCnt);
    }

    // Add to the counter.
    // @param count      - the amount to release.
    void ReleaseCountAndPossiblySignal(Type count=1)
    {
        ULONG ptn = Hash();

        p[ptn].AcquireSpinLock();

        if (mWaiters.IsEmpty() && !p[ptn].GetWaitingFlagLockHeld()) {
            p[ptn].ReleaseCountLockHeld(count);
            p[ptn].ReleaseSpinLock();
        }
        else {
            p[ptn].ReleaseSpinLock();
            ProvideResourcesToWaiters(count);
        }
    }


    // Subtract waiter->ResourcesNeeded() from the counter, blocking the waiter if none are available.
    // When the full count of needed resources become available, the waiter will be signaled.
    // @param waiter - the BasicWaiter requesting resources.
    // Returns: true if the resources were granted, false otherwise
    bool RequestCount(BasicWaiter * waiter)
    {
        ULONG ptn = Hash();

        Type countNeeded = waiter->ResourcesNeeded();

        // Try the current core first.
        Type countAcquired = p[ptn].RequestCountUnlocked(countNeeded);

        // If the current core is out of resources, try all cores.  If we're facing a local shorage
        // of resources but not a global shortage, we'll get them here.
        if (countAcquired < countNeeded) {
            for (ULONG i = 0; i < mNumCpus; i++) {
                ULONG part = (ptn + (i+1)) % mNumCpus;
                countAcquired += p[part].RequestCountUnlocked(countNeeded - countAcquired);
                if (countAcquired == countNeeded) {
                    break;
                }
            }
        }

        // Still no luck, so get ready to queue the waiter on the mWaiters list.
        if (countAcquired < countNeeded) {
            AcquireAllPartitionLocks();

            // Try one more time on all cores.  Resources could have been returned before
            // we got the global lock.
            for (ULONG i = 0; i < mNumCpus; i++) {
                countAcquired += p[i].RequestCountLockHeld(countNeeded - countAcquired);
                if (countAcquired == countNeeded) {
                    break;
                }
            }

            // We still didn't get what we need, so queue waiter.  Save the resources we
            // did manage to acquire in the anti-starvation reserve.
            if (countAcquired < countNeeded) {
                mReserveCount += countAcquired;
                AddWaiterAllPartitionLocksHeld(waiter);
            }

            ReleaseAllPartitionLocks();
        }

        return (countAcquired == countNeeded);
    }

    // Get the current total count across all partitions.
    // Locks are not held!
    // Returns: Current total count.
    Type GetCurrentCount() const {
        Type result = 0;

        for (ULONG i = 0; i < mNumCpus; i++) {
            result += p[i].GetCurrentCount();
        }

        return result;
    }

    // Get the number of requests that have had to wait for
    // the semaphore.
    // Returns: Total number of requests pended.
    ULONGLONG NumberOfRequestsPended() const {
        return mNumberOfRequestsPended;
    }

private:

    // Returns a partition number within the range allowed, based on the CPU that
    // execution occurs on.
    ULONG Hash() const { 
        return csx_p_get_processor_id() % mNumCpus;
    }

    // Provide the given number of resources to waiters.  Signal waiters as long as the resources
    // last.  Any leftover resources are put on the current core.
    // @param count - The number of resources to provide to waiters.
    void ProvideResourcesToWaiters(Type count)
    {
        AcquireAllPartitionLocks();

        mReserveCount += count;

        // Take the reserve and use it to drain as many waiters as possible.
        while (!mActive && !mWaiters.IsEmpty()) {
            BasicWaiter *waiter = mWaiters.RemoveHead();
            if (waiter->ResourcesNeeded() <= mReserveCount) {
                mReserveCount -= waiter->ResourcesNeeded();
                mActive = true;
                ReleaseAllPartitionLocks();
                waiter->Signaled(EMCPAL_STATUS_SUCCESS);
                AcquireAllPartitionLocks();
                mActive = false;
            }
            else {
                // Insufficient reserve.  Put the waiter back and stop.
                mWaiters.AddHead(waiter);
                break;
            }
        }

        // If there are no more waiters, then clean up.
        if (mWaiters.IsEmpty()) {
            // Clear local wait flags.
            SetWaitingFlagAllPartitionLocksHeld(false);

            // If there is still a reserve count, return it to the local partition.
            if (mReserveCount) {
                ULONG ptn = Hash();
                p[ptn].ReleaseCountLockHeld(mReserveCount);
                mReserveCount = 0;
            }
        }

        ReleaseAllPartitionLocks();
    }

    // Acquire the lock on every partition from lowest to highest.
    // This functions as the global lock.
    void AcquireAllPartitionLocks() {
        for (ULONG i = 0; i < mNumCpus; i++) {
            p[i].AcquireSpinLock();
        }
    }

    // Release the lock on every partition from lowest to highest.
    void ReleaseAllPartitionLocks() {
        for (ULONG i = 0; i < mNumCpus; i++) {
            // Release in reverse order
            p[mNumCpus-(i+1)].ReleaseSpinLock();
        }
    }

    // Add the given waiter to the tail of the queue of waiters.
    // Sets the partition flags indicating that a waiter is waiting.
    // @param waiter - The waiter to add to the queue.
    void AddWaiterAllPartitionLocksHeld(BasicWaiter *waiter) {
        SetWaitingFlagAllPartitionLocksHeld(true);
        mWaiters.AddTail(waiter);
        mNumberOfRequestsPended++;
    }

    // Set the flag indicating that someone is waiting on all partitions.
    // @param v - The new boolean value for the flag.
    void SetWaitingFlagAllPartitionLocksHeld(bool v) {
        for (ULONG i = 0; i < mNumCpus; i++) {
            p[i].SetWaitingFlagLockHeld(v);
        }
    }


    // There is a counter per partition per element. We lay out the data to reduce
    // cache-line contention for the typical cases.
    class CSX_ALIGN_N(CSX_AD_L1_CACHE_LINE_SIZE_BYTES) Partition : public BasicLockedObject
    {
    public:

        Partition() : mCount(0), mWaiting(false) {}

        // Request the given count.
        // Acquires and releases the partition lock, which must not be held when calling.
        // @param count - The number of resources being requested.
        // Returns: The requested number of resources, if mCount is that large.
        //          Otherwise, returns all of mCount, which then becomes zero.
        Type RequestCountUnlocked(Type count)
        {
            AcquireSpinLock();
            Type result = RequestCountLockHeld(count);
            ReleaseSpinLock();
            return  result;
        }

        // Release the given count to the local count.
        // The partition lock must be held.
        // @param count - The number of resources being released.
        void ReleaseCountLockHeld(Type count)
        {
            mCount += count;
        }

        // Request the given count.
        // The partition lock must be held.
        // @param count - The number of resources being requested.
        // Returns: The requested number of resources, if mCount is that large.
        //          Otherwise, returns all of mCount, which then becomes zero.
        Type RequestCountLockHeld(Type count)
        {
            Type result = count <= mCount ? count : mCount;
            mCount -= result;
            return result;
        }

        // Set the wait flag indicating that some waiter is waiting for resources.
        // The partition lock must be held.
        // @param v - The new value for the waiting flag.
        void SetWaitingFlagLockHeld(bool v) { mWaiting = v; }

        // Get the value of the wait flag, indicating whether some waiter is waiting for resources.
        // Returns: The value of the flag.
        bool GetWaitingFlagLockHeld() const { return mWaiting; }

        // Returns the current count.  The lock is not taken.
        Type GetCurrentCount() const { return mCount; }

    private:

        // Semaphore count for this partition.
        Type mCount;

        // Indicates whether a waiter has queued itself after failing to get resources
        // on this partition.
        bool mWaiting;

    } ;

    // Number of partitions allocated for p;
    int mNumCpus;

    // An allocated array of mNumCpus;
    Partition* p;

    // Count that has been acquired but is being reserved for waiters.  Ensures that enough resources to
    // keep the first waiter from starving will accumulate.
    // Protected by the global lock (all partition locks).
    Type mReserveCount;

    // Global list of waiters.  Locking is provided by the SpinLock
    // inherited from BasicLockedObject.
    // Protected by the global lock (all partition locks).
    BasicListOfBasicWaiter mWaiters;

    // Flag to detect recursion.
    // Protected by the global lock (all partition locks).
    bool mActive;

    // The number of requests that fail to get resources and have to wait.
    ULONGLONG mNumberOfRequestsPended;

};


#endif  // __BasicWaiterPartitionedCountingSemaphore_h__
