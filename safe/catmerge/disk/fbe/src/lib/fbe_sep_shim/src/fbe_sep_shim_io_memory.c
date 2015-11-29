#include "fbe_sep_shim_private_interface.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_multicore_queue.h"

/*local definitions*/
/* AR 498376: 
 *   For now increase the value from 1000 to 1100.  When DCA and queueing work starts, this value will be getting from CCR (cache)
 */

#define FBE_SEP_SHIM_MAX_ALLOCATED_STRUCTURES	1100


/*local varaibles*/
static fbe_sep_shim_perf_stat_per_core_t    fbe_sep_shim_stats_data[FBE_CPU_ID_MAX];
static fbe_sep_shim_perf_stat_per_core_t    fbe_sep_shim_wait_stats_data[FBE_CPU_ID_MAX];
static fbe_multicore_queue_t 				fbe_sep_shim_io_in_progress_queue;
static fbe_multicore_queue_t 				fbe_sep_shim_pre_allocated_ios_queue;
static fbe_cpu_id_t			 				fbe_sep_shim_cpu_count = 0;
static fbe_bool_t							fbe_sep_shim_stats_enabled = FBE_FALSE;


/*local function*/
static void fbe_sep_shim_increment_io_stats(fbe_cpu_id_t cpu_id);
static void fbe_sep_shim_decrement_io_stats(fbe_cpu_id_t cpu_id);

/*************************************************************************************************************************/
  
static fbe_bool_t fbe_sep_shim_shutdown_in_progress = FBE_FALSE;

void fbe_sep_shim_set_shutdown_in_progress(void)
{
    fbe_sep_shim_shutdown_in_progress = FBE_TRUE;
}

fbe_bool_t fbe_sep_shim_is_shutdown_in_progress(void)
{
    return fbe_sep_shim_shutdown_in_progress;
}

void fbe_sep_shim_init_io_memory(void)
{
    fbe_u32_t                   io_strcut_count = 0;
    fbe_sep_shim_io_struct_t *  sep_shim_io_struct = NULL;
    fbe_cpu_id_t                cpu_id = 0; 

    fbe_multicore_queue_init (&fbe_sep_shim_io_in_progress_queue);
    fbe_multicore_queue_init (&fbe_sep_shim_pre_allocated_ios_queue);
    fbe_sep_shim_cpu_count = fbe_get_cpu_count();

    /* Initialize the data structures that are used to track the requests
     * that are waiting for memory
     */
    fbe_sep_shim_init_waiting_request_data();
    for (cpu_id = 0; cpu_id < fbe_sep_shim_cpu_count; cpu_id++)
    {
        fbe_zero_memory(&fbe_sep_shim_stats_data[cpu_id], sizeof(fbe_sep_shim_perf_stat_per_core_t));
        fbe_zero_memory(&fbe_sep_shim_wait_stats_data[cpu_id], sizeof(fbe_sep_shim_perf_stat_per_core_t));
        for (io_strcut_count = 0; io_strcut_count < FBE_SEP_SHIM_MAX_ALLOCATED_STRUCTURES; io_strcut_count++)
        {
            sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)fbe_memory_ex_allocate(sizeof(fbe_sep_shim_io_struct_t));
            if (sep_shim_io_struct == NULL)
            {
                fbe_sep_shim_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s, can't allocate memory for IO: %u\n", __FUNCTION__, io_strcut_count);
            }
            else
            {
                sep_shim_io_struct->packet = NULL;
                sep_shim_io_struct->associated_io = NULL;
                sep_shim_io_struct->associated_device = NULL;
                fbe_queue_element_init(&sep_shim_io_struct->queue_element);
                fbe_multicore_queue_push(&fbe_sep_shim_pre_allocated_ios_queue, &sep_shim_io_struct->queue_element, cpu_id);
            }
        }
    }

}

