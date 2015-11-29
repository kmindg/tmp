#ifndef __CacheVolumeStatistics_h__
#define __CacheVolumeStatistics_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2002,2009-2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheVolumeStatistics.h
//
// Contents:
//      Defines the ConsumedDevice class. 
//
// Revision History:
//--
# include "BasicVolumeDriver/BasicVolumeParams.h"
# include "EmcPAL_Memory.h"
#include "processorCount.h"

#ifndef ADM_HIST_SIZE
#define ADM_HIST_SIZE 10
#endif
#ifndef ADM_REHIT_HIST_SIZE
#define ADM_REHIT_HIST_SIZE 10
#endif

// The statistics below of type STATCTR happen to be 32 bit on Rockies while
// KH only wants to support 64 bit counters.   Therefore we have added this compile
// time #ifdef which will switch the counter types on KH and Rockies.
//
// Also define formatType to be used for printf output.
#ifdef C4_INTEGRATED
typedef UINT_64 STATCTR;
#define formatType "llu"
#else
typedef UINT_32 STATCTR;
#define formatType "u"
#endif // C4_INTEGRATED

typedef struct _CacheVolumeStatisticsStruct {
    STATCTR mNumRdHits;        /* number of cache read hits per unit */
    STATCTR mNumRdWrHits;      /* number of cache read hits per unit (at least one page dirty) */
    STATCTR mNumRdRdHits;      /* number of cache read hits per unit (all pages clean) */
    STATCTR mNumRdMiss;        /* number of cache read misses per unit */
    STATCTR mNumBlkPref;       /* number of cache blocks prefetched */
    STATCTR mNumPfUnused;      /* number of prefetched blks never used */
    STATCTR mNumReadBlock;     /* number of host blocks read */
    STATCTR mNumPendingRd;     /* number of pending reads */

    STATCTR mNumWriteBlock;    /* number of host blocks written */
    STATCTR mNumWrHits;        /* number of cache write hits per unit (all pages dirty) */
    STATCTR mNumWrMiss;        /* number of cache write misses per unit (clean or miss) */
    STATCTR mNumWrThru;        /* number of cache write thru */
    STATCTR mNumDelayedWr;     /* number of delayed writes (waits for pages)*/
    STATCTR mNumThrottledWr;   /* number of delayed writes (write throttle- RG exceeds quota)*/
    STATCTR mFastWrites;       /* writes that didn't have to wait for pages */
    STATCTR mForcedFlushes;    /* number of forced flush requests */
    STATCTR mFlushes;          /* number of flush requests */
    STATCTR mBlocksFlushed;    /* number of blocks flushed */
    STATCTR mNumBackfillRd;    /* number of read I/O's sent to the back end to backfill */
    STATCTR mBlocksBackfilled; /* number of invalid blocks backfilled (loaded) for flushing */

    STATCTR mNumPgRdHits;      /* number of cache pages hit for read */
    STATCTR mNumPgRdMiss;      /* number of cache pages missed for read */
    STATCTR mNumPgWrHits;      /* number of cache pages hit for write */
    STATCTR mNumPgWrMiss;      /* number of cache pages missed for write */
    
    STATCTR mNumDataMoveHits;           /* number of complete cache hits during read phase */
    STATCTR mNumDataMoveMiss;           /* number of partial or complete cache miss for read phase */
    STATCTR mNumDataMoveBlockCopied;    /* number of blocks copied from source volume */
    STATCTR mNumDataMoveBlockCloned;    /* number of block copied to new page during extract */
    STATCTR mNumDataMoveBlockWritten;   /* number of blocks written to destination volume */
    STATCTR mNumDataMovePgHits;         /* number of cache page hits for read phase */
    STATCTR mNumDataMovePgMiss;         /* number of cache page misses for read phase */
    STATCTR mNumDataMovePending;        /* number of pended data move requests */

    UINT_64 mBlocksZeroed;              /* Number of cache blocks zeroed */
    STATCTR mZeroFillCommands;          /* Number of zero fill commands */

    EMCPAL_TIME_USECS mCumReadResp;  /* cumulative read response time */
    EMCPAL_TIME_USECS mCumWriteResp; /* cumulative write response time */
    EMCPAL_TIME_USECS mCumDataMoveResp;
    EMCPAL_TIME_USECS mCumWriteXferTime;  // Time from start I/O to transfer done
    EMCPAL_TIME_USECS mCumWriteCommitTime; // Time from statt I/O until commit done

    UINT_64 mSumQLengthOnArrival;
    STATCTR mArrivalsToNonZeroQ;

    UINT_64 mNumReadAndPin;  /* Number or Read and Pin Commands */
    UINT_64 mNumBlocksPinned; /* Number of blocks pinned */
    UINT_64 mNumReadAndPinHits; /* Number of Read and Pin Hits */
    UINT_64 mNumReadAndPinMiss; /* Number of Read and Pin Misses */
    UINT_64 mNumReadAndPinPgHits; /* Number of Read and Pin Page Hits */
    UINT_64 mNumReadAndPinPgMiss; /* Number of Read and Pin Page Misses */
    UINT_32 mCurrentVGCredits;    /* The current number of VG credits */
    UINT_32 mWriteThrottled;      /* Is the unit write throttled */

    UINT_64 mNumRefsWarmedWithHint; /* Number of references warmed with hint */
    UINT_64 mNumWrUsedZFVirtualRefs; /* Number of writes used virtual references to update ZF bitmap */
    UINT_64 mNumWrUsedNonZFVirtualRefs; /* Number of writes used virtual references for user data */
    UINT_64 mNumVirtualRefsInZFWr; /* Number of virtual references used to update ZF bitmap */
    UINT_64 mNumVirtualRefsInNonZFWr; /* Number of virtual references used to update non ZF bitmap */
    UINT_64 mNumWrNotUsedVirtualRefDueToSizeLimit; /* Number of write IOs which couldn't use virtual reference due to big size of IO*/
    UINT_64 mNumRdUsedVirtualRef; /* Number of reads used virtual references for transfering user data*/
    UINT_64 mNumVirtualRefInRd; /* Number of virtual references used in read*/
    UINT_64 mNumCacheMissWithVirtualRef; /* Number of cache misses with holding virtual references*/
  
    int     mQLength;                   /* Proxy QLength for Volume */

    UINT_64 mNumReadHitOnly;  /* Number of Read Hit Only Commands */
    UINT_64 mNumReadHitOnlySuccess;  /* Number of Read Hit Only Success */
    UINT_64 mNumReadHitOnlyMiss;  /* Number of Read Hit Only Misses (return TOO_LATE) */
    UINT_64 mNumReadHitOnlyAlerted;  /* Number of Read Hit Only Alerted (not dirty in cache) */
    UINT_64 mNumReadHitOnlyDiscard;  /* Number of Read Hit Only Discard on DCA_READ completion */

    STATCTR mReadHistogram[ADM_HIST_SIZE+1];
    STATCTR mWriteHistogram[ADM_HIST_SIZE+1];

    /* RehitHistogram: bucket how many times read/write page was hit: Placeholder: not implemented yet */
    UINT_64 mRdRehitHist[ADM_HIST_SIZE+1];
    UINT_64 mWrRehitHist[ADM_HIST_SIZE+1];

} CacheVolumeStatisticsStruct, *pCacheVolumeStatisticsStruct;

