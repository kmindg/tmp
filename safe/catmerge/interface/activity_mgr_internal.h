#ifndef ACTIVITY_MGR_INTERNAL_H
#define ACTIVITY_MGR_INTERNAL_H

// File: activity_mgr_internal.h      Component:  Background Activity Manager

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2009 - 2010
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:
//
//      This header file defines the internal interface exported by the Background 
//      Activity Manager subsystem. This is the interface used by different components
//      of the BAM framework to communicate with each other.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#if defined(__cplusplus) || defined(c_plusplus)		// For components written in C++
extern "C" {
#endif

#include "generic_types.h"


//  InBand API for Activity Manager to use.
//  InBand module supply I/O count query API for Activity Manager to compute the BG I/O ratio.
typedef struct activity_mgr_ib_api_s
{
    //  Background Activity Manager invokes the following InBand API function periodically
    //  to collect arriving BG I/O ratio
    void (*inband_get_io_counts)
    (
        UINT_64*                       host_io_counter,			//  Host I/O counter
        UINT_64*                       bg_io_counter,			//  BG I/O counter
        BOOL                           reset_counters           //  If this is true, IB reset counters and return 0
    );
        
} activity_mgr_ib_api_t;


//  A component invokes this function to get the CPU usage for each core.
//  The return value is a pointer to a float array, with one value for each
//  core on the SP. Each value will be between 0-1, where 0 represents completely
//  idle CPU, and 1 means 100% busy CPU.
float*  cpu_usage_get_all_cores_usage(void);

//  A component invokes this function to get the CPU usage for a single core.
//  This library function returns the array of float that represents the CPU
//  usage. The value will be between 0-1, where 0 represents completely idle 
//  CPU, and 1 means 100% busy CPU.
float  cpu_usage_get_single_core_usage(int);


//  Export function for IB to call during init to set up the IB API function.
void  activity_mgr_set_ib_api(activity_mgr_ib_api_t*);

#if defined(__cplusplus) || defined(c_plusplus)
};
#endif

#endif  // ACTIVITY_MGR_INTERNAL_H
