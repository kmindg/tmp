//**********************************************************************
//.++
// COMPANY CONFIDENTIAL:
//     Copyright (C) EMC Corporation, 2009
//     All rights reserved.
//     Licensed material - Property of EMC Corporation.
//.--
//**********************************************************************

//**********************************************************************
//.++
//  FILE NAME:
//      K10TierAdmin.h
//
//  DESCRIPTION:
//     This is the data structure and interface definition for Admin dll between
// the auto-tiering policy engine and MLU driver.
// We are currently planning to use a c++ style interface declaration and definition.
// It is different from more commonly used TLD based admin dll.
//
//  REVISION HISTORY:
//     1. inital draft by siyu zhou   6-24-2009
//.--
//**********************************************************************
//
//
//


#ifndef MLU_TIER_ADMIN_H
#define MLU_TIER_ADMIN_H

#include "k10defs.h"
#include "MluSupport.h"
#include "CaptivePointer.h"
#include "K10TraceMsg.h"
#include "K10MLUAdminExport.h"
#include "mluTypes.h"
#include "mluIoctls.h"
#include "cppetlSlist.h"
#include "K10MLUAutoTieringMessages.h"

#define TIERADMIN_API CSX_CLASS_EXPORT

//------------------const and enum type definitions---------------------------------------------
//typedef ULONG32             AUTO_TIERING_STATUS;
//#define NUMBER_OF_COARSE_TIERS 3;

//------------------Version number -----------------------------------------------------------------------

#define TIER_ADMIN_CURRENT_VERSION                      1

//----------------- Flags indicating the changes in the auto-tiering data-----------------
typedef ULONG64     CONFIG_CHANAGE_BIT;
#define ADMIN_CHANGED_NONE                              0x0
#define ADMIN_CHANGED_NUM_POOLS                         0X1
#define POOL_CHANGED                                    0x2
#define MLU_CHANGED                                     0x4
#define FLU_CHANGED                                     0x8
#define TIER_CHANGED                                    0x10
#define RAIDGROUP_CHANGED                               0X20

/*-------------------------------------------------------------------------------
//  Name:
//      PE_THROTTLE_ENUM
//
//  Description:
//      This enum defines different throttle values for a task in the Policy
//      Engine(PE). THROTTLE_NONE means throttle does not matter or not applicable.
//-------------------------------------------------------------------------------*/
typedef enum _AT_THROTTLE_ENUM
{
	AT_THROTTLE_NONE = 0,
	AT_THROTTLE_LOW ,
	AT_THROTTLE_MEDIUM ,
	AT_THROTTLE_HIGH
}AT_THROTTLE_ENUM;

typedef struct _SLICE_ID
{
    MLU_THIN_POOL_ID        poolOID;
    MLU_FLU_OBJECT_ID       fluOID;
    MLU_FS_ID               fsOID;
    ULONG64                 offset;
    ULONG64                 length;
} SLICE_ID;


typedef struct _TAKeyUid
{
	BYTE	uid[16];		// wwn id is 128 bit, 16 Byte
} TAKeyUid;


#define MAX_TAKEY_STRING_LENGTH  40

enum TIERADMIN_KEY_TYPE {
    TIERADMIN_KEY_TYPE_NONE,
    TIERADMIN_KEY_TYPE_OBJECT_ID,
    TIERADMIN_KEY_TYPE_WWN_ID
};

enum TIERADMIN_MLU_TYPE
{
    TIERADMIN_MLU_TYPE_INVALID = 0,
    TIERADMIN_MLU_TYPE_LU,
    TIERADMIN_MLU_TYPE_UFS
};

//
// both ufs and lun have the unique id accross the system.
// So generate the unique mlu key id in the following formula.
// mlutype << 32 |uniqtypeid
//
#define TA_UNIQ_MLU_KEY_ID(mlutype,uniqtypeid) (((mlutype+0ll) << 32) | (uniqtypeid))

//
// Specical Properties for LU and UFS
//
typedef union _TA_MLU_SPECIAL_PROPERTIES
{
    K10_WWID                mWWN;                   //lu wwn
    MLU_OBJECT_ID_EXTERNAL  mUFSOid;
}TA_MLU_SPECIAL_PROPERTIES,* PTA_MLU_SPECIAL_PROPERTIES;


class TIERADMIN_API TAKey
{
public:
    TAKey();
    TAKey(TIERADMIN_KEY_TYPE keyType,         // key type
        ULONG64            key_id);         // 64 bit object external id

    TAKey(TIERADMIN_KEY_TYPE keyType,         // key type
        K10_WWID            key_wwn);       // 128 bit wwwn id

    TAKey( const char *in_str);
    TAKey( const TAKey& key );

    BOOL operator ==(const TAKey &other) const;

    TIERADMIN_KEY_TYPE	GetKeyType() const	{ return mKeyType; }
    TAKeyUid            GetID() const { return mID;}
    const char *		ToString() const	{ return mStrBuf; }

private:
    TIERADMIN_KEY_TYPE	mKeyType;
    TAKeyUid            mID;
    char mStrBuf[MAX_TAKEY_STRING_LENGTH];
};