// Statistics for Volume Hiistogram to be present it to OBS framework
typedef struct _CacheVolumeHistogramStruct {
  
   STATCTR   mReadHistogram;
   STATCTR   mWriteHistogram;
   STATCTR   mRdRehitHist;
   STATCTR   mWrRehitHist;

} CacheVolumeHistogramStruct, *pCacheVolumeHistogramStruct;

// Expose some per-proxy counters
typedef struct CacheVolumeProxy_s {
    UINT_64 TotalRequestCount;
    UINT_64 SumQLengthOnArrival;
    UINT_64 AverageFlushTime;   /* Average write time per core in microseconds */
    STATCTR ArrivalsToNonZeroQ;
} CacheVolumeProxy;

// Statistics about CacheVolume per proxy that can be summed to get
// statistics for the volume.
class CacheVolumeProxyStatistics {
public:
    friend class CacheVolumeStatistics;

    CacheVolumeProxyStatistics() {
        ReInitialize();
    }
    void ReInitialize()  {
        mStatsBlock.mQLength = 0;
        Reset();
    }
    ~CacheVolumeProxyStatistics() {};

    void Reset() {
        mStatsBlock.mNumRdHits = 0;
        mStatsBlock.mNumRdWrHits = 0;
        mStatsBlock.mNumRdRdHits = 0;
        mStatsBlock.mNumRdMiss = 0;
        mStatsBlock.mNumBlkPref = 0;
        mStatsBlock.mNumPfUnused = 0;
        mStatsBlock.mNumReadBlock = 0;
        mStatsBlock.mNumPendingRd = 0;

        mStatsBlock.mNumWriteBlock = 0;
        mStatsBlock.mNumWrHits = 0;
        mStatsBlock.mNumWrMiss = 0;
        mStatsBlock.mNumWrThru = 0;
        mStatsBlock.mNumDelayedWr = 0;
        mStatsBlock.mNumThrottledWr = 0;
        mStatsBlock.mFastWrites = 0;
        mStatsBlock.mForcedFlushes = 0;
        mStatsBlock.mFlushes = 0;
        mStatsBlock.mBlocksFlushed = 0;
        mStatsBlock.mNumBackfillRd = 0;
        mStatsBlock.mBlocksBackfilled = 0;

        mStatsBlock.mNumPgRdHits = 0;
        mStatsBlock.mNumPgRdMiss = 0;
        mStatsBlock.mNumPgWrHits = 0;
        mStatsBlock.mNumPgWrMiss = 0;

        mStatsBlock.mNumDataMoveHits = 0;
        mStatsBlock.mNumDataMoveMiss = 0;
        mStatsBlock.mNumDataMoveBlockCopied = 0;
        mStatsBlock.mNumDataMoveBlockCloned = 0;
        mStatsBlock.mNumDataMoveBlockWritten = 0;
        mStatsBlock.mNumDataMovePgHits = 0;
        mStatsBlock.mNumDataMovePgMiss = 0;
        mStatsBlock.mNumDataMovePending = 0;

        mStatsBlock.mBlocksZeroed = 0;
        mStatsBlock.mZeroFillCommands = 0;

        mStatsBlock.mCumReadResp = 0;
        mStatsBlock.mCumWriteResp = 0;
        mStatsBlock.mCumDataMoveResp = 0;
        mStatsBlock.mCumWriteXferTime= 0;
        mStatsBlock.mCumWriteCommitTime = 0;

        mStatsBlock.mSumQLengthOnArrival = 0;
        mStatsBlock.mArrivalsToNonZeroQ  = 0;
        // mQLength is only zeroed in original constructor

        for(int i = 0; i < ADM_HIST_SIZE+1; i++) {
            mStatsBlock.mReadHistogram[i] = 0;
            mStatsBlock.mWriteHistogram[i] = 0;
            mStatsBlock.mRdRehitHist[i] = 0;
            mStatsBlock.mWrRehitHist[i] = 0;
        }

        mStatsBlock.mNumReadAndPin = 0;
        mStatsBlock.mNumBlocksPinned = 0;
        mStatsBlock.mNumReadAndPinHits = 0;
        mStatsBlock.mNumReadAndPinMiss = 0;
        mStatsBlock.mNumReadAndPinPgHits = 0;
        mStatsBlock.mNumReadAndPinPgMiss = 0;
        mStatsBlock.mCurrentVGCredits = 0;
        mStatsBlock.mWriteThrottled = 0;

	mStatsBlock.mNumRefsWarmedWithHint = 0;
	mStatsBlock.mNumWrUsedZFVirtualRefs = 0;
	mStatsBlock.mNumWrUsedNonZFVirtualRefs = 0;
	mStatsBlock.mNumVirtualRefsInZFWr = 0;
	mStatsBlock.mNumVirtualRefsInNonZFWr = 0;
	mStatsBlock.mNumWrNotUsedVirtualRefDueToSizeLimit = 0;
	mStatsBlock.mNumRdUsedVirtualRef = 0;
	mStatsBlock.mNumVirtualRefInRd = 0;
	mStatsBlock.mNumCacheMissWithVirtualRef = 0;   

        mStatsBlock.mNumReadHitOnly = 0;
        mStatsBlock.mNumReadHitOnlySuccess = 0;
        mStatsBlock.mNumReadHitOnlyMiss = 0;
        mStatsBlock.mNumReadHitOnlyAlerted = 0;
        mStatsBlock.mNumReadHitOnlyDiscard = 0;

    }

