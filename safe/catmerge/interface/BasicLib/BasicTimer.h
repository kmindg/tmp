/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2007,2009, 2011 EMC Corporation.
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

#ifndef __BasicTimer_h__
#define __BasicTimer_h__

//++
// File Name: BasicTimer.h
//    
// Contents:
//      Defines a generic BasicFixedRateTimer class.
//
// Revision History:
//      Michal Dobosz - Created.
//--

extern "C" {
# include "k10ntddk.h"
# include "ktrace.h"
};

# include "k10defs.h"
# include "K10SpinLock.h"
# include "SinglyLinkedList.h"
# include "DoublyLinkedList.h"

#include "EmcPAL.h"


#define SECOND_TO_MICROSECOND(x)   ((x) * 1000000)
#define MILLISECOND_TO_MICROSECOND(x)   ((x) * 1000)

class BasicFixedRateTimer;

// A BasicTimerElement is a base class which allows its subclass to queued to one
// BasicFixedRateTimer, where the BasicFixedRateTimer will call the virtual function
// BasicTimerElement::TimerTriggeredNoLocksHeld() when the timer expires, if the element
// has not been dequeued.
//
class BasicTimerElement {
public:

    // Creates a new BasicTimerElement.
    BasicTimerElement() : mAssociatedTimer(NULL), mFlink(NULL), mBlink(NULL), mDeadline(0), mTimestamp(0) {}

    // Allows the sub-class to set this BasicTimerElement's timestamp when its not being
    // timed.
    VOID SetTimestamp() { if (!mAssociatedTimer) { mTimestamp = EmcpalTimeReadMilliseconds(); } }

private:

    friend struct ListOfBasicTimerElementNoConstructor;
    friend struct DoublyLinkedListOfBasicTimerElement;
    friend class BasicFixedRateTimer;

    // Singly/Doubly linked list forward link field.
    BasicTimerElement     * mFlink;
    
    // Doubly linked list backward link field.
    BasicTimerElement     * mBlink;

    // The BasicFixedRateTimer that this element is being timed by.  NULL indicates no
    // timer is outstanding.
    BasicFixedRateTimer   * mAssociatedTimer;

    // When will this BasicTimerElement expire?
    EMCPAL_TIME_MSECS    mDeadline;

    // When did we start timing this BasicTimerElement?
    EMCPAL_TIME_MSECS    mTimestamp;

    // The sub-class must implement this function.
    //
    // This function is called if the timer elapses. At the time of the call, the element
    // has been removed from the timer's list, and is no longer associated with a timer.
    //
    // This callback is made with no locks held.  It is legal for this callback to add the
    // timer back to the clients callback list.
    //
    // @param whichTimer - the timer that this object was associated with.
    // @param seconds - the number of seconds since the element was added to the timer's
    //                  queue.
    virtual VOID TimerTriggeredNoLocksHeld(BasicFixedRateTimer * whichTimer, ULONG seconds) = 0;
};

DLListDefineListType(BasicTimerElement, DoublyLinkedListOfBasicTimerElement);

DLListDefineInlineCppListTypeFunctions(BasicTimerElement, DoublyLinkedListOfBasicTimerElement, mFlink, mBlink);

SLListDefineListType(BasicTimerElement, ListOfBasicTimerElement);

SLListDefineInlineCppListTypeFunctions(BasicTimerElement, ListOfBasicTimerElement, mFlink);

// A BasicFixedRateTimer has a single timer rate.  When a timer is started for a
// BasicTimerElement the time that the element's timer will trigger is simply the current
// time plus the timer rate. If the timer is not stopped for an element, the element's
// function TimerTriggeredNoLocksHeld() will be called (which also stops the timer).
//
// Note: It is legal for TimerTriggeredNoLocksHeld() to re-start the timer.
//
// There are two standard use patterns for BasicFixedRateTimer/BasicTimerElement:
//   - the timer is started then stopped, and the call to TimerTriggeredNoLocksHeld()
//     occurs under some exceptional condition.
//   - the timer is started, and the timer is normally allowed to trigger for the element
//
// Note: The fixed rate timer is a simple implemenation, since the timer expiration time
// is always later for elements that have the timer started later.  No sorting is
// required.
class BasicFixedRateTimer {
public:
    // Creates a new BasicFixedRateTimer.
    // @param timeoutInMicrosecond - The number of microseconds from when a timer is started until its 
    //                  TimerTriggeredNoLocksHeld() function  will be called.
    //                  If zero, Timeout must be called before any timer is started.
    // @param unit - It specifies unit of the value which is passed as timeOut. It can
    //                  be second, millisecond etc.
    BasicFixedRateTimer(ULONGLONG timeoutInMicrosecond = 0);
    BasicFixedRateTimer(char *name);

    // Destructs the BasicFixedRateTimer.
    ~BasicFixedRateTimer();

    // Replaces the timeout timeout value set on construction.  May be done only prior to
    // any timer being started.
    //
    // @param timeOut - The number of seconds that an element will sit on the mElements
    //                  list until it's TimerTriggered() function will be called.
    //
    VOID SetTimeout(ULONG timeOut);
    
