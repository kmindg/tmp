
/*****************************/

#ifdef UMODE_ENV
#define I_AM_NATIVE_CODE
#endif

#ifdef SIMMODE_ENV
#define I_AM_NATIVE_CODE
#endif

#include "fbe/fbe_platform.h"

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_MemoryAlloc.h"
#include "EmcPAL_Misc.h"
#include <string.h>

#ifdef FBE_PLATFORM_USE_WINDDK
#include "fbe/fbe_ktrace_interface.h"
#endif
#ifdef FBE_PLATFORM_USE_WIN32
#include <process.h>
#include <stdio.h>
#endif

/*****************************/

static void
fbe_platform_complain(
    const char *format,
    ...)
{
    va_list ap;
    va_start(ap, format);
#if defined(FBE_PLATFORM_USE_WIN32)
    vprintf(format, ap);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    csx_p_vprint_native(format, ap);
#elif defined(FBE_PLATFORM_USE_PAL)
    csx_p_vprint_native(format, ap);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_vprint_native(format, ap);
#else
#error "WTF"
#endif
    va_end(ap);
}

static __inline void
fbe_platform_break(
    void)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    DebugBreak();
#elif defined(FBE_PLATFORM_USE_WINDDK)
    DbgBreakPoint();
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalDebugBreakPoint();
#elif defined(FBE_PLATFORM_USE_CSX)
    CSX_BREAK();
#else
#error "WTF"
#endif
}

/*****************************/

