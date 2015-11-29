/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2006 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: fbe_transport_main.c
 ****
 **** DESCRIPTION:
 **** NOTES:
 ****    <TBS>
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_transport.h"
#include "fbe/fbe_multicore_spinlock.h"

/* Global variables */
fbe_transport_service_t transport_service;

/****************************************************************************
 * Local variables
 ****************************************************************************/
static fbe_multicore_spinlock_t fbe_transport_multicore_spinlock;

/****************************************************************************
 * Forward declaration
 ****************************************************************************/
static fbe_status_t fbe_transport_preserve_packet_stack(fbe_packet_t * packet);
static fbe_status_t fbe_transport_sync_completion_function(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static __forceinline fbe_status_t fbe_transport_record_callback(fbe_packet_t * packet, fbe_packet_completion_function_t completion_function);

/* Save the highest stack number for any non-zero packet stack */
fbe_packet_level_t packet_stack_max_level = 0;
static fbe_atomic_t max_packet_callstack_depth = 0;
static fbe_u32_t  fbe_transport_coarse_time = 0;

static fbe_atomic_32_t last_time_packet_recorded = 0xffffffff;
#define PACKET_LOGGING_TIME_THREADHOLD  20000  /* 20 seconds */
#define PACKET_LOGGING_WINDOW           40000  /* 40 seconds */

void fbe_transport_log_packet(fbe_packet_t *packet, fbe_u32_t current_time);
void fbe_transport_log_bouncer(fbe_queue_element_t *queue_element);

void fbe_transport_log_bouncer(fbe_queue_element_t *queue_element)
{
    fbe_u32_t current_time;
    fbe_u32_t delta_time;
    fbe_atomic_32_t last_recorded;
    fbe_u64_t magic_number;

    current_time = fbe_transport_get_coarse_time();

    if (current_time > last_time_packet_recorded)
    {
        delta_time = current_time - last_time_packet_recorded;
    }
    else
    {
        delta_time = last_time_packet_recorded - current_time;
    }

    if (delta_time < PACKET_LOGGING_WINDOW)
    {
        // nothing to do
        return;
    }

    last_recorded = fbe_atomic_32_exchange(&last_time_packet_recorded, current_time);

    if (current_time > last_recorded)
    {
        delta_time = current_time - last_recorded;
    }
    else
    {
        delta_time = last_recorded - current_time;
    }

    /* only trace if this trace is outside the windows */
    if (delta_time > PACKET_LOGGING_WINDOW)
    {
        magic_number = *(fbe_u64_t *)(((fbe_u8_t *)queue_element) + sizeof(fbe_queue_element_t));
        if(magic_number == FBE_MAGIC_NUMBER_BASE_PACKET)
        {
            fbe_packet_t * packet;
            packet = fbe_transport_queue_element_to_packet(queue_element);
            fbe_transport_log_packet(packet, current_time);
        }
    }
    else
    {
        // restore last recorded time.
        current_time = fbe_atomic_32_exchange(&last_time_packet_recorded, last_recorded);
    }
    return;
}

void fbe_transport_log_packet(fbe_packet_t *packet, fbe_u32_t current_time)
{
    fbe_u8_t tracker_current_index;  /* Entry to start printing at. */
    fbe_u8_t tracker_entries;     

    fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "packet logger start: Packet %p current time 0x%x\n", 
                            packet, current_time);

    tracker_current_index = packet->tracker_current_index;
    if (packet->tracker_index_wrapped > 0)
    {
        tracker_entries = FBE_PACKET_TRACKER_DEPTH ;
    }
    else
    {
        tracker_entries = tracker_current_index ;
    }
    for (;tracker_entries>0; tracker_entries --)
    {
        if (tracker_current_index == 0)
        {
            tracker_current_index = FBE_PACKET_TRACKER_DEPTH - 1;
        }
        tracker_current_index --;
        fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "packet logger %p 0x%x 0x%x\n", 
                                packet->tracker_ring[tracker_current_index].callback_fn, 
                                packet->tracker_ring[tracker_current_index].coarse_time,
                                packet->tracker_ring[tracker_current_index].action );
    }
    fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "packet logger end %p\n", 
                            packet);

    return;
}

