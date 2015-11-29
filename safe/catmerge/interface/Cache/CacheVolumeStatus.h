#ifndef __CacheVolumeStatus_h__
#define __CacheVolumeStatus_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2011
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheVolumeStatus.h
//
// Contents:
//      Defines the CacheVolume Status passed between the cache and the admin
//      layer. 
//
// Revision History:
//--

# include "BasicVolumeDriver/BasicVolumeParams.h"
# include "CacheStates.h"

#define NUM_VOL_STATUS_SPARES (8)

class CacheVolumeStatus : public BasicVolumeStatus {
public:
    // Default values 
    CacheVolumeStatus() { Initialize(); };
    ~CacheVolumeStatus() {};

    // Provide an initialize method which sets the default values.
    void Initialize()
    {
        mWriteCacheState = Enabled;
        mVolumeCacheDirty = false;
        mVolumeOffLine = true;
        EmcpalZeroMemory(&mSpares[0], sizeof(UINT_32) * NUM_VOL_STATUS_SPARES);
    }   

    // Provide access to the status.
    CacheState getReadCacheState() { return Enabled; };
    CacheState getWriteCacheState() { return mWriteCacheState; };
    bool isVolumeCacheDirty() { return mVolumeCacheDirty; };
    bool isVolumeOffLine() { return mVolumeOffLine; };

    // Setters
    void setWriteCacheState(CacheState state) { mWriteCacheState = state; };
    void setVolumeCacheDirty(bool value) { mVolumeCacheDirty = value; };
    void setVolumeOffLine(bool value) { mVolumeOffLine = value; };

private:
    CacheState mWriteCacheState;             // Write cache state (write-back, cleaning or write-thru)
    bool mVolumeCacheDirty;                  // Volume is currently Cache dirty
    bool mVolumeOffLine;                     // Volume is currently off-line
    UINT_32 mSpares[NUM_VOL_STATUS_SPARES];
};

#endif  // __CacheVolumeStatus_h__

