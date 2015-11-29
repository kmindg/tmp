#ifndef __dsos_h
#define __dsos_h

//***************************************************************************
// Copyright (C) EMC Corporation 2013
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//
// Data Services OS Abstraction (DSOS)
//
// This is a simple set of trivial wrappers for Mutexes/Events/Semaphores/Threads/Named-Pipes
// intended for use by data services user space code.
//
// It provides a HANDLE-based system for a set of objects representing 
// Mutexes/Events/Semaphores/Threads/Named-Pipes
// with an underlying implmentation that is either Win32 or CSX based
//

/**************************************************/

#ifdef ALAMOSA_WINDOWS_ENV
#define DSOS_USE_WINDOWS
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

#ifdef DSOS_USE_WINDOWS
#include "csx_ext.h"
#include <windows.h>
#define DSOS_ERROR_INVALID_PARAMETER ERROR_INVALID_PARAMETER
#define DSOS_ERROR_BROKEN_PIPE ERROR_BROKEN_PIPE
#define DSOS_ERROR_MORE_DATA ERROR_MORE_DATA
#define DSOS_ERROR_NO_DATA ERROR_NO_DATA
#define DSOS_ERROR_PIPE_BUSY ERROR_PIPE_BUSY
#define DSOS_ERROR_FILE_NOT_FOUND ERROR_FILE_NOT_FOUND
#define DSOS_INVALID_HANDLE_VALUE INVALID_HANDLE_VALUE
#else
#include "csx_ext.h"
#define DSOS_ERROR_INVALID_PARAMETER CSX_STATUS_INVALID_PARAMETER
#define DSOS_ERROR_BROKEN_PIPE CSX_STATUS_END_OF_FILE
#define DSOS_ERROR_MORE_DATA CSX_STATUS_BUFFER_OVERFLOW
#define DSOS_ERROR_NO_DATA CSX_STATUS_BUFFER_TOO_SMALL
#define DSOS_ERROR_PIPE_BUSY CSX_STATUS_BUSY
#define DSOS_ERROR_FILE_NOT_FOUND CSX_STATUS_OBJECT_NOT_FOUND
#define DSOS_INVALID_HANDLE_VALUE CSX_NULL
#endif                                     /* DSOS_USE_WINDOWS */

/**************************************************/

typedef DWORD DSOS_ERROR;
#ifdef DSOS_USE_WINDOWS
typedef LPTHREAD_START_ROUTINE DSOS_THREAD_BODY;
#else
/* *INDENT-OFF* */
typedef DWORD (*DSOS_THREAD_BODY)(PVOID);
/* *INDENT-ON* */
#endif                                     /* DSOS_USE_WINDOWS */
typedef PVOID DSOS_THREAD_CONTEXT;

typedef PVOID DSOS_HANDLE;
typedef DWORD DSOS_TIMEOUT_MSECS;

#ifdef __cplusplus
#define DSOS_API extern "C" CSX_GLOBAL
#else
#define DSOS_API CSX_GLOBAL
#endif
#define DSOS_CC __cdecl

/**************************************************/

DSOS_API VOID DSOS_CC DsosSetLastErrorValue(
    DSOS_ERROR Error);

DSOS_API DSOS_ERROR DSOS_CC DsosGetLastErrorValue(
    VOID);

DSOS_API csx_cstring_t DSOS_CC DsosGetLastErrorString(
    VOID);

/**************************************************/

DSOS_API BOOL DSOS_CC DsosHandleCloseMaybe(
    DSOS_HANDLE handle);

DSOS_API VOID DSOS_CC DsosHandleCloseAlways(
    DSOS_HANDLE handle);

/**************************************************/

#ifdef DSOS_USE_WINDOWS
DSOS_API HANDLE
DsosHandleGetNtHandle(
    DSOS_HANDLE handle);
#endif                                     /* DSOS_USE_WINDOWS */

DSOS_API VOID DSOS_CC DsosHandleWait(
    DSOS_HANDLE handle);

#ifdef DSOS_USE_WINDOWS
#define DSOS_WAIT_WORKED WAIT_OBJECT_0
#define DSOS_WAIT_TIMEDOUT WAIT_TIMEOUT
#define DSOS_WAIT_ABANDONED WAIT_ABANDONED
#else
#define DSOS_WAIT_WORKED CSX_STATUS_SUCCESS
#define DSOS_WAIT_TIMEDOUT CSX_STATUS_TIMEOUT
#define DSOS_WAIT_ABANDONED CSX_STATUS_LOCK_OWNER_DIED
#endif                                     /* DSOS_USE_WINDOWS */

DSOS_API DWORD DSOS_CC DsosHandleTimedWait(
    DSOS_HANDLE handle,
    DSOS_TIMEOUT_MSECS timeoutMsecs);

/**************************************************/

DSOS_API DSOS_HANDLE DSOS_CC DsosSemaphoreCreate(
    csx_cstring_t name,
    csx_nuint_t init_count,
    csx_nuint_t max_count);

DSOS_API BOOL DSOS_CC DsosSemaphorePostMaybe(
    DSOS_HANDLE handle);

