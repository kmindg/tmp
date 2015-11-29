//***************************************************************************
/// Copyright (C) EMC Corporation 2012
/// All rights reserved.
/// Licensed material -- property of EMC Corporation
//**************************************************************************/

///////////////////////////////////////////////////////////
///  BasicCPUGroupThreadPriorityMonitorBasicWaiterInterface.h
///  Definition of simple interfaces to queue BasicWaiters to system threads.
//
///////////////////////////////////////////////////////////

#if !defined(_BasicCPUGroupPriorityMontitoredThreadBasicWaiterInterface_h_)
#define _BasicCPUGroupPriorityMontitoredThreadBasicWaiterInterface_h_

#include "BasicLib/BasicWaiter.h"

#ifdef  INTERSP_LOCK_EXPORT
#define INTERSP_LOCK_DLL        CSX_MOD_EXPORT
#else
#define INTERSP_LOCK_DLL        CSX_MOD_IMPORT
#endif // INTERSP_LOCK_EXPORT


// Provide a bare-bones C++ interface that allows a C++ file to only pull in BasicWaiter.h, but no other declarations (like CPP shell).

/// Queues the BasicWaiter to be passed to  BasicWaiter::Signaled() in the context of the specified CPU's priority montitored RealTime thread.
/// The handler must not block the calling thread.  Note that the handler may actually be called on ANY thread, only
/// biased towards the current CPU.
/// @param toQueue - The BasicWaiter to queue.   It provides BasicWaiter::Signaled(), GetQueuingClass() 
/// @param cpu - the preferred CPU queue this operation to.  This is used, rather than BasicWaiter::PreferredCPU().
CSX_EXTERN_C INTERSP_LOCK_DLL
void  BasicCPUGroupPriorityMontitoredThreadQueueBasicWaiter( class BasicWaiter * toQueue,  UINT_32 cpu); 

#endif 