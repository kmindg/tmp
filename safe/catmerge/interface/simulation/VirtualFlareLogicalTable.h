/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               VirtualFlareLogicalTable.h
 ***************************************************************************
 *
 * DESCRIPTION: Define VirtualFlareLogicalTable class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    04/11/2011  Bhavesh Patel Initial Version
 *
 **************************************************************************/
#ifndef __VIRTUALFLARELOGICALTABLE_
#define __VIRTUALFLARELOGICALTABLE_

#include "simulation/DistributedService.h"
#include "simulation/AbstractSystemConfigurator.h"
# include "simulation/AbstractSPControl.h"
#include "simulation/BvdSimDeviceManagement.h"
#include "K10Assert.h"
#include "k10defs.h"

#if defined(VFDRIVER_EXPORT)
#define VFDRIVERPUBLIC CSX_MOD_EXPORT 
#else
#define VFDRIVERPUBLIC CSX_MOD_IMPORT 
#endif

// FIXME: It needs to come from registry or we need some mechanism to be in sync with BVD.
#define MAX_INTERNAL_VOLUMES 0x10

// VirtualFlareLogicalTable provides support for this many LUNs max.
#define VFLT_MaxVolumeSupported (10000 + MAX_INTERNAL_VOLUMES)

// ********* Below defines is used for flag value for services
// Zero is the default value of flag which indicates its not initialized.
#define SERVICE_NOT_INITIALIZED 0

// This flag will be set once service is initialized.
#define SERVICE_INITIALIZED (VFLT_MaxVolumeSupported + 1)
#define SERVICE__STOPPED (VFLT_MaxVolumeSupported + 2)
//***** service flag end....

// Each byte represent volume state of the particular volume. 
// Below is the bit index for the particular attribute within one byte.
// 0 - 3 bit for volume state on SPA.
// 4 - 7 bit for volume state on SPB.
enum FlareVolumeState {
    VolumeReadyBit_SPA = 0,
    VolumeDegradedBit_SPA = 1,
    VolumePathNotPreferredBit_SPA = 2,
    SPA_RESERVED2 = 3,
    VolumeReadyBit_SPB = 4,
    VolumeDegradedBit_SPB = 5,    
    VolumePathNotPreferredBit_SPB = 6,
    SPB_RESERVED2 = 7,    
};

enum VFLT_SPIdentity {
    VFLT_SPID_NOT_SPECIFIED = 0,
    VFLT_SPA,
    VFLT_SPB,
    VFLT_BothSP
};

// Forward declaration.
class VirtualFlareLogicalTable;

static VirtualFlareLogicalTable * sVolumeDataLogicalTable = NULL;

// It is a singleton class.
class VirtualFlareLogicalTable
{
public:    
    // Returns instance of the class. If it is not allocated then it will allocate and
    // return the instance.
    inline static VirtualFlareLogicalTable * GetInstance() {
        if(!sVolumeDataLogicalTable) {
            sVolumeDataLogicalTable = new VirtualFlareLogicalTable("VirtualFlareLogicalTableService", 
                                                        VFLT_MaxVolumeSupported);
        }

        return sVolumeDataLogicalTable;        
    }

    // Returns unique index to the logical table by using provided WWN.
    static ULONG GetUniqueVFLTIndex(K10_WWID key) {
        ULONG index = 0;
        ULONG64 node = *(__int64*)&key.node.bytes;
        switch(node) {
            case SYSTEM_VOLUME_NODE:                
                index = (ULONG)*(__int64*)&key.nPort.bytes;
#if defined(USE_CPP_FF_ASSERT)
                FF_ASSERT_NO_THIS(index < MAX_INTERNAL_VOLUMES);
#else // USE_CPP_FF_ASSERT
                FF_ASSERT(index < MAX_INTERNAL_VOLUMES);
#endif // USE_CPP_FF_ASSERT
                break;
            case USER_VOLUME_NODE:
                index = (ULONG)(MAX_INTERNAL_VOLUMES + *(__int64*)&key.nPort.bytes);
                break;
            default:
#if defined(USE_CPP_FF_ASSERT)
                FF_ASSERT_NO_THIS(0);
#else // USE_CPP_FF_ASSERT
                FF_ASSERT(0);
#endif // USE_CPP_FF_ASSERT
                break;
        }
        
        return index;
    }

