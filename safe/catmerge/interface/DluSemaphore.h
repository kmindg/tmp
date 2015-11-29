#ifndef    _DLUSEMAPHORE_H_
#define    _DLUSEMAPHORE_H_

//***************************************************************************
// Copyright (C) Data General Corporation 1989-1999
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      DluSemaphore.c
//
// Contents:
//      Implements a simple Semaphore on top of DLS Locks
//
//      The DLU  Semaphore provides a simple semaphore interface to the
//      K10 Distributed Lock Service. A DLU Semaphore may be opened,
//      waited for, released, and closed.
//
//      The DLU Semaphore functions all take a PDLU_SEMAPHORE argument,
//      which must point to a DLU Semaphore in NonPaged memory.
//      DluSemaphoreDlsCallback() accesses some of the fields of the
//      DLU_SEMAPHORE, and as a callback may be executed in an arbitrary
//      thread context.
//
//      The underlying asynchrony of the Distributed Lock Service is hidden
//      from the DLU Semaphore clients. DLU Semaphore operations are all
//      synchronous. Each DLU Semaphore operation makes a Distributed Lock
//      Service call, and is prepared to wait in the DLU_SEMAPHORE's
//      (local NT) dsCallBackSemaphore if the Distributed Lock Service
//      operation returns a PENDING status code.  If the Distributed Lock
//      Operation returns a PENDING status code, DluSemaphoreDlsCallback()
//      will release the dsCallBackSemaphore.
//
//      A local NT semaphore in the DLU_SEMAPHORE "shadows" the DLU_SEMAPHORE.
//      This serializes access to the DLU_SEMAPHORE, such that a thread on
//      an SP that holds a DLU_SEMAPHORE is synchronized not only with any
//      threads on the other SP, but also with threads its own SP.
//
//
// Exported:
//
//      DistLockUtilOpenSemaphore()
//      DistLockUtilCloseSemaphore()
//      DistLockUtilWaitForSemaphore()
//      DistLockUtilReleaseSemaphore()
//
// Revision History:
//      5/7/1999    MWagner   Created
//      2/12/2004   CVaidya   Added DistLockUtilOpenSemaphoreWithDelay()
//
//--

//++
// Include Files
//--

#ifdef __cplusplus
extern "C" {
#endif

//++
// Interface
//--

#include "k10ntddk.h"
#include "DluTypes.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//++
// End Includes
//--

//++
// Function:
//      DistLockUtilOpenSemaphore
//
// Description:
//      DistLockUtilOpenSemaphore()    Opens the underlying DLS Lock.
//
//
// Arguments:
//      PDLU_SEMAPHORE    pSemaphore - initialized  DLU Semaphore
//      LOGICAL           flags
//
// Return Value:
//      STATUS_SUCCESS:  the  Semaphore was opened, it can now be used
//      Others:   
//
// Revision History:
//      5/7/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilOpenSemaphore(
                         PDLU_SEMAPHORE     pSemaphore,
                         LOGICAL            flags
                         );
// .End DistLockUtilOpenSemaphore

//++
// Function:
//      DistLockUtilCloseSemaphore
//
// Description:
//      DistLockUtilCloseSemaphore()  waits on the local semaphore
//                                    (IRQL <= DISPATCH_LEVEL), and
//                                    then closes the underlying DLS
//                                    lock
//
// Arguments:
//      PDLU_SEMAPHORE     pSemaphore
//      LOGICAL            flags
//
// Return Value:
//      STATUS_SUCCESS:  the  Dls Lock was closed, the Dlu Semaphore
//                       is no longer usable.
//      Others:   
//
// Revision History:
//      5/7/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilCloseSemaphore(
                          PDLU_SEMAPHORE     pSemaphore,
                          LOGICAL            flags
                          );
// .End DistLockUtilCloseSemaphore

