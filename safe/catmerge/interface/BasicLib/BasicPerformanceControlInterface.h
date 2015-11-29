#if !defined(_BasicPerformanceControlInterface_h_)
#define _BasicPerformanceControlInterface_h_

/***************************************************************************
 * Copyright (C) EMC Corporation 2012-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *
 *              Class declaration: BasicPerformanceControlInterface
 *    This file defines the public interface used to control and monitor
 *    the work distribution policies to help improve performance.
 *
 ***************************************************************************/

#include "BasicLib/BasicWaiterQueuingClass.h"

enum BasicPerformanceAffinityPolicy
{
    PULL_MODEL = 1,
    LEGACY_AFFINITY = 2,
    ROUNDROBIN_AFFINITY=9,
    ALWAYS_ROUNDROBIN_AFFINITY,
    ALWAYS_ROUNDROBIN_1UP_AFFINITY,
    NUMA_STRIPE_AFFINITY,
    NEXT_AFFINITY,
    MUST_BE_LAST
};

// This structure provides a simple place to add control for new policies and
// their potential parameters.
// 
// Additional aspects to the work distribution policies should be added to 
// this structure.  An example would be if we wanted to add flags to control
// whether to include LUN and/or LBA range in the bias.  And if LBA range, 
// fields to specify the parameters of the range.
typedef struct
{
   enum BasicPerformanceAffinityPolicy   AffinityPolicy;

 
   // Maximum # of items to service from each per-CPU work queue before moving to next queue
   UINT_16E                              MaxItemsPerQueue[QC_NumQueueingClass];
   
   // Allows control of the time a worker thread is allowed to stall before a crash is forced.
   // Useful for debug.
   UINT_16E                              MaxSecondsWithNoThreadForwardProgress;
   
   // The number of items to pull from the thread's own work queue at a time, up to the max.
   UINT_16E                              DrainBurstSizeOwnQueue;

   // The number of items to pull from other threads' work queues at a time, up to the max.
   UINT_16E                              DrainBurstSizeOtherQueue;
} BasicPerformancePolicy;



#endif // _BasicPerformanceControlInterface_h_