    // Sets SPID based on the SP on which the code is running.
    void SetSPID(SPIdentity_t spIdentity) {
        mSpId = spIdentity;
        if(mSpId == SPA) {
            mMyShmemService = mSPANotifierService;
            mPeerShmemService = mSPBNotifierService;
        }
        else {
            mMyShmemService = mSPBNotifierService;
            mPeerShmemService = mSPANotifierService;
        }
    }

    // Returns notifier service for current SP.
    shmem_service * GetNotifierService() {
        FF_ASSERT(mMyShmemService);
        return mMyShmemService;
    }
    
    // Marks volume as Ready in shared memory segment. It acquires lock before updating the
    // shared memory.
    void MarkVolumeReady(K10_WWID key, VFLT_SPIdentity whichSp) {
        ChangeVolumeReadyState(SET, key, whichSp);
    }
    
    // Marks volume as NotReady in shared memory segment. It acquires lock before updating the
    // shared memory.
    void MarkVolumeNotReady(K10_WWID key, VFLT_SPIdentity whichSp) {
        ChangeVolumeReadyState(CLEAR, key, whichSp);    
    }

    // Marks volume as Degraded in shared memory segment. It acquires lock before updating the
    // shared memory.
    void MarkVolumeDegraded(K10_WWID key, VFLT_SPIdentity whichSp) {
        ChangeVolumeDegradedState(SET, key, whichSp);
    }

    // Marks volume as NotDegraded in shared memory segment. It acquires lock before updating the
    // shared memory.
    void MarkVolumeNotDegraded(K10_WWID key, VFLT_SPIdentity whichSp) {
        ChangeVolumeDegradedState(CLEAR, key, whichSp);
    }

    // Marks volume as path not preferred in shared memory segment. It acquires lock before updating the
    // shared memory.
    void MarkVolumePathNotPreferred(K10_WWID key, VFLT_SPIdentity whichSp) {
        ChangeVolumePathNotPreferredState(SET, key, whichSp);
    }

    // Marks volume as path preferred in shared memory segment. It acquires lock before updating the
    // shared memory.
    void MarkVolumePathPreferred(K10_WWID key, VFLT_SPIdentity whichSp) {
        ChangeVolumePathNotPreferredState(CLEAR, key, whichSp);
    }

    // It reads shared memory segment for the particular volume and returns true if volume has been 
    // marked as Ready in shared memory for the SP on which code is running.
    bool IsVolumeReady(K10_WWID key, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED) {
        char volFlag;
        char readyFlag = 0;
        VFLT_SPIdentity spId = whichSp;

        if(spId == VFLT_SPID_NOT_SPECIFIED) {
            spId = (mSpId == SPA) ? VFLT_SPA : VFLT_SPB;
        }
        
        if(spId == VFLT_SPA) {
            readyFlag = 1 << VolumeReadyBit_SPA;
        }
        else if(spId == VFLT_SPB) {
            readyFlag = 1 << VolumeReadyBit_SPB;
        }
        else if(spId == VFLT_BothSP) {
            readyFlag = 1 << VolumeReadyBit_SPA;
            readyFlag |= 1 << VolumeReadyBit_SPB;
        }
        else {
            FF_ASSERT(0);
        }

        AcquireSegmentLock();
        volFlag = GetVolumeFlagLockHeld(key);
        ReleaseSegmentLock();

        return ((volFlag  & (readyFlag)) != 0);        
    }

    // It reads shared memory segment for the particular volume and returns true if volume has been 
    // marked as Degraded in shared memory for the SP on which code is running.
    bool IsVolumeDegraded(K10_WWID key, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED) {
        char volFlag;
        char degradedFlag = 0;

        VFLT_SPIdentity spId = whichSp;

        if(spId == VFLT_SPID_NOT_SPECIFIED) {
            spId = (mSpId == SPA) ? VFLT_SPA : VFLT_SPB;
        }

        if(spId == VFLT_SPA) {
            degradedFlag = 1 << VolumeDegradedBit_SPA;
        }
        else if(spId == VFLT_SPB) {
            degradedFlag = 1 << VolumeDegradedBit_SPB;
        }
        else if(spId == VFLT_BothSP) {
            degradedFlag = 1 << VolumeDegradedBit_SPA;
            degradedFlag |= 1 << VolumeDegradedBit_SPB;
        }
         else {
            FF_ASSERT(0);
        }

        AcquireSegmentLock();
        volFlag = GetVolumeFlagLockHeld(key);
        ReleaseSegmentLock();

        return ((volFlag  & (degradedFlag)) != 0);        
    }