void fbe_sep_shim_destroy_io_memory(void)
{
	fbe_u32_t					io_strcut_count = 0;
	fbe_cpu_id_t				cpu_id = 0;	
    fbe_sep_shim_io_struct_t *	sep_shim_io_struct = NULL;
	fbe_u32_t					delay_count = 0;
    
	for (cpu_id = 0; cpu_id < fbe_sep_shim_cpu_count; cpu_id++) {

		/*do we have ios in flight ?*/
		while (fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress != 0 && delay_count < 100) {
			if (delay_count == 0) {
				fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s %llu outstanding IOs when destroying !!\n", __FUNCTION__, (unsigned long long)fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress);
			}
			fbe_thread_delay(100);
			delay_count++;
		}

		if (delay_count == 100) {
			fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s %llu outstanding IOs after 10sec, we will leak memory !!\n", __FUNCTION__, (unsigned long long)fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress);
			continue;
		}

        for (io_strcut_count = 0; io_strcut_count < FBE_SEP_SHIM_MAX_ALLOCATED_STRUCTURES; io_strcut_count++) {
			sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)fbe_multicore_queue_pop(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
			fbe_memory_ex_release((void *)sep_shim_io_struct);
		}
	}

	fbe_multicore_queue_destroy (&fbe_sep_shim_io_in_progress_queue);
	fbe_multicore_queue_destroy (&fbe_sep_shim_pre_allocated_ios_queue);
	fbe_sep_shim_destroy_waiting_requests();
}

fbe_status_t fbe_sep_shim_get_io_structure(void *io_request, fbe_sep_shim_io_struct_t **io_struct)
{
    fbe_status_t                status = FBE_STATUS_INVALID;
	fbe_cpu_id_t 				cpu_id = fbe_get_cpu_id();
	fbe_bool_t			queue_empty = FBE_FALSE;

	/* Check if we have request that are already waiting for resources */
	queue_empty = fbe_sep_shim_is_waiting_request_queue_empty(cpu_id);
	if(!queue_empty) {
	    /* There are already request waiting and so enqueue this one */
	    status = fbe_sep_shim_enqueue_request(io_request, cpu_id);
        *io_struct = NULL;
	    return status;
	}

	*io_struct = fbe_sep_shim_process_io_structure_request(cpu_id, io_request);
	if(*io_struct == NULL) {

        /* There is no memory and so enqueue the request */
        status = fbe_sep_shim_enqueue_request(io_request, cpu_id);
	    return status;
	}

	return FBE_STATUS_OK;
}

fbe_sep_shim_io_struct_t * fbe_sep_shim_process_io_structure_request(fbe_cpu_id_t cpu_id, void *io_request)
{
    fbe_sep_shim_io_struct_t *sep_shim_io_struct;

    fbe_multicore_queue_lock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);

    if (fbe_sep_shim_is_shutdown_in_progress()){

        fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "sep shim: I/O %p received while shutdown in progress.\n", io_request);
        fbe_sep_shim_display_irp(io_request, FBE_TRACE_LEVEL_INFO);
        fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
        return NULL;
    } 
    if (!fbe_multicore_queue_is_empty(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id)) 
    {
        sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)fbe_multicore_queue_pop(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
        sep_shim_io_struct->core_take_from = cpu_id;
        sep_shim_io_struct->coarse_time = fbe_transport_get_coarse_time();
        sep_shim_io_struct->coarse_wait_time = sep_shim_io_struct->coarse_time;

        /*techincally we have a lock for this queue but since we always lock the queue of the packets in the return path as well, this is safe
        and will save us another spinlock for the pending queue*/
        fbe_multicore_queue_push(&fbe_sep_shim_io_in_progress_queue, &sep_shim_io_struct->queue_element, cpu_id);
    }
    else
    {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "*** %s out of pre allocated memory *** !!\n", __FUNCTION__);
        fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
        return NULL;
    }

    if (fbe_sep_shim_stats_enabled)
    {
        fbe_sep_shim_increment_io_stats(cpu_id);
    }

    fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
    return sep_shim_io_struct;

}
void fbe_sep_shim_return_io_structure(fbe_sep_shim_io_struct_t *io_struct)
{
//    fbe_memory_ptr_t memory_ptr = NULL;
    fbe_cpu_id_t    cpu_id = io_struct->core_take_from;

    if (io_struct->packet != NULL)
    {
        /* This is a debug check, and we should consider removing it for the product. */
        if (io_struct->packet->current_level != FBE_TRANSPORT_DEFAULT_LEVEL)
        {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s IO %p packet (%p) current level %d, not yet complete?\n", 
                               __FUNCTION__, io_struct->associated_io, io_struct->packet, io_struct->packet->current_level);
        }

        fbe_transport_destroy_packet(io_struct->packet);
        io_struct->packet = NULL;
    }

    if (io_struct->memory_request.ptr != NULL)
    {
        //fbe_memory_request_get_entry_mark_free(&io_struct->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&io_struct->memory_request);
        io_struct->memory_request.ptr = NULL;
    }

    if (io_struct->buffer.ptr != NULL)
    {
        fbe_memory_free_request_entry(&io_struct->buffer);
        io_struct->buffer.ptr = NULL;
    }
    io_struct->xor_mem_move_ptr = NULL;

    fbe_multicore_queue_lock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);

    fbe_queue_remove(&io_struct->queue_element);  // remove from IO in progress queue

    io_struct->associated_io = NULL;
    io_struct->associated_device = NULL;
    io_struct->flare_dca_arg = NULL;
    io_struct->dca_table = NULL;

    fbe_multicore_queue_push_front(&fbe_sep_shim_pre_allocated_ios_queue, &io_struct->queue_element, cpu_id);

    if (fbe_sep_shim_stats_enabled)
    {
        fbe_sep_shim_decrement_io_stats(cpu_id);
    }

    fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);

    /* If a shutdown is in progress do not wake up any waiting requests. */
    if (!fbe_sep_shim_is_shutdown_in_progress()              &&
        !fbe_sep_shim_is_waiting_request_queue_empty(cpu_id)    ) {
        /* There might be some resources waiting for requests wake them up */
        fbe_sep_shim_wake_waiting_request(cpu_id);
    }
}

