#ifndef __CacheDriverStatistics_h__
#define __CacheDriverStatistics_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010 - 2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheDriverStatistics.h
//
// Contents:
//      Defines Cache Driver Statistics
//
// Revision History:
//--

#include "BasicVolumeDriver/BasicVolumeParams.h"
#include "Cache/ReplacementParams.h"
#include "processorCount.h"

// The statistics below of type STATCTR happen to be 32 bit on Rockies while
// KH only wants to support 64 bit counters.   Therefore we have added this compile
// time #ifdef which will switch the counter types on KH and Rockies.
//
//
#ifdef C4_INTEGRATED
typedef UINT_64 STATCTR;
#define formatType "llu"
#else
typedef UINT_32 STATCTR;
#define formatType "u"
#endif // C4_INTEGRATED

typedef struct _CacheDriverStatisticsStruct {
    
    UINT_32           mPerformanceTimestamp;
    STATCTR           mBlocksFlushed;
    STATCTR           mFlushCount;
    STATCTR           mNumberOfCPUs;
    ULONGLONG         mNumberOfCleanPages;
    ULONGLONG         mNumberOfDirtyPages;
    ULONGLONG         mNumberOfFreePages;
    ULONGLONG         mMaxNumberOfPages;
    ULONGLONG         mNumberOfPages;
    ULONGLONG         mMBOfDirtyData;
    STATCTR           mRdHits;
    STATCTR           mRdMiss;
    STATCTR           mWrHits;
    STATCTR           mWrMiss;
    STATCTR           mDmHits;
    STATCTR           mDmMiss;
    STATCTR           mRequestCountWaits;
    STATCTR           mReplacementTrack2Used;
    STATCTR           mReplacementTrack1Used;
    STATCTR           mReplacementTrack0Used;
    ULONGLONG         mNumForcedFlushes;
    ULONGLONG         mPeerHintsUsed;
    ULONGLONG         mPeerHintsReclaimed;
    ULONGLONG         mNumberOfPeerHints;
    
    ULONGLONG         mDwRequests;
    ULONGLONG         mDwMjBlocks;
    ULONGLONG         mDwDcaBlocks;
    ULONGLONG         mDwZeroBlocks;
    ULONGLONG         mDwDiscardBlocks;
    ULONGLONG         mDwCopyBlocks;
    ULONGLONG         mDwRenameBlocks;    
    ULONGLONG         mCumDwXferTime;
    ULONGLONG         mCumDwAccessTime;
    ULONGLONG         mCumDwCommitTime;
    ULONGLONG         mCumDwRequestTime;

    ULONGLONG         mNumPageLayoutTo512;
    ULONGLONG         mNumPageLayoutTo520;
} CacheDriverStatisticsStruct, *pCacheDriverStatisticsStruct;

// The replacementtrack structure used with OBS framework.
//
typedef struct _CacheReplacementTrackStatsStruct {

     STATCTR         mTrackNumber;        // Replacement Track number
     STATCTR         mCleanQlength;
     STATCTR         mDirtyQlength;
     STATCTR         mCleanedQlength;
     STATCTR         mOldestAgeEstimate;
     STATCTR         mPreCleaningAge;
     STATCTR         mReadHits;
     STATCTR         mPagesReplaced;
     STATCTR         mWriteHits;
     STATCTR         mNonDirtyWrites;
     STATCTR         mRecentAverageAcrossDeviceWriteTime;
     STATCTR         mPreviouslyDirty;

     void Reset()
     {
        mTrackNumber     = 0;
        mCleanQlength    = 0;
        mDirtyQlength    = 0;
        mCleanedQlength  = 0;
        mOldestAgeEstimate = 0;
        mPreCleaningAge  = 0;
        mReadHits = 0;
        mPagesReplaced = 0;
        mWriteHits = 0;
        mNonDirtyWrites = 0;
        mRecentAverageAcrossDeviceWriteTime = 0;
        mPreviouslyDirty = 0;
     }

} CacheReplacementTrackStatsStruct, *pCacheReplacementTrackStatsStruct;

