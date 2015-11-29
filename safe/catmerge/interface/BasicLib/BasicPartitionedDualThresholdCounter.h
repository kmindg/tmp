#ifndef __BasicPartitionedDualThresholdCounter_h__
#define __BasicPartitionedDualThresholdCounter_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2013
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedDualThresholdCounter.h
//
// Contents:
//      Defines the BasicPartitionedDualThresholdCounter class. 
//
// Revision History:
//      05/07/2013 Initial Revision

# include "csx_ext_p_core.h"
# include "BasicLib/BasicPartitionedLockAccumulator.h"

// Provides a counter mechanism that is distributed over multiple cores to avoid cache
// contention. This allows very frequent increments and decrements while keeping the
// accesses core local. It allows specification of a high and low Threshold, and allows
// for efficient query as to whether the count is:
//     - at or below the low Threshold
//     - between the low and high threshold
//     - at or above the high threshold
//
// FIX-ENHANCE: Not yet implemented.
// It provides notification on transitions between these three states.  The Thresholds
// are specified at initialization time, when all the counters are zeroed.
// 
// The implemention partitions the counters and Thresholds, so that each partition has
// a counter, and high and low threshold.  The following must always hold.
//      - the partition's counter, high, and low must be => 0
//      - low < high within each paritition.
//      - if in any partition, counter > low, then counter must be >= low in all partitions.
//      - if in any partition, counter > high, then counter must be >= high in all partitions.
// Counter values, and high and low may be moved between partitions to accomplish this.  Movement
// of high and low is always done with the global lock held.
// 
// @param Type - the type of counter.  
template <class Type>
class BasicPartitionedDualThresholdCounter  {
public:

    enum RelativeToThreshold { BelowThreshold, BetweenThreshold, AboveThreshold };

    BasicPartitionedDualThresholdCounter() : mLastPosition(BelowThreshold),
        mInitialized(false), mNumTransitionsToBelowThreshold(0), mNumTransitionsBelowToBetweenThreshold(0),
        mNumTransitionsAboveToBetweenThreshold(0),  mNumTransitionsToAboveThreshold(0)
    {    
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        mNumCpus = sys_info.os_cpu_count;
        mArray = new PerPartitionData[mNumCpus];
        FF_ASSERT(mArray);
    }

    ~BasicPartitionedDualThresholdCounter() {
        if(mArray) {
            delete[] (mArray);
        }
    }

    // Changes the threshold values, which will be strictured on demand.
    // @param low - the value to set the low threshold to.
    // @param high - the valye to set the high threshold to
    void SetThresholds(Type low, Type high);

    // Allocates memory for per core partition data and Initializes the threshold. 
    // It does this only once. If it is already initialized then it will be no-op.
    void Initialize(Type low, Type high);

    // Add to the counter.  Typically this acquires a single per core lock, and is cheap.
    // More is incurred when the counter value is close to one of the thresholds.
    //
    // @param by      - the amount to add.
    // NOTE: This may change the current state.
    VOID Increment(Type by = 1);

    // Subtract to the counter. Typically this acquires a single per core lock. More is
    // incured when the counter value is close to one of the threshold.
    //
    // @param by      - the amount to subtract.
    // NOTE: This may change the current state.
    VOID Decrement(Type by = 1);

    // Returns a close-to-accurate representation of whether we are BelowThreshold, BetweenThreshold, or
    // AboveThreshold the two thresholds.  This call is very cheap.  Since no locks are held,
    // there are no guarantees that the state has not changed since return statement,
    // but this is sufficient for the known use cases.   As the counter moves away from the 
    // thresholds, this becomes stable.
    RelativeToThreshold CurrentState() const { return mLastPosition; }

    // Provides a sum of all of the counters, but this is unlocked.  
    // NOTE: Acquiring the spin lock will *not* make this atomic.
    Type Sum() const {
        Type sumValue = 0;
        for(UINT_32 cpu = 0; cpu < mNumCpus; cpu++) {
            sumValue += mArray[cpu].CurrentCount();
        }
        return sumValue;
    }

