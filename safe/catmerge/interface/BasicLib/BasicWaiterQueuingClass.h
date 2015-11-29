//***************************************************************************
// Copyright (C) EMC Corporation 2012
//
// Licensed material -- property of EMC Corporation
//**************************************************************************/

#ifndef __BasicWaiterQueuingClass_h__
#define __BasicWaiterQueuingClass_h__

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicWaiterQueuingClass.h
//
// Contents:
//      Defines the BasicWaiterQueuingClass enum 
//
// Revision History:
//--


// The queueing class defines the quality of service desired from the system threads.  The Default and Alternate groups
// get the same QOS, but we will drain a number of items from one before moving to the other.  Queuing similar objects
// to the queue and dissimilar to the alternate should reduce L2 cache misses.  After items are drain from those queues,
// a smaller number of items will be drained from the LowPriorityGroup, and then back to the Default group.
enum BasicWaiterQueuingClass {QC_FastGroup = 0, QC_DefaultGroup = 1, QC_AlternateGroup = 2, QC_LowPriorityGroup = 3, QC_NumQueueingClass = 4 };

#endif  // __BasicWaiter_h__

