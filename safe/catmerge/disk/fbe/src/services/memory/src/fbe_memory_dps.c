
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe_base_service.h"
#include "fbe_base_object.h"
#include "fbe_memory_private.h"


typedef enum memory_dps_thread_flag_e{
    MEMORY_DPS_THREAD_RUN,
    MEMORY_DPS_THREAD_STOP,
    MEMORY_DPS_THREAD_DONE
}memory_dps_thread_flag_t;

static fbe_thread_t					memory_dps_thread_handle;
static fbe_rendezvous_event_t		memory_dps_event;
static memory_dps_thread_flag_t		memory_dps_thread_flag;

static fbe_bool_t					memory_dps_expanded = FBE_FALSE;

static fbe_memory_header_t * fbe_memory_dps_queue_element_to_header(fbe_queue_element_t * queue_element);

static fbe_queue_head_t fbe_memory_pool_allocation_head;

static fbe_bool_t memory_dps_size_reduced = FBE_FALSE; /* Skip sanity check during memory destroy */

typedef struct memory_dps_queue_s{
	fbe_queue_head_t	head;
	fbe_u64_t			number_of_chunks;
	fbe_atomic_t		number_of_free_chunks;
}memory_dps_queue_t;

typedef enum memory_dps_type_e{
	MEMORY_DPS_TYPE_CONTROL,
	MEMORY_DPS_TYPE_DATA,
	MEMORY_DPS_TYPE_LAST,
}memory_dps_type_t;

static fbe_spinlock_t     memory_dps_lock;
static memory_dps_queue_t memory_dps_queue[FBE_MEMORY_DPS_QUEUE_ID_LAST][MEMORY_DPS_TYPE_LAST];

static fbe_bool_t is_data_pool_initialized = FBE_FALSE;

enum {
	MEMORY_DPS_PRIORITY_MAX = 128
} memory_dps_constants_e;

static fbe_atomic_t priority_hist[MEMORY_DPS_PRIORITY_MAX];
static fbe_queue_head_t memory_dps_priority_queue[MEMORY_DPS_PRIORITY_MAX];

static memory_dps_queue_t * 
memory_get_dps_queue(fbe_memory_dps_queue_id_t queue_id, fbe_cpu_id_t cpu_id, memory_dps_type_t type)
{
	/* TODO Sanity checking */
	if(is_data_pool_initialized){
		return &memory_dps_queue[queue_id][type];
	} else {
		return &memory_dps_queue[queue_id][MEMORY_DPS_TYPE_CONTROL];
	}
}

/* Reserved pools */
static memory_dps_queue_t memory_dps_reserved_queue[FBE_MEMORY_DPS_QUEUE_ID_LAST][MEMORY_DPS_TYPE_LAST];

static memory_dps_queue_t * 
memory_get_dps_reserved_queue(fbe_memory_dps_queue_id_t queue_id, fbe_cpu_id_t cpu_id, memory_dps_type_t type)
{
	
	/* TODO Sanity checking */
	if(is_data_pool_initialized){
		return &memory_dps_reserved_queue[queue_id][type];
	} else {
		return &memory_dps_reserved_queue[queue_id][MEMORY_DPS_TYPE_CONTROL];
	}
}

/* Fast pools */
static fbe_spinlock_t     memory_dps_fast_lock[FBE_CPU_ID_MAX];
static memory_dps_queue_t memory_dps_fast_queue[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX][MEMORY_DPS_TYPE_LAST];
 
/* Allocation balance gate */
static fbe_bool_t memory_dps_fast_queue_balance_enable = FBE_TRUE;

static fbe_status_t fbe_memory_request_balance_entry(fbe_memory_request_t * memory_request);


static memory_dps_queue_t * 
memory_get_dps_fast_queue(fbe_memory_dps_queue_id_t queue_id, fbe_cpu_id_t cpu_id, memory_dps_type_t type)
{
	/* TODO Sanity checking */
	if(is_data_pool_initialized){
		return &memory_dps_fast_queue[queue_id][cpu_id][type];
	} else {
		return &memory_dps_fast_queue[queue_id][cpu_id][MEMORY_DPS_TYPE_CONTROL];
	}
}

static void memory_dps_fill_request_from_queue(fbe_memory_request_t * memory_request, memory_dps_queue_t * queue, fbe_bool_t is_data_queue);


static fbe_u32_t memory_reserved_pool_consumed_chunks_count[FBE_MEMORY_DPS_QUEUE_ID_LAST];

static fbe_u64_t            memory_dps_request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX][MEMORY_DPS_TYPE_LAST];
static fbe_u64_t            memory_dps_deferred_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX][MEMORY_DPS_TYPE_LAST];

//static fbe_queue_head_t					memory_dps_request_queue_head;
static fbe_atomic_t						memory_dps_request_queue_count = 0;
static fbe_memory_request_priority_t	memory_dps_request_queue_priority_max = 0; /* Max priority request on the queue */
static fbe_time_t						memory_dps_deadlock_time = 0;

static fbe_u64_t            memory_dps_request_aborted_count;
static fbe_bool_t           memory_dps_b_requested_aborted = FBE_FALSE;

static fbe_u64_t            memory_reserved_pool_max_allocated_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
static fbe_u64_t			memory_pool_max_allocation_size[FBE_MEMORY_DPS_QUEUE_ID_LAST];


static fbe_memory_dps_init_parameters_t memory_dps_number_of_chunks = {0, 0, 0, 0, 0};

static fbe_memory_dps_init_parameters_t memory_dps_number_of_data_chunks = {0, 0, 0, 0, 0};


/* Forward declarations */
static void memory_dps_thread_func(void * context);
static void memory_dps_dispatch_queue(void);
static void memory_dps_process_entry(fbe_memory_request_t * memory_request);

static fbe_status_t memory_dps_request_to_queue_id(fbe_memory_request_t * memory_request, 
													  fbe_memory_dps_queue_id_t * control_queue_id,
													  fbe_memory_dps_queue_id_t * data_queue_id);

static fbe_status_t memory_dps_queue_id_to_chunk_size(fbe_memory_dps_queue_id_t memory_dps_queue_id, fbe_memory_chunk_size_t * chunk_size);


static fbe_status_t fbe_memory_dps_add_memory_to_queues(memory_dps_type_t type);

static fbe_status_t fbe_memory_dps_add_memory_to_fast_queues(fbe_memory_dps_queue_id_t memory_dps_queue_id, 
															 fbe_u32_t number_of_chunks,
															 memory_dps_type_t type);
static fbe_cpu_id_t memory_dps_core_count;


static fbe_status_t fbe_memory_dps_add_memory_to_dps_queue(fbe_memory_dps_queue_id_t memory_dps_queue_id, 
														   fbe_u32_t number_of_chunks,
														    memory_dps_type_t type);

static fbe_status_t fbe_memory_dps_add_memory_to_reserved_queue(fbe_memory_dps_queue_id_t memory_dps_queue_id, 
																fbe_u32_t number_of_chunks,
																memory_dps_type_t type);

/* Statistics ONLY */
static fbe_u64_t memory_dps_fast_request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];
static fbe_u64_t memory_dps_fast_data_request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];

/* Max number of reserved I/O 's supported */
static fbe_u32_t FBE_MEMORY_RESERVED_IO_MAX = 0;

static fbe_u64_t fbe_memory_reserved_io_count = 0; /* How many reserved I/O s are in-flight now */
static fbe_memory_io_master_t * reserved_memory_io_master = NULL; 

static fbe_u64_t fbe_memory_deadlock_counter[FBE_CPU_ID_MAX];

static void memory_dps_process_reserved_request(fbe_memory_request_t * memory_request);


static fbe_memory_allocation_function_t memory_dps_allocation_function = NULL; /* SEP will reroute allocation to CMM */
static fbe_memory_release_function_t memory_dps_release_function = NULL; /* SEP will reroute release to CMM */

//static fbe_status_t fbe_memory_free_request_dc_entry(fbe_memory_request_t * memory_request);
static fbe_status_t fbe_memory_request_priority_dc_entry(fbe_memory_request_t * memory_request);




static fbe_bool_t memory_request_priority_is_allocation_allowed(fbe_memory_request_t * memory_request);
static fbe_bool_t memory_request_try_to_allocate(fbe_memory_request_t * memory_request);



fbe_status_t fbe_memory_dps_set_memory_functions(fbe_memory_allocation_function_t allocation_function,
                                                 fbe_memory_release_function_t release_function)
{
	memory_dps_allocation_function = allocation_function;
	memory_dps_release_function = release_function;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_dps_init_number_of_chunks(fbe_memory_dps_init_parameters_t *num_chunks_p)
{   
    fbe_u32_t number_of_chunks = 0;

	number_of_chunks += num_chunks_p->packet_number_of_chunks;
    number_of_chunks += num_chunks_p->block_64_number_of_chunks;
    number_of_chunks += num_chunks_p->block_1_number_of_chunks;
	number_of_chunks += num_chunks_p->fast_packet_number_of_chunks;
    number_of_chunks += num_chunks_p->fast_block_64_number_of_chunks;
    number_of_chunks += num_chunks_p->fast_block_1_number_of_chunks;
	number_of_chunks += num_chunks_p->reserved_packet_number_of_chunks;
    number_of_chunks += num_chunks_p->reserved_block_64_number_of_chunks;
    number_of_chunks += num_chunks_p->reserved_block_1_number_of_chunks;


	if (memory_dps_number_of_chunks.packet_number_of_chunks != 0) {/* Multiple initialization not supported */
		memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: user tries to set number of packet chunks to: %d, we already have: %d chunks\n", 
                             __FUNCTION__, 
                             num_chunks_p->packet_number_of_chunks, 
                             memory_dps_number_of_chunks.packet_number_of_chunks);

		return FBE_STATUS_OK;
	}

    if (memory_dps_number_of_chunks.block_64_number_of_chunks != 0) {/* Multiple initialization not supported */
		memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: user tries to set number of 64 block chunks to: %d, we already have: %d chunks\n", 
                             __FUNCTION__, 
                             num_chunks_p->block_64_number_of_chunks, 
                             memory_dps_number_of_chunks.block_64_number_of_chunks);

		return FBE_STATUS_OK;
	}

    if (memory_dps_number_of_chunks.block_1_number_of_chunks != 0) {/* Multiple initialization not supported */
		memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: user tries to set number of 1 block chunks to: %d, we already have: %d chunks\n", 
                             __FUNCTION__, 
                             num_chunks_p->block_1_number_of_chunks, 
                             memory_dps_number_of_chunks.block_1_number_of_chunks);

		return FBE_STATUS_OK;
	}

    /* The sum of individual chunk counts cannot be larger than the number of main chunks. */
    if (number_of_chunks > num_chunks_p->number_of_main_chunks)
    {
        memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: sum of packet and blocks is %d > number_of_main_chunks %d\n", 
                             __FUNCTION__, 
                             number_of_chunks, 
                             num_chunks_p->number_of_main_chunks);
    }

    memory_dps_number_of_chunks = *num_chunks_p;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_dps_init_number_of_data_chunks(fbe_memory_dps_init_parameters_t *num_chunks_p)
{   
    fbe_u32_t number_of_chunks = 0;

	number_of_chunks += num_chunks_p->packet_number_of_chunks;
    number_of_chunks += num_chunks_p->block_64_number_of_chunks;
	number_of_chunks += num_chunks_p->fast_packet_number_of_chunks;
    number_of_chunks += num_chunks_p->fast_block_64_number_of_chunks;
	number_of_chunks += num_chunks_p->reserved_packet_number_of_chunks;
    number_of_chunks += num_chunks_p->reserved_block_64_number_of_chunks;


	if (memory_dps_number_of_data_chunks.packet_number_of_chunks != 0) {/* Multiple initialization not supported */
		memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: user tries to set number of packet chunks to: %d, we already have: %d chunks\n", 
                             __FUNCTION__, 
                             num_chunks_p->packet_number_of_chunks, 
                             memory_dps_number_of_data_chunks.packet_number_of_chunks);

		return FBE_STATUS_OK;
	}

    if (memory_dps_number_of_data_chunks.block_64_number_of_chunks != 0) {/* Multiple initialization not supported */
		memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: user tries to set number of 64 block chunks to: %d, we already have: %d chunks\n", 
                             __FUNCTION__, 
                             num_chunks_p->block_64_number_of_chunks, 
                             memory_dps_number_of_data_chunks.block_64_number_of_chunks);

		return FBE_STATUS_OK;
	}

    /* The sum of individual chunk counts cannot be larger than the number of main chunks. */
    if (number_of_chunks > num_chunks_p->number_of_main_chunks)
    {
        memory_service_trace(FBE_TRACE_LEVEL_WARNING,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: sum of packet and blocks is %d > number_of_main_chunks %d\n", 
                             __FUNCTION__, 
                             number_of_chunks, 
                             num_chunks_p->number_of_main_chunks);
    }

    memory_dps_number_of_data_chunks = *num_chunks_p;

    memory_service_trace(FBE_TRACE_LEVEL_INFO,
	                     FBE_TRACE_MESSAGE_ID_INFO,
						 "DPS: data chunks: %d\n", memory_dps_number_of_data_chunks.number_of_main_chunks);

	is_data_pool_initialized = FBE_TRUE;
    return FBE_STATUS_OK;
}




