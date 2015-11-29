#ifndef __BasicPartitionedCounterArrayWithAtomicEnableFlag_h__
#define __BasicPartitionedCounterArrayWithAtomicEnableFlag_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedCounterArrayWithAtomicEnableFlag.h
//
// Contents:
//      Defines the BasicPartitionedCounterArrayWithAtomicEnableFlag class. 
//
// Revision History:
//--


# include "csx_ext_p_core.h"

// Provides a counter mechanism that is distributed over multiple cores to avoid cache
// contention. This allows very frequent increments and decrements while keeping the
// accesses core local. It models a singly dimensioned array of counters (or a single
// counter with all default arguments), but the internal implementation divides each of
// those into per-partition counters, to reduce cache movement between cores.
//
// In addition to the counter, an enable bit is atomically maintained, such that the
// element can be marked as enabled only if the counter is non-zero, and marked as
// disabled, only if the counter is zero.   This allows asynchronous action to be taken on
// a transition from zero to non-zero, where the holder of the counter can later enable,
// and all other callers of Add will be informed that the enable has not occured.
// Similarly, a subtract that results in zero can initiate an asynchronous action that
// results in a disable.
// 
// @param Type - the type of counter
// @param NumElements - the number of counters supported.  Default is 1, which allows
//                      counter # to be left off Add/Subtract calls.
template <class Type, int NumElements=1>
class BasicPartitionedCounterArrayWithAtomicEnableFlag  {

    
public:

    BasicPartitionedCounterArrayWithAtomicEnableFlag() : mTotalNonZero(0) {
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        mNumCpus = sys_info.os_cpu_count;
        // If assert fails, fix struct NonZeroBitmap which is using a ULONGLONG
        // as a bitmask.   A ULONGLONG is currently 64 bits.  The 63 bit is used
        // as an enabled bit, so if we have a system with more than 63 CPUs this
        // implementation will have to change.
        FF_ASSERT(mNumCpus < 64);
        p = new BasicCounters[mNumCpus];
        FF_ASSERT(p);
    }

    ~BasicPartitionedCounterArrayWithAtomicEnableFlag() {
        delete p;
    }

    // Returns the number of bytes that this class dynamically allocates
    // when it is instantiated.
    static ULONG GetAllocationSizeBytes() {
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        csx_nuint_t cpuCnt = sys_info.os_cpu_count;
        return (sizeof(BasicCounters) * cpuCnt);
    }

    void AcquireSpinLock(ULONG element) {
        mNonZero[element].AcquireSpinLock();
    }

    void ReleaseSpinLock(ULONG element) {
        mNonZero[element].ReleaseSpinLock();
    }

    // Returns true if all counters are 0. The caller can make this atomic by acquiring
    // the spinlock for the element. Independent of Enabled.
    // @param element - the element  add to act on
    bool IsZero(ULONG element) const { return mNonZero[element].IsZero(); }

    // Returns true if all counters are 0, false otherwise.
    bool AllZero() const { return mTotalNonZero == 0; }

    // Returns true if the element is enabled, regardless of the count.
    // @param element - the element  add to act on
    bool IsEnabled(ULONG element) const { return mNonZero[element].IsEnabled(); }

    // Set the enabled flag to a specific value for the specific element, if that setting
    // is consistent with the count.  Setting to true is only legal if the count is > 0,
    // and setting to false is legal only if the count == 0.
    // @param element - the element  add to act on
    // @param Enabled - the desired value of the enabled flag.
    // Returns: true if the enabled flag == the parameter, false if the change was not
    // legal.
    bool SetEnabled(ULONG element, bool Enabled) { return mNonZero[element].SetEnabled(Enabled); }
    
    // Force the enabled flag to a specific value even if that value is inconsistent
    // with the count.
    // @param element - the element add to act on
    // @param Enabled - the desired value of the enabled flag.
    void ForceEnabled(ULONG element, bool Enabled) { mNonZero[element].ForceEnabled(Enabled); }