/*!**************************************************************
 * fbe_sep_shim_get_perf_stat()
 ****************************************************************
 * @brief
 *  The function is used to get the performance stats and copy it to
 *  bvd performance stats structure.
 *
 * @param packet - The packet to return
 *        
 * @return none 
 *
 ****************************************************************/
fbe_status_t fbe_sep_shim_get_perf_stat(fbe_sep_shim_get_perf_stat_t *stat)
{
    fbe_copy_memory(&stat->core_stat, &fbe_sep_shim_stats_data, sizeof(fbe_sep_shim_perf_stat_per_core_t) * FBE_CPU_ID_MAX );
	stat->structures_per_core = FBE_SEP_SHIM_MAX_ALLOCATED_STRUCTURES;
    
    return FBE_STATUS_OK;
}

void fbe_sep_shim_perf_stat_enable(void)
{
	fbe_sep_shim_perf_stat_clear();
	fbe_sep_shim_stats_enabled = FBE_TRUE;
}

void fbe_sep_shim_perf_stat_disable(void)
{
	fbe_sep_shim_stats_enabled = FBE_FALSE;
}

void fbe_sep_shim_perf_stat_clear(void)
{
    fbe_cpu_id_t				cpu_id = 0;	

    for (cpu_id = 0; cpu_id < fbe_sep_shim_cpu_count; cpu_id++) {
		fbe_multicore_queue_lock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
		fbe_zero_memory(&fbe_sep_shim_stats_data[cpu_id], sizeof(fbe_sep_shim_perf_stat_per_core_t));
		fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
	}

}

/*should be called under the lock and only when stats is enabled*/
static void fbe_sep_shim_increment_io_stats(fbe_cpu_id_t cpu_id)
{
	/*! @todo: Make sure that all IOs (like PSM) 
	 *  are core affined to save locking and atomic operations here
	 */
	fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress++;
	fbe_sep_shim_stats_data[cpu_id].total_ios++;

	if (fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress > 
		fbe_sep_shim_stats_data[cpu_id].max_io_q_depth) {
		fbe_sep_shim_stats_data[cpu_id].max_io_q_depth = fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress;
	}
}

/*should be called under the lock and only when stats is enabled*/
static void fbe_sep_shim_decrement_io_stats(fbe_cpu_id_t cpu_id)
{
	/*! @todo: Make sure that all IOs (like PSM) 
	 *  are core affined to save locking and atomic operations here
	 */
	fbe_sep_shim_stats_data[cpu_id].current_ios_in_progress--;
}

/*should be called under the lock and only when stats is enabled*/
void fbe_sep_shim_increment_wait_stats(fbe_cpu_id_t cpu_id)
{
	/*! @todo: Make sure that all IOs (like PSM) 
	 *  are core affined to save locking and atomic operations here
	 */
	fbe_sep_shim_wait_stats_data[cpu_id].current_ios_in_progress++;
	fbe_sep_shim_wait_stats_data[cpu_id].total_ios++;

	if (fbe_sep_shim_wait_stats_data[cpu_id].current_ios_in_progress > 
		fbe_sep_shim_wait_stats_data[cpu_id].max_io_q_depth) {
		fbe_sep_shim_wait_stats_data[cpu_id].max_io_q_depth = fbe_sep_shim_wait_stats_data[cpu_id].current_ios_in_progress;
	}
}

