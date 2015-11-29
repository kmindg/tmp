#ifndef __FBE_PLATFORM_H
#define __FBE_PLATFORM_H

/*****************************/

#if defined(UMODE_ENV)
#ifdef ALAMOSA_WINDOWS_ENV
#define FBE_PLATFORM_USE_WIN32
#else
#define FBE_PLATFORM_USE_CSX
#endif
#elif defined(SIMMODE_ENV)
#define FBE_PLATFORM_USE_PAL
#else
#define FBE_PLATFORM_USE_PAL
#endif

#include "csx_ext.h"
#if defined(FBE_PLATFORM_USE_WIN32)
#include "fbe/fbe_types.h"
#include "EmcPAL.h"
#elif defined(FBE_PLATFORM_USE_WINDDK)
#include "EmcPAL.h"
#include "fbe/fbe_types.h"
#elif defined(FBE_PLATFORM_USE_PAL)
#include "EmcPAL.h"
#include "fbe/fbe_types.h"
#elif defined(FBE_PLATFORM_USE_CSX)
#include "EmcPAL.h"
#include "fbe/fbe_types.h"
#else
#error "WTF"
#endif

/*****************************/

/*Semaphore*/

typedef struct fbe_semaphore_s {
#if defined(FBE_PLATFORM_USE_WIN32)
     void * Semaphore;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KSEMAPHORE Semaphore;
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_SEMAPHORE Semaphore;
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_sem_t Semaphore;
#else
#error "WTF"
#endif
} fbe_semaphore_t;

/*Mutex*/

typedef struct fbe_mutex_s {
#if defined(FBE_PLATFORM_USE_WIN32)
    void * Mutex;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KSEMAPHORE Mutex;
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_MUTEX Mutex;
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mut_t Mutex;
#else
#error "WTF"
#endif
} fbe_mutex_t;

/*Event*/

typedef struct fbe_rendezvous_event_s {
#if defined(FBE_PLATFORM_USE_WIN32)
    void * Event;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KEVENT Event;
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_RENDEZVOUS_EVENT Event;
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mre_t Event;
#else
#error "WTF"
#endif
} fbe_rendezvous_event_t;

/* Spinlock */

typedef struct fbe_spinlock_s {
#if defined(FBE_PLATFORM_USE_WIN32)
    fbe_semaphore_t Spinlock;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EMCPAL_KIRQL OldIrql;
    KSPIN_LOCK Spinlock;
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_SPINLOCK Spinlock;
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_spl_t Spinlock;
#else
#error "WTF"
#endif
} fbe_spinlock_t;

/*Thread*/

typedef void *fbe_thread_user_context_t;
typedef void (
    *fbe_thread_user_root_t) (
    fbe_thread_user_context_t context);

typedef struct fbe_thread_s {
#if defined(FBE_PLATFORM_USE_WIN32)
    void * ThreadHandle;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    HANDLE ThreadHandle;
    PKTHREAD ThreadObject;
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_THREAD ThreadObject;
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_thr_handle_t ThreadHandle;
#else
#error "WTF"
#endif
    void *thread;                          //PZPZ must be non-NULL for existing thread
} fbe_thread_t;

/*
 * The algorithm determines which core we want to affinitize the threads. 
 * This can be enhanced in future to fit the needs.
 */
typedef enum fbe_thread_affinity_algorithm_e {
    HIGHEST_CORE,
    LOWEST_CORE,
    ROUND_ROBIN
} fbe_thread_affinity_algorithm_t;

/*****************************/

void fbe_semaphore_init_named(
    fbe_semaphore_t * sem,
    const char * name,
    fbe_s32_t Count,
    fbe_s32_t Limit);                           /* SAFEBUG - garbage argument */
#define fbe_semaphore_init(_sem, _count, _limit) fbe_semaphore_init_named(_sem, CSX_STRINGIFY(_sem) CSX_STRINGIFY(CSX_MY_LINE), _count, _limit)

void fbe_semaphore_release(
    fbe_semaphore_t * sem,
    fbe_s32_t Increment,                        /* SAFEBUG - garbage argument */
    fbe_s32_t Adjustment,
    fbe_bool_t Wait);                         /* SAFEBUG - garbage argument */

// infinite wait == NULL TimeoutNt
EMCPAL_STATUS fbe_semaphore_wait(
    fbe_semaphore_t * sem,
    EMCPAL_PLARGE_INTEGER TimeoutNt);             /* SAFEBUG - garbage argument type */