    void IncRdHitsLockHeld()            { mStatsBlock.mNumRdHits++; }
    void IncRdWrHitsLockHeld()          { mStatsBlock.mNumRdWrHits++; }
    void IncRdrdHitsLockHeld()          { mStatsBlock.mNumRdRdHits++; }
    void IncRdMissLockHeld()            { mStatsBlock.mNumRdMiss++; }

    void AddBlkPrefLockHeld(ULONG x)    { mStatsBlock.mNumBlkPref += x; }
    void AddPfUnusedLockHeld(ULONG x)   { mStatsBlock.mNumPfUnused += x; }
    void AddReadBlockLockHeld(ULONG x)  { mStatsBlock.mNumReadBlock += x; }
    void IncPendingRdLockHeld()         { mStatsBlock.mNumPendingRd++; }
    void IncDataMoveMissLockHeld()      { mStatsBlock.mNumDataMoveMiss++; }
    void IncDataMoveHitsLockHeld()      { mStatsBlock.mNumDataMoveHits++; }
    void AddDMBlockCopiedLockHeld(ULONG x)  { mStatsBlock.mNumDataMoveBlockCopied += x; }
    void AddDMBlockClonedLockHeld(ULONG x)  { mStatsBlock.mNumDataMoveBlockCloned += x; }
    void AddDMBlockWrittenLockHeld(ULONG x)  { mStatsBlock.mNumDataMoveBlockWritten += x; }
    void IncPendingDataMoveLockHeld()   { mStatsBlock.mNumDataMovePending++; }

