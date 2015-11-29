#ifndef __ReplacementParams_h__
#define __ReplacementParams_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      ReplacementParams.h
//
// Contents:
//      Defines parameters related to ReplacementPool and Replacement tracks. 
//
// Revision History: Jakir Phoplunkar
//      11-23-2009 -- Created. Jakir Phoplunkar
//--

// Default mPreCleanIncrementOnCleanedWriteHit to 10ms
#define DEFAULT_PRECLEAN_INCREMENT_ON_CLEANED_WRITE_HIT_MS 10000

// Time since the last write-throttle to make writes on a volume high priority -- Time in usec -- Default totalling 20 sec
// We make writes high priority when throttling to get more flushes to the disk and drain the pending queue faster
#define DEFAULT_RECENTWRITETHROTTLELOOKBACKTIME (20*1000*1000)

// Parameters related to replacement track.
struct ReplacementTrackParameters
{    
    ReplacementTrackParameters() : mWeight(1), minResponseTime(0), minNumberHits(0), 
        mVolumeHit(0), mWorstCaseResponseTimeOfSlowestDevice( 16000 ),
        mInitialPreCleanValue(2*1000*1000), // 2 secs
        mPreCleanMinValue(10*1000),
        mPreCleanMaxValue(120*1000*1000), // 120 sec
        mPreCleanIncrementOnCleanedWriteHit(DEFAULT_PRECLEAN_INCREMENT_ON_CLEANED_WRITE_HIT_MS),
        mPreCleanIncrementOnCleanedReadHit(0),
        mPreCleanDecrementOnCleanedPageAllocation(10),
        mPreCleanDecrementOnYoungerCleanPageAllocation(1000),
        mWriteThrottleLowerLimitMinimum(2*1000*1000),  // 2 seconds
        mWriteThrottleMaxThrottleRatio16(8*16), // 8x flush rate
        mWriteThrottleMinThrottleRatio16(8) // 50% of flush rate
    {}

    // Check for sanity of values.
    bool Validate() const {
       if ( mWeight == 0) return false;

       if (mPreCleanMaxValue < 1000) return false;
       if (mPreCleanMaxValue < mPreCleanMinValue) return false;
       if (mInitialPreCleanValue > 100*1000*1000) return false;
       
       return true;
    }

    // Weight of this track
    ULONG mWeight; 
    
    // Selection criteria for the track based on which it will be decided if the incoming
    // reference is to be added in this track or not.

    // Minimum response time this track should have.
    ULONG minResponseTime;

    // Minimum number of hits a reference has to have before it gets added to this track.
    ULONG minNumberHits;

    // Recent Read/Write hit rate must be above this value.
    ULONG mVolumeHit;

    // An estimate of the response time of the slowest device when it is single threaded.
    // This is used for write-throttling to avoid throttling slow devices
    EMCPAL_TIME_USECS   mWorstCaseResponseTimeOfSlowestDevice;


    // Now parameters that affect the track behavior

    // Initial pre-clean age value (set on every update).
    EMCPAL_TIME_USECS   mInitialPreCleanValue;

    // The pre-cleaning age should never go below this value.
    EMCPAL_TIME_USECS   mPreCleanMinValue;

    // Idle flush timer.   The pre-cleaning age should never go above this value.
    EMCPAL_TIME_USECS   mPreCleanMaxValue;

    // How much should the pre-clean timer be incremented on a write hit on the cleaned track?
    EMCPAL_TIME_USECS   mPreCleanIncrementOnCleanedWriteHit;

    // How much should the pre-clean timer be incremented on a write hit on the cleaned track?
    EMCPAL_TIME_USECS   mPreCleanIncrementOnCleanedReadHit;

    // On any allocation from the Cleaned list, decrement the PreCleanTime by this much.
    EMCPAL_TIME_USECS   mPreCleanDecrementOnCleanedPageAllocation;

    // On any allocation from the Clean list when the Dirty list is older, decrement the PreCleanTime by this much.
    EMCPAL_TIME_USECS   mPreCleanDecrementOnYoungerCleanPageAllocation;

    // If the drain time for a volume group is below this value, never write-throttle. If the
    // drain time for a volume group exceeds the calculated write throttle (which cannot be lower than this value),
    // write miss completions will be delayed to match (at a sliding ratio) the BE completion rate, so that the device cannot
    // consume too much cache.
    EMCPAL_TIME_USECS   mWriteThrottleLowerLimitMinimum;

    // The ratio (times 16) of incoming write pages allowed to the number of flushed pages when throttling first begins,
    // which is when the drain time plus the pre-cleaning age = the oldest page age.
    // This is the maximum ratio used; the ratio scales down to 1 when the drain time = oldest page age.
    // This must be >= 16 (ie., ratio >= 1) so that when throttling first starts, it does not throttle too much
    // It should be > 16, to allow a linear throttling region around where the ratio is 1:1
    ULONG   mWriteThrottleMaxThrottleRatio16;

    // The minimum ratio (times 16) of incoming write pages allowed to the number of flushed pages when
    // the drain time plus the pre-cleaning age is greater than the oldest page, so that the throttle ratio has
    // scaled down from the max, through 1, and is going toward 0. This limits the minimum value so we are
    // always allowing some incoming write pages.
    // This must be <= 16 (ie., ratio <= 1) or we will not write throttle enough as drain time increases
    // It should be < 16 to allow a linear throttling region around where the ratio is 1:1
    ULONG   mWriteThrottleMinThrottleRatio16;
};