    // Get the number of transitinos that have been made. NOT atomic even per partition.
    // Records transitions to threshold regions, as well as directions. Note that since
    // invariants prevent transitions from below the threshold to above and vice versa, 
    // transitions to below must have been from between thresholds and likewise for 
    // transitions to above.
    //
    // @param toBelow        - OUT for transitions to below the threshold
    // @param belowToBetween - OUT for transitions from below to between
    // @param aboveToBetween - OUT for transitions from above to between
    // @param toAbove        - OUT for transitions to above the threshold
    void GetTransitionStats(ULONGLONG& toBelow, ULONGLONG& belowToBetween, ULONGLONG& aboveToBetween, ULONGLONG& toAbove) const {
        toBelow = mNumTransitionsToBelowThreshold;
        belowToBetween = mNumTransitionsBelowToBetweenThreshold;
        aboveToBetween = mNumTransitionsAboveToBetweenThreshold;
        toAbove = mNumTransitionsToAboveThreshold;
    }

    // Clear statistics for the counter
    void ClearTransitionStats() {
        mNumTransitionsToBelowThreshold = 0;
        mNumTransitionsBelowToBetweenThreshold = 0;
        mNumTransitionsAboveToBetweenThreshold = 0;
        mNumTransitionsToAboveThreshold = 0;
    }

    public: // Interfaces for incrementing and fetching per-partition statistics

    // Record a global state transition from oldPos to this partition's state. Is a
    // no-op if the old state and new states are the same.
    // EVALUATE: Global state should never change unless all locks are held.  Therefore,
    // no locking should be needed here.  This needs to be verified in the code.
    // @param newPos - New state.
    void RecordTransition(RelativeToThreshold newPos) {
        RelativeToThreshold oldPos = mLastPosition;

        if (oldPos == newPos) {
            return;
        }
        if (newPos == BelowThreshold) {
            mNumTransitionsToBelowThreshold++;
        }
        else if (newPos == BetweenThreshold) {
            if (oldPos == BelowThreshold) {
                mNumTransitionsBelowToBetweenThreshold++;
            }
            else {
                mNumTransitionsAboveToBetweenThreshold++;
            }
        }
        else {
            mNumTransitionsToAboveThreshold++;
        }
    }

    // Check consistency of the entire data structure, including individual partitions and
    // global state.  Does not take locks, so is reliable only when the test serializes
    // calls sufficiently.
    //
    // This function also serves as a kind of documentation of the invariants that are
    // expected to be satisfied.
    void DebugAssertGlobalConsistency();
    
    class CSX_ALIGN_N(CSX_AD_L1_CACHE_LINE_SIZE_BYTES) PerPartitionData : public BasicLockedObject {
    public:
        PerPartitionData() : mCounter(0), mHigh(0), mLow(0) {}

        // On destruction, counter should be zero.
        ~PerPartitionData() {
            DBG_ASSERT(mCounter == 0);
        }

        // Allows the threshold value to be put into some partition at initalization,
        // so it can be distributed based on demand.
        // @param low - the value to set the low threshold to.
        // @param high - the valye to set the high threshold to
        void SetThresholds(Type low, Type high) { 
            DBG_ASSERT(low <= high);
            mLow = low; mHigh = high; 
        }

        // Allow the counter to increase, possibly exceeding thresholds.
        // @param value - the amount to increment by.
        void ForceAddLockHeld(Type value) {
            DBG_ASSERT(mHigh >= mLow);
            mCounter += value;
        }

        // Allow the counter to decrease by the given value, it may cross the
        // thresholds.
        // @param value - the amount to decrement by.
        void ForceSubtractLockHeld(Type value) {
            DBG_ASSERT(mHigh >= mLow);
            DBG_ASSERT(mCounter >= value);
            mCounter -= value;
        }