    void AddWriteBlockLockHeld(ULONG x) { mStatsBlock.mNumWriteBlock += x; }
    void IncWrHitsLockHeld()            { mStatsBlock.mNumWrHits++; }
    void IncWrMissLockHeld()            { mStatsBlock.mNumWrMiss++; }
    void IncWrThruLockHeld()            { mStatsBlock.mNumWrThru++; }
    void IncDelayedWrLockHeld()         { mStatsBlock.mNumDelayedWr++; }
    void IncThrottledWrLockHeld()       { mStatsBlock.mNumThrottledWr++; }
    void IncFastWritesLockHeld()        { mStatsBlock.mFastWrites++; }
    void IncForcedFlushesLockHeld()     { mStatsBlock.mForcedFlushes++; }
    void IncFlushesLockHeld()           { mStatsBlock.mFlushes++; }
    void AddBlocksFlushedLockHeld(ULONG x)  { mStatsBlock.mBlocksFlushed += x; }
    void IncBackfillRdLockHeld()     { mStatsBlock.mNumBackfillRd++; }
    void AddBlocksBackfilledLockHeld(ULONG x)  { mStatsBlock.mBlocksBackfilled += x; }

    void AddPgRdHitsLockHeld(ULONG x)   { mStatsBlock.mNumPgRdHits += x; }
    void AddPgRdMissLockHeld(ULONG x)   { mStatsBlock.mNumPgRdMiss += x; }
    void AddPgWrHitsLockHeld(ULONG x)   { mStatsBlock.mNumPgWrHits += x; }
    void AddPgWrMissLockHeld(ULONG x)   { mStatsBlock.mNumPgWrMiss += x; }
    void AddPgDataMoveHitsLockHeld(ULONG x)   { mStatsBlock.mNumDataMovePgHits += x; }
    void AddPgDataMoveMissLockHeld(ULONG x)   { mStatsBlock.mNumDataMovePgMiss += x; }