    // It reads shared memory segment for the particular volume and returns true if volume has been 
    // marked as Path not preferred in shared memory for the SP on which code is running.
    bool IsPathNotPreferred(K10_WWID key, VFLT_SPIdentity whichSp = VFLT_SPID_NOT_SPECIFIED) {
        char volFlag;
        char pathNotPreferredFlag = 0;

        VFLT_SPIdentity spId = whichSp;

        if(spId == VFLT_SPID_NOT_SPECIFIED) {
            spId = (mSpId == SPA) ? VFLT_SPA : VFLT_SPB;
        }

        if(spId == VFLT_SPA) {
            pathNotPreferredFlag = 1 << VolumePathNotPreferredBit_SPA;
        }
        else if(spId == VFLT_SPB) {
            pathNotPreferredFlag = 1 << VolumePathNotPreferredBit_SPB;
        }
        else if(spId == VFLT_BothSP) {
            pathNotPreferredFlag = 1 << VolumePathNotPreferredBit_SPA;
            pathNotPreferredFlag |= 1 << VolumePathNotPreferredBit_SPB;
        }
         else {
            FF_ASSERT(0);
        }

        AcquireSegmentLock();
        volFlag = GetVolumeFlagLockHeld(key);
        ReleaseSegmentLock();

        return ((volFlag  & (pathNotPreferredFlag)) != 0);
    }

    // It reads shared memory segment for the particular volume and returns true if volume has been
    // marked as Ready in shared memory for the peer SP.
    bool IsVolumeReadyOnPeer(K10_WWID key) {
        char volFlag;
        FlareVolumeState state;

        if(mSpId == SPA) {
            state = VolumeReadyBit_SPB;
        }
        else if(mSpId == SPB) {
            state = VolumeReadyBit_SPA;
        }
        else {
            FF_ASSERT(0);
        }

        AcquireSegmentLock();
        volFlag = GetVolumeFlagLockHeld(key);
        ReleaseSegmentLock();

        return ((volFlag  & (1 << state)) != 0);        
    }

    // It reads shared memory segment for the particular volume and returns true if volume has been 
    // marked as Degraded in shared memory for the peer SP.
    bool IsVolumeDegradedOnPeer(K10_WWID key) {
        char volFlag;
        FlareVolumeState state;

        if(mSpId == SPA) {
            state = VolumeDegradedBit_SPB;
        }
        else if(mSpId == SPB) {
            state = VolumeDegradedBit_SPA;
        }
        else {
            FF_ASSERT(0);
        }

        AcquireSegmentLock();
        volFlag = GetVolumeFlagLockHeld(key);
        ReleaseSegmentLock();

        return ((volFlag  & (1 << state)) != 0);        
    }    

    void Initialize() {
        // Mark all the volume Ready by default.
        for(UINT_32 i = 0; i < VFLT_MaxVolumeSupported; i++) {
            char flag = 1 << VolumeReadyBit_SPA;
            flag |= 1 << VolumeReadyBit_SPB;
            SetVolumeFlag(i, flag);
        }
    }

private:

    // It specify the bit operation.
    enum VolumeTableBitOperation {
        SET = 1,
        CLEAR = 2,
    };

    // Constructor
    VirtualFlareLogicalTable(const char* serviceName, 
                                                    UINT_32 size = (VFLT_MaxVolumeSupported * 2)) :
        mSize(size)
    {
        mSegment = new shmem_segment((char *)serviceName, mSize * 2);
        FF_ASSERT(mSegment);

        char uniqueServiceName[64];
        strncpy((char*)uniqueServiceName, serviceName, 64);
        strncat((char*)uniqueServiceName, "_SPA", 64);
        mSPANotifierService = new shmem_service(mSegment, (char*)uniqueServiceName, mSize);        
        FF_ASSERT(mSPANotifierService);

        bool serviceAlreadyInitialized = false;

        if(mSPANotifierService->get_flags() != SERVICE_NOT_INITIALIZED) {
            serviceAlreadyInitialized = true;
        }
        else {
            mSPANotifierService->set_flags(SERVICE_INITIALIZED);
        }

        strncpy((char*)uniqueServiceName, serviceName, 64);
        strncat((char*)uniqueServiceName, "_SPB", 64);        
        mSPBNotifierService = new shmem_service(mSegment, (char*)uniqueServiceName, mSize);   
        FF_ASSERT(mSPBNotifierService);

        if(mSPBNotifierService->get_flags() != SERVICE_NOT_INITIALIZED) {
            serviceAlreadyInitialized = true;
        }
        else {
            mSPBNotifierService->set_flags(SERVICE_INITIALIZED);
        }
      
        // We want to initialize logical table only if we have newly allocated it.
        if(!serviceAlreadyInitialized) {            
            Initialize();
            KvPrint("VirtualFlareLogicalTable initialized");
        }
        
        KvPrint("VirtualFlareLogicalTable constructor");
    }

