#ifndef __CacheVolumeParams_h__
#define __CacheVolumeParams_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2014
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CacheVolumeParams.h
//
// Contents:
//      Defines the CacheVolume Parameters passed between the cache and 
//      the admin layer. 
//
// Note: The members of this class have a default value of zero or false so 
//       that they maybe initialized correctly as part of a larger block
//       of generic memory which is initialized with a call to EmcpalZeroMemory().
//
// Revision History:
//--

#include "BasicVolumeDriver/BasicVolumeParams.h"
#include "EmcPAL_Memory.h"
#include <stdio.h>

#ifdef I_AM_DEBUG_EXTENSION
#include "csx_ext_dbgext.h"
#define DISPLAY csx_dbg_ext_print 
#elif defined(_WDBGEXTS_) && defined(KDEXT_64BIT)
#define DISPLAY(...) nt_dprintf(__VA_ARGS__)
// FIXME: Need to have mut_printf for mut tests but not sure how to do it.
//              as mut_printf will take firt argument as level.
//#elif defined(MUT_API) 
//#define DISPLAY mut_printf
#else
#define DISPLAY(...) fprintf(fileHandle, __VA_ARGS__)
#endif 

#define NUM_VOL_PARAM_SPARES (8)

// Size of ZF bitmap
#define SIZE_OF_ZF_BITMAP_IN_SECTORS 127

// Value which should uniquely identify the volume across other volumes, for all time.
// Can be the WWN.
typedef UINT_64 CacheUniqueVolumeKey;

class CacheVolumeParams : public FilterDriverVolumeParams {
public:
    static const ULONG SectorsPerZeroFillBit = 2048;  // 1MB @ 512 byte sectors

    // Default values 
    CacheVolumeParams() {
        Initialize();
    };

    ~CacheVolumeParams() {};

    // Typical initialization:
    //              CacheVolumeParams volParams;
    //              memset(&volParams, 0, sizeof(CacheVolumeParams));
    //              volParams.Initialize();
    // Which makes sure that unused space, including bitfield space, is zero.

    // Provide an initialize method which sets the default values.
    // This is public incase admin needs it.
    void Initialize()
    {
//        commented out until the compiler supports static_assert.
//        static_assert((PVOID)reinterpret_cast<CacheVolumeParams *> (0) == (PVOID) static_cast<BasicVolumeParams*>(reinterpret_cast<CacheVolumeParams*>(0)),
//                      "CacheVolumeParams and BasicVolumeParams are misaligned");
//
        mWriteCacheDisabled = FALSE;
        mAutoPrefetchDisable = FALSE;
        mReadEntirePageDisable = FALSE;
        mAllowWriteWhenNotReady = FALSE;
        mAllowReadWhenNotReady = FALSE;
        mAvoidTornWritesWhenWriteCacheDisabled = FALSE;
        mNonTransactional = false;
        mShouldAlwaysBypassCache = 0;
        mImportance = 0;
        mUniqueKey = 0;
        mDataFormat = 0;    // LBA 0 == BE LBA 0; bitmap size 1MB/bit
        mZeroFillBitmapLba = 0;      
        mCacheLost = FALSE;
        mWriteThroughThresholdSectors = 0;//Default is zero, meaning infinite threshold.
        mReadEntirePageThreshold = 0;    // Use system default
        mUserVolSize = 0;
        mAutoInsertKey = K10_WWID_Zero();
        mPad = 0;
        mAutoInsertBelow = false;


        EmcpalZeroMemory(&mSpares[0], sizeof(UINT_32) * NUM_VOL_PARAM_SPARES);
    }   

