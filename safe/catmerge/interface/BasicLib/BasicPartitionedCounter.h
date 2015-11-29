#ifndef __BasicPartitionedCounter_h__
#define __BasicPartitionedCounter_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2011
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedCounter.h
//
// Contents:
//      Defines the BasicPartitionedCounter class. 
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
// It provides notification on transitions between counts of 0 and 1.
// 
// @param Type - the type of counter
// @param NumElements - the number of counters supported.  Default is 1, which allows
//                      counter # to be left off Add/Subtract calls.
// @param NumParts - the number of partitions of the counter
template <class Type, int NumElements=1, int NumParts=8>
class BasicPartitionedCounter  {
public:

    void AcquireSpinLock(ULONG element) {
        mNonZero[element].AcquireSpinLock();
    }

    void ReleaseSpinLock(ULONG element) {
        mNonZero[element].ReleaseSpinLock();
    }

    // Returns true if all counters are 0. The caller can make this atomic by acquiring
    // the spinlock for the element
    bool IsZero(ULONG element) const { return mNonZero[element].mBitmap == 0; }

    // Add to the counter.
    // @param element - the element # to add to.
    // @param by      - the amount to add.
    // Returns: true on a transistion from 0, false otherwise.  
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

                FF_ASSERT( (mNonZero[element].mBitmap & (((ULONGLONG)1)<<ptn)) == 0);

                bool result = (mNonZero[element].mBitmap == 0);


                // Mark global bitmap has this counter is non-zero.
                mNonZero[element].mBitmap |= (((ULONGLONG)1)<<ptn);

                // The spin-lock protects the transtion from zero.
                p[ptn].mCounter[element] = by;
 
                mNonZero[element].ReleaseSpinLock();
                return result;
            }
            else {

                Type result;

                result = InlineInterlockedCompareAndExchangeULONGLONG(&p[ptn].mCounter[element], counterCopy, counterCopy+by);

                if(result == (counterCopy)) {
                    return false;
                }

            }
        }
    }

    // Subtract from the counter.
    // @param element - the element # to subtract from.
    // @param by - the amount to subtract.
    // Returns: true on a transistion to 0, false otherwise.
    bool Subtract(ULONG element, Type by=1)
    {
        // Select a partition to try to substract from.
        ULONG ptn = Hash();
        ULONG CSX_MAYBE_UNUSED debugCounter = 0;
        ULONG hasLock = 0;
        ULONG first = ptn;

        while(by)  {
            Type counterCopy = p[ptn].mCounter[element];

            FF_ASSERT(debugCounter++ < 1000000);

            Type sub = by;
            if (counterCopy < sub) {  sub = counterCopy; }

            // We only need to modify the global bitmap of non-zero counters on
            // transitition to 0 don't need to modify the global bitmap.

            if (counterCopy-sub == 0) {
                
                // Is this slot completely empty?
                if (counterCopy == 0) {
                    ptn = (ptn + 1) % NumParts;

                    // Did all slots seem to be empty (possible that another thread
                    // incremented behind us, then decremented ahead of us).
                    if (ptn == first) {
                        if (!hasLock) {

                            mNonZero[element].AcquireSpinLock();
                        }
                        hasLock++;   
                        // Die if we took an entire pass with the lock held, and found nothing.
                        // 1 for first pass, 1 for second pass, 1 if lock acquired for another reason.
                        FF_ASSERT(hasLock <= 3);
                    }
                    continue;
                }

                if (!hasLock) {
                    mNonZero[element].AcquireSpinLock();
                    hasLock = 1;
                }

                Type result = InlineInterlockedCompareAndExchangeULONGLONG(&p[ptn].mCounter[element], counterCopy, 0);

                if(result != counterCopy) {
                    continue;
                }

                FF_ASSERT( (mNonZero[element].mBitmap & (((ULONGLONG)1)<<ptn)) != 0);
                
                // Mark global bitmap as this counter is zero.
                mNonZero[element].mBitmap &= ~(((ULONGLONG)1)<<ptn); 
                
                if (mNonZero[element].mBitmap == 0) {
                    FF_ASSERT(by-sub == 0);
                    mNonZero[element].ReleaseSpinLock();
                    return true;
                }

                by -= sub;

                // Forward progress.
                if (hasLock) hasLock = 1;


            }
            else {

                Type result = InlineInterlockedCompareAndExchangeULONGLONG(&p[ptn].mCounter[element], counterCopy, counterCopy-sub);

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

    // Provides a sum of all of the counters, but this is unlocked.  Acquiring the spin
    // lock will *not* make this atomic.
    Type Sum(ULONG element) const 
    {
        Type sum = 0;

        for(ULONG i = 0; i < NumParts; i++) {
            sum += p[i].mCounter[element];
        }
        return sum;
    }

private:

    // Returns a partition number within the range allowed, based on the CPU that
    // execution occurs on.
    ULONG Hash() const { 
        // FIX use OS independent implementation.
        return csx_p_get_processor_id() % NumParts;
    }



    // There is a counter per partition per element. We lay out the data to reduce
    // cache-line contention for the typical cases.

    CSX_ALIGN_N(CSX_AD_L1_CACHE_LINE_SIZE_BYTES) struct Counters  {

        Counters() { for(ULONG i= 0; i < NumElements; i++) { mCounter[i]=0; } }
        // ULONG
        volatile Type       mCounter[NumElements];

    } p[NumParts];

    struct NonZeroBitmap : BasicLockedObject {
        NonZeroBitmap() : mBitmap(0) {}

        // bomb compliation if too many partitions for ULONGLONG
        typedef char Asserter[NumParts<=64];

        // bit per partition, indicating that the corresponding counter is zero or not
        volatile ULONGLONG   mBitmap;
    } mNonZero[NumElements];
};



#endif  // __BasicPartitionedCounter_h__