    VirtualFlareLogicalTable() : mSize(0) 
    {}

    // Perform the operation on particular bit and notifies current SP and peer SP if requested.
    void ChangeVolumeReadyState(VolumeTableBitOperation   operation,
                                                                K10_WWID key, 
                                                                VFLT_SPIdentity whichSp) {
        char flag = 0;
        VFLT_SPIdentity spId = whichSp;

        if(spId == VFLT_SPID_NOT_SPECIFIED) {
            spId = VFLT_BothSP;
        }

        if(spId == VFLT_SPA) {
            flag = 1 << VolumeReadyBit_SPA;
        }
        else if(spId == VFLT_SPB) {
            flag = 1 << VolumeReadyBit_SPB;
        }
        else if(spId == VFLT_BothSP) {
            flag = 1 << VolumeReadyBit_SPA;
            flag |= 1 << VolumeReadyBit_SPB;
        }
        else{
            FF_ASSERT(0);
        }

        KvPrint("Changing Volume (%016llX:%016llX) state to %s on %s",
                (unsigned long long)(__int64)key.nPort,
        (unsigned long long)(__int64)key.node,
        (operation == SET) ? "READY" : "NOT READY",
        GetSpIdString(spId));

        if(operation == SET) {
            SetVolumeFlag(key, flag);
        }
        else if(operation == CLEAR) {
            ClearVolumeFlag(key, flag);            
        }
        else {
            FF_ASSERT(0);
        }

        // Notify SPs appropriately.
        NotifySp(GetUniqueVFLTIndex(key), spId);
    }

    // Perform the operation on particular bit and notifies current SP and peer SP if requested.
    void ChangeVolumeDegradedState(VolumeTableBitOperation operation, 
                                                                        K10_WWID key, 
                                                                        VFLT_SPIdentity whichSp) {
        char flag = 0;

        VFLT_SPIdentity spId = whichSp;

        if(spId == VFLT_SPID_NOT_SPECIFIED) {
            spId = VFLT_BothSP;
        }

        if(spId == VFLT_SPA) {
            flag = 1 << VolumeDegradedBit_SPA;
        }
        else if(spId == VFLT_SPB) {
            flag = 1 << VolumeDegradedBit_SPB;
        }
        else if(spId == VFLT_BothSP) {
            flag = 1 << VolumeDegradedBit_SPA;
            flag |= 1 << VolumeDegradedBit_SPB;
        }
         else {
            FF_ASSERT(0);
        }

        KvPrint("Changing Volume (%016llX:%016llX) state to %s on %s",
                (unsigned long long)(__int64)key.nPort,
        (unsigned long long)(__int64)key.node,
        (operation == SET) ? "DEGRADED" : "NOT DEGRADED",
        GetSpIdString(spId));

        if(operation == SET) {
            SetVolumeFlag(key, flag);
        }
        else if(operation == CLEAR) {
            ClearVolumeFlag(key, flag);            
        }
        else {
            FF_ASSERT(0);
        }

        // Notify SPs appropriately.
        NotifySp(GetUniqueVFLTIndex(key), spId);
    }    

    // Perform the operation on particular bit and notifies current SP and peer SP if requested.
    void ChangeVolumePathNotPreferredState(VolumeTableBitOperation operation,
                                                                        K10_WWID key,
                                                                        VFLT_SPIdentity whichSp) {
        char flag = 0;

        VFLT_SPIdentity spId = whichSp;

        if(spId == VFLT_SPID_NOT_SPECIFIED) {
            spId = VFLT_BothSP;
        }

        if(spId == VFLT_SPA) {
            flag = 1 << VolumePathNotPreferredBit_SPA;
        }
        else if(spId == VFLT_SPB) {
            flag = 1 << VolumePathNotPreferredBit_SPB;
        }
        else if(spId == VFLT_BothSP) {
            flag = 1 << VolumePathNotPreferredBit_SPA;
            flag |= 1 << VolumePathNotPreferredBit_SPB;
        }
         else {
            FF_ASSERT(0);
        }

        KvPrint("Changing Volume (%016llX:%016llX) state to %s on %s",
                (unsigned long long)(__int64)key.nPort,
        (unsigned long long)(__int64)key.node,
        (operation == SET) ? "PATH NOT PREFERRED" : "PATH PREFERRED",
        GetSpIdString(spId));

        if(operation == SET) {
            SetVolumeFlag(key, flag);
        }
        else if(operation == CLEAR) {
            ClearVolumeFlag(key, flag);
        }
        else {
            FF_ASSERT(0);
        }

        // Notify SPs appropriately.
        NotifySp(GetUniqueVFLTIndex(key), spId);
    }