    // Provide access to parameters with the sense corrected.
    bool IsReadCacheEnabled() const { return TRUE; };
    bool IsWriteThru() const { return mWriteCacheDisabled; };
    bool IsAutoPrefetchDisabled() const { return mAutoPrefetchDisable; };
    bool IsReadEntirePageDisabled() const { return mReadEntirePageDisable; };
    bool IsWriteAllowedWhenNotReady() const { return mAllowWriteWhenNotReady; };
    bool IsNonTransactional() const { return mNonTransactional; };
    bool IsReadAllowedWhenNotReady() const { return mAllowReadWhenNotReady; };
    bool IsAvoidTornWritesWhenWriteCacheDisabled() const { return mAvoidTornWritesWhenWriteCacheDisabled; };
    bool IsCacheLost() const { return mCacheLost; };
    bool ShouldAlwaysBypassCache() const { return (mShouldAlwaysBypassCache != 0); };

    UINT_32 getVolumeImportance() const { return mImportance; };
    LBA_T GetWriteThroughThreshold() const { return mWriteThroughThresholdSectors; };
    ULONG GetReadEntirePageThreshold() const { return mReadEntirePageThreshold; }
    LBA_T GetUserVolumeSize() const {return mUserVolSize; };
    ULONG getSectorsPerZeroFillBit() { return SectorsPerZeroFillBit; };  // Constant, not parameter.
    ULONGLONG getZFLba() { return mZeroFillBitmapLba; };
    VolumeIdentifier GetAutoInsertKey() { return mAutoInsertKey; };

    // Setters for each with sense inverted.
    void setWriteThru(bool value) { mWriteCacheDisabled = value; };
    void setAutoPrefetchDisable(bool value) { mAutoPrefetchDisable = value; };
    void setReadEntirePageDisable(bool value) { mReadEntirePageDisable = value; };
    void setWritesAllowedWhenNotReady(bool value) { mAllowWriteWhenNotReady = value; };
    void setReadsAllowedWhenNotReady(bool value)  { mAllowReadWhenNotReady = value; };
    void setAvoidTornWritesWhenWriteCacheDisabled(bool value) { mAvoidTornWritesWhenWriteCacheDisabled = value; };
    void setVolumeImportance(UINT_32 imp) { mImportance = imp; };
    void setWriteThroughThreshold(LBA_T value) { mWriteThroughThresholdSectors = value; };
    void setReadEntirePageThreshold(UINT_32 value) { mReadEntirePageThreshold = value; }
    void setAutoInsertKey(VolumeIdentifier value) {mAutoInsertKey = value; };
    void setNonTransactional(bool nonTransactional)  { mNonTransactional = nonTransactional; }
    void setShouldAlwaysBypassCache(ULONG bypassEnabled) { mShouldAlwaysBypassCache = bypassEnabled; }


    // Calculates the bitmap size based on lunSize sectors.
    // @param lunSize - size in sectors.
    LBA_T bitmapSize(LBA_T lunSize) {
        ULONGLONG bits = (lunSize - 1)/SectorsPerZeroFillBit + 1;
        ULONGLONG bytes = (bits - 1)/8 + 1;
        LBA_T bmSectors = (bytes - 1)/512 + 1;
        return bmSectors + 128;  // We need to add room for the 64KB bitmap header
    }

    // Returns the bitmap size in sectors.
    LBA_T bitmapSize() {
        return bitmapSize(mUserVolSize);
    }

    // Sets the user volume size to userSize sectors.
    // @param userSize -  size in sectors.
    void setUserVolumeSize(LBA_T userSize) {
        mUserVolSize = userSize;
    }

    // If userSize and overallSize specified and hasBitmap is true, then figures out if bitmap fits.
    // If userSize specified and overallSize is zero and hasBitmap is true, then put bitmap above userSize.
    // If userSize is zero and overallSize is specified and hasBitmap is true, then put bitmap under overallSize,
    // and usersize under bitmap.
    // @param userSize - user size in sectors.
    // @param overallSize - overall size in sectors.
    // @param hasBitmap - whether to place a bitmap.