#if defined(FBE_PLATFORM_USE_WIN32)
#define FBE_PLATFORM_ASSERT(_e) \
     do { if (!(_e)) { fbe_platform_complain("Error: %s failed at %s:%d\n", #_e, __FUNCTION__, __LINE__); fbe_platform_break(); } } while (0)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#define FBE_PLATFORM_ASSERT(_e) \
     do { if (!(_e)) { fbe_platform_complain("Error: %s failed at %s:%d\n", #_e, __FUNCTION__, __LINE__); fbe_platform_break(); } } while (0)
#elif defined(FBE_PLATFORM_USE_PAL)
#define FBE_PLATFORM_ASSERT(_e) \
     do { if (!(_e)) { fbe_platform_complain("Error: %s failed at %s:%d\n", #_e, __FUNCTION__, __LINE__); fbe_platform_break(); } } while (0)
#elif defined(FBE_PLATFORM_USE_CSX)
#define FBE_PLATFORM_ASSERT(_e) CSX_ASSERT_H_RDC(_e)
#else
#error "WTF"
#endif

/*****************************/

#ifdef FBE_PLATFORM_USE_WIN32

static EMCPAL_STATUS
fbe_win32_object_wait_timedout(
    HANDLE Handle,
    fbe_u32_t TimeoutMsecs)
{
    DWORD timeout_msecs = TimeoutMsecs;
    DWORD result;
    DWORD err;
    result = WaitForSingleObject(Handle, timeout_msecs);
    if (result != WAIT_OBJECT_0 && result != WAIT_TIMEOUT) {
        LPVOID      lpMsgBuf;  
        err = GetLastError(); 
        fbe_platform_complain("fbe_winddk: %s Critical error %d (it should never happen)\n", __FUNCTION__, err);
        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL );

        fbe_platform_complain("fbe_winddk: %s Critical error %s\n", __FUNCTION__, lpMsgBuf);
        /* FormatMessage() returns a buffer allocated via LocalAlloc(), thus we need to use LocalFree(). 
         */
        LocalFree(lpMsgBuf);
    }
    FBE_PLATFORM_ASSERT(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT);
    return (result == WAIT_OBJECT_0) ? EMCPAL_STATUS_SUCCESS : EMCPAL_STATUS_TIMEOUT;
}

static void
fbe_win32_object_wait_forever(
    HANDLE Handle)
{
    DWORD result;
    result = WaitForSingleObject(Handle, INFINITE);
    FBE_PLATFORM_ASSERT(result == WAIT_OBJECT_0);
}

#endif

/*****************************/

void
fbe_semaphore_init_named(
    fbe_semaphore_t * sem,
    const char *name,
    fbe_s32_t Count,
    fbe_s32_t Limit)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(name);
    sem->Semaphore = CreateSemaphore(NULL, Count, Limit, NULL);
    FBE_PLATFORM_ASSERT(sem->Semaphore != NULL);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(name);
    KeInitializeSemaphore(&sem->Semaphore, Count, Limit);
#elif defined(FBE_PLATFORM_USE_PAL)
    FBE_UNREFERENCED_PARAMETER(Limit);     /* SAFEBUG */
    EmcpalSemaphoreCreate(EmcpalDriverGetCurrentClientObject(), &sem->Semaphore, name, Count);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_sem_create_always(CSX_MY_MODULE_CONTEXT(), &sem->Semaphore, name, (csx_nuint_t) Count, (csx_nuint_t) Limit);
#else
#error "WTF"
#endif
}

void
fbe_semaphore_release(
    fbe_semaphore_t * sem,
    fbe_s32_t Increment,
    fbe_s32_t Adjustment,
    fbe_bool_t Wait)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    FBE_UNREFERENCED_PARAMETER(Increment); /* SAFEBUG */
    FBE_UNREFERENCED_PARAMETER(Wait);      /* SAFEBUG */
    worked = ReleaseSemaphore(sem->Semaphore, Adjustment, NULL);
    FBE_PLATFORM_ASSERT(worked);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeReleaseSemaphore(&sem->Semaphore, Increment, Adjustment, Wait);
#elif defined(FBE_PLATFORM_USE_PAL)
    FBE_UNREFERENCED_PARAMETER(Increment); /* SAFEBUG */
    FBE_UNREFERENCED_PARAMETER(Wait);      /* SAFEBUG */
    EmcpalSemaphoreSignal(&sem->Semaphore, Adjustment);
#elif defined(FBE_PLATFORM_USE_CSX)
    FBE_UNREFERENCED_PARAMETER(Increment); /* SAFEBUG */
    FBE_UNREFERENCED_PARAMETER(Wait);      /* SAFEBUG */
    csx_p_sem_postn(&sem->Semaphore, (csx_nuint_t) Adjustment);
#else
#error "WTF"
#endif
}

EMCPAL_STATUS
fbe_semaphore_wait(
    fbe_semaphore_t * sem,
    EMCPAL_PLARGE_INTEGER TimeoutNt)
{
    EMCPAL_STATUS status;
#if defined(FBE_PLATFORM_USE_WIN32)
    if (TimeoutNt == NULL) {
        fbe_win32_object_wait_forever(sem->Semaphore);
        status = EMCPAL_STATUS_SUCCESS;
    } else {
        FBE_PLATFORM_ASSERT(TimeoutNt->QuadPart < 0);
        status = fbe_win32_object_wait_timedout(sem->Semaphore, (fbe_u32_t)EMCPAL_CONVERT_100NSECS_TO_MSECS(-TimeoutNt->QuadPart));
    }
#elif defined(FBE_PLATFORM_USE_WINDDK)
    if (TimeoutNt != NULL) {
        FBE_PLATFORM_ASSERT(TimeoutNt->QuadPart < 0);
    }
    status = KeWaitForSingleObject(&sem->Semaphore, Executive, KernelMode, FALSE, TimeoutNt);
#elif defined(FBE_PLATFORM_USE_PAL)
    if (TimeoutNt == NULL) {
        status = EmcpalSemaphoreWait(&sem->Semaphore, EMCPAL_TIMEOUT_INFINITE_WAIT);
    } else {
        FBE_PLATFORM_ASSERT(TimeoutNt->QuadPart < 0);
        status = EmcpalSemaphoreWait(&sem->Semaphore, EMCPAL_CONVERT_100NSECS_TO_MSECS(-TimeoutNt->QuadPart));
    }
#elif defined(FBE_PLATFORM_USE_CSX)
    if (TimeoutNt == NULL) {
        csx_p_sem_wait(&sem->Semaphore);
        status = EMCPAL_STATUS_SUCCESS;
    } else {
        FBE_PLATFORM_ASSERT(TimeoutNt->QuadPart < 0);
        status = csx_p_sem_timedwait(&sem->Semaphore, (csx_timeout_msecs_t) EMCPAL_CONVERT_100NSECS_TO_MSECS(-TimeoutNt->QuadPart));
    }
#else
#error "WTF"
#endif
    return status;
}

fbe_status_t
fbe_semaphore_wait_ms(
    fbe_semaphore_t * sem,
    fbe_u32_t timeout_msecs)
{
    EMCPAL_STATUS status;
#if defined(FBE_PLATFORM_USE_WIN32)
    if (timeout_msecs == 0) {
        fbe_win32_object_wait_forever(sem->Semaphore);
        status = EMCPAL_STATUS_SUCCESS;
    } else {
        status = fbe_win32_object_wait_timedout(sem->Semaphore, timeout_msecs);
    }
#elif defined(FBE_PLATFORM_USE_WINDDK)
    if (timeout_msecs == 0) {
        status = KeWaitForSingleObject(&sem->Semaphore, Executive, KernelMode, FALSE, NULL);
    } else {
        EMCPAL_LARGE_INTEGER TimeoutNt;
        TimeoutNt.QuadPart = (fbe_s64_t) (-10000LL * (fbe_s64_t) timeout_msecs);
        status = KeWaitForSingleObject(&sem->Semaphore, Executive, KernelMode, FALSE, &TimeoutNt);
    }
#elif defined(FBE_PLATFORM_USE_PAL)
    if (timeout_msecs == 0) {
        status = EmcpalSemaphoreWait(&sem->Semaphore, EMCPAL_TIMEOUT_INFINITE_WAIT);
    } else {
		/* CSX!!! see csx_ext_consts.h */
		if(timeout_msecs > CSX_TIMEOUT_MSECS_MAX){
			timeout_msecs = CSX_TIMEOUT_MSECS_MAX;
		}
        status = EmcpalSemaphoreWait(&sem->Semaphore, timeout_msecs);
    }
#elif defined(FBE_PLATFORM_USE_CSX)
    if (timeout_msecs == 0) {
        csx_p_sem_wait(&sem->Semaphore);
        status = EMCPAL_STATUS_SUCCESS;
    } else {
        status = csx_p_sem_timedwait(&sem->Semaphore, (csx_timeout_msecs_t) timeout_msecs);
    }
#else
#error "WTF"
#endif
    switch (status) {
    case EMCPAL_STATUS_SUCCESS:
        return FBE_STATUS_OK;
    case EMCPAL_STATUS_TIMEOUT:
        return FBE_STATUS_TIMEOUT;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

void
fbe_semaphore_destroy(
    fbe_semaphore_t * sem)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = CloseHandle(sem->Semaphore);
    FBE_PLATFORM_ASSERT(worked);
    sem->Semaphore = (HANDLE)0xdeadbeef;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(sem);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalSemaphoreDestroy(&sem->Semaphore);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_sem_destroy(&sem->Semaphore);
#else
#error "WTF"
#endif
}

/*****************************/

void
fbe_mutex_init_named(
    fbe_mutex_t * mutex,
    const char *name)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(name);
    mutex->Mutex = CreateSemaphore(NULL, 1, 1, NULL);
    FBE_PLATFORM_ASSERT(mutex->Mutex != NULL);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(name);
    KeInitializeSemaphore(&mutex->Mutex, 1, 1);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalMutexCreate(EmcpalDriverGetCurrentClientObject(), &mutex->Mutex, name);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mut_create_always(CSX_MY_MODULE_CONTEXT(), &mutex->Mutex, name);
#else
#error "WTF"
#endif
}

void
fbe_mutex_lock(
    fbe_mutex_t * mutex)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    fbe_win32_object_wait_forever(mutex->Mutex);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeWaitForSingleObject(&mutex->Mutex, Executive, KernelMode, FALSE, NULL);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalMutexLock(&mutex->Mutex);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mut_lock(&mutex->Mutex);
#else
#error "WTF"
#endif
}

void
fbe_mutex_unlock(
    fbe_mutex_t * mutex)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = ReleaseSemaphore(mutex->Mutex, 1, NULL);
    FBE_PLATFORM_ASSERT(worked);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeReleaseSemaphore(&mutex->Mutex, 0, 1, FALSE);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalMutexUnlock(&mutex->Mutex);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mut_unlock(&mutex->Mutex);
#else
#error "WTF"
#endif
}

void
fbe_mutex_destroy(
    fbe_mutex_t * mutex)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = CloseHandle(mutex->Mutex);
    FBE_PLATFORM_ASSERT(worked);
    mutex->Mutex = (HANDLE)0xdeadbeef;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(mutex);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalMutexDestroy(&mutex->Mutex);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mut_destroy(&mutex->Mutex);
#else
#error "WTF"
#endif
}

/*****************************/

void
fbe_rendezvous_event_init_named(
    fbe_rendezvous_event_t * event,
    const char *name)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(name);
    event->Event = CreateEvent(NULL, TRUE, FALSE, "fbe_event");
    FBE_PLATFORM_ASSERT(event->Event != NULL);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(name);
    KeInitializeEvent(&event->Event, NotificationEvent, FALSE);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalRendezvousEventCreate(EmcpalDriverGetCurrentClientObject(), &event->Event, name, FALSE);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mre_create_always(CSX_MY_MODULE_CONTEXT(), &event->Event, name);
#else
#error "WTF"
#endif
}

EMCPAL_STATUS
fbe_rendezvous_event_wait(
    fbe_rendezvous_event_t * event,
    fbe_u32_t timeout_msecs)
{
    EMCPAL_STATUS status;
#if defined(FBE_PLATFORM_USE_WIN32)
    if (timeout_msecs == 0) {
        fbe_win32_object_wait_forever(event->Event);
        status = EMCPAL_STATUS_SUCCESS;
    } else {
        status = fbe_win32_object_wait_timedout(event->Event, timeout_msecs);
    }
#elif defined(FBE_PLATFORM_USE_WINDDK)
    if (timeout_msecs == 0) {
        status = KeWaitForSingleObject(&event->Event, Executive, KernelMode, FALSE, NULL);
    } else {
        EMCPAL_LARGE_INTEGER TimeoutNt;
        TimeoutNt.QuadPart = (fbe_s64_t) (-10000LL * (fbe_s64_t) timeout_msecs);
        status = KeWaitForSingleObject(&event->Event, Executive, KernelMode, FALSE, &TimeoutNt);
    }
#elif defined(FBE_PLATFORM_USE_PAL)
    if (timeout_msecs == 0) {
        status = EmcpalRendezvousEventWait(&event->Event, EMCPAL_TIMEOUT_INFINITE_WAIT);
    } else {
        status = EmcpalRendezvousEventWait(&event->Event, timeout_msecs);
    }
#elif defined(FBE_PLATFORM_USE_CSX)
    if (timeout_msecs == 0) {
        csx_p_mre_wait(&event->Event);
        status = EMCPAL_STATUS_SUCCESS;
    } else {
        status = csx_p_mre_timedwait(&event->Event, timeout_msecs);
    }
#else
#error "WTF"
#endif
    return status;
}

void
fbe_rendezvous_event_set(
    fbe_rendezvous_event_t * event)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = SetEvent(event->Event);
    FBE_PLATFORM_ASSERT(worked);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeSetEvent(&event->Event, 0, FALSE);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalRendezvousEventSet(&event->Event);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mre_signal(&event->Event);
#else
#error "WTF"
#endif
}

void
fbe_rendezvous_event_clear(
    fbe_rendezvous_event_t * event)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = ResetEvent(event->Event);
    FBE_PLATFORM_ASSERT(worked);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    KeClearEvent(&event->Event);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalRendezvousEventClear(&event->Event);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_mre_clear(&event->Event);
#else
#error "WTF"
#endif
}

