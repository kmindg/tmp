#ifndef _EMCPAL_H_
#define _EMCPAL_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2008-2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcPAL.h
 *
 * @brief
 *      The main header file for the platform abstraction layer, included by clients that link against EmcPAL.
 *
 *      The PAL was created to improve portability by introducing
 *      a layer of abstraction between the CLARiiON code and the 
 *      native windows code.
 *
 * Interface Functions:
 *      EmcpalClientRegister()
 *      EmcpalClientUnregister()
 *      EmcpalClientGetRegistryPath()
 *      EmcpalClientSetRegistryPath()
 *      EmcpalClientGetName()
 *      EmcpalClientGetCsxModuleContext()
 *      
 *      EmcpalMutexCreate()
 *      EmcpalMutexDestroy()
 *      EmcpalMutexLock()
 *      EmcpalMutexTrylock()
 *      EmcpalMutexUnlock()    
 *
 *      EmcpalRecMutexCreate()
 *      EmcpalRecMutexDestroy()
 *      EmcpalRecMutexLock()
 *      EmcpalRecMutexTrylock()
 *      EmcpalRecMutexUnlock()
 *
 *      EmcpalSemaphoreCreate()
 *      EmcpalSemaphoreDestroy()
 *      EmcpalSemaphoreWait() 
 *      EmcpalSemaphoreTryWait() 
 *      EmcpalSemaphoreSignal()
 *
 *      EmcpalSyncEventCreate()
 *      EmcpalSyncEventDestroy()
 *      EmcpalSyncEventSet()
 *      EmcpalSyncEventClear()
 *      EmcpalSyncEventWait()
 *      EmcpalSyncEventWaitUserSpecial()
 *
 *      EmcpalRendezvousEventCreate()
 *      EmcpalRendezvousEventDestroy()
 *      EmcpalRendezvousEventSet()
 *      EmcpalRendezvousEventClear()
 *      EmcpalRendezvousEventWait ()
 *      EmcpalRendezvousEventWaitUserSpecial ()
 *
 *      EmcpalMonostableTimerCreate ()
 *      EmcpalMonostableTimerDestroy ()
 *      EmcpalMonostableTimerStart ()
 *      EmcpalMonostableTimerCancel ()
 *
 *      EmcpalAstableTimerCreate ()
 *      EmcpalAstableTimerDestroy ()
 *      EmcpalAstableTimerStart ()
 *      EmcpalAstableTimerCancel ()
 *
 *      EmcpalTimeReadMilliseconds ()
 *      EmcpalTimeReadMicroseconds ()
 *
 *      EmcpalThreadCreate ()
 *      EmcpalThreadDestroy ()
 *      EmcpalThreadRunning ()
 *      EmcpalThreadExit ()
 *      EmcpalThreadPrioritySet ()
 *      EmcpalThreadPrioritySetExternal ()
 *      EmcpalThreadPriorityGet ()
 *      EmcpalThreadPriorityGetExternal ()
 *      EmcpalThreadAffine ()
 *      EmcpalThreadSleep ()
 *      EmcpalThreadSleepOptimistic ()
 *      EmcpalThreadWait ()
 *
 *      EmcpalSpinlockCreate ()
 *      EmcpalSpinlockDestroy ()
 *
 *      EmcpalGetRegistryValueAsString()
 *      EmcpalGetRegistryValueAsMultiStr()
 *      EmcpalGetRegistryValueAsUint32()
 *      EmcpalGetRegistryValueAsUint64()
 *      EmcpalGetRegistryValueAsBinary()
 *
 *      EmcpalGetRegistryValueAsStringDefault()
 *      EmcpalGetRegistryValueAsMultiStrDefault()
 *      EmcpalGetRegistryValueAsUint32Default()
 *      EmcpalGetRegistryValueAsUint64Default()
 *      EmcpalGetRegistryValueAsBinaryDefault()
 *
 *      EmcpalSetRegistryValueAsString()
 *      EmcpalSetRegistryValueAsMultiStr()
 *      EmcpalSetRegistryValueAsUint32()
 *      EmcpalSetRegistryValueAsUint64()
 *      EmcpalSetRegistryValueAsBinary()
 *
 *      EmcpalCheckRegistryKey()
 *      EmcpalCreateRegistryKey()
 *      EmcpalFreeRegistryMem()
 *
 *      EmcpalDeleteRegistryValue()
 *      EmcpalRegistryOpenKey()
 *      EmcpalRegistryCloseKey()
 *      EmcpalRegistryQueryKeyInfo()
 *      EmcpalRegistryQueryValueInfo()
 *      EmcpalRegistryEnumerateKey()
 *      EmcpalRegistryEnumerateValueKey()
 *
 * Types:
 *      EMCPAL_CLIENT
 *      EMCPAL_MUTEX
 *      EMCPAL_RMUTEX
 *      EMCPAL_SEMAPHORE
 *      EMCPAL_SYNC_EVENT
 *      EMCPAL_RENDEZVOUS_EVENT
 *      EMCPAL_DPC_CALLBACK
 *      EMCPAL_DPC_CONTEXT
 *      EMCPAL_TIMER_CALLBACK
 *      EMCPAL_TIMER_CONTEXT
 *      EMCPAL_TIMEOUT_MSECS
 *      EMCPAL_TIME_MSECS
 *
 * Macros:    
 *      EMCPAL_CONVERT_USECS_TO_100NSECS
 *      EMCPAL_CONVERT_MSECS_TO_100NSECS
 *      EMCPAL_CONVERT_SECS_TO_100NSECS
 *      EMCPAL_CONVERT_100NSECS_TO_MSECS
 *      EMCPAL_CONVERT_TO_WDM_RELATIVE_TIME
 *
 * Constants:
 *      EMCPAL_MINIMUM_TIMEOUT_MSECS
 *      EMCPAL_TIMEOUT_INFINITE_WAIT
 *      EMCPAL_WIN_POOL_TAG
 *		EMCPAL_CSX_POOL_TAG
 *      EMCPAL_EVENT_TYPE_SYNC
 *      EMCPAL_EVENT_TYPE_RENDEZVOUS
 *
 * Function/variable decorations:
 *      EMCPAL_INLINE
 *      EMCPAL_EXPORT
 *      EMCPAL_IMPORT
 *      EMCPAL_GLOBAL
 *      EMCPAL_API
 *      EMCPAL_LOCAL
 *
 * Revision History:
 *     12-08 Chris Gould - Created.
 *
 */

//++
// Include files
//--

#include "csx_ext.h"
#include "csx_rt_time_if.h"
#include "EmcPAL_Configuration.h"
#include "EmcPAL_Status.h"
#include "EmcPAL_Platform_Includes.h"
#include "EmcPAL_Misc.h"
#include "EmcPAL_Retired.h"
#include "EmcUTIL.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * End Includes
 */

/*
 * Defines
 */

/*!
 * @addtogroup emcpal_core_structures
 * @{
 * Major system object types
 *   @typedef   PEMCPAL_DEVICE_OBJECT 
 *		@brief represents created 'device'
 *   @typedef   PEMCPAL_FILE_OBJECT 
 *		@brief represents open of a 'device'
 *   @typedef   PEMCPAL_NATIVE_DRIVER_OBJECT 
 *		@brief represents system 'driver' that devices are associated with
 *   @typedef   EMCPAL_IRP
 *		@brief represents an abstracted NT IRP
 *   @typedef   PEMCPAL_IRP 
 *		@brief represents pointer to an abstracted NT IRP
 *   @typedef   EMCPAL_IO_STACK_LOCATION
 *		@brief represents an abstracted NT IRP stack location
 *   @typedef	PEMCPAL_IO_STACK_LOCATION 
 *		@brief represents pointer to an abstracted NT IRP stack location
 *
 */
#ifdef EMCPAL_USE_CSX_DCALL
typedef csx_p_device_handle_t    PEMCPAL_DEVICE_OBJECT;
typedef csx_p_device_reference_t PEMCPAL_FILE_OBJECT;
typedef csx_p_driver_handle_t    PEMCPAL_NATIVE_DRIVER_OBJECT;
typedef csx_p_dcall_body_t       EMCPAL_IRP, *PEMCPAL_IRP;
typedef csx_p_dcall_level_t      EMCPAL_IO_STACK_LOCATION, *PEMCPAL_IO_STACK_LOCATION;
typedef csx_p_dcall_buffer_t     EMCPAL_MDL, *PEMCPAL_MDL;
typedef csx_p_dcall_status_t     EMCPAL_IRP_STATUS_BLOCK, *PEMCPAL_IRP_STATUS_BLOCK;
#define PAL_FULL_IRP_AVAIL
#else // EMCPAL_USE_CSX_DCALL
struct _DEVICE_OBJECT;
struct _FILE_OBJECT;
struct _DRIVER_OBJECT;
typedef struct _DEVICE_OBJECT *PEMCPAL_DEVICE_OBJECT;
typedef struct _FILE_OBJECT   *PEMCPAL_FILE_OBJECT;
typedef struct _DRIVER_OBJECT *PEMCPAL_NATIVE_DRIVER_OBJECT;
#ifdef EMCPAL_CASE_MAINLINE
#ifndef EMCPAL_BUILDING_USER_MODE_CODE_IN_KERNEL_BUILD
#include "ntddk.h"
#endif
#endif
#if defined(_NTDDK_) || !defined(ALAMOSA_WINDOWS_ENV)
typedef IRP EMCPAL_IRP, *PEMCPAL_IRP;
typedef IO_STACK_LOCATION EMCPAL_IO_STACK_LOCATION, *PEMCPAL_IO_STACK_LOCATION;
typedef MDL EMCPAL_MDL, *PEMCPAL_MDL;
typedef IO_STATUS_BLOCK EMCPAL_IRP_STATUS_BLOCK, *PEMCPAL_IRP_STATUS_BLOCK;
#define PAL_FULL_IRP_AVAIL
#else
struct _IRP;
struct _IO_STACK_LOCATION;
struct _MDL;
struct _IO_STATUS_BLOCK;
typedef struct _IRP *PEMCPAL_IRP;
typedef struct _IO_STACK_LOCATION EMCPAL_IO_STACK_LOCATION, *PEMCPAL_IO_STACK_LOCATION;
typedef struct _MDL *PEMCPAL_MDL;								/*!< EmcPAL MDL */
typedef struct _IO_STATUS_BLOCK *PEMCPAL_IRP_STATUS_BLOCK;		/*!< EmcPAL IRP status block */
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - hassle around use of DDK types in non-kernel build */
#endif // EMCPAL_USE_CSX_DCALL
/* @} END OF emcpal_configuration */

#ifdef PAL_FULL_IRP_AVAIL
typedef const EMCPAL_IRP_STATUS_BLOCK *PCEMCPAL_IRP_STATUS_BLOCK;
#endif

#ifndef EMCPAL_CASE_WK
typedef csx_nuint_t EMCPAL_ACCESS_MASK;
typedef csx_p_execution_state_e EMCPAL_KIRQL, *EMCPAL_PKIRQL;
typedef csx_lis64_t EMCPAL_LARGE_INTEGER, *EMCPAL_PLARGE_INTEGER;
#else
typedef ACCESS_MASK EMCPAL_ACCESS_MASK;
#ifdef EMCPAL_CASE_NTDDK_EXPOSE
typedef unsigned int EMCPAL_KIRQL, *EMCPAL_PKIRQL;
#else
typedef KIRQL EMCPAL_KIRQL, *EMCPAL_PKIRQL;
#endif
typedef LARGE_INTEGER EMCPAL_LARGE_INTEGER, *EMCPAL_PLARGE_INTEGER;
#endif

#include "EmcPAL_Ioctl.h"

/*!
 * @addtogroup emcpal_time
 * @{
 */
/*!
 * @brief
 *      Constant should be passed to PAL APIs to indicate that the
 *      caller does not want to wait with a timeout
 */
#define EMCPAL_TIMEOUT_INFINITE_WAIT (csx_u64_t)(-1)

/*!
 * @brief
 * Convert from microseconds to NT time units which are expressed in
 * hundreds of nanoseconds
 */
#define EMCPAL_CONVERT_USECS_TO_100NSECS(_us) (_us * CSX_XSEC_PER_USEC)
/*!
 * @brief
 * Convert from milliseconds to NT time units which are expressed in
 * hundreds of nanoseconds
 */
#define EMCPAL_CONVERT_MSECS_TO_100NSECS(_ms) (_ms * CSX_XSEC_PER_MSEC)
/*!
 * @brief
 * Convert from seconds to NT time units which are expressed in
 * hundreds of nanoseconds
 */
#define EMCPAL_CONVERT_SECS_TO_100NSECS(_s)   (_s  * CSX_XSEC_PER_SEC)

/*!
 * @brief
 *       This macro converts the time values in units of 100nSec that are
 *       returned by Windows to the standard Emcpal units of milliseconds.
 */
#define EMCPAL_CONVERT_100NSECS_TO_MSECS(_ns) (_ns / CSX_XSEC_PER_MSEC)

/*!
 * @brief
 *       This macro converts the time values in units of 100nSec that are
 *       returned by Windows to the standard Emcpal units of microseconds.
 */
#define EMCPAL_CONVERT_100NSECS_TO_USEC(_ns) (_ns / CSX_XSEC_PER_USEC)

/*!
 * @brief
 *       Express an NT time value as a relative timeout
 *       by making sure it is negative
 */
#define EMCPAL_CONVERT_TO_WDM_RELATIVE_TIME(_t) (((_t) > 0) ? -(_t) : (_t));

/*!
 * @brief
 *      Convert Milliseconds to Seconds
 */
#define EMCPAL_CONVERT_MSECS_TO_SECS(_ms) ((_ms) / CSX_MSEC_PER_SEC)
/*!
 * @brief
 *      Convert Seconds to Milliseconds
 */
#define EMCPAL_CONVERT_SECS_TO_MSECS(_s)  ((_s) * CSX_MSEC_PER_SEC)

CSX_STATIC_INLINE
UINT_64
EMCPAL_NT_RELATIVE_TIME_TO_MSECS(EMCPAL_PLARGE_INTEGER tzNeg100Ns)
{
    if(NULL == tzNeg100Ns)
    {
        return EMCPAL_TIMEOUT_INFINITE_WAIT;
    }
    else
    {
        LONGLONG qp = tzNeg100Ns->QuadPart;
        qp *= -1LL;
        return (ULONG) EMCPAL_CONVERT_100NSECS_TO_MSECS(qp);
    }
}


/* @} End emcpal_time group */

/*!
 * @brief
 * system panic codes
 */
#define EMCPAL_SYSTEM_ERROR_BASE    0x79510000				/*!< EmcPAL system error base */

/*! @brief EmcPAL system error */
enum EMCPAL_SYSTEM_ERROR {
        EMCPAL_INVALID_IRQL_NEQL    = EMCPAL_SYSTEM_ERROR_BASE  // Internal Only: IRQ level not equal to %d
                                                            // Recovery: Internal information only
                                                            // Symptoms: Internal debug aid for catching calls
                                                            //           to EmcPAL code from an invalid IRQL

};

#define CSX_SYSTEM_ERROR_BASE       0x79520000				/*!< CSX system error base */

/*! @brief CSX system error */
enum CSX_SYSTEM_ERROR {
        CSX_ERROR_INTERNAL_PANIC    = CSX_SYSTEM_ERROR_BASE     // Internal Only: An internal error has occurred.
                                                            // Recovery: Internal information only
                                                            // Symptoms: This is a debug aid to capture fatal internal errors

};

