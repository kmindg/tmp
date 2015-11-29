//***************************************************************************
/// Copyright (C) EMC Corporation 2013
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

#if !defined(_BasicCPUGroupThreadPriorityMonitor_h_)
#define _BasicCPUGroupThreadPriorityMonitor_h_

#include "BasicLib/BasicThread.h"
#include "BasicLib/BasicLockedObject.h"
#include "BasicLib/BasicDeviceObjectIrpDispatcherThread.h"
#include "BasicLib/BasicObjectTracer.h"
#include "BasicLib/BasicTimer.h"
#include "BasicLib/BasicWaiter.h"
#include "BasicLib/BasicIoRequest.h"
#include "BasicLib/BasicPerformanceControlInterface.h"
#include "TemplatedLIFOSinglyLinkedList.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#ifdef  INTERSP_LOCK_EXPORT
#define INTERSP_LOCK_DLL        CSX_MOD_EXPORT
#else
#define INTERSP_LOCK_DLL        CSX_MOD_IMPORT
#endif // INTERSP_LOCK_EXPORT

#if defined(ALAMOSA_WINDOWS_ENV) && defined(CSX_BV_LOCATION_KERNEL_SPACE)
#define LOW_PRIORITY_MONITOR_THREAD_BASE_PRIORITY (7)
#endif

#ifdef EMCPAL_CASE_WK
// Priority of low priority thread that is monitored for starvation.
#define LOW_PRIORITY_MONITOR_THREAD_PRIORITY (10)
// Priority that high priority thread will yield to when low priority monitor thread is starved.
#define HIGH_PRIORITY_REGISTRANT_THREAD_YIELD_TO_PRIORITY ((EMCPAL_THREAD_PRIORITY)( LOW_PRIORITY_MONITOR_THREAD_PRIORITY - 1 )) //Yield CPU to priority (LOW_PRIORITY_MONITOR_THREAD_PRIORITY-1)
#else
// Priority of low priority thread that is monitored for starvation.
#ifndef CSX_BO_USER_PREEMPTION_DISABLE_SUPPORTED
#define LOW_PRIORITY_MONITOR_THREAD_PRIORITY EMCPAL_THREAD_PRIORITY_NORMAL
// Priority that high priority thread will yield to when low priority monitor thread is starved.
#define HIGH_PRIORITY_REGISTRANT_THREAD_YIELD_TO_PRIORITY EMCPAL_THREAD_PRIORITY_NORMAL
#else
#define LOW_PRIORITY_MONITOR_THREAD_PRIORITY EMCPAL_THREAD_PRIORITY_LOW
// Priority that high priority thread will yield to when low priority monitor thread is starved.
#define HIGH_PRIORITY_REGISTRANT_THREAD_YIELD_TO_PRIORITY EMCPAL_THREAD_PRIORITY_LOW
#endif
#endif


/*
 * Define the DLL declaration.
 */

class BasicPriorityMonitorRegistrant;

typedef LIFOList<BasicPriorityMonitorRegistrant> ListOfBasicPriorityMonitorRegistrant;

/// BasicPriorityMonitorRegistrant is a base class that allows the subclass to register via
/// BasicCPUGroupThreadPriorityMonitorInterface for
/// notifications to change thread priorities when lower priority threads appear to be
/// starved for CPU time.
class BasicPriorityMonitorRegistrant : private ListOfBasicPriorityMonitorRegistrant::ListEntry
{

public:

    friend class LIFOList<BasicPriorityMonitorRegistrant>;

    /// When called back, this indicates that there appears to be CPU starvation, and the
    /// registrant should take action to reduce its demand, typically  changing its priority.
    /// @param targetPriority - the recommended priority for the thread at this point in
    ///                         time.
    virtual void DowngradePriorityLocksMayBeHeld(EMCPAL_THREAD_PRIORITY targetPriority) = 0;

    /// Reasons for why we are restoring priority.
    enum RestorePriorityReason {
        RESTORED_TIMESLICE, // We have been downgraded long enough
        RESTORED_RAN,        // Monitor thread ran
        RESTORED_RUNNING_LONG_TIME // Monitor thread has been able to run consistently
    };