        // Where is this position relative to its thresholds?
        // Represented as a step function, it will look like this:
        // 
        // Above:                                o------>
        // Between:                       o------o
        // Below:    +--------------------o
        //           0                  mLow    mHigh
        //
        // When a partition is at mLow or mHigh, its global position is ambiguous and
        // must be determined from other infomation.
        // Note: mLow == mHigh is permitted.
        //
        RelativeToThreshold PositionLockHeld() const {
            if ( mCounter < mLow) {
                return BelowThreshold;
            }
            
            if (mCounter > mHigh) {
                return AboveThreshold;
            }
            
            return BetweenThreshold;
        }

        // Returns true if exactly on the low threshold
        bool IsAtLowThreshold() const { return mCounter == mLow; }

        // Returns true if exactly on the high threshold
        bool IsAtHighThreshold() const { return mCounter == mHigh; }

        // Returns the current value of the count. It is unlocked.
        Type CurrentCount() const { return *(volatile Type*) &mCounter; }

        // How much do we need for each threshold, to avoid crossing a threshold boundary?
        // If already above the threshold, then we need 0.
        // @param by - the desired increment.
        // @param low   - the amount needed in the low threshold.
        // @param high  - the amount needed in the high threshold.
        //
        // Returns:
        //      returns the number which is allowed to increment in this iteration
        Type DetermineNeededThresholdsForAddLockHeld(Type by, Type & low, Type & high) const;

        // How much count do we need to decrement the count without crossing the threshold bondary?
        //
        // @param by - the desired decrement.
        // @param countNeeded - the amount needed to be at the same position.
        //
        // Returns:
        //      returns the number which is allowed to decrement of this iteration.
        Type DetermineNeededCountForSubtractLockHeld(Type by, Type & countNeeded) const;

        // Transfer thresholds to another partition, with locks on both partitions held. It determines
        //  how much thresholds it can transfer to other partition without changing its current
        //  position.
        //
        // @param lowNeeded - the desired amount of low threshold to transfer.  Reduced by the amount actually transferred.
        //                    Set to zero if this partition is already over threshold.
        // @param highNeeded - the desired amount of high threshold to transfer.  Reduced by the amount actually transferred.
        //                    Set to zero if this partition is already over threshold.
        // @param to - the partition transfer it to.
        void TransferThresholdAsNeededLocksHeld(Type & by, Type  & lowNeeded, Type & highNeeded, PerPartitionData * to);

        // Transfer counts to another partition with locks on both the partition held.
        //
        // @param countNeeded - the desired amount of count to transfer. Reduced by the amount actually transferred.
        // @param to - the partition transfer to.
        // Returns: true if the decrement should restart because values changed.
        bool TransferCountAsNeededLocksHeld(Type & countNeeded, PerPartitionData * to);

        // Zeros this parititon by moving all counters/thresholds to the other partition with the locks held
        //  for both the partitions.
        //
        // @param to - the partition transfer it to.
        void TransferAllValuesLocksHeld(PerPartitionData * to);
        
        // Returns true if we could yield desired thresholds, or
        // we are already above a threshold.  Can be called w/o lock held.
        bool HasNeededThresholdOrInfo(Type lowNeeded, Type highNeeded) const {
            if ( (lowNeeded && mCounter < mLow) ||
                    (highNeeded && mCounter < mHigh) ) {
                return true;
            }
    
            return false;
        }

        // Returns true if we could yield desired count otherwise false. It can
        // be called without lock held.
        bool CouldYieldCountOfAtLeastOne() const {
            return (mCounter != mLow && mCounter != mHigh);
        }

        // Debug-only interface for checking consistency.
        void GetValues(Type& count, Type& low, Type& high) const {
            count = mCounter;
            low = mLow;
            high = mHigh;
        }


    private:

        // The current count in this partition.
        Type  mCounter;

        // The current high threshold in this partition.
        Type  mHigh;

        // The current low threshold in this partition.
        // mLow <= mHigh must always be true.
        Type  mLow;

    };

private:

    // Returns a partition number within the range allowed, based on the CPU that
    // execution occurs on.
    ULONG Hash() const { 
        return csx_p_get_processor_id() % mNumCpus;
    }

    // Size of array
    UINT_32    mNumCpus;

    // Per CPU data.
    PerPartitionData  * mArray;