static fbe_status_t fbe_memory_dps_allocate_main_pool(fbe_memory_dps_init_parameters_t * init_parameters, 
													  fbe_bool_t retry_forever, 
													  fbe_u32_t* chunks_added,
													  memory_dps_type_t type)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t main_pool_chunk_size = 0;	
	fbe_u32_t try_allocation_size;	
	fbe_u32_t try_chunk_count, allocated_chunk_count = 0;	
	fbe_u8_t * allocated_memory_ptr = NULL;
	fbe_memory_header_t * current_memory_header = NULL;
	fbe_u8_t * main_pool_ptr = NULL;
	fbe_u32_t i;
	fbe_u32_t chunks_added_to_main_pool;
	memory_dps_queue_t * dps_queue = NULL;
    fbe_u64_t try_bytes;

    chunks_added_to_main_pool = 0;

#if 0
    main_pool_chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO * FBE_MEMORY_DPS_64_BLOCKS_IO_PER_MAIN_CHUNK;
	
	/* Add headers */
	main_pool_chunk_size += FBE_MEMORY_DPS_64_BLOCKS_IO_PER_MAIN_CHUNK * 2 * FBE_MEMORY_DPS_HEADER_SIZE;
#endif
	main_pool_chunk_size = 1024 * 1024; /* 1 MB */

    memory_service_trace(FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_INFO,
					"main_pool_chunk_size %d\n", main_pool_chunk_size);
    try_chunk_count = init_parameters->number_of_main_chunks;
    try_bytes = ((fbe_u64_t) main_pool_chunk_size * (fbe_u64_t)try_chunk_count) + FBE_MEMORY_DPS_HEADER_SIZE;
    if (try_bytes > FBE_U32_MAX)
    {
        try_chunk_count = 1024; /* Limit to a Gig if we overflow */
    }

	while (allocated_chunk_count < init_parameters->number_of_main_chunks)
	{
		try_allocation_size = main_pool_chunk_size * try_chunk_count + FBE_MEMORY_DPS_HEADER_SIZE;
		if(init_parameters->memory_ptr == NULL){
			memory_service_trace(FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"MCRMEM: DPS %d type: %d\n", try_allocation_size, type);

			if(memory_dps_allocation_function == NULL){
				allocated_memory_ptr = fbe_memory_native_allocate(try_allocation_size);
			} else {
				allocated_memory_ptr = memory_dps_allocation_function(try_allocation_size);
			}	
		} else {
			allocated_memory_ptr = init_parameters->memory_ptr;
		}

		/* If we can't get all we wanted, we should try to reduce the size */
		if (allocated_memory_ptr == NULL) {
			try_chunk_count /= 2; 
			/* let's retry forever if we can't even get one block */
			if (try_chunk_count < 1) {
                memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: Can't allocate %d chunk!\n", __FUNCTION__, try_chunk_count);
                if (retry_forever == FBE_TRUE) {
                    try_chunk_count = 1;
                    memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: Can't allocate one chunk!\n", __FUNCTION__);
                }
                else {
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;  // break out of allocation loop
                }
			}
			continue;
		}
		allocated_chunk_count += try_chunk_count;

        fbe_spinlock_lock(&memory_dps_lock);
		/* Put it on the queue */
		current_memory_header = (fbe_memory_header_t *)allocated_memory_ptr;
		current_memory_header->magic_number = 0;
		if(init_parameters->memory_ptr == NULL){
			current_memory_header->memory_chunk_size = try_allocation_size; 
		} else {
			current_memory_header->memory_chunk_size = 0; /* We did not allocated the memory */
		}
		current_memory_header->number_of_chunks = try_chunk_count; 
		fbe_queue_push(&fbe_memory_pool_allocation_head, &current_memory_header->u.queue_element);
		main_pool_ptr = (fbe_u8_t *) allocated_memory_ptr + FBE_MEMORY_DPS_HEADER_SIZE;

		dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_MAIN, 0/* cpu */, type);

		/* Initialize main pool chunks */
		for(i = 0; i < try_chunk_count; i++){
			current_memory_header = (fbe_memory_header_t *)(main_pool_ptr + i * main_pool_chunk_size);
			current_memory_header->magic_number = 0;
			current_memory_header->memory_chunk_size = main_pool_chunk_size - FBE_MEMORY_DPS_HEADER_SIZE; 
			current_memory_header->number_of_chunks = 1; 
			fbe_queue_element_init(&current_memory_header->u.queue_element);
			current_memory_header->data = (fbe_u8_t *)current_memory_header + FBE_MEMORY_DPS_HEADER_SIZE;
			/* Put it on the queue */
			fbe_queue_push(&dps_queue->head,&current_memory_header->u.queue_element);
			dps_queue->number_of_chunks++;
			dps_queue->number_of_free_chunks++;
            chunks_added_to_main_pool ++;
		}
        fbe_spinlock_unlock(&memory_dps_lock);
	}

    *chunks_added = chunks_added_to_main_pool;
    return status;
}

fbe_status_t fbe_memory_dps_add_memory(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t package_id;
    fbe_u32_t chunks_added_to_main_pool;

    fbe_get_package_id(&package_id);

    if ((memory_dps_expanded == FBE_TRUE)||(package_id!=FBE_PACKAGE_ID_NEIT))
    {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: Can't add more memory for package %d!\n", __FUNCTION__, package_id);
    }
    else
    {
        memory_dps_expanded = FBE_TRUE;  // only expand once
        
        fbe_memory_dps_allocate_main_pool(&memory_dps_number_of_chunks, FBE_FALSE, &chunks_added_to_main_pool, MEMORY_DPS_TYPE_CONTROL);
        // we should consider return chunks_added_to_main_pool back to caller in the future
        memory_service_trace(FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: DPS add %d chunk more memory for package %d!\n", 
                                __FUNCTION__, chunks_added_to_main_pool, package_id);

        if (chunks_added_to_main_pool != 0)
        {
            status = FBE_STATUS_OK;
        }
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t
fbe_memory_dps_init(fbe_bool_t b_retry_forever)
{
    fbe_u32_t id, cpu, type;
    EMCPAL_STATUS       nt_status;
    fbe_u32_t chunks_added_to_main_pool;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;
    fbe_status_t  status;
    memory_dps_queue_t * dps_queue = NULL;
    fbe_memory_header_t * current_memory_header = NULL;
    fbe_queue_element_t * queue_element = NULL;
    fbe_u8_t * main_pool_ptr = NULL;

    fbe_get_package_id(&package_id);

    memory_dps_core_count = fbe_get_cpu_count();

	/* Initialize queues */	
	//fbe_queue_init(&memory_dps_request_queue_head);
    memory_dps_request_queue_count = 0;
    memory_dps_request_aborted_count = 0;
    memory_dps_b_requested_aborted = FBE_FALSE;  

    for(cpu = 0; cpu < FBE_CPU_ID_MAX; cpu++){
		fbe_spinlock_init(&memory_dps_fast_lock[cpu]);
        fbe_memory_deadlock_counter[cpu] = 0;
		for(id = 0; id < FBE_MEMORY_DPS_QUEUE_ID_LAST; id++){
			memory_dps_fast_request_count[id][cpu] = 0;
			memory_dps_fast_data_request_count[id][cpu] = 0;
			for(type = 0; type < MEMORY_DPS_TYPE_LAST; type ++){
				dps_queue = memory_get_dps_fast_queue(id, cpu, type);
		
				fbe_queue_init(&dps_queue->head);
				dps_queue->number_of_chunks = 0;
				dps_queue->number_of_free_chunks = 0;
                             
                memory_dps_deferred_count[id][cpu][type] = 0; 
                memory_dps_request_count[id][cpu][type] = 0;				
			}
        }
    }

	/* Initialize priority queues and statistics */
	for(id = 0; id < MEMORY_DPS_PRIORITY_MAX; id++){
		priority_hist[id] = 0;
		fbe_queue_init(&memory_dps_priority_queue[id]);
	}

	fbe_queue_init(&fbe_memory_pool_allocation_head);
	for(id = 0; id < FBE_MEMORY_DPS_QUEUE_ID_LAST; id++){
		for(type = 0; type < MEMORY_DPS_TYPE_LAST; type ++){
			dps_queue = memory_get_dps_queue(id, 0 /* cpu */, type);
			fbe_queue_init(&dps_queue->head);
			dps_queue->number_of_chunks = 0;
			dps_queue->number_of_free_chunks = 0;

			dps_queue = memory_get_dps_reserved_queue(id, 0 /* cpu */, type);
			fbe_queue_init(&dps_queue->head);
			dps_queue->number_of_chunks = 0;
			dps_queue->number_of_free_chunks = 0;
		}
		memory_reserved_pool_consumed_chunks_count[id] = 0;
		memory_reserved_pool_max_allocated_chunks[id] = 0;
		memory_pool_max_allocation_size[id] = 0;
	}

	fbe_memory_reserved_io_count = 0; 

	/* Initialize fast pools */
	//for(i = 0; i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++){
		//fbe_multicore_queue_init(&fbe_memory_fast_pool[i]);
	//}

	fbe_spinlock_init(&memory_dps_lock);

    status = fbe_memory_dps_allocate_main_pool(&memory_dps_number_of_chunks, b_retry_forever, &chunks_added_to_main_pool, MEMORY_DPS_TYPE_CONTROL);
	if(is_data_pool_initialized && (status == FBE_STATUS_OK)){
		status = fbe_memory_dps_allocate_main_pool(&memory_dps_number_of_data_chunks, b_retry_forever, &chunks_added_to_main_pool, MEMORY_DPS_TYPE_DATA);
	}

    if (status == FBE_STATUS_OK){
        fbe_memory_dps_add_memory_to_queues(MEMORY_DPS_TYPE_CONTROL);
        if(is_data_pool_initialized){
            fbe_memory_dps_add_memory_to_queues(MEMORY_DPS_TYPE_DATA);
        }

	memory_dps_thread_flag = MEMORY_DPS_THREAD_RUN;
    
	fbe_rendezvous_event_init(&memory_dps_event);

        /* Start memory thread */
        nt_status = fbe_thread_init(&memory_dps_thread_handle, "fbe_mem_dps", memory_dps_thread_func, NULL);
        if (nt_status != EMCPAL_STATUS_SUCCESS) {
	    memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: fbe_thread_init fail\n", __FUNCTION__);
	}
    }
    else
    {
        /* Destroy spinlock */
        fbe_spinlock_destroy(&memory_dps_lock);
        /* Free memory */
        queue_element = fbe_queue_pop(&fbe_memory_pool_allocation_head);
        while (queue_element != NULL) {
            current_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);
            main_pool_ptr = (fbe_u8_t *)current_memory_header;//fbe_memory_dps_queue_element_to_header(queue_element);
            if(current_memory_header->memory_chunk_size > 0){
                if(memory_dps_release_function == NULL){
                    fbe_memory_native_release(main_pool_ptr);
                } else {
                    memory_dps_release_function(main_pool_ptr);
                }
            }
            queue_element = fbe_queue_pop(&fbe_memory_pool_allocation_head);
        }
        /* Destroy fast pools */
        for(cpu = 0; cpu < FBE_CPU_ID_MAX; cpu++){
            fbe_spinlock_destroy(&memory_dps_fast_lock[cpu]);
        }
        /* Clear this out so we can initialize memory again. */
        fbe_zero_memory(&memory_dps_number_of_chunks, sizeof(fbe_memory_dps_init_parameters_t));
        fbe_zero_memory(&memory_dps_number_of_data_chunks, sizeof(fbe_memory_dps_init_parameters_t));
        is_data_pool_initialized = FBE_FALSE;
    }

	return status;
}

fbe_status_t
fbe_memory_dps_destroy(void)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t i, j, type;
	fbe_queue_element_t * queue_element = NULL;
	fbe_u8_t * main_pool_ptr = NULL;
    fbe_bool_t  b_chunks_in_use = FBE_FALSE;
    fbe_u64_t in_use_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
	fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;
	memory_dps_queue_t * dps_queue = NULL;
	fbe_memory_header_t * current_memory_header = NULL;
	fbe_u32_t priority_hist_max = 0;

    fbe_get_package_id(&package_id);
	if(package_id == FBE_PACKAGE_ID_SEP_0){
		for(i = 0; i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++){
			for(j = 0; j < memory_dps_core_count; j++){
				memory_dps_fast_request_count[i][j];
				memory_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
									FBE_TRACE_MESSAGE_ID_INFO,
									"MEMSTAT: fast queue_id %d core %d request_count %lld\n", 
									i, j, (long long)memory_dps_fast_request_count[i][j]);

			}
		}

		for(i = 0; i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++){
			memory_service_trace(FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"MEMSTAT: reserved_pool_max_allocated_chunks %d count  %lld\n", 
								i, (long long)memory_reserved_pool_max_allocated_chunks[i]);
		}
	}

    memory_dps_thread_flag = MEMORY_DPS_THREAD_STOP; 

	fbe_rendezvous_event_set(&memory_dps_event);

	fbe_thread_wait(&memory_dps_thread_handle);
	fbe_thread_destroy(&memory_dps_thread_handle);

	fbe_rendezvous_event_destroy(&memory_dps_event);

	/* Destroy queues */
	//fbe_queue_destroy(&memory_dps_request_queue_head);	

	for(type = 0; type < MEMORY_DPS_TYPE_LAST; type ++){
		for(i = 0; i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++){
			dps_queue = memory_get_dps_queue(i, 0 /* cpu */, type);
			fbe_queue_destroy(&dps_queue->head);
		}
	}

	fbe_spinlock_destroy(&memory_dps_lock);

    /*! @note If there are any memory requests outstanding
     *        don't free the memory.
     */

    FBE_ASSERT_AT_COMPILE_TIME(FBE_MEMORY_DPS_QUEUE_ID_MAIN < FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET);

	for(type = 0; type < MEMORY_DPS_TYPE_LAST; type ++){
		for(i = (FBE_MEMORY_DPS_QUEUE_ID_MAIN + 1); i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++){
			dps_queue = memory_get_dps_queue(i, 0 /* cpu */, type);

			in_use_chunks[i] = dps_queue->number_of_chunks - dps_queue->number_of_free_chunks;
			if (in_use_chunks[i] != 0 && memory_dps_size_reduced == FBE_FALSE){
				b_chunks_in_use = FBE_TRUE; 
			}
		}
	}

    /* If there are no outstanding memory requests free the memory */
    if (b_chunks_in_use == FBE_FALSE){
        queue_element = fbe_queue_pop(&fbe_memory_pool_allocation_head);
        while (queue_element != NULL) {
			current_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);
            main_pool_ptr = (fbe_u8_t *)current_memory_header;//fbe_memory_dps_queue_element_to_header(queue_element);			
			if(current_memory_header->memory_chunk_size > 0){
				if(memory_dps_release_function == NULL){
					fbe_memory_native_release(main_pool_ptr);
				} else {
					memory_dps_release_function(main_pool_ptr);
				}
			}
            queue_element = fbe_queue_pop(&fbe_memory_pool_allocation_head);
        } 
    }
    else
    {
        /* Else don't free the memory */
        status = FBE_STATUS_BUSY;
        
        memory_service_trace(FBE_TRACE_LEVEL_WARNING,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: in-use chunks for pool packet: %lld; 64-blk: %lld\n", __FUNCTION__,
                             in_use_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET], 
                             in_use_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO]);
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Leaking entire dps memory pool\n", __FUNCTION__);  

    }

	/* Destroy fast pools */
	for(i = 0; i < FBE_CPU_ID_MAX; i++){
		//fbe_multicore_queue_destroy(&fbe_memory_fast_pool[i]);
		fbe_spinlock_destroy(&memory_dps_fast_lock[i]);

	}

	for(i = 0; i < MEMORY_DPS_PRIORITY_MAX; i++){
		if(priority_hist[i] != 0){
			fbe_queue_destroy(&memory_dps_priority_queue[i]);
			memory_service_trace(FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"priority_hist[%d] = %lld\n", i, priority_hist[i]);
			if( i > priority_hist_max){
				priority_hist_max = i;
			}
		}
	}

		memory_service_trace(FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_INFO,
							"priority_hist_max = %d \n", priority_hist_max);


    /* Clear this out so we can initialize memory again. */
    fbe_zero_memory(&memory_dps_number_of_chunks, sizeof(fbe_memory_dps_init_parameters_t));
	fbe_zero_memory(&memory_dps_number_of_data_chunks, sizeof(fbe_memory_dps_init_parameters_t));
	is_data_pool_initialized = FBE_FALSE;
    return status;
}