//++
// Function:
//      DistLockUtilWaitForSemaphore
//
// Description:
//      DistLockUtilWaitForSemaphore()  Waits on the local Semaphore,
//                                      (IRQL <= DISPATCH_LEVEL)
//                                      then attempts to get the underlying 
//                                      Dls lock in Write mode. If 
//                                      DLU_SEMAPHORE_PEER_NO_WAIT is specified,
//                                      then DistLockServiceConvertLock() will 
//                                      be passed DLS_CLIENT_CONVERT_LOCK_NO_WAIT 
//
//                                      If the DLS call is not successful, the
//                                      local semaphore is released.
//                                      (IRQL <= DISPATCH_LEVEL)
//
//
//
// Arguments:
//                            PDLU_SEMAPHORE     pSemaphore,
//                            LOGICAL            flags
//
// Return Value:
//      STATUS_SUCCESS:  the Semaphore is now held by the calling thread.
//      DLU_STATUS_WARNING_SEMAPHORE_LOCK_CONVERT_DENIED:
//                       the Semaphore could not be obtained without waiting
//
// Revision History:
//      5/17/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilWaitForSemaphore(
                            PDLU_SEMAPHORE     pSemaphore,
                            LOGICAL            flags
                            );

// .End DistLockUtilWaitForSemaphore

//++
// Function:
//      DistLockUtilReleaseSemaphore
//
// Description:
//      DistLockUtilReleaseSemaphore() converts the underlying Dls lock to Null mode. 
//                                     Then it releases the local NT semaphore.
//                                     (IRQL <= DISPATCH_LEVEL)
//
// Arguments:
//                            PDLU_SEMAPHORE     pSemaphore,
//                            LOGICAL            flags
//
// Return Value:
//      STATUS_SUCCESS:  the Semaphore was released
//      Others:   
//
// Revision History:
//      5/17/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilReleaseSemaphore(
                            PDLU_SEMAPHORE     pSemaphore,
                            LOGICAL            flags
                            );

// .End DistLockUtilReleaseSemaphore
//++
// Function:
//      DistLockUtilInitializeSemaphore
//
// Description:
//      DistLockUtilInitializeSemaphore()  initializes a DLU Semaphore
//
// Arguments:
//    PDLU_SEMAPHORE           pSemaphore a user allocated DLU_SEMAPHORE
//    PDLS_LOCK_NAME           pDlsName   the name of the semaphore
//    PVOID                    pBac       a user supplied pointer, usually
//                                        a pointer back to the structure
//                                        protected by the lock
//
// Return Value:
//      STATUS_SUCCESS:  the  program will halt
//      Others:   the program will not halt
//
// Revision History:
//      10/8/1999   MWagner    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilInitializeSemaphore(
    PDLU_SEMAPHORE           pSemaphore,
    PDLS_LOCK_NAME           pDlsName,
    PVOID                    pBack
           );
// .End DistLockUtilInitializeSemaphore

//++
// Function:
//      DistLockUtilOpenSemaphoreWithDelay
//
// Description:
//      DistLockUtilOpenSemaphoreWithDelay()    Opens the underlying DLS Lock.
//      This is similar to DistLockUtilOpenSemaphore() but allows the user to
//      specify the DLU "Fail Fast" delay value in minutes.
//      WARNING!!! This function was specifically created to be used by Flare/CM.
//      Specifying an infinite or a large delay value could cause/appear to cause
//      SP(s)to hang.
//      It is advisable that anyone wanting to use this function first check with 
//      the concerned DLS person.
// Arguments:
//      PDLU_SEMAPHORE    pSemaphore - initialized  DLU Semaphore
//      LOGICAL           flags
//      ULONG             FailFastDelayInMinutes
// Return Value:
//      STATUS_SUCCESS:  the  Semaphore was opened, it can now be used
//      Others:   
//
// Revision History:
//      2/12/2004   CVaidya    Created.
//
//--
EMCPAL_STATUS
CSX_MOD_EXPORT
DistLockUtilOpenSemaphoreWithDelay(
                         PDLU_SEMAPHORE     pSemaphore,
                         LOGICAL            flags,
                         ULONG              FailFastDelayInMinutes
                         );
// .End DistLockUtilOpenSemaphoreWithDelay

#ifdef __cplusplus
};
#endif

// End DluSemaphore.h

#endif //  _DLUSEMAPHORE_H_