/*!
 * @addtogroup emcpal_configuration
 * @{
 *
 * @brief Function/variable decorations
 *
 *      These decorations are used to control the scope that various PAL APIs
 *      and variables are accessible from.
 *
 *      @li EMCPAL_GLOBAL       - global within current driver
 *      @li EMCPAL_LOCAL        - local
 *      @li EMCPAL_EXPORT       - export to other drivers
 *      @li EMCPAL_IMPORT       - import from other drivers
 *      @li EMCPAL_API          - used to decorate PAL APIs so they are
 *                            exported by the PAL and imported by PAL users.
 *                            We do this by defining EMCPAL_PAL_BUILD in the PAL
 *                            SOURCES.mak file. 
 *      @li EMCPAL_INLINE       - request compiler to inline function
 *
 * @version
 *      12/08   CGould - Created.
 *
 */
#define EMCPAL_GLOBAL					/*!< EmcPAL global */
#define EMCPAL_LOCAL CSX_LOCAL			/*!< EmcPAL local */

#define EMCPAL_EXPORT CSX_MOD_EXPORT	/*!< EmcPAL export */
#define EMCPAL_IMPORT CSX_MOD_IMPORT	/*!< EmcPAL import */

/*! @brief EmcPAL API */
#ifdef EMCPAL_PAL_BUILD
#define EMCPAL_API EMCPAL_EXPORT
#else
#define EMCPAL_API EMCPAL_IMPORT
#endif

#define EMCPAL_INLINE CSX_INLINE		/*!< EmcPAL inline */
/* End Function/variable decorations */

/*!
 * @brief
 *      The object IDs used to tag data structures as a certain type.
 */
typedef enum _EMCPAL_OID_T
{
        EMCPAL_OID_THREAD  = 0x54687264,                // ascii 'Thrd'
        EMCPAL_OID_INVALID = 0

} EMCPAL_OID_T;

/*!
 * @brief
 *      Contains state information about a PAL client.
 *
 *      Each driver that wants to communicate with the PAL must register
 *      as a client so the PAL can track resources allocated on the client's
 *      behalf.  If a driver uses the PAL driver shell they will be automatically
 *      registered as a PAL client when the driver loads and unregistered when
 *      the driver unloads.  This structure should be considered opaque to PAL
 *      users.  The contents of this structure should only be accessed using the
 *      exported PAL client APIs declared below and defined in EmcPAL_Client.c
 *
 */
typedef struct _EMCPAL_CLIENT
{
    ULONG                 magic;			/*!< Magic number ued to detect valid client object */
    TEXT                 *clientName;		/*!< Name of the client */
    csx_dlist_entry_t     clientListEntry;	/*!< Entry on the list of PAL clients */
    TEXT                 *registryPath;		/*!< Path to the client registry parameters */
    csx_module_context_t  csxModuleContext;	/*!< CSX module context we must pass to any APIs that create CSX resources */
    csx_nsint_t           moduleArgc;           /*!< Number of arguments passed to CSX driver from Linux shell */
    csx_string_t         *moduleArgv;           /*!< Pointer to list of arguments passed to CSX driver from Linux shell */
} EMCPAL_CLIENT, *PEMCPAL_CLIENT;			/*!< PEMCPAL_CLIENT - Pointer to EMCPAL_CLIENT */
/* @}  End emcpal_configuration */

/*! 
 * @{
 * @ingroup emcpal_time
 *		Used to pass time values to the various PAL interfaces that use them.  
 *		All PAL times are in units of millisecond and
 *      are relative to the current time.  The PAL does not use absolute
 *      time values.
 */
typedef UINT_64E EMCPAL_TIMEOUT_MSECS;
typedef UINT_64E EMCPAL_TIME_MSECS;
typedef UINT_64E EMCPAL_TIME_USECS;
/* @} */

/*! @addtogroup emcpal_time 
 *  @{
 */
/*!
 * @brief
 *      The minimum time interval that can be used
 *      for PAL API timeouts.
 */
#define EMCPAL_MINIMUM_TIMEOUT_MSECS 15

/* @} END OF emcpal_time */

/*! @{
 *  @ingroup emcpal_memory
 * 
 * Constant:
 *      EMCPAL_WIN_POOLTAG
 *      EMCPAL_CSX_POOL_TAG
 *
 * @brief
 *      This constant is used to tag the memory allocated
 *		by EmcPAL
 */
#define EMCPAL_WIN_POOL_TAG     'laPE'				/*!< EmcPAL Win pool tag */
#define EMCPAL_CSX_POOL_TAG		CSX_TAG("laPE")		/*!< EmcPAL CSX pool tag */
/* @} END OF emcpal_memory */

/*!
 *  @{
 *  @ingroup emcpal_thread
 *
 *  @brief
 *      The relative priority of a given system thread.
 */
#if !defined (EMCPAL_USE_CSX_THR) || defined (EMCPAL_DEBUGEXT)

// We use numeric values for Windows kernel to map thread priorities.
// 0 (LOW_PRIORITY) is illegal to use on Windows kernel.
#define EMCPAL_THREAD_PRIORITY_VERY_LOW                 (LOW_PRIORITY+1)
#define EMCPAL_THREAD_PRIORITY_LOW                      (LOW_PRIORITY+3)
#define EMCPAL_THREAD_PRIORITY_NORMAL                   (LOW_PRIORITY+8)
#define EMCPAL_THREAD_PRIORITY_HIGH                     (LOW_PRIORITY+13)
#define EMCPAL_THREAD_PRIORITY_VERY_HIGH                (LOW_PRIORITY+15)
#define EMCPAL_THREAD_PRIORITY_REALTIME_VERY_LOW        (LOW_REALTIME_PRIORITY-1)
#define EMCPAL_THREAD_PRIORITY_REALTIME_LOW             LOW_REALTIME_PRIORITY
#define EMCPAL_THREAD_PRIORITY_REALTIME_NORMAL          LOW_REALTIME_PRIORITY
#define EMCPAL_THREAD_PRIORITY_REALTIME_HIGH            (LOW_REALTIME_PRIORITY+1)
#define EMCPAL_THREAD_PRIORITY_REALTIME_VERY_HIGH       HIGH_PRIORITY
#define EMCPAL_THREAD_PRIORITY_INVALID                  (MAXIMUM_PRIORITY + 1)

#endif
/* @} emcpal_thread */

/*!
 * @{
 * @ingroup emcpal_synch_coord
 * @brief
 * 
 * If we're using CSX then we keep track of the event type.
 * This is currently only necessary so the usersim DDK
 * emulation can tell the event type that was passed in to
 * APIs like IoBuildDeviceIoControlRequest.  Once the IRP abstractions
 * are in place this will no longer be necessary.
 */
#define EMCPAL_EVENT_TYPE_SYNC          0xFF00AA00
#define EMCPAL_EVENT_TYPE_RENDEZVOUS    0xFF00AA01
/* @} emcpal_synch_coord */

/*!
 * @addtogroup emcpal_synch_coord
 * @{
 */
/*! @brief Synchronous event */
#if defined (EMCPAL_USE_CSX_ARE) && !defined (EMCPAL_DEBUGEXT)
typedef struct CSX_ALIGN_NATIVE {
    UINT_32E event_type;
    csx_p_are_t event;
} EMCPAL_SYNC_EVENT, *PEMCPAL_SYNC_EVENT;
#else                                   /* EMCPAL_USE_CSX_ARE */
typedef KEVENT                  EMCPAL_SYNC_EVENT        , *PEMCPAL_SYNC_EVENT;
#endif                                  /* EMCPAL_USE CSX_ARE */

/*! @brief Rendezvous event */
#if defined (EMCPAL_USE_CSX_MRE) && !defined (EMCPAL_DEBUGEXT)
typedef struct CSX_ALIGN_NATIVE {
    UINT_32E event_type;
    csx_p_mre_t event;
} EMCPAL_RENDEZVOUS_EVENT, *PEMCPAL_RENDEZVOUS_EVENT;
#else                                   /* EMCPAL_USE_CSX_MRE */
typedef KEVENT                  EMCPAL_RENDEZVOUS_EVENT  , *PEMCPAL_RENDEZVOUS_EVENT;
#endif                                  /* EMCPAL_USE_CSX_MRE */
/* @} emcpal_synch_coord */

/*!
 * @addtogroup emcpal_synch_coord
 * @{
 */
#if defined (EMCPAL_USE_CSX_SEM) && !defined (EMCPAL_DEBUGEXT)
/*! @brief EmcPAL semaphore */
typedef csx_p_sem_t             EMCPAL_SEMAPHORE         , *PEMCPAL_SEMAPHORE;	/*!<  Pointer to EmcPAL semaphore */
#else                                   /* EMCPAL_USE_CSX_SEM */
/*! @brief EmcPAL semaphore */
typedef KSEMAPHORE              EMCPAL_SEMAPHORE         , *PEMCPAL_SEMAPHORE;	/*!<  Pointer to EmcPAL semaphore */
#endif                                  /* EMCPAL_USE_CSX_SEM */


#if defined (EMCPAL_USE_CSX_SPL) && !defined (EMCPAL_DEBUGEXT)
typedef csx_p_spl_t             EMCPAL_SPINLOCK          , *PEMCPAL_SPINLOCK;
typedef csx_p_spl_t             EMCPAL_SPINLOCK_QUEUED,         *PEMCPAL_SPINLOCK_QUEUED;
typedef csx_nuint_t             EMCPAL_SPINLOCK_QUEUE_NODE, *PEMCPAL_SPINLOCK_QUEUE_NODE;
#else                                   /* EMCPAL_USE_CSX_SPL */
/*! @brief EmcPAL spinlock */
typedef struct CSX_ALIGN_NATIVE _EMCPAL_SPINLOCK
{
	KSPIN_LOCK		lockObject; /*!< Lock object */
	KIRQL				irql; /*!< IRQL for lock */	 
} EMCPAL_SPINLOCK, *PEMCPAL_SPINLOCK;		/*!< Pointer to EmcPal spinlock*/

/*! @brief EmcPAL queued spinlock */
typedef KSPIN_LOCK			EMCPAL_SPINLOCK_QUEUED, *PEMCPAL_SPINLOCK_QUEUED;	/*!< Pointer to EmcPal queued spinlock */
/*! @brief EmcPAL spinlock queue node */
typedef KLOCK_QUEUE_HANDLE	EMCPAL_SPINLOCK_QUEUE_NODE, *PEMCPAL_SPINLOCK_QUEUE_NODE; /*!< Pointer to EmcPAL spinlock queue node */
#endif                                  /* EMCPAL_USE_CSX_SPL */
/* @} emcpal_synch_coord */

/*!
 * @addtogroup emcpal_thread
 * @{
 */
#define EMCPAL_THREAD_NAME_MAX_LENGTH 32

#if defined (EMCPAL_USE_CSX_THR) && !defined (EMCPAL_DEBUGEXT)
typedef csx_thread_id_t  EMCPAL_THREAD_TID;
typedef csx_p_thr_handle_t     EMCPAL_THREAD_ID;
typedef csx_p_thr_body_t                EMCPAL_THREAD_START_FUNCTION;
typedef csx_p_thr_context_t     EMCPAL_THREAD_CONTEXT;

typedef enum {
    EMCPAL_THREAD_PRIORITY_VERY_LOW = 1,
    EMCPAL_THREAD_PRIORITY_LOW,
    EMCPAL_THREAD_PRIORITY_NORMAL,
    EMCPAL_THREAD_PRIORITY_HIGH,
    EMCPAL_THREAD_PRIORITY_VERY_HIGH,
    EMCPAL_THREAD_PRIORITY_REALTIME_VERY_LOW,
    EMCPAL_THREAD_PRIORITY_REALTIME_LOW,
    EMCPAL_THREAD_PRIORITY_REALTIME_NORMAL,
    EMCPAL_THREAD_PRIORITY_REALTIME_HIGH,
    EMCPAL_THREAD_PRIORITY_REALTIME_VERY_HIGH,
    EMCPAL_THREAD_PRIORITY_INVALID
} EMCPAL_THREAD_PRIORITY;

typedef csx_processor_mask_t    EMCPAL_PROCESSOR_MASK    , *PEMCPAL_PROCESSOR_MASK;

typedef struct _EMCPAL_THREAD
{
        EMCPAL_OID_T                                    oid;
        csx_p_thr_handle_t                              threadObject;
        EMCPAL_THREAD_START_FUNCTION    startFunction;
        EMCPAL_THREAD_CONTEXT                   threadContext;
    EMCPAL_PROCESSOR_MASK           requestedAffinityMask;
    csx_char_t                      name[EMCPAL_THREAD_NAME_MAX_LENGTH];
    csx_bool_t                      isDetached;

} EMCPAL_THREAD, *PEMCPAL_THREAD;
#else                                   /* EMCPAL_USE_CSX_THR */
/*! @brief EmcPAL thread ID */
typedef PRKTHREAD                   EMCPAL_THREAD_ID;
/*! @brief EmcPAL thread start routine */
typedef PKSTART_ROUTINE         EMCPAL_THREAD_START_FUNCTION;
/*! @brief EmcPAL thread context */
typedef PVOID                           EMCPAL_THREAD_CONTEXT;
/*! @brief EmcPAL thread priority */
typedef KPRIORITY                       EMCPAL_THREAD_PRIORITY;

/*! @brief EmcPAL processor affinity mask */
typedef KAFFINITY			EMCPAL_PROCESSOR_MASK        , *PEMCPAL_PROCESSOR_MASK; /*!< Pointer to EmcPAL processor affinity mask */

/*! @brief EmcPAL thread descriptor */
typedef struct _EMCPAL_THREAD
{
	EMCPAL_OID_T					oid;				/*!< OID */
    PKTHREAD                        threadObject;		/*!< Thread Object */
	EMCPAL_THREAD_START_FUNCTION	startFunction;		/*!< Start function */
	EMCPAL_THREAD_CONTEXT			threadContext;		/*!< Thread context */
    EMCPAL_PROCESSOR_MASK           requestedAffinityMask; /*!< Affinity mask */
    EMCPAL_SYNC_EVENT               threadObjectWait;
    csx_char_t                      name[EMCPAL_THREAD_NAME_MAX_LENGTH]; /*!< Name */
    csx_bool_t                      isDetached; /*!< TRUE if Detached */
} EMCPAL_THREAD, *PEMCPAL_THREAD;						/*!< Pointer to EmcPAL thread */
/* @} emcpal_thread */
#endif                                  /* EMCPAL_USE_CSX_THR */

/*!
 * @addtogroup emcpal_timer
 * @{
 */
#if defined (EMCPAL_USE_CSX_TIM_DPC)
/*! @brief EmcPAL DPC callback */
typedef csx_p_dpc_callback_t    EMCPAL_DPC_CALLBACK;
/*! @brief EmcPAL DPC context */
typedef csx_p_dpc_context_t     EMCPAL_DPC_CONTEXT;
#else                                   /* EMCPAL_USE_CSX_TIM_DPC */
/*! @brief EmcPAL DPC callback */
typedef PKDEFERRED_ROUTINE      EMCPAL_DPC_CALLBACK;
/*! @brief EmcPAL DPC context */
typedef PVOID                   EMCPAL_DPC_CONTEXT;
#endif                                  /* EMCPAL_USE_CSX_TIM_DPC */


#if defined (EMCPAL_USE_CSX_TIM_DPC)
/*! @brief EmcPAL DPC callback */
typedef csx_p_tim_callback_t   EMCPAL_TIMER_CALLBACK;
/*! @brief EmcPAL DPC context */
typedef csx_p_tim_context_t    EMCPAL_TIMER_CONTEXT;

typedef struct CSX_ALIGN_NATIVE _EMCPAL_MONOSTABLE_TIMER	/*!< EmcPAL MONOTABLE timer */
{
    csx_p_tim_t				timerObject; /*!< Timer object */

	EMCPAL_TIMER_CALLBACK	callback; /*!< Callback function that is called when the timer expires */
	EMCPAL_TIMER_CONTEXT	enduserContext; /*!< Context structure passed to callback function */

} EMCPAL_MONOSTABLE_TIMER, *PEMCPAL_MONOSTABLE_TIMER;		/*!< ptr to EmcPAL MONOTABLE timer */