void
fbe_rendezvous_event_destroy(
    fbe_rendezvous_event_t * event)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = CloseHandle(event->Event);
    FBE_PLATFORM_ASSERT(worked);
    event->Event = (HANDLE)0xdeadbeef;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(event);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalRendezvousEventDestroy(&event->Event);
#elif defined(FBE_PLATFORM_USE_CSX)
#else
    csx_p_mre_destroy(&event->Event);
#error "WTF"
#endif
}

PEMCPAL_RENDEZVOUS_EVENT
fbe_rendezvous_event_get_internal_event(
    fbe_rendezvous_event_t * rendezvous_event)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    CSX_PANIC(); //PZPZ
    return NULL;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    return &rendezvous_event->Event;
#elif defined(FBE_PLATFORM_USE_PAL)
    return &rendezvous_event->Event;
#elif defined(FBE_PLATFORM_USE_CSX)
    CSX_PANIC(); //PZPZ
    return NULL;
#else
#error "WTF"
#endif
}

/*****************************/

void
fbe_spinlock_init_named(
    fbe_spinlock_t * lock,
    const char *name)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(name);
    lock->Spinlock.Semaphore = CreateSemaphore(NULL, 1, 1, NULL);
    FBE_PLATFORM_ASSERT(lock->Spinlock.Semaphore != NULL);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(name);
    KeInitializeSpinLock(&lock->Spinlock);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalSpinlockCreate(EmcpalDriverGetCurrentClientObject(), &lock->Spinlock, name);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &lock->Spinlock, name);
#else
#error "WTF"
#endif
}



void
fbe_spinlock_destroy(
    fbe_spinlock_t * lock)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = CloseHandle(lock->Spinlock.Semaphore);
    FBE_PLATFORM_ASSERT(worked);
    lock->Spinlock.Semaphore = (HANDLE)0xdeadbeef;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    FBE_UNREFERENCED_PARAMETER(lock);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalSpinlockDestroy(&lock->Spinlock);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_spl_destroy_nid(&lock->Spinlock);
#else
#error "WTF"
#endif
}

/*****************************/

typedef struct {
#if defined(FBE_PLATFORM_USE_WIN32)
    void * StartSem1;
    void * StartSem2;
#endif
    fbe_thread_user_root_t StartRoutine;
    fbe_thread_user_context_t StartContext;
} fbe_thread_root_context_t;

