#ifndef __FctVolParams_h__
#define __FctVolParams_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2002
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      FctVolParams.h
//
// Contents:
//
// Revision History:
//--
#include "FctVolStates.h"
#include "BasicVolumeDriver/BasicVolumeParams.h"

struct K10EFDCacheVolumeParams : public FilterDriverVolumeParams
{
    ULONG            SeqNo;
    EFDC_VOL_STATE   ConfiguredState;
    K10_LU_ID        SavedWWN;
    CHAR             SavedUpperDeviceName[K10_DEVICE_ID_LEN];  // For DevMap reporting
    UCHAR            spare[8];
};

#endif  // __FctVolParams_h__