fbe_status_t fbe_transport_record_callback_with_action(fbe_packet_t * packet,
                                                       fbe_packet_completion_function_t completion_function,
                                                       stack_tracker_action action)
{
    packet->tracker_ring[packet->tracker_current_index].callback_fn = completion_function;
    packet->tracker_ring[packet->tracker_current_index].coarse_time = fbe_transport_get_coarse_time();
    packet->tracker_ring[packet->tracker_current_index].action = action;
    packet->tracker_current_index ++;
    if (packet->tracker_current_index>=FBE_PACKET_TRACKER_DEPTH)
    {
        packet->tracker_current_index = 0;
        packet->tracker_index_wrapped ++;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_transport_initialize_packet_common(fbe_packet_t * packet)
{
    /* This check is not valid since we allocate packets from pools that contain "data". 
     * The data could have any pattern, even a pattern that matches this magic. 
     * We also sometimes do not clear out our magic number in the performance path in 
     * an effort to avoid touching the packet on completion. 
     */
#if 0
	if(packet->magic_number == FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
						 		FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error packet already initialized", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}
#endif

#if 0
	/* In case of a destroyed packet, copy all non-zero packet stack
	 * to the top and zero out the non-zero portion. If the stack is full,
	 * only zero the first 2 entries.
	 */
	if (packet->magic_number == FBE_MAGIC_NUMBER_DESTROYED_PACKET){
		fbe_transport_preserve_packet_stack(packet);
	}
	else {	
		/* Zero packet stack */
		fbe_zero_memory(packet->stack, FBE_PACKET_STACK_SIZE * sizeof(fbe_packet_stack_t));
	}
#endif

	/* Zero out the first 2 entries */
	fbe_zero_memory(packet->stack, 2 * sizeof(fbe_packet_stack_t));

	packet->magic_number = FBE_MAGIC_NUMBER_BASE_PACKET;

	/* The address header */
	packet->package_id = FBE_PACKAGE_ID_INVALID; 
	packet->service_id = FBE_SERVICE_ID_INVALID; 
	packet->class_id = FBE_CLASS_ID_INVALID;
	packet->object_id = FBE_OBJECT_ID_INVALID;
	/* End of address */

	packet->packet_status.code = FBE_STATUS_INVALID;
	packet->packet_status.qualifier = 0;
	packet->packet_attr = 0;

	/* packet->expiration_time = 0; *//* Zero means no expiration time. It is recommended to always set reasonable value */

	//fbe_atomic_exchange(&packet->packet_state, FBE_PACKET_STATE_IN_PROGRESS);
	packet->packet_state = FBE_PACKET_STATE_IN_PROGRESS;

	packet->base_edge = NULL;

    packet->packet_priority = FBE_PACKET_PRIORITY_NORMAL;
    packet->tag = FBE_TRANSPORT_TAG_INVALID;

	packet->packet_cancel_function = NULL;
	packet->packet_cancel_context = NULL;
	fbe_transport_init_expiration_time(packet);

	packet->cpu_id = FBE_CPU_ID_INVALID;

	packet->current_level = FBE_TRANSPORT_DEFAULT_LEVEL; /* To send the packet we need to get next stack location */

	/* Initialize queue elements so we can check if packet is queued or not */
	/* fbe_queue_element_init(&packet->initiator_queue_element);*/
	fbe_queue_element_init(&packet->queue_element);

#if 0
	fbe_semaphore_init(&packet->completion_semaphore, 0, FBE_PACKET_STACK_SIZE);
#endif

	/* Initialize subpacket area */
	packet->master_packet = NULL;
	fbe_queue_init(&packet->subpacket_queue_head);
	fbe_queue_element_init(&packet->subpacket_queue_element);
        
        /* This has a huge performance impact, but we need it to detect CSX resource queue corruption */
	packet->subpacket_count = 0;
    fbe_transport_packet_clear_callstack_depth(packet);
    packet->tracker_current_index = 0;
    packet->tracker_index_wrapped = 0;
        //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &packet->subpacket_queue_lock.lock);
        //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &packet->subpacket_queue_lock.lock, CSX_NULL);

	/*by default the packet load is normal*/
	packet->traffic_priority = FBE_TRAFFIC_PRIORITY_NORMAL;

    /* Initialize the resource priority and memory request */
    packet->resource_priority = 0;
    packet->saved_resource_priority = FBE_MEMORY_DPS_UNDEFINED_PRIORITY; 
    packet->resource_priority_boost = 0; /* Clear the flags */

    fbe_memory_request_init(&packet->memory_request);

    /* Initialize the I/O stamp */
    packet->io_stamp = FBE_PACKET_IO_STAMP_INVALID;
	fbe_payload_ex_init(&packet->payload_union.payload_ex);

    packet->physical_drive_service_time_ms = 0;
    packet->physical_drive_IO_time_stamp = 0; 
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_initialize_sep_packet(fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_transport_initialize_packet_common(packet);

    return status;
}

fbe_status_t 
fbe_transport_initialize_packet(fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_transport_initialize_packet_common(packet);

    return status;
}


/* This function does some cleanup, but do NOT release memory */
fbe_status_t 
fbe_transport_destroy_packet(fbe_packet_t * packet)
{
    fbe_atomic_t callstack_depth;
	/* We better to check if the queue is empty */
	if(!fbe_queue_is_empty(&packet->subpacket_queue_head)){
        /* Panic - Indicates the packet is still on the queue. */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: subpacket_queue is NOT empty\n", __FUNCTION__);
	}

	if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
			"Critical error: %s Invalid Packet (0x%p) Magic Number: %llX\n", 
			__FUNCTION__, packet,
			(unsigned long long)packet->magic_number);
	}

	if(fbe_queue_is_element_on_queue(&packet->queue_element)){
        /* Panic - Indicates the packet is still on the queue. */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: The packet is on the queue\n", __FUNCTION__);
	}

	fbe_queue_destroy(&packet->subpacket_queue_head);

	if(packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED){
		fbe_semaphore_destroy(&packet->completion_semaphore);
		packet->packet_attr &= ~FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED;
	}

	packet->subpacket_count = 0;
	callstack_depth = fbe_atomic_exchange(&max_packet_callstack_depth, packet->callstack_depth);
    if (callstack_depth > packet->callstack_depth)
    {
        fbe_atomic_exchange(&max_packet_callstack_depth, callstack_depth);
    }
    fbe_transport_packet_clear_callstack_depth(packet);
	packet->magic_number = FBE_MAGIC_NUMBER_DESTROYED_PACKET;

	return FBE_STATUS_OK;
}

/* We will implement various sanity checking before actual reuse */
fbe_status_t 
fbe_transport_reuse_packet(fbe_packet_t * packet)
{
    fbe_atomic_t callstack_depth;

    /* In case of a reuse packet, copy all non-zero packet stack
	 * to the top and zero out the non-zero portion. If the stack is full,
	 * only zero the first 2 entries.
	 */
	//fbe_transport_preserve_packet_stack(packet);
	/* Zero out the first 2 entries */
	fbe_zero_memory(packet->stack, 2 * sizeof(fbe_packet_stack_t));

	
	packet->magic_number = FBE_MAGIC_NUMBER_BASE_PACKET;

	/* The address header */
	packet->package_id = FBE_PACKAGE_ID_INVALID; 
	packet->service_id = FBE_SERVICE_ID_INVALID; 
	packet->class_id = FBE_CLASS_ID_INVALID;
	packet->object_id = FBE_OBJECT_ID_INVALID;
	/* End of address */

    packet->base_edge = NULL;

	packet->packet_status.code = FBE_STATUS_INVALID;
	packet->packet_status.qualifier = 0;
    if (packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED){ /* SAFEBUG - without this logic we trash live semaphore objects */
	    packet->packet_attr = FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED;
    } else {
	    packet->packet_attr = 0;
    }

//Peter P. needs to check    packet->traffic_priority = FBE_TRAFFIC_PRIORITY_NORMAL;
	packet->packet_priority = FBE_PACKET_PRIORITY_NORMAL;
	packet->tag = FBE_TRANSPORT_TAG_INVALID;

	packet->packet_cancel_function = NULL;
	packet->packet_cancel_context = NULL;
	/* packet->expiration_time = 0; *//* Zero means no expiration time. It is recommended to always set reasonable value */
//Peter P. needs to check    fbe_transport_init_expiration_time(packet);
//Peter P. needs to check    packet->cpu_id = FBE_CPU_ID_INVALID;
	packet->current_level = FBE_TRANSPORT_DEFAULT_LEVEL; /* To send the packet we need to get next stack location */

	packet->packet_state = FBE_PACKET_STATE_IN_PROGRESS;

	/* Initialize queue elements so we can check if packet is queued or not */
	/* fbe_queue_element_init(&packet->initiator_queue_element);*/
	fbe_queue_element_init(&packet->queue_element);

	/* Initialize subpacket area */
	packet->master_packet = NULL;
	fbe_queue_init(&packet->subpacket_queue_head);
	fbe_queue_element_init(&packet->subpacket_queue_element);

//Peter P. needs to check    packet->io_stamp = FBE_PACKET_IO_STAMP_INVALID;
	/* payload initialization */
	fbe_payload_ex_init(&packet->payload_union.payload_ex);

    /* Initialize the resource priority and memory request */
    packet->resource_priority = 0;
    packet->saved_resource_priority = FBE_MEMORY_DPS_UNDEFINED_PRIORITY; 
    packet->resource_priority_boost = 0; /* reset flags */


//Peter P. needs to check    packet->subpacket_count = 0;
	callstack_depth = fbe_atomic_exchange(&max_packet_callstack_depth, packet->callstack_depth);
    if (callstack_depth > packet->callstack_depth)
    {
        fbe_atomic_exchange(&max_packet_callstack_depth, callstack_depth);
    }
    fbe_transport_packet_clear_callstack_depth(packet);
    packet->tracker_current_index = 0;
    packet->tracker_index_wrapped = 0;

	if(packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED){
		fbe_semaphore_destroy(&packet->completion_semaphore);
		packet->packet_attr &= ~FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED;
	}

    fbe_memory_request_init(&packet->memory_request);

    packet->physical_drive_service_time_ms = 0;
    packet->physical_drive_IO_time_stamp = 0; 

// Peter P. needs to check
// NOTE We are not initing the following fields of the fbe_packet_t:
//   fbe_transport_timer_t        transport_timer;
//   fbe_semaphore_t              completion_semaphore;
// The stack[] is not inited for entries 2 through FBE_PACKET_STACK_SIZE-1
//   fbe_packet_stack_t           stack[FBE_PACKET_STACK_SIZE]; 
//   fbe_u8_t                     tracker_index[FBE_PACKET_STACK_SIZE];
//   fbe_packet_tracker_entry_t   tracker_ring[FBE_PACKET_TRACKER_DEPTH];


    return FBE_STATUS_OK;
}

static void 
fbe_transport_set_current_operation_status(fbe_packet_t * packet, fbe_status_t status_code)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_operation_header_t *    payload_operation_header = NULL;

    payload = fbe_transport_get_payload_ex(packet);
        if(payload->current_operation != NULL) {
            /* update the current operation status if it is not null. */
            payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;
            payload_operation_header->status = status_code;
        }
    }

fbe_status_t
fbe_transport_get_current_operation_status(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_operation_header_t *    payload_operation_header = NULL;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    payload_operation_header = (fbe_payload_operation_header_t *)payload->current_operation;
    status = payload_operation_header->status;
    return status;
}


fbe_status_t
fbe_transport_build_control_packet(fbe_packet_t * packet, 
									fbe_payload_control_operation_opcode_t				control_code,
									fbe_payload_control_buffer_t			buffer,
									fbe_payload_control_buffer_length_t	in_buffer_length,
									fbe_payload_control_buffer_length_t	out_buffer_length,
									fbe_packet_completion_function_t		completion_function,
									fbe_packet_completion_context_t			completion_context)
{
	fbe_status_t status;
	fbe_payload_control_buffer_length_t buffer_length;
	fbe_payload_ex_t  * payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL;

	if(completion_function != NULL){
		status = fbe_transport_set_completion_function(	packet, completion_function, completion_context);
		if(status != FBE_STATUS_OK){
			return status;
		}
	}

	/* Find maximum */
	buffer_length = in_buffer_length;
	if(buffer_length < out_buffer_length){
		buffer_length = out_buffer_length;
	}

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation(control_operation, control_code, buffer, buffer_length);

	return FBE_STATUS_OK;
}

fbe_payload_control_operation_opcode_t 
fbe_transport_get_control_code(fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL;
	fbe_payload_control_operation_opcode_t opcode;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);

	if(control_operation != NULL){
		fbe_payload_control_get_opcode(control_operation, &opcode);
		return opcode;
	} else {
		/* return fbe_io_block_get_control_code(&packet->io_block); */
		return FBE_PAYLOAD_CONTROL_OPERATION_OPCODE_INVALID; 
	}
}