    /// Informs the registrant is should return to its normal priority.
    virtual void RestorePriorityLocksMayBeHeld(RestorePriorityReason reason) = 0 ;

    // Checks if registrant should boost priority
    virtual bool BoostIfStarved(EMCPAL_TIME_USECS timeout) = 0;

    /// Returns: the affinty of the registrant, which is used to match it with a monitor group.
    /// The registrant must use the same affinity groupings as the monitor group.
    virtual EMCPAL_PROCESSOR_MASK GetAffinity() const = 0;

    // Checks for any stuck thread. If thread is stuck then it takes appropriate action.
    virtual void DebugCheckForStuckLocksMayBeHeld() = 0;

};

/// Defines the API to the monitor component, excluding the mechanism to attach to it.
/// This interface allows:
///     - Registration of high priority threads which will then get callbacks if there is
///       evidence they are starving lower priority threads.  Such callbacks will tell the
///       registrants that they should lower or restore their CPU priority.  The time where
///       the CPU priority is lower will be bounded and of short duration, perhaps 1/3 of the time.
///     - Queuing of BasicIoRequests/BasicWaiters to CPU Group affined high priority registered (see previous) threads provided by this component.
class BasicPerCpuThreadPriorityMonitorInterface
{

public:

    /// Allows registration for callbacks to downgrade & restore priority.
    /// The registrant must have the same affinity as one of the CPU Groups.
    /// @param registrant - the object to register for callbacks.
    virtual void EnablePriorityDowngrade(BasicPriorityMonitorRegistrant* registrant) = 0;

    /// Undoes prior registration for callbacks to downgrade & restore priority.
    /// @param registrant - the object to registered for callbacks.
    virtual void DisablePriorityDowngrade(BasicPriorityMonitorRegistrant* registrant) = 0;

    /// Returns a the current CPU #.
    static UINT_32 CurrentCPU()
    {
        return csx_p_get_processor_id();
    }

    /// Uses the next IRP stack location to queue the IRP, so that the completion routine
    /// will be called in the thread context.  Optimized to have a tread per CPU to avoid
    /// CPU switches.
    /// @param Irp - the Irp to queue to the thread, and the second parameter to pass to CompletionRoutine
    /// @param CompletionRoutine - the function to call in the thread context.
    /// @param context - the third parameter to pass to CompletionRoutine
    virtual void QueueIrpCompletionToThread(PBasicIoRequest Irp,
                                            PBasicIoCompletionRoutine CompletionRoutine,
                                            PVOID context,
                                            UINT_32 cpu) = 0;

    /// Complete the request,  where the status and information are already set in the
    /// request. If the request is flagged as a paired request, we must also complete the
    /// sibling request with the status set in Irp. Completes the IRP in the current
    /// context, or queues it to a different thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// WDM: IoCompleteRequest(this, IO_NO_INCREMENT);
    virtual void CompleteRequestLimitRecursion(PBasicIoRequest Irp) = 0;

    /// Complete the request with the status and information specified. If this is flagged
    /// as a paired request, we must also complete the sibling request with the same
    /// status. Completes the IRP in the current context, or queues it to a different
    /// thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// @param status - the status to report for the command.
    /// @param info - 64 bits of infomation which is dependent on the command type.
    /// WDM: IRP::IoStatus.Status = status; IRP::IoStatus.Information = info; IoCompleteRequest(this, IO_NO_INCREMENT);
    virtual void CompleteRequestLimitRecursion(PBasicIoRequest Irp,
                                               EMCPAL_STATUS Status,
                                               ULONG_PTR info = 0) = 0;

    /// Complete the request with the status and information specified. If this is flagged
    /// as a paired request, we must also complete the sibling request with the same
    /// status. Completes the IRP in the current context, or queues it to a different
    /// thread as a function of the stack size and input cpu.
    /// @param Irp - the request to complete.
    /// @param status - the status to report for the command.
    /// @param info - 64 bits of infomation which is dependent on the command type.
    /// @param cpu - CPU on which to schedule completion
    virtual void CompleteRequestLimitRecursionFull(PBasicIoRequest Irp,
                                               EMCPAL_STATUS Status,
                                               ULONG_PTR info,
                                               UINT_32 cpu) = 0;