    void AddCumReadRespLockHeld(EMCPAL_TIME_USECS response)  { mStatsBlock.mCumReadResp += response; }
    void AddCumWriteRespLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumWriteResp += response; }
    void AddCumXferTimeLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumWriteXferTime += response; }
    void AddCumCommitTimeLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumWriteCommitTime += response; }

    void AddCumDataMoveRespLockHeld(EMCPAL_TIME_USECS response) { mStatsBlock.mCumDataMoveResp += response; }
    int  IncQLengthLockHeld();
    int  DecQLengthLockHeld();

    void AddBlocksZeroedLockHeld(UINT_64 x)     { mStatsBlock.mBlocksZeroed += x; }
    void IncZeroFillCommandsLockHeld()          { mStatsBlock.mZeroFillCommands ++; }

    // Compute Histogram index: 0 .. ADM_HIST_SIZE  Note: Arrays[ADM_HIST_SIZE+1] for overflow.
    int HistIndex(UINT_32 blocks) const ;
    void IncReadHistLockHeld(UINT_32 sectors)   { mStatsBlock.mReadHistogram[HistIndex(sectors)]++; }
    void IncWriteHistLockHeld(UINT_32 sectors)  { mStatsBlock.mWriteHistogram[HistIndex(sectors)]++; }
    // Compute RehitHist index: 0 .. ADM_HIST_SIZE: number of hits on page, before repurpose.
    int RehitIndex(UINT_32 hits) const ;
    void IncRdRehitHistLockHeld(UINT_32 hits)  { mStatsBlock.mRdRehitHist[RehitIndex(hits)]++; }
    void IncWrRehitHistLockHeld(UINT_32 hits)  { mStatsBlock.mWrRehitHist[RehitIndex(hits)]++; }

    void IncReadAndPinLockHeld()            { mStatsBlock.mNumReadAndPin++; }
    void AddBlocksPinnedLockHeld(UINT_64 x) { mStatsBlock.mNumBlocksPinned += x; }
    void IncReadAndPinHitsLockHeld()        { mStatsBlock.mNumReadAndPinHits++; }
    void IncReadAndPinMissLockHeld()        { mStatsBlock.mNumReadAndPinMiss++; }
    void AddReadAndPinPgHitsLockHeld(UINT_64 x) { mStatsBlock.mNumReadAndPinPgHits += x; }
    void AddReadAndPinPgMissLockHeld(UINT_64 x) { mStatsBlock.mNumReadAndPinPgMiss += x; }

    void SetCurrentVGCredits(UINT_32 credits) { mStatsBlock.mCurrentVGCredits += credits; }
    void SetWriteThrottled(UINT_32 throttled) { mStatsBlock.mWriteThrottled += throttled; }

    void AddNumRefWarmedWithHintLockHeld(UINT_32 numWarmHint) { mStatsBlock.mNumRefsWarmedWithHint += numWarmHint; }    
    void IncNumWrUsedZFVirtualRefLockHeld()       { mStatsBlock.mNumWrUsedZFVirtualRefs ++; }    
    void IncNumWrUsedNonZFVirtualRefLockHeld()    { mStatsBlock.mNumWrUsedNonZFVirtualRefs ++; }    
    void IncNumVirtualRefsInZFWrLockHeld(UINT_64 addedNum = 1) { mStatsBlock.mNumVirtualRefsInZFWr += addedNum; }
    void IncNumVirtualRefsInNonZFWrLockHeld(UINT_64 addedNum = 1) { mStatsBlock.mNumVirtualRefsInNonZFWr += addedNum; }	
    void IncNumWrNotUsedVirtualRefDueToSizeLimitLockHeld()  { mStatsBlock.mNumWrNotUsedVirtualRefDueToSizeLimit ++; }
    void IncNumRdUsedVirtualRefLockHeld() { mStatsBlock.mNumRdUsedVirtualRef++; }
    void IncNumVirtualRefInRdLockHeld(UINT_64 addedNum = 1) { mStatsBlock.mNumVirtualRefInRd += addedNum; }
    void IncNumCacheMissWithVirtualRefLockHeld() {  mStatsBlock.mNumCacheMissWithVirtualRef++; }

    void IncReadHitOnlyLockHeld()            { mStatsBlock.mNumReadHitOnly++; }
    void IncReadHitOnlySuccessLockHeld()     { mStatsBlock.mNumReadHitOnlySuccess++; }
    void IncReadHitOnlyMissLockHeld()        { mStatsBlock.mNumReadHitOnlyMiss++; }
    void IncReadHitOnlyAlertedLockHeld()     { mStatsBlock.mNumReadHitOnlyAlerted++; }
    void IncReadHitOnlyDiscardLockHeld()     { mStatsBlock.mNumReadHitOnlyDiscard++; }

    STATCTR GetRdHits()         { return mStatsBlock.mNumRdHits; }
    STATCTR GetRdWrHits()       { return mStatsBlock.mNumRdWrHits; }
    STATCTR GetRdRdHits()       { return mStatsBlock.mNumRdRdHits; }
    STATCTR GetRdMiss()         { return mStatsBlock.mNumRdMiss; }
    STATCTR GetBlkPref()        { return mStatsBlock.mNumBlkPref; }
    STATCTR GetPfUnused()       { return mStatsBlock.mNumPfUnused; }
    STATCTR GetReadBlock()      { return mStatsBlock.mNumReadBlock; }
    STATCTR GetPendingRd()      { return mStatsBlock.mNumPendingRd; }

    STATCTR GetDmHits()         { return mStatsBlock.mNumDataMoveHits; }
    STATCTR GetDmMiss()         { return mStatsBlock.mNumDataMoveMiss; }
    STATCTR GetDmBlock()        { return mStatsBlock.mNumDataMoveBlockCopied; }
    STATCTR GetDmBlockCloned()  { return mStatsBlock.mNumDataMoveBlockCloned; }
    STATCTR GetDmBlockWritten()  { return mStatsBlock.mNumDataMoveBlockWritten; }
    STATCTR GetDmPending()      { return mStatsBlock.mNumDataMovePending; }

    STATCTR GetWriteBlock()     { return mStatsBlock.mNumWriteBlock; }
    STATCTR GetWrHits()         { return mStatsBlock.mNumWrHits; }
    STATCTR GetWrMiss()         { return mStatsBlock.mNumWrMiss; }
    STATCTR GetWrThru()         { return mStatsBlock.mNumWrThru; }
    STATCTR GetDelayedWr()      { return mStatsBlock.mNumDelayedWr; }
    STATCTR GetThrottledWr()    { return mStatsBlock.mNumThrottledWr; }
    STATCTR GetFastWrites()     { return mStatsBlock.mFastWrites; }
    STATCTR GetForcedFlushes()  { return mStatsBlock.mForcedFlushes; }
    STATCTR GetFlushes()        { return mStatsBlock.mFlushes; }
    STATCTR GetBlocksFlushed()  { return mStatsBlock.mBlocksFlushed; }
    STATCTR GetBackfillRd()     { return mStatsBlock.mNumBackfillRd; }
    STATCTR GetBlocksBackfilled() { return mStatsBlock.mBlocksBackfilled; }

    STATCTR GetPgRdHits()       { return mStatsBlock.mNumPgRdHits; }
    STATCTR GetPgRdMiss()       { return mStatsBlock.mNumPgRdMiss; }
    STATCTR GetPgWrHits()       { return mStatsBlock.mNumPgWrHits; }
    STATCTR GetPgWrMiss()       { return mStatsBlock.mNumPgWrMiss; }
    STATCTR GetPgDmHits()       { return mStatsBlock.mNumDataMovePgHits; }
    STATCTR GetPgDmMiss()       { return mStatsBlock.mNumDataMovePgMiss; }

    EMCPAL_TIME_USECS GetCumReadResp()  { return mStatsBlock.mCumReadResp; }
    EMCPAL_TIME_USECS GetCumWriteResp() { return mStatsBlock.mCumWriteResp; }
    EMCPAL_TIME_USECS GetCumDataMoveResp() { return mStatsBlock.mCumDataMoveResp; }
    EMCPAL_TIME_USECS GetCumWriteXferTime()  { return mStatsBlock.mCumWriteXferTime; }
    EMCPAL_TIME_USECS GetCumWriteCommitTime()  { return mStatsBlock.mCumWriteCommitTime; }

    UINT_64 GetSumQLengthOnArrival()  { return mStatsBlock.mSumQLengthOnArrival; }
    STATCTR GetArrivalsToNonZeroQ()   { return mStatsBlock.mArrivalsToNonZeroQ; }
    int     GetQLength()              { return mStatsBlock.mQLength; }
    STATCTR GetReadHist(UINT_32 idx)    { return mStatsBlock.mReadHistogram[idx]; }
    STATCTR GetWriteHist(UINT_32 idx)   { return mStatsBlock.mWriteHistogram[idx]; }
    UINT_64 GetRdRehitHist(UINT_32 idx)   { return mStatsBlock.mRdRehitHist[idx]; }
    UINT_64 GetWrRehitHist(UINT_32 idx)   { return mStatsBlock.mWrRehitHist[idx]; }

    UINT_64 GetBlocksZeroed()       { return mStatsBlock.mBlocksZeroed; }
    STATCTR GetZeroFillCommands()   { return mStatsBlock.mZeroFillCommands; }

    UINT_64 GetReadAndPin()   { return mStatsBlock.mNumReadAndPin; }
    UINT_64 GetBlocksPinned() { return mStatsBlock.mNumBlocksPinned; }
    UINT_64 GetReadAndPinHits() { return mStatsBlock.mNumReadAndPinHits; }
    UINT_64 GetReadAndPinMiss() { return mStatsBlock.mNumReadAndPinMiss; }
    UINT_64 GetReadAndPinPgHits() { return mStatsBlock.mNumReadAndPinPgHits; }
    UINT_64 GetReadAndPinPgMiss() { return mStatsBlock.mNumReadAndPinPgMiss; }

    UINT_32 GetCurrentVGCredits() { return mStatsBlock.mCurrentVGCredits; }
    UINT_32 GetWriteThrottled() { return mStatsBlock.mWriteThrottled; }


    UINT_64 GetNumRefWarmedWithHint() { return mStatsBlock.mNumRefsWarmedWithHint; }
    UINT_64 GetNumWrUsedZFVirtualRef() { return mStatsBlock.mNumWrUsedZFVirtualRefs; }
    UINT_64 GetNumWrUsedNonZFVirtualRef() { return mStatsBlock.mNumWrUsedNonZFVirtualRefs; }
    UINT_64 GetNumVirtualRefsInZFWr() { return mStatsBlock.mNumVirtualRefsInZFWr; }
    UINT_64 GetNumVirtualRefsInNonZFWr() { return mStatsBlock.mNumVirtualRefsInNonZFWr; }
    UINT_64 GetNumWrNotUsedVirtualRefDueToSizeLimit() { return mStatsBlock.mNumWrNotUsedVirtualRefDueToSizeLimit; }
    UINT_64 GetNumRdUsedVirtualRef() { return mStatsBlock.mNumRdUsedVirtualRef; }
    UINT_64 GetNumVirtualRefInRd() { return mStatsBlock.mNumVirtualRefInRd; }
    UINT_64 GetNumCacheMissWithVirtualRef() {	return mStatsBlock.mNumCacheMissWithVirtualRef; }

    UINT_64 GetReadHitOnly()        { return mStatsBlock.mNumReadHitOnly; }
    UINT_64 GetReadHitOnlySuccess() { return mStatsBlock.mNumReadHitOnlySuccess; }
    UINT_64 GetReadHitOnlyMiss()    { return mStatsBlock.mNumReadHitOnlyMiss; }
    UINT_64 GetReadHitOnlyAlerted() { return mStatsBlock.mNumReadHitOnlyAlerted; }
    UINT_64 GetReadHitOnlyDiscard() { return mStatsBlock.mNumReadHitOnlyDiscard; }

    void DebugDisplay(ULONG flags, FILE* fileHandle = stdout);