    // This API allows the modification of a timeout value when mTimerActive  is true, but it is 
    // under precondition that it must be called only inside TimerTriggeredNoLocksHeld() within 
    // TimerDPC callback DPC context, by this, it could ensure to use new value to start up next
    // DPC timer without race condition.
    VOID SetTimeoutInTimerActive(ULONG timeOut);

    // Replaces the timeout value set on construction.  May be done only prior to
    // any timer being started.
    //
    // @param timeoutInMicrosecond - The number of microseconds that an element will sit on 
    //                  the mElements list until it's TimerTriggered() function will be called.
    //
    VOID SetTimeoutMicroSecond(EMCPAL_TIME_USECS timeoutInMicrosecond);

    // Returns: The number of microseconds that an element will sit on 
    //                  the mElements list until it's TimerTriggered() function will be called.
    EMCPAL_TIME_USECS GetTimeoutMicroSeconds() const { return mTimeout * 1000; }

    // Starts timing the element.  An element may have only one timer running at a time.
    // 
    // Impelementation: Timestamps an arbitrary element and adds it to the list of
    // elements. Starts the KTIMER if it hasn't been started yet.  The caller may hold
    // arbitrary other spin locks (but not mLock).
    //
    // @param element - The element to start the timer on.
    //
    VOID StartTimerLocksMayBeHeld(BasicTimerElement & element);

    // Stops timing the BasicTimerElement.
    //
    // Implementation: Removes the element from the timer list. Let the KTIMER expire on
    // its own, because this is cheaper than constantly canceling and restarting it.
    //
    // The caller may hold arbitrary other spin locks (but not mLock).
    //
    // @param elementToRemove - the element to stop the timer on.
    //
    // Returns: true if element was removed, false if element is not on the timer list.
    // Note: A false return may indicate that
    // BasicTimerElement::TimerTriggeredNoLocksHeld() being called or about to be called.
    // Care should be taken to handle this race properly.
    bool StopTimerLocksMayBeHeld(BasicTimerElement & elementToRemove);

    // Stops timing the BasicTimerElement.
    //
    // Implementation: Removes the element from the timer list. Let the KTIMER expire on
    // its own, because this is cheaper than constantly canceling and restarting it.
    // Ensures that the element is not about to be called or in the process 
    // of being called prior to returning.  This requires that the caller NOT hold
    // any locks that the timer function would try to acquire.
    //
    // @param elementToRemove - the element to stop the timer on.
    //
    // Note: On return when the element was not found, this function ensures, via somewhat expensive means,
    // that BasicTimerElement::TimerTrigger() is not about to be called or in the 
    // process of being called.  Therefore this call should be avoided when the element is
    // not actually waiting for a timer.
    void StopTimerNoLocksHeld(BasicTimerElement & elementToRemove);


protected:

    // Name used to identify this timer
    char                                    *mName;

    // Protects this object's data structures.
    K10SpinLock                             mLock;

    // The number of ticks that an element will sit on the mElements list until it's
    // TimerTriggered() function will be called.
    EMCPAL_TIMEOUT_MSECS                    mTimeout;

    // The timer that starts the DPC.
    EMCPAL_MONOSTABLE_TIMER	            mTimer;

    // Is the timer active?
    bool                                    mTimerActive;

    // is the timer callback function executing now?
    bool                                    mTimerExecuting;

    // A doubly linked list of BasicTimerElements.
    //
    // Note: The oldest element is always at the head of the list.
    DoublyLinkedListOfBasicTimerElement     mElements;

private:

    // Acquire the spin lock that protects all of the members of a BasicFixedRateTimer.
    // Note: Used when running at IRQL <= DISPATCH_LEVEL.
    VOID AcquireSpinLock() { mLock.Acquire(); }

    // Release the spin lock that protects all of the members of a BasicFixedRateTimer.
    // Note: Used when the lock was acquired at IRQL <= DISPATCH_LEVEL.
    VOID ReleaseSpinLock() { mLock.Release(); }

    // Acquire the spin lock that protects all of the members of a BasicFixedRateTimer.
    // Note: Used when running at IRQL == DISPATCH_LEVEL.
    VOID AcquireSpinLockAtDpc() { mLock.AcquireAtDpc(); }

    // Release the spin lock that protects all of the members of a BasicFixedRateTimer.
    // Note: Used when the lock was acquired at IRQL == DISPATCH_LEVEL.
    VOID ReleaseSpinLockFromDpc() { mLock.ReleaseFromDpc(); }

    // Starts the KTIMER if it has not already been started.
    //
    VOID StartTimerDPCLockHeld(EMCPAL_TIME_MSECS timeoutValue);

    // C interface for DPC execution.
    //
    // @param deferredContext - the BasicFixedRateTimer this DPC has been scheduled for.
    static VOID TimerDPC(EMCPAL_TIMER_CONTEXT deferredContext);

    // The real body of the Timer DPC.
    VOID TimerDPC();
};

typedef BasicFixedRateTimer * PBasicTimer;

#endif // __BasicTimer_h__