fbe_status_t fbe_memory_request_init(fbe_memory_request_t * memory_request)
{
    memory_request->request_state = FBE_MEMORY_REQUEST_STATE_INITIALIZED;
    memory_request->ptr = NULL;
	memory_request->data_ptr = NULL;
    memory_request->io_stamp = FBE_MEMORY_REQUEST_IO_STAMP_INVALID;
	memory_request->memory_io_master = NULL;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_memory_request_is_build_chunk_complete(fbe_memory_request_t * memory_request)
{
    /* This check is only valid for the `allocate chunk' api */
	if (((memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK) ||
         (memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_SYNC)) &&
	    (memory_request->number_of_objects >= 1)                                   &&
        (memory_request->ptr == NULL)                                                 )
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

fbe_status_t 
fbe_memory_request_mark_aborted_complete(fbe_memory_request_t * memory_request)
{
    /* This is used to restart an aborted memory request that happene dto allocate before being aborted */
    if ((memory_request->request_state != FBE_MEMORY_REQUEST_STATE_ABORTED) ||
        (memory_request->ptr == NULL)                                          )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED;
    return FBE_STATUS_OK;
}

fbe_bool_t 
fbe_memory_request_is_in_use(fbe_memory_request_t * memory_request)
{
    /* This check is only valid for the `allocate chunk' api */
    if ((memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK)                       ||
        (memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY) || 
        (memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED)              || 
        (memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ABORTED))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

fbe_bool_t 
fbe_memory_request_is_allocation_complete(fbe_memory_request_t * memory_request)
{
    fbe_memory_request_state_t state = memory_request->request_state;

    /* This check is only valid for the `allocate chunk' api */
    if ((state != FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY) && 
        (state != FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED)              && 
        (state != FBE_MEMORY_REQUEST_STATE_ABORTED))
    {
		memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "dps: request: %p callback: %p ptr: %p state: %d bad\n", 
                             memory_request, memory_request->completion_function, 
							 memory_request->ptr, state);
    }

   if (memory_request->ptr != NULL || memory_request->data_ptr != NULL) {
       return FBE_TRUE;
   }

   return FBE_FALSE;
}

fbe_bool_t 
fbe_memory_request_is_immediate(fbe_memory_request_t * memory_request)
{
    /* This check is only valid for the `allocate chunk' api */
    if (memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

fbe_bool_t 
fbe_memory_request_is_aborted(fbe_memory_request_t * memory_request)
{

    /* Simply return if the memory request has been aborted or not */
    if (memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ABORTED)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

fbe_status_t fbe_memory_build_chunk_request(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_t number_of_objects,
                                            fbe_memory_request_priority_t priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context)
{
	fbe_cpu_id_t cpu_id = 0;

    if (fbe_memory_request_is_in_use(memory_request) == FBE_TRUE)
    {
        memory_request->chunk_size = 0;
		memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                    "%s: memory_request: 0x%p ptr: 0x%p state: %d is still in use\n", 
                             __FUNCTION__, memory_request, memory_request->ptr, memory_request->request_state);
		return FBE_STATUS_GENERIC_FAILURE;
    }
	fbe_queue_element_init(&memory_request->queue_element);
	memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK;
	memory_request->chunk_size = chunk_size;
	memory_request->number_of_objects = number_of_objects;
    memory_request->priority = priority;

	if(io_stamp == FBE_MEMORY_REQUEST_IO_STAMP_INVALID){
		cpu_id = fbe_get_cpu_id();
		memory_request->io_stamp = io_stamp | ((fbe_u64_t)cpu_id << FBE_PACKET_IO_STAMP_SHIFT);
	} else {
		memory_request->io_stamp = io_stamp;
	}

	memory_request->ptr = NULL;
	memory_request->data_ptr = NULL;
	memory_request->completion_function = completion_function;
	memory_request->completion_context = completion_context;

	memory_request->memory_io_master = NULL;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_memory_build_chunk_request_sync(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_t number_of_objects,
                                            fbe_memory_request_priority_t priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context)
{
	fbe_memory_build_chunk_request(memory_request, chunk_size, number_of_objects,
                                         priority, io_stamp, completion_function, completion_context);

	memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_SYNC;

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_abort_request(fbe_memory_request_t * in_memory_request)
{
    fbe_memory_request_t * memory_request;	
	fbe_memory_dps_queue_id_t memory_dps_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_u32_t i;

	memory_dps_request_to_queue_id(in_memory_request, &memory_dps_queue_id, &data_queue_id);

    fbe_spinlock_lock(&memory_dps_lock);

	for(i = 0; i < MEMORY_DPS_PRIORITY_MAX; i++){
		memory_request = (fbe_memory_request_t *)fbe_queue_front(&memory_dps_priority_queue[i]);
		while(memory_request != NULL) {
			if(memory_request == in_memory_request){
				/* If the request isn't aborted, mark it aborted, insert at front and signal dispatch */
				if (memory_request->request_state != FBE_MEMORY_REQUEST_STATE_ABORTED){
					memory_dps_request_aborted_count++;
                    memory_dps_b_requested_aborted = FBE_TRUE;
					memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ABORTED;
					//fbe_queue_remove(&memory_request->queue_element);
					//fbe_queue_push_front(&memory_dps_request_queue_head[memory_dps_queue_id], &memory_request->queue_element);
					fbe_rendezvous_event_set(&memory_dps_event);
				}
				fbe_spinlock_unlock(&memory_dps_lock);
				return FBE_STATUS_OK;
			}
			memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
		} /* while(memory_request != NULL) { */			
	} /* for(i = 0; i < MEMORY_DPS_PRIORITY_MAX; i++){ */

	fbe_spinlock_unlock(&memory_dps_lock);
    /* Is is possible that callback is in progress and therefore the request isn't on the queue */
	return FBE_STATUS_OK;
}

static fbe_memory_header_t *
fbe_memory_dps_queue_element_to_header(fbe_queue_element_t * queue_element)
{
	fbe_memory_header_t * memory_header;
	memory_header = (fbe_memory_header_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_memory_header_t  *)0)->u.queue_element));
	return memory_header;
}

fbe_memory_header_t *
fbe_memory_dps_chunk_queue_element_to_header(fbe_queue_element_t * queue_element)
{
	fbe_memory_header_t * memory_header;
	memory_header = (fbe_memory_header_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_memory_header_t  *)0)->queue_element));
	return memory_header;
}

static void memory_dps_thread_func(void * context)
{
	EMCPAL_STATUS nt_status;

	FBE_UNREFERENCED_PARAMETER(context);
	
	memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                     "%s entry\n", __FUNCTION__);

    while(1)    
	{
		nt_status = fbe_rendezvous_event_wait(&memory_dps_event, 200);
		//nt_status = fbe_semaphore_wait(&memory_dps_event, NULL);
		if(memory_dps_thread_flag == MEMORY_DPS_THREAD_RUN) {
			memory_dps_dispatch_queue();
		} else {
			break;
		}
    }

    memory_dps_thread_flag = MEMORY_DPS_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void 
memory_dps_process_entry(fbe_memory_request_t * memory_request)
{
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	memory_dps_queue_t * dps_queue = NULL;
	fbe_cpu_id_t cpu_id = 0;
	
	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;
	cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	if(number_of_objects_dc.split.data_objects > 0){
		dps_queue = memory_get_dps_queue(data_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_TRUE);
		if(memory_request->memory_io_master != NULL){		
			memory_request->memory_io_master->chunk_counter[data_queue_id] += number_of_objects_dc.split.data_objects;
		}
		
	} 
	if(number_of_objects_dc.split.control_objects > 0){
		dps_queue = memory_get_dps_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_FALSE);
		if(memory_request->memory_io_master != NULL){		
			memory_request->memory_io_master->chunk_counter[control_queue_id] += number_of_objects_dc.split.control_objects;
		}		
	}
}