// Statistics about Replacement Tracks.
class ReplacementTrackStatistics
{    

public:

    // Age of the oldest reference in the track
    ULONGLONG mOldestAgeEstimate;

    // Precleaning Age in microsecs
    ULONGLONG mPreCleaningAge;

    // Number of Read hits
    ULONGLONG mReadHits;

    // Number of Read miss
    ULONGLONG mPagesReplaced;

    // Number of Write overwrites
    ULONGLONG mWriteHits;

    // Non-dirty page writes
    ULONGLONG mNonDirtyWrites;

    // Average time for a reference to be written across all devices
    ULONGLONG mRecentAverageAcrossDeviceWriteTime;
    
    // Average time for a reference to be flushed including pending queue time
    ULONGLONG mRecentAverageAcrossDeviceFlushTime;

    // Average age of the oldest clean or cleaned page
    ULONGLONG mRecentAverageOldestPageAge;

    // Number of writes to clean page which had been previously dirty.
    ULONGLONG mPreviouslyDirty;

public:

    // Constructor and Deconstuctor
    ReplacementTrackStatistics()
    {
        Reset();
    };

    ~ReplacementTrackStatistics() {};

    // Reset clears all fields
    void Reset()
    {
        mPreCleaningAge       = 0;
        mOldestAgeEstimate            = 0;
        mReadHits             = 0;
        mPagesReplaced             = 0;
        mWriteHits            = 0;
        mNonDirtyWrites       = 0;
        mRecentAverageAcrossDeviceWriteTime         = 0;
        mRecentAverageAcrossDeviceFlushTime         = 0;
        mPreviouslyDirty      = 0;
    };

    // Operator = will copy one stats object to another.
    ReplacementTrackStatistics & operator = (const ReplacementTrackStatistics & right)
    {
        DBG_ASSERT(this != &right);
        mPreCleaningAge       = right.mPreCleaningAge;
        mOldestAgeEstimate    = right.mOldestAgeEstimate;
        mReadHits             = right.mReadHits;
        mPagesReplaced        = right.mPagesReplaced;
        mWriteHits            = right.mWriteHits;
        mNonDirtyWrites       = right.mNonDirtyWrites;
        mRecentAverageAcrossDeviceWriteTime         = right.mRecentAverageAcrossDeviceWriteTime;
        mRecentAverageAcrossDeviceFlushTime         = right.mRecentAverageAcrossDeviceFlushTime;
        mPreviouslyDirty      = right.mPreviouslyDirty;
        return *this;
    };
private:
    ReplacementTrackStatistics & operator = (const ReplacementTrackStatistics * right);

};

// The maximum replacement track a pool can have.
// ENHANCE: 3 is an expedient that is related to existing SP Cache installs that 
// were created using the old BVD default of 200.
const ULONG MaxReplacementTrackPerPool = 3;

// Statistics about the Replacement Pools.
class ReplacementPoolStatistics
{

public:
    // Age of the oldest reference in the pool in microseconds.
    ULONGLONG mOldestReferenceAge;

    // Number of tracks
    ULONGLONG mNumTracks;

public:

    ReplacementPoolStatistics()
    {
        Reset();
    };

    ~ReplacementPoolStatistics() {};

    void Reset()
    {
        mOldestReferenceAge     = 0;
        mNumTracks              = 0;
    };

    // Operator = will copy one stats object to another.
    ReplacementPoolStatistics & operator = (const ReplacementPoolStatistics * right)
    {
        FF_ASSERT(this != right);
        mOldestReferenceAge     = right->mOldestReferenceAge;
        mNumTracks              = right->mNumTracks;
        return *this;
    };
};

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - kernel/user packing mismatches */

// Parameters related to replacement pool.
struct ReplacementPoolParameters
{
    ReplacementPoolParameters() : mVersion(0), mNumberOfTracks(1), mRebalanceDisparity(2), mIdleThreshold(2), 
        mRecentWriteThrottleLookbackTime(DEFAULT_RECENTWRITETHROTTLELOOKBACKTIME) {}

    // Determine a parameter change is legal.  *this has the current
    // parameters
    bool Validate() const {
        if( mNumberOfTracks > MaxReplacementTrackPerPool || mNumberOfTracks == 0) {
            return false;
        }

        for (ULONG i=0; i < mNumberOfTracks; i++) {
            if (!mTrackParam[i].Validate()) {
                return false;
            }
        }

        return true;
    }

    // Updating this structure with mVersion != CurrentVersion will cause the
    // parameters to revert to the defaults whenever an SP boots.
    // Changing CurrentVersion will cause all systems to revert to the new default parameters.
    enum { UseDefaults=0, CurrentVersion = 4 };

    // The version to store in this structure.  If differs from CurrentVersion, the parameters
    // will be forced back to their defaults.
    UINT_16E  mVersion;

    // Number of tracks present in the pool
    UINT_16E  mNumberOfTracks;

    // Idle threshold for the pool.
    ULONG    mIdleThreshold;

    // Number of seconds difference between the age of oldest reference on each SP
    // before which rebalance will take place.
    ULONG mRebalanceDisparity;

    UINT_32 mRecentWriteThrottleLookbackTime;
    
    // Replacement Track parameters
    ReplacementTrackParameters mTrackParam[MaxReplacementTrackPerPool];
};

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - kernel/user packing mismatches */

#endif  // __ReplacementParams_h__
