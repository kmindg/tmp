#ifndef __BasicPartitionedLockAccumulator_h__
#define __BasicPartitionedLockAccumulator_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2013
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedLockAccumulator.h
//
// Contents:
//      Defines the BasicPartitionedLockAccumulator class. 
//
// Revision History:
//      05/07/2013 Initial Revision


# include "csx_ext_p_core.h"

// Allows locks from a subset of partitions to be acquired in arbitrary order, with deadlock avoidance.
// This class is NOT itself thread safe.  It is intended for use as a automatic (stack) variable used
// by a single thread/actor/context.
//
//
// @param LockType - the class of the lock, assumed to inherit from BasicLockedObject
//
// Avoids deadlock by release and reacquire of some locks, to ensure consistent ordering.
// Always releases first lock acquired last, to preserve correct IRQL.
// WARNING: the premise is that the order of release is unimportant, except the first must be 
// released last.
template <class LockType> class BasicPartitionedLockAccumulator  {
    typedef UINT_64 CpuMaskType;
    static const int MaxPartitions = sizeof(CpuMaskType) * BITS_PER_BYTE;
public:
    // Construction with no locks held.
    // @param locks - an array of a subclass of BasicLockedObject
    // @param numElements - the number of elements in the array.
    BasicPartitionedLockAccumulator(LockType * locks, ULONG numElements) 
        : mHeld(0), mLocks(locks), mFirst(0), mNumElements(numElements)
    {
        // Number of locks handled by this class has to be lower or equal to
        // the max limit that this class can handle.
        FF_ASSERT(numElements <= MaxPartitions);
    }

    // Construction with exactly one lock already held.  Remembers that lock is held.
    // @param locks - an array of a subclass of BasicLockedObject
    // @param numElements - the number of elements in the array.
    // @param partition - the partition number (array index) of the lock already held.
    BasicPartitionedLockAccumulator(LockType * locks, ULONG numElements, UINT_32 partition) 
        : mHeld((CpuMaskType)1 << partition), mLocks(locks), mFirst(partition), mNumElements(numElements)
    {
        // Mumber of locks handled by this calss has to be lower or equal to
        // the max limit that this class can handle.    
        DBG_ASSERT(numElements <= MaxPartitions);

        // Partition index has to be less than numElements.
        DBG_ASSERT(partition < numElements );

#if DBG || UMODE_ENV || SIMMODE_ENV
        // Verifies that lock at given partition index is already held.
        mLocks[mFirst].AssertLockHeld();
#endif
    }

    // On destruction, release all the locks.
    ~BasicPartitionedLockAccumulator() { ReleaseAllLocks(); }

    // Acquires the lock for the partition specified. To avoid deadlock, it
    //  releases all the already acquired locks above given partition and re-acquires.
    //  It releases all the acquired locks if index of very first acquired lock is greater
    //  than given partition index.
    // 
    // @param partition - Desired index at which lock needs to be acquired.
    //
    // Returns:
    //    true - locks acquired with no locks released and reacquired OR lock
    //                  is already acquired.
    //    false - deadlock avoidance required some locks to be released
    //                  and re-acquired.
    bool GetOneMoreLock(UINT_32 partition) {

        DBG_ASSERT(partition < mNumElements);
        
        // Will this be the first lock acquired?
        if (mHeld == 0) { 
            mFirst = partition; 
            mHeld = ((CpuMaskType)1 << partition);
            mLocks[partition].AcquireSpinLock();
            return true;
        }

        // Desired lock already held?
        if (mHeld & ((CpuMaskType)1 << partition)) {
            return true;
        }


        // Optimistic acquire, possibly out of order.
        if (mLocks[partition].TryAcquireSpinLock()) {
            mHeld |= ((CpuMaskType)1 << partition);
            return true;
        }

        // Which locks are we going to need back?
        CpuMaskType locksNeeded = mHeld;

        // Release higher numbered locks to avoid deadlock.   If that includes
        // the first lock we acquired, release them all.  That is because 
        // we must release the first lock acquired last, so we cannot release it if 
        // we hold other locks.
        // NOTE: we want mFirst to be the first lock we acquired.  If we release them
        // all, then we don't care about the prior mFirst value.
        ReleaseHigherLocksInclusive((mFirst > partition) ? 0 : partition);

        // Ignore the ones we still hold.
        // Assume mHeld == 0x1111, and partition == 7.
        // We will release 8 and 12 mHeld=> 0x0011.
        // locksNeeded will become 0x1100, i.e., exactly the locks we released.
        locksNeeded &= ~mHeld;


        // If we didn't release any locks, then we can simply get the next one.
        if (locksNeeded == 0) {
            mLocks[partition].AcquireSpinLock();
            mHeld |= ((CpuMaskType)1 << partition);
            return true;
        }

        // Add to the list the one we need.
        locksNeeded |= ((CpuMaskType)1 << partition);

        // Get back all the locks we previously held plus the one we want, in order.
        for(UINT i = 0; locksNeeded; i++) {
            DBG_ASSERT(i < mNumElements);
            if (locksNeeded & ((CpuMaskType)1 << i)) {
                mLocks[i].AcquireSpinLock();

                // If we went to no locks, we have to reset which lock
                // we acquired first.
                if (mHeld == 0) { mFirst = i; }
                
                mHeld |= ((CpuMaskType)1 << i);
                locksNeeded ^= ((CpuMaskType)1 << i);
            }
        }
        return false;
    }



    // Give up all locks acquired.
    void ReleaseAllLocks()
    {
        ReleaseHigherLocksInclusive();
    }

private:

    // Releases all locks held for partitions >= parameter.
    // Default is all locks.
    // First lock acquired is released last.
    // @param releaseFrom - the lowest partition number to release (if held).
    void ReleaseHigherLocksInclusive(UINT_32 releaseFrom = 0)
    {
        // The caller needs to request the release of all locks if releasing the first.
        DBG_ASSERT(mHeld == 0 || releaseFrom == 0 || mFirst < releaseFrom);

        // Release all locks but the first lock acquired, since the first has possibly stored 
        // a different IRQL.
        UINT_32 i = releaseFrom;
        CpuMaskType  held = mHeld & ~((CpuMaskType)1 << mFirst);
        do {
            if ((held & ((CpuMaskType)1 << i))) {
                mLocks[i].ReleaseSpinLock();
                mHeld ^= ((CpuMaskType)1 << i);
                held ^= ((CpuMaskType)1 << i);
            }
            i++;

            // Generate mask of all bits set below position i,
            // then NOT that, so we have a mask of all bits in position i
            // and above.
            // Example:   i = 15
            //            ((CpuMaskType)1 <<15)  == 0x0000000000008000
            //            -1:         0x0000000000007fff
            //            ~:          0xffffffffffff8000
        } while (( held & ~(((CpuMaskType)1 << i) - 1)) != 0);


        // Now release the first lock acquired (if any)
        if(mHeld && (mFirst >= releaseFrom)) { 
            mLocks[mFirst].ReleaseSpinLock(); 
            mHeld ^= ((CpuMaskType)1 << mFirst);
            DBG_ASSERT(mHeld == 0);
            mFirst = 0;
        }
        
        DBG_ASSERT(mFirst == 0 || (mHeld && mFirst < releaseFrom));
    }

    // Bitmap of which locks are held.
    CpuMaskType mHeld;

    // The array of locks.
    LockType *    mLocks;

    // Number of locks handle by this class.
    UINT_32 mNumElements;

    // The number of the first lock acquired.  Don't care if mHeld == 0.
    UINT_32 mFirst;
};


#endif  // __BasicPartitionedLockAccumulator_h__