private:
    CacheVolumeStatisticsStruct mStatsBlock;
};

// HistIndex returns index 0 .. ADM_HIST_SIZE-1 for under 1024 blocks, ADM_HIST_SIZE for overflow
inline int CacheVolumeProxyStatistics::HistIndex(UINT_32 blocks) const
{
    if (blocks < (1 << ADM_HIST_SIZE)) {
        for (int i = (ADM_HIST_SIZE - 1); i >= 0; i--) {
            if ((blocks & (0x1 << i)) != 0) {
                return (i);
            }
        }
    }
    return ADM_HIST_SIZE; // overflow histogram.
}

inline int CacheVolumeProxyStatistics::RehitIndex(UINT_32 hits) const
{
    return (hits < ADM_HIST_SIZE) ? hits : ADM_HIST_SIZE;
}

inline int CacheVolumeProxyStatistics::IncQLengthLockHeld()
{
    mStatsBlock.mSumQLengthOnArrival += mStatsBlock.mQLength + 1;
    if (mStatsBlock.mQLength > 0) {
        mStatsBlock.mArrivalsToNonZeroQ++;
    }
    return ++mStatsBlock.mQLength;
}

inline int CacheVolumeProxyStatistics::DecQLengthLockHeld()
{
    return --mStatsBlock.mQLength;
}