    // Notify SPs about the volume state change based on the spId.
    void NotifySp(UINT_32 tableIndex, VFLT_SPIdentity spId) {
        if(spId == VFLT_BothSP) {
            // Notifying both the SPs about the state change.
            KvPrint("Notifying SPA");
            mSPANotifierService->set_flags(tableIndex);
            mSPANotifierService->notify();

            KvPrint("Notifying peer SPB");
            mSPBNotifierService->set_flags(tableIndex);
            mSPBNotifierService->notify();                             
        }
        else if(spId == VFLT_SPA) {
            KvPrint("Notifying SPA");
            mSPANotifierService->set_flags(tableIndex);
            mSPANotifierService->notify();
        }
        else {
            FF_ASSERT(spId == VFLT_SPB);
            KvPrint("Notifying SPB");
            mSPBNotifierService->set_flags(tableIndex);
            mSPBNotifierService->notify();                 
        }        
    }
public:    
    // Sets particular bit in shared memory for particular volume. It acquires segment lock before updating the 
    // shared memory.
    void SetVolumeFlag(K10_WWID key, char flag) {
        ULONG volumeIndex = GetUniqueVFLTIndex(key);
        SetVolumeFlag(volumeIndex, flag);
    }

    void SetVolumeFlag(ULONG volumeIndex, char flag) {
        char * basePtr = (char*) mSegment->get_base();

        AcquireSegmentLock();
        char curState;
        memcpy(&curState,(void*)((basePtr + volumeIndex)),sizeof(char));
        
        curState |= flag;
        memcpy((void*)((basePtr + volumeIndex)), &curState, sizeof(char));
        
        ReleaseSegmentLock();
    }
private:
    // Clears particular bit in shared memory for particular volume. It acquires segment lock before updating the 
    // shared memory.
    void ClearVolumeFlag(K10_WWID key, char flag) {
        ULONG volumeIndex = GetUniqueVFLTIndex(key);
        ClearVolumeFlag(volumeIndex, flag);
    }

    void ClearVolumeFlag(ULONG volumeIndex, char flag) {
        char * basePtr = (char*) mSegment->get_base();

        AcquireSegmentLock();
        char curState;
        memcpy(&curState,(void*)((basePtr + volumeIndex)),sizeof(char));
        
        FF_ASSERT(curState & (flag));
        curState &= ~(flag);
        memcpy((void*)((basePtr + volumeIndex)), &curState, sizeof(char));

        ReleaseSegmentLock();        
    }

    // Returns volume attributes for the particular volume.
    char GetVolumeFlagLockHeld(K10_WWID key) {
        char * basePtr = (char*) mSegment->get_base();

        ULONG volumeId = GetUniqueVFLTIndex(key);

        char curState;
        memcpy(&curState,(void*)((basePtr + volumeId)),sizeof(char));

        return curState;
    }

    // Retruns text string for SP identifier
    char * GetSpIdString(VFLT_SPIdentity sp) {
        switch (sp)
        {
            case VFLT_BothSP:
                return "both SPs";
            case VFLT_SPA:
                return "SPA";
            case VFLT_SPB:
                return "SPB";
            case VFLT_SPID_NOT_SPECIFIED:
                return "not specified";
            default:
                return "unknown";
        }
    }
    
    inline void AcquireSegmentLock() {
        mSegment->lock();
    }

    inline void ReleaseSegmentLock() {
        mSegment->unlock();
    }

    const char *mName;

    // Size of segment.
    const UINT_32 mSize;

    // Shared memory segment.
    shmem_segment *mSegment;

    // Service for SPA.
    shmem_service *mSPANotifierService;

    // Service for SPB.
    shmem_service *mSPBNotifierService;

    shmem_service * mMyShmemService;
    shmem_service * mPeerShmemService;

    // SP's identity.
    SPIdentity_t mSpId;
};

#endif // __VIRTUALFLARELOGICALTABLE_
