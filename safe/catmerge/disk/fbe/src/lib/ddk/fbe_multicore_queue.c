
#include "fbe/fbe_multicore_queue.h"
#include "fbe_trace.h"
#include "fbe_base.h"
#include "fbe/fbe_memory.h"

static fbe_cpu_id_t multicore_queue_cpu_count = FBE_CPU_ID_INVALID;


static void
multicore_queue_trace(fbe_trace_level_t trace_level,
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
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_MULTICORE_QUEUE,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

/* This function will initialize and allocate all resources */
fbe_status_t 
fbe_multicore_queue_init(fbe_multicore_queue_t * multicore_queue)
{
	fbe_cpu_id_t i;

	if(multicore_queue_cpu_count == FBE_CPU_ID_INVALID){
		multicore_queue_cpu_count = fbe_get_cpu_count();

		multicore_queue_trace(FBE_TRACE_LEVEL_INFO, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
								"%s multicore_queue_cpu_count = %d\n", __FUNCTION__, multicore_queue_cpu_count);

	}

	multicore_queue->entry = (fbe_multicore_queue_entry_t *)fbe_allocate_contiguous_memory(multicore_queue_cpu_count * sizeof(fbe_multicore_queue_entry_t));

	if(multicore_queue->entry == NULL){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Can't allocated %d bytes\n", __FUNCTION__, multicore_queue_cpu_count * sizeof(fbe_multicore_queue_entry_t));
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

	for(i = 0; i < multicore_queue_cpu_count; i++){
		fbe_queue_init(&multicore_queue->entry[i].head);
		fbe_spinlock_init(&multicore_queue->entry[i].lock);
	}


	return FBE_STATUS_OK;
}

/* This function will destroy and release all resources */
fbe_status_t 
fbe_multicore_queue_destroy(fbe_multicore_queue_t * multicore_queue)
{
	fbe_cpu_id_t i;

	if(multicore_queue_cpu_count == FBE_CPU_ID_INVALID){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s multicore_queue_cpu_count not initialized\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(multicore_queue->entry == NULL){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid parameter %p\n", __FUNCTION__, multicore_queue);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	for(i = 0; i < multicore_queue_cpu_count; i++){
		fbe_queue_destroy(&multicore_queue->entry[i].head);
		fbe_spinlock_destroy(&multicore_queue->entry[i].lock);
	}

	fbe_release_contiguous_memory(multicore_queue->entry);
	multicore_queue->entry = NULL;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_multicore_queue_push(fbe_multicore_queue_t * multicore_queue, 
						  fbe_queue_element_t * element, 
						  fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}
    if (fbe_queue_is_element_on_queue(element))
    {
		multicore_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s element %p already on the queue.\n", __FUNCTION__, element);
    }

	fbe_queue_push(&multicore_queue->entry[cpu_id].head, element);
	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_multicore_queue_push_front(fbe_multicore_queue_t * multicore_queue, 
								fbe_queue_element_t * element,
								fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_queue_push_front(&multicore_queue->entry[cpu_id].head, element);

	return FBE_STATUS_OK;
}

fbe_queue_element_t * 
fbe_multicore_queue_pop(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{
	fbe_queue_element_t * element;

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return NULL;
	}

	element = fbe_queue_pop(&multicore_queue->entry[cpu_id].head);

	return element;
}

fbe_bool_t 
fbe_multicore_queue_is_empty(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{
	fbe_bool_t  is_empty;

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return FBE_TRUE;
	}

	is_empty = fbe_queue_is_empty(&multicore_queue->entry[cpu_id].head);

	return is_empty;
}

fbe_queue_element_t * 
fbe_multicore_queue_front(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{
	fbe_queue_element_t * element;

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return NULL;
	}

	element = fbe_queue_front(&multicore_queue->entry[cpu_id].head);

	return element;
}


/* return next element or NULL for error */
fbe_queue_element_t * 
fbe_multicore_queue_next(fbe_multicore_queue_t * multicore_queue, 
											   fbe_queue_element_t * element, 
											   fbe_cpu_id_t cpu_id)
{
	fbe_queue_element_t * next_element;

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return NULL;
	}

	next_element = fbe_queue_next(&multicore_queue->entry[cpu_id].head, element);

	return next_element;
}

/* return next element or NULL for error */
fbe_queue_element_t * 
fbe_multicore_queue_prev(fbe_multicore_queue_t * multicore_queue, 
											   fbe_queue_element_t * element, 
											   fbe_cpu_id_t cpu_id)
{
	fbe_queue_element_t * prev_element;

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return NULL;
	}

	prev_element = fbe_queue_next(&multicore_queue->entry[cpu_id].head, element);

	return prev_element;
}


fbe_u32_t 
fbe_multicore_queue_length(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{
	fbe_u32_t length;

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return 0;
	}

	length = fbe_queue_length(&multicore_queue->entry[cpu_id].head);

	return length;
}

fbe_status_t 
fbe_multicore_queue_lock(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_lock(&multicore_queue->entry[cpu_id].lock);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_multicore_queue_unlock(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_unlock(&multicore_queue->entry[cpu_id].lock);
	return FBE_STATUS_OK;
}

fbe_queue_head_t * 
fbe_multicore_queue_head(fbe_multicore_queue_t * multicore_queue, fbe_cpu_id_t cpu_id)
{

	if(cpu_id >= multicore_queue_cpu_count){
		multicore_queue_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_queue_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_queue_cpu_count);

		return NULL;
	}

	return &multicore_queue->entry[cpu_id].head;
}
