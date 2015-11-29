// DRAMCacheCommand.h: interface for the DRAMCacheCommand class
//
// Copyright (C) 2010   EMC Corporation
//
//
// The DRAMCacheCommand class encapsulates the DRAM Cache IOCTLs used directly by admin.
// The use of implementation class (DRAMCacheCommandImpl) helps avoid some header and
// class hierarcy issues.
//
//
//  Revision History
//  ----------------
//  05 Jan 10   R. Hicks    Initial version.
//  22 Mar 10   R. Hicks    DRAM cache statistics support
//  01 Apr 10   R. Hicks    Support new DRAM cache "hidden layering" model
//  10 Apr 10   R. Hicks    Add GetVolumeReadCacheState() / GetVolumeWriteCacheState()
//  26 May 10   R. Hicks    Add delta polling support
//  27 May 11   A. Spang    Add Commit support
//  28 Jul 11   M. Iyer     ARS 427627:  getting the cache dirty status from SP Cache driver instead of flare driver when SP cache is enabled.
//  04 May 12   C. Hopkins  Added management of Fast Cache volumes for the Rockies 'MCC Elevation' project
//  08 Aug 31   H. Weiner   AR 505103: Support for getting write aside

//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------Defines

#ifndef DRAM_CACHE_COMMAND_H
#define DRAM_CACHE_COMMAND_H

#include <windows.h>

#include "user_generics.h"
#include "SafeHandle.h"
#include "SimpUtil.h"
#include "CaptivePointer.h"
#include "K10TraceMsg.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
#include <K10Assert.h>
#include <EmcPAL.h>
#include "HashUtil.h"
#include <Cache/CacheDriverParams.h>
#include <Cache/CacheDriverStatus.h>
#include <Cache/CacheDriverStatistics.h>
#include <Cache/CacheVolumeParams.h>
#include <Cache/CacheVolumeStatus.h>
#include <Cache/CacheVolumeStatistics.h>

#define DRAM_CACHE_DEVICE_NAME_CHAR      "\\\\.\\DRAMCache"

#define FCT_TO_DRAM_DEVICE_NAME_AND_FORMAT     "\\Device\\FctVol%d"
#define FCT_TO_MCR_DEVICE_NAME_AND_FORMAT      "\\Device\\FlareDisk%d"
#define FCT_TO_VFLARE_DEVICE_NAME_AND_FORMAT   "\\Device\\vflare%d"

typedef struct _DRAMCacheVolumeData
{
    CacheVolumeParams       params;
    CacheVolumeStatus       status;
    CacheVolumeStatistics   statistics;

    struct _DRAMCacheVolumeData     *pPrev;
    struct _DRAMCacheVolumeData     *pNext;
} DRAMCacheVolumeData;

class DRAMCacheCommandImpl
{
public:
    DRAMCacheCommandImpl();
    ~DRAMCacheCommandImpl();

    //
    // Get methods
    //

    void        Repoll(bool bPerfData, bool bPollFastCache = false);

    ULONG       GetCachePageSizeSectors()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverStatus.getCachePageSizeSectors();
    }

    bool        IsWCAEnabled()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverParams.IsWCAEnabled();
    }

    CacheState  GetReadCacheState()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverStatus.getReadCacheState();
    }

    CacheState  GetWriteCacheState()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverStatus.getWriteCacheState();
    }

    ULONGLONG   GetMaxCacheSizeMB()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverStatus.getMaxCacheSizeMB();
    }

    ULONGLONG   GetDirtyPageTargetBytes()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverStatus.getDirtyPageTargetBytes();
    }

    UINT_32     GetReadCacheOnlySizeMB()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverParams.GetReadCacheOnlySizeMB();
    }

    bool        IsWriteThru(const K10_WWID &wwid);

    CacheState  GetVolumeReadCacheState(const K10_WWID &wwid);

    CacheState  GetVolumeWriteCacheState(const K10_WWID &wwid);

    LBA_T GetWriteAside(const K10_WWID &wwid);

    CacheDriverStatistics & GetDriverStatistics()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mDriverStatistics;
    }

    CacheVolumeStatistics & GetVolumeStatistics(const K10_WWID &wwid);
    bool        IsCacheLost(const K10_WWID &wwid);

    //
    // Set methods
    //

    void        SetWriteCacheDisable(bool value)
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        mDriverParams.setWriteCacheDisable(value);
    }

    void        SetWriteThru(const K10_WWID &wwid, bool value);

    void        SetWriteAside(const K10_WWID &wwid, UINT_32 writeAside);

    void        CommitDriverChanges();

    void        CommitVolumeChanges(const K10_WWID &wwid);

    void CreateVolume(CacheVolumeParams &volParams, K10_DVR_OBJECT_NAME objectName,
                      DWORD objectLunNumber, const char* baseConsumedNameAndFormat);
    void        DestroyVolume(const K10_WWID &wwid);
    void        RenameVolume(const K10_WWID &oldId, const K10_WWID &newId);
    void        ClearCacheDirtyLun(const K10_WWID &wwid);
    void        ClearStatistics();

    bool        VolumeExists(const K10_WWID &wwid);
    int         GetVolumeCount()
    {
        if(mbRepollFlag)
        {
            Repoll(true);
        }
        return mVolCount;
    }
    const DRAMCacheVolumeData * GetNextVolume(const DRAMCacheVolumeData *pLastVolume);

    void        Commit();

    bool ShrinkSpCacheByMB(ULONGLONG delta);

    void NDUFailureCleanUp();

    void BootCompleted();

    void ValidateFctVolume(K10_WWID wwid);
    void ScrubFctVolumes();

