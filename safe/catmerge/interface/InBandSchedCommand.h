// InBandSchedCommand.h: interface for the InBandSchedCommand class
//
// Copyright (C) 2010   EMC Corporation
//
//
// The In Band Scheduler Command class encapsulates the In Band Scheduler IOCTLs used directly by admin.
// The use of implementation class (InBandSchedCommandImpl) helps avoid some header and
// class hierarcy issues.
//
//
//  Revision History
//  ----------------

//////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------Defines

#ifndef INBAND_SCHED_COMMAND_H
#define INBAND_SCHED_COMMAND_H

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
#include <InBandSched/InBandSchedVolumeParams.h>

#define INBAND_SCHED_DEVICE_NAME_CHAR      "\\\\.\\InBandSched"


typedef struct _InBandSchedVolumeData
{
    InBandSchedVolumeParams     params;

} InBandSchedVolumeData;

class InBandSchedCommandImpl
{
public:
    InBandSchedCommandImpl();
    ~InBandSchedCommandImpl();

    //
    // Get methods
    //
    void        Repoll(bool bPerfData);

    //
    // Set methods
    //
    void        CommitDriverChanges();

    void        CommitVolumeChanges(K10_WWID wwid);

    void        CreateVolume(InBandSchedVolumeParams &volParams, K10_DVR_OBJECT_NAME objectName, DWORD objectLunNumber);
    void        DestroyVolume(K10_WWID wwid);
    void        RenameVolume(K10_WWID oldId, K10_WWID newId);

    bool        VolumeExists(K10_WWID wwid);
    int         GetVolumeCount()    { return mVolCount; }
    K10_WWID &  GetVolumeWwn(int index) { return (*mnapVolumeList)[index].params.LunWwn; }

private:
    void GetVolumes(bool bPerfData);

    InBandSchedVolumeData & FindVolume(K10_WWID wwid);

    SafeHandle          mhFileHandle;
    SafeIoctl           mSafeIoctl;

    NPtr <K10TraceMsg>  mnpK;
    char                *mpProc;

    int mVolCount;

    NAPtr <InBandSchedVolumeData>   mnapVolumeList;

    NPtr< HashUtil<K10_WWID, InBandSchedVolumeData *> > mnpVolumeHashTable;
};

class CSX_CLASS_EXPORT  InBandSchedCommand
{
public:

    InBandSchedCommand();
    ~InBandSchedCommand();

    //
    // Get methods
    //
    void        Repoll(bool bPerfData);


    //
    // Set methods
    //
    void        CommitDriverChanges();

    void        CommitVolumeChanges(K10_WWID wwid);

    void        CreateVolume(InBandSchedVolumeParams &volParams, K10_DVR_OBJECT_NAME objectName, DWORD objectLunNumber);
    void        DestroyVolume(K10_WWID wwid);
    void        RenameVolume(K10_WWID oldId, K10_WWID newId);

    bool        VolumeExists(K10_WWID wwid);
    int         GetVolumeCount();
    K10_WWID &  GetVolumeWwn(int index);

private:
    InBandSchedCommandImpl *mpImpl;
};


#endif