#if defined(FBE_PLATFORM_USE_WIN32)
static DWORD WINAPI
fbe_thread_starter(
    LPVOID lpParameter)
{
    /*CSX_P_THR_CALL_CONTEXT;*/ /*sharel - commented out per Pete, so fbecli can work in userspace */
    fbe_thread_root_context_t *context = (fbe_thread_root_context_t *) lpParameter;
    fbe_thread_user_root_t StartRoutine = context->StartRoutine;
    fbe_thread_user_context_t StartContext = context->StartContext;
    {
        BOOL worked;
        worked = ReleaseSemaphore(context->StartSem1, 1, NULL); 
        FBE_PLATFORM_ASSERT(worked);
    }
    fbe_win32_object_wait_forever(context->StartSem2);
    CloseHandle(context->StartSem1);
    CloseHandle(context->StartSem2);
    free(context);
    /*CSX_P_THR_CALL_ENTER();*//*sharel - commented out per Pete, so fbecli can work in userspace */
    StartRoutine(StartContext);
    fbe_thread_exit(0);
    return 0;
}
#elif defined(FBE_PLATFORM_USE_WINDDK)
static VOID NTAPI
fbe_thread_starter(
    PVOID lpParameter)
{
#ifdef ALAMOSA_WINDOWS_ENV
    CSX_P_THR_CALL_CONTEXT;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - thread context issues */
    fbe_thread_root_context_t *context = (fbe_thread_root_context_t *) lpParameter;
    fbe_thread_user_root_t StartRoutine = context->StartRoutine;
    fbe_thread_user_context_t StartContext = context->StartContext;
    EmcpalFreeUtilityPool(context);
#ifdef ALAMOSA_WINDOWS_ENV
    CSX_P_THR_CALL_ENTER();
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - thread context issues */
    StartRoutine(StartContext);
    fbe_thread_exit(0);
}
#elif defined(FBE_PLATFORM_USE_PAL)
static VOID WINAPI
fbe_thread_starter(
    PVOID lpParameter)
{
    fbe_thread_root_context_t *context = (fbe_thread_root_context_t *) lpParameter;
    fbe_thread_user_root_t StartRoutine = context->StartRoutine;
    fbe_thread_user_context_t StartContext = context->StartContext;
    EmcpalFreeUtilityPool(context);
    StartRoutine(StartContext);
    fbe_thread_exit(0);
}
#elif defined(FBE_PLATFORM_USE_CSX)
static csx_void_t
fbe_thread_starter(
    csx_pvoid_t lpParameter)
{
    fbe_thread_root_context_t *context = (fbe_thread_root_context_t *) lpParameter;
    fbe_thread_user_root_t StartRoutine = context->StartRoutine;
    fbe_thread_user_context_t StartContext = context->StartContext;
    csx_p_util_mem_free(context);
    csx_p_thr_started(CSX_NULL);
    {
        StartRoutine(StartContext);
    }
}
#else
#error "WTF"
#endif

/*****************************/

EMCPAL_STATUS
fbe_thread_init(
    fbe_thread_t * thread,
    const char *name,
    fbe_thread_user_root_t StartRoutine,
    fbe_thread_user_context_t StartContext)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    fbe_thread_root_context_t *context;

    FBE_UNREFERENCED_PARAMETER(name);

    thread->thread = NULL; //PZPZ
    context = (fbe_thread_root_context_t *) malloc(sizeof(fbe_thread_root_context_t));
    if (context == NULL) {
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }
    context->StartRoutine = StartRoutine;
    context->StartContext = StartContext;
    context->StartSem1 = CreateSemaphore(NULL, 0, 1, NULL);
    FBE_PLATFORM_ASSERT(context->StartSem1 != NULL);
    context->StartSem2 = CreateSemaphore(NULL, 0, 1, NULL);
    FBE_PLATFORM_ASSERT(context->StartSem2 != NULL);

#if 0 //PZPZ cleanup
    thread->ThreadHandle = CreateThread(NULL, 0, fbe_thread_starter, context, 0, NULL);
#else
    thread->ThreadHandle = (HANDLE) _beginthreadex(NULL, 0, fbe_thread_starter, context, 0, NULL);
#endif
    if (thread->ThreadHandle == NULL || thread->ThreadHandle == INVALID_HANDLE_VALUE) {
        fbe_platform_complain("fbe_winddk: unable to start thread: %d\n", GetLastError());
        fbe_platform_break();
        free(context);
        return EMCPAL_STATUS_UNSUCCESSFUL;
    }
    thread->thread = thread; //PZPZ
    fbe_win32_object_wait_forever(context->StartSem1);
    {
        BOOL worked;
        worked = ReleaseSemaphore(context->StartSem2, 1, NULL); 
        FBE_PLATFORM_ASSERT(worked);
    }
    return EMCPAL_STATUS_SUCCESS;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EMCPAL_STATUS status;
    fbe_thread_root_context_t *context;

    FBE_UNREFERENCED_PARAMETER(name);

    thread->thread = NULL; //PZPZ

    context = (fbe_thread_root_context_t *) EmcpalAllocateUtilityPool(EmcpalNonPagedPool, sizeof(fbe_thread_root_context_t));
    if (context == NULL) {
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }
    context->StartRoutine = StartRoutine;
    context->StartContext = StartContext;
    status = PsCreateSystemThread(&thread->ThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, fbe_thread_starter, context);
    if (status != EMCPAL_STATUS_SUCCESS) {
#if 0
        fbe_platform_complain("fbe_winddk: unable to start thread: %x\n", status);
        fbe_platform_break();
#endif
        EmcpalFreeUtilityPool(context);
        return status;
    }
    status = ObReferenceObjectByHandle(thread->ThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *) & thread->ThreadObject, NULL);
    FBE_PLATFORM_ASSERT(status == EMCPAL_STATUS_SUCCESS);
    thread->thread = thread; //PZPZ
    return EMCPAL_STATUS_SUCCESS;
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_STATUS status;
    fbe_thread_root_context_t *context;
    thread->thread = NULL; //PZPZ
    context = (fbe_thread_root_context_t *) EmcpalAllocateUtilityPool(EmcpalNonPagedPool, sizeof(fbe_thread_root_context_t));
    if (context == NULL) {
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }
    context->StartRoutine = StartRoutine;
    context->StartContext = StartContext;
    status = EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &thread->ThreadObject, name, fbe_thread_starter, context);
    if (status != EMCPAL_STATUS_SUCCESS) {
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
        fbe_platform_complain("fbe_winddk: unable to start thread: %x\n", status);
        fbe_platform_break();
#endif
        EmcpalFreeUtilityPool(context);
        return status;
    }
    thread->thread = thread; //PZPZ

    /*
     * Adjust Thread Priority
     */
    fbe_set_thread_to_data_path_priority(thread);

    return EMCPAL_STATUS_SUCCESS;