typedef double PERFORMANCE_ESTIMATE;

class AutoTieringAdminImpl;
//pool list implementation
template  <class T> class TierAdminListImpl
{
public:
    TierAdminListImpl( AutoTieringAdminImpl *pImpl );
    ~TierAdminListImpl( );

    // get the number of pools in the full pool list
    ULONG                   GetCount() const { return mlLocalList->queueLength(); }

    //find a pool in the full pool list with a key
    T*                      FindElement(TAKey &key ) const ;

    //get the pool at a index
    T*     operator[](ULONG idx)  const;

    void                    AddToList(T* pT);

    void                    FreeLists();

private:
    AutoTieringAdminImpl*   mpAdminImpl;
    ULONG                   mCount;       //number of elements in the list
    asetl::SList<T> 	    *mlLocalList; // the list
};

//----------- Auto tiering status codes----------------------------------------------------------------
//#define MLU_AUTO_TIERING_OK       0
//#define MLU_AUTO_TIERING_ERROR_END


//this is the class that implement most of work for auto-tiering admin.
//it will be hidden behind the interface definition to be more flexible.

class SlicePoolList;
class ProcessObjList;
class ProcessObj;
//---------------------------entry point---------------------------------------------
//AutoTieringAdmin class is the entry point of Auto tiering admin.
//It has a AutoTieringAdminImpl member which does every work befind the scene.
//It has a SlicePoolList member which represends all the thin pools in the system.
//First thing to do is to call refresh() to get thin pool data from kernel mlu driver.
//Next, call GetSlicePoolList() to get the thin pool list.
// ----------------------------------------------------------------------------------
class TIERADMIN_API 	AutoTieringAdmin
{
public:
    AutoTieringAdmin(ULONG version = TIER_ADMIN_CURRENT_VERSION);
    ~AutoTieringAdmin();

    // get new pool data from MLU driver.
    // deltaToken is the token used to identifiy what has changed since last poll.
    // if deltatoken == 0, it means a full poll which will return all the objects.
    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                    Poll(MLU_DELTA_TOKEN &deltaToken);

    // It will send enumerate process object IOCTL to mlu driver with process object
    // type set to SliceRelocation. This interfare is called frequently by policy
    // engine to find out if a slice relocation process is completed.
    // After this call, PE can get the process object list and deleted process object list
    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                    EnumerateProcessObjs();


    //return a list of pools which are changed since last poll
	//Poll must be called first
    //No exception
    const SlicePoolList*     GetDeltaPoolList() const ;

    //return a list of pools which are delted since last poll
    //Poll must be called first
    //No Exception
    const SlicePoolList*     GetDeletedPoolList() const ;

    //return a list of slice relocation process objects who complete the relocation
	// EnumerateProcessObjs must be called first
    // No Exception
    const ProcessObjList*   GetProcessObjList() const;

    //return a list of deleted slice relocation process objects
	// EnumerateProcessObjs must be called first
    // No Exception
    const ProcessObjList*   GetDeletedProcessObjList() const;

    // EnumerateProcessObjs must be called first
    // No Exception
    AutoTieringAdminImpl*   GetTierAdminImpl()              { return mpImpl;}

    // start slice relication to a destination tier.
    // slice: the slice needs to be relocated.
    // raidGroup: the destination raid group.
    // throttleValue: throttle enum.
    // tier: the destination tier with the performance estimate.
    // procObj: the relocation process Object ID returned to the caller.
    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                    StartSliceRelocation(SLICE_ID  slice,          /*slice to be relocated*/
                                             ULONG32   tgtRaidGroupID, /*target raid group*/
                                             AT_THROTTLE_ENUM     throttleValue, /*throttle*/
                                             /*OUT*/PMLU_START_SLICE_RELOCATION_OUT_BUF pProcObjID,
                                             ULONG64 option); /*relocation option see mlutype.h for detail*/

    // abort a single slice relocation process
    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                    AbortSliceRelocations(MLU_PROCESS_OBJECT_ID procObjID);

    // delete a slice relocation process object
    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                    DeleteSliceRelocationProcessObject(MLU_PROCESS_OBJECT_ID procObjID);


private:
    AutoTieringAdminImpl    *mpImpl;
    SlicePoolList           *mpDeltaPoolList;
    SlicePoolList           *mpDeletedPoolList;
    ProcessObjList          *mpProcessObjList;
    ProcessObjList          *mpDeletedProcessObjList;

    VOID                    UpdatePoolLists();
    VOID                    FreePoolLists();

    //since process object enumeration happens very frequently(every minute),
    // we need seperate interfaces
    VOID                    UpdateProcessObjLists();
    VOID                    FreeProcessObjLists();

};


//-------------------------thin pool list class-------------------------------------
//Thin pool list, it can get a pool from a index, find a pool from a its id.
//It also can tell if any member of it has changed.

class SlicePoolProperties;

