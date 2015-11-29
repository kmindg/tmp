#ifndef __BasicPartitionedCounterArray_h__
#define __BasicPartitionedCounterArray_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2011
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedCounterArray.h
//
// Contents:
//      Defines the BasicPartitionedCounterArray class. 
//
// Revision History:
//--


# include "csx_ext_p_core.h"

// Provides a counter mechanism that is distributed over multiple cores to avoid cache
// contention. This allows very frequent increments and decrements while keeping the
// accesses core local. It models a singly dimensioned array of counters 
// but the internal implementation divides each of
// those into per-partition counters, to reduce cache movement between cores.
// The GetValue operation is more expensive and only approximately accurate
// if the values are changing (can return small negatives even if not logically
// possible).  If changes are stopped, the values will stabalize and be consistent with all the calls,
// i.e., the counters here are accurate over time, and do not accumulate errors.  It is only
// reading while they are changing that may be off.
//
// @param NumElements - the number of counters supported.  Default is 1, which allows
//                      counter # to be left off Add/Subtract calls.
template <int NumElements=1>
class BasicPartitionedCounterArray  {
public:

    BasicPartitionedCounterArray() 
    {
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        mNumCpus = sys_info.os_cpu_count;
        mArray = new Counters[mNumCpus];
        FF_ASSERT(mArray);
    }

    ~BasicPartitionedCounterArray()
    {
        delete mArray;
    }

    // Add to the counter.
    // @param element - the element # to add 1 to.
    void Add(ULONG element)
    {
        EmcpalInterlockedIncrement64(&mArray[csx_p_get_processor_id()].mCounter[element]);
    }

    // Add a value to the counter.
    // @param element - the element # to add value to.
    // @param value - the value to be added.
    void Add(ULONG element, INT_64 value)
    {
        EmcpalInterlockedExchangeAdd64(&mArray[csx_p_get_processor_id()].mCounter[element], value);
    }

    // Subtract from the counter.
    // @param element - the element # to subtract 1 from.
    void Subtract(ULONG element)
    {
        EmcpalInterlockedDecrement64(&mArray[csx_p_get_processor_id()].mCounter[element]);
    }

    // Subtract from the counter.
    // @param element - the element # to subtract value from.
    // @param value - the value to be subtracted.
    void Subtract(ULONG element, INT_64 value)
    {
        // Since there isn't a subtract64, add a negative of the value passed in.
        INT_64 subValue = (-1) * value;
        EmcpalInterlockedExchangeAdd64(&mArray[csx_p_get_processor_id()].mCounter[element], subValue);
    }

    // Decrement from, increment toElement.
    void MoveCount(ULONG fromElement, ULONG toElement)
    {
        Counters * counter =  &mArray[csx_p_get_processor_id()];
        EmcpalInterlockedDecrement64(&(counter->mCounter[fromElement]));
        EmcpalInterlockedIncrement64(&(counter->mCounter[toElement]));
    }


    // Provides a sum of all of the counters, but this is unlocked.  Acquiring some
    // lock will *not* make this atomic.
    INT_64 GetValue(ULONG element) const 
    {
        INT_64 sum = 0;

        for(ULONG cpu = 0; cpu < mNumCpus; cpu++) {
            sum += mArray[cpu].mCounter[element];
        }
        return sum;
    }

    // Provides a sum of all of the counters, for all elements, but this is unlocked.  Acquiring some
    // lock will *not* make this atomic.
    void GetValue(INT_64  result[NumElements]) const 
    {
        for(ULONG j = 0; j < NumElements; j++) {
            result[j] = 0;
        }

        for(ULONG cpu = 0; cpu < mNumCpus; cpu++) {

            // We SUM all elements per CPU for efficiency.
            for(ULONG j = 0; j < NumElements; j++) {
                result[j] += mArray[cpu].mCounter[j];
            }
        }
    
        //return sum;
    }
    
    // If the output requested is unsigned, then the values should
    // logically never be negative.  If they are, this is just some window
    // we hit with changes, so we force the result ot zero to avoid
    // confusing the caller.
    void GetValue(UINT_64  result[NumElements]) const 
    {
        for(ULONG j = 0; j < NumElements; j++) {
            result[j] = 0;
        }

        for(ULONG cpu = 0; cpu < mNumCpus; cpu++) {
            for(ULONG j = 0; j < NumElements; j++) {
                result[j] += mArray[cpu].mCounter[j];
            }
        }
        for(ULONG j = 0; j < NumElements; j++) {
            if (((INT_64)(result)[j]) < 0) {
                result[j] = 0;
            }
        }
    }


private:

    // There is a counter per partition per element. We lay out the data to reduce
    // cache-line contention for the typical cases.

    struct CSX_ALIGN_N(CSX_AD_L1_CACHE_LINE_SIZE_BYTES) Counters  {

        Counters() { for(ULONG i= 0; i < NumElements; i++) { mCounter[i]=0; } }
        // ULONG
        volatile INT_64       mCounter[NumElements];

    };

    // Size of array
    UINT_32    mNumCpus;

    
    // Array of counters.
    Counters * mArray;
};



#endif  // __BasicPartitionedCounter_h__