// Statistics about the cache driver.
class CacheDriverStatistics  : public BasicVolumeDriverStatistics
{

public:

    CacheDriverStatistics()  { 
        Reset();
    }

    ~CacheDriverStatistics() {};

    void Reset()
    { 
        mStatsBlock.mPerformanceTimestamp       = 0;
        mStatsBlock.mBlocksFlushed              = 0;
        mStatsBlock.mFlushCount                 = 0;
        mStatsBlock.mNumberOfCPUs               = 0;
        mStatsBlock.mNumberOfCleanPages         = 0;
        mStatsBlock.mNumberOfDirtyPages         = 0;
        mStatsBlock.mNumberOfFreePages          = 0;
        mStatsBlock.mMaxNumberOfPages           = 0;
        mStatsBlock.mNumberOfPages              = 0;
        mStatsBlock.mMBOfDirtyData              = 0;
        mStatsBlock.mRdHits                     = 0;
        mStatsBlock.mRdMiss                     = 0;
        mStatsBlock.mWrHits                     = 0;
        mStatsBlock.mWrMiss                     = 0;
        mStatsBlock.mDmMiss                     = 0;
        mStatsBlock.mDmHits                     = 0;
        mStatsBlock.mRequestCountWaits          = 0;
        mStatsBlock.mNumForcedFlushes           = 0;
        mStatsBlock.mReplacementTrack2Used      = 0;
        mStatsBlock.mReplacementTrack1Used      = 0;
        mStatsBlock.mReplacementTrack0Used      = 0;
        mStatsBlock.mPeerHintsUsed              = 0;
        mStatsBlock.mPeerHintsReclaimed         = 0;
        mStatsBlock.mNumberOfPeerHints          = 0;

        mStatsBlock.mDwRequests                 = 0;
        mStatsBlock.mDwMjBlocks                 = 0;
        mStatsBlock.mDwDcaBlocks                = 0;
        mStatsBlock.mDwZeroBlocks               = 0;
        mStatsBlock.mDwDiscardBlocks            = 0;
        mStatsBlock.mDwCopyBlocks               = 0;
        mStatsBlock.mDwRenameBlocks             = 0;
        mStatsBlock.mCumDwXferTime              = 0;
        mStatsBlock.mCumDwAccessTime            = 0;
        mStatsBlock.mCumDwCommitTime            = 0;
        mStatsBlock.mCumDwRequestTime           = 0;

        mStatsBlock.mNumPageLayoutTo512         = 0;
        mStatsBlock.mNumPageLayoutTo520         = 0;
    }

    // ClearStats() doesn't change Pages counts.
    void ClearStats()
    { 
        mStatsBlock.mPerformanceTimestamp       = 0;
        mStatsBlock.mBlocksFlushed              = 0;
        mStatsBlock.mFlushCount                 = 0;
        mStatsBlock.mRdHits                     = 0;
        mStatsBlock.mRdMiss                     = 0;
        mStatsBlock.mWrHits                     = 0;
        mStatsBlock.mWrMiss                     = 0;
        mStatsBlock.mDmMiss                     = 0;
        mStatsBlock.mDmHits                     = 0;
        mStatsBlock.mRequestCountWaits          = 0;
        mStatsBlock.mNumForcedFlushes           = 0;
        mStatsBlock.mReplacementTrack2Used      = 0;
        mStatsBlock.mReplacementTrack1Used      = 0;
        mStatsBlock.mReplacementTrack0Used      = 0;
        mStatsBlock.mPeerHintsUsed              = 0;
        mStatsBlock.mPeerHintsReclaimed         = 0;
        mStatsBlock.mNumberOfPeerHints          = 0;
        
        mStatsBlock.mDwRequests                 = 0;
        mStatsBlock.mDwMjBlocks                 = 0;
        mStatsBlock.mDwDcaBlocks                = 0;
        mStatsBlock.mDwZeroBlocks               = 0;
        mStatsBlock.mDwDiscardBlocks            = 0;
        mStatsBlock.mDwCopyBlocks               = 0;
        mStatsBlock.mDwRenameBlocks             = 0;        
        mStatsBlock.mCumDwXferTime              = 0;
        mStatsBlock.mCumDwAccessTime            = 0;
        mStatsBlock.mCumDwCommitTime            = 0;
        mStatsBlock.mCumDwRequestTime           = 0;

        mStatsBlock.mNumPageLayoutTo512         = 0;
        mStatsBlock.mNumPageLayoutTo520         = 0;
    }