//---------- pool properties classes-------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
class TIERADMIN_API SlicePoolProperties
{
    friend class SlicePoolImpl;
public:
    //constructor
    SlicePoolProperties( MLU_THIN_POOL_ID pool_id,
                        ULONG64          capacity,
                        ULONG64          freeSpace,
                        ULONG32          fluCount,
                        UCHAR            recoveryFlag,
                        EMCPAL_STATUS         status,
                        ULONG32          state,
                        BOOLEAN          statsOn,
                        BOOLEAN          sliceRelocationOn,
                        BOOLEAN          scheduledAutoTieringOn,
                        ULONG32          userVisibleID,
                        MLU_DELTA_TOKEN  deltaToken,
                        BOOLEAN          optimized);

    SlicePoolProperties(MLU_SLICE_POOL_PROPERTIES &buf );
    //for deleted pool object
    SlicePoolProperties(MLU_THIN_POOL_ID pool_id,
                       MLU_DELTA_TOKEN  deltaToken,
                       BOOLEAN fDeletedPool);
    //copy constructor
    SlicePoolProperties( const SlicePoolProperties& );

    //assignment operator
    SlicePoolProperties& operator=(const SlicePoolProperties&);

    //for update pool properties
    SlicePoolProperties(MLU_THIN_POOL_ID pool_id);

    SlicePoolProperties(){};
    //destructor
    ~SlicePoolProperties(){};

    //get/set stats accounting flag
    VOID                    SetStatsFlag(BOOLEAN bOn) {mStatsOn = bOn;}
    BOOLEAN                 GetStatsFlag() { return mStatsOn; }

    //get/set slice relocation flag
    VOID                    SetSliceRelocationFlag(BOOLEAN bEnabled) { mSliceRelocationOn = bEnabled; }
    BOOLEAN                 GetSliceRelocationFlag() { return mSliceRelocationOn; }

    //get/set shecule auto tiering flag
    VOID                    SetScheduledAutoTieringFlag(BOOLEAN bEnabled) { mScheduledAutoTieringOn = bEnabled; }
    BOOLEAN                 GetScheduledAutoTieringFlag() { return mScheduledAutoTieringOn; }

    MLU_THIN_POOL_ID        GetObjID () { return mObjID;}
    ULONG64                 GetCapacityInBlocks() { return mCapacity; }
    ULONG32                 GetFluCount() { return mFluCount; }
    UCHAR                   GetRecoveryFlag() { return mRecoveryFlag;}
    EMCPAL_STATUS           GetStatus() { return mStatus;}
    ULONG32                 GetPoolState() {return mState;}
    ULONG64                 GetFreeSpaceInBlocks() {return mFreeSpace;}
    ULONG32                 GetUserVisibleID() {return mUserVisibleID;}
    int                     GetStringLengh();
    VOID                    ToString(char* buffer, int lengh);
    VOID                    Update(SlicePoolProperties &prop);
    BOOLEAN                 GetOptimizedFlag() {return mOptimizedFlag;}


private:

private:
    MLU_THIN_POOL_ID        mObjID ;                //thin pool object ID
    ULONG64                 mCapacity;              //usable capacity of a pool in blocks
    ULONG64                 mFreeSpace;             //free space in a pool
    ULONG32                 mFluCount;              //flu counts
    UCHAR                   mRecoveryFlag;          //recovery flag
    EMCPAL_STATUS           mStatus;                //status indicating if pool is healthy enough to do slice relication
    ULONG32                 mState;                 //pool public state
    BOOLEAN                 mStatsOn;               //stats counting active flag. default value true
    BOOLEAN                 mSliceRelocationOn;     //slice relocation permit flag. default value true.
    BOOLEAN                 mScheduledAutoTieringOn;//scheduled auto tiering flag. user controllable.
    MLU_DELTA_TOKEN         mDeltaToken;            //thin pool delta token
    BOOLEAN                 mDeletedPool;           // indicates if this pool is in the deleted pool list.
    ULONG32                 mUserVisibleID;            //pool ID used by the user (Unisphere)
    BOOLEAN                 mOptimizedFlag;         // indicates if this pool is optimized
};

//------------ Thin pool class-----------------------------------------------------------
// A thin pool includes a bunch of FLUs, MLUs, Tiers ans Slices.
// Refresh function of thin pool will issue several ioctrls to MLU drivers to enumerate all the
// MLUs, FLUs, Tiers, Slices and fill up the their lists.
//
//---------------------------------------------------------------------------------------
class SlicePoolImpl;
class TierList;
class FLUList;
class MLUList;
class SliceList;
class Slice;
class Tier;
class SliceIOStats;
class RaidGroupList;
class RaidGroup;

class TIERADMIN_API SlicePool
{
    friend class AutoTieringAdmin;
    //friend class SlicePoolListImpl;
    friend class SlicePoolImpl;

public:
    SlicePool(AutoTieringAdminImpl   *adminImpl,
             TAKey                  &key,
             SlicePoolProperties     &prop);

     ~SlicePool();

     const      TierList& GetTierList() const           { return *mpTierList; }