    // Add to the counter.
    // @param element - the element # to add to.
    // @param by      - the amount to add.
    // Returns: false if disabled, true if is enabled.  If true, then enabled cannot
    // transition back to disabled because of the incremented counter.
    bool Add(ULONG element, Type by=1)
    {
        ULONG ptn = Hash();


        for(;;) {
            Type counterCopy = p[ptn].mCounter[element];

            // If the specific counter is already > 0, then this cannot be a transition
            // from 0, and we don't need to modify the global bitmap.
            if (counterCopy == 0) {
                mNonZero[element].AcquireSpinLock();

                // Only the spinlock holder can transition from 0.
                if (p[ptn].mCounter[element] != 0) { 
                    mNonZero[element].ReleaseSpinLock();
                    continue;
                }

                // Track the total number of array elements that are non-zero.
                if (mNonZero[element].IsZero()) EmcpalInterlockedIncrement(&mTotalNonZero);

                FF_ASSERT( (mNonZero[element].mBitmap & (((ULONGLONG)1)<<ptn)) == 0);

                // Mark global bitmap has this counter is non-zero.
                mNonZero[element].mBitmap |= (((ULONGLONG)1)<<ptn);  

                // The spin-lock protects the transtion from zero.
                p[ptn].mCounter[element] = by;
 
                mNonZero[element].ReleaseSpinLock();

                // Report enabled after 
                return mNonZero[element].IsEnabled();
            }
            else {

                Type result;

                result = InlineInterlockedCompareAndExchangeULONGLONG(&p[ptn].mCounter[element], counterCopy, counterCopy+by);

                if(result == (counterCopy)) {
                    return mNonZero[element].IsEnabled();
                }
            }
        }
    }

    // Subtract from the counter.
    // @param element - the element # to subtract from.
    // @param by - the amount to subtract.
    // Returns: true if the resulting count is zero, false otherwise
    bool Subtract(ULONG element, Type by=1)
    {
        // Select a partition to try to substract from.
        ULONG ptn = Hash();
        ULONG debugCounter = 0;
        ULONG hasLock = 0;
        ULONG first = ptn;

        while(by)  {
            Type counterCopy = p[ptn].mCounter[element];

            FF_ASSERT(debugCounter < 1000000);
            debugCounter++;

            Type sub = by;
            if (counterCopy < sub) {  sub = counterCopy; }

            // We only need to modify the global bitmap of non-zero counters on
            // transitition to 0 don't need to modify the global bitmap.

            if (counterCopy-sub == 0) {
                
                // Is this slot completely empty?
                if (counterCopy == 0) {
                    ptn = (ptn + 1) % mNumCpus;

                    // Did all slots seem to be empty (possible that another thread
                    // incremented behind us, then decremented ahead of us).
                    if (ptn == first) {
                        if (!hasLock) {

                            mNonZero[element].AcquireSpinLock();
                        }
                        hasLock++;   
                        // Die if we took an entire pass with the lock held, and found
                        // nothing.
                        // 1 for first pass, 1 for second pass, 1 if lock acquired for
                        // another reason.
                        FF_ASSERT(hasLock <= 3);
                    }
                    continue;
                }

                if (!hasLock) {
                    mNonZero[element].AcquireSpinLock();
                    hasLock = 1;
                }

                Type result;

                result = InlineInterlockedCompareAndExchangeULONGLONG(&p[ptn].mCounter[element], counterCopy, 0);

                if(result != counterCopy) {
                    continue;
                }

                FF_ASSERT( (mNonZero[element].mBitmap & (((ULONGLONG)1)<<ptn)) != 0);

                // Mark global bitmap as this counter is zero.
                mNonZero[element].mBitmap &= ~(((ULONGLONG)1)<<ptn);

                if (mNonZero[element].IsZero()) {
                    FF_ASSERT(by-sub == 0);
                    (VOID) EmcpalInterlockedDecrement(&mTotalNonZero);
                    mNonZero[element].ReleaseSpinLock();
                    return true;
                }

                by -= sub;

                // Forward progress.
                if (hasLock) hasLock = 1;


            }
            else {
                Type result;

                result = InlineInterlockedCompareAndExchangeULONGLONG(&p[ptn].mCounter[element], counterCopy, counterCopy-sub);

                if(result == (counterCopy)) {
                    by -= sub;

                    // Forward progress.
                    if (hasLock) hasLock = 1;
                }
            }
        }

        if (hasLock) mNonZero[element].ReleaseSpinLock();
        return false;
    }

