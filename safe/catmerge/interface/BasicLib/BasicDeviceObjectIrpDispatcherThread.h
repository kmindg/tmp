//***************************************************************************
// Copyright (C) EMC Corporation 2005-2009,2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

///////////////////////////////////////////////////////////
//  BasicDeviceObjectIrpDispatcherThread.h Definition of the
//  BasicDeviceObjectIrpDispatcherThread class.
//
//  Created on:      26-Jan-2005 4:52:11 PM
///////////////////////////////////////////////////////////

#if !defined(_BasicDeviceObjectIrpDispatcherThread_h_)
#define _BasicDeviceObjectIrpDispatcherThread_h_
# include "BasicVolumeDriver/BasicVolume.h"
# include "BasicLib/BasicThread.h"
# include "BasicLib/BasicLockedObject.h"
# include "BasicLib/BasicCPUGroupPriorityMontitoredThreadCInterface.h"

// Thread is declared as stuck if it has not made the progress for
// this amount of time.
#if defined(SIMMODE_ENV)
#define STUCK_DETECTION_THRESHOLD   (15 * 1000 * 1000 * 60) // 15 minutes
#else
#define STUCK_DETECTION_THRESHOLD   (5 * 1000 * 1000) // 5 seconds
#endif

// Panic action if stuck
typedef enum _PanicIfStuckAction 
{
    PanicIfStuckKernelPanic = 0x0, 
    PanicIfStuckCsxPanic = 0x1,
} 
PanicIfStuckAction;

// The BasicDeviceObjectIrpDispatcherThread class is used to queue arriving IRPs to a
// thread for that thread to dispatch, rather than dispatching the IRP in the caller's
// context.  The IRPs must have arrived on a device that derives from BasicDeviceObject.
//
// Queueing a IRP to a thread triggers the thread to run.  The thread will call
// BasicDeviceObject::DispatchQueuedIRP() for each queued IRP.
//
// Irp completions from lower drivers may also be queued to this thread, but the queuer
// must specify the next function.  The typical paradigm is to have two non-member
// functions to service IRP completions.
// - the first simply queues the IRP to the thread, passing the address of the second
//   function when queueing
// - the second function implements the completion handler in the context of the thread.  
//
// The thread is not created on construction.  There are two approaches for creating the
// actual thread, as defined in BasicThread:
//  - Call InitializeThread() to create a new thread.  
//  - Call UseCurrentThread() in the context of a thread to use.  
// 
class BasicDeviceObjectIrpDispatcherThread: public BasicRealTimeThread, public BasicLockedObject {

public:
   // Constructs the object, but does not create the thread.
   // @param numQueues - Each thread can have a number of queues, and the thread
   //                    will drain m requests from the first queue, n requests from
   //                    from the second.  This allows items to be grouped to reduce 
   //                    cache footprint or to bias towards running more from one
   //                    group than the other.
   BasicDeviceObjectIrpDispatcherThread(UINT_32 numQueues = 2);

   // Destructor.  Will mark the thread as needing termination, and wait for it to
   // terminate.
   virtual ~BasicDeviceObjectIrpDispatcherThread();

    // Call BasicDeviceObject::DispatchQueuedIRP() in the context of the thread. This
    // function calls Irp->MarkPending(). The Irp's current
    // IO_STACK_LOCATION::DeviceObject->DeviceExtension is assumed to contain a
    // BasicDeviceObject.
    //
    // @param Irp - the Irp to queue to the thread.
    VOID QueueIrpToThread(PBasicIoRequest  Irp);

    /// Queues the IRP to be pased to the handler in the context of this CPU's priority montitored RealTime thread.
    /// The handler must not block the calling thread.
    /// @param qc - Queue in which to place the Irp.
    /// @param Irp - the Irp to queue to the thread.
    /// @param handler - the function to call in the thread's context
    /// @param Context - value to pass to handler
    VOID QueueIrpToThread(BasicWaiterQueuingClass qc, PBasicIoRequest Irp, VOID (*handler)(PEMCPAL_IRP, PVOID), PVOID Context);
  

    // Uses the next IRP stack location to queue the IRP, so that the completion routine
    // will be called in the thread context.
    //
    // @param Irp - the Irp to queue to the thread, and the second parameter to pass to
    //              CompletionRoutine
    // @param devObj - the first parameter to pass to CompletionRoutine
    // @param CompletionRoutine - the function to call in the thread context.  If NULL,
    //                            just call
    //                            BasicIoRequest::CompleteRequestCompletionStatusAlreadySet().
    // @param context - the third parameter to pass to CompletionRoutine
    void QueueIrpCompletionToThread(PBasicIoRequest  Irp, 
        PBasicIoCompletionRoutine CompletionRoutine =NULL, 
        PVOID context=NULL);

    // Calls Device->CallDriver(Irp) in the target thread's context.
    //
    // @param Irp - the Irp to queue to the thread, and the second parameter to pass to
    //              CompletionRoutine
    // @param Device - The device to send the IRP to.
    void QueueCallDriverToThread(PBasicIoRequest Irp, class BasicConsumedDevice * Device);

    // Queues a basic waiter so that the call to Signaled() is in the thread's context
    // rather than the current context.
    // @param waiter - the item to queue.
    // @param status - the value to pass to the Signaled function.
    void QueueWaiterToThread(BasicWaiterWithStatus * waiter, EMCPAL_STATUS status);
    void QueueWaiterToThread( BasicWaiter * waiter);