fbe_status_t fbe_memory_dps_ctrl_data_to_chunk_size(fbe_memory_chunk_size_t ctrl_size,
                                                    fbe_memory_chunk_size_t data_size,
                                                    fbe_memory_chunk_size_t *combined_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(data_size) {
        case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
                if (ctrl_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET) {
                    *combined_p = FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET;
                }
                else if (ctrl_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO) {
                    *combined_p = FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS;
                }
                else {
                    memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s: Invalid ctrl_size = %d data_size = %d\n", __FUNCTION__, ctrl_size, data_size);
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                break;

        case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
                if (ctrl_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET) {
                    *combined_p = FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET;
                }
                else if (ctrl_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO) {
                    *combined_p = FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS;
                }
                else {
                    memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s: Invalid ctrl_size = %d data_size = %d\n", __FUNCTION__, ctrl_size, data_size);
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                break;
		default:
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: Invalid data_size = %d ctrl_size = %d\n", __FUNCTION__, data_size, ctrl_size);
			return FBE_STATUS_GENERIC_FAILURE;
	};
    return status;
}

fbe_status_t fbe_memory_dps_chunk_size_to_ctrl_bytes(fbe_memory_chunk_size_t chunk_size,
                                                     fbe_memory_chunk_size_t *bytes_p)
{
    switch(chunk_size) {
        case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
		case FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET:
		case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET:
            *bytes_p = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
			break;
		case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
		case FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS:
		case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS:
            *bytes_p = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
			break;
        case FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO:
            *bytes_p = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
			break;
		default:
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: Invalid chunk_size = %d \n", __FUNCTION__, chunk_size);
			return FBE_STATUS_GENERIC_FAILURE;
	}
    return FBE_STATUS_OK;
}
fbe_status_t fbe_memory_dps_chunk_size_to_data_bytes(fbe_memory_chunk_size_t chunk_size,
                                                     fbe_memory_chunk_size_t *bytes_p)
{
    switch(chunk_size) {
        case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
		case FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET:
		case FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS:
            *bytes_p = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
			break;
		case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
		case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET:
		case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS:
            *bytes_p = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
			break;
        case FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO:
            *bytes_p = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
			break;
		default:
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: Invalid chunk_size = %d \n", __FUNCTION__, chunk_size);
			return FBE_STATUS_GENERIC_FAILURE;
	}
    return FBE_STATUS_OK;
}
static fbe_status_t 
memory_dps_request_to_queue_id(fbe_memory_request_t * memory_request, 
													  fbe_memory_dps_queue_id_t * control_queue_id,
													  fbe_memory_dps_queue_id_t * data_queue_id)
{
	*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_INVALID;
	*data_queue_id = FBE_MEMORY_DPS_QUEUE_ID_INVALID;

	switch(memory_request->chunk_size) {
		case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
			break;
		case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
			break;
        case  FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO:
            *control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO;
			break;
		case FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET:
			*data_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
			break;
		case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET:
			*data_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
			break;
		case FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS:
			*data_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
			break;
		case FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS:
			*data_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
			break;
		default:
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: Invalid chunk_size = %d \n", __FUNCTION__, memory_request->chunk_size);
			return FBE_STATUS_GENERIC_FAILURE;
	}

	if(*data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){ /* New type of request */
		fbe_memory_number_of_objects_dc_t number_of_objects_dc;
		number_of_objects_dc.number_of_objects = memory_request->number_of_objects;
		if(number_of_objects_dc.split.data_objects == 0){			
			*data_queue_id = FBE_MEMORY_DPS_QUEUE_ID_INVALID;
		}

		if(number_of_objects_dc.split.control_objects == 0){
			*control_queue_id = FBE_MEMORY_DPS_QUEUE_ID_INVALID;
		}

	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_get_next_memory_header(fbe_memory_header_t * current_memory_header, fbe_memory_header_t ** next_memory_header)
{
    if(next_memory_header == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *next_memory_header = (fbe_memory_header_t *) current_memory_header->u.hptr.next_header;
    return FBE_STATUS_OK;
}

fbe_memory_header_t *
fbe_memory_get_first_memory_header(fbe_memory_request_t * memory_request_p)
{
    fbe_memory_header_t *master_memory_header = NULL;

    master_memory_header = memory_request_p->ptr;
    return(master_memory_header);
}
fbe_memory_header_t *
fbe_memory_get_first_data_memory_header(fbe_memory_request_t * memory_request_p)
{
    fbe_memory_header_t *master_memory_header = NULL;

    master_memory_header = memory_request_p->data_ptr;
    return(master_memory_header);
}

fbe_memory_hptr_t *
fbe_memory_get_next_memory_element(fbe_memory_hptr_t * current_memory_element)
{
    if (current_memory_element->next_header == NULL)
    {
        return NULL;
    }
    return(&((fbe_memory_header_t *)current_memory_element->next_header)->u.hptr);
}

fbe_u32_t
fbe_memory_get_queue_length(fbe_memory_request_t * memory_request_p)
{
    fbe_u32_t   queue_length = 0;
    fbe_memory_header_t *current_memory_header = NULL;

    current_memory_header = memory_request_p->ptr;
    while (current_memory_header != NULL)
    {
        queue_length++;
        current_memory_header = (fbe_memory_header_t *)current_memory_header->u.hptr.next_header;
    }
    return(queue_length);
}
fbe_u32_t
fbe_memory_get_data_queue_length(fbe_memory_request_t * memory_request_p)
{
    fbe_u32_t   queue_length = 0;
    fbe_memory_header_t *current_memory_header = NULL;

    current_memory_header = memory_request_p->data_ptr;
    while (current_memory_header != NULL)
    {
        queue_length++;
        current_memory_header = (fbe_memory_header_t *)current_memory_header->u.hptr.next_header;
    }
    return(queue_length);
}

fbe_u8_t *
fbe_memory_header_element_to_data(fbe_memory_hptr_t * queue_element)
{
	fbe_memory_header_t * memory_header;
	memory_header = (fbe_memory_header_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_memory_header_t  *)0)->u.hptr));
	return memory_header->data;
}

fbe_u8_t *
fbe_memory_header_to_data(fbe_memory_header_t * memory_header)
{
	return memory_header->data;
}

fbe_memory_header_t *
fbe_memory_header_data_to_master_header(fbe_u8_t * data)
{
    fbe_memory_header_t * memory_header;
    memory_header = (fbe_memory_header_t *) (data - (fbe_u8_t *)(&((fbe_memory_header_t  *)0)->data));
	return memory_header->u.hptr.master_header;
}

fbe_status_t fbe_memory_fill_dps_statistics(fbe_memory_dps_statistics_t * dps_pool_stats)
{
    fbe_u32_t                           pool_index = 0;
	fbe_u32_t							cpu_id;
	memory_dps_queue_t * dps_queue = NULL;
    
    for(pool_index = FBE_MEMORY_DPS_QUEUE_ID_MAIN; pool_index < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_index++) {
		dps_queue = memory_get_dps_queue(pool_index, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);

		dps_pool_stats->number_of_chunks[pool_index] = (fbe_u32_t)dps_queue->number_of_chunks;
		dps_pool_stats->number_of_free_chunks[pool_index]= (fbe_u32_t)dps_queue->number_of_free_chunks;

		dps_queue = memory_get_dps_queue(pool_index, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);

		dps_pool_stats->number_of_data_chunks[pool_index] = (fbe_u32_t)dps_queue->number_of_chunks;
		dps_pool_stats->number_of_free_data_chunks[pool_index]= (fbe_u32_t)dps_queue->number_of_free_chunks;

		/* How many requests are currently on the queue */
		dps_pool_stats->request_queue_count[pool_index] = memory_dps_request_queue_count;
    }

	for(pool_index = FBE_MEMORY_DPS_QUEUE_ID_MAIN; pool_index < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_index++) {
		for(cpu_id = 0; cpu_id < memory_dps_core_count; cpu_id++){
			dps_queue = memory_get_dps_fast_queue(pool_index, cpu_id, MEMORY_DPS_TYPE_CONTROL);

			dps_pool_stats->fast_pool_number_of_chunks[pool_index][cpu_id] = (fbe_u32_t)dps_queue->number_of_chunks;
			dps_pool_stats->fast_pool_number_of_free_chunks[pool_index][cpu_id] = (fbe_u32_t)dps_queue->number_of_free_chunks; /* Not accurate */

			dps_queue = memory_get_dps_fast_queue(pool_index, cpu_id, MEMORY_DPS_TYPE_DATA);
			dps_pool_stats->fast_pool_number_of_data_chunks[pool_index][cpu_id] = (fbe_u32_t)dps_queue->number_of_chunks;
			dps_pool_stats->fast_pool_number_of_free_data_chunks[pool_index][cpu_id] = (fbe_u32_t)dps_queue->number_of_free_chunks; /* Not accurate */

			dps_pool_stats->fast_pool_request_count[pool_index][cpu_id] = memory_dps_fast_request_count[pool_index][cpu_id];
			dps_pool_stats->fast_pool_data_request_count[pool_index][cpu_id] = memory_dps_fast_data_request_count[pool_index][cpu_id];

            /* How many requests we received */
            dps_pool_stats->request_count[pool_index][cpu_id] = memory_dps_request_count[pool_index][cpu_id][MEMORY_DPS_TYPE_CONTROL];
            dps_pool_stats->request_data_count[pool_index][cpu_id] = memory_dps_request_count[pool_index][cpu_id][MEMORY_DPS_TYPE_DATA];
                
            /* How many requests we deferred (queued) */
            dps_pool_stats->deferred_count[pool_index][cpu_id] = memory_dps_deferred_count[pool_index][cpu_id][MEMORY_DPS_TYPE_CONTROL];
            dps_pool_stats->deferred_data_count[pool_index][cpu_id] = memory_dps_deferred_count[pool_index][cpu_id][MEMORY_DPS_TYPE_DATA];
        }
	}
    for(cpu_id = 0; cpu_id < memory_dps_core_count; cpu_id++){
        dps_pool_stats->deadlock_count[cpu_id] = fbe_memory_deadlock_counter[cpu_id];
    }
    return FBE_STATUS_OK;
}

/* assuming the caller holds necessary spin lock */
static fbe_status_t fbe_memory_dps_add_memory_to_fast_queues(fbe_memory_dps_queue_id_t memory_dps_queue_id, 
															 fbe_u32_t number_of_chunks,
															 memory_dps_type_t type)
{
	fbe_memory_header_t * current_memory_header = NULL;
	fbe_memory_header_t * master_memory_header = NULL;
	fbe_queue_element_t * queue_element = NULL;
    fbe_u32_t  queue_memory_size = 0;
	fbe_u64_t  offset;
	fbe_u32_t i;
	fbe_cpu_id_t cpu_id;
	fbe_u32_t core_counter;
	memory_dps_queue_t * main_dps_queue = NULL;
	memory_dps_queue_t * dps_queue = NULL;

    switch (memory_dps_queue_id) {
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
        break;
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        break;
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
        break;
    default:
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						    FBE_TRACE_MESSAGE_ID_INFO,
						    "%s: invalid queue id %d !\n", __FUNCTION__, memory_dps_queue_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	main_dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_MAIN, 0 /* cpu */, type);
	core_counter = 0;
	for(i = 0; i < number_of_chunks; i++){
		queue_element = fbe_queue_pop(&main_dps_queue->head);
		if (queue_element == NULL) {
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: main pool out of memory !\n", __FUNCTION__);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		main_dps_queue->number_of_free_chunks--;
		master_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);
		master_memory_header->magic_number = FBE_MEMORY_HEADER_MAGIC_NUMBER; /* It is allocated */

		offset = 0;
		while(offset + queue_memory_size + FBE_MEMORY_DPS_HEADER_SIZE <=  master_memory_header->memory_chunk_size){
			current_memory_header = (fbe_memory_header_t *)(master_memory_header->data + offset);
			current_memory_header->magic_number = 0;
			current_memory_header->memory_chunk_size = queue_memory_size; 
			current_memory_header->number_of_chunks = 1; 
			fbe_queue_element_init(&current_memory_header->u.queue_element);
			current_memory_header->data = (fbe_u8_t *)current_memory_header + FBE_MEMORY_DPS_HEADER_SIZE;
			/* Put it on the queue */
			//fbe_queue_push(&memory_dps_queue[memory_dps_queue_id].head,&current_memory_header->u.queue_element);
			cpu_id = core_counter % memory_dps_core_count;
			core_counter++;

			dps_queue = memory_get_dps_fast_queue(memory_dps_queue_id, cpu_id, type);

			fbe_queue_push(&dps_queue->head, &current_memory_header->u.queue_element);
			dps_queue->number_of_chunks++;
			dps_queue->number_of_free_chunks++;
			offset += queue_memory_size + FBE_MEMORY_DPS_HEADER_SIZE;
		}
	}/* for(i = 0; i < number_of_chunks; i++) */

	/* Trace how much memory we got */
	for(i = 0; i < memory_dps_core_count; i++){
		dps_queue = memory_get_dps_fast_queue(memory_dps_queue_id, i, MEMORY_DPS_TYPE_CONTROL);
		memory_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
							FBE_TRACE_MESSAGE_ID_INFO,
							"Fast pool size %d core %d chunks %lld\n", queue_memory_size, i, dps_queue->number_of_chunks);

	}
    return FBE_STATUS_OK;
}

/* assuming the caller holds necessary spin lock */
static fbe_status_t fbe_memory_dps_add_memory_to_reserved_queue(fbe_memory_dps_queue_id_t memory_dps_queue_id, 
																fbe_u32_t number_of_chunks,
																memory_dps_type_t type)
{
	fbe_memory_header_t * current_memory_header = NULL;
	fbe_memory_header_t * master_memory_header = NULL;
	fbe_queue_element_t * queue_element = NULL;
    fbe_u32_t  queue_memory_size = 0;
	fbe_u64_t  offset;
	fbe_u32_t i;
	memory_dps_queue_t * main_dps_queue = NULL;
	memory_dps_queue_t * reserved_dps_queue = NULL;


    switch (memory_dps_queue_id) {
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
        break;
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        break;
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
        break;
    default:
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						    FBE_TRACE_MESSAGE_ID_INFO,
						    "%s: invalid queue id %d !\n", __FUNCTION__, memory_dps_queue_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	main_dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_MAIN, 0/* cpu */, type);
	reserved_dps_queue = memory_get_dps_reserved_queue(memory_dps_queue_id, 0/* cpu */, type);

	for(i = 0; i < number_of_chunks; i++){
		queue_element = fbe_queue_pop(&main_dps_queue->head);
		if (queue_element == NULL) {
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: main pool out of memory !\n", __FUNCTION__);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		main_dps_queue->number_of_free_chunks--;
		master_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);
		master_memory_header->magic_number = FBE_MEMORY_HEADER_MAGIC_NUMBER; /* It is allocated */

		offset = 0;
		while(offset + queue_memory_size + FBE_MEMORY_DPS_HEADER_SIZE <=  master_memory_header->memory_chunk_size){
			current_memory_header = (fbe_memory_header_t *)(master_memory_header->data + offset);
			current_memory_header->magic_number = 0;
			current_memory_header->memory_chunk_size = queue_memory_size; 
			current_memory_header->number_of_chunks = 1; 
			fbe_queue_element_init(&current_memory_header->u.queue_element);
			current_memory_header->data = (fbe_u8_t *)current_memory_header + FBE_MEMORY_DPS_HEADER_SIZE;

			/* Put it on the queue */			
			fbe_queue_push(&reserved_dps_queue->head, &current_memory_header->u.queue_element);
			reserved_dps_queue->number_of_chunks++;
			reserved_dps_queue->number_of_free_chunks++;
			offset += queue_memory_size + FBE_MEMORY_DPS_HEADER_SIZE;
		}
	}/* for(i = 0; i < number_of_chunks; i++) */

	/* Trace how much memory we got */
	memory_service_trace(FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"Reserved pool size %d chunks %lld\n", queue_memory_size, reserved_dps_queue->number_of_chunks);


    return FBE_STATUS_OK;
}

static fbe_status_t fbe_memory_dps_add_memory_to_dps_queue(fbe_memory_dps_queue_id_t memory_dps_queue_id, 
														   fbe_u32_t number_of_chunks,
														   memory_dps_type_t type)
{
	fbe_memory_header_t * current_memory_header = NULL;
	fbe_memory_header_t * master_memory_header = NULL;
	fbe_queue_element_t * queue_element = NULL;
    fbe_u32_t  queue_memory_size = 0;
	fbe_u64_t  offset;
	fbe_u32_t i;
	memory_dps_queue_t * main_dps_queue = NULL;
	memory_dps_queue_t * dps_queue = NULL;


    switch (memory_dps_queue_id) {
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
        break;
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        break;
    case FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO:
        queue_memory_size = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
        break;
    default:
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						    FBE_TRACE_MESSAGE_ID_INFO,
						    "%s: invalid queue id %d !\n", __FUNCTION__, memory_dps_queue_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	main_dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_MAIN, 0/* cpu */, type);
	dps_queue = memory_get_dps_queue(memory_dps_queue_id, 0/* cpu */, type);

	for(i = 0; i < number_of_chunks; i++){
		queue_element = fbe_queue_pop(&main_dps_queue->head);
		if (queue_element == NULL) {
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s: main pool out of memory !\n", __FUNCTION__);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		main_dps_queue->number_of_free_chunks--;
		master_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);
		master_memory_header->magic_number = FBE_MEMORY_HEADER_MAGIC_NUMBER; /* It is allocated */

		offset = 0;
		while(offset + queue_memory_size + FBE_MEMORY_DPS_HEADER_SIZE <=  master_memory_header->memory_chunk_size){
			current_memory_header = (fbe_memory_header_t *)(master_memory_header->data + offset);
			current_memory_header->magic_number = 0;
			current_memory_header->memory_chunk_size = queue_memory_size; 
			current_memory_header->number_of_chunks = 1; 
			fbe_queue_element_init(&current_memory_header->u.queue_element);
			current_memory_header->data = (fbe_u8_t *)current_memory_header + FBE_MEMORY_DPS_HEADER_SIZE;

			/* Put it on the queue */			
			fbe_queue_push(&dps_queue->head, &current_memory_header->u.queue_element);
			dps_queue->number_of_chunks++;
			dps_queue->number_of_free_chunks++;
			offset += queue_memory_size + FBE_MEMORY_DPS_HEADER_SIZE;
		}
	}/* for(i = 0; i < number_of_chunks; i++) */

	/* Trace how much memory we got */
	memory_service_trace(FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"Dps pool size %d chunks %lld\n", queue_memory_size, dps_queue->number_of_chunks);


    return FBE_STATUS_OK;
}


static fbe_status_t fbe_memory_dps_add_memory_to_queues(memory_dps_type_t type)
{

	fbe_memory_dps_init_parameters_t * params = &memory_dps_number_of_chunks;
    fbe_package_id_t package_id;
	memory_dps_queue_t * dps_queue = NULL;

    fbe_get_package_id(&package_id);


	if(type == MEMORY_DPS_TYPE_DATA) {
		params = &memory_dps_number_of_data_chunks;
	}

    /* allocate for reserved FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET */
    fbe_memory_dps_add_memory_to_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET, params->packet_number_of_chunks, type);

    /* allocate for reserved FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO */
    fbe_memory_dps_add_memory_to_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO, params->block_64_number_of_chunks, type);

    /* allocate for reserved FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO */
    fbe_memory_dps_add_memory_to_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO, params->block_1_number_of_chunks, type);


    /* allocate for fast FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET */
    fbe_memory_dps_add_memory_to_fast_queues(FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET, params->fast_packet_number_of_chunks, type);

    /* allocate for fast FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO */
    fbe_memory_dps_add_memory_to_fast_queues(FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO, params->fast_block_64_number_of_chunks, type);

    /* allocate for fast FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO */
    fbe_memory_dps_add_memory_to_fast_queues(FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO, params->fast_block_1_number_of_chunks, type);

	if(package_id == FBE_PACKAGE_ID_SEP_0){
        FBE_MEMORY_RESERVED_IO_MAX = 1;
    }

	/* allocate for reserved FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET */
	fbe_memory_dps_add_memory_to_reserved_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET, params->reserved_packet_number_of_chunks, type);

	/* allocate for reserved FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO */
	fbe_memory_dps_add_memory_to_reserved_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO, params->reserved_block_64_number_of_chunks, type);

	/* allocate for reserved FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO */
	fbe_memory_dps_add_memory_to_reserved_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO, params->reserved_block_1_number_of_chunks, type);

	dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_MAIN, 0 /* cpu */, type);
    dps_queue->number_of_chunks = dps_queue->number_of_free_chunks;

	return FBE_STATUS_OK;
}

