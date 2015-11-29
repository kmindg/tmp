#ifndef __CacheDriverStatus_h__
#define __CacheDriverStatus_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2014
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheDriverStatus.h
//
// Contents:
//      Defines the CacheDriver Status passed between the cache and the admin
//      layer. 
//
// Revision History:
//--

# include "BasicVolumeDriver/BasicVolumeParams.h"
# include "CacheStates.h"

#define NUM_DRV_STATUS_SPARES (6)

class CacheDriverStatus : public BasicVolumeDriverStatus {
public:
    // Default values 
    CacheDriverStatus() { Initialize(); };
    ~CacheDriverStatus() {};

    // Provide an initialize method which sets the default values.
    void Initialize()
    {
        mWriteCacheState = Enabled;
        mPad = Enabled;
        mReadCacheState = Enabled;
        mDirtyPageTarget = 0;
        mMinCacheSize = 0;
        mMaxCacheSize = 0;
        mCurrentCacheSize = 0;
        mPageSize = 16;
        mCacheNotRecovered = 0;
        mCurrentPreservationSizeMB = 0;
        EmcpalZeroMemory(&mSpares[0], sizeof(ULONG) * NUM_DRV_STATUS_SPARES);
    }   

    // Provide access to the status.
    CacheState getReadCacheState() const { return mReadCacheState; };
    CacheState getWriteCacheState() { return mWriteCacheState; };
    ULONGLONG getDirtyPageTargetBytes() { return mDirtyPageTarget; };
    ULONGLONG getMinCacheSizeMB() { return mMinCacheSize; };
    ULONGLONG getMaxCacheSizeMB() { return mMaxCacheSize; };
    ULONGLONG getCurrentPmpPreservationSizeMB() { return mCurrentPreservationSizeMB; };
    ULONG getCurrentCacheSizeMB() { return mCurrentCacheSize; };
    ULONG getCachePageSizeSectors() const { return mPageSize; };
    ULONG getCacheNotRecovered() const { return mCacheNotRecovered; }

    // Setters (used by the Cache driver only)
    void setWriteCacheState(CacheState state) { mWriteCacheState = state; };
    void setReadCacheState(CacheState state) { 
        // Read cache state is only allowed to be Enabled or Disabled (in bypass)
        if (state == Enabled) {
            mReadCacheState = Enabled; 
        } else {
            mReadCacheState = Disabled;
        }
    };
    void setDirtyPageTarget(ULONGLONG value) { mDirtyPageTarget = value; };
    void setMinCacheSizeMB(ULONGLONG value) { mMinCacheSize = value; };
    void setMaxCacheSizeMB(ULONGLONG value) { mMaxCacheSize = value; };
    void setCurrentCacheSizeMB(ULONG value) { mCurrentCacheSize = value; };
    void setCurrentPmpPreservationSizeMB(ULONGLONG value) { mCurrentPreservationSizeMB = value; };
    void setCachePageSizeSectors(ULONG value) { 
        // Legal possibilities include 8K, 16K or 64K only.
        FF_ASSERT((value == 16) || 
                  (value == 32) ||
                  (value == 128));
        mPageSize = value;
    }
    void setCacheNotRecovered( ULONG value ) { mCacheNotRecovered = value; }

    // Copy constructor
    CacheDriverStatus & operator= (const CacheDriverStatus &right)
    {
        if (&right != this) {
            mWriteCacheState = right.mWriteCacheState;
            mDirtyPageTarget = right.mDirtyPageTarget;
            mMinCacheSize = right.mMinCacheSize;
            mMaxCacheSize = right.mMaxCacheSize;
            mCurrentCacheSize = right.mCurrentCacheSize;
            mReadCacheState = right.mReadCacheState;
            mCacheNotRecovered = right.mCacheNotRecovered;
            mCurrentPreservationSizeMB = right.mCurrentPreservationSizeMB;

            if (mPageSize == 16 || mPageSize == 32 ||  mPageSize == 128) {
                // Only allow legal page sizes to be copied
                mPageSize = right.mPageSize;
            } else {
                // If the page size is zero set it to the default value
                mPageSize = 16;
            }
        }
        return *this;
    };

    // Copy constructor from buffer
    CacheDriverStatus & operator= (const char * buffer)
    {
        CacheDriverStatus *right = (CacheDriverStatus *)buffer;

        mWriteCacheState = right->mWriteCacheState;
        mDirtyPageTarget = right->mDirtyPageTarget;
        mMinCacheSize = right->mMinCacheSize;
        mMaxCacheSize = right->mMaxCacheSize;
        mCurrentCacheSize = right->mCurrentCacheSize;
        mReadCacheState = right->mReadCacheState;
        mCacheNotRecovered = right->mCacheNotRecovered;
        mCurrentPreservationSizeMB = right->mCurrentPreservationSizeMB;

        if (mPageSize == 16 || mPageSize == 32 || mPageSize == 128) {
            // Only allow legal page sizes to be copied
            mPageSize = right->mPageSize;
        } else {
            // If the page size is zero set it to the default value
            mPageSize = 16;
        }

        return *this;
     };


private:
    CacheState mWriteCacheState;             // Write cache state
    CacheState mPad; /* SAFEBUG - to keep this 8 byte aligned */
    ULONGLONG mDirtyPageTarget;              // Dirty Page Target in bytes
    ULONGLONG mMinCacheSize;                 // Minimum Cache size in MB
    ULONGLONG mMaxCacheSize;                 // Maximum Cache size in MB
    ULONGLONG mCurrentPreservationSizeMB;    // 0, if not vaulting to PMP, otherwise amount of cache to be preserved.
    ULONG mCurrentCacheSize;                 // Current Cache size in MB
    ULONG mPageSize;                         // Default cache page size in sectors
    CacheState mReadCacheState;              // Read Cache state (Disabled == Bypass mode).
    ULONG mCacheNotRecovered;                // The cache is not able to be recovered from either peer nor vault
    ULONG mSpares[NUM_DRV_STATUS_SPARES];
};


class CacheDriverStatusFilter : public BasicDriverStatusFilter {
public:
    // Default values 
    CacheDriverStatusFilter() {};
    ~CacheDriverStatusFilter() {};
};

#endif  // __CacheDriverStatus_h__