     const      RaidGroupList& GetRaidGroupList() const       { return *mpRaidGroupList; }
     // get a list of flus
     const      FLUList& GetFLUList() const          { return *mpFLUList; }


     const      MLUList& GetMLUList() const             { return *mpMLUList; }

     SlicePoolProperties &GetProperties() ;

     // thin  poll  from pool object will
     // 1. MLU_GET_THIN_POOL_PROPERTIES to get thin pool properties.
     // 2. MLU_ENUMERATE_THIN_LUS to enumerate MLUs.
     // 3. Get Tier inforation from tier library.
     // 4. Build the MLU lists, Raid group lists.
     // Exception: it might throw K10Exception, caller must have exception handling.
     VOID               Poll(MLU_DELTA_TOKEN &deltaToken);

     // 1. MLU_GET_THIN_POOL_PROPERTIES to get thin pool properties.
     // Updates its properties with new data
     // Exception: it might throw K10Exception, caller must have exception handling.
     VOID               UpdatePropertiesFromDriver();

     TAKey              GetKey() const;

     SlicePoolImpl       *GetSlicePoolImpl() { return mpImpl;}

     // modify the stats flag.
     // must call commitChanges to send update to driver
     VOID               SetStatsFlag(BOOLEAN bOn);
     //must call commitChanges to send update to driver
     VOID               SetSliceRelocationFlag(BOOLEAN bEnabled);
     //must call commitChanges to send update to driver
     VOID               SetScheduledAutoTieringFlag(BOOLEAN bEnabled);

     //1. IOCTL_MLU_SET_THIN_POOL_PROPERTIES to set thin pool properties.
     //Exception: it might throw K10Exception, caller must have exception handling.
     VOID               CommitChanges();


     CONFIG_CHANAGE_BIT GetChangedParams() const                { return mChangedParams; }

private:
     SlicePoolImpl       *mpImpl;
     TierList           *mpTierList;                // predefined user visible coarse tier
     RaidGroupList      *mpRaidGroupList;           // raid group list.
     FLUList            *mpFLUList;                 // flu list
     MLUList            *mpMLUList;                 // mlu list

     //create/delete all the lists.
     VOID               UpdateLists();
     VOID               FreeLists();

     CONFIG_CHANAGE_BIT       mChangedParams;
    // CONFIG_CHANAGE_STATUS       mUncommittedParams;
};

class TIERADMIN_API SlicePoolList
{
public:

   SlicePoolList(TierAdminListImpl <SlicePool> *pImpl);

   ULONG        GetCount() const;
   SlicePool     &operator[](ULONG) const;

   SlicePool     * GetSlicePool(ULONG idx) const;

   SlicePool     * FindSlicePool(TAKey &key) const;

private:
   TierAdminListImpl <SlicePool>    *mpImpl;

};
//----------------------------------------------------------------------------------------------

//----------------------------ProcessObjProperties----------------------------------

class TIERADMIN_API ProcessObjProperties
{
public:
    //constructor for existing process object
    ProcessObjProperties(MLU_PROCESS_OBJECT_PROPERTIES& prop);
    //constructor for deleted process object
    ProcessObjProperties(MLU_DELETED_PROCESS_OBJECT& prop);

    ~ProcessObjProperties(){};
public:
    MLU_PROCESS_OBJECT_ID     GetProcessObjID() { return mProcessObjectId;}
    EMCPAL_STATUS             GetStatus() { return mProcessStatus;}
    MLU_DELTA_TOKEN           GetDeltaToken() { return mDeltaToken;}
    ULONG32                   GetProcessObjState() {return mProcessState;}
    ULONG32                   GetProcessObjType()   {return mProcessOperationType;}
    int                       GetStringLengh();
    EMCPAL_STATUS             GetFailedStatus() { return mFailedStatus;}
    VOID                      ToString(char* buffer, int lengh);

    VOID                      SetStatus(EMCPAL_STATUS status){ mProcessStatus = status;}
    VOID                      SetFailedStatus(EMCPAL_STATUS status){ mFailedStatus = status;}

	// only apply for Relocation Process
	MLU_SLICE_RELOCATION_SLICE_ID	GetDestinationSlice() { return mDestinationSlice; }
private:
    //process object ID
    MLU_PROCESS_OBJECT_ID     mProcessObjectId;
    //process object delta token
    MLU_DELTA_TOKEN           mDeltaToken;
    //process object status. for deleted process object, the value is S_OK
    EMCPAL_STATUS             mProcessStatus;
    ULONG32                   mProcessState;
    ULONG32                   mProcessOperationType;
	// this is specific to relocation process
	MLU_SLICE_RELOCATION_SLICE_ID mDestinationSlice;
    //if the state is aborted. the status/reason of why it failed.
    EMCPAL_STATUS             mFailedStatus;
};
//--------------------------Process Object------------------------------------------
class ProcessObjImpl;
class TIERADMIN_API ProcessObj
{
public:
    ProcessObj(AutoTieringAdminImpl *pImpl,
               TAKey &key,
               ProcessObjProperties &prop);

    ~ProcessObj();