DSOS_API VOID DSOS_CC DsosSemaphorePostAlways(
    DSOS_HANDLE handle);

/**************************************************/

DSOS_API DSOS_HANDLE DSOS_CC DsosAutoResetEventCreate(
    csx_cstring_t name);

DSOS_API BOOL DSOS_CC DsosAutoResetEventSignalMaybe(
    DSOS_HANDLE handle);

DSOS_API VOID DSOS_CC DsosAutoResetEventSignalAlways(
    DSOS_HANDLE handle);

/**************************************************/

DSOS_API DSOS_HANDLE DSOS_CC DsosManualResetEventCreate(
    csx_cstring_t name);

DSOS_API BOOL DSOS_CC DsosManualResetEventSignalMaybe(
    DSOS_HANDLE handle);

DSOS_API BOOL DSOS_CC DsosManualResetEventResetMaybe(
    DSOS_HANDLE handle);

DSOS_API VOID DSOS_CC DsosManualResetEventSignalAlways(
    DSOS_HANDLE handle);

DSOS_API VOID DSOS_CC DsosManualResetEventResetAlways(
    DSOS_HANDLE handle);

/**************************************************/

DSOS_API DSOS_HANDLE DSOS_CC DsosMutexCreate(
    csx_cstring_t name);

DSOS_API BOOL DSOS_CC DsosMutexUnlockMaybe(
    DSOS_HANDLE handle);

DSOS_API VOID DSOS_CC DsosMutexUnlockAlways(
    DSOS_HANDLE handle);

/**************************************************/

DSOS_API DSOS_HANDLE DSOS_CC DsosThreadCreate(
    csx_cstring_t name,
    DWORD stackSize,
    DSOS_THREAD_BODY threadBody,
    DSOS_THREAD_CONTEXT threadContext,
    DWORD * threadIdRv);

DSOS_API BOOL DSOS_CC DsosThreadTerminate(
    DSOS_HANDLE handle);

/**************************************************/

DSOS_API DSOS_HANDLE DSOS_CC DsosNamedPipeOpen(
    csx_cstring_t name);

DSOS_API DSOS_HANDLE DSOS_CC DsosNamedPipeCreate(
    csx_cstring_t name,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut);

DSOS_API BOOL DSOS_CC DsosNamedPipeConnect(
    DSOS_HANDLE handle);

DSOS_API BOOL DSOS_CC DsosNamedPipeWait(
    csx_cstring_t name,
    DSOS_TIMEOUT_MSECS timeoutMsecs);

DSOS_API BOOL DSOS_CC DsosNamedPipeRead(
    DSOS_HANDLE handle,
    VOID *buffer,
    DWORD length,
    DWORD * lengthReadRv);

DSOS_API BOOL DSOS_CC DsosNamedPipeWrite(
    DSOS_HANDLE handle,
    const VOID *buffer,
    DWORD length,
    DWORD * lengthWrittenRv);

DSOS_API BOOL DSOS_CC DsosNamedPipeFlush(
    DSOS_HANDLE handle);

DSOS_API BOOL DSOS_CC DsosNamedPipeDisconnect(
    DSOS_HANDLE handle);

/**************************************************/

#ifdef DSOS_USE_WINDOWS
typedef CRITICAL_SECTION DSOS_CRITICAL_SECTION;
typedef CRITICAL_SECTION DSOS_CRITICAL_SECTION_R;
#else
typedef csx_p_mut_t DSOS_CRITICAL_SECTION;
typedef csx_p_rmut_t DSOS_CRITICAL_SECTION_R;
#endif                                     /* DSOS_USE_WINDOWS */

DSOS_API VOID DSOS_CC DsosInitializeCriticalSection(
    DSOS_CRITICAL_SECTION * cs);

DSOS_API VOID DSOS_CC DsosDeleteCriticalSection(
    DSOS_CRITICAL_SECTION * cs);

DSOS_API VOID DSOS_CC DsosEnterCriticalSection(
    DSOS_CRITICAL_SECTION * cs);

DSOS_API VOID DSOS_CC DsosLeaveCriticalSection(
    DSOS_CRITICAL_SECTION * cs);

DSOS_API VOID DSOS_CC DsosInitializeCriticalSectionR(
    DSOS_CRITICAL_SECTION_R * cs);

DSOS_API VOID DSOS_CC DsosDeleteCriticalSectionR(
    DSOS_CRITICAL_SECTION_R * cs);

DSOS_API VOID DSOS_CC DsosEnterCriticalSectionR(
    DSOS_CRITICAL_SECTION_R * cs);

DSOS_API VOID DSOS_CC DsosLeaveCriticalSectionR(
    DSOS_CRITICAL_SECTION_R * cs);

DSOS_API BOOL DSOS_CC
DsosSetNamedPipeHandleState( DSOS_HANDLE handle,
    DWORD *lpMode,
    DWORD *lpMaxCollectionCount,
    DWORD *lpCollectDataTimeout);
/**************************************************/

#endif                                     /* __dsos_h */