    // Operator += will sum 2 CacheDriverStatistics objects together into one object.
    CacheDriverStatistics & operator += (const CacheDriverStatistics & right)
    {
        mStatsBlock.mPerformanceTimestamp       = 0;
        mStatsBlock.mBlocksFlushed              += right.mStatsBlock.mBlocksFlushed;
        mStatsBlock.mFlushCount                 += right.mStatsBlock.mFlushCount;
        mStatsBlock.mNumberOfCleanPages         += right.mStatsBlock.mNumberOfCleanPages;
        mStatsBlock.mNumberOfDirtyPages         += right.mStatsBlock.mNumberOfDirtyPages;
        mStatsBlock.mNumberOfFreePages          += right.mStatsBlock.mNumberOfFreePages;
        mStatsBlock.mMaxNumberOfPages           += right.mStatsBlock.mMaxNumberOfPages;
        mStatsBlock.mNumberOfPages              += right.mStatsBlock.mNumberOfPages;
        mStatsBlock.mMBOfDirtyData              += right.mStatsBlock.mMBOfDirtyData;
        mStatsBlock.mRequestCountWaits          += right.mStatsBlock.mRequestCountWaits;
        mStatsBlock.mRdHits                     += right.mStatsBlock.mRdHits;
        mStatsBlock.mRdMiss                     += right.mStatsBlock.mRdMiss;
        mStatsBlock.mWrHits                     += right.mStatsBlock.mWrHits;
        mStatsBlock.mWrMiss                     += right.mStatsBlock.mWrMiss;
        mStatsBlock.mDmMiss                     += right.mStatsBlock.mDmMiss;
        mStatsBlock.mDmHits                     += right.mStatsBlock.mDmHits;
        mStatsBlock.mNumForcedFlushes           += right.mStatsBlock.mNumForcedFlushes;
        mStatsBlock.mReplacementTrack2Used      += right.mStatsBlock.mReplacementTrack2Used;
        mStatsBlock.mReplacementTrack1Used      += right.mStatsBlock.mReplacementTrack1Used;
        mStatsBlock.mReplacementTrack0Used      += right.mStatsBlock.mReplacementTrack0Used;
        mStatsBlock.mPeerHintsUsed              += right.mStatsBlock.mPeerHintsUsed;
        mStatsBlock.mPeerHintsReclaimed         += right.mStatsBlock.mPeerHintsReclaimed;
        mStatsBlock.mNumberOfPeerHints          += right.mStatsBlock.mNumberOfPeerHints;
        
        mStatsBlock.mDwRequests                 += right.mStatsBlock.mDwRequests;
        mStatsBlock.mDwMjBlocks                 += right.mStatsBlock.mDwMjBlocks;
        mStatsBlock.mDwDcaBlocks                += right.mStatsBlock.mDwDcaBlocks;
        mStatsBlock.mDwZeroBlocks               += right.mStatsBlock.mDwZeroBlocks;
        mStatsBlock.mDwDiscardBlocks            += right.mStatsBlock.mDwDiscardBlocks;
        mStatsBlock.mDwCopyBlocks               += right.mStatsBlock.mDwCopyBlocks;
        mStatsBlock.mDwRenameBlocks             += right.mStatsBlock.mDwRenameBlocks;
        mStatsBlock.mCumDwXferTime              += right.mStatsBlock.mCumDwXferTime;
        mStatsBlock.mCumDwAccessTime            += right.mStatsBlock.mCumDwAccessTime;
        mStatsBlock.mCumDwCommitTime            += right.mStatsBlock.mCumDwCommitTime;
        mStatsBlock.mCumDwRequestTime           += right.mStatsBlock.mCumDwRequestTime;

        mStatsBlock.mNumPageLayoutTo512         += right.mStatsBlock.mNumPageLayoutTo512;
        mStatsBlock.mNumPageLayoutTo520         += right.mStatsBlock.mNumPageLayoutTo520;
        return *this;
    };

