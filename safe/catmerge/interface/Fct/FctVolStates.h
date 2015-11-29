#ifndef __FctVolStates_h__
#define __FctVolStates_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      FctVolStates.h
//
// Contents:
//
// Revision History:
//--
#include "BasicVolumeDriver/BasicVolumeParams.h"

#define FCT_SYSTEM_LU_NUMBER            8193

#define FCT_FULL_STATE_VOLOUME_PRESENT  0x80
#define FCT_FULL_STATE_PERSISTED        0x40

enum EFDC_VOL_STATE
{
    EFDC_VOL_STATE_DISABLED = 0,
    EFDC_VOL_STATE_DISABLING = 1,
    EFDC_VOL_STATE_ENABLED = 2,
    EFDC_VOL_STATE_CACHE_DIRTY = 3,
    EFDC_VOL_STATE_INTERNAL_USE = 4,
    EFDC_VOL_STATE_ENABLING = 5,

    EFDC_VOL_STATE_MAX = EFDC_VOL_STATE_ENABLING
};

struct FctAdminVolState
{
    BasicVolumeCreationMethod   HowCreated;
    char                        State;
    K10_LU_ID                   LunWwn;       
};

struct K10EFDCacheFullVolumeStates
{
    ULONG            PlatformVolCount;
    FctAdminVolState      VolState[1];
};

#endif  // __FctVolStates_h__

