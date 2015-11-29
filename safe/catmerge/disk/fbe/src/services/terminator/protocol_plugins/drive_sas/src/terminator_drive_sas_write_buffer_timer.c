#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe_terminator.h"
#include "terminator_drive_sas_write_buffer_timer.h"

/* Idle timer triggers idle queue dispatch. The resolution in milliseconds */
#define TERMINATOR_SAS_DRIVE_WRITE_BUFFER_TIMER_IDLE_TIME 200L /* 200 ms.*/

typedef enum write_buffer_timer_thread_flag_e{
    WRITE_BUFFER_TIMER_THREAD_RUN,
    WRITE_BUFFER_TIMER_THREAD_STOP,
    WRITE_BUFFER_TIMER_THREAD_DONE
}write_buffer_timer_thread_flag_t;

static fbe_spinlock_t        write_buffer_timer_lock;
static fbe_queue_head_t        write_buffer_timer_idle_queue_head;

static fbe_thread_t            write_buffer_timer_idle_queue_thread_handle;
static write_buffer_timer_thread_flag_t  write_buffer_timer_idle_queue_thread_flag;


/* Forward declaration */
static void write_buffer_timer_idle_queue_thread_func(void * context);
static void write_buffer_timer_dispatch_idle_queue(void);

fbe_status_t
terminator_sas_drive_write_buffer_timer_init(void)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    /* Initialize write_buffer_timer lock */
    fbe_spinlock_init(&write_buffer_timer_lock);

    /* Initialize write_buffer_timer queues */
    fbe_queue_init(&write_buffer_timer_idle_queue_head);

    /* Start idle thread */
    write_buffer_timer_idle_queue_thread_flag = WRITE_BUFFER_TIMER_THREAD_RUN;
	fbe_thread_init(&write_buffer_timer_idle_queue_thread_handle, "terminator_write_buf_timer", write_buffer_timer_idle_queue_thread_func, NULL);

    return FBE_STATUS_OK;
}

fbe_status_t
terminator_sas_drive_write_buffer_timer_destroy(void)
{
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    if(write_buffer_timer_idle_queue_thread_flag == WRITE_BUFFER_TIMER_THREAD_RUN) {
        /* Stop idle_queue thread */
        write_buffer_timer_idle_queue_thread_flag = WRITE_BUFFER_TIMER_THREAD_STOP;
        fbe_thread_wait(&write_buffer_timer_idle_queue_thread_handle);
        fbe_thread_destroy(&write_buffer_timer_idle_queue_thread_handle);

        fbe_queue_destroy(&write_buffer_timer_idle_queue_head);

        fbe_spinlock_destroy(&write_buffer_timer_lock);
    }

    return FBE_STATUS_OK;
}

static void
write_buffer_timer_idle_queue_thread_func(void * context)
{

    FBE_UNREFERENCED_PARAMETER(context);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    while(1)
    {
        fbe_thread_delay(TERMINATOR_SAS_DRIVE_WRITE_BUFFER_TIMER_IDLE_TIME);
        if(write_buffer_timer_idle_queue_thread_flag == WRITE_BUFFER_TIMER_THREAD_RUN) {
            write_buffer_timer_dispatch_idle_queue();
        } else {
            break;
        }
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s done\n", __FUNCTION__);

    write_buffer_timer_idle_queue_thread_flag = WRITE_BUFFER_TIMER_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void
write_buffer_timer_dispatch_idle_queue(void)
{
    terminator_sas_drive_write_buffer_timer_t * write_buffer_timer = NULL;
    terminator_sas_drive_write_buffer_timer_t * next_write_buffer_timer = NULL;
    fbe_time_t current_time;

    fbe_spinlock_lock(&write_buffer_timer_lock);

    if(fbe_queue_is_empty(&write_buffer_timer_idle_queue_head)){
        fbe_spinlock_unlock(&write_buffer_timer_lock);
        return;
    }

    next_write_buffer_timer = (terminator_sas_drive_write_buffer_timer_t *)fbe_queue_front(&write_buffer_timer_idle_queue_head);
    current_time = fbe_get_time();

    /* Loop over all elements and check if they expire */
    while(next_write_buffer_timer != NULL) {
        write_buffer_timer = next_write_buffer_timer;
        next_write_buffer_timer = (terminator_sas_drive_write_buffer_timer_t *)fbe_queue_next(&write_buffer_timer_idle_queue_head, &next_write_buffer_timer->queue_element);

        if(write_buffer_timer->expiration_time <= current_time){
            fbe_queue_remove(&write_buffer_timer->queue_element);
            fbe_spinlock_unlock(&write_buffer_timer_lock);

            sas_drive_process_payload_write_buffer_timer_func(&write_buffer_timer->context);
            fbe_terminator_free_memory(write_buffer_timer);

            fbe_spinlock_lock(&write_buffer_timer_lock);
            /* We have to start form the head */
            next_write_buffer_timer = (terminator_sas_drive_write_buffer_timer_t *)fbe_queue_front(&write_buffer_timer_idle_queue_head);
        }
    } /* while(next_write_buffer_timer != NULL) */

    fbe_spinlock_unlock(&write_buffer_timer_lock);
}

fbe_status_t
terminator_sas_drive_write_buffer_set_timer(fbe_time_t timeout_msec, write_buffer_timer_context_t context)
{
    terminator_sas_drive_write_buffer_timer_t *timer = fbe_terminator_allocate_memory(sizeof(terminator_sas_drive_write_buffer_timer_t));
    if(timer == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: allocate write buffer timer memory failed\n",
			 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    timer->context = context;
    timer->expiration_time = fbe_get_time() + timeout_msec;
    fbe_queue_element_init(&timer->queue_element);

    fbe_spinlock_lock(&write_buffer_timer_lock);
    fbe_queue_push(&write_buffer_timer_idle_queue_head, &timer->queue_element);
    fbe_spinlock_unlock(&write_buffer_timer_lock);
    return FBE_STATUS_OK;
}

fbe_status_t
terminator_sas_drive_write_buffer_cancel_timer(terminator_sas_drive_write_buffer_timer_t * timer)
{
    fbe_spinlock_lock(&write_buffer_timer_lock);
    fbe_queue_remove(&timer->queue_element);
    fbe_spinlock_unlock(&write_buffer_timer_lock);

    fbe_terminator_free_memory(timer);

    return FBE_STATUS_OK;
}