    ProcessObjProperties    &GetProperties() ;
    TAKey                   GetKey() const;

private:
    ProcessObjImpl          *mpImpl;
};
//-----------------------Process Object list----------------------------------------

class TIERADMIN_API ProcessObjList
{
public:

   ProcessObjList(TierAdminListImpl <ProcessObj> *pImpl);

   ULONG            GetCount() const;
   ProcessObj       &operator[](ULONG) const;

   ProcessObj       *GetProcessObj(ULONG idx) const;

   ProcessObj       *FindProcessObj(TAKey &key) const;

private:
   TierAdminListImpl <ProcessObj>    *mpImpl;

};

//-------------------------MLU list class------------------------------------
class MLU;

class TIERADMIN_API MLUList
{

public:

   MLUList(TierAdminListImpl <MLU> *pImpl);

   ULONG            GetCount() const;
   MLU              &operator[](ULONG ) const;

   MLU              *GetMLU(ULONG idx) const;

   MLU              *FindMLU(TAKey &key) const;

   VOID             Update( );

private:
    TierAdminListImpl <MLU>    *mpImpl;

};

//-------------------------MLU properties-------------------------------------
class TIERADMIN_API MLUProperties
{
public:
    //constructor
    MLUProperties(MLU_THIN_POOL_ID        poolID,
                  ULONG32                 lunID,
                  MLU_FS_ID               fileSystemOID,
                  ULONG64                 capacity,
                  EMCPAL_STATUS           status,
                  MLU_TIER_PREFERENCE     tierPreference,
                  MLU_SLICE_RELOCATION_POLICY relocationPolicy,
                  MLU_DELTA_TOKEN         deltaToken,
                  TIERADMIN_MLU_TYPE      type,
                  MLU_LU_ENUM_TYPE        lunType,
                  TA_MLU_SPECIAL_PROPERTIES specicalProperty);

    MLUProperties (MLU_GET_LU_PROPERTIES_OUT_BUF &buf);

    MLUProperties (MLU_UFS_PROPERTIES &buf);

    MLUProperties(){};

    //copy constructor
    MLUProperties( const MLUProperties& );

    //assignment operator
    MLUProperties& operator=(const MLUProperties&);
    //destructor

    ~MLUProperties(){};

    //get/set flag

    //
    // Common functions for both LUN and UFS
    //
    MLU_THIN_POOL_ID        GetPoolID() { return mPoolID; } //id of pool where this MLU is lcoated.
    MLU_FS_ID               GetFSID() { return mFileSystemOID; }         //file system object ID
    ULONG64                 GetCapacity() { return mCapacity; }              //Capacity in blocks
    EMCPAL_STATUS           GetStatus() { return mStatus; }               //MLU current status
    MLU_TIER_PREFERENCE     GetTierPreference() { return mTierPreference; }        // lun tier preference(high, normal, low)
    MLU_SLICE_RELOCATION_POLICY     GetRelocationPolicy() { return mRelocationPolicy; }       // auto tiering preferencestate
    MLU_DELTA_TOKEN         GetDeltaToken() { return mDeltaToken; }
    ULONG32                 GetMluID() { return mMluID; }
    TIERADMIN_MLU_TYPE      GetType(){ return mType;}
    MLU_LU_ENUM_TYPE        GetLunType(){ return mLunType; }

    VOID                    SetTieringPreference(MLU_TIER_PREFERENCE  preference) { mTierPreference = preference; }
    VOID                    SetRelocationPolicy(MLU_SLICE_RELOCATION_POLICY policy) { mRelocationPolicy = policy; }

    VOID                    ToString(char* buffer, int length);
    int                     GetStringLengh();
    VOID                    Update(MLUProperties &prop);

    //
    // Functions only can be called for UFS
    //
    MLU_OBJECT_ID_EXTERNAL GetUFSOID(){
        return mSpecicalProperty.mUFSOid;}

   //
   // Functions only can be called for LUN
   //
   K10_WWID                GetWWN() {
       return mSpecicalProperty.mWWN; }

private:


private:
    //
    // Common Properties for LU and UFS
    //
    MLU_THIN_POOL_ID        mPoolID;                //id of pool where this MLU is lcoated.
    MLU_FS_ID               mFileSystemOID;         //file system object ID
    ULONG64                 mCapacity;              //Capacity in blocks
    EMCPAL_STATUS           mStatus;                //MLU current status                                           //
    MLU_TIER_PREFERENCE     mTierPreference;        // lun tier preference(high, normal, low)
    MLU_SLICE_RELOCATION_POLICY     mRelocationPolicy;      // auto tiering preferencestate
                                                    // (off, preference-biased, optimized).
    MLU_DELTA_TOKEN         mDeltaToken;
    ULONG32                 mMluID;                 // Mapped LUN ID or UFS ID
    TIERADMIN_MLU_TYPE      mType;
    MLU_LU_ENUM_TYPE        mLunType;               // LUN Type (the field is valid for non-UFS)

    //
    // Specical Properties for LU and UFS
    //
    TA_MLU_SPECIAL_PROPERTIES mSpecicalProperty;
};