    // Operator = will copy one stats object to another.
    CacheDriverStatistics & operator = (const CacheDriverStatistics & right)
    {
        FF_ASSERT(this != &right);
        mStatsBlock.mPerformanceTimestamp       = right.mStatsBlock.mPerformanceTimestamp;
        mStatsBlock.mBlocksFlushed              = right.mStatsBlock.mBlocksFlushed;
        mStatsBlock.mFlushCount                 = right.mStatsBlock.mFlushCount;
        mStatsBlock.mNumberOfCPUs               = right.mStatsBlock.mNumberOfCPUs;
        mStatsBlock.mNumberOfCleanPages         = right.mStatsBlock.mNumberOfCleanPages;
        mStatsBlock.mNumberOfDirtyPages         = right.mStatsBlock.mNumberOfDirtyPages;
        mStatsBlock.mNumberOfFreePages          = right.mStatsBlock.mNumberOfFreePages;
        mStatsBlock.mMaxNumberOfPages           = right.mStatsBlock.mMaxNumberOfPages;
        mStatsBlock.mNumberOfPages              = right.mStatsBlock.mNumberOfPages;
        mStatsBlock.mMBOfDirtyData              = right.mStatsBlock.mMBOfDirtyData;
        mStatsBlock.mRequestCountWaits          = right.mStatsBlock.mRequestCountWaits;
        mStatsBlock.mRdHits                     = right.mStatsBlock.mRdHits;
        mStatsBlock.mRdMiss                     = right.mStatsBlock.mRdMiss;
        mStatsBlock.mWrHits                     = right.mStatsBlock.mWrHits;
        mStatsBlock.mWrMiss                     = right.mStatsBlock.mWrMiss;
        mStatsBlock.mDmMiss                     = right.mStatsBlock.mDmMiss;
        mStatsBlock.mDmHits                     = right.mStatsBlock.mDmHits;
        mStatsBlock.mNumForcedFlushes           = right.mStatsBlock.mNumForcedFlushes;
        mStatsBlock.mReplacementTrack2Used      = right.mStatsBlock.mReplacementTrack2Used;
        mStatsBlock.mReplacementTrack1Used      = right.mStatsBlock.mReplacementTrack1Used;
        mStatsBlock.mReplacementTrack0Used      = right.mStatsBlock.mReplacementTrack0Used;
        mStatsBlock.mPeerHintsUsed              = right.mStatsBlock.mPeerHintsUsed;
        mStatsBlock.mPeerHintsReclaimed         = right.mStatsBlock.mPeerHintsReclaimed;
        mStatsBlock.mNumberOfPeerHints          = right.mStatsBlock.mNumberOfPeerHints;
        
        mStatsBlock.mDwRequests                 = right.mStatsBlock.mDwRequests;
        mStatsBlock.mDwMjBlocks                 = right.mStatsBlock.mDwMjBlocks;
        mStatsBlock.mDwDcaBlocks                = right.mStatsBlock.mDwDcaBlocks;
        mStatsBlock.mDwZeroBlocks               = right.mStatsBlock.mDwZeroBlocks;
        mStatsBlock.mDwDiscardBlocks            = right.mStatsBlock.mDwDiscardBlocks;
        mStatsBlock.mDwCopyBlocks               = right.mStatsBlock.mDwCopyBlocks;
        mStatsBlock.mDwRenameBlocks             = right.mStatsBlock.mDwRenameBlocks;
        mStatsBlock.mCumDwXferTime              = right.mStatsBlock.mCumDwXferTime;
        mStatsBlock.mCumDwAccessTime            = right.mStatsBlock.mCumDwAccessTime;
        mStatsBlock.mCumDwCommitTime            = right.mStatsBlock.mCumDwCommitTime;
        mStatsBlock.mCumDwRequestTime           = right.mStatsBlock.mCumDwRequestTime;

        mStatsBlock.mNumPageLayoutTo512         = right.mStatsBlock.mNumPageLayoutTo512;
        mStatsBlock.mNumPageLayoutTo520         = right.mStatsBlock.mNumPageLayoutTo520;
        return *this;
    };