static void 
memory_dps_process_reserved_request(fbe_memory_request_t * memory_request)
{
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	memory_dps_queue_t * dps_queue = NULL;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;
	
	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_reserved_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects) {
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
							 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							 "%s: Not enough memory in reserved pool\n", __FUNCTION__);  
		
		}
	}

	if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_reserved_queue(data_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.data_objects) {
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
							 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							 "%s: Not enough memory in reserved pool\n", __FUNCTION__);  
		
		}
	}

	if(!is_data_pool_initialized && (data_queue_id == control_queue_id)){
		dps_queue = memory_get_dps_reserved_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects)) {
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
							 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							 "%s: Not enough memory in reserved pool\n", __FUNCTION__);  
		
		}
	}


	if(number_of_objects_dc.split.data_objects > 0){
		dps_queue = memory_get_dps_reserved_queue(data_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_TRUE);
		memory_request->memory_io_master->reserved_chunk_counter[data_queue_id] += number_of_objects_dc.split.data_objects;
	} 
	if(number_of_objects_dc.split.control_objects > 0){
		dps_queue = memory_get_dps_reserved_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_FALSE);
		memory_request->memory_io_master->reserved_chunk_counter[control_queue_id] += number_of_objects_dc.split.control_objects;
	}

	memory_request->flags |= FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL;
	memory_request->memory_io_master->flags |= FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL;
	memory_request->memory_io_master->priority = memory_request->priority;
}



