/* #include "fbe_event_service.h" */
#include "fbe_event.h"
#include "fbe_trace.h"
#include "fbe_base.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"
#include "fbe_base_service.h"
#include "fbe_block_transport.h"

#define FBE_EVENT_SERVICE_MAX_QUEUE_DEPTH 10240

typedef enum event_service_thread_flag_e{
    EVENT_SERVICE_THREAD_RUN,
    EVENT_SERVICE_THREAD_STOP,
    EVENT_SERVICE_THREAD_DONE
}event_service_thread_flag_t;

static fbe_spinlock_t       event_service_lock; 

static fbe_queue_head_t     event_service_run_queue_head;
static fbe_queue_head_t     event_service_pending_queue_head;

static fbe_semaphore_t      event_service_run_queue_semaphore;
static fbe_thread_t         event_service_run_queue_thread_handle;
static event_service_thread_flag_t  event_service_run_queue_thread_flag;
static fbe_block_transport_server_t * in_fly_event_block_tranport_p;
static fbe_object_id_t              in_fly_event_object_id;

typedef struct fbe_event_service_s{
    fbe_base_service_t  base_service;
}fbe_event_service_t;

static fbe_event_service_t event_service;
static fbe_status_t fbe_event_service_control_entry(fbe_packet_t * packet);

fbe_service_methods_t fbe_event_service_methods = {FBE_SERVICE_ID_EVENT, fbe_event_service_control_entry};

/* Forward declaration */
static void event_service_run_queue_thread_func(void * context);
static void event_service_dispatch_run_queue(void);
static fbe_status_t event_service_dispatch_run_queue_completion(fbe_event_t * event, fbe_event_completion_context_t context);