fbe_status_t 
fbe_transport_increment_stack_level(fbe_packet_t * packet)
{
	if(packet->current_level + 1 >= FBE_PACKET_STACK_SIZE) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "Error: %s Stack exhausted, curr_level:0x%x, packet:0x%p\n", 
                                 __FUNCTION__, packet->current_level, packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			"Error: %s Invalid Packet (0x%p) Magic Number: %llX",
			__FUNCTION__, packet,
			(unsigned long long)packet->magic_number);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	packet->current_level++;

	/* Make sure the next one is within the limit and if so zero it out as well. */
	if(packet->current_level + 1 < FBE_PACKET_STACK_SIZE ) {
		/* Zero packet stack */
		fbe_zero_memory(&packet->stack[packet->current_level + 1], sizeof(fbe_packet_stack_t));
	}

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_transport_decrement_stack_level(fbe_packet_t * packet)
{
	if(packet->current_level < 1) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "error: %s No stack entry, level:0x%x, packet:0x%p\n", 
                                 __FUNCTION__, packet->current_level, packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			"Error: %s Invalid Packet (0x%p) Magic Number: %llX",
			__FUNCTION__, packet,
			(unsigned long long)packet->magic_number);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	packet->current_level--;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_enqueue_packet(fbe_packet_t * packet, fbe_queue_head_t * queue_head)
{
	fbe_packet_state_t old_state;
	fbe_queue_element_t	* queue_element = NULL;
	queue_element = fbe_transport_get_queue_element(packet);

    if (fbe_queue_is_element_on_queue(queue_element))
    {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			"error: %s packet is already on the queue", 
			__FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
    }

	if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			"error: %s Invalid Packet (0x%p) Magic Number: %llX", 
			__FUNCTION__, packet,
			(unsigned long long)packet->magic_number);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_queue_push(queue_head, queue_element);

	old_state = fbe_transport_exchange_state(packet, FBE_PACKET_STATE_QUEUED);
	
	switch(old_state) {
	case FBE_PACKET_STATE_IN_PROGRESS:
		break;
	case FBE_PACKET_STATE_CANCELED:
            if ((packet->packet_attr & FBE_PACKET_FLAG_DO_NOT_CANCEL) == 0)
            {
                if (fbe_transport_get_status_code(packet) != FBE_STATUS_CANCELED) 
                {
                fbe_transport_set_status(packet, FBE_STATUS_CANCEL_PENDING, 0);
                if (packet->packet_cancel_function != NULL)
                {
                    packet->packet_cancel_function(packet->packet_cancel_context);
                }
		}
		}
		break;
	default:
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s: Invalid packet state %X \n", __FUNCTION__, old_state);
	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_remove_packet_from_queue(fbe_packet_t * packet)
{
	fbe_packet_state_t old_state;
	fbe_queue_element_t	* queue_element = NULL;

	queue_element = fbe_transport_get_queue_element(packet);

	if(!fbe_queue_is_element_on_queue(queue_element)){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			                      "%s queue_element not on queue \n", __FUNCTION__);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			"Error: %s Invalid Packet (%p) Magic Number: %X", 
			__FUNCTION__, packet, (unsigned int)packet->magic_number);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_queue_remove(queue_element);

	old_state = fbe_transport_exchange_state(packet, FBE_PACKET_STATE_IN_PROGRESS);

	switch(old_state) {
	case FBE_PACKET_STATE_QUEUED:
        /* Clear cancel function */
        fbe_transport_set_cancel_function(packet, NULL, NULL);
		break;
    case FBE_PACKET_STATE_CANCELED:
        old_state = fbe_transport_exchange_state(packet, FBE_PACKET_STATE_CANCELED);
        if(old_state != FBE_PACKET_STATE_IN_PROGRESS){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s: Invalid old state: %d. Another thread may have changed it.\n", __FUNCTION__, old_state);
        }
	break;
	default:
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid packet state %X \n", __FUNCTION__, old_state);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

fbe_packet_t *
fbe_transport_dequeue_packet(fbe_queue_head_t * queue_head)
{
	fbe_packet_state_t old_state;
	fbe_queue_element_t	* queue_element = NULL;
	fbe_packet_t * packet = NULL;

	if(fbe_queue_is_empty(queue_head)){
		return NULL;
	}

	queue_element = fbe_queue_pop(queue_head);
	packet = fbe_transport_queue_element_to_packet(queue_element);

	if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
			FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
			"Error: %s Invalid Packet (%p) Magic Number: %X",
			__FUNCTION__, packet, (unsigned int)packet->magic_number);

		return NULL;
	}

	old_state = fbe_transport_exchange_state(packet, FBE_PACKET_STATE_IN_PROGRESS);

	switch(old_state) {
	case FBE_PACKET_STATE_QUEUED:
        /* Clear cancel function */
        fbe_transport_set_cancel_function(packet, NULL, NULL);
		break;
    case FBE_PACKET_STATE_CANCELED:
        old_state = fbe_transport_exchange_state(packet, FBE_PACKET_STATE_CANCELED);
        if(old_state != FBE_PACKET_STATE_IN_PROGRESS){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s: Invalid old state: %d. Another thread may have changed it.\n", __FUNCTION__, old_state);
        }
        break;
	default:
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid packet state %X \n", __FUNCTION__, old_state);
	}

	return packet;
}

static int fbe_transport_init_count = 0; /* SAFEBUG - this can get called twice in usersim.exe case - this is not a good fix, just a workaround */