    // Change (subtract) previous statistics
    CacheDriverStatistics & operator -= (const CacheDriverStatistics & right)
    {
        mStatsBlock.mPerformanceTimestamp = 0;
        mStatsBlock.mBlocksFlushed              -= right.mStatsBlock.mBlocksFlushed;
        mStatsBlock.mFlushCount                 -= right.mStatsBlock.mFlushCount;
        mStatsBlock.mRequestCountWaits          -= right.mStatsBlock.mRequestCountWaits;
        mStatsBlock.mRdHits                     -= right.mStatsBlock.mRdHits;
        mStatsBlock.mRdMiss                     -= right.mStatsBlock.mRdMiss;
        mStatsBlock.mWrHits                     -= right.mStatsBlock.mWrHits;
        mStatsBlock.mWrMiss                     -= right.mStatsBlock.mWrMiss;
        mStatsBlock.mDmMiss                     -= right.mStatsBlock.mDmMiss;
        mStatsBlock.mDmHits                     -= right.mStatsBlock.mDmHits;
        mStatsBlock.mNumForcedFlushes           -= right.mStatsBlock.mNumForcedFlushes;
        mStatsBlock.mReplacementTrack2Used      -= right.mStatsBlock.mReplacementTrack2Used;
        mStatsBlock.mReplacementTrack1Used      -= right.mStatsBlock.mReplacementTrack1Used;
        mStatsBlock.mReplacementTrack0Used      -= right.mStatsBlock.mReplacementTrack0Used;
        mStatsBlock.mPeerHintsUsed              -= right.mStatsBlock.mPeerHintsUsed;
        mStatsBlock.mPeerHintsReclaimed         -= right.mStatsBlock.mPeerHintsReclaimed;
        mStatsBlock.mNumberOfPeerHints          -= right.mStatsBlock.mNumberOfPeerHints;
        
        mStatsBlock.mDwRequests                 -= right.mStatsBlock.mDwRequests;
        mStatsBlock.mDwMjBlocks                 -= right.mStatsBlock.mDwMjBlocks;
        mStatsBlock.mDwDcaBlocks                -= right.mStatsBlock.mDwDcaBlocks;
        mStatsBlock.mDwZeroBlocks               -= right.mStatsBlock.mDwZeroBlocks;
        mStatsBlock.mDwDiscardBlocks            -= right.mStatsBlock.mDwDiscardBlocks;
        mStatsBlock.mDwCopyBlocks               -= right.mStatsBlock.mDwCopyBlocks;
        mStatsBlock.mDwRenameBlocks             -= right.mStatsBlock.mDwRenameBlocks;
        mStatsBlock.mCumDwXferTime              -= right.mStatsBlock.mCumDwXferTime;
        mStatsBlock.mCumDwAccessTime            -= right.mStatsBlock.mCumDwAccessTime;
        mStatsBlock.mCumDwCommitTime            -= right.mStatsBlock.mCumDwCommitTime;
        mStatsBlock.mCumDwRequestTime           -= right.mStatsBlock.mCumDwRequestTime;

        mStatsBlock.mNumPageLayoutTo512         -= right.mStatsBlock.mNumPageLayoutTo512;
        mStatsBlock.mNumPageLayoutTo520         -= right.mStatsBlock.mNumPageLayoutTo520;
        return *this;
    };

