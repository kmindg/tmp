/************************************************************************
*
*  Copyright (c) 2009-2011 EMC Corporation.
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

#ifndef __BasicDeferredProcedureCall__
#define __BasicDeferredProcedureCall__

# include "EmcPAL_DriverShell.h"
# include "EmcPAL_Dpc.h"
//++
// File Name:
//      BasicDeferredProcedureCall.h
//
// Contents:
//  Defines the BasicDeferredProcedureCall class.
//
//
// Revision History:
//--


// An object which inherits from this class can request to be called back from a different
// context.  This is used to break recursion, or to trigger additional action after locks
// are released.
class BasicDeferredProcedureCall {
public:
    inline BasicDeferredProcedureCall();
    inline ~BasicDeferredProcedureCall();

    // Called indicating that the deferred procedure call occured. Spin locks are never
    // held at the time of this call.  Some interrupts will be disabled at the time of the
    // call.  This function must not lower the interrupt level, and it must not attempt to
    // block the calling context.
    virtual void DeferredProcedureCallHandler() = 0;

    // Triggers the DeferredProcedureCallHandler() to be called.  Spinlocks may be held by
    // the caller.  The
    inline void DeferredProcedureCallSchedule();

    // Use this to target the Dpc to run on a specific processor.
    inline void SetTargetProcessor(unsigned char processorNumber);

private:

    static VOID StaticDPCHandler(PVOID tthis)
    {
        BasicDeferredProcedureCall * This = (BasicDeferredProcedureCall*)tthis;
        This->DeferredProcedureCallHandler();
    }
    
    // Define base DPC function that will set any required context (e.g. CSX) prior to calling our DPC
    static EMCPAL_DEFINE_DPC_FUNC(StaticDPCHandlerBase, StaticDPCHandler)

    EMCPAL_DPC_OBJECT myDPC;
};

inline BasicDeferredProcedureCall::BasicDeferredProcedureCall() {
    EmcpalDpcCreate(EmcpalDriverGetCurrentClientObject(), 
                    &myDPC, StaticDPCHandlerBase, StaticDPCHandler, this, "BasicDPC"); 
}

inline BasicDeferredProcedureCall::~BasicDeferredProcedureCall() {
    EmcpalDpcDestroy(&myDPC);
}
inline void BasicDeferredProcedureCall::DeferredProcedureCallSchedule() {
    EmcpalDpcExecute(&myDPC);
}

inline void BasicDeferredProcedureCall::SetTargetProcessor(unsigned char processorNumber) {
    EmcpalDpcSetProcessor(&myDPC, processorNumber);
}


#endif  // __BasicDeferredProcedureCall__