typedef struct CSX_ALIGN_NATIVE _EMCPAL_ASTABLE_TIMER		/*!< EmcPAL ASTABLE timer */
{
    csx_p_tim_t				timerObject; /*!< Timer object */

	EMCPAL_TIMER_CALLBACK	callback; /*!< Callback function that is called when the timer expires */
	EMCPAL_TIMER_CONTEXT	enduserContext; /*!< Context structure passed to callback function */
	EMCPAL_TIMEOUT_MSECS	timeout; /*!< Timeout period */

} EMCPAL_ASTABLE_TIMER, *PEMCPAL_ASTABLE_TIMER;				/*!< ptr to EmcPAL ASTABLE timer */
#else                                   /* EMCPAL_USE_CSX_TIM_DPC */
typedef PVOID              EMCPAL_TIMER_CONTEXT;
typedef VOID (*EMCPAL_TIMER_CALLBACK)(EMCPAL_TIMER_CONTEXT);

typedef struct CSX_ALIGN_NATIVE _EMCPAL_MONOSTABLE_TIMER	/*!< EmcPAL MONOTABLE timer */
{
        KTIMER                                  timerObject;
        KDPC                                    dpcObject;

        EMCPAL_TIMER_CALLBACK   callback;
        EMCPAL_TIMER_CONTEXT    enduserContext;

} EMCPAL_MONOSTABLE_TIMER, *PEMCPAL_MONOSTABLE_TIMER;		/*!< ptr to EmcPAL MONOTABLE timer */

typedef struct CSX_ALIGN_NATIVE _EMCPAL_ASTABLE_TIMER		/*!< EmcPAL ASTABLE timer */
{
        KTIMER                                  timerObject;
        KDPC                                    dpcObject;

        EMCPAL_TIMER_CALLBACK   callback;
        EMCPAL_TIMER_CONTEXT    enduserContext;

} EMCPAL_ASTABLE_TIMER, *PEMCPAL_ASTABLE_TIMER;				/*!< ptr to EmcPAL ASTABLE timer */
#endif                                  /* EMCPAL_USE_CSX_TIM_DPC */
/* @} END emcpal_timer */

/*!
 * @addtogroup emcpal_mutex
 * @{
 */

/*!
 * @brief
 *      A PAL mutex.  
 *
 *      Unlike NT mutexes, PAL mutexes
 *      are not recursive.  If the owner of a mutex attempts to reacquire it
 *      the PAL will PANIC.  This structure should be considered opaque by
 *      PAL users and its contents should only be accessed via the PAL
 *      mutex APIs
 *
 * Members:
 *  ownerThread         :   On checked/debug builds we use this field to track
 *                          the current owner of the mutex.  This is used as a debug
 *                          aid to track attempts to acquire the mutex recursively or
 *                          attempts to release the mutex by a thread other than the
 *                          owner.
 *  osMutex             :   This is the native OS mutex object.  In the SSPG case a CSX
 *                          mutex is used.  In the standard CLARiiON environment we use
 *                          a semaphore to implement the mutex so it cannot be recursively
 *                          acquired.
 */
typedef struct CSX_ALIGN_NATIVE
{
#ifndef CSX_BV_FLAVOR_RETAIL
#define EMCPAL_DEBUG_MUTEX_OWNER		/*!< EmcPAL debug mutex owner */
#endif

#ifdef EMCPAL_DEBUG_MUTEX_OWNER
    csx_thread_id_t ownerThread; /*!< Thread */
#endif

#if defined (EMCPAL_USE_CSX_MUT) && !defined (EMCPAL_DEBUGEXT)
    csx_p_mut_t     osMutex; /*!< Mutex */
#else
    KSEMAPHORE      osMutex; /*!< Mutex */
#endif

} EMCPAL_MUTEX, *PEMCPAL_MUTEX;		/*!< Pointer to EmcPAL mutex */
//.End


/*!
 * @brief
 *      This structure represents a recursive PAL mutex.  This structure should be
 *      considered opaque by PAL users and its contents should only be accessed
 *      via the PAL recursive mutex APIs
 *
 * Members:
 *  osMutex             :   This is the native OS recursive mutex object.  In the SSPG case a CSX
 *                          rmutex is used.  In the standard CLARiiON environment we use
 *                          a Windows KMUTEX.
 */
typedef struct CSX_ALIGN_NATIVE
{
#if defined (EMCPAL_USE_CSX_RMUT) && !defined (EMCPAL_DEBUGEXT)
    csx_p_rmut_t    osMutex; /*!< Mutex */
#else
    KMUTEX          osMutex; /*!< Mutex */
#endif

} EMCPAL_RMUTEX, *PEMCPAL_RMUTEX;		/*!< Pointer to EmcPAL recursive mutex */

/* @} emcpal_mutex */
/*
 * Function prototypes
 */

/*!
 * @addtogroup emcpal_client
 * @{
 */
/*!
 * @brief
 *      This function is used to register a new client with the PAL.
 *
 * @return
 *      STATUS_SUCCESS:                 client was registered with the PAL
 *      STATUS_OBJECT_NAME_EXISTS:      a client with the given name is currently registered
 *                                      with the PAL
 *      
 * @version
 *      12/08  CGould Created.
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalClientRegister(
    PEMCPAL_CLIENT          pPalClient,			/*!< pointer to the client to be registered */
    TEXT                    *clientName,		/*!< name of the client to register (must be unique) */
    csx_module_context_t    csxModuleContext,	/*!< CSX module context to associate with the client.  Any CSX resources
                                                allocated by PAL APIs with be associated with this module context. */
    TEXT                    *registryPath,	/*!< default path where configuration data for the client is stored*/
    csx_nsint_t             moduleArgc,         /*!< Number of arguments passed to CSX driver from Linux shell */
    csx_string_t            *moduleArgv         /*!< Pointer to list of arguments passed to CSX driver from Linux shell */
    );

/*!
 * @brief
 *      This function is used to unregister a client with the PAL.
 *
 * @return
 *      STATUS_SUCCESS:                 client was unregistered with the PAL
 *      STATUS_OBJECT_NAME_NOT_FOUND:   the client could not be found
 *      
 * @version
 *      12/08  CGould Created.
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalClientUnregister(
    PEMCPAL_CLIENT pPalClient	/*!< pointer to the client to be unregistered */
    );

/*!
 * @brief
 *      This function is used to retrieve the name of a PAL client
 *
 * @return
 *      pointer to a string containing the PAL client's name
 *      
 * @version
 *      12/08  CGould Created.
 */
EMCPAL_API
TEXT *
EmcpalClientGetName(
    PEMCPAL_CLIENT pPalClient	/*!< pointer to a PAL client */
    );

/*!
 * @brief
 *      This function is used to retrieve the registry path of a PAL client.
 *      This path is the default location where PAL client configuration data
 *      should be stored.
 *
 * @return
 *      pointer to a string containing the PAL client's registry path
 *      
 * @version
 *      12/08  CGould Created.
 */
EMCPAL_API
TEXT *
EmcpalClientGetRegistryPath(
    PEMCPAL_CLIENT pPalClient	/*!< pointer to a PAL client */
    );


/*!
 * @brief
 *      This function is used to set the registry path of a PAL client.
 *      This path is the default location where PAL client configuration data
 *      should be stored.
 *
 * @return
 *      
 * @version
 *      12/11  Manjit Singh Created.
 */
EMCPAL_API
VOID
EmcpalClientSetRegistryPath(
    PEMCPAL_CLIENT pPalClient,	/*!< pointer to a PAL client */
    TEXT* registrypath          /*!< registry path */
    );

/*!
 * @brief
 *      This function is used to retrieve the CSX module context associated with
 *      a PAL client.  
 * 
 *      PAL clients provide a CSX module context that is used
 *      to create any CSX resources that may be allocated on behalf of a client.
 *      Clients can use this API to retrieve their CSX module context if they want
 *      to use CSX directly.
 *
 * @return
 *      pointer to the PAL client's CSX module context
 *      
 * @version
 *      12/08  CGould Created.
 */
CSX_STATIC_FINLINE
csx_module_context_t
EmcpalClientGetCsxModuleContext(
    PEMCPAL_CLIENT  pPalClient)
{
    CSX_ASSERT_H_DC(pPalClient != NULL);
    CSX_ASSERT_H_DC(pPalClient->magic == 0xEEFFAABB);
    return pPalClient->csxModuleContext;
}

/*!
 * @brief
 *      This function is used to retrieve the CSX module parameters.
 *
 *      PAL clients provide a number of parameters passed to the module
 *      Clients can use this API to retrieve argc.
 *
 * @return
 *      argc of passed parameters
 *
 * @version
 *      15/05  Yuri Stotski Created.
 */
CSX_STATIC_FINLINE
csx_nsint_t
EmcpalClientGetCsxModuleArgc(
    PEMCPAL_CLIENT  pPalClient)
{
    CSX_ASSERT_H_DC(pPalClient != NULL);
    CSX_ASSERT_H_DC(pPalClient->magic == 0xEEFFAABB);
    return pPalClient->moduleArgc;
}

/*!
 * @brief
 *      This function is used to retrieve the CSX module parameters.
 *
 *      PAL clients provide a pointer to parameters array
 *      Clients can use this API to retrieve argv.
 *
 * @return
 *      argv pointer to passed parameters array
 *
 * @version
 *      15/05  Yuri Stotski Created.
 */
CSX_STATIC_FINLINE
csx_string_t*
EmcpalClientGetCsxModuleArgv(
    PEMCPAL_CLIENT  pPalClient)
{
    CSX_ASSERT_H_DC(pPalClient != NULL);
    CSX_ASSERT_H_DC(pPalClient->magic == 0xEEFFAABB);
    return pPalClient->moduleArgv;
}

/* @} End Client interfaces */

/*!
 * Mutex interfaces
 * @addtogroup emcpal_mutex
 * @{
 */

/*!
 * @brief
 *      Creates and initializes a standard mutex from the user-supplied
 *      storage. Caller must destroy the mutex before their module can be unloaded.
 *
 *      NOTE:   Unlike Windows mutexes, PAL mutexes are not recursive.
 *
 * @return
 *      STATUS_SUCCESS if mutex was created
 *      otherwise error code bubbled up 
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalMutexCreate (PEMCPAL_CLIENT pPalClient,	/*!< PAL client that owns this mutex */
				   PEMCPAL_MUTEX  pMutex,		/*!< Pointer to the mutex to be created */
				   const TEXT    *pName		/*!< Mutex's name (Ascii string) */
                   );

CSX_STATIC_INLINE
VOID
EmcpalMutexMustDestroy(PEMCPAL_MUTEX pMutex)
{
#if 0
     if (csx_p_always_true()) {
         CSX_PANIC();
     }
#endif
}

/*!
 * @brief
 *      Destroys a mutex.
 *
 * @return
 *      EMCPAL_SUCCESS
 */
EMCPAL_API
VOID
EmcpalMutexDestroy(
    PEMCPAL_MUTEX   pMutex	/*!< Ptr to the mutex to be destroyed */
    );

/*!
 * @brief
 *      Atomically waits on, and locks a mutex.
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalMutexLock(
    PEMCPAL_MUTEX   pMutex	/*!< Ptr to the mutex to lock */
    );

/*!
 * @brief
 *      Attempts to lock a mutex.  
 *
 *		If the mutex is available the mutex will
 *      be taken and STATUS_SUCCESS will be returned to the caller.  If the
 *      mutex is not available then the function will return STATUS_ACCESS_DENIED.
 *
 * @return
 *      STATUS_SUCCESS if the mutex was successfully locked
 *      STATUS_ACCESS_DENIED if the mutex wasn't available 
 *
 * @version
 *      1/13/09 cgould - created
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalMutexTrylock(
    PEMCPAL_MUTEX   pMutex	/*!< Ptr to the mutex to unlock */
    );

/*!
 * @brief
 *      Releases (unlocks) a mutex.
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalMutexUnlock(
    PEMCPAL_MUTEX   pMutex	/*!< Ptr to the mutex to unlock */
    );

/* @} emcpal_mutex */

/*!
 * Recursive Mutex interfaces
 * @addtogroup emcpal_recmutex
 * @{
 */

/*!
 * @brief
 *      Creates and initializes a recursive mutex from the user-supplied
 *      storage. Caller must destroy the mutex before their module can be unloaded.
 *
 * @return
 *      STATUS_SUCCESS if mutex was created
 *      otherwise error code bubbled up 
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalRecMutexCreate (PEMCPAL_CLIENT  pPalClient,	/*!< PAL client that owns this mutex */
				      PEMCPAL_RMUTEX  pMutex,		/*!< Ptr to the recursive mutex to be created */
				      const TEXT     *pName		/*!< Mutex's name (Ascii string) */
                      );

/*!
 * @brief
 *      Destroys a recursive mutex.
 *
 * @return
 *      EMCPAL_SUCCESS
 */
EMCPAL_API
VOID
EmcpalRecMutexDestroy(
    PEMCPAL_RMUTEX   pMutex	/*!< Ptr to the mutex to be destroyed */
    );

/*!
 * @brief
 *      Atomically waits on, and locks a recursive mutex.  
 *
 *		If the caller already holds the mutex this call will return
 *      without blocking.
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalRecMutexLock(
    PEMCPAL_RMUTEX   pMutex	/*!< Ptr to the mutex to lock */
    );

/*!
 * @brief
 *      Attempts to lock a recursive mutex.  
 *
 *		If the mutex is available or
 *      the calling thread has previously acquired the mutex, it will
 *      be taken and STATUS_SUCCESS will be returned to the caller.  If the
 *      mutex is not available then the function will return STATUS_ACCESS_DENIED.
 *
 * @return
 *      - STATUS_SUCCESS if the mutex was successfully locked
 *      - STATUS_ACCESS_DENIED if the mutex wasn't available 
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalRecMutexTrylock(
    PEMCPAL_RMUTEX   pMutex	/*!< Ptr to the mutex to unlock */
    );

/*!
 * @brief
 *      Releases (unlocks) a recursive mutex.
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalRecMutexUnlock(
    PEMCPAL_RMUTEX   pMutex	/*!< Ptr to the mutex to unlock */
    );

/* @} End Recursive Mutex interfaces */

/*!
 * Semaphore interfaces
 * @addtogroup emcpal_semaphore
 * @{
 */

/*!
 * @brief
 *      Creates and initializes a semaphore object from the user-supplied 
 *      storage. Semaphore objects must be destroyed before your module can be unloaded.
 *
 * @return
 *      - STATUS_SUCCESS 
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSemaphoreCreate(
    PEMCPAL_CLIENT      pPalClient,	/*!< PAL client that owns this semaphore */
    PEMCPAL_SEMAPHORE   pSema,		/*!< Pointer to the semaphore to be created */
    const TEXT          *pName,		/*!< Semaphore's name (Ascii string) */
    LONG                initCnt	/*!< Initial count assigned to the semaphore.  (Non-zero value
									     implies the semaphore is in the signalled state.) */
    );

CSX_STATIC_INLINE
VOID
EmcpalSemaphoreMustDestroy(PEMCPAL_SEMAPHORE  pSema)
{
#if 0
     if (csx_p_always_true()) {
         CSX_PANIC();
     }
#endif
}

/*!
 * @brief
 *      Destroys a semaphore object
 *
 * @return
 *      Nothing
 */
EMCPAL_API
VOID
EmcpalSemaphoreDestroy(
    PEMCPAL_SEMAPHORE  pSema	/*!< Ptr to the semaphore to be destroyed */
    );