fbe_status_t fbe_memory_build_dc_request(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_dc_t number_of_objects,
                                            fbe_memory_request_priority_t priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context)
{
	fbe_cpu_id_t cpu_id = 0;

    if (fbe_memory_request_is_in_use(memory_request) == FBE_TRUE)
    {
        memory_request->chunk_size = 0;
		memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                    "%s: memory_request: 0x%p ptr: 0x%p state: %d is still in use\n", 
                             __FUNCTION__, memory_request, memory_request->ptr, memory_request->request_state);
		return FBE_STATUS_GENERIC_FAILURE;
    }
	fbe_queue_element_init(&memory_request->queue_element);
	memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK;
	memory_request->chunk_size = chunk_size;
	memory_request->number_of_objects = number_of_objects.number_of_objects;
    memory_request->priority = priority;

	if(io_stamp == FBE_MEMORY_REQUEST_IO_STAMP_INVALID){
		cpu_id = fbe_get_cpu_id();
		memory_request->io_stamp = io_stamp | ((fbe_u64_t)cpu_id << FBE_PACKET_IO_STAMP_SHIFT);
	} else {
		memory_request->io_stamp = io_stamp;
	}

	memory_request->ptr = NULL;
	memory_request->data_ptr = NULL;
	memory_request->completion_function = completion_function;
	memory_request->completion_context = completion_context;

	memory_request->memory_io_master = NULL;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_memory_build_dc_request_sync(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_dc_t number_of_objects,
                                            fbe_memory_request_priority_t priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context)
{
	fbe_memory_build_dc_request(memory_request, chunk_size, number_of_objects,
                                         priority, io_stamp, completion_function, completion_context);

	memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_SYNC;

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_memory_request_entry(fbe_memory_request_t * memory_request)
{
	fbe_cpu_id_t cpu_id;
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_bool_t is_sync = FBE_FALSE;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	memory_dps_queue_t * dps_queue = NULL;
	fbe_bool_t out_of_fast_memory = FBE_FALSE;
	fbe_status_t status;

	fbe_queue_init(&memory_request->chunk_queue);
	memory_request->flags = 0;
	memory_request->magic_number = FBE_MAGIC_NUMBER_MEMORY_REQUEST;



	if(memory_request->priority >= MEMORY_DPS_PRIORITY_MAX){
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: Invalid priority % d \n", __FUNCTION__, memory_request->priority);
	}
	
	//fbe_atomic_increment(&priority_hist[memory_request->priority]);

	/* If we have requests on the queue - we need to check the priority */
	if(memory_dps_request_queue_count > 0){
		if(memory_dps_request_queue_priority_max > memory_request->priority){
			return fbe_memory_request_priority_dc_entry(memory_request); /* This will take care of reserved memory as well */
		}
	}

	cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;

	if(memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_SYNC){
		is_sync = FBE_TRUE;
	}

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;

	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	/* lock the queue */
	fbe_spinlock_lock(&memory_dps_fast_lock[cpu_id]);

	if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
        memory_dps_fast_data_request_count[data_queue_id][cpu_id]++;
		dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.data_objects){
            memory_dps_request_count[data_queue_id][cpu_id][MEMORY_DPS_TYPE_DATA]++;
			out_of_fast_memory = FBE_TRUE;
		}
	}
    else {
        number_of_objects_dc.split.data_objects = 0;
    }

	if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
        memory_dps_fast_request_count[control_queue_id][cpu_id]++;
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){
			/* We out of control fast pool */
            memory_dps_request_count[control_queue_id][cpu_id][MEMORY_DPS_TYPE_CONTROL]++;
			out_of_fast_memory = FBE_TRUE;
		}
	}
    else {
        number_of_objects_dc.split.control_objects = 0;
    }

	if(!is_data_pool_initialized && (data_queue_id == control_queue_id) &&
       (control_queue_id < FBE_MEMORY_DPS_QUEUE_ID_LAST)){
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects)){
			/* We out of data fast pool */
            memory_dps_request_count[control_queue_id][cpu_id][MEMORY_DPS_TYPE_CONTROL]++;
			out_of_fast_memory = FBE_TRUE;
		}
	}

	if(out_of_fast_memory == FBE_TRUE){
		fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
		if(memory_dps_fast_queue_balance_enable == FBE_TRUE){
			status = fbe_memory_request_balance_entry(memory_request);
			if(status == FBE_STATUS_OK){ /* We got the memory from another pool */
				if(!is_sync){
					memory_request->completion_function( memory_request, memory_request->completion_context);
					return FBE_STATUS_PENDING;
				}
				return FBE_STATUS_OK;
			}
		}


		return fbe_memory_request_priority_dc_entry(memory_request);
	}

	/* If we here we have a memory for this request */
	if(number_of_objects_dc.split.data_objects > 0){
		dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_TRUE);
	} 
	if(number_of_objects_dc.split.control_objects > 0){
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_FALSE);
	}

	memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY;
	memory_request->flags |= FBE_MEMORY_REQUEST_FLAG_FAST_POOL;
	fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
	if(!is_sync){
		memory_request->completion_function( memory_request, memory_request->completion_context);
		return FBE_STATUS_PENDING;
	}
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_memory_free_request_entry(fbe_memory_request_t * memory_request)
{
	fbe_memory_header_t * current_memory_header = NULL;	
	fbe_queue_element_t * queue_element = NULL;
	fbe_memory_dps_queue_id_t control_queue_id = 0;
	fbe_memory_dps_queue_id_t data_queue_id = 0;
	fbe_cpu_id_t cpu_id;
    fbe_u32_t freed_chunks = 0;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	memory_dps_queue_t * dps_queue = NULL;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;

	/* Temporary solution to allow clients to free "aborted" memory requests */
	if(fbe_queue_is_empty(&memory_request->chunk_queue)){
		memory_service_trace(FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s: memory_request 0x%p chunk_queue is empty \n", __FUNCTION__, memory_request);

		/* There is nothing to release */
		return FBE_STATUS_OK;
	}

	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	if(memory_request->flags & FBE_MEMORY_REQUEST_FLAG_FAST_POOL){
		cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
		if(cpu_id == FBE_CPU_ID_INVALID){
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: Invalid cpu_id \n", __FUNCTION__);

			memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ERROR;
			return FBE_STATUS_GENERIC_FAILURE;
		}

		fbe_spinlock_lock(&memory_dps_fast_lock[cpu_id]);
		while(queue_element = fbe_queue_pop(&memory_request->chunk_queue)){
			current_memory_header = fbe_memory_dps_chunk_queue_element_to_header(queue_element);
			if((current_memory_header->magic_number & FBE_MEMORY_HEADER_MASK) == FBE_MEMORY_HEADER_MAGIC_NUMBER_DATA){
				dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);	
			} else {
				dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);	
			}
			fbe_queue_push_front(&dps_queue->head, &current_memory_header->u.queue_element);
			fbe_atomic_increment(&dps_queue->number_of_free_chunks);
			/* If we have enough free chunks we can enable balancing */
			if(dps_queue->number_of_free_chunks > (dps_queue->number_of_chunks >> 1)){
				memory_dps_fast_queue_balance_enable = FBE_TRUE;
			}
		}
		fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
	    
		memory_request->request_state = FBE_MEMORY_REQUEST_STATE_DESTROYED;
		memory_request->ptr = NULL;
        memory_request->data_ptr = NULL;

		return FBE_STATUS_OK;
	} /* If fast pool */
		
    fbe_spinlock_lock(&memory_dps_lock);

	while(queue_element = fbe_queue_pop(&memory_request->chunk_queue)){
		current_memory_header = fbe_memory_dps_chunk_queue_element_to_header(queue_element);
		if(memory_request->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){
			if((current_memory_header->magic_number & FBE_MEMORY_HEADER_MASK) == FBE_MEMORY_HEADER_MAGIC_NUMBER_DATA){
				dps_queue = memory_get_dps_reserved_queue(data_queue_id, 0/* cpu */, MEMORY_DPS_TYPE_DATA);
			} else {
				dps_queue = memory_get_dps_reserved_queue(control_queue_id, 0/* cpu */, MEMORY_DPS_TYPE_CONTROL);
			}
		} else {
			if((current_memory_header->magic_number & FBE_MEMORY_HEADER_MASK) == FBE_MEMORY_HEADER_MAGIC_NUMBER_DATA){
				dps_queue = memory_get_dps_queue(data_queue_id, 0/* cpu */, MEMORY_DPS_TYPE_DATA);
			} else {
				dps_queue = memory_get_dps_queue(control_queue_id, 0/* cpu */, MEMORY_DPS_TYPE_CONTROL);
			}
		}
		fbe_queue_push(&dps_queue->head, &current_memory_header->u.queue_element);
		fbe_atomic_increment(&dps_queue->number_of_free_chunks);
        freed_chunks++;
	}
    if(number_of_objects_dc.split.data_objects + number_of_objects_dc.split.control_objects != freed_chunks)
    {
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "mem free req entry: number_of_objects %d != freed_chunks %d\n", 
                             memory_request->number_of_objects, freed_chunks);  
    }

	if(memory_request->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){
		memory_request->memory_io_master->reserved_chunk_counter[control_queue_id] -= number_of_objects_dc.split.control_objects;
		memory_request->memory_io_master->reserved_chunk_counter[data_queue_id] -= number_of_objects_dc.split.data_objects;

		if ((memory_request->memory_io_master->reserved_chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET] == 0) &&
            (memory_request->memory_io_master->chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET] == 0) &&
            (memory_request->memory_io_master->reserved_chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] == 0) &&
            (memory_request->memory_io_master->chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] == 0)) {
				if(memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){
					/* This io_master are not in reserve any more */
					memory_request->memory_io_master->priority = 0;
					if(fbe_memory_reserved_io_count != 1){
						memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
										 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
										 "%s: Invalid fbe_memory_reserved_io_count\n", __FUNCTION__);  
					}


					memory_request->memory_io_master->flags &= ~FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL;
					fbe_memory_reserved_io_count--;
					reserved_memory_io_master = NULL;
				}
		}
	} else { /* Not reserved pool */
		if(memory_request->memory_io_master != NULL){
			memory_request->memory_io_master->chunk_counter[control_queue_id] -= number_of_objects_dc.split.control_objects;
			memory_request->memory_io_master->chunk_counter[data_queue_id] -= number_of_objects_dc.split.data_objects;

			if ((memory_request->memory_io_master->reserved_chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET] == 0) &&
                (memory_request->memory_io_master->chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET] == 0) &&
                (memory_request->memory_io_master->reserved_chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] == 0) &&
                (memory_request->memory_io_master->chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] == 0) &&
				(memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL)) {

					if(memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){
						/* This io_master are not in reserve any more */
						memory_request->memory_io_master->priority = 0;
						if(fbe_memory_reserved_io_count != 1){
							memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
											 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
											 "%s: Invalid fbe_memory_reserved_io_count\n", __FUNCTION__);  

						}

						memory_request->memory_io_master->flags &= ~FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL;
						fbe_memory_reserved_io_count--;
						reserved_memory_io_master = NULL;
					}
			}
		}

	    //memory_pool_number_of_allocated_chunks[control_queue_id] -= number_of_objects_dc.split.control_objects;
		//memory_dps_queue[control_queue_id].number_of_free_chunks += number_of_objects_dc.split.control_objects;

		//memory_pool_number_of_allocated_chunks[data_queue_id] -= number_of_objects_dc.split.data_objects;
		//memory_dps_queue[data_queue_id].number_of_free_chunks += number_of_objects_dc.split.data_objects;

	}

	if(memory_dps_request_queue_count != 0) {
		fbe_rendezvous_event_set(&memory_dps_event);
	}

	fbe_spinlock_unlock(&memory_dps_lock);
    memory_request->request_state = FBE_MEMORY_REQUEST_STATE_DESTROYED;
    memory_request->ptr = NULL;
	memory_request->data_ptr = NULL;
	memory_request->memory_io_master = NULL;
	memory_request->flags = 0;

	return FBE_STATUS_OK;
}

static fbe_status_t 
memory_dps_queue_id_to_chunk_size(fbe_memory_dps_queue_id_t memory_dps_queue_id, fbe_memory_chunk_size_t * chunk_size)
{
	switch(memory_dps_queue_id) {
		case FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET:
			*chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
			break;
		case FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO:
			*chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
			break;
        case  FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO:
            *chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
			break;
		default:
			*chunk_size = 0;
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
		                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                     "%s: Invalid chunk_size queue id is %x\n", __FUNCTION__, memory_dps_queue_id);
			return FBE_STATUS_GENERIC_FAILURE;
	}
	return FBE_STATUS_OK;
}

static fbe_bool_t 
memory_request_priority_is_allocation_allowed(fbe_memory_request_t * memory_request)
{
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_bool_t is_allocation_allowed;
	memory_dps_queue_t * dps_queue = NULL;
    fbe_cpu_id_t cpu_id;

    cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;
	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	is_allocation_allowed = FBE_TRUE;

	if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
        if (!fbe_queue_is_element_on_queue(&memory_request->queue_element))
        {
            memory_dps_deferred_count[control_queue_id][cpu_id][MEMORY_DPS_TYPE_CONTROL]++;	
        }
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){				
			is_allocation_allowed = FBE_FALSE;
		}
	}

	if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_queue(data_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);
        if (!fbe_queue_is_element_on_queue(&memory_request->queue_element))
        {
            memory_dps_deferred_count[data_queue_id][cpu_id][MEMORY_DPS_TYPE_DATA]++;	
        }
		if(dps_queue->number_of_free_chunks  < number_of_objects_dc.split.data_objects){					
			is_allocation_allowed = FBE_FALSE;
		}
	}

	if(!is_data_pool_initialized && (data_queue_id == control_queue_id)){
		dps_queue = memory_get_dps_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);

        if ((!fbe_queue_is_element_on_queue(&memory_request->queue_element)) &&
             (control_queue_id < FBE_MEMORY_DPS_QUEUE_ID_LAST))
        {
            memory_dps_deferred_count[control_queue_id][cpu_id][MEMORY_DPS_TYPE_CONTROL]++;	
        }
		if(dps_queue->number_of_free_chunks  < (fbe_u32_t)(number_of_objects_dc.split.data_objects + number_of_objects_dc.split.control_objects)){
			is_allocation_allowed = FBE_FALSE;
		}
	}

	return is_allocation_allowed;
}

static fbe_status_t
fbe_memory_request_priority_dc_entry(fbe_memory_request_t * memory_request)
{
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_bool_t is_allocation_allowed;
	fbe_bool_t is_sync = FBE_FALSE;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;

	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	if(memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_SYNC){
		memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK;
		is_sync = FBE_TRUE;
	}

	fbe_spinlock_lock(&memory_dps_lock);

	is_allocation_allowed = memory_request_priority_is_allocation_allowed(memory_request);

	if((is_allocation_allowed == FBE_TRUE) && 
			((memory_dps_request_queue_count == 0) || (memory_dps_request_queue_priority_max < memory_request->priority))){
		memory_dps_process_entry(memory_request);
		memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY;
		fbe_spinlock_unlock(&memory_dps_lock);
		if(!is_sync){
			memory_request->completion_function( memory_request, memory_request->completion_context);
			return FBE_STATUS_PENDING;
		}
		return FBE_STATUS_OK;
	}

	if(memory_request->memory_io_master != NULL) {
		/* Check if we already taped to reserve pool */
		if(memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){
			memory_dps_process_reserved_request(memory_request);
			memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY;
			fbe_spinlock_unlock(&memory_dps_lock);
			if(!is_sync){
				memory_request->completion_function( memory_request, memory_request->completion_context);
				return FBE_STATUS_PENDING;
			}
			return FBE_STATUS_OK;
		}
	}

	/* We may want to check that completion function is not NULL and if it is fail the request */
	fbe_queue_push(&memory_dps_priority_queue[memory_request->priority], &memory_request->queue_element);

	fbe_atomic_increment(&memory_dps_request_queue_count);  

	if(memory_request->memory_io_master != NULL) { /* We can not maintain reservation for requests without memory_io_master */
		if(memory_dps_request_queue_priority_max < memory_request->priority){
			memory_dps_request_queue_priority_max = memory_request->priority;
		}
	}

    fbe_spinlock_unlock(&memory_dps_lock);
	return FBE_STATUS_PENDING;
}