private:
    void GetDriverParams();
    void GetDriverStatus();
    void GetDriverStats();
    void GetVolumes(bool bPerfData);
    void GetFastCacheVolumeData();
    HRESULT CreateFastCacheVolume(K10_WWID Key, K10_WWID RealWWN, K10_DVR_OBJECT_NAME RealUpperName,
                                  DWORD Lun, CHAR * pLowerName, const char * baseConsumedNameAndFormat);
    HRESULT     DestroyFastCacheVolume(K10_WWID Key);
    void UpdateFctVolumeWWN(K10_LU_ID Key, VolumeIdentifier NewWWN);

    void AddVolumeToList(DRAMCacheVolumeData &volData);
    void RemoveVolumeFromList(const K10_WWID &wwid);
    void ClearList();

    DRAMCacheVolumeData & FindVolume(const K10_WWID &wwid);

    SafeHandle          mhFileHandle;
    SafeHandle          mhFctFileHandle;
    SafeIoctl           mSafeIoctl;

    NPtr <K10TraceMsg>  mnpK;
    char                *mpProc;

    CacheDriverParams   mDriverParams;

    CacheDriverStatus   mDriverStatus;

    CacheDriverStatistics   mDriverStatistics;

    ULONGLONG   mLastDriverChangeCount;

    int mVolCount;
    bool mbRepollFlag;

    DRAMCacheVolumeData *mpListHead;

    // Holds address of SPCache memory which we have freed to reduce the
    // size of SPCache. We will need this address to return the memory to
    // SPCache in case of NDU failure.
    ULONG64      mShrunkSPCacheAddr;

    NPtr< HashUtil<K10_WWID, DRAMCacheVolumeData *> >   mnpVolumeHashTable;

    CPtr <csx_uchar_t>     mcpFctVolData;
};

class CSX_CLASS_EXPORT  DRAMCacheCommand
{
public:

    DRAMCacheCommand();
    ~DRAMCacheCommand();

    //
    // Get methods
    //

    void        Repoll(bool bPerfData, bool bPollFastCache = false);

    ULONG       GetCachePageSizeSectors();

    bool        IsWCAEnabled();

    CacheState  GetReadCacheState();

    CacheState  GetWriteCacheState();

    ULONGLONG   GetMaxCacheSizeMB();

    ULONGLONG   GetDirtyPageTargetBytes();

    UINT_32     GetReadCacheOnlySizeMB();

    bool        IsWriteThru(const K10_WWID &wwid);

    CacheState  GetVolumeReadCacheState(const K10_WWID &wwid);

    CacheState  GetVolumeWriteCacheState(const K10_WWID &wwid);

    CacheVolumeStatistics & GetVolumeStatistics(const K10_WWID &wwid);

    CacheDriverStatistics & GetDriverStatistics();

    bool        IsCacheLost(const K10_WWID &wwid);

    LBA_T       GetWriteAside(const K10_WWID &wwid);

    //
    // Set methods
    //

    void        SetWriteCacheDisable(bool value);

    void        SetWriteThru(const K10_WWID &wwid, bool value);

    void        SetWriteAside(const K10_WWID &wwid, UINT_32 writeAside);

    void        CommitDriverChanges();

    void        CommitVolumeChanges(const K10_WWID &wwid);

    void CreateVolume(CacheVolumeParams &volParams, K10_DVR_OBJECT_NAME objectName,
                      DWORD objectLunNumber, const char* baseConsumedNameAndFormat);
    void        DestroyVolume(const K10_WWID &wwid);
    void        RenameVolume(const K10_WWID &oldId, const K10_WWID &newId);

    void        ClearCacheDirtyLun(const K10_WWID &wwid);
    void        ClearStatistics();

    bool        VolumeExists(const K10_WWID &wwid);
    int         GetVolumeCount();
    const DRAMCacheVolumeData * GetNextVolume(const DRAMCacheVolumeData *pLastVolume);

    void        Commit();

    bool ShrinkSpCacheByMB(ULONGLONG delta);

    void NDUFailureCleanUp();

    void BootCompleted();

    void ValidateFctVolume(K10_WWID wwid);

    void ScrubFctVolumes();
private:
    DRAMCacheCommandImpl *mpImpl;
};


#endif
