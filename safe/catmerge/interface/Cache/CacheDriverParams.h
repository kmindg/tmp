#ifndef __CacheDriverParams_h__
#define __CacheDriverParams_h__

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
//      Defines the CacheDriver Parameters passed into the cache from the admin
//      layer. 
//
// Revision History:
//--

#include "Cache/CacheDriverAdminParams.h"
#include "EmcPAL_Memory.h"
#include "Cache/ReplacementParams.h"

#define NUM_DRV_PARAM_SPARES (3)

//Timer threshold for High Priority BEIOReq from high priority volume -- default 105 seconds -- Unit is second -- 0: not used, -1: disable this timer checking
#define DEFAULT_HIGH1PRIORITYBEREQTIMEOUT (105) 

//Timer threshold for High Priority BEIOReq from NONE HIGH priority volume -- default 120 seconds -- Unit is second -- 0: not used, -1: disable this timer checking
#define DEFAULT_HIGH2PRIORITYBEREQTIMEOUT (120)

//Timer threshold for Low Priority BEIOReq -- default 180 seconds -- Unit is second -- 0: not used, -1: disable this timer checking
#define DEFAULT_LOWPRIORITYBEREQTIMEOUT (180) 

// Periodically timer to check all outstanding BEIOReqs’s processing time against above 3 priority time thresholds -- default 15s -- Unit is second
#define DEFAULT_SCANBEREQTIMERINTERVAL (15) 


class CacheDriverParams : public CacheDriverAdminParams {
public:
    // Default values 
    CacheDriverParams() { Initialize(); };
    ~CacheDriverParams() {};

    // Provide an initialize method which sets the default values.
    void Initialize()
    {
        mMccHigh1PriorityBEReqTimeOut = DEFAULT_HIGH1PRIORITYBEREQTIMEOUT;
        mMccHigh2PriorityBEReqTimeOut = DEFAULT_HIGH2PRIORITYBEREQTIMEOUT;
        mMccLowPriorityBEReqTimeOut = DEFAULT_LOWPRIORITYBEREQTIMEOUT;
        mMccScanBEReqTimerInterval = DEFAULT_SCANBEREQTIMERINTERVAL;
        mMaxDirtyAge = DEFAULT_MAX_DIRTY_AGE;
        mMinVGQueueDepth = 0;
        mEnableAllWritesAsLowPriorityIfWriteThrottled = false;
        
        EmcpalZeroMemory(&mSpares[0], sizeof(UINT_32) * NUM_DRV_PARAM_SPARES);
    }   

    SHORT GetHigh1PriorityBEReqTimeOut() const { return mMccHigh1PriorityBEReqTimeOut; }
    SHORT GetHigh2PriorityBEReqTimeOut() const { return mMccHigh2PriorityBEReqTimeOut; }
    SHORT GetLowPriorityBEReqTimeOut() const { return mMccLowPriorityBEReqTimeOut; }
    SHORT GetScanBEReqTimerInterval() const { return mMccScanBEReqTimerInterval; }
    
    // Get the Maximum dirty age. Return default is the current parameter is zero.
    EMCPAL_TIME_USECS GetMaximumDirtyAge() const {
        EMCPAL_TIME_USECS defaultMaxDirtyAge = DEFAULT_MAX_DIRTY_AGE;
        return mMaxDirtyAge ? mMaxDirtyAge : defaultMaxDirtyAge;
    }
    
    // Get the minimum VG queue depth
    ULONG GetMinVGQueueDepth() const { return mMinVGQueueDepth; }
    
    // Should writes be sent as low priority when write throttled?
    bool ShouldWritesBeLowPriorityIfWriteThrottled() const 
    { 
        return mEnableAllWritesAsLowPriorityIfWriteThrottled; 
    }
    
    // Set the minimum VG queue depth
    // @param value - Minimum number of credits a Volume Group can use.
    void SetMinVGQueueDepth(ULONG value) { mMinVGQueueDepth = value; }

    // set to new value to first type of high priority BE req timer, unit is sec.
    void setHigh1PriorityBEReqTimeOut(SHORT value)  { mMccHigh1PriorityBEReqTimeOut = value; }
    