fbe_status_t
fbe_transport_init(void)
{    
    if (fbe_transport_init_count++) {
        return FBE_STATUS_OK;
    }
    fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_packet_t) < FBE_MEMORY_CHUNK_SIZE);

    /* Make sure the chunk size is aligned to our alignment size.
     * We want all chunks to be aligned so packets are aligned. 
     * There are other places like the terminator that
     * check this at run time.
     */
    FBE_ASSERT_AT_COMPILE_TIME((FBE_MEMORY_CHUNK_SIZE % FBE_MEMORY_CHUNK_ALIGN_BYTES) == 0);
	fbe_transport_timer_init();
	fbe_transport_run_queue_init();
	fbe_multicore_spinlock_init(&fbe_transport_multicore_spinlock);
    
    max_packet_callstack_depth = 0;
	
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_destroy(void)
{
    if (--fbe_transport_init_count == 0) {
	fbe_transport_run_queue_destroy();	

        fbe_multicore_spinlock_destroy(&fbe_transport_multicore_spinlock);

	fbe_transport_timer_destroy();
    }
     
    fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s max callstack depth %d.\n", 
                            __FUNCTION__, (int)max_packet_callstack_depth);

    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_transport_check_packet_before_completion(fbe_packet_t * packet)
{
	fbe_queue_element_t	* queue_element = NULL;
	queue_element = fbe_transport_get_queue_element(packet);
	/* Something seriosly bad going on. Most likely we forgot to remove packet from terminator queue */
	if(!(packet->packet_attr & FBE_PACKET_FLAG_ENABLE_QUEUED_COMPLETION) && fbe_queue_is_element_on_queue(queue_element)){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s queue_element is on the queue. Packet (%p) \n", __FUNCTION__, packet);	
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

#define PACKET_CALLBACK_INDEX (packet->tracker_index[level])

fbe_status_t
fbe_transport_complete_packet(fbe_packet_t * packet)
{
	fbe_status_t status;
	volatile fbe_packet_level_t level = packet->current_level;	
    fbe_u32_t prev_tracker_index;
    fbe_u32_t current_timestamp;

	fbe_transport_check_packet_before_completion(packet);

	for( ; level >= 0; ) {
		if (packet->current_level != level) {
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "Error: %s Invalid Packet (%p) Current level: %d, Expected Level: %d \n", 
                           __FUNCTION__, packet, packet->current_level, level);

			return FBE_STATUS_GENERIC_FAILURE;
		}

		if(packet->magic_number != FBE_MAGIC_NUMBER_BASE_PACKET){
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
				FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
				"Error: %s Invalid Packet (%p) Magic Number: %X", 
				__FUNCTION__, packet, (unsigned int)packet->magic_number);

			return FBE_STATUS_GENERIC_FAILURE;
		}

		packet->current_level--;
		if(packet->stack[level].completion_function != NULL) {
            //callback_index = packet->tracker_index[level];
            /* make sure it hasn't wrapped */
            if ((packet->stack[level].completion_function==packet->tracker_ring[PACKET_CALLBACK_INDEX].callback_fn) &&
                ((packet->tracker_ring[PACKET_CALLBACK_INDEX].action & PACKET_STACK_COMPLETE) == 0))
            {
                current_timestamp = fbe_transport_get_coarse_time();
                if (current_timestamp > packet->tracker_ring[PACKET_CALLBACK_INDEX].coarse_time) 
                {
                    packet->tracker_ring[PACKET_CALLBACK_INDEX].coarse_time = current_timestamp - packet->tracker_ring[PACKET_CALLBACK_INDEX].coarse_time;
                }
                else
                {
                    packet->tracker_ring[PACKET_CALLBACK_INDEX].coarse_time = 0;
                }
                packet->tracker_ring[PACKET_CALLBACK_INDEX].action |= (PACKET_STACK_COMPLETE|PACKET_STACK_TIME_DELTA);

                if(packet->tracker_ring[PACKET_CALLBACK_INDEX].coarse_time > PACKET_LOGGING_TIME_THREADHOLD)
                {
                    if ((level + 1) < FBE_PACKET_STACK_SIZE) 
                    {
                        prev_tracker_index = packet->tracker_index[level + 1];
                        if ((packet->payload_union.payload_ex.sg_list != NULL) &&  /* we only care about IO */
                            (prev_tracker_index < FBE_PACKET_TRACKER_DEPTH) &&
                            ((packet->tracker_ring[prev_tracker_index].action & PACKET_STACK_TIME_DELTA) == PACKET_STACK_TIME_DELTA) &&
                            (packet->tracker_ring[prev_tracker_index].coarse_time < PACKET_LOGGING_TIME_THREADHOLD))
                        {
                            fbe_transport_log_bouncer(&packet->queue_element);
                        }
                    }
                }
            }
            else
            {
                /* if it wraps, let's at least record the time it completes */
                fbe_transport_record_callback_with_action(packet, packet->stack[level].completion_function,
                                                          PACKET_STACK_COMPLETE | PACKET_STACK_SET);
            }
			status = packet->stack[level].completion_function(packet, packet->stack[level].completion_context);
/* SAFEMESS - need a knob to enable this */
#if 0
    /* This has a huge performance impact, but we need it to detect CSX resource queue corruption */
    {
         csx_p_resource_list_validate(CSX_MY_MODULE_CONTEXT());
    }

#endif
			if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED) {
				break;
			}

			if(status == FBE_STATUS_CONTINUE){/* The new completion function was set */
				level = packet->current_level;
			} else {
				level--;
			}
		} else {
			level--;
		}
	}
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_edge_init(fbe_base_edge_t * base_edge)
{
	base_edge->next_edge = NULL;
	base_edge->hook = NULL;
	base_edge->client_id = FBE_OBJECT_ID_INVALID;
	base_edge->server_id = FBE_OBJECT_ID_INVALID;
	base_edge->client_index = FBE_EDGE_INDEX_INVALID;
	base_edge->server_index = FBE_EDGE_INDEX_INVALID;
	base_edge->path_state = FBE_PATH_STATE_INVALID;
	base_edge->path_attr = 0;
	base_edge->transport_id = FBE_TRANSPORT_ID_INVALID;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_edge_reinit(fbe_base_edge_t * base_edge)
{
	return fbe_base_edge_init(base_edge);
}

fbe_status_t 
fbe_transport_set_completion_function(	fbe_packet_t * packet,
										fbe_packet_completion_function_t completion_function,
										fbe_packet_completion_context_t  completion_context)
{
	if(packet->current_level >= FBE_PACKET_STACK_SIZE -1) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "Error: %s stack exhausted, curr_level:0x%x, packet:0x%p\n", 
                                 __FUNCTION__, packet->current_level, packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	packet->current_level++;
    if (packet->callstack_depth < packet->current_level) {
        packet->callstack_depth = packet->current_level;
    }
#if 0
    if (packet->callstack_depth>50) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "Error: %s callstack depth:0x%x, packet:0x%p\n", 
                                 __FUNCTION__, packet->callstack_depth, packet);
    }
#endif
	packet->stack[packet->current_level].completion_function = completion_function;
	packet->stack[packet->current_level].completion_context = completion_context;

    packet->tracker_index[packet->current_level] = packet->tracker_current_index;
    fbe_transport_record_callback(packet, completion_function);

	return FBE_STATUS_OK;
}

static __forceinline fbe_status_t fbe_transport_record_callback(fbe_packet_t * packet,
										fbe_packet_completion_function_t completion_function)
{
    packet->tracker_ring[packet->tracker_current_index].callback_fn = completion_function;
    packet->tracker_ring[packet->tracker_current_index].coarse_time = fbe_transport_get_coarse_time();
    packet->tracker_ring[packet->tracker_current_index].action = PACKET_STACK_SET;
    packet->tracker_current_index ++;
    if (packet->tracker_current_index>=FBE_PACKET_TRACKER_DEPTH)
    {
        packet->tracker_current_index = 0;
        packet->tracker_index_wrapped ++;
    }
    return FBE_STATUS_OK;
}