    // This is updated on all transitions, when sufficient locks are held to keep it exact. 
    RelativeToThreshold    mLastPosition;

    // Indicates that object is initialized or not.
    bool mInitialized;

    // Statistics counters
    ULONGLONG mNumTransitionsToBelowThreshold;
    ULONGLONG mNumTransitionsBelowToBetweenThreshold;
    ULONGLONG mNumTransitionsAboveToBetweenThreshold;
    ULONGLONG mNumTransitionsToAboveThreshold;
};

template <class Type>
void BasicPartitionedDualThresholdCounter<Type>::DebugAssertGlobalConsistency() 
{
#if defined (SIMMODE_ENV)
    Type sumTotal = 0;
    Type lowTotal = 0;
    Type highTotal= 0;

    bool havePartitionAbove = false;
    bool havePartitionBetween = false;
    bool havePartitionBelow = false;

    // Get global counts
    for(UINT_32 cpu = 0; cpu < mNumCpus; cpu++) {
        Type sum;
        Type low;
        Type high;
        mArray[cpu].GetValues(sum, low, high);

        sumTotal += sum;
        lowTotal += low;
        highTotal += high;

        // Keep track of which states are represented. We must never have partitions
        // in all three states, and the global state must be consistent with the
        // partition states.
        if(sum > high) { havePartitionAbove = true; }
        else if (sum > low && sum < high) { havePartitionBetween = true; }
        else if (sum < low) { havePartitionBelow = true; }
    }

    // Scan partitions again to verify that they are individually consistent with
    // global counts.
    for(UINT_32 cpu = 0; cpu < mNumCpus; cpu++) {
        Type sum;
        Type low;
        Type high;
        mArray[cpu].GetValues(sum, low, high);

        // If any partition is below, this one must be on the low threshold
        if (havePartitionBelow) { DBG_ASSERT(sum <= low); }
        if (havePartitionAbove) { DBG_ASSERT(sum >= high); }
    }

    // Check global consistency
    if (sumTotal > highTotal){
        DBG_ASSERT(mLastPosition == AboveThreshold || mLastPosition == BetweenThreshold);
        DBG_ASSERT(!havePartitionBelow);
    }
    else if (sumTotal >= lowTotal) {
        DBG_ASSERT(!havePartitionBelow || !havePartitionAbove);
    }
    else {
        // The following shouldn't work, but it seems to in practice.
        DBG_ASSERT(mLastPosition == BelowThreshold /* || mLastPosition == BetweenThreshold */);
        DBG_ASSERT(!havePartitionAbove);
    }
#endif
}

template <class Type>
Type BasicPartitionedDualThresholdCounter<Type>::
PerPartitionData::DetermineNeededThresholdsForAddLockHeld(Type by, Type & low, Type & high) const
{
    DBG_ASSERT(mHigh >= mLow);

     Type result = by; 

     if (mCounter <= mHigh) { 
         if (mCounter == mLow) {
             // If any other partitions are below, bring them to between.
             low = result;
         }
         else if (mCounter < mLow && (mCounter + result > mLow)) { 
             result = mLow - mCounter;
             low = 0;
         } 
         else { 
             // Either given increment will not cross the low threshold or
             // counter is already above the low threshold.
             low = 0; 
         }

         if (mCounter == mHigh) {
             // If any other partitions are below, bring them to between.
             high = result;
         }
         else if (mCounter < mHigh && (mCounter + result > mHigh)) { 
             // Hack to deal with dependencies between low and high.  Needs some
             // restructuring to be expressed cleanly.
             if (mCounter < mLow) {
                 // We've already dealt with this case above.  Just go to mLow.
                 high = 0;
             }
             else if (mCounter == mLow) {
                 // Increase mHigh as far as we plan to increase mCounter
                 high = mCounter + result - mHigh;
             }
             else {
                 // Above low threshold already
                 result = mCounter + result - mHigh;
             }
         }
         else { 
             // Given increment will not cross the high threshold.
             high = 0; 
         } 
     }
     else { 
         // Counter is already above the high threshold so other partition can cross
         // the threshold so it doesn't require any threshold to transfer.
         high = 0; 
         low = 0; 
     }


     DBG_ASSERT(high + mHigh >= low + mLow);
     DBG_ASSERT(mHigh > mLow || high == low);

     return result;
}