static fbe_status_t 
fbe_event_service_init(fbe_packet_t * packet)
{    
    EMCPAL_STATUS nt_status;

    fbe_event_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    /* Initialize event_service lock */
    fbe_spinlock_init(&event_service_lock);

    /* Initialize event_service queues */
    fbe_queue_init(&event_service_run_queue_head);
    fbe_queue_init(&event_service_pending_queue_head);

    fbe_semaphore_init(&event_service_run_queue_semaphore, 0, FBE_EVENT_SERVICE_MAX_QUEUE_DEPTH);

    in_fly_event_block_tranport_p = NULL;
    in_fly_event_object_id = FBE_OBJECT_ID_INVALID;    

    /* Start run_queue thread */
    event_service_run_queue_thread_flag = EVENT_SERVICE_THREAD_RUN;
    nt_status = fbe_thread_init(&event_service_run_queue_thread_handle, "fbe_evt_svcrq", event_service_run_queue_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_service_set_service_id((fbe_base_service_t *) &event_service, FBE_SERVICE_ID_EVENT);

    fbe_base_service_init((fbe_base_service_t *) &event_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}    

fbe_status_t 
fbe_event_service_destroy(fbe_packet_t * packet)
{
    /* Stop run_queue thread */
    event_service_run_queue_thread_flag = EVENT_SERVICE_THREAD_STOP;  
    fbe_semaphore_release(&event_service_run_queue_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&event_service_run_queue_thread_handle);
    fbe_thread_destroy(&event_service_run_queue_thread_handle);

    fbe_semaphore_destroy(&event_service_run_queue_semaphore);  
    fbe_queue_destroy(&event_service_run_queue_head);
    fbe_queue_destroy(&event_service_pending_queue_head);

    fbe_spinlock_destroy(&event_service_lock);

    fbe_base_service_destroy((fbe_base_service_t *) &event_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_event_service_control_entry(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_event_service_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &event_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_event_service_destroy( packet);
            break;

        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&event_service, packet);
            break;
    }

    return status;
}

/* Dispatch first element on the queue */
static void 
event_service_dispatch_run_queue(void)
{
    fbe_queue_element_t * event_queue_element;
    fbe_event_t *         event = NULL;

    fbe_spinlock_lock(&event_service_lock);
    if(fbe_queue_is_empty(&event_service_run_queue_head))
    {
        fbe_spinlock_unlock(&event_service_lock);
        return;
    }

    event_queue_element = fbe_queue_pop(&event_service_run_queue_head);
    event = fbe_event_queue_element_to_event(event_queue_element);

    /* save the block transport server to track it while event is in fly */
    in_fly_event_block_tranport_p = event->block_transport_server;

    /* fbe_queue_push(&event_service_pending_queue_head, &event->queue_element); */
    fbe_spinlock_unlock(&event_service_lock);

    fbe_block_transport_server_send_event(event->block_transport_server, event);


    /* clear the block transport server as it is no longer in process */
    fbe_spinlock_lock(&event_service_lock);
    in_fly_event_block_tranport_p = NULL;
    fbe_spinlock_unlock(&event_service_lock);    
    return;
}


static fbe_status_t 
event_service_dispatch_run_queue_completion(fbe_event_t* event, fbe_event_completion_context_t context)
{
    fbe_event_stack_t* current_event_stack = NULL;
    fbe_event_stack_t* new_event_stack = NULL;
    fbe_lba_t          current_start_lba;
    fbe_lba_t          current_end_lba;
    fbe_block_count_t  current_block_count;
    fbe_lba_t          orig_start_lba;
    fbe_lba_t          orig_end_lba;
    fbe_block_count_t  orig_block_count;
    fbe_lba_t          current_offset;
    fbe_lba_t          prev_offset;

    /* get current entry on event stack */
    current_event_stack = fbe_event_get_current_stack(event);

    /* get the current offset, lba and block count of the current stack */
    fbe_event_stack_get_extent(current_event_stack, &current_start_lba, &current_block_count);
    fbe_event_stack_get_current_offset(current_event_stack, &current_offset);
    fbe_event_stack_get_previous_offset(current_event_stack, &prev_offset);

    /* get the current stack end lba. */
    current_end_lba = current_start_lba + current_block_count + prev_offset - 1;

    /* release the stack */
    fbe_event_release_stack(event, current_event_stack);

    /* get the original event stack lba range. */
    current_event_stack = fbe_event_get_current_stack(event);
    fbe_event_stack_get_extent(current_event_stack, &orig_start_lba, &orig_block_count);

    /* get the original stack end lba and if it is grater than current then
     * complete the event.
     */
    orig_end_lba = orig_start_lba + orig_block_count - 1;
    if (current_end_lba >= orig_end_lba)
    {
        return FBE_STATUS_OK;
    }

    /* allocate new entry on event stack */
    new_event_stack = fbe_event_allocate_stack(event);

    /* update the block count with left number of blocks from the current offset, block 
     * transport will update the block count based on edge capacity.
     */
    fbe_event_stack_set_extent(new_event_stack,
                               current_offset,
                               (fbe_block_count_t) (orig_end_lba - current_offset + 1));

    /* set new entry's completion function */
    fbe_event_stack_set_completion_function(new_event_stack, event_service_dispatch_run_queue_completion, NULL);

    /* deliver event */
    fbe_spinlock_lock(&event_service_lock);
    fbe_queue_push(&event_service_run_queue_head, &event->queue_element);
    fbe_spinlock_unlock(&event_service_lock);
    fbe_semaphore_release(&event_service_run_queue_semaphore, 0, 1, FALSE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


static void 
event_service_run_queue_thread_func(void * context)
{
	fbe_thread_set_affinity(&event_service_run_queue_thread_handle, 0x1);

    while(1)    
    {
        fbe_semaphore_wait(&event_service_run_queue_semaphore, NULL);       

        if(event_service_run_queue_thread_flag == EVENT_SERVICE_THREAD_RUN) {
            event_service_dispatch_run_queue();
        } else {
            break;
        }
    }

    event_service_run_queue_thread_flag = EVENT_SERVICE_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


fbe_status_t 
fbe_event_service_send_event(fbe_event_t* event)
{
    fbe_block_count_t   block_count;
    fbe_event_stack_t*  current_event_stack = NULL;
    fbe_lba_t           lba;
    fbe_event_stack_t*  new_event_stack = NULL;

    /* get current entry on event stack */
    current_event_stack = fbe_event_get_current_stack(event);

    /* get lba and block count from current entry */
    fbe_event_stack_get_extent(current_event_stack, &lba, &block_count);

    /* validate current entry's lba and block count */
    if ((lba == FBE_LBA_INVALID) || (block_count == 0))
    {
        fbe_event_type_t event_type;

        /* get current entry's event type */
        fbe_event_get_type(event, &event_type);

        /* trace send event error - invalid extent */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s event extent invalid, event type: 0x%x, lba: 0x%llx, block count: 0x%llx\n",
                                 __FUNCTION__, event_type,
				 (unsigned long long)lba,
				 (unsigned long long)block_count);

        /* set failure status in event and complete event */
        fbe_event_set_status(event, FBE_EVENT_STATUS_GENERIC_FAILURE);
        fbe_event_complete(event);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* allocate new entry on event stack */
    new_event_stack = fbe_event_allocate_stack(event);

    /* set new entry's extent info (lba and block count) */
    fbe_event_stack_set_extent(new_event_stack, lba, block_count);

    /* set new entry's current and previous offsets */
    fbe_event_stack_set_current_offset(new_event_stack, 0);
    fbe_event_stack_set_previous_offset(new_event_stack, 0);

    /* set new entry's completion function */
    fbe_event_stack_set_completion_function(new_event_stack, event_service_dispatch_run_queue_completion, NULL);

    /* now, deliver event */
    fbe_spinlock_lock(&event_service_lock);
    fbe_queue_push(&event_service_run_queue_head, &event->queue_element);
    fbe_spinlock_unlock(&event_service_lock);
    fbe_semaphore_release(&event_service_run_queue_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}



fbe_status_t 
fbe_event_service_get_block_transport_event_count(fbe_u32_t *event_count, 
struct  fbe_block_transport_server_s *block_transport_server)
{

 /* This routine determines how many events are there in event service queue which are 
  * belongs to block transport server. It also consider if the event is in fly.
  */
     

    fbe_event_t *event_p;
    fbe_u32_t count = 0;
    fbe_queue_element_t * event_queue_element;
    fbe_block_transport_server_t *event_queue_block_transport_server;

    /* Check if any associate event in fly for requested block transport server */
    if(in_fly_event_block_tranport_p == block_transport_server)
    {
        count++;
    }

    fbe_spinlock_lock(&event_service_lock);

    /* get the first element from queue */
    event_queue_element = fbe_queue_front(&event_service_run_queue_head);

    /* calculate number of events in queue associate with block transport server */
    while (event_queue_element != NULL)
    {
        event_p = fbe_event_queue_element_to_event(event_queue_element);
        fbe_event_get_block_transport_server(event_p, &event_queue_block_transport_server);
        if(block_transport_server == event_queue_block_transport_server)
        {
            count++;
        }
        event_queue_element = fbe_queue_next(&event_service_run_queue_head, event_queue_element);
    }

    fbe_spinlock_unlock(&event_service_lock);

/*
    fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                         "get event count, in flight 0x%x, destroy block trans 0x%x, event cnt %d, queue len %d\n", 
                         in_flight_event_block_tranport_p, 
                         block_transport_server, 
                         count, fbe_queue_length(&event_service_run_queue_head) );
*/

    *event_count = count;
    
    return    FBE_STATUS_OK;

}
void
fbe_event_service_in_fly_event_save_object_id(fbe_object_id_t object_id)
{
    /* save this client id while event is in fly to determine that 
     * destination object has outstanding event which is on the way after remove from
     * event service run queue so object can take appropriate action if needed.
     */
    fbe_spinlock_lock(&event_service_lock);
    in_fly_event_object_id = object_id;
    fbe_spinlock_unlock(&event_service_lock);   
    return;
}            

void
fbe_event_service_in_fly_event_clear_object_id()
{
     /* clear the object id as in fly event is processed */
    fbe_spinlock_lock(&event_service_lock);
    in_fly_event_object_id = FBE_OBJECT_ID_INVALID;
    fbe_spinlock_unlock(&event_service_lock);   
    return;
}            


fbe_status_t 
fbe_event_service_client_is_event_in_fly(fbe_object_id_t object_id, bool *is_event_in_fly)
{

 /* This routine determines if there is any event in fly for the given destination client id. */

    *is_event_in_fly = FBE_FALSE;

    fbe_spinlock_lock(&event_service_lock);

    if(in_fly_event_object_id == object_id)
    {
        *is_event_in_fly = FBE_TRUE;
    }

    fbe_spinlock_unlock(&event_service_lock);

    return FBE_STATUS_OK;   
}