    // Provides a sum of all of the counters for a single array element, but this is
    // unlocked.  Acquiring the spin lock will *not* make this atomic.
    Type Sum(ULONG element) const 
    {
        Type sum = 0;

        for(ULONG i = 0; i < mNumCpus; i++) {
            sum += p[i].mCounter[element];
        }
        return sum;
    }


    // Returns the sum across all array elements and partitions. For 8000 8-ptns ULONGLONG
    // ths will do 64K adds, reading 4000 cache lines sequentially.
    // FIX:  can we keep a count by partition across all elements which is not a memory
    // barrier instruction but is atomically incremented/decremented itself, and then just
    // sum this across partitions?  (Particular partitions could end up with negative
    // values.
    Type Sum() const 
    {
        Type sum = 0;

        for(ULONG i = 0; i < mNumCpus; i++) {
            sum += p[i].Sum();
        }
        return sum;
    }

private:

    // Returns a partition number within the range allowed, based on the CPU that
    // execution occurs on.
    ULONG Hash() const { 
        // FIX use OS independent implementation.
        return csx_p_get_processor_id() % mNumCpus;
    }



    // There is a counter per partition per element. We lay out the data to reduce
    // cache-line contention for the typical cases.

    typedef CSX_ALIGN_N(CSX_AD_L1_CACHE_LINE_SIZE_BYTES)  struct Counters  {

        Counters() { for(ULONG i= 0; i < NumElements; i++) { mCounter[i]=0; } }

        Type Sum() const 
        { 
            Type sum = 0;
            for(ULONG i= 0; i < NumElements; i++) { sum += mCounter[i]; }
        }

        volatile Type       mCounter[NumElements];

    } BasicCounters, *PBasicCounters;
    PBasicCounters p;

    struct NonZeroBitmap : BasicLockedObject {
        NonZeroBitmap() : mBitmap(0) {}

        static const ULONGLONG EnabledBit = 0x8000000000000000ULL;

        bool SetEnabled(bool Enabled) {
            bool result = false;
            AcquireSpinLock();

            if (Enabled) {
                if ((mBitmap & ~EnabledBit) != 0) {
                    mBitmap |= EnabledBit;
                    result = true;
                }
            }
            else {
                if ((mBitmap & ~EnabledBit) == 0) {
                    mBitmap &= ~EnabledBit;
                    result = true;
                }
            }
            ReleaseSpinLock();
            return result;
        }
        
        void ForceEnabled(bool Enabled) {
            AcquireSpinLock();
            
            if (Enabled) {
                mBitmap |= EnabledBit;
            }
            else {
                mBitmap &= ~EnabledBit;
            }
            
            ReleaseSpinLock();
        }

        bool IsEnabled() const {
            return ((mBitmap & EnabledBit)  != 0);
        }

        bool IsZero() const {
            return ((mBitmap & ~EnabledBit)  == 0);
        }
            

        // We only can support 63 cpus with this implementation.  THe constructor of the
        // parent class will assert if there are more than 63 cpus on the system.
        //
        // Note We use the high bit per partition, indicating that the corresponding counter is zero or not
        volatile ULONGLONG   mBitmap;
    } mNonZero[NumElements];

    // Number of BasicCounter Elements
    LONG mNumCpus;

    // The number of counters that are non-zero.
    LONG mTotalNonZero;
};



#endif  // __BasicPartitionedCounterArrayWithAtomicEnableFlag_h__