    /// Complete the request,  where the status and information are already set in the
    /// request. If the request is flagged as a paired request, we must also complete the
    /// sibling request with the status set in Irp. Completes the IRP in the current
    /// context, or queues it to a different thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// @param PreferredCPU - the CPU on which a deferred request should run
    /// WDM: IoCompleteRequest(this, IO_NO_INCREMENT);
    virtual void CompleteRequestLimitRecursionCPU(PBasicIoRequest Irp, UINT_32 PreferredCPU) = 0;

    /// Called waiter->Signalled() in this context, or queues it to a different context,
    /// depending on stack size and current CPU context.
    virtual void SignalWaiterLimitRecursion(BasicWaiterWithStatus* waiter,
                                            EMCPAL_STATUS status) = 0;

    /// Called waiter->Signalled() in this context, or queues it to a different context,
    /// depending on stack size alone.
    virtual void SignalWaiterLimitRecursionBasedOnStackSizeAlone(BasicWaiterWithStatus* waiter,
                                            EMCPAL_STATUS status) = 0;
    
    /// Calls CompleteRequestComplationStatusAlreadySet() on the IRP from a thread context,
    /// discarding any recursion.
    /// @param Irp - the Irp to queue to the thread, and the second parameter to pass to CompletionRoutine
    /// @param cpu  - which CPU's queue to sent it to.  Defaults to current CPU.
    virtual void QueueIrpToThreadToComplete(PBasicIoRequest Irp,
                                            UINT_32 cpu) = 0;

    /// Calls waiter->Signaled(status) from a different context, eliminating recursion.
    /// @param waiter - the waiter to signal.
    /// @param status - the status to pass to Signaled().
    /// @param cpu  - which CPU's queue to sent it to.  Defaults to current CPU.
    virtual void QueueWaiterToThread(BasicWaiterWithStatus* waiter,
                                     EMCPAL_STATUS status, UINT_32 cpu) = 0;
    virtual void QueueWaiterToThread(BasicWaiter* waiter,
                                      UINT_32 cpu) = 0;


    /// Calls waiter->Signaled(status) from a different context, eliminating recursion.
    /// @param waiter - the waiter to signal.
    /// @param status - the status to pass to Signaled().
    /// @param lun - host LU number, used as key when Affinity Striping is enabled.
    /// @param lba - host LBA, used as key when Affinity Striping is enabled.
    virtual void QueueWaiterToBestThread(BasicWaiterWithStatus* waiter,
                                     EMCPAL_STATUS status, ULONG64 lun, ULONG64 lba) = 0;

    /// Call BasicDeviceObject::DispatchQueuedIRP() in the context of the thread. This
    /// function calls Irp->MarkPending(). The Irp's current
    /// IO_STACK_LOCATION::DeviceObject->DeviceExtension is assumed to contain a
    /// BasicDeviceObject.
    /// @param Irp - the Irp to queue to the thread.
    /// @param cpu  - which CPU's queue to sent it to.  Defaults to current CPU.
    virtual VOID QueueIrpToThread(PBasicIoRequest Irp,
                                  UINT_32 cpu = CurrentCPU()) = 0;

    /// Queues the IRP to be pased to the handler in the context of this CPU's priority montitored RealTime thread.
    /// The handler must not block the calling thread.
    /// @param Irp - the Irp to queue to the thread.
    /// @param handler - the function to call in the thread's context
    /// @param Context - value to pass to handler
    /// @param cpu  - which CPU's queue to sent it to.  Defaults to current CPU.
    /// @param qc - which priority queue to put it in. Defaults to QC_DefaultGroup.
    virtual VOID QueueIrpToThread(PBasicIoRequest Irp, VOID (*handler)(PEMCPAL_IRP, PVOID), PVOID Context,
                                  UINT_32 cpu = CurrentCPU(), BasicWaiterQueuingClass qc = QC_DefaultGroup) = 0;