/*!
 * @brief
 *      Waits on a semaphore object
 *
 *      NOTE - The timeout is in milliseconds, and must be non-negative.
 *      if a nonzero timeout value is specified it must be at least 15ms
 *      for portability reasons.
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if a timeout was specified and the wait timed out
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSemaphoreWait(
    PEMCPAL_SEMAPHORE  pSema,		/*!< Ptr to the semaphore to wait on */
    EMCPAL_TIMEOUT_MSECS timeout	/*!< timeout value in milliseconds. (0 = no timeout) */
    );

/*!
 *  @brief
 *       Waits on a semaphore object (can be interrupted by APC, signal, etc)
 *
 *      NOTE - The timeout is in milliseconds, and must be non-negative.
 *      if a nonzero timeout value is specified it must be at least 15ms
 *      for portability reasons.
 *  @pre
 *  Missing detail
 *
 *  @post
 *  Missing detail
 * 
 *  @return
 *      EMCPAL_STATUS_SUCCESS if the wait succeeded <br />
 *      EMCPAL_STATUS_TIMEOUT if a timeout was specified and the wait timed out
 *      EMCPAL_STATUS_ALERTED if wait was aborted due to APC/signal 
 * 
 *  @version
 *       3/11/13   CGould - Created.
 * 
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSemaphoreWaitUserSpecial(
    PEMCPAL_SEMAPHORE       pSema,  /*!< Ptr to the semaphore to wait on */
    EMCPAL_TIMEOUT_MSECS    timeout /*!< timeout value in milliseconds. (0 = no timeout) */
    );

/*!
 * @brief
 *      Tries to lock and decrement a semaphore object, but returns 
 *      immediately whether or not the semaphore was available.
 *
 * @return
 *      - STATUS_SUCCESS if the lock attempt succeeded
 *      - STATUS_TIMEOUT if the lock attempt failed
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSemaphoreTryWait(
    PEMCPAL_SEMAPHORE  pSema	/*!< Ptr to the semaphore to try to lock */
    );

/*!
 * @brief
 *      Signals (increments) the count of a semaphore object by the number 
 *      specified.
 *
 * @return
 *      Nothing
 *
 */
EMCPAL_API
VOID
EmcpalSemaphoreSignal (PEMCPAL_SEMAPHORE  pSema,	/*!< Ptr to the semaphore to signal */
					   LONG               count	/*!< Value to increment the semaphore by */
                       );

EMCPAL_API
ULONG
EmcpalSemaphoreQuery(PEMCPAL_SEMAPHORE  pSema); /*!< Ptr to the semaphore to query */

/* @} End Semaphore interfaces */

/*!
 * @addtogroup emcpal_sync_events
 * @{
 */

/*!
 * @brief
 *      Creates an sync event object. The event object must be destroyed before your module can be unloaded.
 *
 * @return
 *      - STATUS_SUCCESS if the event was created
 *      - otherwise error code bubbled up
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSyncEventCreate (PEMCPAL_CLIENT      pPalClient,	/*!< PAL client that owns this event */
					   PEMCPAL_SYNC_EVENT  pEvt,		/*!< Pointer to event object */
					   const TEXT          *pName,		/*!< Object name (ascii string) */
					   BOOL             initialState	/*!< Whether the event should created in the signalled state */
                       );

CSX_STATIC_INLINE
VOID
EmcpalSyncEventMustDestroy(PEMCPAL_SYNC_EVENT  pEvt)
{
#if 0
     if (csx_p_always_true()) {
         CSX_PANIC();
     }
#endif
}

/*!
 * @brief
 *      Destroys a SyncEvent object
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalSyncEventDestroy(
    PEMCPAL_SYNC_EVENT  pEvt	/*!< Pointer to event object */
    );

/*!
 *      EmcpalSyncEventSet
 *
 * @brief
 *      Sets an sync event object
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalSyncEventSet(
    PEMCPAL_SYNC_EVENT  pEvt	/*!< Pointer to event object */
    );

/*!
 * @brief
 *      Clears a sync event object
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalSyncEventClear(
    PEMCPAL_SYNC_EVENT  pEvt	/*!< Pointer to event object */
    );

/*!
 * @brief
 *      Waits on a sync event object
 *
 *      NOTE - The timeout is in milliseconds, and must be non-negative.
 *      if a nonzero timeout value is specified it must be at least 15ms
 *      for portability reasons.
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if a timeout was specified and the wait timed out
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSyncEventWait (PEMCPAL_SYNC_EVENT  pEvt,		/*!< Pointer to event object*/
					 EMCPAL_TIMEOUT_MSECS pTimeout	/*!< timeout value (in millisecs) */
                     );

/*!
 * @brief
 *      Waits on a sync event object
 *
 *      NOTE - The timeout is in milliseconds, and must be non-negative.
 *      if a nonzero timeout value is specified it must be at least 15ms
 *      for portability reasons.
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if a timeout was specified and the wait timed out
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSyncEventWaitUserSpecial (PEMCPAL_SYNC_EVENT  pEvt,	/*!< Pointer to event object */
                            EMCPAL_TIMEOUT_MSECS pTimeout	/*!< timeout value (in millisecs) */
                            );


/*!
 * @brief
 *      Try to wait on a sync event.  
 *
 *		If the event is in a signaled state
 *      the state will be cleared and the function will return success to
 *      the caller.  If the event is not in a signaled state the function
 *      will return immediately with a status of STATUS_TIMEOUT
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if the wait did not succeed
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSyncEventTryWait(
    PEMCPAL_SYNC_EVENT      pEvt	/*!< Pointer to event object */
    );

/*!
 * @brief
 *      This function will set a sync event to a nonsignaled state and return
 *      the previous state to the caller.
 *
 * @return
 *      - TRUE if the event was in the signaled state
 *      - FALSE if the event was not in the signaled state
 *
 */
EMCPAL_API
BOOL
EmcpalSyncEventReset(
    PEMCPAL_SYNC_EVENT      pEvt	/*!< Pointer to event object */
    );

/* @} End Synchronization Event interfaces */

/*!
 * Rendezvous Event interfaces
 * @addtogroup emcpal_rendezvous__events
 * @{
 */

/*!
 * @brief
 *      Creates a rendezvous event object. Object must be destroyed before your module can be unloaded.
 *
 * @return
 *      - STATUS_SUCCESS if the event was created
 *      - otherwise error code bubbled up
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalRendezvousEventCreate (PEMCPAL_CLIENT            pPalClient,	/*!< PAL client that owns this event */
							 PEMCPAL_RENDEZVOUS_EVENT  pEvt,		/*!< Pointer to event object */
							 const TEXT                *pName,		/*!< Object name (ascii string) */
							 BOOL                   initialState	/*!< Whether the event should created in the signalled state */
                             );

CSX_STATIC_INLINE
VOID
EmcpalRendezvousEventMustDestroy(PEMCPAL_RENDEZVOUS_EVENT  pEvt)
{
#if 0
     if (csx_p_always_true()) {
         CSX_PANIC();
     }
#endif
}

/*!
 *  @brief
 *       Destroys a rendezvous event object
 * 
 *  @pre
 *	Missing detail header file
 *
 *  @post
 *  Missing detail header file
 *
 *  @return
 *       None
 * 
 *  @version
 *       10/24/08   AMD - Created.
 * 
 */
EMCPAL_API
VOID
EmcpalRendezvousEventDestroy(
    PEMCPAL_RENDEZVOUS_EVENT    pEvt  /*!< Pointer to event object */
    );

/*!
 * @brief
 *      Sets a rendezvous event object
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalRendezvousEventSet(
    PEMCPAL_RENDEZVOUS_EVENT    pEvt	/*!< Pointer to event object */
    );

/*!
 *++
 * @brief
 *      Clears a rendezvous event object
 *
 * @return
 *      None
 *
 */
EMCPAL_API
VOID
EmcpalRendezvousEventClear(
    PEMCPAL_RENDEZVOUS_EVENT    pEvt	/*!< Pointer to event object */
    );

/*!
 * @brief
 *      Waits on a rendezvous event object
 *
 *      NOTE - The timeout is in milliseconds, and must be non-negative.
 *      if a nonzero timeout value is specified it must be at least 15ms
 *      for portability reasons.
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if a timeout was specified and the wait timed out
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalRendezvousEventWait(
    PEMCPAL_RENDEZVOUS_EVENT    pEvt,		/*!< Pointer to event object */
    EMCPAL_TIMEOUT_MSECS        timeout	/*!< timeout value (in millisecs) */
    );

/*!
 * @brief
 *      Waits on a rendezvous event object
 *
 *      NOTE - The timeout is in milliseconds, and must be non-negative.
 *      if a nonzero timeout value is specified it must be at least 15ms
 *      for portability reasons.
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if a timeout was specified and the wait timed out
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalRendezvousEventWaitUserSpecial(
    PEMCPAL_RENDEZVOUS_EVENT    pEvt,		/*!< Pointer to event object */
    EMCPAL_TIMEOUT_MSECS        timeout	/*!< timeout value (in millisecs) */
    );

/*!
 * @brief
 *      Try to wait on a rendezvous event object.  
 *
 *		If the event is in a
 *      signaled state the event will remain in that state and the function
 *      will return STATUS_SUCCESS to the caller.  If the event is not in the
 *      signaled state then the function will return immediately with a status
 *      of STATUS_TIMEOUT.
 *
 * @return
 *      - STATUS_SUCCESS if the wait succeeded
 *      - STATUS_TIMEOUT if the event was not signaled
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalRendezvousEventTryWait(
	PEMCPAL_RENDEZVOUS_EVENT  	pEvt	/*!< Pointer to event object */
    );

/*!
 * @brief
 *      This function will return a boolean value indicating whether or not
 *      a rendezvous event was in the signaled state.
 *
 * @return
 *      - TRUE if the event was in the signaled state
 *      - FALSE if the event was not in the signaled state
 *
 */
EMCPAL_API
BOOL
EmcpalRendezvousEventWasSignaled(
	PEMCPAL_RENDEZVOUS_EVENT  	pEvt	/*!< Pointer to event object */
    );

/* @} End Rendezvous Event interfaces */

/*!
 * @addtogroup emcpal_timer
 * @{
 */

/*!
 * @brief
 *      Creates a standard one-shot timer
 *
 * @return
 *      - STATUS_INVALID_PARAMETER - No callback
 *      - STATUS_SUCCESS - Otherwise
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalMonostableTimerCreate (PEMCPAL_CLIENT         	pPalClient,	/*!< PAL client that owns this timer */
							 PEMCPAL_MONOSTABLE_TIMER	pTimer,		/*!< Ptr to the timer to be created */
							 const TEXT                *pName,		/*!< Timer's name (Ascii string) */
							 EMCPAL_TIMER_CALLBACK	   	pCallback,	/*!< The function to call when the timer expires */
							 EMCPAL_TIMER_CONTEXT		pContext	/*!< An opaque argument to the callback */
                             );

/*!
 * @brief
 *      Destroys a timer.
 *
 *		Timer destroys perform a
 *      synchronous cancel so the caller doesn't have to worry
 *      whether the timer is running when destroying it.
 *
 * @return
 *      Nothing
 *
 */
EMCPAL_API
VOID
EmcpalMonostableTimerDestroy (PEMCPAL_MONOSTABLE_TIMER  pTimer	/*!< the timer to destroy */
                              );

/*!
 * @brief
 *      Starts an one-shot timer.
 *
 * @return
 *      STATUS_SUCCESS
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalMonostableTimerStart (PEMCPAL_MONOSTABLE_TIMER    pTimer,	/*!< the timer to start */
							EMCPAL_TIMEOUT_MSECS  		timeout /*!< Timeout in milliseconds */
                            );

/*!
 * @brief
 *      Cancels an one-shot timer.
 *
 * @return
 *      Nothing
 *
 */
EMCPAL_API
VOID
EmcpalMonostableTimerCancel (PEMCPAL_MONOSTABLE_TIMER  pTimer	/*!< the timer to cancel */
                             );

/*!
 * @brief
 *      Creates a free-running timer
 *
 * @return
 *      - STATUS_INVALID_PARAMETER - No callback
 *      - STATUS_SUCCESS - Otherwise
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalAstableTimerCreate (PEMCPAL_CLIENT         pPalClient,	/*!< PAL client that owns this timer */
						  PEMCPAL_ASTABLE_TIMER	 pTimer,		/*!< Ptr to the timer to be created */
						  const TEXT            *pName,			/*!< Timer's name (Ascii string)*/
						  EMCPAL_TIMER_CALLBACK	 pCallback,		/*!< The function to call when the timer expires */
						  EMCPAL_TIMER_CONTEXT	 pContext		/*!< An opaque argument to the callback */
                          );

/*!
 * @brief
 *      Destroys a free-running timer.
 *
 *		Timer destroys perform a
 *      synchronous cancel so the caller doesn't have to worry
 *      whether the timer is running when destroying it.
 *
 * @return
 *      STATUS_SUCCESS
 *
 */
EMCPAL_API
VOID
EmcpalAstableTimerDestroy (PEMCPAL_ASTABLE_TIMER  pTimer	/*!< the timer to destroy */
                           );

/*!
 * @brief
 *      Starts a free-running timer.
 *
 * Arguments:
 *
 *      pTimer - the timer to start.
 *
 * @return
 *      STATUS_SUCCESS
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalAstableTimerStart (PEMCPAL_ASTABLE_TIMER      pTimer,	/*!< the timer to start */
						 EMCPAL_TIMEOUT_MSECS		timeout /*!< Timeout in milliseconds */
                         );

/*!
 * @brief
 *      Cancel a free running timer.
 *
 * @return
 *      VOID
 *
 */
EMCPAL_API
VOID
EmcpalAstableTimerCancel (PEMCPAL_ASTABLE_TIMER  pTimer	/*!< the timer to cancel */
                          );

/* @} END OF emcpal_timer */

/*!
 * @addtogroup emcpal_time
 * @{
 */


/*!
 * @brief
 *      Returns the time since boot in milliseconds.
 *
 * @return
 *      Millisecond tick count - keeps moving even when in debugger
 *
 */
EMCPAL_API
EMCPAL_TIME_MSECS
EmcpalTimeReadMilliseconds (VOID);

/*!
 * @brief
 *      Returns the time since boot in microseconds.
 *
 * @return
 *      Microsecond tick count - keeps moving even when in debugger
 *
 */
CSX_STATIC_FINLINE
EMCPAL_TIME_MSECS
EmcpalTimeReadMicroseconds (VOID)
{
	EMCPAL_TIME_USECS  curTime;

#ifdef EMCPAL_USE_CSX_TIM_DPC

    csx_rt_time_get_clock_time_usecs(&curTime);

#else
    EMCPAL_INT_PANIC("EmcpalTimeReadMicroseconds: CSX timers required\n");
#endif

	return (curTime);

}  // end EmcpalTimeReadMicroseconds ()

/*!
 * @brief
 *      Returns the tick count since boot - stops moving when in debugger
 *
 * @return
 *      tick count since boot - use EmcpalQueryTimeIncrement() to know how long a tick is in actual time units
 *
 */
EMCPAL_API
void
EmcpalQueryTickCount(EMCPAL_LARGE_INTEGER *tickCount /*!< Location to return tick count */
                     );

/*!
 * @brief
 *      Returns the time since 1601 in 100 nanosecond units
 *
 * @return
 *      100 nanosecond units since 1601
 *
 */
EMCPAL_API
void
EmcpalQuerySystemTime(EMCPAL_TIME_100NSECS *systemTime /*!< Location to return time in 100nsec units */
                      );

/*!
 * @brief
 *      Returns the number of 100ns units since system boot
 *
 * @return
 *      100 nanosecond units since system boot
 *
 */
EMCPAL_API csx_u64_t EmcpalQueryInterruptTime (void);

/*!
 * @brief
 *      Missing descriptions (TBD)
 *
 * @return
 *      Missing descriptions
 *
 */