// CacheVolumeStatistics to be summed per proxy, accessible via BVD/admin
// Changes revlock with admin (DRAMCacheCom and MMan/MManMP_Redirector/LUNObjects.cpp)

class CacheVolumeStatistics : public BasicVolumeStatistics, public CacheVolumeProxyStatistics {
public:
    static const int StatsMaxCPUs = MAX_CORES;

    CacheVolumeStatistics()  { 
        Reset();
    }
    ~CacheVolumeStatistics() {};

    void Reset() { EmcpalZeroMemory(this, sizeof(*this)); }
    void SetTimestamp(UINT_32 stamp)    { Performance_Timestamp = stamp; }
    UINT_32 GetPerformance_Timestamp() { return Performance_Timestamp; }

    // Operator += will add CacheVolumeProxyStatistics into CacheVolumeStatistics.
    CacheVolumeStatistics & operator += (const CacheVolumeProxyStatistics * right) {
        mStatsBlock.mNumRdHits += right->mStatsBlock.mNumRdHits;
        mStatsBlock.mNumRdWrHits += right->mStatsBlock.mNumRdWrHits;
        mStatsBlock.mNumRdRdHits += right->mStatsBlock.mNumRdRdHits;
        mStatsBlock.mNumRdMiss += right->mStatsBlock.mNumRdMiss;
        mStatsBlock.mNumBlkPref += right->mStatsBlock.mNumBlkPref;
        mStatsBlock.mNumPfUnused += right->mStatsBlock.mNumPfUnused;
        mStatsBlock.mNumReadBlock += right->mStatsBlock.mNumReadBlock;
        mStatsBlock.mNumPendingRd += right->mStatsBlock.mNumPendingRd;

        mStatsBlock.mNumWriteBlock += right->mStatsBlock.mNumWriteBlock;
        mStatsBlock.mNumWrHits += right->mStatsBlock.mNumWrHits;
        mStatsBlock.mNumWrMiss += right->mStatsBlock.mNumWrMiss;
        mStatsBlock.mNumWrThru += right->mStatsBlock.mNumWrThru;
        mStatsBlock.mNumDelayedWr += right->mStatsBlock.mNumDelayedWr;
        mStatsBlock.mNumThrottledWr += right->mStatsBlock.mNumThrottledWr;
        mStatsBlock.mFastWrites += right->mStatsBlock.mFastWrites;
        mStatsBlock.mForcedFlushes += right->mStatsBlock.mForcedFlushes;
        mStatsBlock.mFlushes += right->mStatsBlock.mFlushes;
        mStatsBlock.mBlocksFlushed += right->mStatsBlock.mBlocksFlushed;
        mStatsBlock.mNumBackfillRd += right->mStatsBlock.mNumBackfillRd;
        mStatsBlock.mBlocksBackfilled += right->mStatsBlock.mBlocksBackfilled;

        mStatsBlock.mNumPgRdHits += right->mStatsBlock.mNumPgRdHits;
        mStatsBlock.mNumPgRdMiss += right->mStatsBlock.mNumPgRdMiss;
        mStatsBlock.mNumPgWrHits += right->mStatsBlock.mNumPgWrHits;
        mStatsBlock.mNumPgWrMiss += right->mStatsBlock.mNumPgWrMiss;
        
        mStatsBlock.mNumDataMoveHits += right->mStatsBlock.mNumDataMoveHits;
        mStatsBlock.mNumDataMoveMiss += right->mStatsBlock.mNumDataMoveMiss;
        mStatsBlock.mNumDataMoveBlockCopied += right->mStatsBlock.mNumDataMoveBlockCopied;
        mStatsBlock.mNumDataMoveBlockCloned += right->mStatsBlock.mNumDataMoveBlockCloned;
        mStatsBlock.mNumDataMoveBlockWritten += right->mStatsBlock.mNumDataMoveBlockWritten;
        mStatsBlock.mNumDataMovePgHits += right->mStatsBlock.mNumDataMovePgHits;
        mStatsBlock.mNumDataMovePgMiss += right->mStatsBlock.mNumDataMovePgMiss;
        mStatsBlock.mNumDataMovePending += right->mStatsBlock.mNumDataMovePending;

        mStatsBlock.mBlocksZeroed += right->mStatsBlock.mBlocksZeroed;
        mStatsBlock.mZeroFillCommands += right->mStatsBlock.mZeroFillCommands;

        mStatsBlock.mCumReadResp += right->mStatsBlock.mCumReadResp;
        mStatsBlock.mCumWriteResp += right->mStatsBlock.mCumWriteResp;
        mStatsBlock.mCumWriteXferTime += right->mStatsBlock.mCumWriteXferTime;
        mStatsBlock.mCumWriteCommitTime += right->mStatsBlock.mCumWriteCommitTime;
        mStatsBlock.mCumDataMoveResp += right->mStatsBlock.mCumDataMoveResp;
        mStatsBlock.mSumQLengthOnArrival += right->mStatsBlock.mSumQLengthOnArrival;
        mStatsBlock.mArrivalsToNonZeroQ  += right->mStatsBlock.mArrivalsToNonZeroQ;
        mStatsBlock.mQLength += right->mStatsBlock.mQLength;
        for(int i = 0; i < ADM_HIST_SIZE+1; i++) {
            mStatsBlock.mReadHistogram[i] += right->mStatsBlock.mReadHistogram[i];
            mStatsBlock.mWriteHistogram[i] += right->mStatsBlock.mWriteHistogram[i];
        }
        for(int i = 0; i < ADM_REHIT_HIST_SIZE+1; i++) {
            mStatsBlock.mRdRehitHist[i] += right->mStatsBlock.mRdRehitHist[i];
            mStatsBlock.mWrRehitHist[i] += right->mStatsBlock.mWrRehitHist[i];
        }

        mStatsBlock.mNumReadAndPin += right->mStatsBlock.mNumReadAndPin;
        mStatsBlock.mNumBlocksPinned += right->mStatsBlock.mNumBlocksPinned;
        mStatsBlock.mNumReadAndPinHits += right->mStatsBlock.mNumReadAndPinHits;
        mStatsBlock.mNumReadAndPinMiss += right->mStatsBlock.mNumReadAndPinMiss;
        mStatsBlock.mNumReadAndPinPgHits += right->mStatsBlock.mNumReadAndPinPgHits;
        mStatsBlock.mNumReadAndPinPgMiss += right->mStatsBlock.mNumReadAndPinPgMiss;

        mStatsBlock.mCurrentVGCredits += right->mStatsBlock.mCurrentVGCredits;
        mStatsBlock.mWriteThrottled += right->mStatsBlock.mWriteThrottled;

	mStatsBlock.mNumRefsWarmedWithHint += right->mStatsBlock.mNumRefsWarmedWithHint;
	mStatsBlock.mNumWrUsedZFVirtualRefs += right->mStatsBlock.mNumWrUsedZFVirtualRefs;
	mStatsBlock.mNumWrUsedNonZFVirtualRefs += right->mStatsBlock.mNumWrUsedNonZFVirtualRefs;
	mStatsBlock.mNumVirtualRefsInZFWr += right->mStatsBlock.mNumVirtualRefsInZFWr;
	mStatsBlock.mNumVirtualRefsInNonZFWr += right->mStatsBlock.mNumVirtualRefsInNonZFWr;
	mStatsBlock.mNumWrNotUsedVirtualRefDueToSizeLimit += right->mStatsBlock.mNumWrNotUsedVirtualRefDueToSizeLimit;
	mStatsBlock.mNumRdUsedVirtualRef += right->mStatsBlock.mNumRdUsedVirtualRef;
	mStatsBlock.mNumVirtualRefInRd += right->mStatsBlock.mNumVirtualRefInRd;
	mStatsBlock.mNumCacheMissWithVirtualRef += right->mStatsBlock.mNumCacheMissWithVirtualRef;
 
        mStatsBlock.mNumReadHitOnly += right->mStatsBlock.mNumReadHitOnly;
        mStatsBlock.mNumReadHitOnlySuccess += right->mStatsBlock.mNumReadHitOnlySuccess;
        mStatsBlock.mNumReadHitOnlyMiss += right->mStatsBlock.mNumReadHitOnlyMiss;
        mStatsBlock.mNumReadHitOnlyAlerted += right->mStatsBlock.mNumReadHitOnlyAlerted;
        mStatsBlock.mNumReadHitOnlyDiscard += right->mStatsBlock.mNumReadHitOnlyDiscard;

        return *this;
    };

