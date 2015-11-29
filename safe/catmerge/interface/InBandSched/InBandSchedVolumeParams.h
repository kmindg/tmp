#ifndef __InBandSchedVolumeParams_h__
#define __InBandSchedVolumeParams_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      InBandSchedVolumeParams.h
//
// Contents:
//      Defines the InBandSchedVolume Parameters passed between the In-band
//      scheduler and 
//      the admin layer. 
//
// Note: The members of this class have a default value of zero or false so 
//       that they maybe initialized correctly as part of a larger block
//       of generic memory which is initialized with a call to RtlZeroMemory().
//
// Revision History:
//--

#include "InBandSched/InBandSchedVolumeAdminParams.h"

class InBandSchedVolumeParams : public FilterDriverVolumeParams
{
    // No extra parameters.
};

/*  KM: Comment this out for now
class InBandSchedVolumeParams : public InBandSchedVolumeAdminParams {
public:
    // Default values 
    InBandSchedVolumeParams() 
    {
        Initialize(); 
    };
    ~InBandSchedVolumeParams() {};

    // Provide an initialize method which sets the default values.
    // This is public incase admin needs it.
    void Initialize()
    {
        // call the base class initialize function since we can't have virtual functions
        InBandSchedVolumeAdminParams::Initialize();
    }   

};
*/
#endif  // __InBandSchedVolumeParams_h__