    /// Calls Device->CallDriver(Irp) in the target thread's context.
    ///
    /// @param Irp - the Irp to queue to the thread, and the second parameter to pass to
    ///              CompletionRoutine
    /// @param Device - The device to send the IRP to.
    /// @param cpu  - which CPU's queue to sent it two.  Defaults to current CPU.
    virtual void QueueCallDriverToThread(PBasicIoRequest Irp, class BasicConsumedDevice * Device, UINT_32 cpu = CurrentCPU()) = 0;

    /// Returns the actual number of CPUs
    virtual UINT_32 GetNumberOfCPUs() const = 0;

    /// Set the work distribution policy.
    /// @param policy - New policy.
    virtual VOID SetWorkDistributionPolicy(BasicPerformancePolicy *policy) = 0;

    /// Return the current work distribution policy
    /// @param policy - Return policy in buffer provided
    virtual VOID GetWorkDistributionPolicy(BasicPerformancePolicy *policy) = 0;

    /// Set the AffinityStripeSize
    /// @param StripeSizeInBlocks - the affinity stripe element size
    virtual VOID SetAffinityStripeSize(UINT_64 stripeSizeInBlocks) = 0; // 0 = Disabled

    /// Blocks the calling thread if low priority thread associated with the CPU
    /// we are currently running on is starved.  Returns immediately if low priority
    /// thread is not starving, otherwise blocks until system thread runs and
    /// processes all prior work.
    virtual VOID BlockIfLowPriorityThreadStarved(VOID) = 0;

};

/// Use this method to get the singleton BasicCPUGroupThreadServiceProvider object.
CSX_EXTERN_C INTERSP_LOCK_DLL
BasicPerCpuThreadPriorityMonitorInterface* getBasicCPUThreadServiceProvider(void);

/// This base class attaches to the BasicPerCpuThreadPriorityMonitor component, and exposes
/// the BasicPerCpuThreadPriorityMonitorInterface to its subclasses.
class BasicPerCpuThreadPriorityMonitorProxy : public BasicPerCpuThreadPriorityMonitorInterface
{

public:

    /// Constructs the object, but attach must be called later.
    BasicPerCpuThreadPriorityMonitorProxy() : mMonitor(NULL) {}

    /// Attach to the BasicCPUThreadServiceProvider
    bool Attach()
    {
        mMonitor = getBasicCPUThreadServiceProvider();

        if(!mMonitor) {

            return FALSE;
        }

        return TRUE;
    }

    /// Allows registration for callbacks to downgrade & restore priority.
    /// The registrant must have the same affinity as one of the CPU Groups.
    /// @param registrant - the object to register for callbacks.
    void EnablePriorityDowngrade(BasicPriorityMonitorRegistrant* registrant) CSX_CPP_OVERRIDE
    {
        mMonitor->EnablePriorityDowngrade(registrant);
    }

    /// Undoes prior registration for callbacks to downgrade & restore priority.
    /// @param registrant - the object to registered for callbacks.
    void DisablePriorityDowngrade(BasicPriorityMonitorRegistrant* registrant) CSX_CPP_OVERRIDE
    {
        mMonitor->DisablePriorityDowngrade(registrant);
    }

    /// Returns a the current CPU #.
    static UINT_32 CurrentCPU()
    {
        return csx_p_get_processor_id();
    }