EMCPAL_API
void
EmcpalSystemTimeToLocalTime(EMCPAL_TIME_100NSECS *SystemTime,	/*!< System time */
                            EMCPAL_TIME_100NSECS *LocalTime	/*!< System time adjusted for time zone */
                            );

/*!
 * @brief
 *      Missing descriptions (TBD)
 *
 * @return
 *      Missing descriptions
 *
 */
EMCPAL_API
ULONG
EmcpalQueryTimeIncrement(void);

/*!
 * @brief
 *      Missing descriptions (TBD)
 *
 * @return
 *      Missing descriptions
 *
 */
EMCPAL_API
UINT_64E
EmcpalGetTimestampCounter(UINT_64E *TimestampFrequency	/*!< Need detail */
                          );


/*EMCPAL_LOCAL CSX_FINLINE
void
EmcpalTimeToTimeFields(EMCPAL_TIME_100NSECS * time, TIME_FIELDS *timeFields)
{
#ifdef EMCPAL_USE_CSX_TIM_DPC
        csx_precise_time_t t;
        csx_date_t d;
        csx_p_cvt_nt_time_to_precise_time_info(EMCPAL_CONVERT_100NSECS_TO_MSECS(time->QuadPart), &t);
        csx_p_cvt_precise_time_info_to_date_info(&t,&d);
        timeFields->Weekday=d.tm_wday;
        timeFields->Year=d.tm_year;
        timeFields->Month=d.tm_mon;
        timeFields->Day=d.tm_mday;
        timeFields->Hour=d.tm_hour;
        timeFields->Minute=d.tm_min;
        timeFields->Second=d.tm_sec;
        timeFields->Milliseconds=t.tv_usec/1000;
#else
    RtlTimeToTimeFields(time, timeFields);
#endif
}*/

/* @} END OF emcpal_time */


/*!
 * Thread interfaces
 * @addtogroup emcpal_thread
 * @{
 */

/*!
 * @brief
 *      Creates a new thread.
 *
 * @return
 *      - STATUS_SUCCESS (currently always)
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalThreadCreate (PEMCPAL_CLIENT					pPalClient,	/*!< Ptr to owner's EmcPAL client object*/
					PEMCPAL_THREAD					pThread,	/*!< Thread handle returned by OS */
					const TEXT				   		*pName,		/*!< Name of thread (ascii) */
					EMCPAL_THREAD_START_FUNCTION   	startFunc,	/*!< The start function for the new thread */
					EMCPAL_THREAD_CONTEXT			threadContext	/*!< The opaque start function argument */
                    );

/*!
 * @brief
 *      Creates a new thread with the specified thread affinity.
 *
 * @return
 *      - STATUS_SUCCESS (currently always)
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalThreadCreateWithAffinity (PEMCPAL_CLIENT		pPalClient,	/*!< Ptr to owner's EmcPAL client object*/
					PEMCPAL_THREAD					pThread,	/*!< Thread handle returned by OS */
					const TEXT				   		*pName,		/*!< Name of thread (ascii) */
					EMCPAL_THREAD_START_FUNCTION   	startFunc,	/*!< The start function for the new thread */
					EMCPAL_THREAD_CONTEXT			threadContext,	/*!< The opaque start function argument */
					ULONGLONG						affinity);	/*!< The desired affinity (or 0 for all) */
/*!
 * @brief
 *      Destroys (cleans up) a thread.
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadDestroy (PEMCPAL_THREAD  pThread	/*!< Thread handle */
                     );

/*!
 * @brief
 *      Exits the current thread.  (I.e., this function doesn't return.)
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadExit (VOID);

/*!
 * @brief
 *      Changes the priority of the current thread.
 *
 * @return
 *      The thread's priority prior to it being changed.
 *
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadPrioritySet (EMCPAL_THREAD_PRIORITY		newPriority	/*!< The new priority */
                         );

/*!
 * @brief
 *      Changes the priority of the intended thread object.
 *
 * @return
 *      The thread's priority prior to it being changed.
 *
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadPrioritySetExternal (PEMCPAL_THREAD     pThread,	/*!< Thread handle */
                                 EMCPAL_THREAD_PRIORITY     newPriority	/*!< The new priority */
                                 );

/*!
 * @brief
 *      Retrieves the thread's priority
 *
 * @return
 *      Thread's current priority
 *
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadPriorityGet (VOID);

/*!
 * @brief
 *      Retrieves the thread's priority of the intended thread object
 *
 * @return
 *      Thread's current priority
 *
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadPriorityGetExternal (PEMCPAL_THREAD     pThread  /*!< the thread handle */
                                 );

/*!
 * @brief
 *      Affines the specified thread according to the specified processor
 *		mask.
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadAffine (PEMCPAL_THREAD			pThread,		/*!< the thread handle */
					EMCPAL_PROCESSOR_MASK	newAffinity	/*!< the processor mask of the preferred CPUs */
                    );

/*!
 * @brief
 *      Windows kernel does not round robin processor for newly created threads which are not
 *      affined so we must do it for them.  No op in all environments but Windows kernel
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadSoftAffineBalance (PEMCPAL_THREAD			pThread		/*!< the thread handle */
                               );

/*!
 * @brief
 *      Cause the current thread to sleep for the specified time (in milliseconds).
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadSleep (EMCPAL_TIMEOUT_MSECS	waitTime	/*!< wait time in milliseconds */
                   );

/*!
 * @brief
 *      Cause the current thread to sleep for the specified time (in
 *      milliseconds).  
 *		
 *		This is used when we need to try to sleep for
 *      less than the minimum number of milliseconds (15ms).  This is not
 *      guaranteed to sleep for this amount of time in all operating environments.
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadSleepOptimistic (EMCPAL_TIMEOUT_MSECS	waitTime	/*!< wait time in milliseconds */
                             );

/*!
 * @brief
 *      Waits for the specified thread to exit (like pthread_join)
 *
 * @return
 *      Void
 *
 */
EMCPAL_API
VOID
EmcpalThreadWait (PEMCPAL_THREAD  pThread	/*!< the thread that will exit*/
                  );

/*!
 * @brief
 *      Returns a unique ID value for the current thread.
 *
 * @return
 *      The thread's ID value
 *
 */
EMCPAL_API
EMCPAL_THREAD_ID
EmcpalThreadGetID (VOID);

/*!
 * @brief
 *      Returns whether a thread object has been created, but not yet destroyed
 *
 * @return
 *      TRUE - The thread is alive
 *
 */
EMCPAL_API
BOOL
EmcpalThreadAlive (PEMCPAL_THREAD  pThread	/*!< the thread object */
                   );
/*!
 * @brief
 *      This API should only be used on kernel threads. Behavior will be incorrect
 *      on user space threads. Avoid this function if at all possible. Use
 *      EmcpalThreadDataPathPriorityInversionProtectionBegin/End or
 *      EmcPalSetThreadToDataPathPriority instead.
 *
 *      Manipulates thread base priority.
 *
 * @param pThread Thread whose base priority should be adjusted.
 * @param targetPriorityIncrement  The increment to add to the base priority. This value is
 *           relative to process base priority.
 *
 * @return
 *      Previous increment
 */
EMCPAL_API
LONG
EmcpalThreadBasePrioritySetExternal(PEMCPAL_THREAD pThread, LONG targetPriorityIncrement);

/*!
 * @brief
 *      This API should only be used on kernel threads. Behavior will be incorrect
 *      on user space threads. Avoid this function if at all possible. Use
 *      EmcpalThreadDataPathPriorityInversionProtectionBegin/End or
 *      EmcPalSetThreadToDataPathPriority instead.
 *
 *      Sets current thread base priority to specified value. On Windows, this function
 *      assumes the process base priority is 8, which should be correct for kernel threads.
 *
 * @param newPriority  New base priority.
 *
 * @return
 *      Previous priority.
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadBaseAndPrioritySet (EMCPAL_THREAD_PRIORITY newPriority);

/*!
 * @brief
 *      This API should only be used on kernel threads. Behavior will be incorrect
 *      on user space threads. Avoid this function if at all possible. Use
 *      EmcpalThreadDataPathPriorityInversionProtectionBegin/End or
 *      EmcPalSetThreadToDataPathPriority instead.
 *
 *      Sets thread base priority to specified value. On Windows, this function
 *      assumes the process base priority is 8, which should be correct for kernel
 *      threads.
 *
 * @param pThread Thread whose priority should be adjusted.
 * @param newPriority  New priority.
 *
 * @return
 *      Previous priority.
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadBaseAndPrioritySetExternal (PEMCPAL_THREAD pThread, EMCPAL_THREAD_PRIORITY newPriority);

/*!
 * @brief
 *      Boosts current thread priority to data path priority to avoid priority inversion issues.
 *
 * @return
 *      Previous priority: This value should be stored and passed back to
 *            EmcpalThreadDataPathPriorityInversionProtectionEnd().
 */

EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcpalThreadDataPathPriorityInversionProtectionBegin(VOID);

/*!
 * @brief
 *      Restore current thread priority to original priority when priority inversion protection
 *      is no longer necessary.
 *
 * @param origPriority  Original priority of thread returned from
 *             EmcpalThreadDataPathPriorityInversionProtectionBegin().
 *
 * @return
 *      None
 */

EMCPAL_API
VOID
EmcpalThreadDataPathPriorityInversionProtectionEnd(EMCPAL_THREAD_PRIORITY origPriority);

/*!
 * @brief
 *      This API should only be used on kernel threads. Behavior will be incorrect
 *      on user space threads. 
 *
 *      Changes current thread base priority to data path priority.  The expected use
 *      case is to call this in your thread entry function once for threads that
 *      should be running at data path priority.
 *
 * @return
 *      None
 */
EMCPAL_API
VOID
EmcPalSetThreadToDataPathPriority(VOID);

/*!
 * @brief
 *      This API should only be used on kernel threads. Behavior will be incorrect
 *      on user space threads. 
 *
 *      Changes specified thread base priority to data path priority.  The expected use
 *      case is to call this function once after thread creation for a thread that
 *      should be running at data path priority.
 *
 * @param pThread Thread whose priority should be adjusted.
 *
 * @return
 *      None
 */
EMCPAL_API
VOID
EmcPalSetThreadToDataPathPriorityExternal(PEMCPAL_THREAD pThread);

#ifdef EMCPAL_USE_CSX_THR
#if defined(CSX_BO_USER_PREEMPTION_DISABLE_SUPPORTED) && !defined(SIMMODE_ENV)
#define EMCPAL_DATA_PATH_THREAD_PRIORITY EMCPAL_THREAD_PRIORITY_VERY_HIGH
#else /* defined(CSX_BO_USER_PREEMPTION_DISABLE_SUPPORTED) && !defined(SIMMODE_ENV) */
#define EMCPAL_DATA_PATH_THREAD_PRIORITY EMCPAL_THREAD_PRIORITY_REALTIME_LOW
#endif /* defined(CSX_BO_USER_PREEMPTION_DISABLE_SUPPORTED) && !defined(SIMMODE_ENV) */
#else /* EMCPAL_USE_CSX_THR */
#define EMCPAL_DATA_PATH_THREAD_PRIORITY EMCPAL_THREAD_PRIORITY_VERY_HIGH
#endif /* EMCPAL_USE_CSX_THR */

/*!
 * @brief
 *      Returns the current priority level that is used for data path priority.
 *
 * @return
 *      current priority level that is used for data path priority.
 */
EMCPAL_API
EMCPAL_THREAD_PRIORITY
EmcPalGetDataPathPriority(VOID);

/*!
 * @brief
 *      Returns a pointer identifying the current user-space process associated with a thread
 *
 * @return
 *      Pointer identifying the current user-space process associated with a thread
 */
CSX_STATIC_INLINE
PVOID
EmcpalThreadAssociatedProcess(VOID)
{
#if defined(ALAMOSA_WINDOWS_ENV) && (!defined(UMODE_ENV)) && (!defined(SIMMODE_ENV))
    return (PVOID) PsGetCurrentProcessId();
#else
    return NULL;
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - API is not really viable or useful in cases other than the Windows kernel case */
}

/* @} End emcpal_thread */

/*!
 * @addtogroup emcpal_spinlock
 * @{
 * Spinlock interfaces
 */

/*!
 * @brief
 *      Creates a spinlock object from the supplied memory.
 * @param pPalClient Pointer to owner's EmcPAL client object
 * @param pSpinlock The spinlock object
 * @param pName Name of spinlock (ascii)
 *
 * @return STATUS_SUCCESS
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSpinlockCreate (PEMCPAL_CLIENT    pPalClient,
                                          PEMCPAL_SPINLOCK      pSpinlock,
                                          const TEXT            *pName);

CSX_STATIC_INLINE
VOID
EmcpalSpinlockMustDestroy(PEMCPAL_SPINLOCK  pSpinlock)
{
#if 0
     if (csx_p_always_true()) {
         CSX_PANIC();
     }
#endif
}

/*!
 * @brief
 *      Destroys a spinlock object from the supplied memory.
 *
 * @return
 *      STATUS_SUCCESS
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSpinlockDestroy (PEMCPAL_SPINLOCK  pSpinlock	/*!< The spinlock object */
                       );

/*!
 * @brief
 *      Creates a queued spinlock object from the supplied memory.
 *
 *		Note that this function exists because it is forseen that the
 *		future CSX implementation will require a unique spinlock object. 
 *
 * @param pPalClient Pointer to owner's EmcPAL client object
 * @param pSpinlock The spinlock object
 * @param pName Name of spinlock (ascii)
 *
 * @return
 *      STATUS_SUCCESS
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSpinlockQueuedCreate (PEMCPAL_CLIENT                      pPalClient,
                                                        PEMCPAL_SPINLOCK_QUEUED pSpinlock,
                                                        const TEXT                      *pName);

CSX_STATIC_INLINE
VOID
EmcpalSpinlockQueuedMustDestroy(PEMCPAL_SPINLOCK_QUEUED pSpinlock)
{
#if 0
     if (csx_p_always_true()) {
         CSX_PANIC();
     }
#endif
}

/*!
 * @brief
 *      Destroys a queued spinlock object.
 *
 * @return
 *      STATUS_SUCCESS
 *
 */
EMCPAL_API
EMCPAL_STATUS
EmcpalSpinlockQueuedDestroy (PEMCPAL_SPINLOCK_QUEUED  pSpinlock	/*!< The spinlock object */
                             );

/* @} End emcpal_spinlock */


/*!
 * Thread interfaces
 * @addtogroup emcpal_registry
 * @{
 * Registry Access interfaces
 */
/*! 
 * @{
 * @ingroup emcpal_registry
 */
/*!
 *      Define the access flags used when opening registry keys.
 */

#define EMCPAL_REG_KEY_QUERY_VALUE         EMCUTIL_REG_KEY_QUERY_VALUE
#define EMCPAL_REG_KEY_SET_VALUE           EMCUTIL_REG_KEY_SET_VALUE
#define EMCPAL_REG_KEY_CREATE_SUB_KEY      EMCUTIL_REG_KEY_CREATE_SUB_KEY
#define EMCPAL_REG_KEY_ENUMERATE_SUB_KEYS  EMCUTIL_REG_KEY_ENUMERATE_SUB_KEYS
#define EMCPAL_REG_KEY_READ                EMCUTIL_REG_KEY_READ
#define EMCPAL_REG_KEY_WRITE               EMCUTIL_REG_KEY_WRITE
#define EMCPAL_REG_KEY_EXECUTE             EMCUTIL_REG_KEY_EXECUTE
#define EMCPAL_REG_KEY_ALL_ACCESS          EMCUTIL_REG_KEY_ALL_ACCESS

/* @} end ingroup */
/*! 
 * @{
 * @ingroup emcpal_registry
 */
