
#include "fbe/fbe_multicore_spinlock.h"
#include "fbe_trace.h"
#include "fbe_base.h"
#include "fbe/fbe_memory.h"

static fbe_cpu_id_t multicore_spinlock_cpu_count = FBE_CPU_ID_INVALID;


static void
multicore_spinlock_trace(fbe_trace_level_t trace_level,
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
fbe_multicore_spinlock_init(fbe_multicore_spinlock_t * multicore_spinlock)
{
	fbe_cpu_id_t i;

	if(multicore_spinlock_cpu_count == FBE_CPU_ID_INVALID){
		multicore_spinlock_cpu_count = fbe_get_cpu_count();

		multicore_spinlock_trace(FBE_TRACE_LEVEL_INFO, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
								"%s multicore_spinlock_cpu_count = %d\n", __FUNCTION__, multicore_spinlock_cpu_count);

	}

	multicore_spinlock->entry = (fbe_multicore_spinlock_entry_t *)fbe_allocate_contiguous_memory(multicore_spinlock_cpu_count * sizeof(fbe_multicore_spinlock_entry_t));

	if(multicore_spinlock->entry == NULL){
		multicore_spinlock_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Can't allocated %d bytes\n", __FUNCTION__, multicore_spinlock_cpu_count * sizeof(fbe_multicore_spinlock_entry_t));
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

	for(i = 0; i < multicore_spinlock_cpu_count; i++){
		fbe_spinlock_init(&multicore_spinlock->entry[i].lock);
	}


	return FBE_STATUS_OK;
}

/* This function will destroy and release all resources */
fbe_status_t 
fbe_multicore_spinlock_destroy(fbe_multicore_spinlock_t * multicore_spinlock)
{
	fbe_cpu_id_t i;

	if(multicore_spinlock_cpu_count == FBE_CPU_ID_INVALID){
		multicore_spinlock_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s multicore_spinlock_cpu_count not initialized\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(multicore_spinlock->entry == NULL){
		multicore_spinlock_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid parameter %p\n", __FUNCTION__, multicore_spinlock);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	for(i = 0; i < multicore_spinlock_cpu_count; i++){
		fbe_spinlock_destroy(&multicore_spinlock->entry[i].lock);
	}

	fbe_release_contiguous_memory(multicore_spinlock->entry);
	multicore_spinlock->entry = NULL;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_multicore_spinlock_lock(fbe_multicore_spinlock_t * multicore_spinlock, fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= multicore_spinlock_cpu_count){
		multicore_spinlock_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_spinlock_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_spinlock_cpu_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_spinlock_lock(&multicore_spinlock->entry[cpu_id].lock);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_multicore_spinlock_unlock(fbe_multicore_spinlock_t * multicore_spinlock, fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= multicore_spinlock_cpu_count){
		multicore_spinlock_trace(FBE_TRACE_LEVEL_ERROR, 
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s cpu_id %d >= multicore_spinlock_cpu_count %d\n", __FUNCTION__, cpu_id, multicore_spinlock_cpu_count);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_unlock(&multicore_spinlock->entry[cpu_id].lock);
	return FBE_STATUS_OK;
}