template <class Type>
Type BasicPartitionedDualThresholdCounter<Type>::
PerPartitionData::DetermineNeededCountForSubtractLockHeld(Type by, Type & countNeeded) const
{ 
    DBG_ASSERT(by > 0);

    Type result = by <= mCounter ? by : mCounter;

    if (mCounter > mHigh) {
        if (result > mCounter - mHigh) {
            result = result - (mCounter - mHigh);
        }
        countNeeded = 0;
    }
    else {
        
        if (mCounter > mLow) {
            if (result > mCounter - mLow) {
                result = result - (mCounter - mLow);
            }
        }

        if (mCounter == mHigh || mCounter == mLow) {
            countNeeded = result;
        }
        else {
            countNeeded = 0;
        }
    }

    return result;
}

template <class Type>
void  BasicPartitionedDualThresholdCounter<Type>::
PerPartitionData::TransferAllValuesLocksHeld( PerPartitionData * to)
{
    to->mLow += mLow;
    mLow = 0;
    to->mHigh += mHigh;
    mHigh = 0;
    to->mCounter += mCounter;
    mCounter = 0;
}

template <class Type>
void  BasicPartitionedDualThresholdCounter<Type>::
PerPartitionData::TransferThresholdAsNeededLocksHeld(Type& by, Type  & lowNeeded, Type & highNeeded, PerPartitionData * to)
{
    DBG_ASSERT(mHigh >= mLow);
    DBG_ASSERT(to->mHigh >= to->mLow);

    DBG_ASSERT(by >= lowNeeded && by >= highNeeded);

    Type lowToTransfer = 0;
    Type highToTransfer = 0;

    if (mCounter < mLow) {
        lowToTransfer = mLow - mCounter;
    }

    if (mCounter < mHigh) {
        highToTransfer = mHigh - mCounter;
    }

    lowToTransfer = CSX_MIN(lowToTransfer, lowNeeded);
    highToTransfer = CSX_MIN(highToTransfer, highNeeded);

    if (mHigh - highToTransfer < mLow - lowToTransfer) {
        lowToTransfer += (mLow - lowToTransfer) - (mHigh - highToTransfer);
    }

    mLow -= lowToTransfer;
    to->mLow += lowToTransfer;
    mHigh -= highToTransfer;
    to->mHigh += highToTransfer;

    lowNeeded -= CSX_MIN(lowToTransfer, lowNeeded);
    highNeeded -= highToTransfer;

    Type change = CSX_MIN(lowToTransfer, highToTransfer);
    if (to->mCounter >= to->mHigh) {
        change = CSX_MAX(lowToTransfer, highToTransfer);
    }

    DBG_ASSERT(change <= by);
    to->mCounter += change;
    by -= change;

    DBG_ASSERT(mHigh >= mLow);
    DBG_ASSERT(to->mHigh >= to->mLow);    
}

template <class Type>
bool  BasicPartitionedDualThresholdCounter<Type>::
PerPartitionData::TransferCountAsNeededLocksHeld(Type & countNeeded, PerPartitionData * to)
{
    bool result = false;
    Type availCount = 0;

    if (mCounter > mHigh) {
        availCount = mCounter - mHigh;
    }
    else if (mCounter > mLow && mCounter <= mHigh) {
        if (mCounter < mHigh || to->mCounter < to->mHigh || to->mCounter == to->mLow) {
            availCount = mCounter - mLow;
        }
    }
    else if (mCounter <= mLow) {
        if (mCounter < mLow || to->mCounter < to->mLow) {
            availCount = mCounter;
        }
    }

    if (availCount) {
        if(availCount > countNeeded) {
            availCount = countNeeded;
        }
    
        mCounter -= availCount;
        countNeeded -= availCount;
    }
    else if (to->mCounter < countNeeded) {
        // To has nothing. Make a transfer of counts and thresholds that will not change
        // either 'this' or 'to' but will make it possible for 'to' to decrement.

        Type change = CSX_MIN(countNeeded - to->mCounter, mCounter);
        mCounter -= change;
        mLow -= change;
        mHigh -= change;
        to->mCounter += change;
        to->mLow += change;
        to->mHigh += change;
    }

    return result;
}

