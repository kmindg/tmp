#if !defined(_BvdSimSynchronizers_h)
#define _BvdSimSynchronizers_h

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *     This file defines the functional interface to Windows functions
 *     calling Windows user-mode synchronization primitives.
 *   
 *
 *  HISTORY
 *     4-Dec-2009     Created.   -Richard Chandler
 *
 *
 ***************************************************************************/

#include "generic_types.h"

// Create a mutex with the given name.  This mutex can be accessed across
// processes.
//
// @param name - A name for the mutex.
// Returns:
//   A VOID pointer to a mutex object if creation succeeds.  This mutex object
//     is opaque, but must be passed as a parameter to other mutex functions.
//   NULL if the creation fails (out of memory, etc.)
PVOID Windows_CreateMutex(char *name);

// Lock the mutex, waiting the given number if milliseconds.
//
// @param mutex - A VOID pointer to a mutex object returned by 
//                Windows_CreateMutex.
// @param timeout - Maximum length of time to wait, in milliseconds.
// Returns:
//   TRUE if the lock is successfully taken, FALSE otherwise.
BOOL Windows_LockMutex(PVOID mutex, ULONG timeout);

// Lock the mutex.  The wait interval is INFINITE.
//
// @param mutex - A VOID pointer to a mutex object returned by 
//                Windows_CreateMutex.
// Returns:
//   TRUE if the lock is successfully taken, FALSE otherwise.
BOOL Windows_LockMutexWaitForever(PVOID mutex);

// Release the mutex, having previously locked it.
//
// @param mutex - A VOID pointer to a mutex object returned by 
//                Windows_CreateMutex.
// Returns:
//   TRUE if the mutex is successfully released, FALSE otherwise.
BOOL Windows_ReleaseMutex(PVOID mutex);

// Close the mutex - close the underlying handle and release all resources.
//
// @param mutex - A VOID pointer to a mutex object returned by 
//                Windows_CreateMutex.
// Returns:
//   TRUE if the mutex is successfully released, FALSE otherwise.
BOOL Windows_CloseMutex(PVOID mutex);

// Create a Semaphore with the given name.  This Semaphore can be accessed across
// processes.
//
// @param name - A name for the Semaphore.
// Returns:
//   A VOID pointer to a Semaphore object if creation succeeds.  This Semaphore object
//     is opaque, but must be passed as a parameter to other Semaphore functions.
//   NULL if the creation fails (out of memory, etc.)
PVOID Windows_CreateSemaphore(char *name);

// Lock the Semaphore, waiting the given number if milliseconds.
//
// @param Semaphore - A VOID pointer to a Semaphore object returned by 
//                Windows_CreateSemaphore.
// @param timeout - Maximum length of time to wait, in milliseconds.
// Returns:
//   TRUE if the lock is successfully taken, FALSE otherwise.
BOOL Windows_LockSemaphore(PVOID Semaphore, ULONG timeout);

// Lock the Semaphore.  The wait interval is INFINITE.
//
// @param Semaphore - A VOID pointer to a Semaphore object returned by 
//                Windows_CreateSemaphore.
// Returns:
//   TRUE if the lock is successfully taken, FALSE otherwise.
BOOL Windows_LockSemaphoreWaitForever(PVOID Semaphore);

// Release the Semaphore, having previously locked it.
//
// @param Semaphore - A VOID pointer to a Semaphore object returned by 
//                Windows_CreateSemaphore.
// Returns:
//   TRUE if the Semaphore is successfully released, FALSE otherwise.
BOOL Windows_ReleaseSemaphore(PVOID Semaphore);

// Close the Semaphore - close the underlying handle and release all resources.
//
// @param Semaphore - A VOID pointer to a Semaphore object returned by 
//                Windows_CreateSemaphore.
// Returns:
//   TRUE if the Semaphore is successfully released, FALSE otherwise.
BOOL Windows_CloseSemaphore(PVOID Semaphore);

#endif