#elif defined(FBE_PLATFORM_USE_CSX)
    csx_status_e status;
    fbe_thread_root_context_t *context;
    thread->thread = NULL; //PZPZ
    context = (fbe_thread_root_context_t *) csx_p_util_mem_alloc_maybe(sizeof(fbe_thread_root_context_t));
    if (context == NULL) {
        return EMCPAL_STATUS_INSUFFICIENT_RESOURCES;
    }
    context->StartRoutine = StartRoutine;
    context->StartContext = StartContext;
    status =
        csx_p_thr_create_maybe(CSX_MY_MODULE_CONTEXT(), &thread->ThreadHandle, CSX_P_THR_TYPE_WAITABLE, name,
        CSX_P_THR_STACK_SIZE_DEFAULT, fbe_thread_starter, CSX_NULL, context);
    if (status != EMCPAL_STATUS_SUCCESS) {
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
        fbe_platform_complain("fbe_winddk: unable to start thread: %x\n", status);
        fbe_platform_break();
#endif
        csx_p_util_mem_free(context);
        return status;
    }
    thread->thread = thread; //PZPZ
    return EMCPAL_STATUS_SUCCESS;
#else
#error "WTF"
#endif
}

void fbe_set_thread_to_data_path_priority(fbe_thread_t * thread)
{

#if defined(FBE_PLATFORM_USE_WIN32) || defined(UMODE_ENV) || defined(SIMMODE_ENV)

    /* do nothing */

#elif defined(FBE_PLATFORM_USE_WINDDK) 
 
    KeSetBasePriorityThread(&thread->ThreadHandle, (LOW_PRIORITY+15));

#elif defined(FBE_PLATFORM_USE_PAL)

    EmcPalSetThreadToDataPathPriorityExternal(&thread->ThreadObject);

#elif defined(FBE_PLATFORM_USE_CSX)

    // No CSX support

#else
#error "WTF"
#endif

}

EMCPAL_STATUS
fbe_thread_wait(
    fbe_thread_t * thread)
{
    EMCPAL_STATUS status;
#if defined(FBE_PLATFORM_USE_WIN32)
    fbe_win32_object_wait_forever(thread->ThreadHandle);
    status = EMCPAL_STATUS_SUCCESS;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    status = KeWaitForSingleObject(thread->ThreadObject, Executive, KernelMode, FALSE, TimeoutNt);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalThreadWait(&thread->ThreadObject);
    status = EMCPAL_STATUS_SUCCESS;
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_thr_wait(&thread->ThreadHandle);
    status = EMCPAL_STATUS_SUCCESS;
#else
#error "WTF"
#endif
    return status;
}

fbe_status_t
fbe_thread_get_affinity(
    fbe_u64_t * affinity)
{
    csx_processor_mask_t thr_affinity = 0;
    csx_processor_mask_t thr_affinity_prev;
    csx_status_e status;
    status = csx_p_thr_affine(CSX_NULL, thr_affinity, &thr_affinity_prev);
    CSX_ASSERT_SUCCESS_H_RDC(status);
    *affinity = thr_affinity_prev;
    return FBE_STATUS_OK;
}

void
fbe_thread_set_affinity(
    fbe_thread_t * thread,
    fbe_u64_t affinity)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    DWORD_PTR affinity_previous;
    DWORD_PTR affinity_mask = (DWORD_PTR)affinity;
    affinity_previous = SetThreadAffinityMask(thread->ThreadHandle,affinity_mask);
    CSX_ASSERT_H_RDC(affinity_previous != 0);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EMCPAL_STATUS           status;
    KAFFINITY               affinity_mask = affinity;
    status = ZwSetInformationThread(thread->ThreadHandle, ThreadAffinityMask, &affinity_mask, sizeof(KAFFINITY));
    CSX_ASSERT_H_RDC(EMCPAL_IS_SUCCESS(status));
#elif defined(FBE_PLATFORM_USE_PAL)
    EMCPAL_PROCESSOR_MASK affinity_mask = affinity;
    EmcpalThreadAffine(&thread->ThreadObject, affinity_mask);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_processor_mask_t thr_affinity = (csx_processor_mask_t)affinity;
    csx_processor_mask_t thr_affinity_prev;
    csx_status_e status;
    status = csx_p_thr_affine(&thread->ThreadHandle, thr_affinity, &thr_affinity_prev);
    CSX_ASSERT_SUCCESS_H_RDC(status);
#else
#error "WTF"
#endif
}

void
fbe_thread_delay(
    fbe_u32_t timeout_msecs)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    Sleep(timeout_msecs);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EMCPAL_LARGE_INTEGER timeout;
    timeout.QuadPart = (fbe_s64_t) (-10000LL * (fbe_s64_t) timeout_msecs);
    KeDelayExecutionThread(KernelMode, TRUE, &timeout);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalThreadSleepOptimistic(timeout_msecs);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_thr_sleep_msecs(timeout_msecs);
#else
#error "WTF"
#endif
}

void
fbe_thread_exit(
    EMCPAL_STATUS ExitStatus)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#if 0 //PZPZ cleanup
    ExitThread(ExitStatus);
#else
    /*csx_p_thr_impersonate_end(CSX_NULL);*//*sharel - commented out per Pete, so fbecli can work in userspace */
    _endthreadex(ExitStatus);
#endif
#elif defined(FBE_PLATFORM_USE_WINDDK)
    csx_p_thr_impersonate_end(CSX_NULL);
    PsTerminateSystemThread(ExitStatus);
#elif defined(FBE_PLATFORM_USE_PAL)
    FBE_UNREFERENCED_PARAMETER(ExitStatus);
    EmcpalThreadExit();
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_thr_exit(CSX_NULL);
#else
#error "WTF"
#endif
}

void
fbe_thread_destroy(
    fbe_thread_t * thread)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    BOOL worked;
    worked = CloseHandle(thread->ThreadHandle);
    FBE_PLATFORM_ASSERT(worked);
    thread->ThreadHandle = (HANDLE)0xdeadbeef;