    STATCTR GetPerformance_Timestamp()        const { return mStatsBlock.mPerformanceTimestamp; }
    STATCTR GetBlocks_Flushed()               const { return mStatsBlock.mBlocksFlushed; }
    STATCTR GetFlush_Count()                  const { return mStatsBlock.mFlushCount; }
    ULONGLONG GetNumber_Clean_Pages()         const { return mStatsBlock.mNumberOfCleanPages; }
    ULONGLONG GetNumber_Dirty_Pages()         const { return mStatsBlock.mNumberOfDirtyPages; }
    ULONGLONG GetNumber_Free_Pages()          const { return mStatsBlock.mNumberOfFreePages; }
    ULONGLONG GetNumber_Total_Pages()         const { return mStatsBlock.mMaxNumberOfPages; }
    ULONGLONG GetNumber_Current_Pages()       const { return mStatsBlock.mNumberOfPages; }
    ULONGLONG GetNumber_MB_Dirty()            const { return mStatsBlock.mMBOfDirtyData; }

    STATCTR GetNumberOfCpus()                 const { return mStatsBlock.mNumberOfCPUs; }
    STATCTR GetRdHits()                       const { return mStatsBlock.mRdHits; }
    STATCTR GetRdMiss()                       const { return mStatsBlock.mRdMiss; }
    STATCTR GetWrHits()                       const { return mStatsBlock.mWrHits; }
    STATCTR GetWrMiss()                       const { return mStatsBlock.mWrMiss; }
    STATCTR GetDmHits()                       const { return mStatsBlock.mDmHits; }
    STATCTR GetDmMiss()                       const { return mStatsBlock.mDmMiss; }
    STATCTR GetRequest_Count_Waits()          const { return mStatsBlock.mRequestCountWaits; }
    ULONGLONG GetNumForcedFlushes()           const { return mStatsBlock.mNumForcedFlushes; }
    STATCTR GetReplacementTrack2Used()        const { return mStatsBlock.mReplacementTrack2Used; }
    STATCTR GetReplacementTrack1Used()        const { return mStatsBlock.mReplacementTrack1Used; }
    STATCTR GetReplacementTrack0Used()        const { return mStatsBlock.mReplacementTrack0Used; }
    ULONGLONG GetPeerHintsUsed()              const { return mStatsBlock.mPeerHintsUsed; }
    ULONGLONG GetPeerHintsReclaimed()         const { return mStatsBlock.mPeerHintsReclaimed; }
    ULONGLONG GetPeerHints()                  const { return mStatsBlock.mNumberOfPeerHints; }
    ULONGLONG GetNumPageLayoutTo512()         const { return mStatsBlock.mNumPageLayoutTo512; }
    ULONGLONG GetNumPageLayoutTo520()         const { return mStatsBlock.mNumPageLayoutTo520; }

