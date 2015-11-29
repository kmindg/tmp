//***************************************************************************
// Copyright (C) EMC Corporation 1989-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      DluTypes.h
//
// Contents:
//      description of functionality
//
// Internal:
//      yowza()
//
// Exported:
//
// Revision History:
//      5/7/1999    MWagner   Created
//      2/13/2004   CVaidya   Added DLU_LARGE_FAIL_FAST_DELAY_IN_MINUTES
//
//--

#ifndef _DLU_TYPES_H_
#define _DLU_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

//++
// Include Files
//--

//++
// Interface
//--

#include "k10ntddk.h"

#include "DlsKernelInterface.h"

#include "EmcPAL.h"

//++
//Variable:
//      DLU_LARGE_FAIL_FAST_DELAY_IN_MINUTES
//
// Description:
//      This value is used when the DistLockUtilOpenSemaphoreWithDelay()
//      gets called. If the caller specified a Fail Fast Delay value 
//      greater than this then we log it to the event viewer since a large
//      delay is likely to cause a sort of SP hang situation.
//      
//
// Revision History:
//      2/13/04   CVaidya    Created.
//
//--
#define  DLU_LARGE_FAIL_FAST_DELAY_IN_MINUTES      (0x06)
//.End DLU_LARGE_FAIL_FAST_DELAY_IN_MINUTES

//++
// Type:
//      DLU_SEMAPHORE
//
// Description:
//      A simple Semaphore
//
// Members:
//
//      EMCPAL_SEMAPHORE      dsCallBackSemaphore semaphore for
//                                                DluSemaphoreDlsCallback()
//
//      DLS_LOCK_NAME         dsDlsLockName       the name of the Semaphore
//                                                see DlsLockTypesExported.h
//
//      DLS_LOCK_HANDLE       dsDlsLockHandle     Underlying DLS Lock
//
//      BOOLEAN               dsSemaphoreHolder   Does this engine hold the Semaphore?
//
//      PVOID                 dsBackPointer       a pointer back to the 
//                                                structure that contains the
//                                                semaphore- it's up to the caller 
//                                                to set this (used for debugging)
//      csx_p_tmut_t          dsLocalTicketMutex  converted the local semaphore to a ticket
//                                                mutex in csx to speed up processing
//      EMCPAL_THREAD_ID      dsThreadID          TO aid in debugging panics the thread ID of the 
// 												  caller requesting the lock
//
// Revision History:
//      5/7/1999   MWagner    Created.
//
//--
typedef struct _DLU_SEMAPHORE
{
      EMCPAL_SEMAPHORE      dsCallBackSemaphore;
      DLS_LOCK_HANDLE       dsDlsLockHandle;
      DLS_LOCK_NAME         dsDlsLockName;
      BOOLEAN               dsSemaphoreHolder;
      PVOID                 dsBackPointer;
      csx_p_tmut_t 			dsLocalTicketMutex;
      EMCPAL_THREAD_ID 		dsThreadID;
} DLU_SEMAPHORE, *PDLU_SEMAPHORE;
//.End                                                                        

//++
// Type:
//      DLU_SYNC_LOCK
//
// Description:
//      A synchronous Dls Lock wrapper
//
// Members:
//      EMCPAL_SEMAPHORE     dslLocalSemaphore   serializes local operations on
//                                                the Dls Lock
//      EMCPAL_SEMAPHORE     dslCallBackSemaphore   allows the local thread to
//                                                wait on the callback                                                 the Dls Lock
//
//      DLS_LOCK_NAME         dslDlsLockName      the name of the Async Lock
//                                                see DlsLockTypesExported.h
//
//      DLS_LOCK_HANDLE       dslDlsLockHandle    Underlying DLS Lock
//
//      DLU_LOCK_MODE         dslLockMode         the local "mirror" of the Dls Lock
//                                                mode on this SP
//
//      DLS_LOCK_CALLBACKFUNC dslCallBack         a function called when a
//                                                ConvertConflict is received.
//
//      PVOID                 dslBackPointer      a pointer back to the 
//                                                structure that contains the
//                                                async lock- it's up to the caller 
//                                                to set this (used for debugging)
//
// Revision History:
//      9/29/1999   MWagner    Created.
//
//--
typedef struct _DLU_SYNC_LOCK
{
      EMCPAL_SEMAPHORE              dslLocalSemaphore;
      EMCPAL_SEMAPHORE              dslCallBackSemaphore;
      DLS_LOCK_HANDLE               dslDlsLockHandle;
      DLS_LOCK_NAME                 dslDlsLockName;
      DLS_LOCK_MODE                 dslLockMode;
      PDLS_LOCK_CALLBACKFUNC        dslCallBack;
      PVOID                         dslBackPointer;
} DLU_SYNC_LOCK, *PDLU_SYNC_LOCK;
//.End                                                                        


//++
// Type:
//      DLU_ASYNC_LOCK
//
// Description:
//      An asynchronous DLS lock wrapper
//
// Members:
//      EMCPAL_SEMAPHORE     dalLocalSemaphore   serializes local operations on
//                                                the Dls Lock
//
//      DLS_LOCK_NAME         dalDlsLockName      the name of the Async Lock
//                                                see DlsLockTypesExported.h
//
//      DLS_LOCK_HANDLE       dalDlsLockHandle    Underlying DLS Lock
//
//      DLU_ASYNC_LOCK_MODE   dalLockMode         the local "mirror" of the Dls Lock
//                                                mode on this SP
//
//      DLS_LOCK_CALLBACKFUNC dalCallBack         a function called when a
//                                                ConvertConflict is received, or
//                                                when a Dlu operation completes.
//
//      PVOID                 dalBackPointer      a pointer back to the 
//                                                structure that contains the
//                                                async lock- it's up to the caller 
//                                                to set this (used for debugging)
//
// Revision History:
//      9/29/1999   MWagner    Created.
//
//--
typedef struct _DLU_ASYNC_LOCK
{
      EMCPAL_SEMAPHORE              dalLocalSemaphore;
      DLS_LOCK_HANDLE               dalDlsLockHandle;
      DLS_LOCK_NAME                 dalDlsLockName;
      DLS_LOCK_MODE                 dalLockMode;
      DLS_LOCK_CALLBACKFUNC         dalCallBack;
      PVOID                         dalBackPointer;
} DLU_ASYNC_LOCK, *PDLU_ASYNC_LOCK;
//.End                                                                        

#ifdef __cplusplus
};
#endif

#endif // _DLU_TYPES_H_

// End DluTypes.h