static void 
memory_dps_fill_request_from_queue(fbe_memory_request_t * memory_request, memory_dps_queue_t * queue, fbe_bool_t is_data_queue)
{
	fbe_memory_header_t * master_memory_header = NULL;
	fbe_memory_header_t * prev_memory_header = NULL;
	fbe_memory_header_t * current_memory_header = NULL;
	fbe_queue_element_t	* queue_element = NULL;	
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_memory_dps_queue_id_t queue_id;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	fbe_memory_chunk_size_t chunk_size;
	fbe_atomic_t number_of_objects;
	fbe_u64_t master_magic;
	fbe_u32_t i;
	fbe_cpu_id_t cpu_id;

	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);
	cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;

	if(is_data_queue){
		queue_id = data_queue_id;
		master_magic = FBE_MEMORY_HEADER_MAGIC_NUMBER_DATA | (cpu_id << FBE_MEMORY_HEADER_SHIFT);
	} else {
		queue_id = control_queue_id;
		master_magic = FBE_MEMORY_HEADER_MAGIC_NUMBER | (cpu_id << FBE_MEMORY_HEADER_SHIFT);
	}

	memory_dps_queue_id_to_chunk_size(queue_id, &chunk_size);

	/* Get all requiered chunks */
	queue_element = fbe_queue_pop(&queue->head);
	master_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);

	if(is_data_queue){
		memory_request->data_ptr = master_memory_header;
		number_of_objects = number_of_objects_dc.split.data_objects;
	} else {
		memory_request->ptr = master_memory_header;
		number_of_objects = number_of_objects_dc.split.control_objects;
	}

	/* Fill master header */
	master_memory_header->magic_number = master_magic;
	master_memory_header->memory_chunk_size = chunk_size; 
	master_memory_header->number_of_chunks = (fbe_u32_t)number_of_objects; 
	master_memory_header->u.hptr.master_header = master_memory_header; 
	master_memory_header->u.hptr.next_header = NULL;
	master_memory_header->data = (fbe_u8_t *)master_memory_header + FBE_MEMORY_HEADER_SIZE;
    master_memory_header->priority = memory_request->priority;

	fbe_queue_push(&memory_request->chunk_queue, &master_memory_header->queue_element);
  
	prev_memory_header = master_memory_header;
	for(i = 1; i < number_of_objects; i++){
		queue_element = fbe_queue_pop(&queue->head);
		current_memory_header = fbe_memory_dps_queue_element_to_header(queue_element);

		prev_memory_header->u.hptr.next_header = current_memory_header;
		current_memory_header->magic_number = master_magic;
		current_memory_header->memory_chunk_size = chunk_size; 
		current_memory_header->number_of_chunks = 0; 
		current_memory_header->u.hptr.master_header = master_memory_header; 
		current_memory_header->u.hptr.next_header = NULL;
		current_memory_header->data = (fbe_u8_t *)current_memory_header + FBE_MEMORY_HEADER_SIZE;
        current_memory_header->priority = memory_request->priority;

		fbe_queue_push(&memory_request->chunk_queue, &current_memory_header->queue_element);

        prev_memory_header = current_memory_header;
	}

	//queue->number_of_free_chunks -= number_of_objects;    
	fbe_atomic_add(&queue->number_of_free_chunks, -number_of_objects);
    
}

/* How much memory we want to leave in a pool */
#define DPS_CHUNK_SHIFT 3 

static fbe_bool_t 
fbe_memory_check_queue_balance(fbe_memory_request_t * memory_request, fbe_cpu_id_t cpu_id)
{
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	memory_dps_queue_t * dps_queue = NULL;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;
	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	/* The queue must have at least 25% of free chunks */
	if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.data_objects){
			return FBE_FALSE; /* Not enough data memory */
		}
		if((dps_queue->number_of_free_chunks - number_of_objects_dc.split.data_objects) < (dps_queue->number_of_chunks >> DPS_CHUNK_SHIFT)){
			return FBE_FALSE; /* Not enough data memory */
		}
	}

	/* The queue must have at least 25% of free chunks */
	if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){
			return FBE_FALSE; /* Not enough control memory */
		}
		if((dps_queue->number_of_free_chunks - number_of_objects_dc.split.control_objects) < (dps_queue->number_of_chunks >> DPS_CHUNK_SHIFT)){
			return FBE_FALSE; /* Not enough control memory */
		}
	}

	if(!is_data_pool_initialized && (data_queue_id == control_queue_id)){
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects)){
			return FBE_FALSE; /* Not enough control memory */
		}
		if((dps_queue->number_of_free_chunks - (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects))
																												< (dps_queue->number_of_chunks >> DPS_CHUNK_SHIFT)){
			return FBE_FALSE; /* Not enough control memory */
		}
	}

	return FBE_TRUE;
}

static fbe_status_t 
fbe_memory_request_balance_try_allocate(fbe_memory_request_t * memory_request, fbe_cpu_id_t cpu_id)
{
	fbe_memory_dps_queue_id_t control_queue_id;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	memory_dps_queue_t * dps_queue = NULL;
	fbe_bool_t out_of_fast_memory = FBE_FALSE;

	fbe_queue_init(&memory_request->chunk_queue);
	memory_request->flags = 0;
	memory_request->magic_number = FBE_MAGIC_NUMBER_MEMORY_REQUEST;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;

	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	/* Lock the queue */
	fbe_spinlock_lock(&memory_dps_fast_lock[cpu_id]);

	if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
        memory_dps_fast_data_request_count[data_queue_id][cpu_id]++;
		dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.data_objects){
			out_of_fast_memory = FBE_TRUE;
			//fbe_debug_break();
		}
	}

	if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
        memory_dps_fast_request_count[control_queue_id][cpu_id]++;
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){
			/* We out of control fast pool */
			out_of_fast_memory = FBE_TRUE;
		}
	}

	if(!is_data_pool_initialized && (data_queue_id == control_queue_id)){
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects)){
			out_of_fast_memory = FBE_TRUE;
		}
	}

	if(out_of_fast_memory == FBE_TRUE){
		fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

	/* If we here we have a memory for this request */
	if(number_of_objects_dc.split.data_objects > 0){
		dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_TRUE);
	} 

	if(number_of_objects_dc.split.control_objects > 0){
		dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
		memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_FALSE);
	}

	fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);

	return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_memory_request_balance_entry(fbe_memory_request_t * memory_request)
{
	fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;
	fbe_cpu_id_t id;
	fbe_bool_t is_balanced = FBE_FALSE;
	fbe_status_t status;

	fbe_queue_init(&memory_request->chunk_queue);
	memory_request->flags = 0;
	memory_request->magic_number = FBE_MAGIC_NUMBER_MEMORY_REQUEST;

	/* Scan all fast queues */
	for(id = 0; (id < memory_dps_core_count) && (cpu_id == FBE_CPU_ID_INVALID); id++){
		if(fbe_memory_check_queue_balance(memory_request, id)){ /* We found pool with enough resources */
			status = fbe_memory_request_balance_try_allocate(memory_request, id);
			if(status == FBE_STATUS_OK){ /* Allocation was successful */
				cpu_id = id; /* We done looking for memory */
			}
		} 
	}

	if(cpu_id == FBE_CPU_ID_INVALID){ /* We out of memory in all pools */
		memory_dps_fast_queue_balance_enable = FBE_FALSE; /* Disable balancing */
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

	/* We found some memory on cpu_id pool */
	if(cpu_id != (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT){
		is_balanced = FBE_TRUE;
	}

	memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY;
	memory_request->flags |= FBE_MEMORY_REQUEST_FLAG_FAST_POOL;
		
	if(is_balanced == FBE_TRUE){
		memory_request->io_stamp &= ~FBE_PACKET_IO_STAMP_MASK; /* Clear cpu information */
		memory_request->io_stamp |= ((fbe_u64_t)cpu_id << FBE_PACKET_IO_STAMP_SHIFT); /* Put new cpu info */
		memory_request->flags |= FBE_MEMORY_REQUEST_FLAG_BALANCED;
	}

	return FBE_STATUS_OK;
}


fbe_bool_t fbe_memory_is_reject_allowed_set(fbe_memory_io_master_t *io_master_p, 
                                            fbe_u8_t flags)
{
    /* The policy is:
     *   If we already notified the client, the flag is set in arrival_reject_allowed_flags.
     *   If the client allows rejection, the flag is set in client_reject_allowed_flags.
     *  
     * If the client allows rejection AND we already notified, then reject allowed is set.
     */
    if ((io_master_p->arrival_reject_allowed_flags & flags) &&
        (io_master_p->client_reject_allowed_flags & flags))
    {
        /* Rejection is allowed.
         */
        return FBE_TRUE;
    }
    /* Rejection is not allowed by us (since we didn't notify yet) or client.
     */
    return FBE_FALSE;
}

/* We will try to allocate memory as hard as we can.
	We will evaluate fast pools first and then regular pool.
*/
static fbe_bool_t 
memory_request_try_to_allocate(fbe_memory_request_t * memory_request)
{
	fbe_memory_number_of_objects_dc_t number_of_objects_dc;
	fbe_memory_dps_queue_id_t data_queue_id;
	fbe_memory_dps_queue_id_t control_queue_id;
	memory_dps_queue_t * dps_queue = NULL;
    fbe_cpu_id_t cpu_id;
	fbe_bool_t out_of_fast_memory;

	number_of_objects_dc.number_of_objects = memory_request->number_of_objects;
	memory_dps_request_to_queue_id(memory_request, &control_queue_id, &data_queue_id);

	/* Loop over all fast pools */

	for(cpu_id = 0; cpu_id < memory_dps_core_count; cpu_id++){
		/* lock the queue */
		//fbe_spinlock_lock(&memory_dps_fast_lock[cpu_id]);
		out_of_fast_memory = FBE_FALSE;

		if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
			dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
			if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.data_objects){
				out_of_fast_memory = FBE_TRUE;
				continue;
			}
		}
		else {
			number_of_objects_dc.split.data_objects = 0;
		}

		if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
			dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
			if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){
				out_of_fast_memory = FBE_TRUE;
				continue;
			}
		}
		else {
			number_of_objects_dc.split.control_objects = 0;
		}

		if(!is_data_pool_initialized && (data_queue_id == control_queue_id) &&
		   (control_queue_id < FBE_MEMORY_DPS_QUEUE_ID_LAST)){
			dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
			if(dps_queue->number_of_free_chunks < (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects)){
				out_of_fast_memory = FBE_TRUE;
				continue;
			}
		}

		if(out_of_fast_memory == FBE_TRUE){
			//fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
			continue; /* We will try another pool */
		}
			
		/* Most likely we have a memory for this request */
		/* Now let's lock the queue and check again */
		fbe_spinlock_lock(&memory_dps_fast_lock[cpu_id]);
		out_of_fast_memory = FBE_FALSE;

		if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
			dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
			if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.data_objects){
				out_of_fast_memory = FBE_TRUE;
			}
		}

		if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
			dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
			if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){
				out_of_fast_memory = FBE_TRUE;
			}
		}

		if(!is_data_pool_initialized && (data_queue_id == control_queue_id) &&
		   (control_queue_id < FBE_MEMORY_DPS_QUEUE_ID_LAST)){
			dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
			if(dps_queue->number_of_free_chunks < (fbe_u32_t)(number_of_objects_dc.split.control_objects + number_of_objects_dc.split.data_objects)){
				out_of_fast_memory = FBE_TRUE;
			}
		}

		if(out_of_fast_memory == FBE_TRUE){
			/* It is possible that we did not get our memory here, because we performad the first check not under the lock */
			fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
			continue; /* We will try another pool */
		}
		

		if(number_of_objects_dc.split.data_objects > 0){
			dps_queue = memory_get_dps_fast_queue(data_queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
			memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_TRUE);
		} 
		if(number_of_objects_dc.split.control_objects > 0){
			dps_queue = memory_get_dps_fast_queue(control_queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
			memory_dps_fill_request_from_queue(memory_request, dps_queue, FBE_FALSE);
		}

		memory_request->flags |= FBE_MEMORY_REQUEST_FLAG_FAST_POOL;
		fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
		return FBE_TRUE; /* We got the memory */
	} /* for(cpu_id = 0; cpu_id < memory_dps_core_count; cpu_id++) */

	/* If we here the fast pools do not have a memory */
    cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;

	if(control_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks < number_of_objects_dc.split.control_objects){
			//if(reserved_memory_io_master != NULL && reserved_memory_io_master == memory_request->memory_io_master){ /* We should always grant it */
			if(memory_request->memory_io_master != NULL && memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){/* We should always grant it */
				memory_dps_process_reserved_request(memory_request);
				return FBE_TRUE;
			}
			return FBE_FALSE; /* We do not have memory */
		}
	}

	if(data_queue_id != FBE_MEMORY_DPS_QUEUE_ID_INVALID){
		dps_queue = memory_get_dps_queue(data_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks  < number_of_objects_dc.split.data_objects){
			//if(reserved_memory_io_master != NULL && reserved_memory_io_master == memory_request->memory_io_master){ /* We should always grant it */
			if(memory_request->memory_io_master != NULL && memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){/* We should always grant it */
				memory_dps_process_reserved_request(memory_request);
				return FBE_TRUE;
			}
			return FBE_FALSE; /* We do not have memory */
		}
	}

	if(!is_data_pool_initialized && (data_queue_id == control_queue_id)){
		dps_queue = memory_get_dps_queue(control_queue_id, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
		if(dps_queue->number_of_free_chunks  < (fbe_u32_t)(number_of_objects_dc.split.data_objects + number_of_objects_dc.split.control_objects)){
			//if(reserved_memory_io_master != NULL && reserved_memory_io_master == memory_request->memory_io_master){ /* We should always grant it */
			if(memory_request->memory_io_master != NULL && memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){/* We should always grant it */
				memory_dps_process_reserved_request(memory_request);
				return FBE_TRUE;
			}
			return FBE_FALSE; /* We do not have memory */
		}
	}

	/* If we here we have a memory for this request */
	memory_dps_process_entry(memory_request);

	return FBE_TRUE; /* We got the memory */
}

