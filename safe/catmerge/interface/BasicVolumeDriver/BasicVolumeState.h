/************************************************************************
*
*  Copyright (c) 2002, 2005-2010 EMC Corporation.
*  All rights reserved.
*
*  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
*  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
*  BE KEPT STRICTLY CONFIDENTIAL.
*
*  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
*  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
*  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
*  MAY RESULT.
*
************************************************************************/
#ifndef __BasicVolumeState__
#define __BasicVolumeState__

//++
// File Name:
//      BasicVolumeState.h
//
// Description:
//  Declaration of the BasicVolumeState class.
//
// Revision History:
//
// 25-Feb-10: Dave Harvey -- Created
//--
#include "BasicVolume.h"
#include "VolumeEventSet.h"

// Contains a copy of the volume state, allowing operations on it.  The volume state consists of
// size, ready, along with other attributes, in addition to a VolumeEventSet, which tracks which
// parameters have been changed.
class BasicVolumeState  {

public:
    // Stores the parameters in the associated fields.
    BasicVolumeState(
        bool isReady = false,
        VOLUME_ATTRIBUTE_BITS volumeAttributes=0,
        ULONGLONG volumeSize = 0,
        TRESPASS_DISABLE_REASONS disableReasons = 0,
        ULONG bytesPerSector = 512,
        VolumeEventSet volumeEventSet = 0);

    // Import IOCTL_FLARE_GET_VOLUME_STATE structure, replacing current parameters with it.
    BasicVolumeState( const FLARE_VOLUME_STATE &  newerParams);

    // Changes the old parameters to the new, adding bits to VolumeEventSet 
    // based on which parameters changed.
    // Return: true - if any of the state changed, false - if the new is same as old
    bool ReplaceParameters( const BasicVolumeState &  newerParams);

    // Takes two sets of parameters and merges them, oring the volume attributes and trespass disable
    // reasons together.
    // @param other - the parameters to combine with this these.
    // @param sumSize - true means the resulting size is the sum of the two, if false, the minimum size is used.
    // @param eitherReady - true means ready is 'or' of both readies, false 'and'
    void CombineParameters( const BasicVolumeState &  other, bool sumSize, bool eitherReady);

    // Removes the specified events from mVolumeSetEvent.  Default is to remove all.
    // Returns the set of events which were actually cleared.
    VolumeEventSet ClearEvents(VolumeEventSet events = (VolumeEventSet)-1) {
        VolumeEventSet cleared = mVolumeEventSet & events;
        mVolumeEventSet ^= cleared;
        return cleared;
    }

    // Returns whether there are any pending events.
    VolumeEventSet GetEvents() const { return mVolumeEventSet;  }

    // Converts current volume state to FLARE_VOLUME_STATE.
    void GetFLARE_VOLUME_STATE(FLARE_VOLUME_STATE &  result) const;

    // Adds the specified events to the mVolumeSetEvent.
    VOID AddEvents(VolumeEventSet events = (VolumeEventSet)-1) {  mVolumeEventSet |= events; }

    // Or's in the specified attributes.
    VOID AddVolumeAttributes(VOLUME_ATTRIBUTE_BITS attrs) ;    
    
    // Replaces the attributes.
    VOID ReplaceVolumeAttributes(VOLUME_ATTRIBUTE_BITS attrs) ;

    // Changes the ready state.
    VOID SetReady(bool isReady);

    // Changes the size.
    VOID SetSize(ULONGLONG size);

    // Changes the Max Program Erase Cycle.
    VOID SetMaxPECycle(ULONGLONG maxPECycle);

    // Changes the current Program Erase Cycle.
    VOID SetCurrentPECycle(ULONGLONG currentPECycle);

    // Changes the Power On Hours.
    VOID SetTotalPowerOnHours(ULONGLONG totalPowerOnHours);

    // Changes the reasons
    VOID SetTrespassDisabledReasons(TRESPASS_DISABLE_REASONS   disableReasons);

