/************************************************************************
*
*  Copyright (c) 2005-2011 EMC Corporation.
*  All rights reserved.
*
*  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
*  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
*  BE KEPT STRICTLY CONFIDENTIAL.
*
*  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
*  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
*  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
*  MAY RESULT.
*
************************************************************************/

#ifndef __BasicThreadWaitEvent__
#define __BasicThreadWaitEvent__
//++
// File Name:
//      BasicThreadWaitEvent.h
//
// Contents:
//  Defines the BasicThreadWaitEvent and  BasicThreadWaitEventManualReset classes.
//
//
// Revision History:
//--
#include "cppshdriver.h"
#include "BasicLib/BasicWaiter.h"
# include "EmcPAL.h"
# include "EmcPAL_DriverShell.h"

#if defined(SIMMODE_ENV)
#define SIM_ASSERT_NOT_DISPATCH_LEVEL()  DBG_ASSERT(EMCPAL_LEVEL_BELOW_DISPATCH())
#else
#define SIM_ASSERT_NOT_DISPATCH_LEVEL()
#endif

// Provides an object that is initially in the unsignaled state.  A thread may wait for
// the object to transition to the signaled state.
class BasicThreadWaitEvent : public BasicWaiter {
public:
    // Construction is in the unsignaled state.
    inline BasicThreadWaitEvent();

    inline ~BasicThreadWaitEvent();

    // Puts the obejct into the unsignaled state.
    inline void ClearSignal();

    inline void Signaled(EMCPAL_STATUS status = EMCPAL_STATUS_SUCCESS);

    // Wait indefinitely for a transition to the signaled state.  Will not wait if the
    // state was already signaled. Always leaves the state unsignaled.
    // Restrictions: No spinlocks may be held by the caller, and no interrupts should be
    // disabled.
    // Returns: The value that was passed into the Signaled() call.
    inline EMCPAL_STATUS WaitForSignal();

    // Wait for a transition to the signaled state till timeout.  Will not wait if the
    // state was already signaled. Always leaves the state unsignaled.
    // It panics on timeout.
    // @param Seconds - the wait time limit.  Waits longer than this time are
    //                         considered bugs and will crash the SP.
    // Restrictions: No spinlocks may be held by the caller, and no interrupts should be
    // disabled.
    // Returns: The value that was passed into the Signaled() call.
    inline EMCPAL_STATUS WaitForSignalPanicOnTimeout(ULONG Seconds);

    // Wait for a transition to the signaled state till timeout.  Will not wait if the
    // state was already signaled. Always leaves the state unsignaled.    // 
    // @param timeoutMilliSeconds - the wait time limit.
    // @param signaledStatus - status of the wait.
    //
    // Restrictions: No spinlocks may be held by the caller, and no interrupts should be
    // disabled.
    // Returns: true if event is signaled.
    //          false if we hit timeout.
    inline bool WaitForSignalOrTimeout(ULONG timeoutMilliSeconds, EMCPAL_STATUS & signaledStatus);

    // This is the windows compatible IRP completion routine.
    // This is callback function which we can register with BasicIoRequest::SetNextCompletionRoutine()
    // So once processiog of IRP completes by lower driver it will automatically gets called.
    // @param unused - This is not being used by this function.
    // @param Irp    - IRP that has been completed by lower driver.
    // @param Context- This is context of BasicThreadWaitEvent.    
    //
    // Returns: We always returns STATUS_MORE_PROCESSING_REQUIRED.
    static inline EMCPAL_STATUS EventWakeup(PVOID unused, BasicIoRequest * Irp, PVOID Context);

    // Handling of IRP completion. Our completion routine EventWakeup() will call
    // this function.
    // @param Irp - Irp that is just completed by lower driver.
    inline void SetWakeupOnIrpCompletion(PBasicIoRequest pIrp);

protected:
    EMCPAL_SYNC_EVENT     mEvent;

    EMCPAL_STATUS   mStatus;
};

inline void BasicThreadWaitEvent::Signaled(EMCPAL_STATUS status) {
    mStatus = status;
    EmcpalSyncEventSet(&mEvent);
}

inline EMCPAL_STATUS BasicThreadWaitEvent::WaitForSignal() {
    SIM_ASSERT_NOT_DISPATCH_LEVEL();
    (void)EmcpalSyncEventWait(&mEvent, EMCPAL_TIMEOUT_INFINITE_WAIT);

    return mStatus;
}

inline EMCPAL_STATUS BasicThreadWaitEvent::WaitForSignalPanicOnTimeout(ULONG Seconds){
    SIM_ASSERT_NOT_DISPATCH_LEVEL();
    EMCPAL_STATUS CSX_MAYBE_UNUSED status = EmcpalSyncEventWait(&mEvent, Seconds*1000);

    FF_ASSERT(EMCPAL_IS_SUCCESS(status) && status != EMCPAL_STATUS_TIMEOUT);
    return mStatus;
}

inline bool BasicThreadWaitEvent::WaitForSignalOrTimeout(ULONG timeoutMilliSeconds, EMCPAL_STATUS & signaledStatus){
    if (timeoutMilliSeconds != 0) { 
        SIM_ASSERT_NOT_DISPATCH_LEVEL(); 
    }
    EMCPAL_STATUS status = EmcpalSyncEventWait(&mEvent, timeoutMilliSeconds);

    if (status == EMCPAL_STATUS_TIMEOUT){
        return false;
    }
    else{
        signaledStatus = mStatus;
    }

    return true;
}

inline BasicThreadWaitEvent::BasicThreadWaitEvent() {
    mStatus = EMCPAL_STATUS_PENDING;
    EmcpalSyncEventCreate(EmcpalDriverGetCurrentClientObject(),
        &mEvent,
        "basicvolumedriver_BasicThreadWaitEvent_re",
        FALSE );
}

inline BasicThreadWaitEvent::~BasicThreadWaitEvent() {
    mStatus = EMCPAL_STATUS_PENDING;
    EmcpalSyncEventDestroy(&mEvent);
}

inline void BasicThreadWaitEvent::ClearSignal() {

    EmcpalSyncEventClear(&mEvent);
};

inline EMCPAL_STATUS BasicThreadWaitEvent::EventWakeup(PVOID unused, BasicIoRequest * Irp, PVOID Context)
{
    BasicThreadWaitEvent * This = (BasicThreadWaitEvent*) Context;
    This->SetWakeupOnIrpCompletion(Irp);

    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}

inline void BasicThreadWaitEvent::SetWakeupOnIrpCompletion(PBasicIoRequest pIrp)
{
    Signaled(pIrp->GetCompletionStatus());
}

#endif  // __BasicDeviceObject__