//-------------------------MLU class-----------------------------------------
class MLUImpl;

class TIERADMIN_API MLU
{

    friend class MLUListImpl;
    friend class SlicePoolImpl;

public:
    MLU (AutoTieringAdminImpl *pImpl,
         TAKey &key,
         MLUProperties &prop);

    MLU( MLU& mlu );

    ~MLU();

    MLUProperties       &GetProperties() ;

    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                UpdatePropertiesFromDriver();

    VOID                SetTieringPreference(MLU_TIER_PREFERENCE  preference) ;
    VOID                SetRelocationPolicy(MLU_SLICE_RELOCATION_POLICY policy);

    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID                CommitChanges();

    TAKey               GetKey() const;

    CONFIG_CHANAGE_BIT  GetChangedParams() const                { return mChangedParams; }

private:
    MLUImpl             *mpImpl;
    CONFIG_CHANAGE_BIT  mChangedParams;
};

//-------------------------------FLU list class-----------------------------------
//
class FLU;
class FLUProperties;

class TIERADMIN_API FLUList
{

public:

    FLUList(TierAdminListImpl <FLU> *mpImpl);

    ULONG           GetCount() const;
    FLU &           operator[](ULONG) const;

    FLU             *GetFLU(ULONG idx) const;

    FLU             *FindFLU(TAKey &key) const;

    VOID            AddToList(FLU*) const;

    VOID            Update( );

private:
    TierAdminListImpl <FLU>     *mpImpl;

};

//-------------------------------FLU properties-----------------------------------
//
//typedef double TIER_DESCRIPTOR;
;

class TIERADMIN_API FLUProperties
{
public:
    //constructor
    FLUProperties(MLU_FLU_OBJECT_ID             objID,
                  ULONG32                       raidGroupID,
                  MLU_THIN_POOL_ID              poolID,
                  K10_WWID                      WWN,
                  FLARE_DRIVE_TYPE              driveType,
                  ULONG64                       sliceCount,
                  TIER_DESCRIPTOR               tierDescriptor,
                  INT                           coarseTierIndex,
                  PERFORMANCE_ESTIMATE          perfEstimate,
                  EMCPAL_STATUS                 status,
                  ULONG64                       maxPECycle,
                  ULONG64                       currentPECycle,
                  ULONG64                       totalPowerOnHours);

    FLUProperties(MLU_FLU_OBJECT_ID             objID,
                  MLU_THIN_POOL_ID              poolID);

    FLUProperties(){};

    FLUProperties(const FLUProperties&);
    FLUProperties& operator=(const FLUProperties&);
    VOID Update(const FLUProperties&);

    //destructor
    ~FLUProperties(){};

    //get/set methods

    TIER_DESCRIPTOR             GetTierDescriptor() const { return mTierDescriptor ;}
    MLU_FLU_OBJECT_ID           GetObjID() const {return mObjID;}
    ULONG32                     GetRaidGroupID() const {return mRaidGroupID;}
    MLU_THIN_POOL_ID            GetPoolID() const { return mPoolID;}
    K10_WWID                    GetWWN() const { return mWWN;}
    FLARE_DRIVE_TYPE            GetDriveType() const { return mDriveType; }
    ULONG64                     GetSliceCount() const {return mSliceCount;}
    INT                         GetTierIndex() const { return mCoarseTierIndex;}
    PERFORMANCE_ESTIMATE        GetPerformanceEstimate() const { return mPerfEstimate;}
    EMCPAL_STATUS               GetStatus() const { return mStatus;}
    ULONG64                     GetBaseOffset() const {return mBaseOffset;}
    ULONG64                     GetHeaderSize() const {return mHeaderSize;}
    ULONG64                     GetDataSize() const {return mDataSize;}
    MLU_TIMESTAMP               GetResetTime() const {return mResetTime;}
    ULONG64                     GetMaxPECycle() const {return mMaxPECycle;}
    ULONG64                     GetCurrentPECycle() const {return mCurrentPECycle;}
    ULONG64                     GetTotalPowerOnHours() const {return mTotalPowerOnHours;}

    VOID                        SetTierDescriptor(TIER_DESCRIPTOR descriptro) { mTierDescriptor = descriptro;}
    VOID                        SetSliceCount(ULONG64 num) { mSliceCount = num;}

    VOID                        SetBaseOffset(ULONG64   offset) { mBaseOffset = offset;}
    VOID                        SetHeaderSize(ULONG64   headerSize) { mHeaderSize =  headerSize;}
    VOID                        SetDataSize(ULONG64     dataSize) { mDataSize =  dataSize;}
    VOID                        SetDriveType(FLARE_DRIVE_TYPE driveType) { mDriveType = driveType; }
    VOID                        SetResetTime(MLU_TIMESTAMP rt) { mResetTime = rt;}
    VOID                        SetMaxPECycle(ULONG64 maxPECycle) { mMaxPECycle = maxPECycle; }
    VOID                        SetCurrentPECycle(ULONG64 currentPECycle) { mCurrentPECycle = currentPECycle; }
    VOID                        SetTotalPowerOnHours(ULONG64 totalPowerOnHours) { mTotalPowerOnHours = totalPowerOnHours; }