// infinite wait == 0 timeout_msecs
fbe_status_t fbe_semaphore_wait_ms(
    fbe_semaphore_t * sem,
    fbe_u32_t timeout_msecs);

void fbe_semaphore_destroy(
    fbe_semaphore_t * sem);

/*****************************/

void fbe_mutex_init_named(
    fbe_mutex_t * mutex,
    const char *name);
#define fbe_mutex_init(_mutex) fbe_mutex_init_named(_mutex, CSX_STRINGIFY(_mutex) CSX_STRINGIFY(CSX_MY_LINE))

void fbe_mutex_lock(
    fbe_mutex_t * mutex);

void fbe_mutex_unlock(
    fbe_mutex_t * mutex);

void fbe_mutex_destroy(
    fbe_mutex_t * mutex);

/*****************************/

void fbe_rendezvous_event_init_named(    
    fbe_rendezvous_event_t * event,
    const char *name);
#define fbe_rendezvous_event_init(_event) fbe_rendezvous_event_init_named(_event, CSX_STRINGIFY(_event) CSX_STRINGIFY(CSX_MY_LINE))

// infinite wait == 0 timeout_msecs
EMCPAL_STATUS fbe_rendezvous_event_wait(
    fbe_rendezvous_event_t * event,
    fbe_u32_t timeout_msecs);

void fbe_rendezvous_event_set(
    fbe_rendezvous_event_t * event);

void fbe_rendezvous_event_clear(
    fbe_rendezvous_event_t * event);

void fbe_rendezvous_event_destroy(
    fbe_rendezvous_event_t * event);

PEMCPAL_RENDEZVOUS_EVENT fbe_rendezvous_event_get_internal_event(
    fbe_rendezvous_event_t * rendezvous_event);

/*****************************/

void fbe_spinlock_init_named(
    fbe_spinlock_t * lock,
    const char *name);

#define fbe_spinlock_init(_lock) fbe_spinlock_init_named(_lock, CSX_STRINGIFY(_lock) CSX_STRINGIFY(CSX_MY_LINE))

void fbe_spinlock_destroy(
    fbe_spinlock_t * lock);

__forceinline static void fbe_spinlock_lock(fbe_spinlock_t * lock)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    fbe_semaphore_wait_ms(&lock->Spinlock, 60000);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeAcquireSpinLock(&lock->Spinlock, &lock->OldIrql);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalSpinlockLock(&lock->Spinlock);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_spl_lock_nid(&lock->Spinlock);
#else
#error "WTF"
#endif
}

__forceinline static void fbe_spinlock_unlock(fbe_spinlock_t * lock)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    fbe_semaphore_release(&lock->Spinlock, 0, 1, FALSE);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeReleaseSpinLock(&lock->Spinlock, lock->OldIrql);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalSpinlockUnlock(&lock->Spinlock);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_spl_unlock_nid(&lock->Spinlock);
#else
#error "WTF"
#endif
}

/*****************************/

EMCPAL_STATUS fbe_thread_init(
    fbe_thread_t * thread,
    const char *name,
    fbe_thread_user_root_t StartRoutine,
    fbe_thread_user_context_t StartContext);


void fbe_set_thread_to_data_path_priority(fbe_thread_t * thread);

EMCPAL_STATUS fbe_thread_wait(
    fbe_thread_t * thread);

fbe_status_t fbe_thread_get_affinity(
    fbe_u64_t * affinity);

void fbe_thread_set_affinity(
    fbe_thread_t * thread,
    fbe_u64_t affinity);

void fbe_thread_delay(
    fbe_u32_t timeout_msecs);

void fbe_thread_exit(
    EMCPAL_STATUS ExitStatus);                  /* SAFEBUG - garbage argument */

void fbe_thread_destroy(
    fbe_thread_t * thread);

void fbe_thread_set_affinity_mask_based_on_algorithm(
    fbe_thread_t * thread,
    fbe_thread_affinity_algorithm_t affinity_algorithm);

fbe_u32_t fbe_get_cpu_count(
    void);

fbe_cpu_id_t fbe_get_cpu_id(
    void);

void fbe_get_number_of_cpu_cores(
    fbe_u32_t * n_cores);

/*****************************/

fbe_s32_t fbe_compare_string(
    fbe_u8_t * src1,
    fbe_u32_t s1_length,
    fbe_u8_t * src2,
    fbe_u32_t s2_length,
    fbe_bool_t case_sensitive);