fbe_u32_t fbe_transport_get_coarse_time()
{
    if(fbe_transport_coarse_time!=0)
    {
        return fbe_transport_coarse_time;
    }
    else
    {
        /* api ? */
        return (fbe_u32_t) fbe_get_time();
    }

}
fbe_status_t fbe_transport_set_coarse_time(fbe_u32_t current_time)
{
    fbe_transport_coarse_time = current_time;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_sync_completion_type(fbe_packet_t * packet, fbe_transport_completion_type_t completion_type)
{

	packet->packet_attr |= FBE_PACKET_FLAG_SYNC;
#if 1
	if(!(packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED)){
		fbe_semaphore_init(&packet->completion_semaphore, 0, FBE_PACKET_STACK_SIZE);
		packet->packet_attr |= FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED;
	}
#endif
	fbe_transport_set_completion_function(packet, fbe_transport_sync_completion_function, (fbe_packet_completion_context_t)completion_type);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_transport_sync_completion_function(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_transport_completion_type_t completion_type = (fbe_transport_completion_type_t)context;

    fbe_semaphore_release(&packet->completion_semaphore, 0, 1, FALSE);
	if(completion_type == FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED){
		return FBE_STATUS_MORE_PROCESSING_REQUIRED;
	}

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_wait_completion(fbe_packet_t * packet)
{
	fbe_semaphore_wait(&packet->completion_semaphore, NULL);
	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_transport_wait_completion_ms(fbe_packet_t * packet, fbe_u32_t milliseconds)
{
    return fbe_semaphore_wait_ms(&packet->completion_semaphore, milliseconds);
}   


fbe_status_t 
fbe_transport_cancel_packet(fbe_packet_t * packet)
{
	fbe_packet_state_t old_state;

	old_state = fbe_transport_exchange_state(packet, FBE_PACKET_STATE_CANCELED);

    switch(old_state) {
	case FBE_PACKET_STATE_IN_PROGRESS:
    case FBE_PACKET_STATE_CANCELED:
		break;
	case FBE_PACKET_STATE_QUEUED:
        if ((packet->packet_attr & FBE_PACKET_FLAG_DO_NOT_CANCEL) == 0) {
		fbe_transport_set_status(packet, FBE_STATUS_CANCEL_PENDING, 0);
		if(packet->packet_cancel_function != NULL){
			packet->packet_cancel_function(packet->packet_cancel_context);
            }
		}
		break;
	default:
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Error: Invalid packet state %X \n", __FUNCTION__, old_state);
	}

	return FBE_STATUS_OK;
    
}

fbe_status_t 
fbe_transport_remove_subpacket(fbe_packet_t * base_subpacket)
{
	fbe_packet_t * packet = base_subpacket->master_packet;
    fbe_atomic_t subpacket_count;
    
	if(packet == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Critical error: %s packet %p; Master packet is NULL", 
                                __FUNCTION__, base_subpacket);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	subpacket_count = fbe_atomic_decrement(&packet->subpacket_count);

	if(subpacket_count != 0){ /* This operation allowed for one subpacket ONLY */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"Critical error: %s Packet %p: subpacket_count %lld ", 
                                __FUNCTION__, packet,
				(long long)subpacket_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_queue_remove(&base_subpacket->subpacket_queue_element);

	base_subpacket->master_packet = NULL;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_destroy_subpackets(fbe_packet_t * packet)
{
	fbe_queue_element_t * queue_element = NULL;
	fbe_packet_t * sub_packet = NULL;

	while(!fbe_queue_is_empty(&packet->subpacket_queue_head)){
		queue_element = fbe_queue_pop(&packet->subpacket_queue_head);
		sub_packet = fbe_transport_subpacket_queue_element_to_packet(queue_element);
		fbe_transport_destroy_packet(sub_packet);
	}

	return FBE_STATUS_OK;
}

/* There are cases where we want to remove the packet and check for an empty queue
 * under the lock. 
 * This is useful in cases where we have multiple things on the queue 
 * and we are using the empty queue as a trigger to complete 
 * the master packet. 
 */
fbe_status_t 
fbe_transport_remove_subpacket_is_queue_empty(fbe_packet_t * subpacket_p,
                                              fbe_bool_t *b_is_empty)
{
    fbe_packet_t * master_packet_p = subpacket_p->master_packet;
	fbe_atomic_t subpacket_count;

	if(master_packet_p == NULL){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Critical error: %s packet %p; Master packet is NULL", 
                                __FUNCTION__, subpacket_p);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    /* Reset the master_packet pointer to NULL before decreasing the count atomically */
	subpacket_p->master_packet = NULL;
	subpacket_count = fbe_atomic_decrement(&master_packet_p->subpacket_count);

	if(subpacket_count > 0){
		*b_is_empty = FBE_FALSE;
	} else {
		*b_is_empty = FBE_TRUE;
	}

	return FBE_STATUS_OK;
}

static fbe_status_t
fbe_transport_preserve_packet_stack(fbe_packet_t * packet)
{
	fbe_packet_level_t level;
	fbe_packet_level_t i, tmp_level;

	/* The for..loop is to check the last non-zero entry in the stack and
	 * store the largest depth to a global. 
	 */
	for(level = 0; level < FBE_PACKET_STACK_SIZE; level++) {
		if (packet->stack[level].completion_function == NULL) {
			/* Save the largest number to a global */
			if (level > packet_stack_max_level) {
				packet_stack_max_level = level - 1;
				break;
			}
		}
	}

	if (level < FBE_PACKET_STACK_SIZE){
		for(tmp_level = FBE_PACKET_STACK_SIZE - 1, i = level - 1; i >= 0; i--, tmp_level--) {
			/* Copy the non-zero packet stack to the top */
			packet->stack[tmp_level] = packet->stack[i];
		}
	}

	/* Zero out the first 2 entries */
	fbe_zero_memory(packet->stack, 2 * sizeof(fbe_packet_stack_t));

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_add_subpacket(fbe_packet_t * packet, fbe_packet_t * base_subpacket)
{
    fbe_status_t status;

    fbe_transport_set_io_stamp_from_master(packet, base_subpacket);

    if(packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_BY_PORT){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_COMPLETION_BY_PORT);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_MONITOR_OP){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_MONITOR_OP);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_REDIRECTED){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_REDIRECTED);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_CONSUMED){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_CONSUMED);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_DO_NOT_QUIESCE);
    }

    base_subpacket->packet_priority = packet->packet_priority;

    status  = fbe_transport_get_status_code(packet);

    if ( ((packet->packet_attr & FBE_PACKET_FLAG_DO_NOT_CANCEL) == 0) &&
         ((status == FBE_STATUS_CANCELED) || (status == FBE_STATUS_CANCEL_PENDING)) ){
        return status;
    } 

    fbe_queue_push(&packet->subpacket_queue_head, &base_subpacket->subpacket_queue_element);
    base_subpacket->master_packet = packet;
    if(packet->packet_attr & FBE_PACKET_FLAG_SYNC){
        base_subpacket->packet_attr |= FBE_PACKET_FLAG_SYNC;
    }

	/* We need to guarantee that subpacket has higher resource priority */
	if(base_subpacket->resource_priority <= packet->resource_priority){
		base_subpacket->resource_priority = packet->resource_priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
	}

	if(packet->payload_union.payload_ex.payload_memory_operation != NULL){
		base_subpacket->payload_union.payload_ex.payload_memory_operation = packet->payload_union.payload_ex.payload_memory_operation;
	}

	/* Propagate encryption key_handle */
	base_subpacket->payload_union.payload_ex.key_handle = packet->payload_union.payload_ex.key_handle;

    fbe_atomic_increment(&packet->subpacket_count);
    return FBE_STATUS_OK;
    
}

fbe_status_t 
fbe_transport_complete_packet_async(fbe_packet_t * packet)
{
	fbe_queue_head_t        tmp_queue;

    if (fbe_queue_is_element_on_queue(&packet->queue_element))
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "packet %p is on queue !!\n", packet);
    }
    /* initialize the queue. */
    fbe_queue_init(&tmp_queue);

    /* enqueue this packet temporary queue before we enqueue it to run queue. */
    fbe_queue_push(&tmp_queue, &packet->queue_element);

    /*!@note Queue this request to run queue to break the i/o context to avoid the stack
     * overflow, later this needs to be queued with the thread running in same core.
     */
    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
	fbe_queue_destroy(&tmp_queue);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_get_traffic_priority(fbe_packet_t * packet, fbe_traffic_priority_t *priority)
{
	*priority = packet->traffic_priority;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_traffic_priority(fbe_packet_t * packet, fbe_traffic_priority_t priority){
	packet->traffic_priority = priority;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_set_tag(fbe_packet_t * packet, fbe_u8_t tag)
{
    packet->tag = tag;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_get_tag(fbe_packet_t * packet, fbe_u8_t * tag)
{
    *tag = packet->tag;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_get_packet_priority(fbe_packet_t * packet, fbe_packet_priority_t * packet_priority)
{
    * packet_priority = packet->packet_priority;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_set_packet_priority(fbe_packet_t * packet, fbe_packet_priority_t packet_priority)
{
    packet->packet_priority = packet_priority;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_get_server_index(fbe_packet_t * packet, fbe_edge_index_t* server_index)
{
    fbe_base_edge_t * base_edge;

    base_edge = fbe_transport_get_edge(packet);

	/* Monitor packets do not have an edge.
		Later on we will learn client index form the permission event.
		But for now we will use 0 as it will fit all cases exept system drives.
	*/
	*server_index = 0;

	if(base_edge != NULL){
		fbe_base_transport_get_server_index(base_edge, server_index);
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_get_client_index(fbe_packet_t * packet, fbe_edge_index_t* client_index)
{
    fbe_base_edge_t * base_edge;

    base_edge = fbe_transport_get_edge(packet);

	/* Monitor packets do not have an edge.
		Later on we will learn client index form the permission event.
		But for now we will use 0 as it will fit all cases exept system drives.
	*/
	*client_index = 0;

	if(base_edge != NULL){
		fbe_base_transport_get_client_index(base_edge, client_index);
	}

    return FBE_STATUS_OK;
}

fbe_base_edge_t *
fbe_transport_get_edge(fbe_packet_t * packet)
{
    return packet->base_edge;
}


fbe_payload_block_operation_t *
fbe_transport_get_block_operation(fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_block_operation(payload);

    return block_operation_p;
}

fbe_status_t
fbe_transport_get_transport_id(fbe_packet_t * packet, fbe_transport_id_t * transport_id)
{
    *transport_id = packet->base_edge->transport_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_get_packet_attr(fbe_packet_t * packet, fbe_packet_attr_t * packet_attr)
{
    *packet_attr = packet->packet_attr;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_packet_attr(fbe_packet_t * packet, fbe_packet_attr_t packet_attr)
{
    packet->packet_attr |= packet_attr;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_clear_packet_attr(fbe_packet_t * packet, fbe_packet_attr_t packet_attr)
{
	packet->packet_attr &= ~packet_attr;
	return FBE_STATUS_OK;
}

fbe_packet_io_stamp_t
fbe_transport_get_io_stamp(fbe_packet_t * packet)
{
    return packet->io_stamp;
}

fbe_status_t
fbe_transport_set_io_stamp(fbe_packet_t * packet,
                          fbe_packet_io_stamp_t io_stamp)
{
	if(packet->cpu_id == FBE_CPU_ID_INVALID){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid cpu_id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    packet->io_stamp = (io_stamp & ~FBE_PACKET_IO_STAMP_MASK) | ((fbe_u64_t)(packet->cpu_id) << FBE_PACKET_IO_STAMP_SHIFT);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_io_stamp_from_master(fbe_packet_t * master_packet, fbe_packet_t * subpacket)
{
	subpacket->cpu_id = master_packet->cpu_id;
	subpacket->io_stamp = master_packet->io_stamp;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_np_lock_from_master(fbe_packet_t * master_packet, fbe_packet_t * subpacket)
{
	fbe_packet_attr_t          packet_attr;
    fbe_transport_get_packet_attr(master_packet, &packet_attr);
	if(packet_attr & FBE_PACKET_FLAG_NP_LOCK_HELD) 
	{		
		fbe_transport_set_packet_attr(subpacket, FBE_PACKET_FLAG_NP_LOCK_HELD);
	}
    return FBE_STATUS_OK;
}

fbe_packet_t *
fbe_transport_payload_to_packet(fbe_payload_ex_t * payload)
{
    fbe_packet_t * packet;
	packet = (fbe_packet_t  *)((fbe_u8_t *)payload - (fbe_u8_t *)(&((fbe_packet_t  *)0)->payload_union.payload_ex));
    return packet;
}

fbe_packet_resource_priority_t
fbe_transport_get_resource_priority(fbe_packet_t * packet)
{
	return packet->resource_priority;
}

void
fbe_transport_set_resource_priority(fbe_packet_t * packet,
                                    fbe_packet_resource_priority_t resource_priority)
{
	packet->resource_priority = resource_priority;
    return;
}

void fbe_transport_save_resource_priority(fbe_packet_t * packet)
{
    if(packet->saved_resource_priority == FBE_MEMORY_DPS_UNDEFINED_PRIORITY)
    {
        packet->saved_resource_priority = packet->resource_priority;
    }
    else
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Critical error: %s Packet (0x%p) already has saved Priority: %d", 
                                __FUNCTION__, packet, packet->saved_resource_priority);
    }
    return;
}

void fbe_transport_restore_resource_priority(fbe_packet_t * packet)
{
    if(packet->saved_resource_priority == FBE_MEMORY_DPS_UNDEFINED_PRIORITY)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Critical error: %s Packet (0x%p) has invalid saved Priority: %d", 
                                __FUNCTION__, packet, packet->saved_resource_priority);
    }
    else
    {
        packet->resource_priority = packet->saved_resource_priority;
        packet->saved_resource_priority = FBE_MEMORY_DPS_UNDEFINED_PRIORITY;
        packet->resource_priority_boost = 0; /* reset flags */
    }
    return;
}

fbe_status_t 
fbe_transport_set_resource_priority_from_master(fbe_packet_t * master_packet, fbe_packet_t * subpacket)
{
    fbe_packet_resource_priority_t master_packet_priority = fbe_transport_get_resource_priority(master_packet);

    fbe_transport_set_resource_priority(subpacket, master_packet_priority);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_unset_completion_function(fbe_packet_t * packet,
                                        fbe_packet_completion_function_t completion_function,
                                        fbe_packet_completion_context_t  completion_context)
{
    if ((packet->stack[packet->current_level].completion_function == completion_function) &&
        (packet->stack[packet->current_level].completion_context == completion_context)) {
        packet->stack[packet->current_level].completion_function = NULL;
        packet->stack[packet->current_level].completion_context = NULL;
        packet->current_level--;
    }
    return FBE_STATUS_OK;
}

fbe_memory_request_t * 
fbe_transport_get_memory_request(fbe_packet_t * packet)
{
    return &packet->memory_request;
}

fbe_packet_t * 
fbe_transport_get_master_packet(fbe_packet_t * packet)
{
    return packet->master_packet;
}

fbe_status_t 
fbe_transport_get_expiration_time(fbe_packet_t * packet, fbe_time_t *expiration_time)
{
    *expiration_time = packet->expiration_time;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_get_physical_drive_io_stamp(fbe_packet_t * packet, fbe_time_t *time_stamp)
{
    *time_stamp = packet->physical_drive_IO_time_stamp;
	return FBE_STATUS_OK;
}

fbe_bool_t 
fbe_transport_is_packet_expired(fbe_packet_t * packet)
{
	fbe_time_t current_time = 0;
	current_time = fbe_get_time();

    if ((packet->expiration_time != 0) &&
        (current_time >= packet->expiration_time))
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

/* This function sets the expiration time.  It does NOT start the timer */
fbe_status_t 
fbe_transport_set_expiration_time(fbe_packet_t * packet, fbe_time_t expiration_time)
{
    /* A expiration_time_interval of 0 means that there is no expiration time*/
    packet->expiration_time = expiration_time;
	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_transport_set_physical_drive_io_stamp(fbe_packet_t * packet, fbe_time_t io_time_stamp)
{
    packet->physical_drive_IO_time_stamp = io_time_stamp;
	return FBE_STATUS_OK;
}
/* This function propagates the expiration time from one packet to another */
fbe_status_t 
fbe_transport_propagate_expiration_time(fbe_packet_t *new_packet, fbe_packet_t *original_packet)
{
    /* Simply copy current expiration time from one packet to another */
    new_packet->expiration_time = original_packet->expiration_time;
	return FBE_STATUS_OK;
}
/* This function starts the expiration timer */
fbe_status_t 
fbe_transport_set_and_start_expiration_timer(fbe_packet_t * packet, fbe_time_t expiration_time_interval)
{
    fbe_time_t t = 0;
    t = fbe_get_time();

    /* A expiration_time_interval of 0 means that there is no expiration time*/
    if (expiration_time_interval == 0)
    {
        packet->expiration_time = 0;
    }
    else
    {
    packet->expiration_time = t + expiration_time_interval;
    }
    return FBE_STATUS_OK;
}


fbe_packet_t * 
fbe_transport_memory_request_to_packet(fbe_memory_request_t * memory_request)
{
    fbe_packet_t * packet;
    packet = (fbe_packet_t  *)((fbe_u8_t *)memory_request - (fbe_u8_t *)(&((fbe_packet_t  *)0)->memory_request));
    return packet;
}

fbe_packet_t * 
fbe_transport_subpacket_queue_element_to_packet(fbe_queue_element_t * queue_element)
{
    fbe_packet_t * packet;
    packet = (fbe_packet_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_packet_t  *)0)->subpacket_queue_element));
    return packet;
}

fbe_packet_t * 
fbe_transport_timer_to_packet(fbe_transport_timer_t * transport_timer)
{
	fbe_packet_t * packet;
	packet = (fbe_packet_t  *)((fbe_u8_t *)transport_timer - (fbe_u8_t *)(&((fbe_packet_t  *)0)->transport_timer));
	return packet;
}

fbe_queue_element_t * 
fbe_transport_get_queue_element(fbe_packet_t * packet)
{
    return &packet->queue_element;
}

/* This function initializes the expiration time */
fbe_status_t 
fbe_transport_init_expiration_time(fbe_packet_t * packet)
{
    /* A expiration_time_interval of 0 means that there is no expiration time*/
    packet->expiration_time = 0;
	return FBE_STATUS_OK;
}

/* Address related functions */
fbe_status_t 
fbe_transport_get_package_id(fbe_packet_t * packet, fbe_package_id_t * package_id)
{
    *package_id = packet->package_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_package_id(fbe_packet_t * packet, fbe_package_id_t package_id){
	packet->package_id = package_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_get_service_id(fbe_packet_t * packet, fbe_service_id_t * service_id)
{
    *service_id = packet->service_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_get_class_id(fbe_packet_t * packet, fbe_class_id_t * class_id)
{
    *class_id = packet->class_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_object_id(fbe_packet_t * packet, fbe_object_id_t object_id)
{
    packet->object_id = object_id;
    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_transport_get_object_id(fbe_packet_t * packet, fbe_object_id_t * object_id)
{
    *object_id = packet->object_id;
    return FBE_STATUS_OK;
}

/* Queueing related functions */
fbe_packet_t * 
fbe_transport_queue_element_to_packet(fbe_queue_element_t * queue_element)
{
    fbe_packet_t * packet;
    packet = (fbe_packet_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_packet_t  *)0)->queue_element));
    return packet;
}

fbe_status_t 
fbe_transport_set_cancel_function(  fbe_packet_t * packet, 
                                    fbe_packet_cancel_function_t packet_cancel_function,
                                    fbe_packet_cancel_context_t  packet_cancel_context)
{
    packet->packet_cancel_function = packet_cancel_function;
    packet->packet_cancel_context = packet_cancel_context;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_transport_cancel_packet(fbe_packet_t * packet);

fbe_status_t 
fbe_transport_copy_packet_status(fbe_packet_t * src, fbe_packet_t * dst)
{
    dst->packet_status.code = src->packet_status.code;
    dst->packet_status.qualifier = src->packet_status.qualifier;

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_transport_set_magic_number(fbe_packet_t * packet, fbe_u64_t magic_number)
{
    packet->magic_number = magic_number;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_address(fbe_packet_t * packet,
                            fbe_package_id_t package_id,
                            fbe_service_id_t service_id,
                            fbe_class_id_t   class_id,
                            fbe_object_id_t object_id)
{
    packet->package_id = package_id;
    packet->service_id = service_id;
    packet->class_id    = class_id;
    packet->object_id   = object_id;
    return FBE_STATUS_OK;
}

/* State related functions */
fbe_packet_state_t 
fbe_transport_exchange_state(fbe_packet_t * packet, fbe_packet_state_t new_state)
{
    return (fbe_packet_state_t)fbe_atomic_exchange(&packet->packet_state, new_state);
}

fbe_bool_t 
fbe_transport_is_packet_cancelled(fbe_packet_t * packet)
{
    fbe_bool_t b_status;

    /* The packet will only appear cancelled if the do not cancel flag is not set.
     */
    if ((packet->packet_state == FBE_PACKET_STATE_CANCELED) &&
        ((packet->packet_attr & FBE_PACKET_FLAG_DO_NOT_CANCEL) == 0))
    {
        b_status = FBE_TRUE;
    }
    else
    {
        b_status = FBE_FALSE;
    }
    return b_status;
}

fbe_status_t
fbe_base_transport_get_path_state(fbe_base_edge_t * base_edge, fbe_path_state_t * path_state)
{
    *path_state =  base_edge->path_state;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_path_attributes(fbe_base_edge_t * base_edge, fbe_path_attr_t * path_attr)
{
    *path_attr =  base_edge->path_attr;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_set_path_attributes(fbe_base_edge_t * base_edge, fbe_path_attr_t  path_attr)
{
    base_edge->path_attr |= path_attr;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_clear_path_attributes(fbe_base_edge_t * base_edge, fbe_path_attr_t  path_attr)
{
    base_edge->path_attr &= ~path_attr;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_set_server_id(fbe_base_edge_t * base_edge, fbe_object_id_t server_id)
{
    base_edge->server_id = server_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_server_id(fbe_base_edge_t * base_edge, fbe_object_id_t * server_id)
{
    *server_id = base_edge->server_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_set_server_index(fbe_base_edge_t * base_edge, fbe_edge_index_t server_index)
{
    base_edge->server_index = server_index;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_server_index(fbe_base_edge_t * base_edge, fbe_edge_index_t * server_index)
{
    *server_index = base_edge->server_index;
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_base_transport_set_client_id(fbe_base_edge_t * base_edge, fbe_object_id_t client_id)
{
    base_edge->client_id = client_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_client_id(fbe_base_edge_t * base_edge, fbe_object_id_t * client_id)
{
    *client_id = base_edge->client_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_set_client_index(fbe_base_edge_t * base_edge, fbe_edge_index_t client_index)
{
    base_edge->client_index = client_index;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_client_index(fbe_base_edge_t * base_edge, fbe_edge_index_t * client_index)
{
    *client_index = base_edge->client_index;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_set_transport_id(fbe_base_edge_t * base_edge, fbe_transport_id_t transport_id)
{
    base_edge->transport_id = transport_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_transport_id(fbe_base_edge_t * base_edge, fbe_transport_id_t * transport_id)
{
    * transport_id = base_edge->transport_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_set_hook_function(fbe_base_edge_t * base_edge, fbe_edge_hook_function_t hook)
{
    base_edge->hook = hook;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_remove_hook_function(fbe_base_edge_t *base_edge, fbe_packet_t *packet_p, fbe_payload_control_operation_opcode_t opcode)

{
    fbe_status_t    status;
    fbe_payload_ex_t *payload;
    fbe_payload_control_operation_t *control_operation;
    fbe_transport_control_remove_edge_tap_hook_t base_transport_remove_hook;
     
    if (base_edge->hook != (fbe_edge_hook_function_t)NULL)
    {
        payload = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_allocate_control_operation(payload);
        base_transport_remove_hook.object_id = base_edge->client_id;

        fbe_payload_control_build_operation(control_operation,  // control operation
                                    opcode,  // opcode
                                    &base_transport_remove_hook, // buffer
                                    sizeof(fbe_transport_control_remove_edge_tap_hook_t));   // buffer_length 
        fbe_payload_ex_increment_control_operation_level(payload);
        status = (base_edge->hook)(packet_p);
        fbe_payload_ex_release_control_operation(payload, control_operation);
        base_edge->hook = (fbe_edge_hook_function_t)NULL;
        return status;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_get_hook_function(fbe_base_edge_t * base_edge, fbe_edge_hook_function_t * hook)
{
    * hook = base_edge->hook;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_cpu_id(fbe_packet_t * packet, fbe_cpu_id_t cpu_id)
{
	packet->cpu_id = cpu_id;
	packet->io_stamp = (packet->io_stamp & ~FBE_PACKET_IO_STAMP_MASK) | ((fbe_u64_t)(packet->cpu_id) << FBE_PACKET_IO_STAMP_SHIFT);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_monitor_object_id(fbe_packet_t * packet, fbe_object_id_t object_id)
{
	packet->io_stamp = (packet->io_stamp & FBE_PACKET_IO_STAMP_MASK) | ((fbe_u64_t)object_id);
	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_transport_get_monitor_object_id(fbe_packet_t * packet, fbe_object_id_t *object_id_p)
{
	*object_id_p = (fbe_object_id_t)(packet->io_stamp & ~FBE_PACKET_IO_STAMP_MASK);
	return FBE_STATUS_OK;
}

fbe_bool_t fbe_transport_is_monitor_packet(fbe_packet_t *packet_p, fbe_object_id_t object_id)
{
    fbe_object_id_t packet_object_id;
    if ((packet_p->packet_attr & FBE_PACKET_FLAG_MONITOR_OP) == FBE_PACKET_FLAG_MONITOR_OP)
    {
        fbe_transport_get_monitor_object_id(packet_p, &packet_object_id);
        return (packet_object_id == object_id);
    }
    else
    {
        return FBE_FALSE;
    }
}

fbe_status_t 
fbe_transport_get_cpu_id(fbe_packet_t * packet, fbe_cpu_id_t * cpu_id)
{
	* cpu_id = packet->cpu_id;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_lock(fbe_packet_t * packet)
{
	if(packet->cpu_id == FBE_CPU_ID_INVALID){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid cpu_id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_multicore_spinlock_lock(&fbe_transport_multicore_spinlock, packet->cpu_id);
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_unlock(fbe_packet_t * packet)
{
	if(packet->cpu_id == FBE_CPU_ID_INVALID){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid cpu_id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_multicore_spinlock_unlock(&fbe_transport_multicore_spinlock, packet->cpu_id);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_set_master_packet(fbe_packet_t * packet, fbe_packet_t * base_subpacket)
{
    fbe_status_t status;

    fbe_transport_set_io_stamp_from_master(packet, base_subpacket);

    if(packet->packet_attr & FBE_PACKET_FLAG_COMPLETION_BY_PORT){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_COMPLETION_BY_PORT);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_MONITOR_OP){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_MONITOR_OP);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_REDIRECTED){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_REDIRECTED);
    }

    if(packet->packet_attr & FBE_PACKET_FLAG_CONSUMED){
        fbe_transport_set_packet_attr(base_subpacket, FBE_PACKET_FLAG_CONSUMED);
    }

    status  = fbe_transport_get_status_code(packet);


    base_subpacket->master_packet = packet;

	if(packet->packet_attr & FBE_PACKET_FLAG_SYNC){
        base_subpacket->packet_attr |= FBE_PACKET_FLAG_SYNC;
    }

	base_subpacket->payload_union.payload_ex.payload_memory_operation = packet->payload_union.payload_ex.payload_memory_operation;

	return FBE_STATUS_OK;    
}


fbe_transport_error_precedence_t 
fbe_transport_get_error_precedence(fbe_status_t status)
{
	fbe_transport_error_precedence_t error = FBE_TRANSPORT_ERROR_PRECEDENCE_NO_ERROR;

	switch(status) {
	case FBE_STATUS_OK:
		error = FBE_TRANSPORT_ERROR_PRECEDENCE_NO_ERROR;
		break;
	case FBE_STATUS_CANCELED:
		error = FBE_TRANSPORT_ERROR_PRECEDENCE_CANCELLED; /* User cancelled this I/O and going to get what he wanted */
		break;
	case FBE_STATUS_BUSY:
    case FBE_STATUS_EDGE_NOT_ENABLED:
    case FBE_STATUS_TIMEOUT:
        error = FBE_TRANSPORT_ERROR_PRECEDENCE_NOT_READY; /* MCC will retry it */
		break;
	case FBE_STATUS_FAILED:
		error = FBE_TRANSPORT_ERROR_PRECEDENCE_NOT_EXIST;
		break;
	default:
		error = FBE_TRANSPORT_ERROR_PRECEDENCE_UNKNOWN;
    }

	return error;
}

fbe_status_t
fbe_transport_update_master_status(fbe_status_t * master_status, fbe_status_t status)
{
	fbe_transport_error_precedence_t master_error;
	fbe_transport_error_precedence_t error;

	master_error = fbe_transport_get_error_precedence(*master_status);
	error = fbe_transport_get_error_precedence(status);

    if ((*master_status == (fbe_status_t)FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) || (error > master_error) ){
		*master_status = status;
    }

	return FBE_STATUS_OK;
}