/*!
 *************************************************
 * EmcpalGetRegistryValueAs...(). where ... = String, MultiStr, Uint32, Uint64, Binary
 **************************************************
 *
 * Description of the GET Routines:
 *   There is a "Get" function to read the types of values found in the Windows Registry.
 *   <ul>
 *   <li>EmcpalGetRegistryValueAsString()</li>
 *   <li>EmcpalGetRegistryValueAsMultiStr()</li>
 *   <li>EmcpalGetRegistryValueAsUint32()</li>
 *   <li>EmcpalGetRegistryValueAsUint64()</li>
 *   <li>EmcpalGetRegistryValueAsBinary()</li>
 *	 </ul>
 *   In addition, there are versions for the same five value types that return a
 *   default if the value is missing from the registry.
 *   <ul>
 *   <li>EmcpalGetRegistryValueAsStringDefault()</li>
 *   <li>EmcpalGetRegistryValueAsMultiStrDefault()</li>
 *   <li>EmcpalGetRegistryValueAsUint32Default()</li>
 *   <li>EmcpalGetRegistryValueAsUint64Default()</li>
 *   <li>EmcpalGetRegistryValueAsBinaryDefault()</li>
 *	 </ul>
 * Key assumptions:
 *  <ol>
 *  <li>The input path parameter is the full path to the key or parameter in question
 *     (e.g., for Windows Services keys the first part of the path would be 
 *		"\Registry\Machine\System\CurrentControlSet\Services" 
 *		Note that the leading "\" seems to be required.</li>
 *  <li>The caller knows the value "type" when calling a Get or Set function.</li>
 *  <li>No Default value is entered into a value with a Set routine.</li>
 *  <li>in the EmcpalGetRegistryValueAsString and EmcpalGetRegistryValueAsMultiStr
 *     the user must call EmcpalFreeRegistryMem() to free the buffer that is returned.</li>
 *	</ol>
 * Arguments Input to these functions:
 *  @param[in] pRegPath       Pointer to the full path (see assumption # 1 above).
 *  @param[in] pParmName      Pointer to the Name (ID) of the parameter.
 *  @param[in] xRet...		   Caller defined pointer to a ui32, ui65, char-string, or binary-string
 *  @param[in] xxxLen         This parameter is used as input for the EmcpalGetRegistryValueAsBinary
 *                                 function to specify the length of the caller defined storage space
 *                 This input parameter is required for EmcpalGetRegistryValueAsBinary
 *  @param[in] Default...     This parameter is used to specify the default that should be returned
 *                 if the value is not found.
 *  @param[in] defLen         The length in bytes of the default value (including the null terminate).
 *
 *  @param[out] xRet...		   Buffer or UINTxx containing the return value. 
 *
 * @note ADDITIONAL RETURN VALUES IN THE EmcpalGetRegistryValueAsString, 
 *                                 EmcpalGetRegistryValueAsMultiStr, and
 *                                 EmcpalGetRegistryValueAsBinary FUNCTIONS:
 *  @param[out] xxxLen         The length of the xRet buffer including the NULL terminator
 *                 This output parameter is optional for EmcpalGetRegistryValueAsString and
 *                 EmcpalGetRegistryValueAsMultiStr, it may be specified as NULL
 *
 * @return
 *  STATUS         Standard NT status codes
 *
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsString (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        CHAR**                                          ppRetString,
        ULONG*                                          pStringLen);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsStringDefault (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        CHAR**                                          ppRetString,
        ULONG*                                          pStringLen,
    const CHAR *                pDefaultStr);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsMultiStr (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        CHAR**                                          ppRetString,
        ULONG*                                          pStringLen);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsMultiStrDefault (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        CHAR**                                          ppRetString,
        ULONG*                                          pStringLen,
    const CHAR *                pDefaultString,
    ULONG                       defLen);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsUint32 (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        UINT_32E*                                       pRetValue);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsUint32Default (
        const CHAR *                            pRegPath,
        const CHAR *                            pParmName,
        UINT_32E *                                      pReturnValue,
    UINT_32E                                    defaultValue);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsUint64 (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        UINT_64E*                                       pRetValue);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsUint64Default (
        const CHAR *                            pRegPath,
        const CHAR *                            pParmName,
        UINT_64E *                                      pReturnValue,
    UINT_64E                                    defaultValue);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsBinary (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        UINT_8E*                                        pBinBuffer,
        ULONG*                                          pBinLen);

EMCPAL_API
EMCPAL_STATUS  
EmcpalGetRegistryValueAsBinaryDefault (
        const CHAR *                            pRegPath,
        const CHAR *                            pParmName,
        UINT_8E *                                       pBinBuffer,
        ULONG *                                         pBinLen,
    UINT_8E *                   pDefault,
    ULONG                       defLen);
/* @} END ingroup emcpal_registry */

/*! 
 * @{
 * @ingroup emcpal_registry
 */
/*!
 **************************************************
 * EmcpalSetRegistryValueAs...(). where ... = String, MultiStr, Uint32, Uint64, Binary
 **************************************************
 *
 * Description of the four SET Routines:
 *   There is a "Set" routine to write the types of values found in the Windows Registry.
 *  <ul>
 *	 <li>EmcpalSetRegistryValueAsString()</li>
 *   <li>EmcpalSetRegistryValueAsMultiStr()</li>
 *   <li>EmcpalSetRegistryValueAsUint32()</li>
 *   <li>EmcpalSetRegistryValueAsUint64()</li>
 *   <li>EmcpalSetRegistryValueAsBinary()</li>
 *	</ul>
 *
 * Key assumptions:
 *	<ol>
 *  <li>The input path parameter is the full path to the key or parameter in question
 *     (e.g., for Windows Services keys the first part of the path would be 
 *		"\Registry\Machine\System\CurrentControlSet\Services"
 *		Note that the leading "\" seems to be required.</li>
 *  <li>The caller knows the value "type" when calling a Set function (ie, AsString, AsUint32, etc.).</li>
 *  <li>No Default value is entered into a value with a Set routine.</li>
 *	</ol>
 *
 * Arguments Input to this function: 
 *  @param[in]	pRegPath       Pointer to the absolute path value of the Key entry.
 *  @param[in]	pParmName      Pointer to the Name (ID) of the value.
 *  @param[in]	xSet...		   Pointer to the string or value to be Set into the Registry
 *  @param[in]	xxxLen         Length of the char or binary string in xSet...
 *                 the length INCLUDES the terminating null characters, one
 *                 for strings, 2 for multiStrings
 *
 * @return 
 *  STATUS         Standard NT status codes
 *
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalSetRegistryValueAsString (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        const CHAR*                                     pSetString,
        ULONG                                           stringLen);

EMCPAL_API
EMCPAL_STATUS  
EmcpalSetRegistryValueAsMultiStr (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        const CHAR*                                     pSetString,
        ULONG                                           stringLen);

EMCPAL_API
EMCPAL_STATUS  
EmcpalSetRegistryValueAsUint32 (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
              UINT_32E                          setValue);

EMCPAL_API
EMCPAL_STATUS  
EmcpalSetRegistryValueAsUint64 (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
              UINT_64E                          setValue);

EMCPAL_API
EMCPAL_STATUS  
EmcpalSetRegistryValueAsBinary (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName,
        const UINT_8E*                          pSetBinStr,
              ULONG                                     setBinBytes);

/* @} END ingroup emcpal_registry */

/**************************************************
 * EmcpalDeleteRegistryValue
 **************************************************/
/*! @brief
 *   As the names of the above function implies, this function deletes a
 *   value from the OS's Registry.
 *
 * PARAMETERS:
 *   Each call to a EmcpalDeleteRegistryValue function must provide two pointers to arrays of chars:
 *   1) CHAR*  pRegPath  containing the full path to the Key, and
 *   2) CHAR*  pParmName containing the Name of the Value.
 *
 * RETURN VALUES/ERRORS:
 *  Standard NT status codes
 *
 * NOTES:
 * Key assumptions:
 *   1) The input path parameter is the full path to the registry parameter.
 *
 * HISTORY:
 *
**************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalDeleteRegistryValue (
        const CHAR*                                     pRegPath,
        const CHAR*                                     pParmName);

/*! 
 * @{
 * @ingroup emcpal_registry
 */
/*!
 **************************************************
 * EmcpalCheckRegistryKey()
 * EmcpalCreateRegistryKey()
 **************************************************
 *
 * @brief 
 *   Unlike the "Set" and "Get" routines, there is only one "Create" and one "Check" function.
 *   The input path parameter must be the full path.
 *   (e.g., for instance, for Windows Services keys the first part of the path would be
 *	 \\Registry\\Machine\\System\\CurrentControlSet\\Services 
 *
 * Arguments Input to this function:
 *  @param[in] pRegPath    Pointer to the full path of the Key.
 *
 * @return 
 *    STATUS_SUCCESS if the Key exists,<br />
 *    STATUS_INSUFFICIENT_RESOURCES if the path conversion from CHAR to wide CHAR fails<br />
 *	  STATUS_OBJECT_NAME_NOT_FOUND if the Key is not found<br />
 *
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalCheckRegistryKey(
        const CHAR*                     pRegPath);

EMCPAL_API
EMCPAL_STATUS  
EmcpalCreateRegistryKey(
        const CHAR*                     pRegPath);

/* @} end ingroup registry */
/*!
 *
 * @brief 
 *   This function is used to flush a registry key to disk.  This should be used
 *   if it is absolutely necessary to ensure that a key is flushed to disk
 *       immediately after it is written.
 *
 * @param [in] pRegPath Pointer to the full path of the Key.
 *
 * @return 
 *      STATUS_SUCCESS on success or NT status code bubbled up
 *
 **************************************************/
EMCPAL_API
EMCPAL_STATUS
EmcpalFlushRegistryKey(
        const CHAR*                     pRegPath);

/*!
 *************************************************
 * EmcpalAllocRegistryMem()
 **************************************************
 *
 *
 * @brief
 *      Routine to allocate Emcpal registry memory buffer
 *
 * @param [in] size size to allocate
 *
 * @return
 *   VOID *    Address of allocated memory,<br />
 *	 NULL on error
 *
 * @note
 *  EmcpalFreeRegistryMem() should be used to deallocate
 *  the buffer
 *
 ***************************************************/
EMCPAL_API
PVOID
EmcpalAllocRegistryMem(
        ULONG           size);

/*!
 *************************************************
 * EmcpalFreeRegistryMem()
 **************************************************
 *
 * @brief
 *      Routine to free the area that contains the returned string which was created from
 *      within an Emcpal function by the OS function ExFreePoolWithKey with Key = 'laPE'
 *
 * 
 * @param    pPoolArea	  Pointer to VOID
 *
 * @return:
 *    VOID   No value returned
 *
 * @note
 *        Specifically for use with EmcpalGetRegistryValueAsString,
 *    EmcpalGetRegistryValueAsMultiStr, and EmcpalRegistryEnumerateKey
 *    functions which return a buffer.
 *
 * @version
 *  3/15/09: RAB -- Created
 *
 **************************************************/
EMCPAL_API
VOID
EmcpalFreeRegistryMem(
        VOID*           pPoolArea);

/*! @brief EmcPAL registry value type E */
typedef enum {
        EMCPAL_REGISTRY_VALUE_TYPE_INVALID      = 0,
#ifdef REG_SZ
        EMCPAL_REGISTRY_VALUE_TYPE_STRING       = REG_SZ,
        EMCPAL_REGISTRY_VALUE_TYPE_BINARY       = REG_BINARY,
        EMCPAL_REGISTRY_VALUE_TYPE_U32          = REG_DWORD,
        EMCPAL_REGISTRY_VALUE_TYPE_MULTISTRING  = REG_MULTI_SZ,
        EMCPAL_REGISTRY_VALUE_TYPE_U64          = REG_QWORD,
#else
        EMCPAL_REGISTRY_VALUE_TYPE_STRING       = 1,
        EMCPAL_REGISTRY_VALUE_TYPE_BINARY       = 3,
        EMCPAL_REGISTRY_VALUE_TYPE_U32          = 4,
        EMCPAL_REGISTRY_VALUE_TYPE_MULTISTRING  = 7,
        EMCPAL_REGISTRY_VALUE_TYPE_U64          = 11,
#endif
    EMCPAL_REGISTRY_VALUE_TYPE_UNKNOWN      = 99,
} EMCPAL_REGISTRY_VALUE_TYPE_E;

/*! @brief EmcPAL Registry path: generic services */
#define EMCPAL_REGISTRY_PATH_GENERIC_SERVICES    "\\Registry\\Machine\\System\\CurrentControlSet\\Services"
/*! @brief EmcPAL Registry path: K10 config */
#define EMCPAL_REGISTRY_PATH_K10_CONFIG                 "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\K10DriverConfig"
/*! @brief EmcPAL Registry path: SCSI target config */
#define EMCPAL_REGISTRY_PATH_SCSITARG_CONFIG    "\\REGISTRY\\Machine\\System\\CurrentControlSet\\Services\\ScsiTargConfig"
/*! @brief EmcPAL Registry path: K10 max drives */
#define EMCPAL_REGISTRY_PATH_K10_MAX_DRIVES             EMCPAL_REGISTRY_PATH_K10_CONFIG "\\MaxDrives"
/*! @brief EmcPAL Registry path: K10 reboot */
#define EMCPAL_REGISTRY_PATH_K10_REBOOT                 "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Reboot\\Parameters"
/*! @brief EmcPAL Registry path: K10 FCDMTL device */
#define EMCPAL_REGISTRY_PATH_K10_FCDMTL_DEVICE  "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\fcdmtl\\Parameters\\Device"
/*! @brief EmcPAL Registry path: K10 CPD device */
#define EMCPAL_REGISTRY_PATH_K10_CPD_DEVICE             "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\cpd\\Parameters\\Device"
/*! @brief EmcPAL Registry path: K10 SNMP FAMIB */
#define EMCPAL_REGISTRY_PATH_K10_SNMP_FAMIB     "\\REGISTRY\\Machine\\SOFTWARE\\Wow6432Node\\emc\\K10\\SNMP\\FaMib"
/*! @brief EmcPAL Registry path: CMI */
#define EMCPAL_REGISTRY_PATH_CMI                "\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Services\\cmi\\Parameters"
/*! @brief EmcPAL Registry path: K10 global */
#define EMCPAL_REGISTRY_PATH_K10_GLOBAL                 "\\REGISTRY\\Machine\\SOFTWARE\\EMC\\K10\\Global"
/*! @brief EmcPAL Registry path: K10 64-to32 global */
#define EMCPAL_REGISTRY_PATH_K10_64_TO_32_GLOBAL "\\REGISTRY\\Machine\\SOFTWARE\\Wow6432Node\\EMC\\K10\\Global"
/*! @brief EmcPAL Registry path: local machine */
#define EMCPAL_REGISTRY_PATH_LOCAL_MACHINE              "\\REGISTRY\\MACHINE"
/*! @brief EmcPAL Registry path: buffer manager parameters */
#define EMCPAL_REGISTRY_PATH_MLU_BUFFER_MGR     "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\BufferMgr\\Parameters"
/*! @brief EmcPAL Registry path: shared memory parameters */
#define EMCPAL_REGISTRY_PATH_MLU_SHARED_MEM     "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\SharedMemory\\Parameters"
/*! @brief EmcPAL Registry path: NT mirror parameters */
#define EMCPAL_REGISTRY_PATH_NTMIRROR_PARAMETERS "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ntmirror\\Parameters"
/*! @brief EmcPAL Registry path: TDD auto insert drivers */
#define EMCPAL_REGISTRY_PATH_TDD_AUTO_INS_DRV    "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\DiskTargConfig\\AutoInsertDrivers"
/*! @brief EmcPAL Registry path: network control */
#define EMCPAL_REGISTRY_PATH_TCD_NETWORK_CONTROL "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Network"
/*! @brief EmcPAL Registry path: IPV6 parameters */
#define EMCPAL_REGISTRY_PATH_TCD_IPV6_INTERFACES "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip6\\Parameters\\Interfaces"
/*! @brief EmcPAL Registry path: K10 driver config layered info */
#define EMCPAL_REGISTRY_PATH_K10DRV_LAYERED_INFO "\\Registry\\Machine\\system\\CurrentControlSet\\Services\\K10DriverConfig\\LayeredInfo"