    void setUserVolumeSizeAndZFBitmapLocation(LBA_T userSize, LBA_T overallSize, bool hasBitmap) {
        if(!userSize && !overallSize)
            return;
        if(hasBitmap) {
            if (userSize) {
                mUserVolSize = userSize;
                mZeroFillBitmapLba = (userSize + SIZE_OF_ZF_BITMAP_IN_SECTORS) & ~(ULONGLONG)SIZE_OF_ZF_BITMAP_IN_SECTORS;
                if (overallSize && ((bitmapSize(userSize) + mZeroFillBitmapLba) > overallSize)) {
                    mZeroFillBitmapLba = (LBA_T)0;
                }
            }
            else {
                mZeroFillBitmapLba = (overallSize - bitmapSize(overallSize)) & ~(ULONGLONG)SIZE_OF_ZF_BITMAP_IN_SECTORS;
                mUserVolSize = mZeroFillBitmapLba;
            }
        }
        else {
            mUserVolSize = userSize ? userSize : overallSize;
            mZeroFillBitmapLba = (LBA_T)0;
        }
    };
    void setFormat(ULONG format) { mDataFormat = format; };
    void setZFLba(ULONGLONG zeroFillLba) { mZeroFillBitmapLba = zeroFillLba; };
    void setCacheLost(bool value) { mCacheLost = value; };

    void CreateAutoInsertKey(const ULONG Lun) 
    {
        // There are special requirements for a key other than the real WWN
        // that the SP Cache driver is to use when auto-inserting a driver above itself
        mAutoInsertKey = K10_WWID_Zero();
        *(ULONG *) (VOID *) &mAutoInsertKey.node = 1;  
        *(ULONG *) (VOID *) &mAutoInsertKey.nPort = Lun;
    }


    void debugdisplay(FILE* fileHandle = stdout){
        bool WriteCacheDisabled = IsWriteThru();
        bool AutoPrefetchDisable = IsAutoPrefetchDisabled();
        bool ReadEntirePageDisable = IsReadEntirePageDisabled();
        bool AllowWriteWhenNotReady = IsWriteAllowedWhenNotReady();
        bool AllowReadWhenNotReady = IsReadAllowedWhenNotReady();
        bool AvoidTornWritesWhenWriteCacheDisabled = IsAvoidTornWritesWhenWriteCacheDisabled();
        bool NonTransactional = IsNonTransactional();
        bool AlwaysBypassCache = ShouldAlwaysBypassCache();
        
        DISPLAY("Printing Cache Volume Parameters\n");
        DISPLAY("\tLun Wwn                : %016llX:%016llX\n",   swap64(*(_int64*)LunWwn.nPort.bytes ),swap64(*(_int64*)LunWwn.node.bytes));
        DISPLAY("\tGang Id                : %016llX:%016llX\n",   swap64(*(_int64*)GangId.nPort.bytes ),swap64(*(_int64*)GangId.node.bytes));
        DISPLAY("\tUpper Device Name      : %s\n", UpperDeviceName);
        DISPLAY("\tDevice Link Name       : %s\n", DeviceLinkName);
        DISPLAY("\tLower Device Name      : %s\n", LowerDeviceName);
        DISPLAY("\tCacheLost              : %ld\n", (long int)mCacheLost);
        DISPLAY("\tmImportance            : %ld\n", (long int)mImportance);
        DISPLAY("\tmDataFormat            : %ld\n", (long int)mDataFormat);
        DISPLAY("\tmZeroFillBitmapLba     : 0x%llx\n", mZeroFillBitmapLba);
        DISPLAY("\tmWriteThroughThresholdSectors    : %llu\n", mWriteThroughThresholdSectors);
        DISPLAY("\tmReadEntirePageThreshold         : %ld\n", (long int)mReadEntirePageThreshold);
        DISPLAY("\tmUserVolSize           : 0x%llx\n", mUserVolSize);
        DISPLAY("\tWriteCacheDisabled     : %d\n", WriteCacheDisabled);
        DISPLAY("\tNonTransactional       : %d\n", NonTransactional);
        DISPLAY("\tAlwaysBypassCache      : %d\n", AlwaysBypassCache);        
        DISPLAY("\tAutoPrefetchDisable    : %d\n", AutoPrefetchDisable);
        DISPLAY("\tReadEntirePageDisable  : %d\n", ReadEntirePageDisable);
        DISPLAY("\tAllowWriteWhenNotReady : %d\n", AllowWriteWhenNotReady);
        DISPLAY("\tAllowReadWhenNotReady  : %d\n", AllowReadWhenNotReady);
        DISPLAY("\tAvoidTornWritesWhenWriteCacheDisabled: %d\n", AvoidTornWritesWhenWriteCacheDisabled);        
        DISPLAY("\tUniqueKey              : %llu\n", mUniqueKey);
        DISPLAY("\tAutoInsertKey          : %016llX:%016llX\n",   swap64(*(_int64*)mAutoInsertKey.nPort.bytes ),swap64(*(_int64*)mAutoInsertKey.node.bytes)); }
    // Classes with only attributes, with simple GetAttrs/SetAttrs, are not hiding anything, and the result
    // is simply more lines of code vs. accessing the fields directly.  The effect of the  approach
    // of making structures pseudo classes is to increate the amount of typing, and to increase the
    // levels of indirection when searching, without any value add.
public:

    bool mWriteCacheDisabled;       // Write Cache Disable (Cache is in write-through mode)
    bool mAutoPrefetchDisable;      // Automatic Prefetching Disable
    bool mReadEntirePageDisable;    // Read Entire Page Enable
    bool mAllowWriteWhenNotReady;   // Writes are allowed even when the volume is offline
    bool mAllowReadWhenNotReady;    // Reads are allowed even when the volume is offline
    bool mAvoidTornWritesWhenWriteCacheDisabled;  // Avoid torn (partial) writes when the Write Cache is disabled  
    bool mCacheLost;                // false- cache OK, true suffered DL.
    bool mNonTransactional;         // false - transactional writes for any single fault, true - no attempt at transactional writes across faults
    UINT_32 mPad; /* SAFEBUG - to keep this 8 byte aligned */
    UINT_32 mImportance;            // Volume importance (priority)
    LBA_T mUserVolSize;             // User requested Volume Size duirng bind operation
    LBA_T mWriteThroughThresholdSectors ;   // Performance hint: Writes of this size or larger should
                                    // be written through the cache, rather than as write-back.
    CacheUniqueVolumeKey mUniqueKey;// This value is set on creation, and not changable.  Must be unique across all LUNs on an array for all time.
    ULONG                mDataFormat;// How is data layed out?  
                                     //    Format 0: LBA 0 @ back-end LBA 0, optional zero fill bitmap @ mZeroFillBitmapLba w/ 1MB per zf bit.
    ULONG     mShouldAlwaysBypassCache;    // Bypass the cache for the particular volume.
    ULONGLONG mZeroFillBitmapLba;   // Start Lba of ZF bitmap
    VolumeIdentifier mAutoInsertKey; // Key to use when auto inserting other drivers onto this volume
    bool             mAutoInsertBelow; // usually false, used to indicate system and celerra LUNs that should have lower drivers inserted
    ULONG mReadEntirePageThreshold; // Threshold pct to do read-full-page

    UINT_32 mSpares[NUM_VOL_PARAM_SPARES];

};

struct CacheVolumeAction : BasicVolumeAction {


    // Bitmap
    enum { 
        // If a not ready LUN has dirty cache, purge the dirty cache 
        // and mark the LUN as "cache lost" (except when or'd with ClearCacheLost).
        ActionDiscardDirtyDataButOnlyIfDeviceNotReady= 1,

        // If the write cache was somehow lost, allow this LUN to again service I/O.
        // No-op if cache was not lost.
        ActionClearCacheLost = 2,

        // Sets cache lost.
        ActionSetCacheLost = 4,
    };

    static const int SupportedActions = (ActionDiscardDirtyDataButOnlyIfDeviceNotReady
        |ActionClearCacheLost|ActionSetCacheLost);

    // A bitmap of actions to perform, if the bit is set.
    ULONG   SetOfActionsToPerform;
};



#endif  // __CacheVolumeParams_h__

