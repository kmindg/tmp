#ifndef __K10SpinLock_h__
#define __K10SpinLock_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2001-20013 
// All rights reserved. 
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      K10SpinLock.h
//
// Contents:
//      K10SpinLock class 
//
// Revision History:
//  06-15-2001  D. Harvey   Created.
//  06-26-2007  M. Dobosz   Added support DPC level spin locks.
//
//--

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "K10Assert.h"
#include "ktrace.h"  


# if DBG
# define K10_LOCK_DEBUG_CODE(code) code
# else
# define K10_LOCK_DEBUG_CODE(code)
# endif

//++
// Type:
//      K10SpinLock
//
// Description:
//      Structure used to encapsulate an NT spin lock together with a container for the
//      saved IRQL value that is necessary to release the lock.
//
// Operations:

//
// Notes:
//      Lock Acquire/Release's must be strictly paired.  
//
//      In the normal case, if a second lock is acquired while a first is held, then the
//      second lock is released first. In some cases, it is desired to release locks in a
//      different order. For the simple case where a first lock must be held only until
//      the second lock is acquired (e.g., either lock ensures object existence),
//      AcquireRelease() should be used.  If the user of this package needs to execute
//      code inbetween the Acquire of the second lock and the release of the first,
//      OutOfOrderRelease() should be used.
//
// Access:
//      Must be allocated from NON-PAGED POOL Available when IRQL <= DISPATCH_LEVEL
//--
typedef class K10SpinLock
{
public:     
    // Acquire() - get the lock, save the current IRQL, and set the IRQL to
    // DISPATCH_LEVEL.
    inline VOID Acquire();

    inline VOID AssertLockHeld();

    // AcquireAtDpc() - gets the lock when already at DISPATCH_LEVEL.
    inline VOID AcquireAtDpc();

    // Try to acquire Spinlock - 
    // if able to get the lock, save the current IRQL, set the IRQL to DISPATCH_LEVEL and return true.
    // if not able to get the lock return false
    inline BOOL TryAcquire();

    // Properly maintain IRQLs in two K10SpinLock objects when releasing the locks in the
    // order acquired, rather than the more normal case of releasing inner locks before
    // outer locks.
    //
    // @param remainingLock - the lock that will remain held.
    //
    // "*this" will be released by this function.
    //
    // When we got the first lock, we got the IRQL and raised it. When we got the second
    // lock, we stored the raised IRQL.  If we release the locks out of order, then we
    // want to leave the IRQL high.
    inline VOID OutOfOrderRelease(K10SpinLock & remainingLock);

    // An alternative (simpler) method for doing out of order releases. Aquires "*this",
    // then releases the other lock.
    //
    // @param lockToRelease - the lock to release after this lock is acquired.
    //
    inline VOID AcquireRelease(K10SpinLock & lockToRelease);

    // Release() - release the lock, and set the IRQL back to its value prior to the
    // acquire.
    inline VOID Release();

    // ReleaseFromDpc() - releases the lock when already at DISPATCH_LEVEL.
    inline VOID ReleaseFromDpc();

    inline K10SpinLock(csx_cstring_t name = "unk_K10SpinLock");
    inline ~K10SpinLock();

private:
    EMCPAL_SPINLOCK         mLock;    
    EMCPAL_KIRQL                   mIrql; 
    K10_LOCK_DEBUG_CODE(bool mDebugIsHeld;)
} *PK10SpinLock;
//.End

# if !defined(I_AM_DEBUG_EXTENSION) && !defined(UMODE_ENV) && !defined(SIMMODE_ENV) && !defined(I_WONT_BE_USING_SPINLOCKS_THANK_YOU)

inline VOID K10SpinLock::Acquire()
{

    //
    //  Use a local variable, not the lock structure, for the IRQL when acquiring it. 
    //  It's not necessarily safe to touch the saved IRQL field in the structure until
    //  we've finished acquiring the lock.
    //
    EmcpalSpinlockLock(&mLock);
    DBG_ASSERT(!mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;)
}

inline VOID K10SpinLock::AcquireAtDpc()
{
    EmcpalSpinlockLockSpecial(&mLock);
    DBG_ASSERT(!mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;)
}

inline BOOL K10SpinLock::TryAcquire()
{
    BOOL acquired = EmcpalSpinlockTryLock(&mLock);
    if(acquired) 
    {
        DBG_ASSERT(!mDebugIsHeld);
        K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;)
    }
    return acquired;
}
inline VOID K10SpinLock::OutOfOrderRelease(K10SpinLock & remainingLock)
{
    DBG_ASSERT(remainingLock.mDebugIsHeld);
    DBG_ASSERT(mDebugIsHeld);

    DBG_ASSERT(&remainingLock != this);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = FALSE;)
    EmcpalSpinlockUnlockOutOfOrder(&mLock, &remainingLock.mLock);
}

inline VOID K10SpinLock::AcquireRelease(K10SpinLock & lockToRelease)
{
    DBG_ASSERT(lockToRelease.mDebugIsHeld);
    DBG_ASSERT(&lockToRelease != this);

    EmcpalSpinlockLockSpecial(&mLock);  
    DBG_ASSERT(!mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;);

    K10_LOCK_DEBUG_CODE(lockToRelease.mDebugIsHeld = false;);

    EmcpalSpinlockUnlockOutOfOrder(&lockToRelease.mLock, &mLock);
}