    VOID IncRdHitsLockHeld()                        { mStatsBlock.mRdHits++; }
    VOID IncRdMissLockHeld()                        { mStatsBlock.mRdMiss++; }
    VOID IncWrHitsLockHeld()                        { mStatsBlock.mWrHits++; }
    VOID IncWrMissLockHeld()                        { mStatsBlock.mWrMiss++; }
    VOID IncDataMoveMissLockHeld()                  { mStatsBlock.mDmMiss++; }
    VOID IncDataMoveHitsLockHeld()                  { mStatsBlock.mDmHits++; }
    VOID IncFlushesLockHeld()                       { mStatsBlock.mFlushCount++; }
    VOID IncRequestCountWaitsLockHeld()             { mStatsBlock.mRequestCountWaits++; }
    VOID IncForcedFlushesLockHeld()                 { mStatsBlock.mNumForcedFlushes++; }
    VOID IncReplacementTrack2UsedLockHeld()         { mStatsBlock.mReplacementTrack2Used++; }
    VOID IncReplacementTrack1UsedLockHeld()         { mStatsBlock.mReplacementTrack1Used++; }
    VOID IncReplacementTrack0UsedLockHeld()         { mStatsBlock.mReplacementTrack0Used++; }
    VOID IncPeerHintsUsedLockHeld()                 { mStatsBlock.mPeerHintsUsed++; }
    VOID IncPeerHintsReclaimedLockHeld()            { mStatsBlock.mPeerHintsReclaimed++; }
    VOID IncNumPageLayoutTo512LockHeld()            { mStatsBlock.mNumPageLayoutTo512++; }
    VOID IncNumPageLayoutTo520LockHeld()            { mStatsBlock.mNumPageLayoutTo520++; }

    VOID AddBlocksFlushedLockHeld(ULONG x)          { mStatsBlock.mBlocksFlushed += x; }

    void SetTimestamp(UINT_32 stamp)                { mStatsBlock.mPerformanceTimestamp = stamp; }
    VOID SetTotalPages(ULONGLONG x)                 { mStatsBlock.mMaxNumberOfPages = x; }
    VOID SetFreePagesAvailableLockHeld(ULONGLONG x) { mStatsBlock.mNumberOfFreePages = x; }
    VOID SetCurrentPages(ULONGLONG x)               { mStatsBlock.mNumberOfPages = x; }
    VOID SetDirtyPages(ULONGLONG x)                 { mStatsBlock.mNumberOfDirtyPages = x; }
    VOID SetCleanPages(ULONGLONG x)                 { mStatsBlock.mNumberOfCleanPages = x; }
    VOID SetMBDirty(ULONGLONG x)                    { mStatsBlock.mMBOfDirtyData = x; }
    void SetNumberOfCpus(UINT_32 cpus)              { mStatsBlock.mNumberOfCPUs = cpus; }
    VOID SetNumberOfPeerHints(ULONGLONG x)          { mStatsBlock.mNumberOfPeerHints = x; }
    
    // DisparateWrite cumulative requests, blocks
    void IncDwRequests()                            { mStatsBlock.mDwRequests++; }
    void AddDwMjBlocks(ULONG x)                     { mStatsBlock.mDwMjBlocks += x; }
    void AddDwDcaBlocks(ULONG x)                    { mStatsBlock.mDwDcaBlocks += x; }
    void AddDwZeroBlocks(ULONG x)                   { mStatsBlock.mDwZeroBlocks += x; }
    void AddDwDiscardBlocks(ULONG x)                { mStatsBlock.mDwDiscardBlocks += x; }
    void AddDwCopyBlocks(ULONG x)                   { mStatsBlock.mDwCopyBlocks += x; }
    void AddDwRenameBlocks(ULONG x)                 { mStatsBlock.mDwRenameBlocks += x; }
    
    ULONGLONG GetDwRequests()                const  { return mStatsBlock.mDwRequests; }
    ULONGLONG GetDwMjBlocks()                const  { return mStatsBlock.mDwMjBlocks; }
    ULONGLONG GetDwDcaBlocks()               const  { return mStatsBlock.mDwDcaBlocks; }
    ULONGLONG GetDwZeroBlocks()              const  { return mStatsBlock.mDwZeroBlocks; }
    ULONGLONG GetDwDiscardBlocks()           const  { return mStatsBlock.mDwDiscardBlocks; }
    ULONGLONG GetDwCopyBlocks()              const  { return mStatsBlock.mDwCopyBlocks; }
    ULONGLONG GetDwRenameBlocks()            const  { return mStatsBlock.mDwRenameBlocks; }
            