void fbe_sprintf(
    char *pszDest,
    size_t cbDest,
    const char *pszFormat,
    ...) __attribute__((format(__printf_func__,3,4)));

/* opaque_mem_obj can be NULL unless need it for fbe_get_iomem_physical_address */
void *fbe_allocate_io_memory(
    fbe_u32_t number_of_bytes,
    void **opaque_mem_obj); 

void fbe_release_io_memory(
    void *memory_ptr);

/* opaque_mem_obj can be NULL, but should come from fbe_allocate_io_memory */
fbe_u64_t fbe_get_iomem_physical_address(
    void *virtual_address,
    void *opaque_mem_obj);

void *fbe_allocate_contiguous_memory(
    fbe_u32_t number_of_bytes);

void fbe_release_contiguous_memory(
    void *memory_ptr);

fbe_u64_t fbe_get_contigmem_physical_address(
    void *virtual_address);

void *fbe_allocate_nonpaged_pool_with_tag(
    fbe_u32_t number_of_bytes,
    fbe_u32_t tag);

void fbe_release_nonpaged_pool_with_tag(
    void *memory_ptr,
    fbe_u32_t tag);

void *fbe_allocate_io_nonpaged_pool_with_tag(
    fbe_u32_t number_of_bytes,
    fbe_u32_t tag);

void fbe_release_io_nonpaged_pool_with_tag(
    void *memory_ptr,
    fbe_u32_t tag);


void fbe_release_nonpaged_pool(
    void *memory_ptr);

/*****************************/

typedef int fbe_irql_t;
fbe_irql_t fbe_set_irql_to_dpc(
    void);
void fbe_restore_irql(
    fbe_irql_t irql);

/*****************************/

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
__forceinline static void
fbe_DbgBreakPoint(
    void)
{
    CSX_BREAK();
    return;
}
#endif

__forceinline static void
fbe_debug_break(
    void)
{
#if defined(UMODE_ENV)
    CSX_BREAK();                           //  EmcpalDebugBreakPoint();
#elif defined(SIMMODE_ENV)
    CSX_BREAK();
#elif defined(ALAMOSA_WINDOWS_ENV)
    DbgBreakPoint();                       //  EmcpalDebugBreakPoint();
#else
    CSX_BREAK();
#endif
}

/*****************************/
typedef fbe_semaphore_t bgsl_semaphore_t;
typedef fbe_mutex_t bgsl_mutex_t;
typedef fbe_spinlock_t bgsl_spinlock_t;
typedef fbe_thread_t bgsl_thread_t;

void bgsl_semaphore_init(
    bgsl_semaphore_t * sem,
    fbe_s32_t Count,
    fbe_s32_t Limit);
void bgsl_semaphore_release(
    bgsl_semaphore_t * sem,
    fbe_u32_t Increment,
    fbe_s32_t Adjustment,
    fbe_bool_t Wait);
EMCPAL_STATUS bgsl_semaphore_wait(
    bgsl_semaphore_t * sem,
    EMCPAL_PLARGE_INTEGER Timeout);
void bgsl_semaphore_destroy(
    bgsl_semaphore_t * sem);

void bgsl_mutex_init(
    bgsl_mutex_t * mutex);
void bgsl_mutex_lock(
    bgsl_mutex_t * mutex);
void bgsl_mutex_unlock(
    bgsl_mutex_t * mutex);
void bgsl_mutex_destroy(
    bgsl_mutex_t * mutex);

void bgsl_spinlock_init(
    bgsl_spinlock_t * lock);
void bgsl_spinlock_lock(
    bgsl_spinlock_t * lock);
void bgsl_spinlock_unlock(
    bgsl_spinlock_t * lock);
void bgsl_spinlock_destroy(
    bgsl_spinlock_t * lock);

EMCPAL_STATUS bgsl_thread_init(
    bgsl_thread_t * thread,
    const char *name,
    fbe_thread_user_root_t StartRoutine,
    fbe_thread_user_context_t StartContext);
EMCPAL_STATUS bgsl_thread_wait(
    bgsl_thread_t * thread);
void bgsl_thread_delay(
    fbe_u32_t timeout_in_msec);
void bgsl_thread_destroy(
    bgsl_thread_t * thread);
void bgsl_thread_exit(
    IN EMCPAL_STATUS ExitStatus);

/*****************************/
#endif                                     /*  __FBE_PLATFORM_H */