typedef void *  EMCPAL_REGISTRY_HANDLE;		/*!< EmcPAL registry handle */

/*!
 *************************************************
 * EmcpalRegistryOpenKey()
 **************************************************
 *
 * @brief 
 *   Open a handle to the registry key for later use with the enumerate function.
 *
 * 
 *  @param[in] pRegPath			Pointer to the full path of the Key to be opened.
 *	@param[in] parentHandle		Open Emcpal key handle to the parent key for pRegPath.
 *                                              This is optional and may be specified as NULL.  If the
 *                                              parameter is specified pRegPath must be relative to the
 *                                              path used to open the parent handle, otherwose pRegPath
 *                      must be the full path to the Key plus the Key itself
 *                              (e.g., if the key is "Tcip" in the Service registry the path is
 *		                "\Registry\Machine\System\CurrentControlSet\Services\Tcpip"
 *                              Note that the leading "\" seems to be required).
 *	@param[in] accessFlags			Access flags:<br />
 * <ul>
 *	<li>EMCPAL_REG_KEY_QUERY_VALUE			Read Key Values</li>
 *	<li>EMCPAL_REG_KEY_SET_VALUE			Write Key Values </li>
 *	<li>EMCPAL_REG_KEY_CREATE_SUB_KEY		Create Subkeys for the key</li>
 *	<li>EMCPAL_REG_KEY_ENUMERATE_SUB_KEYS	Read the key's subkeys</li>
 *	<li>EMCPAL_REG_KEY_READ					EMCPAL_REG_KEY_QUERY_VALUE | EMCPAL_REG_KEY_ENUMERATE_SUB_KEYS</li>
 *	<li>EMCPAL_REG_KEY_WRITE				EMCPAL_REG_KEY_SET_VALUE | EMCPAL_REG_KEY_CREATE_SUB_KEY</li>
 *	<li>EMCPAL_REG_KEY_EXECUTE				same as EMCPAL_REG_KEY_READ</li>
 *	<li>EMCPAL_REG_KEY_ALL_ACCESS			all access flags set</li>
 * </ul>
 *	@param[out] pEmcpalKeyHandle	Address to store the returned key handle
 *
 * @return 
 *	<ul>
 *    <li>STATUS_SUCCESS the key is opened and the handle created</li>
 *    <li>STATUS_INSUFFICIENT_RESOURCES	insufficient memory is available to complete the request</li>
 *    <li>STATUS_INVALID_HANDLE returned from lower level NT routine</li>
 *    <li>STATUS_ACCESS_DENIED	returned from lower level NT routine</li>
 * </ul>
 * @note
 *        The returned handle must be closed with the EmcpalRegistryCloseKey function to
 *        avoid a memory leak.
 *
 *
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalRegistryOpenKey(
        const CHAR*                                             pRegPath,
        const EMCPAL_REGISTRY_HANDLE    parentHandle,
        const ULONG                                             accessFlags,
        EMCPAL_REGISTRY_HANDLE *                pRetKeyHandle);

/*!
 *************************************************
 * EmcpalRegistryCloseKey()
 **************************************************
 *
 * @brief 
 *   Close a handle to the registry key.
 *
 * 
 * @param[in] EmcpalRegHandle		A registry key handle
 *
 * @return 
 * <ul>
 *  <li>STATUS_SUCCESS the key is closed and the handle deleted</li>
 *	<li>STATUS_INVALID_PARAMETER the parameter is invalid</li>
 *	<li>STATUS_INVALID_HANDLE the handle is not a valid format</li>
 * </ul>
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalRegistryCloseKey(
        EMCPAL_REGISTRY_HANDLE                  EmcpalRegHandle);

/*!
 *************************************************
 * EmcpalRegistryQueryKeyInfo()
 **************************************************
 *
 * @brief 
 *      Provide information about the number and sizes of subkeys for a key
 *
 *	@param[in] EmcpalRegHandle		A registry key handle
 *
 *	@param[out] pNumValues			Address to return the number of values
 *						Optional, may be specified as NULL
 *	@param[out] pMaxValueNameLen	Address to return the max value name length
 *						Optional, may be specified as NULL
 *	@param[out] pMaxValueDataLen	Address to return the max value data length
 *						Optional, may be specified as NULL
 *
 * @return 
 * <ul>
 *	<li>STATUS_SUCCESS the data has been returned</li>
 *	<li>STATUS_INVALID_PARAMETER one or more parameters are invalid</li>
 *	<li>STATUS_INVALID_HANDLE the handle is not a valid format</li>
 * </ul>
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalRegistryQueryKeyInfo(
        EMCPAL_REGISTRY_HANDLE          EmcpalRegHandle,
        ULONG *                                         pNumValues,
        ULONG *                                         pMaxValueNameLen,
        ULONG *                                         pMaxValueDataLen);

/*!
 *************************************************
 * EmcpalRegistryQueryValueInfo()
 **************************************************
 *
 * @brief 
 *      Provide information about the value
 *
 *	@param[in] EmcpalRegHandle		A registry key handle
 *	@param[in] pValueName			The name of the value under the open key that
 *                                              you want to query
 *
 *	@param[out] pDataLength			Address to return the length of the returned data string.
 *						Optional, may be specified as NULL
 *	@param[out] ppData				Address to return data buffer address.
 *                      This buffer will contain an ascii character representation
 *                      of the value's data
 *                                              Optional, may be specified as NULL
 *	@param[out] pRegPathLength		Address to return the length of the returned path.
 *                                              Optional, may be specified as NULL
 *	@param[out] ppRegPath			Address to return path buffer address
 *                                              This name is suitable for use in a call to one of
 *                                              the Emcpal registry get or set functions,
 *                                              Optional, may be specified as NULL
 *	@param[out] pParmNameLength		Address to return the length of the returned param name.
 *                                              Optional, may be specified as NULL
 *	@param[out] ppParmName			Address to return param name buffer address
 *                                              This name is suitable for use in a call to one of
 *                                              the Emcpal registry get or set functions,
 *                                              Optional, may be specified as NULL
 *
 * @return 
 *	<ul>
 *  <li>STATUS_SUCCESS the data has been returned</li>
 *	<li>STATUS_INVALID_PARAMETER one or more parameters are invalid</li>
 *	<li>STATUS_INVALID_HANDLE the handle is not a valid format</li>
 *  </ul>
 *
 * @note
 *      The caller must use EmcpalFreeRegistryMem() to free
 *      the ppData, ppRegPath, and ppParmName buffers after a successful call
 *  to this function
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalRegistryQueryValueInfo(
        EMCPAL_REGISTRY_HANDLE          EmcpalRegHandle,
        CHAR *                                          pValueName,
        ULONG *                                         pDataLength,
        CHAR **                                         ppData,
        ULONG *                                         pRegPathLength,
        CHAR **                                         ppRegPath,
        ULONG *                                         pParmNameLength,
        CHAR **                                         ppParmName);

/*!
 *************************************************
 * EmcpalRegistryEnumerateKey()
 **************************************************
 *
 * @brief 
 *      Provide information about the subkeys of an open registry key
 *
 *	@param[in] EmcpalRegHandle		A registry key handle
 *	@param[in] SubkeyIndex			the index of the desired subkey
 *
 *	@param[out] pNameLen			Address to return the name length
 *						Optional, may be specified as NULL
 *	@param[out] ppName				Address to return the address of the subkey name string
 *						Optional, may be specified as NULL
 *
 * @return 
 *  <ul>
 *	<li>STATUS_SUCCESS         The data has been returned</li>
 *	<li>STATUS_NO_MORE_ENTRIES The Index value is out of range for the registry key 
 *	                       specified by EmcpalRegHandle. 
 *                         For example, if a key has n subkeys, then for any value 
 *                         greater than n-1 the routine returns STATUS_NO_MORE_ENTRIES. </li>
 *	<li>STATUS_INVALID_PARAMETER one or more parameters are invalid</li>
 *	<li>STATUS_INVALID_HANDLE the handle is not a valid format</li>
 *  <li>STATUS_INSUFFICIENT_RESOURCES	insufficient memory is available to complete the request</li>
 *  </ul>
 *
 * @note
 *  THE CALLER MUST CALL EmcpalFreeRegistryMem when done with ppName.
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalRegistryEnumerateKey(
        EMCPAL_REGISTRY_HANDLE          EmcpalRegHandle,
        ULONG                                           SubkeyIndex,
        ULONG *                                         pNameLen,
        CHAR **                                         ppName);

/*!
 *************************************************
 * EmcpalRegistryEnumerateValueKey()
 **************************************************
 *
 * @brief 
 *      Provide information about the value keys of an open registry key
 *
 *	@param[in] EmcpalRegHandle		A registry key handle
 *	@param[in] SubkeyIndex			the index of the desired value key
 *
 *	@param[out] pDataLength			Address to return the length of the returned data.
 *						Optional, may be specified as NULL
 *	@param[out] ppData				Address to return data buffer address
 *						Optional, may be specified as NULL
 *	@param[out] pNameLength			Address to return the name length
 *						Optional, may be specified as NULL
 *	@param[out] ppName				Address to return the address of the value key name string
 *						Optional, may be specified as NULL
 *
 * @return 
 *	<ul>
 *	<li>STATUS_SUCCESS         The data has been returned</li>
 *	<li>STATUS_NO_MORE_ENTRIES The Index value is out of range for the registry key 
 *	                       specified by EmcpalRegHandle. 
 *                         For example, if a key has n subkeys, then for any value 
 *                         greater than n-1 the routine returns STATUS_NO_MORE_ENTRIES. </li>
 *	<li>STATUS_INVALID_PARAMETER one or more parameters are invalid</li>
 *	<li>STATUS_INVALID_HANDLE the handle is not a valid format</li>
 *  <li>STATUS_INSUFFICIENT_RESOURCES	insufficient memory is available to complete the request
 *  plus error returns from underlying system and csx calls.</li>
 * </ul>
 * 
 * @note
 *  THE CALLER MUST CALL EmcpalFreeRegistryMem when done with ppName and ppData
 **************************************************/
EMCPAL_API
EMCPAL_STATUS  
EmcpalRegistryEnumerateValueKey(
        EMCPAL_REGISTRY_HANDLE          EmcpalRegHandle,
        ULONG                                           SubkeyIndex,
        ULONG *                                         pDataLength,
        VOID **                                         ppData,
        ULONG *                                         pNameLength,
        CHAR **                                         ppName);

/*
 * End Registry Access interfaces
 */
/* @} End emcpal_registry */

/*!
 * @addtogroup emcpal_misc
 * @{
 */
/*!
 *************************************************
 * EmcpalTranslateCsxStatusToNtStatus()
 **************************************************
 *
 * @brief 
 *      Provide a translation from CSX status code to NT ones.
 *
 * @param[in] csxStatus The CSX status code.
 *
 * @return 
 *  The NT status code equivalent.
 *
 **************************************************/
EMCPAL_API
EMCPAL_STATUS
EmcpalTranslateCsxStatusToNtStatus(
    csx_status_e csxStatus);

//++
// End Translate CSX status to NT Status
//--

/* @} End emcpal_misc */

/*!
 * @addtogroup emcpal_misc
 * @{
 */
/*!
 *************************************************
 * EmcpalIsDebuggerEnabled()
 **************************************************
 *
 * @brief 
 *      Checks if the debugger is enabled
 *
 * @return 
 *  <ul>
 *	<li>TRUE debugger is enabled
 *  <li>FALSE debuggger is not enabled or information not available
 *  </ul>
 *
 * @note
 * SAFEMESS - no real way to make this accurate for now on Linux, or even survivable
 **************************************************/
EMCPAL_LOCAL EMCPAL_INLINE
BOOL
EmcpalIsDebuggerEnabled(
    VOID)
{
#if defined(CSX_BV_ENVIRONMENT_WINDOWS) && defined(CSX_BV_LOCATION_KERNEL_SPACE)
    return KD_DEBUGGER_ENABLED;
#else
    return csx_rt_proc_recently_been_debugged();
#endif
}

/*!
 * @brief 
 *      Checks if the debugger is enabled
 *
 * @return 
 *  <ul>
 *	<li>TRUE debugger is enabled
 *  <li>FALSE debuggger is not enabled or information not available
 *  </ul>
 *
 * @note
 * SAFEMESS - no real way to make this accurate for now on Linux, or even survivable
 **************************************************/
EMCPAL_LOCAL EMCPAL_INLINE
BOOL
EmcpalIsDebuggerAttached(
    VOID)
{
#if defined(CSX_BV_ENVIRONMENT_WINDOWS) && defined(CSX_BV_LOCATION_KERNEL_SPACE)
    return !KD_DEBUGGER_NOT_PRESENT;
#else
    return csx_rt_proc_recently_been_debugged();
#endif
}
/* @} End emcpal_misc */

/*!
 * @ingroup emcpal_portio
 * @{
 */
/*!
 *************************************************
 * Emcpal READ WRITE PORT Macros
 * Applying CSX API abstraction for READ WRITE PORT
 *
 * Currently NO_ASSERT is used only in NvramstaticLib.c
 * (since it is a crash dump path)
 **************************************************/

#ifdef ALAMOSA_WINDOWS_ENV
#define EMCPAL_WRITE_PORT_ULONG(port,val)  csx_p_port_io_outl_direct(port, val)
#define EMCPAL_WRITE_PORT_UCHAR(port,val)  csx_p_port_io_outb_direct(port, val)
#define EMCPAL_WRITE_PORT_USHORT(port,val) csx_p_port_io_outw_direct(port, val)


#define EMCPAL_READ_PORT_ULONG(port ) csx_p_port_io_inl_direct(port)
#define EMCPAL_READ_PORT_UCHAR(port )  csx_p_port_io_inb_direct(port)
#define EMCPAL_READ_PORT_USHORT(port) csx_p_port_io_inw_direct(port)

#define EMCPAL_WRITE_PORT_ULONG_NO_ASSERT(port,val)  csx_p_port_io_outl(port, val)
#define EMCPAL_WRITE_PORT_UCHAR_NO_ASSERT(port,val)  csx_p_port_io_outb(port, val)
#define EMCPAL_WRITE_PORT_USHORT_NO_ASSERT(port,val) csx_p_port_io_outw(port, val)

#define EMCPAL_READ_PORT_UCHAR_NO_ASSERT(port, val_rv )  csx_p_port_io_inb(port, val_rv)
#define EMCPAL_READ_PORT_ULONG_NO_ASSERT(port, val_rv )  csx_p_port_io_inl (port,  val_rv)
#define EMCPAL_READ_PORT_USHORT_NO_ASSERT(port, val_rv) csx_p_port_io_inw(port,  val_rv)
#else
#define EMCPAL_WRITE_PORT_ULONG(port,val)  csx_port_io_outl(val, port)
#define EMCPAL_WRITE_PORT_UCHAR(port,val)  csx_port_io_outb(val, port)
#define EMCPAL_WRITE_PORT_USHORT(port,val) csx_port_io_outw(val, port)

#define EMCPAL_READ_PORT_ULONG(port ) csx_port_io_inl(port)
#define EMCPAL_READ_PORT_UCHAR(port )  csx_port_io_inb(port)
#define EMCPAL_READ_PORT_USHORT(port) csx_port_io_inw(port)

#define EMCPAL_WRITE_PORT_ULONG_NO_ASSERT(port,val)  csx_port_io_outl(val, port)
#define EMCPAL_WRITE_PORT_UCHAR_NO_ASSERT(port,val)  csx_port_io_outb(val, port)
#define EMCPAL_WRITE_PORT_USHORT_NO_ASSERT(port,val) csx_port_io_outw(val, port)