    VOID                        ToString(char* outStr, int length);
    int                         GetStringLengh();
private:
    MLU_FLU_OBJECT_ID           mObjID;                 //its object ID
    ULONG32                     mRaidGroupID;           //raid group ID
    MLU_THIN_POOL_ID            mPoolID;                //id of pool where this flu is lcoated.
    K10_WWID                    mWWN;                   //flu wwn
    ULONG64                     mSliceCount;            //total number of slices in this flu
    // flu tier descriptor.
    // use macro COASE_TIER_INDEX_FROM_TIER_DESCRIPTOR(tier_descriptor) to get coase tier index
    // use macro PERFORMANCE_ESTIMATE_FROM _TIER_DESCRIPTOR(tier_descriptor) to get performance estimate.
    //
    TIER_DESCRIPTOR             mTierDescriptor;
    INT                         mCoarseTierIndex;
    PERFORMANCE_ESTIMATE        mPerfEstimate;
    EMCPAL_STATUS               mStatus;

    MLU_TIMESTAMP               mResetTime;

    //AR367586. TierAdmin will parse the slice header information for policy engine
    ULONG64                     mBaseOffset;
    ULONG64                     mHeaderSize;
    ULONG64                     mDataSize;
    FLARE_DRIVE_TYPE            mDriveType;

    //FLASHWARE
    ULONG64                     mMaxPECycle;
    ULONG64                     mCurrentPECycle;
    ULONG64                     mTotalPowerOnHours;
};

//------------------------------------Flu IO stats--------------------------------
typedef struct TIERADMIN_API _FluIOStats
{
    MLU_THIN_POOL_ID                mPoolID;
    MLU_FLU_OBJECT_ID               mFLUID;
    MLU_GET_SLICE_INFO_FOR_FLU_OUT_BUF_REV3 mSliceInfo;
} FluIOStats, *PFluIOStats;

//-------------------------------SIEnumerator class----------------------------------------
//
class TIERADMIN_API SIEnumerator
{
public:
  virtual ~SIEnumerator() {};

  virtual UINT64 getBufferSize(const UINT32 num_entries) const = 0;
  virtual UINT32 enumerate(const UINT32 num_entries, PMLU_SLICE_INFO_ENUMERATE_OUT_BUF pOutEnumBuf) = 0;
  virtual MLU_TIMESTAMP getTimestamp() const = 0;
};

//-------------------------------FLU class----------------------------------------
//
class FLUListImpl;
class FLUImpl;

class TIERADMIN_API FLU
{

    friend class FLUListImpl;
    friend class RaidGroupImpl;
    friend class SlicePoolImpl;

public:
    FLU ( AutoTieringAdminImpl *mImpl,
          TAKey &key,
          FLUProperties &prop);
    //this constructor is for PE use only.
    //after constructing a object with it, client can call update properties or collect stats.
    FLU (MLU_THIN_POOL_ID poolID, MLU_FLU_OBJECT_ID fluID, ULONG64 fluSliceCount);

    ~FLU();

    // this method must be called to initialize the slice number.
    // it will issue IOCTL_MLU_GET_SLICE_INFO_FOR_FLU with an empty SLICE_INFO array
    // MLU driver will return the slict count back.
    // Exception: it might throw K10Exception, caller must have exception handling.
    ULONG64            GetSliceCount();

    // sliceCount * sizeof(SLICE_INFO) + size of IOCTLS other fields
    // caller must allocate data buffer in the FluIOStats
    ULONG64            GetIOStatsLen() ;

    // collect IO stats for this FLU.
    // This method will issue IOCTRL IOCTL_MLU_GET_SLICE_INFO_FOR_FLU
    // to get io stats for all the slices in this FLU.
    // caller allocates the databuffer
    // Exception: it might throw K10Exception, caller must have exception handling.
    VOID               CollectIOStats(PFluIOStats, ULONG64 lengh);

    FLUProperties      &GetProperties();
    TAKey              GetKey() const;

    VOID AcquireSIEnumerator(SIEnumerator** pEnumerator);

    VOID ReleaseSIEnumerator(SIEnumerator* pEnumerator);

    // AR367568, Retrieve the slice header information for this flu, sending IOCTL to mlu driver and updating
    // FLU slice information. Policy engine needs the baseOffset etc.
    // It might fail and generate exception
    VOID               GetFluSliceInfo();
private:
    FLUImpl            *mpImpl;

};





//----------------------------------raid group class---------------------------------
// raid group properties class is very simple, it only has a id member.
// ----------------------------------------------------------------------------------
class TIERADMIN_API RaidGroupProperties
{
public:
    RaidGroupProperties(ULONG32 rgID,
                        MLU_THIN_POOL_ID poolID)
        :mID(rgID),  mPoolID(poolID), mSliceCount(0), mIOPSEstimate(0)
        {};
    RaidGroupProperties(){};
    ~RaidGroupProperties(){};