    // RunCount is incremented each time the thread scans the IRP and waiter
    // queues for work to do.  It is incremented whether work is found or not.
    // The primary use case for this information is to determine if a thread
    // has had the chance to run; if the count has changed then the thread
    // had to have run.
    int GetRunCount() {return mRunCount; }

    // Panics the SP if it is stuck.
    //
    // Note: Thread will be declared as stuck if it is not making forward progress
    //      for last pre-defined time.
    // @param timeout - maximum time not running in microseconds
    void PanicIfStuck(EMCPAL_TIME_USECS timeout = STUCK_DETECTION_THRESHOLD);

    // Returns the appoximate (unlocked) work enqueued, since last reset
    UINT_64 CountOfRecentWork() const { 
        return mTotalItemsAdded - mTotalItemsAtLastQuery; 
    }    
    
    // Logically sets count to 0.
    VOID ResetCountOfRecentWork() { 
        mTotalItemsAtLastQuery = mTotalItemsAdded; 
    }
    
    // Change the intended priority of this thread.
    void SetIdealPriority(EMCPAL_THREAD_PRIORITY priority) {mIdealPriority = priority;}

    // Check if this thread is running at its intended priority, 
    // if not try to set it to that value. 
    void SetPriorityToIdealPriority();

    // Set the priority of this thread to the value specified.
    void SetPriorityToTargetPriority(EMCPAL_THREAD_PRIORITY targetPriority);

    // Called on 'Signaled' for BasicWaiter objects.
    virtual USHORT ExeLogStart(PVOID Waiter);

protected:  
    // Implements the dequeue and dispatch.
    virtual void ThreadExecute(bool timedOut);

    // Called each time an item is dequeued, before its being worked on, however the
    // object called it the queue drainer, not the queue (they may be the same).
    // @param queue - the queue set that is being drained.
    virtual void ServicingNext(BasicDeviceObjectIrpDispatcherThread * queue);

    // Remove and signal items on the queue
    // @param waiterQueue - the specific queue to signal.
    // @param drainer - the instance of this class that is draining the queue
    // @param max - the maximum number of items to service in one shot.
    // @param burstDrain - the number of items to pull from the queue at once w/ lock held.
    //                     0 treated as 1, if burstDrain> max, max wins.
    // Returns:  the number of items actually serviced.
    UINT_32 DrainQueue(ListOfBasicWaiter & waiterQueue, BasicDeviceObjectIrpDispatcherThread * drainer, UINT_32 max, UINT_32 burstDrain=5);

    // Remove and signal items on any of the queues
    // @param drainer - the instance of this class that is draining the queue
    // @param maxPerQueue - the maximum number of items to service in one shot, and array indexed by queue number.  
    //                      If NULL, 1 for each queue.
    // @param burstDrain - the number of items to pull from the queue at once w/ lock held.
    //                     0 treated as 1, if burstDrain> max, max wins.
    // Returns:  the number of items actually serviced from all the sub-queues.
    UINT_32 DrainQueues( BasicDeviceObjectIrpDispatcherThread * drainer, UINT_32 * maxPerQueue, UINT_32 burstDrain=5);

    // The total number of items enqueued to this thread.
    UINT_64 mTotalItemsAdded;

    // The total number of items enqueued to this thread.
    UINT_64 mTotalItemsAtLastQuery;
       
    // Incremented each time the IRP and waiter queues are scanned.
    int  mRunCount;

    // The number of waiter queues that this thread services.
    UINT_32 mNumQueues; 

    // Index of the high priority queue.
    UINT_32 mHighPriorityQueue;   
    
    // An array of queues protected by this class's lock.
    ListOfBasicWaiter *                 mBasicWaiterQueue;

    // An array.  A count for each cooresponding queue, which is the
    // maximum that should be serviced 
    // from this queue at one time.   Subclass may modify these.
    UINT_32 *                           mMaxItemsPerQueue;

    // Timestamp to identify when this thread has last processed
    // the queue. If this thread is not doing anything then it will
    // be zero. It is valid only if we are executing ThreadExecute()
    // routine. It updates everytime thread dequeues the item
    // from the queue and before start processing it.
    EMCPAL_TIME_USECS mLastTimeProcessedQueue;

    // The queue that this thread is currently draining. 
    BasicDeviceObjectIrpDispatcherThread * mDraining;

    // A thread that we should signal when an item is added to the queue
    BasicDeviceObjectIrpDispatcherThread * mMyHelper;

    // Have we signaled for help since the queue was last empty?
    bool mHelperSignaled;

    // Number of items this thread serviced from other queue [0] or this queue [1].
    UINT_64              mNumServiced[2];

    // flag to signal thread to lower priority at next opportunity
    bool mLowerPriorityOnNextQueueItem;

    // The priority this thread wants to run at.
    volatile EMCPAL_THREAD_PRIORITY mIdealPriority;

    // Count of number of times MayContinueToRecurse has returned true since
    // this system thread has dequeued the waiter.
    UINT_64 mRecursionsSinceDequeue;

    // Last time we recursed in this system thread.
    EMCPAL_TIME_USECS mLastTimeRecursed;

    // Set to true when priority is boosted due to thread starving.
    bool mStarving;

    // So we can identify if we are running in this threads context.
    csx_thread_id_t     mThreadId;

public:
    // Panic action if stuck
    static PanicIfStuckAction sPanicAction;
    static BOOL sPanicActionInitialized;

};


#endif // !defined(_BasicDeviceObjectIrpDispatcherThread_h_)