fbe_status_t 
fbe_memory_check_state(fbe_cpu_id_t cpu_id, fbe_bool_t check_data_memory)
{
    fbe_memory_dps_queue_id_t queue_id;

    memory_dps_queue_t * dps_queue = NULL;

    /* Lock the queue */
    fbe_spinlock_lock(&memory_dps_fast_lock[cpu_id]);

    queue_id  = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
    dps_queue = memory_get_dps_fast_queue(queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
    if(dps_queue->number_of_free_chunks < (dps_queue->number_of_chunks >> 1)){
        fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
        /* We out of control fast pool */
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    queue_id  = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
    dps_queue = memory_get_dps_fast_queue(queue_id, cpu_id, MEMORY_DPS_TYPE_CONTROL);
    if(dps_queue->number_of_free_chunks < (dps_queue->number_of_chunks >> 1)){
        fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
        /* We out of control fast pool */
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

	if(check_data_memory){
		queue_id  = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET;
		dps_queue = memory_get_dps_fast_queue(queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks < (dps_queue->number_of_chunks >> 1)){
			fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
			/* We out of control fast pool */
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}

		queue_id  = FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO;
		dps_queue = memory_get_dps_fast_queue(queue_id, cpu_id, MEMORY_DPS_TYPE_DATA);
		if(dps_queue->number_of_free_chunks < (dps_queue->number_of_chunks >> 1)){
			fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
			/* We out of control fast pool */
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}
	}

    fbe_spinlock_unlock(&memory_dps_fast_lock[cpu_id]);
    return FBE_STATUS_OK;
}

static void memory_dps_dispatch_queue(void)
{    
    fbe_memory_request_t * memory_request;
    fbe_memory_request_t * next_memory_request;
	
	fbe_queue_head_t abort_queue;
	fbe_queue_head_t alloc_queue;

	fbe_memory_request_t * priority_request = NULL;

	fbe_s32_t i;

	fbe_bool_t out_of_memory = FBE_FALSE;

	fbe_queue_init(&abort_queue);
	fbe_queue_init(&alloc_queue);

    fbe_spinlock_lock(&memory_dps_lock);
    
	fbe_rendezvous_event_clear(&memory_dps_event);

    /* Finish any aborted requests */
    if (memory_dps_b_requested_aborted) {
        for(i = memory_dps_request_queue_priority_max; i >= 0; i--){

            memory_request = (fbe_memory_request_t *)fbe_queue_front(&memory_dps_priority_queue[i]);
            while(memory_request != NULL) { /* Iterate over the queue and satisfy requests */	
                next_memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
                if(memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ABORTED){ /* Get rid of aborted requests */
				    fbe_queue_remove(&memory_request->queue_element);
				    fbe_atomic_decrement(&memory_dps_request_queue_count);					
				    fbe_queue_push(&abort_queue, &memory_request->queue_element);
			    }
                memory_request = next_memory_request;
            } /* while(memory_request != NULL){  */

        }/* for(i = memory_dps_request_queue_priority_max; i >= 0; i--){ */
        memory_dps_b_requested_aborted = FBE_FALSE;
    }

	/* Iterate thru priority queues */
	for(i = memory_dps_request_queue_priority_max; i >= 0 && !out_of_memory; i--){

		memory_dps_request_queue_priority_max = i;

		memory_request = (fbe_memory_request_t *)fbe_queue_front(&memory_dps_priority_queue[i]);
		while(memory_request != NULL && !out_of_memory){ /* Iterate over the queue and satisfy requests */	
			if(memory_request->request_state == FBE_MEMORY_REQUEST_STATE_ABORTED){ /* Get rid of aborted requests */
				next_memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
				fbe_queue_remove(&memory_request->queue_element);
				fbe_atomic_decrement(&memory_dps_request_queue_count);					
				fbe_queue_push(&abort_queue, &memory_request->queue_element);
				memory_request = next_memory_request;
				continue;
			}

			if(memory_request_try_to_allocate(memory_request)){
				/* We succesfully allocated the memory */
				next_memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
				fbe_queue_remove(&memory_request->queue_element);					
				fbe_atomic_decrement(&memory_dps_request_queue_count);					
				fbe_queue_push(&alloc_queue, &memory_request->queue_element);
				memory_request = next_memory_request;
				continue;
			}

			/* If we here - we out of memory */
			out_of_memory = FBE_TRUE;

		} /* while(memory_request != NULL && !out_of_memory){  */
	}/* for(i = memory_dps_request_queue_priority_max; i >= 0; i--){ */

	/* If we was not able to allocate anything */
	if(out_of_memory && fbe_queue_is_empty(&alloc_queue)){
		if(memory_dps_deadlock_time == 0){
			memory_dps_deadlock_time = fbe_get_time();
		}
	}else {
		memory_dps_deadlock_time = 0;
	}

	/* If we did not allocated anything in past 1 second */
	if(memory_dps_deadlock_time != 0 && (fbe_get_elapsed_milliseconds(memory_dps_deadlock_time) > 1000)){
		priority_request = NULL;
		/* Scan for highest priority request with memory I/O master */
		for(i = memory_dps_request_queue_priority_max; (i >= 0) && (priority_request == NULL); i--){
			memory_request = (fbe_memory_request_t *)fbe_queue_front(&memory_dps_priority_queue[i]);

			while((memory_request != NULL) && (priority_request == NULL)){ /* Iterate over the queue and satisfy requests */
				next_memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
				if(memory_request->memory_io_master != NULL){
					priority_request = memory_request;
				}
				memory_request = next_memory_request;
			}
		}

        /* If we have outstanding reserved request at the same priority as priority_request
           we need to scan for other reserved requests 
           */
        if((reserved_memory_io_master != NULL) && 
           (priority_request->priority == reserved_memory_io_master->priority) &&
           (priority_request->memory_io_master != reserved_memory_io_master))
        { /* Scan the Q for reserved requests */
            i = reserved_memory_io_master->priority; /* I think it is OK */
            memory_request = (fbe_memory_request_t *)fbe_queue_front(&memory_dps_priority_queue[i]);
            while(memory_request != NULL)
            { 
                /* Iterate over the queue and satisfy reserved requests */
                next_memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
                if((memory_request->memory_io_master!= NULL) && (memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL))
                {
                    if(memory_request_try_to_allocate(memory_request))
                    {  
                        /* That should ALWAYS be TRUE */
                        /* We succesfully allocated the memory */
                        next_memory_request = (fbe_memory_request_t *)fbe_queue_next(&memory_dps_priority_queue[i], &memory_request->queue_element);
                        fbe_queue_remove(&memory_request->queue_element);                           
                        fbe_atomic_decrement(&memory_dps_request_queue_count);                          
                        fbe_queue_push(&alloc_queue, &memory_request->queue_element);
                    }
                }
                memory_request = next_memory_request;
            } /* Iterate over the queue and satisfy reserved requests */
        } /* If we have outstanding reserved request ... */

        /* If our priority request has a higher priority than reserved request we should change the reserved request */
		if((priority_request != NULL) && (reserved_memory_io_master != NULL) && (reserved_memory_io_master->priority < priority_request->priority)){
			reserved_memory_io_master->flags &= ~FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL;
			reserved_memory_io_master->priority = 0;
			reserved_memory_io_master = NULL;
			fbe_memory_reserved_io_count--;
        }

		if((priority_request != NULL) && (fbe_memory_reserved_io_count < FBE_MEMORY_RESERVED_IO_MAX)) { 
            fbe_cpu_id_t cpu_id = 0;
			memory_request = priority_request;

            cpu_id = (memory_request->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;						
			fbe_memory_deadlock_counter[cpu_id]++;

			fbe_queue_remove(&memory_request->queue_element);

			/* do the allocation */
			memory_dps_process_reserved_request(memory_request);
			fbe_memory_reserved_io_count++;
			reserved_memory_io_master = memory_request->memory_io_master;

			fbe_atomic_decrement(&memory_dps_request_queue_count);					

			fbe_queue_push(&alloc_queue, &memory_request->queue_element);				
		} /* if(fbe_memory_reserved_io_count < FBE_MEMORY_RESERVED_IO_MAX) */
	} /* if(memory_dps_deadlock_time != 0 && (fbe_get_time() - memory_dps_deadlock_time > 1000)) */

    fbe_spinlock_unlock(&memory_dps_lock);    

	/* Complete the allocation requests first */
	while(memory_request = (fbe_memory_request_t *)fbe_queue_pop(&alloc_queue)){
		memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED;
		fbe_transport_run_queue_push_request(memory_request);
	}
	fbe_queue_destroy(&alloc_queue);

    /* Than take care of aborts */
    while(memory_request = (fbe_memory_request_t *)fbe_queue_pop(&abort_queue)){
        if(memory_request->memory_io_master != NULL){
            if(memory_request->memory_io_master->flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL){ /* We aborting reserved request */
                memory_request->memory_io_master->flags &= ~FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL;
                memory_request->memory_io_master->priority = 0;
                reserved_memory_io_master = NULL;
                fbe_memory_reserved_io_count--;
            }
        }
        fbe_transport_run_queue_push_request(memory_request);
    }
    fbe_queue_destroy(&abort_queue);
}

/* This function will reduce available memory size */
fbe_status_t fbe_memory_dps_reduce_size(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
	fbe_u32_t cpu;
	fbe_u32_t id;
	fbe_u32_t type;
	memory_dps_queue_t * dps_queue = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	/* For now we will not look at packet, but just reduce the memory  */
    for(cpu = 0; cpu < FBE_CPU_ID_MAX; cpu++){
		fbe_spinlock_lock(&memory_dps_fast_lock[cpu]);
        fbe_memory_deadlock_counter[cpu] = 0;
		for(id = 0; id < FBE_MEMORY_DPS_QUEUE_ID_LAST; id++){
			for(type = 0; type < MEMORY_DPS_TYPE_LAST; type ++){
				dps_queue = memory_get_dps_fast_queue(id, cpu, type);		
				dps_queue->number_of_free_chunks = 0;
				//dps_queue->number_of_chunks = 0;				
			}
        }
		fbe_spinlock_unlock(&memory_dps_fast_lock[cpu]);
    }

	fbe_spinlock_lock(&memory_dps_lock);

	dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
	//dps_queue->number_of_free_chunks = 1000; /* Enough memory for control operations */
	//dps_queue->number_of_chunks = 100;

	dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO, 0 /* cpu */, MEMORY_DPS_TYPE_CONTROL);
	dps_queue->number_of_free_chunks = 10;
	//dps_queue->number_of_chunks = 10;

	dps_queue = memory_get_dps_queue(FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO, 0 /* cpu */, MEMORY_DPS_TYPE_DATA);
	dps_queue->number_of_free_chunks = 10;
	//dps_queue->number_of_chunks = 10;


	memory_dps_size_reduced = FBE_TRUE;

	fbe_spinlock_unlock(&memory_dps_lock);



    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

	return FBE_STATUS_OK;
}
