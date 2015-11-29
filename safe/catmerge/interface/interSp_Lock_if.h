#ifndef INTERSP_LOCK_IF_H
#define INTERSP_LOCK_IF_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  DESCRIPTION
 *      This file defines the client interface to the ASE
 *      inter-SP lock mechanism.
 *   
 *
 *  HISTORY
 *      17-Jan-2003     Created     Joe Shimkus
 *
 ***************************************************************************/

#include "generics.h"
#include "DluSyncLock.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#if defined(SIMMODE_ENV)
#include "EmcPAL.h"
#endif
#include "EmcUTIL_Resource.h"

/*
 * Define the DLL declaration.
 */
 
#ifdef  INTERSP_LOCK_EXPORT
#define INTERSP_LOCK_DLL        CSX_MOD_EXPORT
#else
#define INTERSP_LOCK_DLL        CSX_MOD_IMPORT
#endif // INTERSP_LOCK_EXPORT

/*
 * Constants.
 */

/*
 * INTERSP_LOCK_ACQUISITION_MODE defines the Inter-SP Lock acquisition modes.
 */
 
typedef enum _INTERSP_LOCK_ACQUISITION_MODE
{
    INTERSP_NO_WAIT,
    INTERSP_WAIT
} INTERSP_LOCK_ACQUISITION_MODE;
  
/*
 * INTERSP_LOCK_MODE defines the Inter-SP Lock modes.
 */
 
typedef enum _INTERSP_LOCK_MODE
{
    INTERSP_EXCLUSIVE,
    INTERSP_SHARED
} INTERSP_LOCK_MODE;

/*
 * The following define the Inter-SP Lock status codes.
 */
 
#define INTERSP_LOCK_SUCCESS        EMCPAL_STATUS_SUCCESS
#define INTERSP_LOCK_HELD           DLS_WARNING_LOCK_CONVERT_DENIED

/*
 * Data Types.
 */
 
/*
 * INTERSP_LOCK defines the Inter-SP Lock structure.
 */
 
typedef struct INTERSP_LOCK
{
    // These are for lockers on the same SP.
    // These are required since DLS isn't multi-thread capable.
#if defined(SIMMODE_ENV)
    EMCPAL_SYNC_EVENT  localSyncEvent;
#else
    EMCUTIL_RESOURCE localLock;
    EMCUTIL_RESOURCE lockerCountLock;
#endif
    UINT_32         lockerCount;
    
    // These are for lockers across SPs.
    DLU_SYNC_LOCK   dluLock;
    DLS_LOCK_NAME   dlsLockName;
    EMCPAL_STATUS        ntStatus;
} INTERSP_LOCK;

/*
 * INTERSP_LOCK_STATUS defines the Inter-SP Lock status type.
 */
 
typedef EMCPAL_STATUS           INTERSP_LOCK_STATUS; 

/*
 * Function interfaces.
 */

CSX_EXTERN_C INTERSP_LOCK_DLL
INTERSP_LOCK_STATUS     
        interSpLockCreateLock(  char                        *lockName,
                                INTERSP_LOCK                *lock);

CSX_EXTERN_C INTERSP_LOCK_DLL
INTERSP_LOCK_STATUS     
        interSpLockDestroyLock( INTERSP_LOCK    *lock);

CSX_EXTERN_C INTERSP_LOCK_DLL
INTERSP_LOCK_STATUS     
        interSpLockGetLock(     INTERSP_LOCK                    *lock,
                                INTERSP_LOCK_MODE               lockMode,
                                INTERSP_LOCK_ACQUISITION_MODE   acquisitionMode);

CSX_EXTERN_C INTERSP_LOCK_DLL
INTERSP_LOCK_STATUS     
        interSpLockReleaseLock( INTERSP_LOCK    *lock);


/*
 * Macros.
 */

#endif // INTERSP_LOCK_IF_H