    /// Uses the next IRP stack location to queue the IRP, so that the completion routine
    /// will be called in the thread context.  Optimized to have a tread per CPU to avoid
    /// CPU switches.
    /// @param Irp - the Irp to queue to the thread, and the second parameter to pass to CompletionRoutine
    /// @param CompletionRoutine - the function to call in the thread context.
    /// @param context - the third parameter to pass to CompletionRoutine
    void QueueIrpCompletionToThread(PBasicIoRequest  Irp,
                                    PBasicIoCompletionRoutine CompletionRoutine,
                                    PVOID context,
                                    UINT_32 cpu = CurrentCPU()) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueIrpCompletionToThread(Irp, CompletionRoutine, context, cpu) ;
    }

    /// Complete the request,  where the status and information are already set in the
    /// request. If the request is flagged as a paired request, we must also complete the
    /// sibling request with the status set in Irp. Completes the IRP in the current
    /// context, or queues it to a different thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// WDM: IoCompleteRequest(this, IO_NO_INCREMENT);
    void CompleteRequestLimitRecursion(PBasicIoRequest Irp) CSX_CPP_OVERRIDE
    {
        UINT_32 cpu = CurrentCPU();

        mMonitor->CompleteRequestLimitRecursionCPU(Irp, cpu);
    }

    /// Complete the request with the status and information specified. If this is flagged
    /// as a paired request, we must also complete the sibling request with the same
    /// status. Completes the IRP in the current context, or queues it to a different
    /// thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// @param status - the status to report for the command.
    /// @param info - 64 bits of infomation which is dependent on the command type.
    /// WDM: IRP::IoStatus.Status = status; IRP::IoStatus.Information = info;
    ///      IoCompleteRequest(this, IO_NO_INCREMENT);
    void CompleteRequestLimitRecursion(PBasicIoRequest Irp, EMCPAL_STATUS Status, ULONG_PTR info = 0) CSX_CPP_OVERRIDE
    {
        UINT_32 cpu = CurrentCPU();

        mMonitor->CompleteRequestLimitRecursionFull(Irp, Status, info, cpu);
    }

    /// Complete the request with the status and information specified. If this is flagged
    /// as a paired request, we must also complete the sibling request with the same
    /// status. Completes the IRP in the current context, or queues it to a different
    /// thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// @param status - the status to report for the command.
    /// @param info - 64 bits of infomation which is dependent on the command type.
    /// @param cpu  - CPU on which to schedule recursion break
    /// WDM: IRP::IoStatus.Status = status; IRP::IoStatus.Information = info;
    ///      IoCompleteRequest(this, IO_NO_INCREMENT);
    void CompleteRequestLimitRecursionFull(PBasicIoRequest Irp, EMCPAL_STATUS Status, ULONG_PTR info, UINT_32 cpu) CSX_CPP_OVERRIDE
    {
        mMonitor->CompleteRequestLimitRecursionFull(Irp, Status, info, cpu);
    }

    /// Complete the request,  where the status and information are already set in the
    /// request. If the request is flagged as a paired request, we must also complete the
    /// sibling request with the status set in Irp. Completes the IRP in the current
    /// context, or queues it to a different thread as a function of the stack size.
    /// @param Irp - the request to complete.
    /// @param PreferredCPU - the CPU on which the request should be run
    /// WDM: IoCompleteRequest(this, IO_NO_INCREMENT);
    void CompleteRequestLimitRecursionCPU(PBasicIoRequest Irp, UINT_32 PreferredCPU) CSX_CPP_OVERRIDE
    {
        mMonitor->CompleteRequestLimitRecursionCPU(Irp, PreferredCPU);
    }

    /// Called waiter->Signalled() in this context, or queues it to a different context,
    /// depending on stack size and current CPU context.
    void SignalWaiterLimitRecursion(BasicWaiterWithStatus* waiter, EMCPAL_STATUS status) CSX_CPP_OVERRIDE
    {
        mMonitor->SignalWaiterLimitRecursion(waiter, status);
    }

    /// Called waiter->Signalled() in this context, or queues it to a different context,
    /// depending on stack size alone.
    void SignalWaiterLimitRecursionBasedOnStackSizeAlone(BasicWaiterWithStatus* waiter, EMCPAL_STATUS status) CSX_CPP_OVERRIDE
    {
        mMonitor->SignalWaiterLimitRecursionBasedOnStackSizeAlone(waiter, status);
    }

    /// Calls CompleteRequestComplationStatusAlreadySet() on the IRP from a thread context,
    /// discarding any recursion.
    /// @param Irp - the Irp to queue to the thread, and the second parameter to pass to CompletionRoutine
    void QueueIrpToThreadToComplete(PBasicIoRequest Irp, UINT_32 cpu = CurrentCPU()) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueIrpToThreadToComplete(Irp, cpu);
    }

    /// Calls waiter->Signaled(status) from a different context, eliminating recursion.
    /// @param waiter - the waiter to signal.
    /// @param status - the status to pass to Signaled().
    void QueueWaiterToThread(BasicWaiterWithStatus* waiter, EMCPAL_STATUS status, UINT_32 cpu = CurrentCPU()) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueWaiterToThread(waiter, status, cpu);
    }
    void QueueWaiterToThread(BasicWaiter * waiter, UINT_32 cpu = CurrentCPU()) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueWaiterToThread(waiter, cpu);
    }

    /// Calls waiter->Signaled(status) from a different context, eliminating recursion.
    /// @param waiter - the waiter to signal.
    /// @param status - the status to pass to Signaled().
    /// @param lun - host LU number, (used as a key when Affinity Striping is enabled).
    /// @param lba - host LBA (used as a key when Affinity Striping is enabled).
    void QueueWaiterToBestThread(BasicWaiterWithStatus* waiter, EMCPAL_STATUS status, ULONG64 lun = -1, ULONG64 lba = -1) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueWaiterToBestThread(waiter, status, lun, lba);
    }

    /// Pass the AffinityStripeSize thru to the monitor 
    /// @param StripeSizeInBlocks - the affinity stripe element size
    void SetAffinityStripeSize(UINT_64 stripeSizeInBlocks)
    {
         mMonitor->SetAffinityStripeSize(stripeSizeInBlocks);
    }

    /// Call BasicDeviceObject::DispatchQueuedIRP() in the context of the thread. This
    /// function calls Irp->MarkPending(). The Irp's current
    /// IO_STACK_LOCATION::DeviceObject->DeviceExtension is assumed to contain a
    /// BasicDeviceObject.
    /// @param Irp - the Irp to queue to the thread.
    VOID QueueIrpToThread(PBasicIoRequest Irp, UINT_32 cpu = CurrentCPU()) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueIrpToThread(Irp, cpu);
    }

    /// Queues the IRP to be pased to the handler in the context of this CPU's priority montitored RealTime thread.
    /// The handler must not block the calling thread.
    /// @param Irp - the Irp to queue to the thread.
    /// @param handler - the function to call in the thread's context
    /// @param Context - value to pass to handler
    /// @param cpu  - which CPU's queue to sent it to.  Defaults to current CPU.
    /// @param qc - which priority queue to put it in. Defaults to QC_DefaultGroup.
    VOID QueueIrpToThread(PBasicIoRequest Irp, VOID (*handler)(PEMCPAL_IRP, PVOID), PVOID Context,
                          UINT_32 cpu = CurrentCPU(), BasicWaiterQueuingClass qc = QC_DefaultGroup) CSX_CPP_OVERRIDE
    {
        mMonitor->QueueIrpToThread(Irp, handler, Context, cpu, qc);
    }

    /// Calls Device->CallDriver(Irp) in the target thread's context.
    ///
    /// @param Irp - the Irp to queue to the thread, and the second parameter to pass to
    ///              CompletionRoutine
    /// @param Device - The device to send the IRP to.
    /// @param cpu  - which CPU's queue to sent it to.  Defaults to current CPU.
    void QueueCallDriverToThread(PBasicIoRequest Irp, BasicConsumedDevice * Device, UINT_32 cpu = CurrentCPU())
    {
		mMonitor->QueueCallDriverToThread(Irp, Device, cpu);
    }


    
    /// Returns the actual number of CPUs
    UINT_32 GetNumberOfCPUs() const CSX_CPP_OVERRIDE { return mMonitor->GetNumberOfCPUs(); }

    /// Set the performance policy.
    /// @param policy - New policy.
    VOID SetWorkDistributionPolicy(BasicPerformancePolicy *policy)
    {
        mMonitor->SetWorkDistributionPolicy(policy);
    }

    /// Return the current performance policy
    VOID GetWorkDistributionPolicy(BasicPerformancePolicy *policy)
    {
        mMonitor->GetWorkDistributionPolicy(policy);
    }

    /// Blocks the calling thread if low priority thread associated with the CPU
    /// we are currently running on is starved.  Returns immediately if low priority
    /// thread is not starving, otherwise blocks until system thread runs and
    /// processes all prior work.
    void BlockIfLowPriorityThreadStarved(void)
    {
        mMonitor->BlockIfLowPriorityThreadStarved();
    }

private:

    /// A BasicCPUThreadServiceProvider
    BasicPerCpuThreadPriorityMonitorInterface* mMonitor;

};

#endif /// !defined(_BasicCPUGroupThreadPriorityMonitor_h_)
