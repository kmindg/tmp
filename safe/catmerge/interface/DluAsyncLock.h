#ifndef    _DLUASYNCLOCK_H_
#define    _DLUASYNCLOCK_H_

//***************************************************************************
// Copyright (C) Data General Corporation 1989-1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//++
// File Name:
//      DluAsyncLock.h
//
// Contents:
//      A DLU AsyncLock is a wrapper around the Dls Lock functionality that
//      correctly handles local thread synchronization.  THe DLU Async Lock
//      also permits "crossgrades" (i.e., conversions to the curent mode
//      are idempotent).
//
//      A DLU Async Lock may be opened, closed and converted. When a thread
//      on another SP requests the lock in a conflicting mode, the user
//      supplied callback function will be called.
//
//      The DLU Async Lock functions all take a PDLU_ASYNC_LOCK argument,
//      which must point to a DLU Async Lock in NonPaged memory.
//      DluAsyncLockDlsCallback() accesses some of the fields of the
//      DLU_ASYNC_LOCK, and as a callback may be executed in an arbitrary
//      thread context.
//
//      Each function holds a local semaphore while making the Dls call.
//      If tbe Dls call does not complete, that semaphore will be released 
//      callback routine. Other callers of the Async routines will pend
//      until that semaphore is released.
//
//      Since the DluAsyncLock functions are synchronized with 
//      KeWaitForSingleObject() they must be called at < DISPATCH_LEVEL.
//
//
//
// Exported:
//      DistLockUtilOpenAsyncLock()
//      DistLockUtilCloseAsyncLock()
//      DistLockUtilAcquireAsyncLock()
//      DistLockUtilReleaseAsyncLock()
//
// Revision History:
//      6/24/2002    spathak   Added a comment for DIMS 75094
//      9/29/1999    MWagner   Created
//
//--

#ifdef __cplusplus
extern "C" {
#endif

//++
// Include Files
//--

#include "k10ntddk.h"
#include "DluTypes.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//++
// End Includes
//--

//++
// Function:
//      DistLockUtilOpenAsyncLock
//
// Description:
//      DistLockUtilOpenAsyncLock() opens the underlying Dls Lock
//
// Arguments:
//      PDLU_ASYNC_LOCK           pAsyncLock    an initialized Async Lock
//      PDLS_LOCK_MAILBOX         pMailBox      an (optional) mailbox
//      LOGICAL                   flags
//
// Return Value:
//      STATUS_SUCCESS:  the  Lock was opened
//      XXX           :  error from DistLockServiceOpenLock()
//
// Revision History:
//      9/29/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilOpenAsyncLock(
                         PDLU_ASYNC_LOCK           pAsyncLock,
                         PDLS_LOCK_MAILBOX         pMailBox,
                         LOGICAL                   flags
                         );
// .End DistLockUtilOpenAsyncLock

//++
// Function:
//      DistLockUtilCloseAsyncLock
//
// Description:
//      DistLockUtilCloseAsyncLock()  closes the Dls Lock
//
// Arguments:
//      PDLU_ASYNC_LOCK           pAsyncLock    the lock to be close
//      LOGICAL                   flags         IGNORED
//
// Return Value:
//      STATUS_SUCCESS:  the  Lock was closed
//      XXX           :  error from DistLockServiceCloseLock()
//
// Revision History:
//      9/30/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilCloseAsyncLock(
                          PDLU_ASYNC_LOCK           pAsyncLock,
                          LOGICAL                   flags
                          );
// .End DistLockUtilCloseAsyncLock

//++
// Function:
//      DistLockUtilConvertAsyncLock
//
// Description:
//      DistLockUtilConvertAsyncLock()  tries convert the underlying Dls Lock to
//      the specified mode. If the lock is already held in that mode, this is 
//      a NOOP.
//      This function cannot be called at IRQL Dispatch Level. It should be called at
//      less than Dispatch level. 
//
// Arguments:
//      PDLU_ASYNC_LOCK           pAsyncLock
//      DLS_LOCK_MODE             mode
//      LOGICAL                   flags
//
// Return Value:
//      STATUS_SUCCESS:  The lock was converted
//      Others:   XXX return from DLS
//
// Revision History:
//      6/24/2002   spathak    Added a comment for DIMS 75094
//      9/30/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilConvertAsyncLock(
                            PDLU_ASYNC_LOCK           pAsyncLock,
                            DLS_LOCK_MODE             mode,
                            LOGICAL                   flags
                            );
// .End DistLockUtilConvertAsyncLock

//++
// Function:
//      DistLockUtilInitializeAsyncLock
//
// Description:
//      DistLockUtilInitializeAsyncLock() initializes an DLU_ASYNC_LOCK structure
//
// Arguments:
//    PDLU_ASYNC_LOCK           pAsyncLock  a user allocated DLU_ASYNC_LOCK
//    PDLS_LOCK_NAME            pDlsName    the name of the lock
//    PDLS_LOCK_CALLBACKFUNC    pfnCallBack a callback function
//    PVOID                     pBack       a user supplied pointer, usually
//                                          a pointer back to the structure
//                                          protected by the lock
//
// Return Value:
//      STATUS_SUCCESS:  the lock was initialized
//
// Revision History:
//      10/7/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilInitializeAsyncLock(
                               PDLU_ASYNC_LOCK           pAsyncLock,
                               PDLS_LOCK_NAME            pDlsName,
                               DLS_LOCK_CALLBACKFUNC     fnCallBack,
                               PVOID                     pBack
                               );
// .End DistLockUtilInitializeAsyncLock

#define DistLockUtilDestroyAsyncLock(a) EmcpalSemaphoreDestroy(&((a)->dalLocalSemaphore))

#ifdef __cplusplus
};
#endif

// End DluAsyncLock.h

#endif //  _DLUASYNCLOCK_H_