    // Returns the current size.
    ULONGLONG GetCurrentVolumeSize() const { return mVolumeSize; }

    // Returns whether the device is ready
    bool IsReady() const { return mIsReadyState; }

    // Returns the Max Program Erase Cycle
    ULONGLONG GetMaxPECycle() const { return mMaxPECycle; }

    // Returns the Current Program Erase Cycle
    ULONGLONG GetCurrentPECycle() const { return mCurrentPECycle; }
    
    // Returns the Total Power On Hours
    ULONGLONG GetTotalPowerOnHours() const { return mTotalPowerOnHours; }
    
    // Returns the current volume attributes.
    VOLUME_ATTRIBUTE_BITS GetVolumeAttributes() const
    {
        return mVolumeAttributes;
    }

    TRESPASS_DISABLE_REASONS GetTrespassDisabledReasons() const { return mTrespassDisableReasons; }
private:
    VOLUME_ATTRIBUTE_BITS       mVolumeAttributes;
    bool                        mIsReadyState;
    ULONGLONG                   mVolumeSize;
    ULONGLONG                   mMaxPECycle;
    ULONGLONG                   mCurrentPECycle;
    ULONGLONG                   mTotalPowerOnHours;
    TRESPASS_DISABLE_REASONS    mTrespassDisableReasons;
    ULONG                       mBytesPerSector;

    VolumeEventSet              mVolumeEventSet;
};

inline BasicVolumeState::BasicVolumeState(
    bool isReady, VOLUME_ATTRIBUTE_BITS volumeAttributes, ULONGLONG volumeSize,
    TRESPASS_DISABLE_REASONS disableReasons, ULONG bytesPerSector, VolumeEventSet volumeEventSet)
    :     mVolumeAttributes(volumeAttributes), 
    mIsReadyState(isReady), 
    mVolumeSize(volumeSize), 
    mVolumeEventSet(volumeEventSet), mTrespassDisableReasons(disableReasons), mBytesPerSector(bytesPerSector) 
{
}

inline BasicVolumeState::BasicVolumeState( const FLARE_VOLUME_STATE &  newerParams) : 
mVolumeAttributes(newerParams.VolumeAttributes), 
mIsReadyState(!newerParams.NotReady), 
mVolumeSize(newerParams.capacity), 
mVolumeEventSet(0) , 
mTrespassDisableReasons(newerParams.TrespassDisableReasons),
mBytesPerSector(newerParams.geometry.BytesPerSector)
{
}
inline    void  BasicVolumeState::GetFLARE_VOLUME_STATE(FLARE_VOLUME_STATE &  result) const
{
    result.VolumeAttributes =  mVolumeAttributes;
    result.NotReady =  !mIsReadyState;
    result.capacity =  mVolumeSize;
    result.TrespassDisableReasons = mTrespassDisableReasons;
    result.geometry.BytesPerSector= mBytesPerSector;

    result.geometry.SectorsPerTrack = 1;
    result.geometry.TracksPerCylinder = 1;
    result.geometry.Cylinders.QuadPart = mVolumeSize;
    result.geometry.MediaType = FixedMedia;

}

inline VOID BasicVolumeState::AddVolumeAttributes(VOLUME_ATTRIBUTE_BITS attrs)
{  
    if ((~mVolumeAttributes) & attrs) {
        mVolumeAttributes |= attrs; 
        mVolumeEventSet |= VolumeEventAttributesChanged;
    }
}  

inline VOID BasicVolumeState::ReplaceVolumeAttributes(VOLUME_ATTRIBUTE_BITS attrs)
{  
    if (mVolumeAttributes != attrs) {
        mVolumeAttributes = attrs; 
        mVolumeEventSet |= VolumeEventAttributesChanged;
    }
}

