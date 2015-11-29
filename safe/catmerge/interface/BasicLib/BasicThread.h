//***************************************************************************
// Copyright (C) EMC Corporation 2005-2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

///////////////////////////////////////////////////////////
//  BasicThread.h
//  Definition of the BasicThread class.
//
//  Created on:      26-Jan-2005 4:52:11 PM
///////////////////////////////////////////////////////////

#if !defined(_BasicThread_h_)
#define _BasicThread_h_

extern "C" {
#include "k10ntddk.h"
};

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "BasicLib/BasicThreadExeLog.h"

// A BasicThread is an abstract base class that creates a thread that repeatedly calls the
// subclass's ThreadExecute() function.
//
// ThreadExecute() will be called periodically (the period is specified on construction)
// and the thread can be triggered to immediately call ThreadExecute() by a call to
// SignalThread(). Typically, ThreadExecute() services all of the work of the thread that
// has been queued since the last time the thread ran.
//
// The thread is not created on construction.  There are two approaches for creating the
// actual thread:
//  - Call InitializeThread() to create a new thread.  This thread will exist until
//    TerminateThread() is called or the destructor is called, at which point the
//    destructor will synchronously terminate the thread.
//  - Call UseCurrentThread() in the context of a thread to use.  UseCurrentThread() will
//    not return until TerminateThread() or the destructor is called, but either of these
//    approaches will not terminate the thread, but instead cause the thread to return.
//
class BasicThread {

public:
    // Make the thread call ThreadExecute() in the near future. If SignalThread() is
    // called twice in a row, ThreadExecute() may be called either once or twice.
    // It should be called at IRQL <= DISPATCH_LEVEL.
    void SignalThread();

    // Create the thread.
    //
    // Return Value:
    //   true if the thread was created, false if it could not be created
    virtual bool InitializeThread();

    // The calling thread is used to implement this object. The calling thread will not
    // return until this object is destructed.
    VOID UseCurrentThread();

    // Set the run-time priority of the thread.
    //
    // Return Value:
    //      Returns previous thread priority. A non-zero value indicates success.
    //      If the priority cannot be set, zero is returned
    //
    // @param newPriority - the new run-time priority of the thread
    EMCPAL_THREAD_PRIORITY SetPriority( EMCPAL_THREAD_PRIORITY newPriority );

    // Set the run-time priority and base priority of the thread.
    //
    // Return Value:
    //      Returns previous thread priority. A non-zero value indicates success.
    //      If the priority cannot be set, zero is returned
    //
    // @param newPriority - the new run-time priority of the thread
    EMCPAL_THREAD_PRIORITY SetPriorityandBasePriority( EMCPAL_THREAD_PRIORITY newPriority );

    // Set the run-time priority and base priority of the thread to the choosen realtime priority
    //
    // Return Value:
    //
    VOID SetPriorityToDataPathPriority();

    // Get the current priority of the thread.
    //
    // Return Value:
    //      current run-time priority of the thread
    EMCPAL_THREAD_PRIORITY GetPriority();

    // Set Affinity of thread to a particular CPU/set of CPUs.
    // May be done before InitializeThread is called.  The default
    // afinity is all existing CPUs.
    //
    // Return Value:
    //      None.
    // @param affMask - bit mask of cores the thread is allowed to run on.  0x1 => core 0; 0x18 core 3,4.
    void SetAffinity(EMCPAL_PROCESSOR_MASK affMask);

    //
    // Returns: The current affinity mask of the thread.
    EMCPAL_PROCESSOR_MASK GetAffinity() const { return mAffinity; }

    // Set the maximum time the thread should wait before waking.
    // Return Value:
    //      None.
    // @param sleepTime - maximum time in milliseconds before next call to ThreadExecute().
    //                    No maximum if zero.
    void SetWakeupInterval(EMCPAL_TIME_MSECS sleepTime) { mWakeupInterval = sleepTime; }

    // This function is called once by the thread as it starts running. The sub-class
    // may override to do any initalization desired.
    virtual void EventThreadStartup() {}

    // This function is called once by the thread prior to exiting. The sub-class
    // may override to do any cleanup desired.
    virtual void EventThreadExiting() {}

protected:
    // Constructs the object, but does not create the thread.
    //
    // @param maxTimeBetweenCalls - the maximum time between calls to ThreadExecute(), in
    //                              milliseconds. No maximum if zero.
    BasicThread(ULONG maxTimeBetweenCalls = 0);

    // Destructor.  Will mark the thread as needing termination, and wait for it to
    // terminate.
    virtual ~BasicThread();

    // The subclass must implement this function. This will be called once when the thread
    // is initialized, and then after a SignalThread call.
    //
    // @param timedOut - true indicates that ThreadExecute is being called because the
    //                   timeout expired, false indicates that SignalThread() triggered
    //                   the call.
    virtual void ThreadExecute(bool timedOut) = 0;

    // Tell the thread to terminate itself, but don't wait.
    void SetTerminate() { mTerminateThread = true; }

    // Reset terminate to false so the same BasicThread can be reused.
    void ClearTerminate() { mTerminateThread = false; }

    // Stop the thread, waiting for it to complete.
    //
    // @param waitForTermination - true indicates that the calling thread should wait
    //                             until the thread stops using this object.
    void TerminateThread(bool waitForTermination = true);

private:
    VOID  ThreadBody();

    static VOID  ThreadBody(void * context);

    // The event the thread waits for when it thinks there is no work to do.
    EMCPAL_SYNC_EVENT           mThreadWaitEvent;

    // The output from EmcpalThreadCreate.
    PEMCPAL_THREAD               mpThread;
    EMCPAL_THREAD                mThreadData;

        // true if a thread is running ThreadBody
    bool                        mThreadRunning;

    // Set to true to request the thread to stop.
    bool                        mTerminateThread;

    // The affinity that has been set for this thread.
    EMCPAL_PROCESSOR_MASK       mAffinity;

    // Maximum number of milliseconds between calls to ThreadExecute().
    EMCPAL_TIME_MSECS           mWakeupInterval;

protected:
    // Per-cpu log of BasicThread executions
    static BasicThreadPerCpuExeLog sExeLog;

    // Routines to record entries to the exe log
    virtual USHORT ExeLogStart (PVOID Data)
    {
        return (sExeLog.LogExeStart (this, Data));
    }
    virtual VOID ExeLogEnd (USHORT Index)
    {
        sExeLog.LogExeEnd (this, Index);
    }
};

class BasicRealTimeThread : public BasicThread {
public:
    BasicRealTimeThread(ULONG maxTimeBetweenCalls) : BasicThread(maxTimeBetweenCalls) { }
    BasicRealTimeThread() : BasicThread(0) { }

    // Create the thread with REALTIME priority
    //
    // Return Value:
    //   true if the thread was created, false if it could not be created
    bool InitializeThread() {
        if (BasicThread::InitializeThread()) {
            SetPriorityToDataPathPriority();
            return true;
        }
        return false;
    }
};
#endif // !defined(_BasicThread_h_)
