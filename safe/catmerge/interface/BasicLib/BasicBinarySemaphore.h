/************************************************************************
*
*  Copyright (c) 2002, 2005-2006 EMC Corporation.
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

#ifndef __BasicBinarySemaphore__
#define __BasicBinarySemaphore__
//++
// File Name:
//      BasicBinarySemaphore.h
//
// Contents:
//  Defines the BasicBinarySemaphore class.
//
//
// Revision History:
//--


# include "EmcPAL.h"
# include "EmcPAL_DriverShell.h"

// A BasicBinarySemaphore provides a locking mechanism that cooperating threads and use to
// ensure only one thread is execuating a critical section at a time.
class BasicBinarySemaphore {
public:
    inline BasicBinarySemaphore();
    inline ~BasicBinarySemaphore();

    // Get ownership of the semaphore, blocking indefinitely if the semaphore is held by
    // another thread.
    // Restrictions: No spinlocks may be held by the caller, and no interrupts should be
    // disabled.
    inline void Acquire();

    // Get ownership of the semaphore, with a strict upper bound on wait time.
    // @param timeoutSeconds - the wait time limit.  Waits longer than this time are
    //                         considered bugs and will crash the SP.
    // Restrictions: No spinlocks may be held by the caller, and no interrupts should be
    // disabled.
    inline void Acquire(ULONG timeoutSeconds);

    // Release ownership of the semaphore, allowing another thread to acquire it.
    inline VOID Release();

private:
    EMCPAL_SEMAPHORE  mSem;
    unsigned long     mCount;
};


inline BasicBinarySemaphore::BasicBinarySemaphore() {
    // init signalled with limit of 1 => one thread wakes up on signal
    EmcpalSemaphoreCreate(
        EmcpalDriverGetCurrentClientObject(),
        &mSem,
        "basicvolumedriver_BasicBinarySemaphore_sem",
        1 );
    mCount = 1;
}
inline BasicBinarySemaphore::~BasicBinarySemaphore() {
    EmcpalSemaphoreDestroy(&mSem);
}


inline void BasicBinarySemaphore::Acquire() 
{  
    EmcpalSemaphoreWait(&mSem, EMCPAL_TIMEOUT_INFINITE_WAIT);
    FF_ASSERT(mCount == 1); // Binary semaphore
    mCount--;
}

inline void BasicBinarySemaphore::Acquire(ULONG timeoutSeconds) 
{
    // Wait for a while, then crash if the semaphore is not available.
    EMCPAL_STATUS CSX_MAYBE_UNUSED status =  EmcpalSemaphoreWait(&mSem, timeoutSeconds*1000);
    FF_ASSERT(mCount == 1); // Binary semaphore
    mCount--;

    FF_ASSERT(EMCPAL_IS_SUCCESS(status) && status != EMCPAL_STATUS_TIMEOUT);
}


inline VOID BasicBinarySemaphore::Release()
{
    FF_ASSERT(mCount == 0); // Binary semaphore
    mCount++;
    EmcpalSemaphoreSignal(&mSem, 1);
}



#endif  // __BasicDeviceObject__