template <class Type>
void BasicPartitionedDualThresholdCounter<Type>::SetThresholds(Type low, Type high)
{ 
    DBG_ASSERT(low <= high);
    
    BasicPartitionedLockAccumulator<PerPartitionData> locks(mArray, mNumCpus);

    (void) locks.GetOneMoreLock(0);

    // Move all current values to partition 0.
    for (UINT_32 ptn = 1; ptn < mNumCpus; ptn++) {
        (void) locks.GetOneMoreLock(ptn);

        mArray[ptn].TransferAllValuesLocksHeld(&mArray[0]);
    }

    // Now partition 0 has all the information.
    mArray[0].SetThresholds(low, high);

    // Since 0 have all information, it also defines state.
    RecordTransition(mArray[0].PositionLockHeld());
    mLastPosition = mArray[0].PositionLockHeld();

    // Destructor releases all the locks.
}

template <class Type>
void BasicPartitionedDualThresholdCounter<Type>::Initialize(Type low, Type high)
{
    if(!mInitialized) {
        SetThresholds(low,high);
        mInitialized = true;
    }
}
    
template <class Type>
void BasicPartitionedDualThresholdCounter<Type>::Increment(Type by)
{
    UINT_32 partition = Hash();

    // Track which locks are held, starting with this partition
    // Destructor release all locks.
    BasicPartitionedLockAccumulator<PerPartitionData> locks(mArray, mNumCpus);

    // As we iterate, we get more and more locks.  

    // Succeeds immediately if lock already acquired. Should always return true.
    if (!locks.GetOneMoreLock(partition)) {
        FF_ASSERT(false);   // never should release for deadlock avoidance here.
    }

    PerPartitionData * p = &mArray[partition];
    
    ULONG restartCounter = 0;

restart:  
    restartCounter++;
    DBG_ASSERT(restartCounter <= 1000);
    
    // Note that this algorithm can return Position only because by >= 1, which would
    // cause it to search all partitions.
    DBG_ASSERT(by > 0);

    Type  lowNeeded = 0; 
    Type  highNeeded = 0;

    // How much more threshold do we need in this partition?
    Type thisTime = p->DetermineNeededThresholdsForAddLockHeld(by, lowNeeded, highNeeded);

    FF_ASSERT(thisTime);

    if (lowNeeded == 0 &&  highNeeded == 0) {
        // Increment the counter. It may change the position.
        p->ForceAddLockHeld(thisTime);

        // Deduct the value from the desired value to be incremented.
        by -= thisTime;

        if ( by == 0) {
            // Since we just did the add, and we still hold the locks, our
            // partition has the exact representation of the global state.
            RelativeToThreshold position = p->PositionLockHeld();

            bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();
            if (position != mLastPosition && !sameThreshold) { 
                RecordTransition(position);
                mLastPosition = position; 
            }
            DebugAssertGlobalConsistency();
            return;
        }

        goto restart;
    }   

    // Try to find free threshold in other partitions, and move it to this one.
    for (UINT_32 ptn = (partition + 1 ) % mNumCpus;ptn != partition;  ptn = (ptn + 1 ) % mNumCpus) {
        PerPartitionData * other = &mArray[ptn];

        // If a CPU seems like it has the needed threshold, get the lock, and try to transfer some.
        if ( other->HasNeededThresholdOrInfo(lowNeeded, highNeeded) ) {

            if (!locks.GetOneMoreLock(ptn)) {
                // Deadlock was avoided, but locks were releases and reacquired.
                goto restart;
            }

            // This may transfer threshold, or it may simply detect that the 
            // threshold is already exceeded in this partition, and zero
            // the needed count.
            Type toTransfer = thisTime;
            other->TransferThresholdAsNeededLocksHeld(toTransfer, lowNeeded, highNeeded, p);

            if (toTransfer < thisTime) {

                // Deduct the value from the desired value to be incremented.
                by -= (thisTime - toTransfer);
                thisTime = toTransfer;

                if ( by == 0) {
                    // Since we just did the add, and we still hold the locks, our
                    // partition has the exact representation of the global state.
                    RelativeToThreshold position = p->PositionLockHeld();

                    bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();
                    if (position != mLastPosition && !sameThreshold) { 
                        RecordTransition(position);
                        mLastPosition = position; 
                    }
                    DebugAssertGlobalConsistency();
                    return;
                }

                if (thisTime == 0){
                    goto restart;
                }
            }
        }
    }


    // We need to lock all the partitions, not just candidate ones.
    for (UINT_32 ptn = (partition + 1 ) % mNumCpus;ptn != partition; ptn = (ptn + 1 ) % mNumCpus) {
        PerPartitionData * other = &mArray[ptn];

        // Make sure we get all locks to cross threshold boundary.
        if (!locks.GetOneMoreLock(ptn)) {
            // Deadlock was avoided, but locks were releases and reacquired.
            goto restart;
        }

        // Are we already above threshold in this partition?
        Type toTransfer = thisTime;
        other->TransferThresholdAsNeededLocksHeld(toTransfer, lowNeeded, highNeeded, p);

        if (toTransfer < thisTime) {
            // Deduct the value from the desired value to be incremented.
            by -= (thisTime - toTransfer);
            thisTime = toTransfer;

            if ( by == 0) {
                // Since we just did the add, and we still hold the locks, our
                // partition has the exact representation of the global state.
                RelativeToThreshold position = p->PositionLockHeld();

                bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();
                if (position != mLastPosition && !sameThreshold) { 
                    RecordTransition(position);
                    mLastPosition = position; 
                }
                DebugAssertGlobalConsistency();
                return;
            }

            if (thisTime == 0){
                goto restart;
            }
        }
    }

    // We didn't get the required threshold to be on the same
    // position so now forcefully increment the counter which
    // will definately change the position.
    p->ForceAddLockHeld(1);

    // Deduct the value from the desired value to be incremented.
    by -= 1;

    if ( by == 0) {
        // Since we just did the add, and we still hold the locks, our
        // partition has the exact representation of the global state.
        RelativeToThreshold position = p->PositionLockHeld();

        bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();
        if (position != mLastPosition && !sameThreshold) { 
            RecordTransition(position);
            mLastPosition = position; 
        }
        DebugAssertGlobalConsistency();
        return;
    }

    goto restart;
}

