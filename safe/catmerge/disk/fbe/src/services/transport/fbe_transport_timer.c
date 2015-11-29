#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_trace.h"
#include "fbe_base.h"

#define FBE_TRANSPORT_TIMER_MAX_QUEUE_DEPTH 10240
/* Idle timer triggers idle queue dispatch. The resolution in milliseconds */
#define FBE_TRANSPORT_TIMER_IDLE_TIMER 100L /* 100 ms.*/

typedef enum transport_timer_thread_flag_e{
    TRANSPORT_TIMER_THREAD_RUN,
    TRANSPORT_TIMER_THREAD_STOP,
    TRANSPORT_TIMER_THREAD_DONE
}transport_timer_thread_flag_t;

static fbe_spinlock_t		transport_timer_lock; 
static fbe_queue_head_t	    transport_timer_idle_queue_head;

/* static fbe_semaphore_t      transport_timer_idle_queue_semaphore; */
static fbe_thread_t			transport_timer_idle_queue_thread_handle;
static transport_timer_thread_flag_t  transport_timer_idle_queue_thread_flag;


/* Forward declaration */ 
static void transport_timer_idle_queue_thread_func(void * context);
static void transport_timer_dispatch_idle_queue(void);

void
transport_timer_trace(fbe_trace_level_t trace_level,
                fbe_trace_message_id_t message_id,
                const char * fmt, ...)
                __attribute__((format(__printf_func__,3,4)));
void
transport_timer_trace(fbe_trace_level_t trace_level,
                fbe_trace_message_id_t message_id,
                const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

	if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_TRANSPORT,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

fbe_status_t 
fbe_transport_timer_init(void)
{
    transport_timer_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

	/* Initialize transport_timer lock */
	fbe_spinlock_init(&transport_timer_lock);

    /* Initialize transport_timer queues */
	fbe_queue_init(&transport_timer_idle_queue_head);
	/*fbe_semaphore_init(&transport_timer_idle_queue_semaphore, 0, FBE_TRANSPORT_TIMER_MAX_QUEUE_DEPTH); */

    /* Start idle thread */
	transport_timer_idle_queue_thread_flag = TRANSPORT_TIMER_THREAD_RUN;
	fbe_thread_init(&transport_timer_idle_queue_thread_handle, "fbe_trans_tim_idle", transport_timer_idle_queue_thread_func, NULL);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_timer_destroy(void)
{
    transport_timer_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

	/* Stop idle_queue thread */
    transport_timer_idle_queue_thread_flag = TRANSPORT_TIMER_THREAD_STOP;  
	/*fbe_semaphore_release(&transport_timer_idle_queue_semaphore, 0, 1, FALSE); */
	fbe_thread_wait(&transport_timer_idle_queue_thread_handle);
	fbe_thread_destroy(&transport_timer_idle_queue_thread_handle);

	/* fbe_semaphore_destroy(&transport_timer_idle_queue_semaphore);	*/
	fbe_queue_destroy(&transport_timer_idle_queue_head);

	fbe_spinlock_destroy(&transport_timer_lock);

    return FBE_STATUS_OK;
}

static void 
transport_timer_idle_queue_thread_func(void * context)
{

	FBE_UNREFERENCED_PARAMETER(context);

    transport_timer_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    while(1)    
	{
		fbe_thread_delay(FBE_TRANSPORT_TIMER_IDLE_TIMER);
		if(transport_timer_idle_queue_thread_flag == TRANSPORT_TIMER_THREAD_RUN) {
			transport_timer_dispatch_idle_queue();
		} else {
			break;
		}
    }

    transport_timer_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s done\n", __FUNCTION__);

    transport_timer_idle_queue_thread_flag = TRANSPORT_TIMER_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void 
transport_timer_dispatch_idle_queue(void)
{
	fbe_transport_timer_t * transport_timer = NULL;
	fbe_transport_timer_t * next_transport_timer = NULL;
	fbe_packet_t * packet = NULL;	

	fbe_spinlock_lock(&transport_timer_lock);

	if(fbe_queue_is_empty(&transport_timer_idle_queue_head)){
		fbe_spinlock_unlock(&transport_timer_lock);
		return;
	}

	/* Loop over all elements and push the to run_queue if needed */
	next_transport_timer = (fbe_transport_timer_t *)fbe_queue_front(&transport_timer_idle_queue_head);
	while(next_transport_timer != NULL) {
		transport_timer = next_transport_timer;
		next_transport_timer = (fbe_transport_timer_t *)fbe_queue_next(&transport_timer_idle_queue_head, &next_transport_timer->queue_element);

		if(transport_timer->expiration_time <= fbe_get_time()){
			fbe_queue_remove(&transport_timer->queue_element);
			packet = fbe_transport_timer_to_packet(transport_timer);
			fbe_spinlock_unlock(&transport_timer_lock);
			fbe_transport_complete_packet(packet);
			fbe_spinlock_lock(&transport_timer_lock);
			/* We have to start form the head */
			next_transport_timer = (fbe_transport_timer_t *)fbe_queue_front(&transport_timer_idle_queue_head);
		}
	} /* while(next_transport_timer != NULL) */

	fbe_spinlock_unlock(&transport_timer_lock);
}

fbe_status_t 
fbe_transport_set_timer(fbe_packet_t * packet,
						fbe_time_t timeout,
						fbe_packet_completion_function_t completion_function,
						fbe_packet_completion_context_t completion_context)
{

	fbe_queue_element_init(&packet->transport_timer.queue_element);
	packet->transport_timer.expiration_time = fbe_get_time() + timeout;

	fbe_transport_set_completion_function(packet, completion_function,  completion_context);

	fbe_spinlock_lock(&transport_timer_lock);
	fbe_queue_push(&transport_timer_idle_queue_head, &packet->transport_timer.queue_element);
	fbe_spinlock_unlock(&transport_timer_lock);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_cancel_timer(fbe_packet_t * packet)
{
	fbe_spinlock_lock(&transport_timer_lock);
	fbe_queue_remove(&packet->transport_timer.queue_element);
	fbe_spinlock_unlock(&transport_timer_lock);

	fbe_transport_complete_packet(packet);

	return FBE_STATUS_OK;
}

