#include "fbe_event.h"
#include "fbe_trace.h"
#include "fbe_base.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_service.h"

void 
fbe_event_trace(fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list argList;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    if (trace_level > service_trace_level) {
        return;
    }

    va_start(argList, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_EVENT,
                     trace_level,
                     message_id,
                     fmt, 
                     argList);
    va_end(argList);
}

fbe_event_t * 
fbe_event_allocate(void)
{
    fbe_event_t * event;
    event = fbe_memory_native_allocate(sizeof(fbe_event_t));
    return event;
}

fbe_status_t 
fbe_event_release(fbe_event_t * event)
{
    fbe_memory_native_release(event);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_init(fbe_event_t * event)
{   
    event->magic_number = FBE_MAGIC_NUMBER_EVENT;
    event->base_edge = NULL;
    fbe_queue_element_init(&event->queue_element); 
    event->queue_context = NULL; 
    event->event_status = FBE_EVENT_STATUS_INVALID;
    fbe_event_exchange_state(event, FBE_EVENT_STATE_IN_PROGRESS);
    event->master_packet = NULL;
    event->current_level = -1;
    event->event_flags = 0;
    /* By default we should indicate that this event is very high priority so that 
     * the object does not check its medic action priority.  Only some events 
     * will set this to their current priority. 
     */
    event->medic_action_priority = FBE_MEDIC_ACTION_HIGHEST_PRIORITY;
    event->event_in_use = FBE_FALSE;

    fbe_zero_memory(event->stack, FBE_EVENT_STACK_SIZE * sizeof(fbe_event_stack_t));

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_destroy(fbe_event_t * event)
{

    if(event->magic_number != FBE_MAGIC_NUMBER_EVENT){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Invalid magic number\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    event->magic_number = 0;

    if(fbe_queue_is_element_on_queue(&event->queue_element)){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s queue_element is on the queue\n", __FUNCTION__);    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    event->master_packet = NULL;

    if(event->current_level != -1){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Invalid current_level %d \n", __FUNCTION__, event->current_level);  
        return FBE_STATUS_GENERIC_FAILURE;
    }
    event->current_level = -1;

    return FBE_STATUS_OK;
}

fbe_event_state_t 
fbe_event_exchange_state(fbe_event_t * event, fbe_event_state_t new_state)
{
    return (fbe_event_state_t)fbe_atomic_exchange(&event->event_state, new_state);
}

 fbe_status_t 
fbe_event_set_status(fbe_event_t * event, fbe_event_status_t event_status)
{
    event->event_status = event_status;
    return FBE_STATUS_OK;
}

 fbe_status_t 
fbe_event_get_status(fbe_event_t * event, fbe_event_status_t * event_status)
{
    *event_status = event->event_status;
    return FBE_STATUS_OK;
}

/* Queueing related functions */
 fbe_event_t * 
fbe_event_queue_element_to_event(fbe_queue_element_t * queue_element)
{
    fbe_event_t * event;
    event = (fbe_event_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_event_t  *)0)->queue_element));
    return event;
}

fbe_queue_element_t * 
fbe_event_get_queue_element(fbe_event_t * event)
{
    return &event->queue_element;
}

fbe_status_t 
fbe_event_complete(fbe_event_t * event)
{
    fbe_status_t status;
    fbe_queue_element_t * queue_element = NULL;
    fbe_event_level_t level;

    queue_element = fbe_event_get_queue_element(event);
    /* Something seriosly bad going on. Most likely we forgot to remove event from the queue */
    if(fbe_queue_is_element_on_queue(queue_element)){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s queue_element is on the queue \n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for( level = event->current_level; level >= 0; level--) {
        if(event->stack[level].completion_function != NULL) {
            status = event->stack[level].completion_function(event, event->stack[level].completion_context);
            if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED) {
                break;
            }
        }
        else {
            /* release the stack if completion function is not set. */
            event->current_level--;
        }
    }
    return FBE_STATUS_OK;
}


 fbe_status_t 
fbe_event_set_master_packet(fbe_event_t * event, fbe_packet_t * master_packet)
{
    event->master_packet = master_packet;   
    return FBE_STATUS_OK;
}

 fbe_status_t 
fbe_event_get_master_packet(fbe_event_t * event, fbe_packet_t ** master_packet)
{
    *master_packet = event->master_packet;  
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_get_type(fbe_event_t * event, fbe_event_type_t * event_type)
{
    *event_type = event->event_type;
    return FBE_STATUS_OK;
}

 fbe_status_t 
fbe_event_set_type(fbe_event_t * event, fbe_event_type_t event_type)
{
    event->event_type = event_type;
    return FBE_STATUS_OK;
}

 fbe_status_t
fbe_event_get_edge(fbe_event_t * event, fbe_base_edge_t ** base_edge)
{
    *base_edge = event->base_edge;
    return FBE_STATUS_OK;
}

 fbe_status_t
fbe_event_set_edge(fbe_event_t * event, fbe_base_edge_t * base_edge)
{
    event->base_edge = base_edge;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_enqueue_event(fbe_event_t * event, fbe_queue_head_t * queue_head)
{
    fbe_event_state_t old_state;
    fbe_queue_element_t * queue_element = NULL;
    queue_element = fbe_event_get_queue_element(event);

    fbe_queue_push(queue_head, queue_element);

    old_state = fbe_event_exchange_state(event, FBE_EVENT_STATE_QUEUED);
    
    switch(old_state) {
    case FBE_EVENT_STATE_IN_PROGRESS:
        break;
    case FBE_EVENT_STATE_CANCELED:
        break;
    default:
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event state %X \n", __FUNCTION__, old_state);
    }

    return FBE_STATUS_OK;
}

fbe_event_t * fbe_event_dequeue_event(fbe_queue_head_t * queue_head)
{
    fbe_event_state_t old_state;
    fbe_queue_element_t * queue_element = NULL;
    fbe_event_t * event = NULL;

    if(fbe_queue_is_empty(queue_head)){
        return NULL;
    }

    queue_element = fbe_queue_pop(queue_head);
    event = fbe_event_queue_element_to_event(queue_element);

    old_state = fbe_event_exchange_state(event, FBE_EVENT_STATE_IN_PROGRESS);

    switch(old_state) {
    case FBE_EVENT_STATE_QUEUED:
        break;
    case FBE_EVENT_STATE_CANCELED:
        break;
    default:
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event state %X \n", __FUNCTION__, old_state);
    }

    return event;
}

fbe_status_t 
fbe_event_remove_event_from_queue(fbe_event_t * event, fbe_queue_head_t * queue_head)
{
    fbe_event_state_t old_state;
    fbe_queue_element_t * queue_element = NULL;

    if(fbe_queue_is_empty(queue_head)){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: The queue is empty\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    queue_element = fbe_event_get_queue_element(event);

    if(!fbe_queue_is_element_on_queue(queue_element)){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s queue_element not on queue \n", __FUNCTION__);    

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_queue_remove(queue_element);

    old_state = fbe_event_exchange_state(event, FBE_EVENT_STATE_IN_PROGRESS);

    switch(old_state) {
    case FBE_EVENT_STATE_QUEUED:
        break;
    case FBE_EVENT_STATE_CANCELED:
        break;
    default:
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event state %X \n", __FUNCTION__, old_state);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_event_stack_t * 
fbe_event_allocate_stack(fbe_event_t * event)
{
    fbe_event_stack_t * event_stack = NULL;

    if(event->current_level >= FBE_EVENT_STACK_SIZE - 1) {
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event->current_level %d \n", __FUNCTION__, event->current_level);

        return NULL;
    }
    event->current_level ++;
    event_stack = &event->stack[event->current_level];
    return event_stack;
}

fbe_event_stack_t * 
fbe_event_get_current_stack(fbe_event_t * event)
{
    fbe_event_stack_t * event_stack = NULL;

    if(event->current_level >= FBE_EVENT_STACK_SIZE){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event->current_level %d \n", __FUNCTION__, event->current_level);

        return NULL;
    }

    if(event->current_level < 0){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event->current_level %d \n", __FUNCTION__, event->current_level);

        return NULL;
    }

    event_stack = &event->stack[event->current_level];
    return event_stack;
}

fbe_status_t
fbe_event_release_stack(fbe_event_t * event, fbe_event_stack_t *event_stack)
{
	if((event->current_level >= FBE_EVENT_STACK_SIZE) || (event->current_level < 0)){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event->current_level %d \n", __FUNCTION__, event->current_level);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((fbe_u8_t*)event_stack != (fbe_u8_t*)&event->stack[event->current_level]){
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: Invalid event_stack pointer %p \n", __FUNCTION__, event_stack);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    event->current_level--;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_stack_set_completion_function(fbe_event_stack_t * event_stack,
                                        fbe_event_completion_function_t completion_function,
                                        fbe_event_completion_context_t  completion_context)
{
    event_stack->completion_function = completion_function;
    event_stack->completion_context = completion_context;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_stack_set_extent( fbe_event_stack_t * event_stack, fbe_lba_t lba, fbe_block_count_t block_count)
{
    event_stack->lba = lba;
    event_stack->block_count = block_count;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_init_permit_request_data(fbe_event_t * event, fbe_event_permit_request_t *permit_request_p,
                                                fbe_permit_event_type_t permit_event_type)
{
    permit_request_p->permit_event_type = permit_event_type;
    permit_request_p->object_id   = FBE_OBJECT_ID_INVALID; 
    permit_request_p->is_start_b  = FBE_FALSE;
    permit_request_p->is_end_b    = FBE_FALSE;
    permit_request_p->unconsumed_block_count = 0;
    permit_request_p->is_beyond_capacity_b = FBE_FALSE;
    permit_request_p->object_index.Index32 = FBE_U32_MAX;
    permit_request_p->object_index.Index64 = FBE_U64_MAX;
    event->event_data.permit_request = *permit_request_p;
    return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_set_permit_request_data(fbe_event_t * event, fbe_event_permit_request_t * permit_request_p)
{
     event->event_data.permit_request = *permit_request_p;
     return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_set_data_request_data(fbe_event_t * event, fbe_event_data_request_t * data_request_p)
{
     event->event_data.data_request = *data_request_p;
     return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_set_verify_report_data(fbe_event_t * event, fbe_event_verify_report_t* verify_report_p)
{
     event->event_data.verify_report = *verify_report_p;
     return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_log_set_source_data(fbe_event_event_log_request_t *event_log_p, 
                                           fbe_u32_t source_bus, fbe_u32_t source_enclosure, fbe_u32_t source_slot)
{
    if ((source_bus > FBE_EVENT_DRIVE_LOCATION_MAX_PORT_VALUE)       ||
        (source_enclosure > FBE_EVENT_DRIVE_LOCATION_MAX_ENCL_VALUE) ||
        (source_slot > FBE_EVENT_DRIVE_LOCATION_MAX_SLOT_VALUE)         )
    {
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: bus: %d encl: %d or slot: %d too large\n", 
                        __FUNCTION__, source_bus, source_enclosure, source_slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    event_log_p->source_pvd_location.bus_number = source_bus;
    event_log_p->source_pvd_location.enclosure_number = source_enclosure;
    event_log_p->source_pvd_location.slot_number = source_slot;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_log_set_destination_data(fbe_event_event_log_request_t *event_log_p, 
                                                fbe_u32_t destination_bus, fbe_u32_t destination_enclosure, fbe_u32_t destination_slot)
{
    if ((destination_bus > FBE_EVENT_DRIVE_LOCATION_MAX_PORT_VALUE)       ||
        (destination_enclosure > FBE_EVENT_DRIVE_LOCATION_MAX_ENCL_VALUE) ||
        (destination_slot > FBE_EVENT_DRIVE_LOCATION_MAX_SLOT_VALUE)         )
    {
        fbe_event_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s: bus: %d encl: %d or slot: %d too large\n", 
                        __FUNCTION__, destination_bus, destination_enclosure, destination_slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    event_log_p->destination_pvd_location.bus_number = destination_bus;
    event_log_p->destination_pvd_location.enclosure_number = destination_enclosure;
    event_log_p->destination_pvd_location.slot_number = destination_slot;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_set_event_log_request_data(fbe_event_t * event, fbe_event_event_log_request_t* event_log_p)
{
     event->event_data.event_log_request = *event_log_p ;
     return FBE_STATUS_OK;       
}


fbe_status_t fbe_event_get_permit_request_data(fbe_event_t * event, fbe_event_permit_request_t* permit_request_p)
{
     *permit_request_p = event->event_data.permit_request;
     return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_get_data_request_data(fbe_event_t * event, fbe_event_data_request_t* data_request_p)
{
     *data_request_p= event->event_data.data_request;
     return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_get_verify_report_data(fbe_event_t * event, fbe_event_verify_report_t* verify_report_p)
{
     *verify_report_p = event->event_data.verify_report;
     return FBE_STATUS_OK;       
}

fbe_status_t fbe_event_get_event_log_request_data(fbe_event_t * event, fbe_event_event_log_request_t* event_log_p)
{
     *event_log_p = event->event_data.event_log_request;
     return FBE_STATUS_OK;       
}



fbe_status_t 
fbe_event_stack_get_extent(fbe_event_stack_t * event_stack, fbe_lba_t * lba, fbe_block_count_t * block_count)
{
    *lba = event_stack->lba;
    *block_count = event_stack->block_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_set_block_transport_server(fbe_event_t * event, struct fbe_block_transport_server_s * block_transport_server)
{
    event->block_transport_server = block_transport_server;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_get_block_transport_server(fbe_event_t * event,struct fbe_block_transport_server_s ** block_transport_server)
{
    * block_transport_server = event->block_transport_server;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_stack_set_current_offset(fbe_event_stack_t * event_stack, fbe_lba_t current_offset)
{
    event_stack->current_offset = current_offset;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_stack_get_current_offset(fbe_event_stack_t * event_stack, fbe_lba_t * current_offset)
{
    * current_offset = event_stack->current_offset;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_stack_set_previous_offset(fbe_event_stack_t * event_stack, fbe_lba_t previous_offset)
{
    event_stack->previous_offset = previous_offset;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_stack_get_previous_offset(fbe_event_stack_t * event_stack, fbe_lba_t * previous_offset)
{
    *previous_offset = event_stack->previous_offset;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_get_flags(fbe_event_t * event, fbe_u32_t * event_flags)
{
    * event_flags = event->event_flags;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_set_flags(fbe_event_t * event, fbe_u32_t  event_flags)
{
    event->event_flags |= event_flags;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_set_medic_action_priority(fbe_event_t * event, fbe_medic_action_priority_t medic_action_priority)
{
    event->medic_action_priority = medic_action_priority;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_event_get_medic_action_priority(fbe_event_t * event, fbe_medic_action_priority_t * medic_action_priority)
{
    *medic_action_priority = event->medic_action_priority;
    return FBE_STATUS_OK;
}