#elif defined(FBE_PLATFORM_USE_WINDDK)
    ZwClose(thread->ThreadHandle);
    ObDereferenceObject(thread->ThreadObject);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalThreadDestroy(&thread->ThreadObject);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_thr_destroy(&thread->ThreadHandle);
#else
#error "WTF"
#endif
}

void
fbe_thread_set_affinity_mask_based_on_algorithm(
    fbe_thread_t * thread,
    fbe_thread_affinity_algorithm_t affinity_algorithm)
{
   csx_nuint_t processor_count = csx_p_get_processor_count();
   csx_nuint_t processor_to_use = 0;
   static csx_nuint_t processor_last = 0;

    switch (affinity_algorithm)
    {
        case HIGHEST_CORE:
              processor_to_use = processor_count-1;
              break;
        case LOWEST_CORE:
              processor_to_use = 0;
              break;
        case ROUND_ROBIN:
              processor_to_use = (processor_last++) % processor_count;
              break;
        default:
           CSX_PANIC();
    }

#if defined(FBE_PLATFORM_USE_WIN32)
{
    DWORD_PTR affinity_previous;
    DWORD_PTR affinity_mask = 1ULL<<processor_to_use;
    affinity_previous = SetThreadAffinityMask(thread->ThreadHandle,affinity_mask);
    CSX_ASSERT_H_RDC(affinity_previous != 0);
}
#elif defined(FBE_PLATFORM_USE_WINDDK)
{
    EMCPAL_STATUS           status;
    KAFFINITY               affinity_mask = 1ULL<<processor_to_use;
    status = ZwSetInformationThread(thread->ThreadHandle, ThreadAffinityMask, &affinity_mask, sizeof(KAFFINITY));
    CSX_ASSERT_H_RDC(EMCPAL_IS_SUCCESS(status));
}
#elif defined(FBE_PLATFORM_USE_PAL)
{
    EMCPAL_PROCESSOR_MASK affinity_mask = 1ULL<<processor_to_use;
    EmcpalThreadAffine(&thread->ThreadObject, affinity_mask);
}
#elif defined(FBE_PLATFORM_USE_CSX)
{
    csx_status_e status;
    csx_processor_mask_t affinity_mask = 1ULL<<processor_to_use;
    status = csx_p_thr_affine(&thread->ThreadHandle, affinity_mask, CSX_NULL);
    CSX_ASSERT_H_RDC(CSX_SUCCESS(status));
}
#else
#error "WTF"
#endif
}

void
fbe_get_number_of_cpu_cores(
    fbe_u32_t * n_cores)
{
    *n_cores = csx_p_get_processor_count();
}

/*****************************/

fbe_cpu_id_t
fbe_get_cpu_count(
    void)
{
    return csx_p_get_processor_count();
}

fbe_cpu_id_t
fbe_get_cpu_id(
    void)
{
    return csx_p_get_processor_id();
}

/*****************************/

fbe_s32_t
fbe_compare_string(
    fbe_u8_t * src1,
    fbe_u32_t s1_length,
    fbe_u8_t * src2,
    fbe_u32_t s2_length,
    fbe_bool_t case_sensitive)
{
    return (strncmp((const char *) src1, (const char *) src2, s1_length));
}

void
fbe_sprintf(
    char *pszDest,
    size_t cbDest,
    const char *pszFormat,
    ...)
{
    int rc;
    va_list ap;
    va_start(ap, pszFormat);
    rc = csx_p_vsnprintf(pszDest, cbDest, pszFormat, ap);
    va_end(ap);
}


#if 0
fbe_s32_t
fbe_compare_string(
    fbe_u8_t * src1,
    fbe_u32_t s1_length,
    fbe_u8_t * src2,
    fbe_u32_t s2_length,
    fbe_bool_t case_sensitive)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

void
fbe_debug_break(
    )
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

void
fbe_zero_memory(
    void *dst,
    fbe_u32_t length)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

void
fbe_copy_memory(
    void *dst,
    const void *src,
    fbe_u32_t length)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

void
fbe_move_memory(
    void *dst,
    const void *src,
    fbe_u32_t length)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

void
fbe_set_memory(
    void *dst,
    unsigned char fill,
    fbe_u32_t length)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

fbe_bool_t
fbe_equal_memory(
    const void *src1,
    const void *src2,
    fbe_u32_t length)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}
#endif

/*****************************/
#if defined(FBE_PLATFORM_USE_PAL) && !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
typedef struct fbe_platform_mem_obj_s {
    fbe_u64_t              fbe_mem_obj_magic1;
    EMCPAL_IOMEMORY_OBJECT fbe_mem_obj_emcpal_iomem_obj;
    fbe_u64_t              fbe_mem_obj_magic2;
} fbe_platform_mem_obj_t, *fbe_platform_mem_obj_p;

#define FBE_MEM_OBJ_MAGIC1 0xFBE1FBE1FBE1FBE1
#define FBE_MEM_OBJ_MAGIC2 0x2FBE2FBE2FBE2FBE
#endif

/*!****************************************************************************
 * @fn fbe_allocate_io_memory(fbe_u32_t number_of_bytes,
 *                            void      **opaque_mem_obj_p)
 ******************************************************************************
 * @brief
 *    This function is the interface used to allocate memory to which IO can be
 *    performed. Caller should free using fbe_free_io_memory function.
 *
 * @param number_of_bytes - Amount of memory requested to be allocated.
 * @param opaque_mem_obj_p - Opaque pointer returned which can be passed to the
 *                           fbe_get_iomem_physical_address routine; this param
 *                           is optional (i.e., can be NULL).
 *
 * @return void * - pointer to the allocated memory for use by the caller
 *
 * @author:
 *     08-Feb-2013 Mike Wahl        Created.
 *
 ******************************************************************************/