    // DisparateWrite cumulative times
    void AddCumDwXferTimeLockHeld(EMCPAL_TIME_USECS response)   { mStatsBlock.mCumDwXferTime += response; }
    void AddCumDwAccessTimeLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumDwAccessTime += response; }
    void AddCumDwCommitTimeLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumDwCommitTime += response; }
    void AddCumDwRequestTimeLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumDwRequestTime += response; }
    ULONGLONG GetCumDwXferTime()  const { return mStatsBlock.mCumDwXferTime;}
    ULONGLONG GetCumDwAccessTime() const { return mStatsBlock.mCumDwAccessTime;}
    ULONGLONG GetCumDwCommitTime() const { return mStatsBlock.mCumDwCommitTime;}
    ULONGLONG GetCumDwRequestTime() const { return mStatsBlock.mCumDwRequestTime;}

    VOID SetNumPageLayoutTo512(ULONGLONG x)         { mStatsBlock.mNumPageLayoutTo512 = x; }
    VOID SetNumPageLayoutTo520(ULONGLONG x)         { mStatsBlock.mNumPageLayoutTo520 = x; }

    void DebugDisplay(ULONG flags, FILE* fileHandle = stdout);
   
    private:
        // These were previously misdeclared.  Bomb the compiler if they are used.
        CacheDriverStatistics & operator += (const CacheDriverStatistics * right);
        CacheDriverStatistics & operator = (const CacheDriverStatistics * right);

        CacheDriverStatisticsStruct mStatsBlock;

};


// The API allows callers to collect the driver summary
// above or the extended statistics below. The extended stats
// return driver stats plus per core stats about it's
// replacement pools and tracks. The Ioctl handler uses the
// the caller's buffer size to determine which format to
// return.
class ReplacementTrackLruStatistics : public ReplacementTrackStatistics
{

public:
    
    // Queue Length for the Lru's clean/cleaned and dirty lists
    ULONGLONG mCleanQlength;
    ULONGLONG mDirtyQlength;
    ULONGLONG mCleanedQlength;

    // Constructor & Destructor
    ReplacementTrackLruStatistics()
    { 
        Reset();
    };

    ~ReplacementTrackLruStatistics() {};

    // Clears the stats object
    void Reset()
    {
        ReplacementTrackStatistics::Reset();

        // Queue Length for the Lru's clean/cleaned and dirty lists
        mCleanQlength      = 0;
        mDirtyQlength      = 0;
        mCleanedQlength    = 0;
    };
};

class ReplacementPoolStatisticsProxy : public ReplacementPoolStatistics
{

public:

    ReplacementTrackLruStatistics mReplacementTrackLruStatistics[MaxReplacementTrackPerPool];

public:

    // Constructor & Destructor
    ReplacementPoolStatisticsProxy()
    { 
        Reset();
    };

    ~ReplacementPoolStatisticsProxy() {};

    // Clears the stats object
    void Reset()
    {
        ReplacementPoolStatistics::Reset();
        for(int i = 0; i < MaxReplacementTrackPerPool; i++)
        {
            mReplacementTrackLruStatistics[i].Reset();
        }
    };
};






class CacheDriverStatisticsExtended : public CacheDriverStatistics
{

public: 

    static const int MaxCPUs = MAX_CORES;

    ReplacementPoolStatisticsProxy mReplacementPoolStatistics[MaxCPUs];

public:

    // Constructor & Destructor
    CacheDriverStatisticsExtended()
    { 
        Reset();
    };

    ~CacheDriverStatisticsExtended() {};

    void Reset()
    {
        CacheDriverStatistics::Reset();
        for(int i = 0; i < MaxCPUs; i++)
        {
            mReplacementPoolStatistics[i].Reset();
        }
    };

    void DebugDisplay(ULONG flags, FILE* fileHandle = stdout);

};



#endif  // __CacheDriverStatistics_h__