    // set to new value to second type of high priority BE req timer, unit is sec.
    void setHigh2PriorityBEReqTimeOut(SHORT value)  {  mMccHigh2PriorityBEReqTimeOut = value; }
    
    // set to new value to Low priority BE req timer, unit is sec.
    void setLowPriorityBEReqTimeOut(SHORT value)  {  mMccLowPriorityBEReqTimeOut = value;}
    
    // set to new value to define the frequency to audit BEIO processing duration against above timers  , unit is sec.
    void setScanBEReqTimerInterval(SHORT value)  {  mMccScanBEReqTimerInterval = value; }
    
    // Set Maximum dirty age value
    // @param time - Max dirty age. If zero, use the default value.
    void SetMaximumDirtyAge(EMCPAL_TIME_USECS time) { 
        mMaxDirtyAge = (time == 0) ? DEFAULT_MAX_DIRTY_AGE : time; 
    }
    
    // Set whether writes should be low priority when write throttled.
    // @param value - true indicates writes should be low priority when write throttled.
    //                false otherwise.
    void SetAllWritesAsLowPriorityIfWriteThrottled(bool value) 
    {
        mEnableAllWritesAsLowPriorityIfWriteThrottled = value; 
    }

    // Copy constructor from buffer
    CacheDriverParams & operator= (const char * buffer)
    {
        CacheDriverParams *right = (CacheDriverParams *)buffer;

        *this = *right;

        return *this;
     };


    // Allow control over replacement tracks.
    ReplacementPoolParameters mReplacementPoolParams;
    
private:    
    // new parameters to make BEIOReq timer configurable, with this 8byte, sizeof(CacheDriverParams) equal to 512
    // the effective override sequence is parameter from PSM > from registry > default value
    // 0 value from PSM means value from registry will take effctive, if also 0 in registry, default value will take effective
    SHORT mMccHigh1PriorityBEReqTimeOut;  //0: not used, -1: disable this timer checking
    
    SHORT mMccHigh2PriorityBEReqTimeOut;  //0: not used, -1: disable this timer checking
    
    SHORT mMccLowPriorityBEReqTimeOut; //0: not used, -1: disable this timer checking
    
    SHORT mMccScanBEReqTimerInterval; // 0: use last corrected value (must be greater than 0)

    // Default maximum dirty age for a reference is 4 minutes.
    static const EMCPAL_TIME_USECS DEFAULT_MAX_DIRTY_AGE = (240 * CSX_USEC_PER_SEC);
    
    EMCPAL_TIME_USECS mMaxDirtyAge; // Maximum amount of time in usec that a reference can remain dirty (0 = default).
    
    UINT_32 mMinVGQueueDepth;       // Minimum Volume Group queue depth 
    
    // MCC currently disables enhanced queueing when write-throttled.  
    // If mEnableAllWritesAsLowPriorityIfWriteThrottled is true indicates enhanced queuing to be always on.
    bool mEnableAllWritesAsLowPriorityIfWriteThrottled;
    
    bool mPad[3];  // To keep the alignment correct on all platforms/compilers.
    
    UINT_32 mSpares[NUM_DRV_PARAM_SPARES];
};
// Note: now with above 5 timer addition, the Cache Driver record size is full of 512 byte (one sector)
// we must use spare field for future new parameter and need control size not cross 512. 


//
// SetOfActionsToPerform is a bitmap.  Multiple bits may be set and the
// driver will check for each one in turn, starting at the low-order bit.
//
struct CacheDriverAction : BasicDriverAction {
    enum
    {
        ActionClearStatistics = 1,
        ActionForceCacheLost = 2,     // Sets cache lost.
        ActionFreeCleanPages = 4,     // Frees unused pages in cache.
        ActionResetPrefetch = 8,      // Reset the prefetch manager.
        ActionSetToPanicWhileDumping = 16,      // Set SP to panic while dumping.
        
        CacheDriverAllActions = (ActionClearStatistics | ActionForceCacheLost | 
                ActionFreeCleanPages | ActionResetPrefetch | ActionSetToPanicWhileDumping)
    };

    // A bitmap of actions to perform, if the bit is set.
    ULONG   SetOfActionsToPerform;
};

#endif  // __CacheDriverParams_h__