/*should be called under the lock and only when stats is enabled*/
void fbe_sep_shim_decrement_wait_stats(fbe_cpu_id_t cpu_id)
{
	/*! @todo: Make sure that all IOs (like PSM) 
	 *  are core affined to save locking and atomic operations here
	 */
	fbe_sep_shim_wait_stats_data[cpu_id].current_ios_in_progress--;
}
fbe_bool_t fbe_sep_shim_is_io_drained(void)
{
    fbe_cpu_id_t cpu_id;
    fbe_u32_t cpu_count;
    cpu_count = fbe_get_cpu_count();

    for (cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
        fbe_multicore_queue_lock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id); 
        if (!fbe_multicore_queue_is_empty(&fbe_sep_shim_io_in_progress_queue, cpu_id)) {
            fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id);
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s cpu: %u still has I/Os. \n", __FUNCTION__, cpu_id);
            return FBE_FALSE; 
        }
        fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id); 
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s cpu: %u I/Os drained. \n", __FUNCTION__, cpu_id);
    }
    return FBE_TRUE; 
}

fbe_status_t fbe_sep_shim_shutdown_bvd(void)
{
    fbe_sep_shim_io_struct_t *sep_shim_io_struct;
    fbe_cpu_id_t cpu_id;
    fbe_u32_t cpu_index;
    fbe_u32_t cpu_count;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_bool_t drained = FBE_FALSE;
    fbe_u32_t wait_seconds = 0;
    fbe_u32_t io_count = 0;

    fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s starting \n", __FUNCTION__);
    /* Grab all locks across all cores.
     * This is needed since once we shut the door any individual core can 
     * check this shutdown flag under that per core lock. 
     */
    cpu_count = fbe_get_cpu_count();
    for (cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
        fbe_multicore_queue_lock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id); 
    }
    /* Shut the door.
     */
    fbe_sep_shim_set_shutdown_in_progress();

    /* Release the locks in the opposite order that we obtained them.
     */
    cpu_id = cpu_count - 1;
    for (cpu_index = 0; cpu_index < cpu_count; cpu_index++, cpu_id--) {
        fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id); 
    }
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s Marked shutdown in progress. \n", __FUNCTION__);

    /* Check all cores for in progress I/Os.
     */
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s Check for in progress I/Os. \n", __FUNCTION__);
    for (cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
        fbe_multicore_queue_lock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id); 
        if (!fbe_multicore_queue_is_empty(&fbe_sep_shim_io_in_progress_queue, cpu_id)) {
            queue_element_p = fbe_multicore_queue_front(&fbe_sep_shim_io_in_progress_queue, cpu_id);
            while (queue_element_p != NULL) {
                io_count++;
                sep_shim_io_struct = (fbe_sep_shim_io_struct_t *)queue_element_p;
                fbe_sep_shim_display_irp(sep_shim_io_struct->associated_io, FBE_TRACE_LEVEL_INFO);
                queue_element_p = fbe_multicore_queue_next(&fbe_sep_shim_io_in_progress_queue, queue_element_p, cpu_id);
            }
        }
        fbe_multicore_queue_unlock(&fbe_sep_shim_pre_allocated_ios_queue, cpu_id); 
    }
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s Check for in progress I/Os Complete. ios found: %u \n", __FUNCTION__, io_count);

    /* Wait for all cores to drain outstanding I/Os.
     */
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s Wait for I/O to Drain. \n", __FUNCTION__);
    drained = fbe_sep_shim_is_io_drained();
    while (!drained) {
        fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s IO is not drained yet, wait 1 sec. \n", __FUNCTION__);
        fbe_thread_delay(1000);
        wait_seconds += 1;
        if (wait_seconds >= (2 * 60)) {
            fbe_sep_shim_trace(FBE_TRACE_LEVEL_WARNING, "%s IO is not drained yet after 120 seconds. Failing shutdown\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        drained = fbe_sep_shim_is_io_drained(); 
    }
    fbe_sep_shim_trace(FBE_TRACE_LEVEL_INFO, "%s All I/O Drained. \n", __FUNCTION__);
    return FBE_STATUS_OK; 
}