inline VOID BasicVolumeState::SetReady(bool isReady) 
{ 
    if (mIsReadyState != isReady) {
        mIsReadyState = isReady; 
        mVolumeEventSet |= VolumeEventReadyStateChanged;
    }
}
inline VOID   BasicVolumeState::SetSize(ULONGLONG size)
{ 
    if (mVolumeSize != size) {
        mVolumeSize = size;
        mVolumeEventSet |= VolumeEventSizeChanged;
    }
}

inline VOID   BasicVolumeState::SetMaxPECycle(ULONGLONG maxPECycle)
{ 
    if (mMaxPECycle != maxPECycle) {
        mMaxPECycle = maxPECycle;
        mVolumeEventSet |= VolumeEventEraseInfoChanged;
    }
}

inline VOID   BasicVolumeState::SetCurrentPECycle(ULONGLONG currentPECycle)
{ 
    if (mCurrentPECycle != currentPECycle) {
        mCurrentPECycle = currentPECycle;
        mVolumeEventSet |= VolumeEventEraseInfoChanged;
    }
}

inline VOID   BasicVolumeState::SetTotalPowerOnHours(ULONGLONG totalPowerOnHours)
{ 
    if (mTotalPowerOnHours != totalPowerOnHours) {
        mTotalPowerOnHours = totalPowerOnHours;
        mVolumeEventSet |= VolumeEventEraseInfoChanged;
    }
}

inline VOID BasicVolumeState::SetTrespassDisabledReasons(TRESPASS_DISABLE_REASONS   disableReasons)
{

    if (mTrespassDisableReasons != disableReasons) {
        mTrespassDisableReasons = disableReasons;
        mVolumeEventSet |= VolumeEventTrespassDisableChanged;
    }
}

inline bool BasicVolumeState::ReplaceParameters( const BasicVolumeState &  other)
{
    bool changed = false;

    // Accumulate changes to the event mask.
    if (mVolumeEventSet != other.mVolumeEventSet) {
        mVolumeEventSet |= other.mVolumeEventSet;
        changed = true;
    }

    if(mVolumeSize != other.mVolumeSize)
    {
        mVolumeSize = other.mVolumeSize;
        mVolumeEventSet |= VolumeEventSizeChanged;
        changed = true;
    }

    if (mIsReadyState != other.mIsReadyState) {
        mIsReadyState = other.mIsReadyState;
        mVolumeEventSet |= VolumeEventReadyStateChanged;
        changed = true;
    }    
    if (mVolumeAttributes != other.mVolumeAttributes) {
        mVolumeAttributes = other.mVolumeAttributes;
        mVolumeEventSet |= VolumeEventAttributesChanged;
        changed = true;
    }    

    if (mTrespassDisableReasons != other.mTrespassDisableReasons) {
        mTrespassDisableReasons = other.mTrespassDisableReasons;
        mVolumeEventSet |= VolumeEventTrespassDisableChanged;
        changed = true;
    }

    if (mBytesPerSector != other.mBytesPerSector) {
        mBytesPerSector = other.mBytesPerSector;
        mVolumeEventSet |=VolumeEventParameterChange;
        changed = true;
    }

    return changed;
}

inline void BasicVolumeState::CombineParameters( const BasicVolumeState &  other, bool sumSize, bool eitherReady)
{
    // Accumulate changes to the event mask.
    mVolumeEventSet |= other.mVolumeEventSet;

    if(sumSize) { mVolumeSize +=  other.mVolumeSize; }
    else { mVolumeSize = ( mVolumeSize < other.mVolumeSize? mVolumeSize : other.mVolumeSize); }

    if (eitherReady) {
        mIsReadyState |= other.mIsReadyState;
    }
    else {
        mIsReadyState &= other.mIsReadyState;
    }

    mTrespassDisableReasons |= other.mTrespassDisableReasons;
    mVolumeAttributes |= other.mVolumeAttributes;
    mBytesPerSector = other.mBytesPerSector;
}

#endif //__BasicVolumeState__
