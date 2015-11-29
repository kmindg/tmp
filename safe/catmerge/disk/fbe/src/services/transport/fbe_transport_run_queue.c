#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe/fbe_sep_shim.h"
#include "fbe_trace.h"
#include "fbe_base.h"

typedef enum transport_run_queue_thread_flag_e{
    TRANSPORT_RUN_QUEUE_THREAD_RUN,
    TRANSPORT_RUN_QUEUE_THREAD_STOP,
    TRANSPORT_RUN_QUEUE_THREAD_DONE
}transport_run_queue_thread_flag_t;


static fbe_transport_external_run_queue_t fbe_transport_external_queue = NULL;
static fbe_transport_external_run_queue_t fbe_transport_external_run_queue_location = NULL;
static fbe_transport_external_run_queue_t fbe_transport_external_fast_queue_location = NULL;

//static fbe_spinlock_t		transport_run_queue_lock; 
//static fbe_queue_head_t	    transport_run_queue_head;

static fbe_thread_t			transport_run_queue_thread_handle[FBE_CPU_ID_MAX];
static transport_run_queue_thread_flag_t  transport_run_queue_thread_flag[FBE_CPU_ID_MAX];

static fbe_rendezvous_event_t transport_run_queue_event[FBE_CPU_ID_MAX];
static fbe_u64_t transport_run_queue_counter[FBE_CPU_ID_MAX];

static fbe_cpu_id_t transport_run_queue_cpu_count;
static fbe_multicore_queue_t transport_run_multicore_queue;
static fbe_u32_t transport_run_queue_current_depth[FBE_CPU_ID_MAX];
static fbe_u32_t transport_run_queue_max_depth[FBE_CPU_ID_MAX];
static fbe_u64_t transport_run_queue_hist_depth[FBE_CPU_ID_MAX][FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE];
static fbe_atomic_t transport_run_queue_io_counter[2] = {0, 0}; /* NUMA Megatron has two sockets */

/* Forward declaration */ 
static void transport_run_queue_thread_func(void * context);

static fbe_u32_t transport_run_batch_size = 12;
static fbe_u32_t transport_run_batch_watermark = 8;
static fbe_bool_t transport_run_batch_enable[2] = {0, 0}; /* Per CPU socket */

void
transport_run_queue_trace(fbe_trace_level_t trace_level,
                fbe_trace_message_id_t message_id,
                const char * fmt, ...)
                __attribute__((format(__printf_func__,3,4)));
