#ifndef __InBandSchedVolumeAdminParams_h__
#define __InBandSchedVolumeAdminParams_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      InBandSchedVolumeAdminParams.h
//
// Contents:
//      Defines the InBandSchedVolume Parameters passed between the In Band Scheduler 
//      and the admin layers. 
//
// Note: The members of this class have a default value of zero or false so 
//       that they maybe initialized correctly as part of a larger block
//       of generic memory which is initialized with a call to RtlZeroMemory().
//
// Revision History:
//--

#include "BasicVolumeDriver/BasicVolumeParams.h"


class InBandSchedVolumeAdminParams : public FilterDriverVolumeParams {
public:
    // Default values 
    InBandSchedVolumeAdminParams() 
    { 
        Initialize(); 
    };
    ~InBandSchedVolumeAdminParams() {};

    // Provide an initialize method which sets the default values.
    void Initialize()
    {
        // nothing here for now. 
        // need to add the job priority info here.
    }   

private:
    // Nothing here for now. 
};

#endif  // __InBandSchedVolumeAdminParams_h__