    // DebugDisplay 
    // @param flags : 0 = fields, 1 = csv
    // @param proxy : count of proxy (CPU), or 0 to omit
    void DebugDisplay(ULONG flags, ULONG proxy = 0, FILE* fileHandle = stdout, K10_WWID* pWwid = NULL);
    
    // Display the CSV header.
    void DebugDisplayHeading(FILE* fileHandle = stdout) const;

    UINT_64 TotalRequestCount(ULONG cpu)   { return Proxy[cpu%StatsMaxCPUs].TotalRequestCount; }
    UINT_64 SumQLengthOnArrival(ULONG cpu) { return Proxy[cpu%StatsMaxCPUs].SumQLengthOnArrival; }
    STATCTR ArrivalsToNonZeroQ(ULONG cpu)  { return Proxy[cpu%StatsMaxCPUs].ArrivalsToNonZeroQ; }

    UINT_32 Performance_Timestamp; /* navi analyzer TOD timestamp */

    UINT_32 mRaidGroup;           /* RAID Group ID */
    UINT_32 mFlu;                 /* Flare LUN ID */
    UINT_32 mFlushingDirtyRefs;   /* Total dirty refs in the b-tree for the RG */
    UINT_64 mTotalDirtyRefs;      /* Total dirty refs for the LUN */

    CacheVolumeProxy Proxy[StatsMaxCPUs];

};


#endif  // __CacheVolumeStatistics_h__