void
transport_run_queue_trace(fbe_trace_level_t trace_level,
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

void fbe_transport_run_queue_set_external_handler(void (*handler) (fbe_queue_element_t *, fbe_cpu_id_t))
{
    fbe_transport_external_run_queue_location = handler;
    fbe_transport_enable_group_priority();
}

void fbe_transport_run_queue_get_external_handler(fbe_transport_external_run_queue_t * value)
{
    *value = fbe_transport_external_run_queue_location;
}

void fbe_transport_enable_group_priority(void)
{
    fbe_transport_external_queue = fbe_transport_external_run_queue_location;
}

void fbe_transport_disable_group_priority(void)
{
    fbe_transport_external_queue = NULL;
}

void fbe_transport_fast_queue_set_external_handler(void (*handler) (fbe_queue_element_t *, fbe_cpu_id_t))
{
    fbe_transport_external_fast_queue_location = handler;
    fbe_transport_enable_fast_priority();
}

void fbe_transport_fast_queue_get_external_handler(fbe_transport_external_run_queue_t * value)
{
    *value = fbe_transport_external_fast_queue_location;
}

void fbe_transport_enable_fast_priority(void)
{
    fbe_transport_external_queue = fbe_transport_external_fast_queue_location;
}

void fbe_transport_disable_fast_priority(void)
{
    fbe_transport_external_queue = NULL;
}

fbe_status_t 
fbe_transport_run_queue_init(void)
{
	fbe_cpu_id_t i;
	fbe_u32_t j;

	transport_run_queue_cpu_count = fbe_get_cpu_count();
       
	transport_run_queue_trace(FBE_TRACE_LEVEL_INFO, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s transport_run_queue_cpu_count = %d\n", __FUNCTION__, transport_run_queue_cpu_count);

	fbe_multicore_queue_init(&transport_run_multicore_queue);

    /* Start thread's */
	for(i = 0; i < transport_run_queue_cpu_count; i++){
		fbe_rendezvous_event_init(&transport_run_queue_event[i]);
		transport_run_queue_counter[i] = 0;

		transport_run_queue_current_depth[i] = 0;
		transport_run_queue_max_depth[i] = 0;
		for(j = 0; j < FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE; j++){
			transport_run_queue_hist_depth[i][j] = 0;
		}
		transport_run_queue_thread_flag[i] = TRANSPORT_RUN_QUEUE_THREAD_RUN;
		fbe_thread_init(&transport_run_queue_thread_handle[i], "fbe_trans_runq", transport_run_queue_thread_func, (void*)(fbe_ptrhld_t)i);
	}

	transport_run_queue_io_counter[0] = 0;
	transport_run_queue_io_counter[1] = 0;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_transport_run_queue_destroy(void)
{
	fbe_cpu_id_t i;

    transport_run_queue_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

	/* Stop thread's */
	for(i = 0; i < transport_run_queue_cpu_count; i++){
		transport_run_queue_thread_flag[i] = TRANSPORT_RUN_QUEUE_THREAD_STOP;
		fbe_rendezvous_event_set(&transport_run_queue_event[i]);
	}

    // notify all run queueu thread
	for(i = 0; i < transport_run_queue_cpu_count; i++){
		fbe_thread_wait(&transport_run_queue_thread_handle[i]);
		fbe_thread_destroy(&transport_run_queue_thread_handle[i]);

		fbe_rendezvous_event_destroy(&transport_run_queue_event[i]);
	}
	
	fbe_multicore_queue_destroy(&transport_run_multicore_queue);

    return FBE_STATUS_OK;
}

static void
fbe_transport_run_queue_push_queue_element(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id)
{
	if(cpu_id >= transport_run_queue_cpu_count){ /* Fix for AR 543043 Need to root cause */
        transport_run_queue_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Invalid cpu_id %llX\n", __FUNCTION__, (unsigned long long)cpu_id);
		cpu_id = 0;
	}

    if (fbe_transport_external_queue != NULL) {
          (*fbe_transport_external_queue)(queue_element, cpu_id);
    }
    else {
        fbe_multicore_queue_lock(&transport_run_multicore_queue, cpu_id);
        fbe_multicore_queue_push(&transport_run_multicore_queue, queue_element, cpu_id);
        fbe_rendezvous_event_set(&transport_run_queue_event[cpu_id]);
        transport_run_queue_counter[cpu_id]++;
        fbe_multicore_queue_unlock(&transport_run_multicore_queue, cpu_id);
    }
}

void
transport_run_queue_perform_callback(fbe_queue_element_t *queue_element, fbe_cpu_id_t thread_number)
{
    fbe_u64_t magic_number;
    fbe_memory_request_t * memory_request = NULL;
    fbe_raid_common_t *common_p = NULL;

    magic_number = *(fbe_u64_t *)(((fbe_u8_t *)queue_element) + sizeof(fbe_queue_element_t));
    if(magic_number == FBE_MAGIC_NUMBER_BASE_PACKET){
        fbe_packet_t * packet;
        fbe_status_t status;

        packet = fbe_transport_queue_element_to_packet(queue_element);
        status = fbe_transport_get_status_code(packet);
        /* We want the client to only see cancelled status. */
        if (status == FBE_STATUS_CANCEL_PENDING) {
            fbe_transport_set_status(packet, FBE_STATUS_CANCELED, 0);
        }

		if(packet->packet_attr & FBE_PACKET_FLAG_BVD){
			packet->packet_attr &= ~FBE_PACKET_FLAG_BVD;
			fbe_transport_set_cpu_id(packet, thread_number);
		}

        fbe_transport_complete_packet(packet);
    } else if(magic_number == FBE_MAGIC_NUMBER_MEMORY_REQUEST){
        memory_request = (fbe_memory_request_t *)queue_element;
        memory_request->completion_function( memory_request, memory_request->completion_context);
    } else if((magic_number == FBE_MAGIC_NUMBER_IOTS_REQUEST) ||
        (magic_number == FBE_MAGIC_NUMBER_SIOTS_REQUEST)){
            common_p = fbe_raid_common_wait_queue_element_to_common(queue_element);
            fbe_raid_common_start(common_p);
    } else {
        transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Invalid magic number %llX\n", __FUNCTION__, (unsigned long long)magic_number);
    }
}

static __forceinline void 
transport_run_queue_get_load_from_other_core(fbe_cpu_id_t thread_number, fbe_u32_t queue_depth, fbe_queue_head_t * tmp_queue)
{
	fbe_cpu_id_t i;
	fbe_cpu_id_t cpu = thread_number;
	fbe_u64_t counter;
	fbe_queue_element_t * queue_element = NULL;
	fbe_u32_t batch_size = transport_run_batch_size;
	
	/* If we woke up and there is nothing on our queue.*/
	if(queue_depth == 0){ 
		batch_size = transport_run_batch_size * 2; /* Let's boost the help to others */
	}

	counter = 0;

	/* Find busy CPU on that socket */
	if(thread_number > 7){ /* 8 - 15 */
		for(i = 8; i < FBE_MIN(16, transport_run_queue_cpu_count); i++){
			if((transport_run_queue_counter[i] > transport_run_batch_size) && /* This core has some staff to do */
				(counter < transport_run_queue_counter[i])){ /* Find the busiest one */
				counter = transport_run_queue_counter[i];
				cpu = i;
			}
		}
	} else { /* 0 - 7 */
		for(i = 0; i < FBE_MIN(8, transport_run_queue_cpu_count); i++){
			if((transport_run_queue_counter[i] > transport_run_batch_size) && /* This core has some staff to do */
				(counter < transport_run_queue_counter[i])){ /* Find the busiest one */
				counter = transport_run_queue_counter[i];
				cpu = i;
			}
		}
	}

	/* Take the request form the busiest core */
	if(counter > 0){
		fbe_multicore_queue_lock(&transport_run_multicore_queue, cpu);					
		while(queue_element = fbe_multicore_queue_pop(&transport_run_multicore_queue, cpu)){
			fbe_queue_push(tmp_queue, queue_element);
			transport_run_queue_counter[cpu]--;
			queue_depth++;
			if(queue_depth >= batch_size){
				break;
			}
		}
		fbe_multicore_queue_unlock(&transport_run_multicore_queue, cpu);					
	} 
}

static __forceinline void 
transport_run_queue_delegate_load_to_other_core(fbe_cpu_id_t thread_number)
{
	fbe_cpu_id_t i;
	fbe_cpu_id_t cpu;
	fbe_u64_t counter;

	counter = transport_run_batch_size;
	/* Find idle CPU on that socket */
	if(thread_number > 7){ /* 8 - 15 */
		for(i = 8; i < FBE_MIN(16, transport_run_queue_cpu_count); i++){
			if((transport_run_queue_counter[i] < transport_run_batch_watermark) && /* This core has some power */
				(counter > transport_run_queue_counter[i])){ /* Find the idle one */
				counter = transport_run_queue_counter[i];
				cpu = i;
			}
		}
	} else { /* 0 - 7 */
		for(i = 0; i < FBE_MIN(8, transport_run_queue_cpu_count); i++){
			if((transport_run_queue_counter[i] < transport_run_batch_watermark) && /* This core has some power */
				(counter > transport_run_queue_counter[i])){ /* Find the idle one */
				counter = transport_run_queue_counter[i];
				cpu = i;
			}
		}
	}
	/* Wake up the idle core core */
	if(counter < transport_run_batch_size){
		fbe_rendezvous_event_set(&transport_run_queue_event[cpu]);
	} else { /* All cores are busy */
		if(thread_number > 7){
			transport_run_batch_enable[1] = FBE_FALSE; 
		} else {
			transport_run_batch_enable[0] = FBE_FALSE; 
		}
	}
}

static void 
transport_run_queue_thread_func(void * context)
{
	fbe_cpu_id_t thread_number;
	fbe_cpu_affinity_mask_t affinity = 0x1;
	fbe_queue_head_t tmp_queue;
	fbe_queue_element_t * queue_element = NULL;
	fbe_u32_t queue_depth;
	fbe_bool_t batch_enabled;

	thread_number = (fbe_cpu_id_t)(csx_ptrhld_t)context;

	fbe_thread_set_affinity(&transport_run_queue_thread_handle[thread_number], 
							affinity << thread_number);

    transport_run_queue_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s %d entry \n", __FUNCTION__, thread_number);

	fbe_queue_init(&tmp_queue);
    while(1)    
	{		
		fbe_rendezvous_event_wait(&transport_run_queue_event[thread_number], 1000); /* Wake up every sec. */
		if(transport_run_queue_thread_flag[thread_number] == TRANSPORT_RUN_QUEUE_THREAD_RUN) {
			
			fbe_multicore_queue_lock(&transport_run_multicore_queue, thread_number);
			//fbe_rendezvous_event_clear(&transport_run_queue_event[thread_number]);
			queue_depth = 0;
			while(queue_element = fbe_multicore_queue_pop(&transport_run_multicore_queue, thread_number)){
				fbe_queue_push(&tmp_queue, queue_element);
                if (transport_run_queue_counter[thread_number] == 0){
					transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s queue %d corrupted\n", __FUNCTION__, thread_number);
                }
				transport_run_queue_counter[thread_number]--;
				queue_depth++;

				if((transport_run_batch_size != 0 ) && (queue_depth >= transport_run_batch_size)){
					break;
				}
			}

			if(queue_element == NULL){ /* We drained the queue */
				fbe_rendezvous_event_clear(&transport_run_queue_event[thread_number]);
			}

			transport_run_queue_current_depth[thread_number] = queue_depth;
			if(queue_depth > transport_run_queue_max_depth[thread_number]){
				transport_run_queue_max_depth[thread_number] = queue_depth;
			}
			if(queue_depth >= FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE){
				queue_depth = FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE - 1;
			}

			transport_run_queue_hist_depth[thread_number][queue_depth]++;			

			fbe_multicore_queue_unlock(&transport_run_multicore_queue, thread_number);

			/* Check if we busy or not */
			if((transport_run_batch_size != 0) && (queue_depth <= transport_run_batch_watermark)){ /* We can do something */
				if(thread_number > 7){
					transport_run_batch_enable[1] = FBE_TRUE; /* Not all cores are busy */
				} else {
					transport_run_batch_enable[0] = FBE_TRUE; /* Not all cores are busy */
				}
				transport_run_queue_get_load_from_other_core(thread_number, queue_depth, &tmp_queue);
			} /* We can do something */

			if(thread_number > 7){
				batch_enabled = transport_run_batch_enable[1];
			} else {
				batch_enabled = transport_run_batch_enable[0];
			}

			/* If we have more requests to process - let's try to delegate them to other cores in the same socket */
			if((transport_run_batch_size != 0) && (transport_run_queue_counter[thread_number] > 4) && batch_enabled){
				transport_run_queue_delegate_load_to_other_core(thread_number);
			}

			/* The actual processing */
			while(queue_element = fbe_queue_pop(&tmp_queue)){
                transport_run_queue_perform_callback(queue_element, thread_number);
			}
		} else {
			break;
		}
	}

	fbe_queue_destroy(&tmp_queue);

    transport_run_queue_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s %llu done\n", __FUNCTION__,
			      (unsigned long long)thread_number);

    transport_run_queue_thread_flag[thread_number] = TRANSPORT_RUN_QUEUE_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


fbe_status_t
fbe_transport_run_queue_push(fbe_queue_head_t * tmp_queue, 
							 fbe_packet_completion_function_t completion_function,
							 fbe_packet_completion_context_t  completion_context)
{
	fbe_packet_t * packet;
	fbe_cpu_id_t cpu_id;
	fbe_queue_element_t * queue_element;

	while(queue_element = fbe_queue_pop(tmp_queue)){
		packet = fbe_transport_queue_element_to_packet(queue_element);

        /* Debug code to add a tracker entry for queue push with no callback.
         * !!! Any changes such as adding/removing variables in this function
         * breaks the CALLER_ADDR_BYTE_OFFSET used below.
         */
#ifndef __GNUC__
        if(completion_function == NULL)
        {
            fbe_u64_t caller_address = 0;

            #ifdef SIMMODE_ENV
                #ifdef _DEBUG
                    #define CALLER_ADDR_BYTE_OFFSET    (0x10)
                #else
                    #define CALLER_ADDR_BYTE_OFFSET    (-0x20)
                #endif
            #else
                #ifdef _DEBUG
                    #define CALLER_ADDR_BYTE_OFFSET    (0x10)
                #else
                    #define CALLER_ADDR_BYTE_OFFSET    (-0x20)
                #endif
            #endif
            caller_address = ((fbe_u64_t) &caller_address) + CALLER_ADDR_BYTE_OFFSET;
            caller_address = *((fbe_u64_t *) (fbe_ptrhld_t)caller_address);
            fbe_transport_record_callback_with_action(packet, (fbe_packet_completion_function_t) (fbe_ptrhld_t)caller_address, PACKET_RUN_QUEUE_PUSH);
        }
#endif // _GNU_C        

		if(completion_function != NULL){
			fbe_transport_set_completion_function(packet, completion_function, completion_context);
		}
		fbe_transport_get_cpu_id(packet, &cpu_id);
		if(cpu_id == FBE_CPU_ID_INVALID){
			cpu_id = fbe_get_cpu_id();
            fbe_transport_set_cpu_id(packet, cpu_id);
            /* Downgrade the error report. Some control path may not set
             * cpuid, but be pushed to transport run queue.
             * This should not bring down the system.
             */
			transport_run_queue_trace(FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s Invalid cpu_id for packet: %p\n",
                                      __FUNCTION__, packet);
		}

		if(packet->packet_state == FBE_PACKET_STATE_QUEUED){
			transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s Invalid packet_state\n", __FUNCTION__);
		}

        fbe_transport_run_queue_push_queue_element(queue_element, cpu_id);
	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_run_queue_get_stats(void * in_stats)
{
	fbe_u32_t i;
	fbe_u32_t j;
	fbe_sep_shim_get_perf_stat_t *stats = (fbe_sep_shim_get_perf_stat_t *)in_stats;

	for(i = 0; i < transport_run_queue_cpu_count; i++){
		fbe_multicore_queue_lock(&transport_run_multicore_queue, i);
		stats->core_stat[i].runq_stats.transport_run_queue_current_depth = transport_run_queue_current_depth[i];
		stats->core_stat[i].runq_stats.transport_run_queue_max_depth = transport_run_queue_max_depth[i];
		for(j = 0; j < FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE; j++){
			stats->core_stat[i].runq_stats.transport_run_queue_hist_depth[j] = transport_run_queue_hist_depth[i][j];
		}
		fbe_multicore_queue_unlock(&transport_run_multicore_queue, i);
	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_run_queue_clear_stats(void)
{
	fbe_u32_t i;
	fbe_u32_t j;

	for(i = 0; i < transport_run_queue_cpu_count; i++){
		fbe_multicore_queue_lock(&transport_run_multicore_queue, i);
		transport_run_queue_current_depth[i] = 0;
		transport_run_queue_max_depth[i] = 0;
		for(j = 0; j < FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE; j++){
			transport_run_queue_hist_depth[i][j] = 0;
		}
		fbe_multicore_queue_unlock(&transport_run_multicore_queue, i);
	}
	return FBE_STATUS_OK;
}

fbe_status_t fbe_transport_run_queue_push_packet(fbe_packet_t * packet, fbe_transport_rq_method_t rq_method)
{
	fbe_cpu_id_t cpu_id;
	fbe_atomic_t counter;
	fbe_cpu_id_t current_cpu_id;

	fbe_transport_get_cpu_id(packet, &current_cpu_id);
	cpu_id = current_cpu_id;

	if(cpu_id == FBE_CPU_ID_INVALID){
		cpu_id = fbe_get_cpu_id();
		transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s Invalid cpu_id\n", __FUNCTION__);
	}

	if(packet->packet_state == FBE_PACKET_STATE_QUEUED){
		transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s Invalid packet_state\n", __FUNCTION__);
	}

	if(rq_method == FBE_TRANSPORT_RQ_METHOD_ROUND_ROBIN){
		if(current_cpu_id >= 8){ /* [Cores 8 - 15] Socket 1 */
			counter = fbe_atomic_increment(&transport_run_queue_io_counter[1]);
			counter = counter >> (rq_method - FBE_TRANSPORT_RQ_METHOD_ROUND_ROBIN);
			cpu_id = counter % 8; /* * cores per socket */
			cpu_id += 8;
		} else {
			counter = fbe_atomic_increment(&transport_run_queue_io_counter[0]);
			counter = counter >> (rq_method - FBE_TRANSPORT_RQ_METHOD_ROUND_ROBIN);
			cpu_id = counter % 8; /* * cores per socket */
		}
		fbe_transport_set_cpu_id(packet, cpu_id);
	}

	if(rq_method == FBE_TRANSPORT_RQ_METHOD_NEXT_CORE){
		cpu_id++;
		if(cpu_id >= transport_run_queue_cpu_count){
			cpu_id = 0;
		}
		fbe_transport_set_cpu_id(packet, cpu_id);
	}

    fbe_transport_run_queue_push_queue_element(&packet->queue_element, cpu_id);

	return FBE_STATUS_OK;
}

fbe_status_t fbe_transport_run_queue_push_request(fbe_memory_request_t * memory_request)
{
	fbe_cpu_id_t cpu_id;

	cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;


    fbe_transport_run_queue_push_queue_element(&memory_request->queue_element, cpu_id);

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_transport_run_queue_push_raid_request(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id)
{
    fbe_u64_t magic_number = FBE_MAGIC_NUMBER_INVALID;

    if(cpu_id == FBE_CPU_ID_INVALID)
    {
        transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s: Invalid cpu_id\n", __FUNCTION__);
    }

    magic_number = *(fbe_u64_t *)(((fbe_u8_t *)queue_element) + sizeof(fbe_queue_element_t));

    if((magic_number != FBE_MAGIC_NUMBER_IOTS_REQUEST) && 
       (magic_number != FBE_MAGIC_NUMBER_SIOTS_REQUEST))
    {
        transport_run_queue_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s: Invalid magic number.\n", __FUNCTION__);
    }

    fbe_transport_run_queue_push_queue_element(queue_element, cpu_id);

    return FBE_STATUS_OK;
}


