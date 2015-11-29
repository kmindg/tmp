#ifndef __CacheDriverAdminParams_h__
#define __CacheDriverAdminParams_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2013
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheDriverParams.h
//
// Contents:
//      Defines the CacheDriver Parameters passed between the DRAM Cache 
//      and the admin layers. 
//
// Note: The members of this class have a default value of zero or false so 
//       that they maybe initialized correctly as part of a larger block
//       of generic memory which is initialized with a call to EmcpalZeroMemory().
//
// Revision History:
//--

#include "BasicVolumeDriver/BasicVolumeParams.h"
#include "EmcPAL_Memory.h"

// Define a number of spare UINT_32 locations which are available for later use.
#define NUM_DRV_ADMIN_SPARES (6)

class CacheDriverAdminParams : public BasicVolumeDriverParams {
public:
    // Default values 
    CacheDriverAdminParams() { Initialize(); };
    ~CacheDriverAdminParams() {};

    // Provide an initialize method which sets the default values.
    // This is public incase admin needs to initalize the object separately.
    void Initialize()
    {
     mWriteCacheDisable = FALSE;
     mAutoPrefetchDisable = FALSE;
     mDeemphasizeTornWriteAvoidance = FALSE;
     mPeerHintDisabled = FALSE;
     mReadCacheOnlySize = 0;
     mMaxInFlightReferenceSenderMessages = 0;  // system default
     mMaxCoalescedPagesPerSenderMessage = 0;             // Use system default
     mWriteThrottleDisable = false;
#ifndef SIMMODE_ENV
     mBEReadCrcCheckEnabled = false;   // If not in simulation mode this check is disabled by DEFAULT
#else
#if DBG
     mBEReadCrcCheckEnabled = true;   // if in simmode and compiled for debug, this check is enabled
#else
     mBEReadCrcCheckEnabled = false;  // if in simmode and NOT compiled for debug, this check is disabled
#endif
#endif
     EmcpalZeroMemory(&mPad, sizeof(mPad));
     EmcpalZeroMemory(&mSpares[0], sizeof(UINT_32) * NUM_DRV_ADMIN_SPARES);
    }   

    // Provide access to parameters with the sense corrected.
    bool IsReadCacheEnabled() const { return TRUE; };
    bool IsWriteThruEnabled() const { return mWriteCacheDisable; };
    bool IsAutoPrefetchDisabled() const { return mAutoPrefetchDisable; };
    bool IsDeemphasizeTornWriteAvoidanceSet() const { return mDeemphasizeTornWriteAvoidance; };
    bool IsPeerHintEnabled() const { return !mPeerHintDisabled; }
    bool IsWCAEnabled() const { return TRUE; };
    bool IsWriteThrottleDisabled() const { return mWriteThrottleDisable; }
    bool IsBEReadCrcCheckEnabled() const { return mBEReadCrcCheckEnabled; }
    UINT_32 GetReadCacheOnlySizeMB() const { return mReadCacheOnlySize; };
    UINT_32 GetMaxInFlightReferenceSenderMessages() const { return mMaxInFlightReferenceSenderMessages; }
    UINT_32 GetMaxCoalescedPagesPerSenderMessage() const { return mMaxCoalescedPagesPerSenderMessage; };

    // Setters 
    // Page size is not setable. It is set in the registry.
    // Read Cache is always enabled so it is not setable.
    void setWriteCacheDisable(bool value) { mWriteCacheDisable = value; };
    void setAutoPrefetchDisable(bool value) { mAutoPrefetchDisable = value; };
    void setDeemphasizeTornWriteAvoidance(bool value) { mDeemphasizeTornWriteAvoidance = value; };
    void setPeerHintEnabled(bool value) { mPeerHintDisabled = !value; }
    void setReadCacheOnlySizeMB(UINT_32 value) { mReadCacheOnlySize = value; };
    void setMaxInFlightReferenceSenderMessages(UINT_16E value) { mMaxInFlightReferenceSenderMessages = value; };
    void setMaxCoalescedPagesPerSenderMessage(UINT_16E value) { mMaxCoalescedPagesPerSenderMessage = value; };
    void setWriteThrottleDisable(bool value) { mWriteThrottleDisable = value; }
    void setBEReadCrcCheckEnabled(bool value) { mBEReadCrcCheckEnabled = value; }