void *
fbe_allocate_io_memory(
    fbe_u32_t number_of_bytes,
    void      **opaque_mem_obj_p)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    return malloc(number_of_bytes);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    PHYSICAL_ADDRESS paddr;
    paddr.QuadPart = 0xffffffffffffffff;
    return MmAllocateContiguousMemory(number_of_bytes, paddr);
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return csx_p_util_mem_alloc_maybe(number_of_bytes);
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return csx_p_util_mem_alloc_maybe(number_of_bytes);
#elif defined(FBE_PLATFORM_USE_PAL) && defined(ALAMOSA_WINDOWS_ENV)
    return EmcpalContiguousMemoryAllocateMaybe(number_of_bytes, CSX_FALSE, CSX_FALSE);
#elif defined(FBE_PLATFORM_USE_PAL)
    return EmcpalIoBufferAllocateMaybe(number_of_bytes);
#elif defined(FBE_PLATFORM_USE_CSX)
    PHYSICAL_ADDRESS paddr;
    paddr.QuadPart = 0xffffffffffffffff;
    return MmAllocateContiguousMemory(number_of_bytes, paddr);
#else
#error "WTF"
#endif
}

/*!****************************************************************************
 * @fn fbe_free_io_memory(void *memory_ptr)
 ******************************************************************************
 * @brief
 *    This function is the interface used to free memory allocated by the
 *    fbe_allocate_io_memory function.
 *
 * @param memory_ptr - Memory pointer returned by fbe_allocate_io_memory
 *
 * @return void
 *
 * @author:
 *     08-Feb-2013 Mike Wahl        Created.
 *
 ******************************************************************************/
void
fbe_release_io_memory(
    void *memory_ptr)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    free(memory_ptr);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    MmFreeContiguousMemory(memory_ptr);
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    csx_p_util_mem_free(memory_ptr);
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    csx_p_util_mem_free(memory_ptr);
#elif defined(FBE_PLATFORM_USE_PAL) && defined(ALAMOSA_WINDOWS_ENV)
    EmcpalContiguousMemoryFree(memory_ptr);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalIoBufferFree(memory_ptr);
#elif defined(FBE_PLATFORM_USE_CSX)
    MmFreeContiguousMemory(memory_ptr);
#else
#error "WTF"
#endif
}

/*!****************************************************************************
 * @fn fbe_get_iomem_physical_address(void *virtual_address,
 *                                    void *opaque_mem_obj_p)
 ******************************************************************************
 * @brief
 *    This function is the interface used to determine the physical address of
 *    memory allocated by the fbe_allocate_io_memory function.
 *
 * @param vitural_address - Address within the range of memory allocated
 * @param opaque_mem_obj_p - Opaque pointer returned by fbe_allocate_io_memory;
 *                           this param is optional (i.e., can be NULL).
 *
 * @return void
 *
 * @author:
 *     08-Feb-2013 Mike Wahl        Created.
 *
 ******************************************************************************/
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
fbe_u64_t
fbe_get_iomem_physical_address(
    void *virtual_address,
    void *opaque_mem_obj_p)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    return (fbe_u64_t)virtual_address;
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return MmGetPhysicalAddress(virtual_address);
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return MmGetPhysicalAddress(virtual_address);
#elif defined(FBE_PLATFORM_USE_PAL)
    if (NULL == opaque_mem_obj_p) {
        return EmcpalFindPhysicalAddress(virtual_address);
    }
    return EmcpalGetIoMemoryPhysicalAddress((PEMCPAL_IOMEMORY_OBJECT)opaque_mem_obj_p,
                                            virtual_address);
#elif defined(FBE_PLATFORM_USE_CSX)
    return MmGetPhysicalAddress(virtual_address);
#else
#error "WTF"
#endif
}
#endif

/*
 * These "contiguous memory" routines are depricated. If I/O memory is needed,
 * use the I/O memory routines directly; if not, use another memory allocation
 * routine that is appropriate (e.g., fbe_allocate_nonpaged_pool_with_tag).
 */
void *
fbe_allocate_contiguous_memory(
    fbe_u32_t number_of_bytes)
{
    return fbe_allocate_io_memory(number_of_bytes, NULL);
}

void
fbe_release_contiguous_memory(
    void *memory_ptr)
{
    fbe_release_io_memory(memory_ptr);
}

#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
fbe_u64_t
fbe_get_contigmem_physical_address(
    void *virtual_address)
{
    return fbe_get_iomem_physical_address(virtual_address, NULL);
}
#endif

void *
fbe_allocate_nonpaged_pool_with_tag(
    fbe_u32_t number_of_bytes,
    fbe_u32_t tag)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(tag);       /* SAFEBUG */
    return malloc(number_of_bytes);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    return EmcpalAllocateUtilityPoolWithTag(EmcpalNonPagedPool, number_of_bytes, tag);
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return EmcpalAllocateUtilityPool(EmcpalNonPagedPool, number_of_bytes); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return csx_p_util_mem_alloc_maybe(number_of_bytes); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_PAL)
    return EmcpalAllocateUtilityPoolWithTag(EmcpalNonPagedPool, (SIZE_T) number_of_bytes, tag);
#elif defined(FBE_PLATFORM_USE_CSX)
    return csx_p_util_mem_tagged_alloc_maybe(CSX_P_UTIL_MEM_TYPE_WIRED, tag, number_of_bytes);
#else
#error "WTF"
#endif
}

void
fbe_release_nonpaged_pool_with_tag(
    void *memory_ptr,
    fbe_u32_t tag)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(tag);       /* SAFEBUG */
    free(memory_ptr);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EmcpalFreeUtilityPoolWithTag(memory_ptr, tag);
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    EmcpalFreeUtilityPool(memory_ptr); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    csx_p_util_mem_free(memory_ptr); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalFreeUtilityPoolWithTag(memory_ptr, tag);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_util_mem_tagged_free(tag, memory_ptr);
#else
#error "WTF"
#endif
}

void
fbe_release_nonpaged_pool(
    void *memory_ptr)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    free(memory_ptr);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EmcpalFreeUtilityPool(memory_ptr);
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalFreeUtilityPool(memory_ptr);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_util_mem_free(memory_ptr);
#else
#error "WTF"
#endif
}

