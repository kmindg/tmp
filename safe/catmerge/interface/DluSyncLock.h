#ifndef    _DLUSYNCLOCK_H_
#define    _DLUSYNCLOCK_H_

//***************************************************************************
// Copyright (C) EMC Corporation 1989-1999,2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      DluSyncLock.h
//
// Contents:
//      A DLU SyncLock is a wrapper around the Dls Lock functionality that
//      correctly handles local thread synchronization, and hides the 
//      underlying asynchrony of the Dls Lock operations.
//
//      A DLU Sync Lock may be opened, closed and converted. When a thread
//      on another SP requests the lock in a conflicting mode, the user
//      supplied callback function will be called.
//
//      The DLU Sync Lock functions all take a PDLU_SYNC_LOCK argument,
//      which must point to a DLU Sync Lock in NonPaged memory.
//      DluSyncLockDlsCallback() accesses some of the fields of the
//      DLU_SYNC_LOCK, and as a callback may be executed in an arbitrary
//      thread context.
//
//      The underlying asynchrony of the Distributed Lock Service is hidden
//      from the DLU SyncLock clients. Each DLU Sync Lock operation makes a 
//      Distributed Lock Service call, and is prepared to wait on the 
//      DLU_SYNC_LOCK's (local NT) dslCallBackSemaphore if the Distributed 
//      Lock Service operation returns a PENDING status code.
//
//      A (local NT) semaphore in the DLU_SYNC_LOCK is also used to serialize
//      thread access to the Dls Lock.
//
//      Since the DluSyncLock functions are synchronized with 
//      KeWaitForSingleObject() they must be called at < DISPATCH_LEVEL.
//
// Exported:
//      DistLockUitlOpenSyncLock()
//      DistLockUitlCloseSyncLock()
//      DistLockUitlInitializeSyncLock()
//      DistLockUitlDestroySyncLock()
//      DistLockUitlAcquireSyncLock()
//      DistLockUitlReleaseSyncLock()
//
// Revision History:
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
//      DistLockUtilOpenSyncLock
//
// Description:
//      DistLockUtilOpenSyncLock() opens the underlying Dls Lock
//
// Arguments:
//      PDLU_SYNC_LOCK           pSyncLock    an initialized Sync Lock
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
DistLockUtilOpenSyncLock(
                        PDLU_SYNC_LOCK           pSyncLock,
                        PDLS_LOCK_MAILBOX         pMailBox,
                        LOGICAL                   flags
                        );
// .End DistLockUtilOpenSyncLock

//++
// Function:
//      DistLockUtilCloseSyncLock
//
// Description:
//      DistLockUtilCloseSyncLock()  closes the Dls Lock
//
// Arguments:
//      PDLU_SYNC_LOCK           pSyncLock    the lock to be close
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
DistLockUtilCloseSyncLock(
                         PDLU_SYNC_LOCK           pSyncLock,
                         LOGICAL                   flags
                         );
// .End DistLockUtilCloseSyncLock

//++
// Function:
//      DistLockUtilDestroySyncLock
//
// Description:
//      DistLockUtilDestroySyncLock() releases system resources held 
//      by the Dls Lock
//
// Arguments:
//      PDLU_SYNC_LOCK           pSyncLock    the lock to be destroyed
//
// Return Value:
//      STATUS_SUCCESS:  the  Lock was destroy
//      XXX           :  error from DistLockServiceDestroyLock()
//
// Revision History:
//      4/3/2009   RChandler    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilDestroySyncLock(
                         PDLU_SYNC_LOCK           pSyncLock
                         );
// .End DistLockUtilDestroySyncLock

//++
// Function:
//      DistLockUtilConvertSyncLock
//
// Description:
//      DistLockUtilConvertSyncLock()  tries convert the underlying Dls Lock to
//      the specified mode. If the lock is already held in that mode, this is 
//      a NOOP.
//
// Arguments:
//      PDLU_SYNC_LOCK           pSyncLock
//      DLS_LOCK_MODE             mode
//      LOGICAL                   flags
//
// Return Value:
//      STATUS_SUCCESS:  the  program will halt
//      Others:   the program will not halt
//
// Revision History:
//      9/30/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilConvertSyncLock(
                           PDLU_SYNC_LOCK           pSyncLock,
                           DLS_LOCK_MODE             mode,
                           LOGICAL                   flags
                           );
// .End DistLockUtilConvertSyncLock

//++
// Function:
//      DistLockUtilInitializeSyncLock
//
// Description:
//      DistLockUtilInitializeSyncLock() initializes an DLU_SYNC_LOCK structure
//
// Arguments:
//    PDLU_SYNC_LOCK            pSyncLock   a user allocated DLU_SYNC_LOCK
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
DistLockUtilInitializeSyncLock(
                              PDLU_SYNC_LOCK            pSyncLock,
                              PDLS_LOCK_NAME            pDlsName,
                              PDLS_LOCK_CALLBACKFUNC    pfnCallBack,
                              PVOID                     pBack
                              );
// .End DistLockUtilInitializeSyncLock

#ifdef __cplusplus
};
#endif

// End DluSyncLock.h
#endif //  _DLUSYNCLOCK_H_