    // Copy constructor
    CacheDriverAdminParams & operator= (const CacheDriverAdminParams &right)
    {
        // Copy the Basic Volume parameters first.
        BasicVolumeDriverParams* left = static_cast<BasicVolumeDriverParams*>(this);
        *left = static_cast<BasicVolumeDriverParams>(right);

        if (&right != this) {
            mWriteCacheDisable = right.mWriteCacheDisable;
            mAutoPrefetchDisable = right.mAutoPrefetchDisable;
            mDeemphasizeTornWriteAvoidance = right.mDeemphasizeTornWriteAvoidance;
            mPeerHintDisabled = right.mPeerHintDisabled;
            mReadCacheOnlySize = right.mReadCacheOnlySize;
            mMaxInFlightReferenceSenderMessages = right.mMaxInFlightReferenceSenderMessages;
            mMaxCoalescedPagesPerSenderMessage = right.mMaxCoalescedPagesPerSenderMessage;
            mWriteThrottleDisable = right.mWriteThrottleDisable;
            mBEReadCrcCheckEnabled = right.mBEReadCrcCheckEnabled;
        }
        return *this;
    };

    // Copy constructor from buffer
    CacheDriverAdminParams & operator= (const char * buffer)
    {
        CacheDriverAdminParams *right = (CacheDriverAdminParams *)buffer;

        // mReadCacheEnabled skipped since it is not changeable.
        mWriteCacheDisable = right->mWriteCacheDisable;
        mAutoPrefetchDisable = right->mAutoPrefetchDisable;
        mDeemphasizeTornWriteAvoidance = right->mDeemphasizeTornWriteAvoidance;
        mPeerHintDisabled = right->mPeerHintDisabled;
        mReadCacheOnlySize = right->mReadCacheOnlySize;
        mMaxInFlightReferenceSenderMessages = right->mMaxInFlightReferenceSenderMessages;
        mMaxCoalescedPagesPerSenderMessage = right->mMaxCoalescedPagesPerSenderMessage;
        mWriteThrottleDisable = right->mWriteThrottleDisable;
        mBEReadCrcCheckEnabled = right->mBEReadCrcCheckEnabled;

        return *this;
    };


private:
    bool mWriteCacheDisable;          // Write Cache Disable
    bool mAutoPrefetchDisable;        // Automatic Prefetching Disable

    // FALSE: All writes of a 1MB (TBD) size or smaller will be
    // atomically applied so that no single failure can 
    // cause a partial write.
    // TRUE: Do not compromise performance to avoid torn
    // writes on single failures, instead assuming that
    // host will rewrite.
    bool mDeemphasizeTornWriteAvoidance;

    // TRUE: Prevent creation of peer hint references 
    // False: Use the peer hint code
    // FIX: Remove this when the pert hint code is known stable.
    bool mPeerHintDisabled;

    UINT_32 mReadCacheOnlySize;       // Read cache only size in MB
                                      // Specifies the amount of memory in
                                      // the cache that can only be used
                                      // for read cache pages. 

    // The number of reference sender messages that can be outstanding
    // at one time.
    UINT_16E mMaxInFlightReferenceSenderMessages;

    // The number of references we max try to put into a message.
    UINT_16E mMaxCoalescedPagesPerSenderMessage;

     // Write throttling disable.
    bool mWriteThrottleDisable;
        
    // BE Read Crc Checking enabled.  Default Fault
    bool mBEReadCrcCheckEnabled;

    bool mPad[2]; /* SAFEBUG - to keep this 8 byte aligned */
private:
    UINT_32 mSpares[NUM_DRV_ADMIN_SPARES]; 
};

#endif  // __CacheDriverAdminParams_h__