inline VOID K10SpinLock::Release()
{
    DBG_ASSERT(mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
    EmcpalSpinlockUnlock(&mLock);
}

inline VOID K10SpinLock::ReleaseFromDpc()
{
    DBG_ASSERT(mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
    EmcpalSpinlockUnlockSpecial(&mLock);
}

inline VOID K10SpinLock::AssertLockHeld()
{
    DBG_ASSERT(mDebugIsHeld);
}

inline K10SpinLock::K10SpinLock(csx_cstring_t name)
{
    EmcpalZeroMemory((void*) &mLock,sizeof(mLock));
    EmcpalSpinlockCreate(EmcpalDriverGetCurrentClientObject(), &mLock, name);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
}

inline K10SpinLock::~K10SpinLock()
{
    EmcpalSpinlockDestroy(&mLock);
}

# elif (defined (UMODE_ENV) || defined(SIMMODE_ENV) || defined (I_AM_DEBUG_EXTENSION)) && !defined(I_WONT_BE_USING_SPINLOCKS_THANK_YOU)

inline VOID K10SpinLock::Acquire() 
{

    //
    //  Use a local variable, not the lock structure, for the IRQL when acquiring it. 
    //  It's not necessarily safe to touch the saved IRQL field in the structure until
    //  we've finished acquiring the lock.
    //
    EmcpalSpinlockLock(&mLock);
    DBG_ASSERT(!mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;)
}

inline VOID K10SpinLock::AcquireAtDpc()
{
    EmcpalSpinlockLockSpecial(&mLock);
    DBG_ASSERT(!mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;)
}

inline BOOL K10SpinLock::TryAcquire()
{
    BOOL acquired = EmcpalSpinlockTryLock(&mLock);
    if(acquired) 
    {
        DBG_ASSERT(!mDebugIsHeld);
        K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;)
    }
    return acquired;
}

inline VOID K10SpinLock::OutOfOrderRelease(K10SpinLock & remainingLock) 
{
    DBG_ASSERT(remainingLock.mDebugIsHeld);
    DBG_ASSERT(mDebugIsHeld);

    DBG_ASSERT(&remainingLock != this);

    K10_LOCK_DEBUG_CODE(mDebugIsHeld = FALSE;)
    EmcpalSpinlockUnlockOutOfOrder(&mLock, &remainingLock.mLock);
}

inline VOID K10SpinLock::AcquireRelease(K10SpinLock & lockToRelease) 
{
    DBG_ASSERT(lockToRelease.mDebugIsHeld);
    DBG_ASSERT(&lockToRelease != this);

    EmcpalSpinlockLockSpecial(&mLock);  
    DBG_ASSERT(!mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = true;);

    K10_LOCK_DEBUG_CODE(lockToRelease.mDebugIsHeld = false;);

    EmcpalSpinlockUnlockOutOfOrder(&lockToRelease.mLock, &mLock);
}

inline VOID K10SpinLock::Release() 
{
    DBG_ASSERT(mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
    EmcpalSpinlockUnlock(&mLock);
}

inline VOID K10SpinLock::ReleaseFromDpc()
{
    DBG_ASSERT(mDebugIsHeld);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
    EmcpalSpinlockUnlockSpecial(&mLock);
}

inline VOID K10SpinLock::AssertLockHeld()
{
    DBG_ASSERT(mDebugIsHeld);
}

inline K10SpinLock::K10SpinLock(csx_cstring_t name) 
{
    EmcpalZeroMemory((void*) &mLock,sizeof(mLock));
    EmcpalSpinlockCreate(EmcpalDriverGetCurrentClientObject(), &mLock, name);
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
}

inline K10SpinLock::~K10SpinLock() 
{
    EmcpalSpinlockDestroy(&mLock);
}

#else
inline VOID K10SpinLock::Acquire() {}
inline VOID K10SpinLock::OutOfOrderRelease(K10SpinLock & remainingLock) {}
inline VOID K10SpinLock::AcquireRelease(K10SpinLock & lockToRelease) {}
inline BOOL K10SpinLock::TryAcquire() {return true;}
inline VOID K10SpinLock::Release() {}
inline VOID K10SpinLock::AssertLockHeld() {}
inline VOID K10SpinLock::AcquireAtDpc() {}
inline VOID K10SpinLock::ReleaseFromDpc() {}
inline K10SpinLock::K10SpinLock(csx_cstring_t name) 
{
    CSX_UNREFERENCED_PARAMETER(name);
    //fixed Coverity warning: dims# 243054
    memset(&mLock,0,sizeof(mLock));
    K10_LOCK_DEBUG_CODE(mDebugIsHeld = false;)
}
inline K10SpinLock::~K10SpinLock() 
{
}

#endif

typedef class K10Mutex
{
public:
    inline VOID Acquire();

    inline VOID Release();

    inline K10Mutex();
    inline ~K10Mutex();

private:
    EMCPAL_MUTEX         mLock;
} *PK10Mutex;
//.End

inline VOID K10Mutex::Acquire()
{
    EmcpalMutexLock(&mLock);
}

inline VOID K10Mutex::Release()
{
    EmcpalMutexUnlock(&mLock);
}

inline K10Mutex::K10Mutex()
{
    EmcpalMutexCreate(EmcpalDriverGetCurrentClientObject(), &mLock, "unk_K10Mutex");
}

inline K10Mutex::~K10Mutex()
{
    EmcpalMutexDestroy(&mLock);
}

#endif // __K10SpinLock_h__