    RaidGroupProperties(const RaidGroupProperties&);
    RaidGroupProperties& operator=(const RaidGroupProperties&);
    VOID Update(const RaidGroupProperties& prop);

    VOID                SetSliceCount(ULONG64 sliceCount);
    VOID                SetIOPSEstimate(double estimate);

    ULONG32             GetRaidGroupID() { return mID;}
    MLU_THIN_POOL_ID    GetPoolID() { return mPoolID;}
    ULONG64             GetSliceCount() { return mSliceCount;}
    double              GetIOPSEstimate() {return mIOPSEstimate;}

    VOID                ToString(char* outStr, int length);
    int                 GetStringLengh();

private:
    ULONG32             mID;                            //Raid group ID
    MLU_THIN_POOL_ID    mPoolID;
    ULONG64             mSliceCount;                    //number of slices in this raid group
    double              mIOPSEstimate;                  //raid group IOPS estimate, we get it from TDL.
};

class RaidGroupImpl;
class TIERADMIN_API RaidGroup
{
    friend class RaidGroupListImpl;
    friend class SlicePoolImpl;
public:
    RaidGroup(AutoTieringAdminImpl *mpImpl,
        //RaidGroupListImpl &List,
        TAKey &key,
        RaidGroupProperties &prop);

    ~RaidGroup();

    RaidGroupProperties     &GetProperties() ;

    TAKey                   GetKey() const;

    DWORD                   AddFLU(const FLUProperties &FluProp);

private:
    RaidGroupImpl           *mpImpl;
};

class RaidGroupListImpl;
class TIERADMIN_API RaidGroupList
{

public:

    RaidGroupList(TierAdminListImpl<RaidGroup> *mpImpl);

    ULONG                   GetCount() const;
    RaidGroup &             operator[](ULONG) const;

    RaidGroup               *GetRaidGroup(ULONG idx) const;

    RaidGroup               *FindRaidGroup(TAKey &key) const;


    VOID                    Update( );

private:
    TierAdminListImpl<RaidGroup>       *mpImpl;

};


//----------------------------------tier list class----------------------------------
class TierListImpl;
class TierProperties;

class TIERADMIN_API TierList
{

public:

   TierList(TierAdminListImpl<Tier> *mpImpl);

   ULONG            GetCount() const;
   Tier             &operator[](ULONG) const;

   Tier             *GetTier(ULONG idx) const;

   Tier             *FindTier(TAKey &key) const;
private:
    TierAdminListImpl<Tier>    *mpImpl;

};
//----------------------------------tier class---------------------------------------
//
class TierImpl;

class TIERADMIN_API Tier
{

    friend class TierListImpl;
    friend class SlicePoolImpl;

public:
    Tier(AutoTieringAdminImpl *mpImpl,
        TAKey &key,
        TierProperties &prop);

    ~Tier();

    TierProperties      &GetProperties() ;

    TAKey               GetKey() const;

    VOID                AddSlice(ULONG64 sliceNum);

private:
    TierImpl            *mpImpl;
};

//----------------------------------tier properties ----------------------------------
//


//coarse tiering object
class TIERADMIN_API TierProperties
{
public:
    //constructor
    TierProperties(MLU_THIN_POOL_ID poolID,
                   TIER_DESCRIPTOR descriptor);

    TierProperties() { };
    //destructor
    ~TierProperties(){};

    //get/set methods
    TIER_DESCRIPTOR         GetDesciptor() { return mDescriptor;}
    int                     GetCoarseTierIndex() { return mCoaseTierIndex;}
    MLU_THIN_POOL_ID        GetPoolID() { return mPoolID;}
    PERFORMANCE_ESTIMATE    GetPerformanceEstimate() { return mPerformance;}
    ULONG64                 GetTotalSliceNum()  { return mSliceNum;}

    VOID                    ToString(char* outStr, int length);
    int                     GetStringLengh();

    VOID                    AddSlice(ULONG64);
    VOID 					Update(TierProperties &prop)
    {
    	 mDescriptor = prop.mDescriptor;
    	 mCoaseTierIndex = prop.mCoaseTierIndex;
    	 mPoolID = prop.mPoolID;
    	 mSliceNum = prop.mSliceNum;
    	 mPerformance = prop.mPerformance;
    }
    TierProperties& operator=(const TierProperties& prop)
    {
      	 mDescriptor = prop.mDescriptor;
       	 mCoaseTierIndex = prop.mCoaseTierIndex;
       	 mPoolID = prop.mPoolID;
       	 mSliceNum = prop.mSliceNum;
       	 mPerformance = prop.mPerformance;
         return *this;
    }

private:
    TIER_DESCRIPTOR         mDescriptor;            //tier descriptor .
    int                     mCoaseTierIndex;        //coase tier index, get it from a macro
    MLU_THIN_POOL_ID        mPoolID;                //id of pool where this coarse tier is lcoated.
    ULONG64                 mSliceNum;              //total number of slices in this tier
    PERFORMANCE_ESTIMATE    mPerformance;           //performance estimate, compute from a macro.
};

#endif