// This API is introduced to get IO memory from nonpaged pool. There are three flavours of the allocation
// fbe_allocate_nonpaged_pool_with_tag(witout tag): This api provides CSX utility memory
// fbe_allocate_io_nonpaged_pool_with_tag(without tag) : This api provides CSX IO memory via Emcpal bufman pool
// fbe_allocate_contigious_memory: This api allocates CSX contigious IO memory, inside CSX layer via csx_p_io_mem_allocate_with_flags
void *
fbe_allocate_io_nonpaged_pool_with_tag(
    fbe_u32_t number_of_bytes,
    fbe_u32_t tag)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(tag);       /* SAFEBUG */
    return malloc(number_of_bytes);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    return EmcpalAllocateUtilityPoolWithTag(EmcpalNonPagedPool, number_of_bytes, tag);
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return EmcpalAllocateUtilityPool(EmcpalNonPagedPool, number_of_bytes); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    return csx_p_util_mem_alloc_maybe(number_of_bytes); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_PAL)
    return EmcpalIoBufferAllocateAlways((SIZE_T) number_of_bytes);
#elif defined(FBE_PLATFORM_USE_CSX)
    return csx_p_util_mem_tagged_alloc_maybe(CSX_P_UTIL_MEM_TYPE_WIRED, tag, number_of_bytes);
#else
#error "WTF"
#endif
}

void
fbe_release_io_nonpaged_pool_with_tag(
    void *memory_ptr,
    fbe_u32_t tag)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    FBE_UNREFERENCED_PARAMETER(tag);       /* SAFEBUG */
    free(memory_ptr);
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EmcpalFreeUtilityPoolWithTag(memory_ptr, tag);
#elif defined(FBE_PLATFORM_USE_PAL) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    EmcpalFreeUtilityPool(memory_ptr); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_CSX) && (defined(UMODE_ENV) || defined(SIMMODE_ENV))
    csx_p_util_mem_free(memory_ptr); /* SAFEBUG - should use tagged API, but simmode code mismatches alloc and free calls */
#elif defined(FBE_PLATFORM_USE_PAL)
    EmcpalIoBufferFree(memory_ptr);
#elif defined(FBE_PLATFORM_USE_CSX)
    csx_p_util_mem_tagged_free(tag, memory_ptr);
#else
#error "WTF"
#endif
}
/*****************************/

fbe_irql_t
fbe_set_irql_to_dpc(
    void)
{
#if defined(FBE_PLATFORM_USE_WIN32)
    return 0;                              //PZPZ
#elif defined(FBE_PLATFORM_USE_WINDDK)
    EMCPAL_KIRQL irql, kirql;
    irql = KeGetCurrentIrql();
    if (irql != DISPATCH_LEVEL) {
        if ((irql != PASSIVE_LEVEL) && (irql != APC_LEVEL)) {
            fbe_KvTrace("%s: irql %d unexpected\n", __FUNCTION__, irql);
        }
        KeRaiseIrql(DISPATCH_LEVEL, &kirql);
    }
    return irql;
#elif defined(FBE_PLATFORM_USE_PAL)
    return 0;                              //PZPZ
#elif defined(FBE_PLATFORM_USE_CSX)
    return 0;                              //PZPZ
#else
#error "WTF"
#endif
}

void
fbe_restore_irql(
    fbe_irql_t irql)
{
#if defined(FBE_PLATFORM_USE_WIN32)
#elif defined(FBE_PLATFORM_USE_WINDDK)
    if (irql != DISPATCH_LEVEL) {
        KeLowerIrql(irql);
    }
#elif defined(FBE_PLATFORM_USE_PAL)
#elif defined(FBE_PLATFORM_USE_CSX)
#else
#error "WTF"
#endif
}

/*****************************/

void
bgsl_semaphore_init(
    bgsl_semaphore_t * sem,
    fbe_s32_t Count,
    fbe_s32_t Limit)
{
    fbe_semaphore_init(sem, Count, Limit);
}
void
bgsl_semaphore_release(
    bgsl_semaphore_t * sem,
    fbe_u32_t Increment,
    fbe_s32_t Adjustment,
    fbe_bool_t Wait)
{
    fbe_semaphore_release(sem, Increment, Adjustment, Wait);
}
EMCPAL_STATUS
bgsl_semaphore_wait(
    bgsl_semaphore_t * sem,
    EMCPAL_PLARGE_INTEGER Timeout)
{
    return fbe_semaphore_wait(sem, Timeout);
}
void
bgsl_semaphore_destroy(
    bgsl_semaphore_t * sem)
{
    fbe_semaphore_destroy(sem);
}

void
bgsl_mutex_init(
    bgsl_mutex_t * mutex)
{
    fbe_mutex_init(mutex);
}
void
bgsl_mutex_lock(
    bgsl_mutex_t * mutex)
{
    fbe_mutex_lock(mutex);
}
void
bgsl_mutex_unlock(
    bgsl_mutex_t * mutex)
{
    fbe_mutex_unlock(mutex);
}
void
bgsl_mutex_destroy(
    bgsl_mutex_t * mutex)
{
    fbe_mutex_destroy(mutex);
}

void
bgsl_spinlock_init(
    bgsl_spinlock_t * lock)
{
    fbe_spinlock_init(lock);
}
void
bgsl_spinlock_lock(
    bgsl_spinlock_t * lock)
{
    fbe_spinlock_lock(lock);
}
void
bgsl_spinlock_unlock(
    bgsl_spinlock_t * lock)
{
    fbe_spinlock_unlock(lock);
}
void
bgsl_spinlock_destroy(
    bgsl_spinlock_t * lock)
{
    fbe_spinlock_destroy(lock);
}


EMCPAL_STATUS
bgsl_thread_init(
    bgsl_thread_t * thread,
    const char *name,
    fbe_thread_user_root_t StartRoutine,
    fbe_thread_user_context_t StartContext)
{
    return fbe_thread_init(thread, name, StartRoutine, StartContext);
}
EMCPAL_STATUS
bgsl_thread_wait(
    bgsl_thread_t * thread)
{
    return fbe_thread_wait(thread);
}
void
bgsl_thread_delay(
    fbe_u32_t timeout_in_msec)
{
    fbe_thread_delay(timeout_in_msec);
}
void
bgsl_thread_destroy(
    bgsl_thread_t * thread)
{
    fbe_thread_destroy(thread);
}
void
bgsl_thread_exit(
    IN EMCPAL_STATUS ExitStatus)
{
    fbe_thread_exit(ExitStatus);
}

/*****************************/