#define EMCPAL_READ_PORT_UCHAR_NO_ASSERT(port, val_rv )  { (*(val_rv)) = csx_port_io_inb(port); }
#define EMCPAL_READ_PORT_ULONG_NO_ASSERT(port, val_rv )  { (*(val_rv)) = csx_port_io_inl (port); }
#define EMCPAL_READ_PORT_USHORT_NO_ASSERT(port, val_rv) { (*(val_rv)) = csx_port_io_inw(port); }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - port I/O */

/* @} end emcpal_portio */

/*
 * Various internal functions that must be exported as they are referenced
 * by convenience macros and inlines that are user-visible
 */

/* CSX team will replace the macro definition to corresponding csx api in future*/
#ifdef ALAMOSA_WINDOWS_ENV
#define EMCPAL_WRITE_REGISTER_UCHAR(register,val) WRITE_REGISTER_UCHAR(register,val)
#define EMCPAL_WRITE_REGISTER_USHORT(register,val) WRITE_REGISTER_USHORT(register,val)
#define EMCPAL_WRITE_REGISTER_ULONG(register,val) WRITE_REGISTER_ULONG(register,val)
#define EMCPAL_WRITE_REGISTER_ULONG64(register,val) WRITE_REGISTER_ULONG64(register,val)

#define EMCPAL_READ_REGISTER_UCHAR(register) READ_REGISTER_UCHAR(register)
#define EMCPAL_READ_REGISTER_USHORT(register) READ_REGISTER_USHORT(register)
#define EMCPAL_READ_REGISTER_ULONG(register) READ_REGISTER_ULONG(register)
#define EMCPAL_READ_REGISTER_ULONG64(register) READ_REGISTER_ULONG64(register)

#define EMCPAL_WRITE_REGISTER_BUFFER_UCHAR(register,buffer,count) WRITE_REGISTER_BUFFER_UCHAR(register,buffer,count)  
#define EMCPAL_WRITE_REGISTER_BUFFER_USHORT(register,buffer,count) WRITE_REGISTER_BUFFER_USHORT(register,buffer,count) 
#define EMCPAL_WRITE_REGISTER_BUFFER_ULONG(register,buffer,count) WRITE_REGISTER_BUFFER_ULONG(register,buffer,count)  

#define EMCPAL_READ_REGISTER_BUFFER_UCHAR(register,buffer,count) READ_REGISTER_BUFFER_UCHAR(register,buffer,count)  
#define EMCPAL_READ_REGISTER_BUFFER_USHORT(register,buffer,count) READ_REGISTER_BUFFER_USHORT(register,buffer,count)
#define EMCPAL_READ_REGISTER_BUFFER_ULONG(register,buffer,count) READ_REGISTER_BUFFER_ULONG(register,buffer,count)  
#else
#define EMCPAL_WRITE_REGISTER_UCHAR(register,val) csx_p_mmio_out_8_1(register,val)
#define EMCPAL_WRITE_REGISTER_USHORT(register,val) csx_p_mmio_out_16_1(register,val)
#define EMCPAL_WRITE_REGISTER_ULONG(register,val) csx_p_mmio_out_32_1(register,val)
#define EMCPAL_WRITE_REGISTER_ULONG64(register,val) csx_p_mmio_out_64_1(register,val)

#define EMCPAL_READ_REGISTER_UCHAR(register) csx_p_mmio_in_8_1(register)
#define EMCPAL_READ_REGISTER_USHORT(register) csx_p_mmio_in_16_1(register)
#define EMCPAL_READ_REGISTER_ULONG(register) csx_p_mmio_in_32_1(register)
#define EMCPAL_READ_REGISTER_ULONG64(register) csx_p_mmio_in_64_1(register)

#define EMCPAL_WRITE_REGISTER_BUFFER_UCHAR(register,buffer,count) csx_p_mmio_out_8_n(register,buffer,count)
#define EMCPAL_WRITE_REGISTER_BUFFER_USHORT(register,buffer,count) csx_p_mmio_out_16_n(register,buffer,count)
#define EMCPAL_WRITE_REGISTER_BUFFER_ULONG(register,buffer,count) csx_p_mmio_out_32_n(register,buffer,count)

#define EMCPAL_READ_REGISTER_BUFFER_UCHAR(register,buffer,count) csx_p_mmio_in_8_n(register,buffer,count)
#define EMCPAL_READ_REGISTER_BUFFER_USHORT(register,buffer,count) csx_p_mmio_in_16_n(register,buffer,count)
#define EMCPAL_READ_REGISTER_BUFFER_ULONG(register,buffer,count) csx_p_mmio_in_32_n(register,buffer,count)
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - memory mapped I/O */


#ifdef EMCPAL_USE_CSX_DCALL

EMCPAL_API
csx_status_e CSX_GX_CI_DEFCC
EmcpalDeviceIrpCallWrapper(
    csx_p_dcall_handler_function_t dcall_handler,
    PEMCPAL_DEVICE_OBJECT pDeviceObject,
    PEMCPAL_IRP pIrp);

EMCPAL_API
csx_status_e CSX_GX_CI_DEFCC
EmcpalDeviceIrpCompletionWrapper(
    csx_p_dcall_completion_function_t level_completion_function,
    csx_p_dcall_completion_context_t level_completion_context,
    PEMCPAL_DEVICE_OBJECT pDeviceObject,
    PEMCPAL_IRP pIrp);

#endif

/*
 * Various abstractions for the IRQL/LEVEL concept that work in all cases
 */
#ifdef EMCPAL_CASE_WK
#define EMCPAL_LEVEL_IS_PASSIVE() (KeGetCurrentIrql() == PASSIVE_LEVEL)
#define EMCPAL_LEVEL_IS_DISPATCH() (KeGetCurrentIrql() == DISPATCH_LEVEL)
#define EMCPAL_LEVEL_NOT_PASSIVE() (KeGetCurrentIrql() != PASSIVE_LEVEL)
#define EMCPAL_LEVEL_NOT_DISPATCH() (KeGetCurrentIrql() != DISPATCH_LEVEL)
#define EMCPAL_LEVEL_ABOVE_PASSIVE() (KeGetCurrentIrql() > PASSIVE_LEVEL)
#define EMCPAL_LEVEL_ABOVE_DISPATCH() (KeGetCurrentIrql() > DISPATCH_LEVEL)
#define EMCPAL_LEVEL_BELOW_DISPATCH() (KeGetCurrentIrql() < DISPATCH_LEVEL)
#define EMCPAL_LEVEL_IS_OR_ABOVE_DISPATCH() (KeGetCurrentIrql() >= DISPATCH_LEVEL)
#define EMCPAL_LEVEL_IS_OR_BELOW_DISPATCH() (KeGetCurrentIrql() <= DISPATCH_LEVEL)
#define EmcpalGetCurrentLevel() KeGetCurrentIrql()
#define EMCPAL_THIS_LEVEL_IS_PASSIVE(_i) ((_i) == PASSIVE_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_DISPATCH(_i) ((_i) == DISPATCH_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_APC(_i) ((_i) == APC_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_ABOVE_DISPATCH(_i) ((_i) > DISPATCH_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_BELOW_DISPATCH(_i) ((_i) < DISPATCH_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_OR_BELOW_DISPATCH(_i) ((_i) <= DISPATCH_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_NOT_PASSIVE(_i) ((_i) != PASSIVE_LEVEL)
#define EMCPAL_THIS_LEVEL_IS_PASSIVE(_i) ((_i) == PASSIVE_LEVEL)
#define EMCPAL_LEVEL_PASSIVE PASSIVE_LEVEL
#define EMCPAL_LEVEL_APC APC_LEVEL
#define EMCPAL_LEVEL_HIGH HIGH_LEVEL
#define EMCPAL_LEVEL_DISPATCH DISPATCH_LEVEL
#define EmcpalLevelRaiseToSpecific(_saved, _ip) KeRaiseIrql(_saved, _ip)
#define EmcpalLevelRaiseToDispatch(_ip) KeRaiseIrql(DISPATCH_LEVEL, _ip)
#define EmcpalLevelRaiseToDispatchPlus(_ip) KeRaiseIrql(DISPATCH_LEVEL + 1, _ip)
#define EmcpalLevelRaiseToIntsDisabled(_ip) KeRaiseIrql(HIGH_LEVEL, _ip)
#define EmcpalLevelLower(_i) KeLowerIrql(_i)
#else
#define EMCPAL_LEVEL_IS_PASSIVE() (csx_p_execution_state_get() == CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_LEVEL_IS_DISPATCH() (csx_p_execution_state_get() == CSX_P_EXECUTION_STATE_NONBLOCKABLE)
#define EMCPAL_LEVEL_NOT_PASSIVE() (csx_p_execution_state_get() != CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_LEVEL_NOT_DISPATCH() (csx_p_execution_state_get() != CSX_P_EXECUTION_STATE_NONBLOCKABLE)
#define EMCPAL_LEVEL_ABOVE_PASSIVE() (csx_p_execution_state_get() != CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_LEVEL_ABOVE_DISPATCH() (csx_p_execution_state_get() == CSX_P_EXECUTION_STATE_INTERRUPT)
#define EMCPAL_LEVEL_BELOW_DISPATCH() (csx_p_execution_state_get() == CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_LEVEL_IS_OR_ABOVE_DISPATCH() (csx_p_execution_state_get() != CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_LEVEL_IS_OR_BELOW_DISPATCH() (csx_p_execution_state_get() != CSX_P_EXECUTION_STATE_INTERRUPT)
#define EmcpalGetCurrentLevel() csx_p_execution_state_get()
#define EMCPAL_THIS_LEVEL_IS_PASSIVE(_i) ((_i) == CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_THIS_LEVEL_IS_DISPATCH(_i) ((_i) == CSX_P_EXECUTION_STATE_NONBLOCKABLE)
#define EMCPAL_THIS_LEVEL_IS_APC(_i) (0)
#define EMCPAL_THIS_LEVEL_IS_ABOVE_DISPATCH(_i) ((_i) == CSX_P_EXECUTION_STATE_INTERRUPT)
#define EMCPAL_THIS_LEVEL_IS_BELOW_DISPATCH(_i) ((_i) == CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_THIS_LEVEL_IS_OR_BELOW_DISPATCH(_i) ((_i) != CSX_P_EXECUTION_STATE_INTERRUPT)
#define EMCPAL_THIS_LEVEL_IS_NOT_PASSIVE(_i) ((_i) != CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_THIS_LEVEL_IS_PASSIVE(_i) ((_i) == CSX_P_EXECUTION_STATE_THREADISH)
#define EMCPAL_LEVEL_PASSIVE CSX_P_EXECUTION_STATE_THREADISH
#define EMCPAL_LEVEL_APC CSX_P_EXECUTION_STATE_NEVERUSED
#define EMCPAL_LEVEL_HIGH ((csx_p_execution_state_e)99) //PZPZ
#define EMCPAL_LEVEL_DISPATCH CSX_P_EXECUTION_STATE_NONBLOCKABLE
#define EmcpalLevelRaiseToSpecific(_saved, _ip) csx_p_execution_state_raise(_saved, _ip)
#define EmcpalLevelRaiseToDispatch(_ip) csx_p_execution_state_raise(CSX_P_EXECUTION_STATE_NONBLOCKABLE, _ip)
#define EmcpalLevelRaiseToDispatchPlus(_ip) csx_p_execution_state_raise(CSX_P_EXECUTION_STATE_NONBLOCKABLE, _ip) //PZPZ
#define EmcpalLevelRaiseToIntsDisabled(_ip) csx_p_execution_state_raise(CSX_P_EXECUTION_STATE_NONBLOCKABLE, _ip) //PZPZ
#define EmcpalLevelLower(_i) csx_p_execution_state_lower(_i)
#endif

#ifndef EMCPAL_CASE_NTDDK_EXPOSE

EMCPAL_API VOID EmcpalBugCheckSimple(ULONG BugCheckCode);
EMCPAL_API VOID EmcpalBugCheck(ULONG BugCheckCode,
                               ULONG_PTR BugCheckParameter1,
                               ULONG_PTR BugCheckParameter2,
                               ULONG_PTR BugCheckParameter3,
                               ULONG_PTR BugCheckParameter4);

/*
 * Simple Workqueue Abstraction
 */
#ifdef EMCPAL_CASE_WK

typedef PWORKER_THREAD_ROUTINE EMCPAL_WORK_QUEUE_BODY_HANDLER;
typedef PVOID EMCPAL_WORK_QUEUE_BODY_CONTEXT;
typedef WORK_QUEUE_ITEM EMCPAL_WORK_QUEUE_ITEM;
typedef WORK_QUEUE_TYPE EMCPAL_WORK_QUEUE_PRIO;

#define EMCPAL_WORK_QUEUE_PRIO_CRITICAL CriticalWorkQueue
#define EMCPAL_WORK_QUEUE_PRIO_DELAYED DelayedWorkQueue

CSX_STATIC_INLINE VOID
EmcpalWorkQueueItemInitialize(
    EMCPAL_WORK_QUEUE_ITEM *work_queue_item,
    EMCPAL_WORK_QUEUE_BODY_HANDLER body_handler,
    EMCPAL_WORK_QUEUE_BODY_CONTEXT body_context)
{
    ExInitializeWorkItem(work_queue_item, body_handler, body_context);
}

CSX_STATIC_INLINE VOID
EmcpalWorkQueueItemEnqueue(
    EMCPAL_WORK_QUEUE_ITEM *work_queue_item,
    EMCPAL_WORK_QUEUE_PRIO work_queue_type)
{
    ExQueueWorkItem(work_queue_item, work_queue_type);
}

#else

typedef csx_p_wq_body_handler_t EMCPAL_WORK_QUEUE_BODY_HANDLER;
typedef csx_p_wq_body_context_t EMCPAL_WORK_QUEUE_BODY_CONTEXT;
typedef csx_p_wq_item_t EMCPAL_WORK_QUEUE_ITEM;
typedef csx_p_wq_prio_e EMCPAL_WORK_QUEUE_PRIO;

#define EMCPAL_WORK_QUEUE_PRIO_CRITICAL CSX_P_WQ_PRIO_HI
#define EMCPAL_WORK_QUEUE_PRIO_DELAYED CSX_P_WQ_PRIO_LO

CSX_STATIC_INLINE VOID
EmcpalWorkQueueItemInitialize(
    EMCPAL_WORK_QUEUE_ITEM *work_queue_item,
    EMCPAL_WORK_QUEUE_BODY_HANDLER body_handler,
    EMCPAL_WORK_QUEUE_BODY_CONTEXT body_context)
{
    csx_p_wq_item_initialize(work_queue_item, body_handler, body_context);
}

CSX_STATIC_INLINE VOID
EmcpalWorkQueueItemEnqueue(
    EMCPAL_WORK_QUEUE_ITEM *work_queue_item,
    EMCPAL_WORK_QUEUE_PRIO work_queue_type)
{
    csx_p_wq_item_enqueue(work_queue_item, work_queue_type);
}

#endif

#endif /* EMCPAL_CASE_NTDDK_EXPOSE */

#ifdef __cplusplus
}
#endif

//++
//.End file EmcPAL.h
//--

#include "EmcPAL_Inline.h"
#include "EmcPAL_Interlocked.h"
#include "EmcPAL_Dpc.h"
#include "EmcPAL_Generic.h"
#include "EmcPAL_List.h"
#include "EmcPAL_MapIoSpace.h"
#include "EmcPAL_MemoryAlloc.h"
#include "EmcPAL_Memory.h"
#include "EmcPAL_Misc.h"
#include "EmcPAL_String.h"

#endif /* _EMCPAL_H_ */
