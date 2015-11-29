/************************************************************************
*
*  Copyright (c) 2002, 2005-2010 EMC Corporation.
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

#ifndef __BasicCountingSemaphore__
#define __BasicCountingSemaphore__
//++
// File Name:
//      BasicCountingSemaphore.h
//
// Contents:
//  Defines the BasicCountingSemaphore class.
//
// A BasicCountingSemaphore provides a locking mechanism that cooperating threads and use to
// ensure only one thread is execuating a critical section at a time.
//
// Revision History:
//      02/05/2010 - Bhavesh Patel
//                              Implemented all the interfaces.
//--


#include "K10Assert.h"

class BasicCountingSemaphore {
public:
    BasicCountingSemaphore(ULONG count = 1);

    ~BasicCountingSemaphore();

    void Initialize(ULONG count=1);

    // Get ownership of the semaphore, blocking indefinitely if the semaphore is held by
    // another thread.
    // Restrictions: No spinlocks may be held by the caller, and no interrupts should be
    // disabled.
    void Acquire(ULONG count=1);

    // Get ownership of the semaphore, if possible without blocking.
    bool TryAcquire(ULONG count=1);

    // Release ownership of the semaphore, allowing another thread to acquire it.
    VOID Release(ULONG count=1);

private:
    EMCPAL_SEMAPHORE  mSem;
};

inline BasicCountingSemaphore::BasicCountingSemaphore(ULONG count)
{
    EmcpalSemaphoreCreate(
        EmcpalDriverGetCurrentClientObject(),
        &mSem,
        "basicvolumedriver_BasicCountingSemaphore_sem",
        count );
}

inline BasicCountingSemaphore::~BasicCountingSemaphore() 
{
    EmcpalSemaphoreDestroy(&mSem);
}

inline void BasicCountingSemaphore::Initialize(ULONG count) 
{

}

inline void BasicCountingSemaphore::Acquire(ULONG count) 
{
    // It tries to acquire requested number of semaphores. It will wait indefinitely if requested
    // number of semaphores is not available.
    for(ULONG i = 0; i < count; i++)
    {
        EmcpalSemaphoreWait(&mSem, EMCPAL_TIMEOUT_INFINITE_WAIT);
    }
}

inline bool BasicCountingSemaphore::TryAcquire(ULONG count) 
{
    //
    // It tries to acquire requested number of semaphores without waiting.
    // If any of them fail, it gives back all the ones it already got and
    // returns false to indicate the resources weren't available.
    //
    for(ULONG i = 0; i < count; i++)
    {
        EMCPAL_STATUS status = EmcpalSemaphoreTryWait(&mSem);
        if (EMCPAL_STATUS_TIMEOUT == status) {
            if (i > 0) {
                Release(i);
            }
            return false;
        }
    }

    return true;
}


inline VOID BasicCountingSemaphore::Release(ULONG count)
{
    // It releases given number of semaphores.
    EmcpalSemaphoreSignal(&mSem, count);
}

#endif  // __BasicCountingSemaphore__
