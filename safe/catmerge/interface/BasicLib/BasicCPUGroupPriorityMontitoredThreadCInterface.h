//***************************************************************************
/// Copyright (C) EMC Corporation 2012
/// All rights reserved.
/// Licensed material -- property of EMC Corporation
//**************************************************************************/

///////////////////////////////////////////////////////////
///  BasicCPUGroupThreadPriorityMonitorInterface.h
///  Definition of the
///  BasicCPUGroupThreadPriorityMonitorInterface and related classes.
//
///  Created on:      26-Jan-2005 4:52:11 PM
///////////////////////////////////////////////////////////

#if !defined(_BasicCPUGroupPriorityMontitoredThreadCInterface_h_)
#define _BasicCPUGroupPriorityMontitoredThreadCInterface_h_

#include "BasicLib/BasicWaiterQueuingClass.h"

#ifdef  INTERSP_LOCK_EXPORT
#define INTERSP_LOCK_DLL        CSX_MOD_EXPORT
#else
#define INTERSP_LOCK_DLL        CSX_MOD_IMPORT
#endif // INTERSP_LOCK_EXPORT


/// Queues the IRP to be pased to the handler in the context of this CPU's priority montitored RealTime thread.
/// The handler must not block the calling thread.
/// @param Irp - the IRP to queue.  OwnerContext[0-1] of the IRP may be overwriten.
/// @param handler - the function to call in the thread context
/// @param Context - the parameter to the handler
CSX_EXTERN_C INTERSP_LOCK_DLL
void  BasicCPUGroupPriorityMontitoredThreadQueueIrpHandlerCurrentCPU(PEMCPAL_IRP Irp,VOID (*handler)(PEMCPAL_IRP, PVOID Context), PVOID Context);

/// Queues the IRP to be pased to the handler in the context of the specified CPU's priority montitored RealTime thread.
/// The handler must not block the calling thread.
/// @param Irp - the IRP to queue.  OwnerContext[0-1] of the IRP may be overwriten.
/// @param handler - the function to call in the thread context
/// @param Context - the parameter to the handler
/// @param cpu - the CPU queue this IRP to.
CSX_EXTERN_C INTERSP_LOCK_DLL 
void BasicCPUGroupPriorityMontitoredThreadQueueIrpHandler(PEMCPAL_IRP Irp, VOID (*handler)(PEMCPAL_IRP, PVOID), PVOID Context, UINT_32 cpu);

// Callback function when a BasicCWaiter is run.  These functions must NEVER block the calling thread.
// @param waiter - the waiter object queue.  The contents of the waiter are undefined on return, but
//                 field is structures whose address can be calculated from the waiter pointer will be defined
//                 as specified prior to the enqueue.
typedef VOID (*BasicCWaiterHandler) (struct BasicCWaiter * waiter) ;



// Allows the C language item it is embedded in to be queued to a system worker thread. The typical usage pattern
// is to make this structure the first field in another structure, allowing the handler function to
// cast its parameter to the outer structure.  Note: C++ should use BasicWaiter class instead. 
//
// Example:
//
// typedef struct Foo { BasicCWaiter waiter; ...} or struct Foo { union u { BasicCWaiter waiter; ...}; ...} Foo;
//
// 
// {
//     FOO * foo;
//     ...
//     // Break recursion by queueing to thread for immediate callback.
//     BasicCPUGroupPriorityMontitoredThreadQueueBasicCWaiterCurrentCPU(QC_AlternateGroup, foo, &MyHandler);
//     return;
// }
//
// VOID  MyHandler(struct BasicCWaiter * waiter) { Foo * f = (Foo*) waiter; ....}
typedef struct BasicCWaiter {
    
    BasicCWaiter * links[2];

    BasicCWaiterHandler handler;
} BasicCWaiter;



/// Queues the BasicCWaiter to be passed to the handler in the context of the specified CPU's priority montitored RealTime thread.
/// The handler must not block the calling thread.  Note that the handler may actually be called on ANY thread, only
/// biased towards the current CPU.
/// @param qc - Identifies which of the CPUs queues to use.  Allows for quality of service.
/// @param toQueue - The BasicCWaiter to queue.  This only memory.  The contents prior to the call will be overwritten,
///                  and the contents of this structure are undefined oncompletion.  However, if this is embedded in another 
///                  structure, the handler could use this pointer to determine the address of the other structure.
/// @param callback - the function to call with the stack unwound on the current or other CPUs thread.
CSX_EXTERN_C INTERSP_LOCK_DLL
void  BasicCPUGroupPriorityMontitoredThreadQueueBasicCWaiterCurrentCPU(BasicWaiterQueuingClass qc, 
                                                                       BasicCWaiter * toQueue, BasicCWaiterHandler callback); 
/// Queues the BasicCWaiter to be passed to the handler in the context of the specified CPU's priority montitored RealTime thread.
/// The handler must not block the calling thread.  Note that the handler may actually be called on ANY thread, only
/// biased towards the current CPU.
/// @param qc - Identifies which of the CPUs queues to use.  Allows for quality of service.
/// @param toQueue - The BasicCWaiter to queue.  This only memory.  The contents prior to the call will be overwritten,
///                  and the contents of this structure are undefined oncompletion.  However, if this is embedded in another 
///                  structure, the handler could use this pointer to determine the address of the other structure.
/// @param callback - the function to call with the stack unwound on the current or other CPUs thread.
/// @param cpu - the preferred CPU queue this operation to.  
CSX_EXTERN_C INTERSP_LOCK_DLL
void  BasicCPUGroupPriorityMontitoredThreadQueueBasicCWaiter(BasicWaiterQueuingClass qc, 
                                                             BasicCWaiter * toQueue, BasicCWaiterHandler callback, 
                                                             UINT_32 cpu); 

/// Blocks the calling thread if low priority thread associated with the CPU
/// we are currently running on is starved.  Returns immediately if low priority
/// thread is not starving, otherwise blocks until system thread runs and
/// processes all prior work.
void  BasicCPUGroupPriorityMonitoredThreadCBlockIfLowPriorityThreadStarved(void);

#endif 