template <class Type>
void BasicPartitionedDualThresholdCounter<Type>::Decrement(Type by)
{
    UINT_32 partition = Hash();

    // Track which locks are held, starting with this partition
    // Destructor release all locks.
    BasicPartitionedLockAccumulator<PerPartitionData> locks(mArray, mNumCpus);

    // As we iterate, we get more and more locks.  

    // Succeeds immediately if lock already acquired. Should always return true.
    if (!locks.GetOneMoreLock(partition)) {
        FF_ASSERT(false);   // never should release for deadlock avoidance here.
    }

    PerPartitionData * p = &mArray[partition];

    ULONG restartCounter = 0;

restart:  
    restartCounter++;
    DBG_ASSERT(restartCounter <= 1000);

    DBG_ASSERT(by > 0);
    Type  countNeeded = 0;

    // Counts needed to be in the same position.
    Type thisTime = p->DetermineNeededCountForSubtractLockHeld(by, countNeeded);

    if (countNeeded == 0 && thisTime > 0) {
        // We don't need any counter from other partition so go
        // ahead and decrement the count.
        p->ForceSubtractLockHeld(thisTime);
        by -= thisTime;

        if (by == 0) {

            // Since we just did the subtract, and we still hold the locks, our
            // partition has the exact representation of the global state.
            RelativeToThreshold position = p->PositionLockHeld();

            bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();

            if (position != mLastPosition && !sameThreshold) {
                RecordTransition(position);
                mLastPosition = position;
            }
            DebugAssertGlobalConsistency();
            return;
        }

        goto restart;
    }
    else if (thisTime == 0) {
        // We started with 0 count.
        countNeeded = by;
        thisTime = by;
    }

    // Try to find available count in other partitions, and move it to this one.
    for (UINT_32 ptn = (partition + 1 ) % mNumCpus;ptn != partition;  ptn = (ptn + 1 ) % mNumCpus) {
        PerPartitionData * other = &mArray[ptn];

        // If a CPU seems like it has the needed count, get the lock, and try to transfer some.
        if ( other->CouldYieldCountOfAtLeastOne() ) {

            if (!locks.GetOneMoreLock(ptn)) {
                // Deadlock was avoided, but locks were releases and reacquired.
                goto restart;
            }

            // This may transfer needed or some count.
            Type toTransfer = countNeeded;
            if (other->TransferCountAsNeededLocksHeld(toTransfer, p)) {
                goto restart;
            }

            if (toTransfer < countNeeded) {
                thisTime -= countNeeded - toTransfer;
                by -= countNeeded - toTransfer;
                countNeeded = toTransfer;

                if (by == 0) {
                    // Since we just did the subtract, and we still hold the locks, our
                    // partition has the exact representation of the global state.
                    RelativeToThreshold position = p->PositionLockHeld();

                    bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();

                    if (position != mLastPosition && !sameThreshold) { 
                        RecordTransition(position);
                        mLastPosition = position; 
                    }
                    DebugAssertGlobalConsistency();
                    return;
                }

                if (thisTime == 0) {
                    goto restart;
                }

            }
            
        }
    }


    // We need to lock all the partitions, not just candidate ones.
    for (UINT_32 ptn = (partition + 1 ) % mNumCpus;ptn != partition;  ptn = (ptn + 1 ) % mNumCpus) {
        PerPartitionData * other = &mArray[ptn];

        // If a CPU seems like it has the needed counter, get the lock, and try to transfer some.
        if (!locks.GetOneMoreLock(ptn)) {
            // Deadlock was avoided, but locks were releases and reacquired.
            goto restart;
        }

        // This may transfer needed or some count.
        Type toTransfer = countNeeded;
        if (other->TransferCountAsNeededLocksHeld(toTransfer, p)) {
            goto restart;
        }

        if (toTransfer < countNeeded) {
            thisTime -= countNeeded - toTransfer;
            by -= countNeeded - toTransfer;
            countNeeded = toTransfer;

            if (by == 0) {
                // Since we just did the subtract, and we still hold the locks, our
                // partition has the exact representation of the global state.
                RelativeToThreshold position = p->PositionLockHeld();

                bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();

                if (position != mLastPosition && !sameThreshold) { 
                    RecordTransition(position);
                    mLastPosition = position; 
                }
                DebugAssertGlobalConsistency();
                return;
            }

            if (thisTime == 0) {
                goto restart;
            }
        }

    }

    DBG_ASSERT(countNeeded > 0 && by );

    // We didn't get all the needed count so subtract whatever we can. Remaining
    // will be subtracted in next iteration.
    p->ForceSubtractLockHeld(1);

    // Deduct the value from the desired count to be decremented.
    by -= 1;

    if (by == 0) {
        // Since we just did the subtraction, and we still hold the locks, our
        // partition has the exact representation of the global state.
        RelativeToThreshold position = p->PositionLockHeld();

        bool sameThreshold = mLastPosition == BelowThreshold && p->IsAtLowThreshold() || mLastPosition == AboveThreshold && p->IsAtHighThreshold();

        if (position != mLastPosition && !sameThreshold) { 
            RecordTransition(position);
            mLastPosition = position; 
        }
        DebugAssertGlobalConsistency();
        return;
    }

    goto restart;
}

#endif  // __BasicPartitionedDualThresholdCounter_h__
