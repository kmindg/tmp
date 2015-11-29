
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_service.h"
#include "fbe_base_object.h"
#include "fbe_memory_private.h"
#include "fbe/fbe_panic.h"

/*! @note.  This size needs to be at least as big as a chunk. 
 *          PVD validates this is true at init time. 
 */
#define FBE_MEMORY_BLOCKS_PER_BUCKET 2048 /* 0x800 - 1MB */
#define FBE_MEMORY_ZERO_BIT_BUCKET_SIZE (FBE_BE_BYTES_PER_BLOCK * FBE_MEMORY_BLOCKS_PER_BUCKET)
#define FBE_MEMORY_INVALID_PATTERN_BIT_BUCKET_SIZE (FBE_BE_BYTES_PER_BLOCK * FBE_MEMORY_BLOCKS_PER_BUCKET)
/* Size of bitbucket we use for reading into.
 */
#define FBE_MEMORY_BIT_BUCKET_BYTES (1024 * 1024) 
#define FBE_MEMORY_CHUNK_FREE  0x00
#define FBE_MEMORY_CHUNK_BUSY  0x01
#define FBE_MEMORY_STATUS_MASK 0x01
#define FBE_MEMORY_MAX_STACK_DEBUG 1000

/* Memory leak debug flag -- set this to 1 to queue allocated memory chunks and trace chunk allocations
 *   If memory leak, use windbg !list &allocated_memory_chunk_queue_head to list leftover chunks
 *   Then search for chunks in traces to see who allocated it/them.
 */
#define FBE_MEMORY_LEAK_DEBUG_ENABLE 0

typedef enum memory_thread_flag_e{
    MEMORY_THREAD_RUN,
    MEMORY_THREAD_STOP,
    MEMORY_THREAD_DONE
}memory_thread_flag_t;

typedef unsigned int memory_chunk_id_t;



/* Declare our service methods */
fbe_status_t fbe_memory_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_memory_service_methods = {FBE_SERVICE_ID_MEMORY, fbe_memory_control_entry};

typedef struct fbe_memory_service_s{
    fbe_base_service_t	base_service;
}fbe_memory_service_t;

static fbe_memory_service_t memory_service;

static fbe_u32_t            memory_number_of_chunks = 0;	/*!< Overall number of chunks */
static char               * memory_start_address = NULL;	/*!< Start address of first chunk */
static char               * memory_bit_bucket_address = NULL; /*!< Start address of the bit bucket */
static fbe_u8_t           * zero_bit_bucket_address = NULL; /*!< Start address of the bit bucket */
static fbe_u8_t           * unmap_bit_bucket_address = NULL; /*!< Start address of the bit bucket */
static fbe_u8_t           * invalid_pattern_bit_bucket_address = NULL; /*!< Start of invalid pattern address of the bit bucket */
static fbe_queue_head_t	    memory_queue_head;				/*!< Memory queue head for requests */
static fbe_spinlock_t       memory_queue_lock;				/*!< Memory queue lock */

static fbe_u32_t			memory_number_of_free_chunks;	/*!< Number of free chunks */
static fbe_thread_t			memory_thread_handle;			/*!< Thread handle.
                                                                Request completion function may be called
                                                                in the context of this thread.
                                                            */

static fbe_semaphore_t      release_semaphore;				/*!< Used to wake up memory thread */

static fbe_queue_head_t		memory_chunk_queue_head;

#if FBE_MEMORY_LEAK_DEBUG_ENABLE /* See definition for debug use */
static fbe_queue_head_t		allocated_memory_chunk_queue_head;
#endif

static fbe_memory_statistics_t	memory_statistics;

fbe_memory_allocation_function_t memory_allocation_function = NULL; /* SEP will reroute allocation to CMM */
fbe_memory_release_function_t memory_release_function = NULL; /* SEP will reroute release to CMM */

//static CMM_POOL_ID				cmm_control_pool_id;
//static CMM_CLIENT_ID			cmm_client_id;

static memory_thread_flag_t  memory_thread_flag;			/*!< Used for communication with memory_tread */

static fbe_sg_element_t memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_LAST * 2];

static fbe_sg_element_t memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_LAST * 2];

/* zero bit bucket sgl for one 520 size block. */
static fbe_sg_element_t zero_bit_bucket_sgl[2]; /* One for data and one terminator */
static fbe_sg_element_t invalid_pattern_bit_bucket_sgl[2];
static fbe_sg_element_t zero_bit_bucket_sgl_1MB[2];
static fbe_sg_element_t invalid_pattern_bit_bucket_sgl_1MB[2];
static fbe_sg_element_t unmap_bit_bucket_sgl[2];

/* environment_limits. */
static fbe_environment_limits_t memory_env_limits;

/** \fn static void memory_allocate_objects(fbe_memory_request_t * memory_request);
    \details 
        This function iterates on status table, founds free chunks and allocates them

    \param memory_request - pointer to memory request
*/
static void memory_allocate_objects(fbe_memory_request_t * memory_request);


/** \fn static void memory_process_entry(fbe_memory_request_t * memory_request);
    \details 
        This function called from the caller or memory thread context to process single request

    \param memory_request - pointer to memory request
*/
static void memory_process_entry(fbe_memory_request_t * memory_request);

/** \fn static void memory_dispatch_queue(void);
    \details 
        This function called from the memory thread context only
        When the memory chunk released and the memory queue is not empty
        the release semaphore is used to wake up the memory thread.
        Memory thread calls this function to process the first request on memory queue.
*/
static void memory_dispatch_queue(void);

/** \fn static void memory_thread_func(void * context);
    \details 
        The memory thread implementation.
        The threas waits for release semaphore, 
        checks the memory_thread_flag flag and calls memory_dispatch_queue function

    \param context - unused
*/
static void memory_thread_func(void * context);

static void memory_analyze_leaked_memory(void);

static fbe_status_t fbe_memory_zero_bit_bucket_initialize_with_zeroed_blocks(void);

static fbe_status_t fbe_memory_unmap_bit_bucket_initialize_with_zeroed_blocks(void);

/** This function will be used for control code
 *  FBE_MEMORY_SERVICE_CONTROL_CODE_GET_ENV_LIMITS */
static fbe_status_t fbe_memory_control_code_get_env_limits(fbe_packet_t *packet);

static fbe_memory_call_stack_trace_debug_t	fbe_memory_stack_table[FBE_MEMORY_MAX_STACK_DEBUG];
static fbe_u32_t fbe_memory_stack_table_index;


void
memory_service_trace(fbe_trace_level_t trace_level,
             fbe_trace_message_id_t message_id,
             const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&memory_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&memory_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_MEMORY,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}
void fbe_memory_get_mem_mb_for_chunk_count(fbe_u32_t number_of_chunks, fbe_u32_t *memory_use_mb_p)
{
    fbe_u32_t memory_mb = 0;
    memory_mb = number_of_chunks * FBE_MEMORY_CHUNK_SIZE;
    memory_mb = ((memory_mb + (1024*1024) - 1) / (1024*1024));
    *memory_use_mb_p = memory_mb;
}
static fbe_status_t 
fbe_memory_init(fbe_packet_t * packet)
{
//	CMM_ERROR cmm_error;
    EMCPAL_STATUS       nt_status;
    fbe_package_id_t my_package_id;
    fbe_status_t status;
	fbe_u32_t i;
	fbe_u8_t * memory_chunk_ptr = NULL;

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_memory_header_t) <= FBE_MEMORY_HEADER_SIZE);

    /* Transport related asserts */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_packet_t) <= FBE_MEMORY_CHUNK_SIZE); /* Old one */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_packet_t) <= FBE_MEMORY_CHUNK_SIZE_FOR_PACKET); /* New DPS model */

    memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                         "%s entry\n", __FUNCTION__);


	fbe_get_package_id(&my_package_id);

    if(memory_number_of_chunks == 0){
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: memory_number_of_chunks not initialized\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_service_init((fbe_base_service_t *) &memory_service);
    fbe_base_service_set_service_id((fbe_base_service_t *)&memory_service, FBE_SERVICE_ID_MEMORY);
    fbe_base_service_set_trace_level((fbe_base_service_t *)&memory_service, fbe_trace_get_default_trace_level());

    /* Check if pool already initialized */
    if(memory_start_address){
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: already initialized\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* All chunks is free now */
    memory_number_of_free_chunks = memory_number_of_chunks;

    fbe_get_package_id(&my_package_id);
	if (my_package_id == FBE_PACKAGE_ID_SEP_0){
		memory_service_trace(FBE_TRACE_LEVEL_INFO, 
							   FBE_TRACE_MESSAGE_ID_INFO, 
							   "MCRMEM: MemService: %d \n", memory_number_of_chunks * FBE_MEMORY_CHUNK_SIZE);
	}

    if(memory_allocation_function == NULL){
        memory_start_address = fbe_allocate_contiguous_memory(memory_number_of_chunks  * (FBE_MEMORY_CHUNK_SIZE + sizeof(fbe_queue_element_t)));
    } else {
        memory_start_address = memory_allocation_function(memory_number_of_chunks  * (FBE_MEMORY_CHUNK_SIZE + sizeof(fbe_queue_element_t)));
    }
    /* Check if memory_start_address is not NULL */
    if(memory_start_address == NULL){
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: entry 2 cmmGetMemoryVA fail (memory_start_address == NULL) \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(memory_allocation_function == NULL) {
        memory_bit_bucket_address = fbe_allocate_contiguous_memory(FBE_MEMORY_BIT_BUCKET_BYTES); /*!< 4K allocation */
    } else {
        memory_bit_bucket_address = memory_allocation_function(FBE_MEMORY_BIT_BUCKET_BYTES); /*!< 4K allocation */
    }

    /* Check for memory bit address */
    if(memory_bit_bucket_address == NULL){
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: entry 2 cmmGetMemoryVA fail (memory_bit_bucket_address == NULL) \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	if(my_package_id == FBE_PACKAGE_ID_SEP_0){
		if(memory_allocation_function == NULL) {
			zero_bit_bucket_address = fbe_allocate_contiguous_memory(FBE_MEMORY_ZERO_BIT_BUCKET_SIZE); /*!< 0x800 blocks allocation */
		} else {
			zero_bit_bucket_address = memory_allocation_function(FBE_MEMORY_ZERO_BIT_BUCKET_SIZE); /*!< 0x800 blocks allocation */
		}

		/* Check zero bit address */
		if(zero_bit_bucket_address == NULL){
			memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								 "%s: entry 2 cmmGetMemoryVA fail (zero_bit_bucket_address == NULL) \n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_GENERIC_FAILURE;
		}
    
		/* initialize the zero bit bucket with zeroed buffer. */
		status = fbe_memory_zero_bit_bucket_initialize_with_zeroed_blocks();
		if(status != FBE_STATUS_OK)
		{
			memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								 "%s: entry 2 cmmGetMemoryVA fail (zero buffer init failed. == NULL) \n", __FUNCTION__);
			fbe_transport_set_status(packet, status, 0);
			fbe_transport_complete_packet(packet);
			return status;
		}

		if(memory_allocation_function == NULL) {
			invalid_pattern_bit_bucket_address = fbe_allocate_contiguous_memory(FBE_MEMORY_INVALID_PATTERN_BIT_BUCKET_SIZE); /*!< 0x800 blocks allocation */
		} else {
			invalid_pattern_bit_bucket_address = memory_allocation_function(FBE_MEMORY_INVALID_PATTERN_BIT_BUCKET_SIZE); /*!< 0x800 blocks allocation */
		}

		/* Check for the invalidate bit pattern */
		if(invalid_pattern_bit_bucket_address == NULL){
			memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								 "%s: entry 2 cmmGetMemoryVA fail (invalid_pattern_bit_bucket_address == NULL) \n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_GENERIC_FAILURE;
		}

        if(memory_allocation_function == NULL) {
			unmap_bit_bucket_address = fbe_allocate_contiguous_memory(FBE_4K_BYTES_PER_BLOCK); /*!< 1 blocks allocation */
		} else {
			unmap_bit_bucket_address = memory_allocation_function(FBE_4K_BYTES_PER_BLOCK); /*!< 1 blocks allocation */
		}

		/* Check zero bit address */
		if(unmap_bit_bucket_address == NULL){
			memory_service_trace(FBE_TRACE_LEVEL_ERROR,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								 "%s: entry 2 cmmGetMemoryVA fail (unmap_bit_bucket_address == NULL) \n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_GENERIC_FAILURE;
		}
    
		/* initialize the unmap bit bucket with zeroed buffer. */
        status = fbe_memory_unmap_bit_bucket_initialize_with_zeroed_blocks();

		/* Initialize memory_bit_bucket_sgl_array */
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512].address = memory_bit_bucket_address;
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512].count = 512;
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512+1].address = NULL;
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512+1].count = 0;

		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520*2].address = memory_bit_bucket_address;
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520*2].count = 520;
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520*2+1].address = NULL;
		memory_bit_bucket_sgl_array[FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520*2+1].count = 0;

		/* Initialize the zero_bit_bucket_sgl_array */
		zero_bit_bucket_sgl[0].address = zero_bit_bucket_address;
		zero_bit_bucket_sgl[0].count = FBE_BE_BYTES_PER_BLOCK;
		zero_bit_bucket_sgl[1].address = NULL;
		zero_bit_bucket_sgl[1].count = 0;

		/* Initialize the invalid_pattern_bit_bucket_sgl_array */
		invalid_pattern_bit_bucket_sgl[0].address = invalid_pattern_bit_bucket_address;
		invalid_pattern_bit_bucket_sgl[0].count = FBE_BE_BYTES_PER_BLOCK;
		invalid_pattern_bit_bucket_sgl[1].address = NULL;
		invalid_pattern_bit_bucket_sgl[1].count = 0;


		/* Initialize the zero_bit_bucket_sgl_array */
		zero_bit_bucket_sgl_1MB[0].address = zero_bit_bucket_address;
		zero_bit_bucket_sgl_1MB[0].count = FBE_MEMORY_ZERO_BIT_BUCKET_SIZE;
		zero_bit_bucket_sgl_1MB[1].address = NULL;
		zero_bit_bucket_sgl_1MB[1].count = 0;

		/* Initialize the invalid_pattern_bit_bucket_sgl_array */
		invalid_pattern_bit_bucket_sgl_1MB[0].address = invalid_pattern_bit_bucket_address;
		invalid_pattern_bit_bucket_sgl_1MB[0].count = FBE_MEMORY_INVALID_PATTERN_BIT_BUCKET_SIZE;
		invalid_pattern_bit_bucket_sgl_1MB[1].address = NULL;
		invalid_pattern_bit_bucket_sgl_1MB[1].count = 0;

        /* Initialize the zero_bit_bucket_sgl_array */
		unmap_bit_bucket_sgl[0].address = unmap_bit_bucket_address;
		unmap_bit_bucket_sgl[0].count = FBE_4K_BYTES_PER_BLOCK;
		unmap_bit_bucket_sgl[1].address = NULL;
		unmap_bit_bucket_sgl[1].count = 0;


	} /* if(my_package_id == FBE_PACKAGE_ID_SEP_0) */



    /* Step 4 init statistics */
    memory_statistics.total_chunks = memory_number_of_chunks;
    memory_statistics.free_chunks = memory_number_of_chunks;
    memory_statistics.total_alloc_count = 0;
    memory_statistics.total_release_count = 0;
    memory_statistics.total_push_count = 0;
    memory_statistics.total_pop_count = 0;
    memory_statistics.current_queue_depth = 0;
    memory_statistics.max_queue_depth = 0;

    memory_statistics.native_alloc_count = memory_statistics.native_release_count = memory_statistics.false_release_count = 0;
    fbe_memory_stack_table_index = 0;
    fbe_zero_memory(fbe_memory_stack_table, sizeof(fbe_memory_stack_table));

 
    /* Step 5. Zero chunks memory */
    fbe_zero_memory(memory_start_address, memory_number_of_chunks  * (FBE_MEMORY_CHUNK_SIZE + sizeof(fbe_queue_element_t)));

    fbe_queue_init(&memory_chunk_queue_head);

#if FBE_MEMORY_LEAK_DEBUG_ENABLE /* See definition for debug use */
    fbe_queue_init(&allocated_memory_chunk_queue_head);
#endif

	memory_chunk_ptr = memory_start_address;
	/* Fill in the memory chunk queue */
	for(i = 0 ; i < memory_number_of_chunks; i++){
		fbe_queue_element_t * queue_element;
		queue_element = (fbe_queue_element_t *)memory_chunk_ptr;
		fbe_queue_element_init(queue_element);
		fbe_queue_push(&memory_chunk_queue_head, queue_element);
		memory_chunk_ptr += sizeof(fbe_queue_element_t) + FBE_MEMORY_CHUNK_SIZE;
	}
	

    /* Initialize memory_queue */
    fbe_queue_init(&memory_queue_head);
    fbe_spinlock_init(&memory_queue_lock);

    memory_thread_flag = MEMORY_THREAD_RUN;    
    
    fbe_semaphore_init(&release_semaphore, 0, memory_number_of_chunks);

    /* Start memory thread */
    nt_status = fbe_thread_init(&memory_thread_handle, "fbe_mem_thr", memory_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: fbe_thread_init fail\n", __FUNCTION__);
    }

    if (my_package_id == FBE_PACKAGE_ID_SEP_0)
    {
        fbe_memory_dps_init(FBE_TRUE /* retry forever */);
        fbe_memory_persistence_init();
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_memory_destroy(fbe_packet_t * packet)
{
    fbe_package_id_t my_package_id;
    
    memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                         "%s entry\n", __FUNCTION__);

    fbe_get_package_id(&my_package_id);
    if (my_package_id == FBE_PACKAGE_ID_SEP_0){
        fbe_memory_dps_destroy();
        fbe_memory_persistence_destroy();
    }

    if(memory_number_of_free_chunks != memory_number_of_chunks) {
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: We have leak of %d chunks \n", __FUNCTION__, memory_number_of_chunks - memory_number_of_free_chunks);
        memory_analyze_leaked_memory();
        fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_BUSY;
    }
    /* There are issues in ESP with memory leaks.  We will just disable this check for now in order to help stabilize all tests
     * We will still fail in free builds or for other packages.
     */
    if( (memory_statistics.native_alloc_count != 0) 
#if defined(SIMMODE_ENV)
         && (my_package_id != FBE_PACKAGE_ID_ESP)
#endif
        ) {
        memory_analyze_native_leak(fbe_memory_stack_table, FBE_MEMORY_MAX_STACK_DEBUG);/*will work in user space only*/

        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: We have leak of %d native allocations \n", __FUNCTION__, 
                             (int)memory_statistics.native_alloc_count);


        fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_BUSY;
    }

    if (memory_statistics.false_release_count != 0) {
            memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: %d attempts were made to free native memory which was not allocated\n", __FUNCTION__, 
                                 (int)memory_statistics.false_release_count);

        fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_BUSY;

    }

    fbe_base_service_destroy((fbe_base_service_t *) &memory_service);

    memory_thread_flag = MEMORY_THREAD_STOP; 

    fbe_semaphore_release(&release_semaphore, 0, 1, FALSE);

    fbe_thread_wait(&memory_thread_handle);
    fbe_thread_destroy(&memory_thread_handle);

    fbe_semaphore_destroy(&release_semaphore);

    fbe_queue_destroy(&memory_chunk_queue_head);

#if FBE_MEMORY_LEAK_DEBUG_ENABLE /* See definition for debug use */
    fbe_queue_destroy(&allocated_memory_chunk_queue_head);
#endif

    if(memory_start_address != NULL){
        //cmmFreeMemoryVA(cmm_client_id, memory_start_address);
        if(memory_release_function == NULL){
            fbe_release_contiguous_memory(memory_start_address);
        } else {
            memory_release_function(memory_start_address);
        }
        memory_start_address = NULL;
    }

    if(memory_bit_bucket_address != NULL){
        //cmmFreeMemoryVA(cmm_client_id, memory_start_address);
        if(memory_release_function == NULL){
            fbe_release_contiguous_memory(memory_bit_bucket_address);
        } else {
            memory_release_function(memory_bit_bucket_address);
        }    
        memory_bit_bucket_address = NULL;
    }

    if(zero_bit_bucket_address != NULL){
        //cmmFreeMemoryVA(cmm_client_id, memory_start_address);
        if(memory_release_function == NULL){
            fbe_release_contiguous_memory(zero_bit_bucket_address);
        } else {
            memory_release_function(zero_bit_bucket_address);
        }    
        zero_bit_bucket_address = NULL;
    }

    if(invalid_pattern_bit_bucket_address != NULL){
        //cmmFreeMemoryVA(cmm_client_id, memory_start_address);
        if(memory_release_function == NULL){
            fbe_release_contiguous_memory(invalid_pattern_bit_bucket_address);
        } else {
            memory_release_function(invalid_pattern_bit_bucket_address);
        }
        invalid_pattern_bit_bucket_address = NULL;
    }

    memory_number_of_chunks = 0;
    
    fbe_spinlock_destroy(&memory_queue_lock);

//	cmmDeregisterClient(cmm_client_id);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_memory_init_number_of_chunks(fbe_memory_number_of_objects_t number_of_chunks)
{   
    if(memory_number_of_chunks != 0) {/* Multiple initialization not supported */
        memory_service_trace(FBE_TRACE_LEVEL_WARNING,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: usert tries to set number of chunks, we already have %X\n", __FUNCTION__,memory_number_of_chunks);

        return FBE_STATUS_OK;
    }

    memory_number_of_chunks = number_of_chunks;
    memory_service_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Initializing %d memory chunks\n", 
                         __FUNCTION__, memory_number_of_chunks);

    return FBE_STATUS_OK;
}    

void fbe_memory_allocate(fbe_memory_request_t * memory_request)
{
    if(memory_request->number_of_objects > FBE_MEMORY_MAX_CHUNKS_PER_REQUEST){
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Invalid  number_of_objects %X\n", __FUNCTION__, memory_request->number_of_objects);
        memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ERROR;
        memory_request->ptr = NULL;
        memory_request->data_ptr = NULL;
        memory_request->completion_function( memory_request, memory_request->completion_context);
        return;
    }

    /* Lock memory queue */
    fbe_spinlock_lock(&memory_queue_lock);

    /* If queue is empty and we have a memory we just call completion function */
    /* We need to take table_lock to protect memory_number_of_free_chunks */

    if(fbe_queue_is_empty(&memory_queue_head) && (memory_number_of_free_chunks >= memory_request->number_of_objects)) {
        memory_process_entry(memory_request);

        fbe_spinlock_unlock(&memory_queue_lock);

        memory_request->completion_function( memory_request, memory_request->completion_context);
    } else{    
        /* Queue the request */
        /*csx_p_print_native( "memory_allocate_object: Queue the request\n");*/
        /* Update statistics */
        memory_statistics.total_push_count++;
        memory_statistics.current_queue_depth++;
        if(memory_statistics.current_queue_depth > memory_statistics.max_queue_depth){
            memory_statistics.max_queue_depth = memory_statistics.current_queue_depth;
        }

        fbe_queue_push(&memory_queue_head, (fbe_queue_element_t * )memory_request);
        fbe_spinlock_unlock(&memory_queue_lock);
    }
}

void fbe_memory_release(void * ptr)
{
	fbe_queue_element_t * queue_element = (fbe_queue_element_t *)(((char *)ptr) - sizeof(fbe_queue_element_t));

    fbe_spinlock_lock(&memory_queue_lock);

#if FBE_MEMORY_LEAK_DEBUG_ENABLE /* See definition for debug use */
    fbe_queue_remove(queue_element); 
#endif

	fbe_queue_push(&memory_chunk_queue_head, queue_element); 
	memory_number_of_free_chunks++;

    if(!fbe_queue_is_empty(&memory_queue_head)) {
        fbe_semaphore_release(&release_semaphore, 0, 1, FALSE);
    }

    fbe_spinlock_unlock(&memory_queue_lock);

    return ;
}

static void memory_dispatch_queue(void)
{
    fbe_memory_request_t * memory_request;

    fbe_spinlock_lock(&memory_queue_lock);

    if(!fbe_queue_is_empty(&memory_queue_head)) {
        memory_request = (fbe_memory_request_t *)fbe_queue_front(&memory_queue_head);
        if(memory_number_of_free_chunks >= memory_request->number_of_objects){
            /* Update statistics */
            memory_statistics.current_queue_depth --;
            memory_statistics.total_pop_count++;

            fbe_queue_pop(&memory_queue_head);
            memory_process_entry(memory_request);

            fbe_spinlock_unlock(&memory_queue_lock);

            memory_request->completion_function(memory_request, memory_request->completion_context); 
            return;
        }
    }

    fbe_spinlock_unlock(&memory_queue_lock);    
}

/* The caller MUST hold memory_queue_lock to serialize access */
static void
memory_allocate_objects(fbe_memory_request_t * memory_request)
{
    fbe_u32_t i;    
    void ** chunk_array;
	fbe_queue_element_t * queue_element;

    for(i = 0; i < memory_request->number_of_objects; i++) {
		queue_element = fbe_queue_pop(&memory_chunk_queue_head);
		if(queue_element == NULL){
			memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
							 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							 "%s Out of memory \n", __FUNCTION__);
			break;
		}
#if FBE_MEMORY_LEAK_DEBUG_ENABLE /* See definition for debug use */
        fbe_queue_push(&allocated_memory_chunk_queue_head, queue_element);

        memory_service_trace(FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "Memory leak debug - allocating queue element 0x%llx\n",
                             queue_element);
#endif

        if(i == 0) { /** This is a first allocated chunk */            
            memory_request->ptr = (fbe_memory_ptr_t)(((fbe_u8_t *)queue_element) + sizeof(fbe_queue_element_t));
            chunk_array = memory_request->ptr;
        }
        chunk_array[i] = (void *)((((fbe_u8_t *)queue_element) + sizeof(fbe_queue_element_t)));
    }

	memory_number_of_free_chunks -= memory_request->number_of_objects;
}

static void memory_process_entry(fbe_memory_request_t * memory_request)
{
    switch(memory_request->request_state) {
    case FBE_MEMORY_REQUEST_STATE_ALLOCATE_OBJECT:
    case FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET:
    case FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER:
        memory_allocate_objects(memory_request);
        break;
    
   default:
		memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
							 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							 "%s Uknown request state %d \n", __FUNCTION__, memory_request->request_state);
        break;
    }
}

static void memory_thread_func(void * context)
{
    EMCPAL_STATUS nt_status;

    FBE_UNREFERENCED_PARAMETER(context);
    
    memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                         "%s entry\n", __FUNCTION__);

    while(1)    
    {
        nt_status = fbe_semaphore_wait(&release_semaphore, NULL);
        if(memory_thread_flag == MEMORY_THREAD_RUN) {
            memory_dispatch_queue();
        } else {
            break;
        }
    }

    memory_thread_flag = MEMORY_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

void memory_get_statistics(fbe_memory_statistics_t * stat)
{
    if (stat == NULL){
      return;
    }
    
    stat->total_chunks = memory_statistics.total_chunks;
    stat->free_chunks = memory_statistics.free_chunks;
    stat->total_alloc_count = memory_statistics.total_alloc_count;
    stat->total_release_count = memory_statistics.total_release_count;
    stat->total_push_count = memory_statistics.total_push_count;
    stat->total_pop_count = memory_statistics.total_pop_count;
    stat->current_queue_depth = memory_statistics.current_queue_depth;
    stat->max_queue_depth = memory_statistics.max_queue_depth;
    /* TODO: Add native_alloc_count & native_release_count.
     */
}

/*!***************************************************************************
 *          fbe_memory_get_dps_stats()
 ***************************************************************************** 
 * 
 * @brief   The function first fill the dps statistics with the existing counters
 *          in dps and then return this structure in a control buffer.
 *
 * @param   packet       -packet.
 *
 * @return  fbe_status_t
 *
 * @author
 *  04/10/2011  Swati Fursule - Created
 *
 *****************************************************************************/
static fbe_status_t
fbe_memory_get_dps_stats(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_memory_dps_statistics_t *memory_stats_buffer_p = NULL;
    //fbe_memory_dps_statistics_t memory_stats = {0};
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_u32_t*)&memory_stats_buffer_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_memory_dps_statistics_t)) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //fbe_memory_fill_dps_statistics(&memory_stats);
    fbe_zero_memory(memory_stats_buffer_p, sizeof(fbe_memory_dps_statistics_t));
    fbe_memory_fill_dps_statistics(memory_stats_buffer_p);

    /* Need to get the lock first then copy the data to the buffer
     * passed in
     */
    //fbe_copy_memory(memory_stats_buffer_p, &memory_stats, buffer_length);   // dst,src,length
    
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_memory_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_memory_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &memory_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_memory_destroy( packet);
            break;

        case FBE_MEMORY_SERVICE_CONTROL_CODE_GET_DPS_STATS:
            status = fbe_memory_get_dps_stats( packet);
            break;

        case FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_ADD_MEMORY:
            status = fbe_memory_dps_add_memory( packet);
            break;

        case FBE_MEMORY_SERVICE_CONTROL_CODE_GET_ENV_LIMITS:
            status = fbe_memory_control_code_get_env_limits(packet);
            break;
            
        case FBE_MEMORY_SERVICE_CONTROL_CODE_PERSIST_SET_PARAMS:
            status = fbe_persistent_memory_control_entry(packet);
            break;

        case FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_REDUCE_SIZE:
            status = fbe_memory_dps_reduce_size( packet);
            break;

        default:
            memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: Unknown control code 0x%X\n", __FUNCTION__, control_code);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_complete_packet(packet);
            break;
    }

    return status;
}

static void 
memory_analyze_leaked_memory(void)
{

}

void * fbe_memory_native_allocate(fbe_u32_t allocation_size_in_bytes)
{
    /* NOTE: This memory allocation may happen from CMM for later versions. */

    void * pmemory = NULL;

    pmemory = fbe_allocate_contiguous_memory(allocation_size_in_bytes);

    if (pmemory != NULL)
    {
        /* TODO: Actual allocation is rounded up to the the nearest system page size multiple.
         * Calculate this and zero memory accordingly.
         */
        fbe_zero_memory(pmemory,allocation_size_in_bytes);
        fbe_spinlock_lock(&memory_queue_lock);

        /*no op in kernel, but in user helps us debug callers for memory leaks*/
        ADD_TO_BACKTRACE_TABLE(fbe_memory_stack_table,
                               pmemory,
                               allocation_size_in_bytes,
                               fbe_memory_stack_table_index,
                               FBE_MEMORY_MAX_STACK_DEBUG)

        memory_statistics.native_alloc_count++;
        fbe_spinlock_unlock(&memory_queue_lock); 
    }

    return pmemory;
}

void fbe_memory_native_release(void * ptr)
{

    /* NOTE: This memory used may be from CMM for later versions. */
    if (ptr == NULL)
    {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Invalid address %p.\n", __FUNCTION__, ptr);

        fbe_spinlock_lock(&memory_queue_lock);
        memory_statistics.false_release_count++;
        fbe_spinlock_unlock(&memory_queue_lock);
    }
    else
    {
        REMOVE_FROM_BACKTRACE_TABLE(fbe_memory_stack_table, ptr, fbe_memory_stack_table_index, FBE_MEMORY_MAX_STACK_DEBUG)
        fbe_release_contiguous_memory(ptr); 
        fbe_spinlock_lock(&memory_queue_lock);
        memory_statistics.native_release_count++;
        memory_statistics.native_alloc_count--;
        fbe_spinlock_unlock(&memory_queue_lock);
    }

    return;
}

/* AJTODO: Temporary hack. Remove... This routine will zero out the memroy it created*/
void * fbe_memory_ex_allocate(fbe_u32_t allocation_size_in_bytes)
{
    /* NOTE: This memory allocation may happen from CMM for later versions. */

    void * pmemory = NULL;

    pmemory = fbe_allocate_io_nonpaged_pool_with_tag ( allocation_size_in_bytes, 'mebf');

    if (pmemory != NULL)
    {
        /* TODO: Actual allocation is rounded up to the the nearest system page size multiple.
         * Calculate this and zero memory accordingly.
         */
        fbe_zero_memory(pmemory,allocation_size_in_bytes);
        fbe_spinlock_lock(&memory_queue_lock);

        /*no op in kernel, but in user helps us debug callers for memory leaks*/
        ADD_TO_BACKTRACE_TABLE(fbe_memory_stack_table,
                               pmemory,
                               allocation_size_in_bytes,
                               fbe_memory_stack_table_index,
                               FBE_MEMORY_MAX_STACK_DEBUG)

        memory_statistics.native_alloc_count++;
        fbe_spinlock_unlock(&memory_queue_lock); 
    }

    return pmemory;
}
/* AJTODO: Temporary hack. Remove...*/
void fbe_memory_ex_release(void * ptr)
{

    /* NOTE: This memory used may be from CMM for later versions. */
    if (ptr == NULL)
    {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: Invalid address %p.\n", __FUNCTION__, ptr);

        fbe_spinlock_lock(&memory_queue_lock);
        memory_statistics.false_release_count++;
        fbe_spinlock_unlock(&memory_queue_lock);
    }
    else
    {
        REMOVE_FROM_BACKTRACE_TABLE(fbe_memory_stack_table, ptr, fbe_memory_stack_table_index, FBE_MEMORY_MAX_STACK_DEBUG)
        fbe_release_io_nonpaged_pool_with_tag(ptr,'mebf'); 
        fbe_spinlock_lock(&memory_queue_lock);
        memory_statistics.native_release_count++;
        memory_statistics.native_alloc_count--;
        fbe_spinlock_unlock(&memory_queue_lock);
    }

    return;
}

fbe_sg_element_t * 
fbe_memory_get_bit_bucket_sgl(fbe_memory_bit_bucket_sgl_type_t memory_bit_bucket_sgl_type)
{
    return &memory_bit_bucket_sgl_array[memory_bit_bucket_sgl_type * 2];
}
fbe_u8_t * 
fbe_memory_get_bit_bucket_addr(void)
{
    return memory_bit_bucket_address;
}

fbe_u32_t 
fbe_memory_get_bit_bucket_size(void)
{
    return FBE_MEMORY_BIT_BUCKET_BYTES;
}

fbe_sg_element_t * 
fbe_zero_get_bit_bucket_sgl(void)
{
    return &zero_bit_bucket_sgl[0];
}

fbe_sg_element_t * 
fbe_invalid_pattern_get_bit_bucket_sgl(void)
{
    return &invalid_pattern_bit_bucket_sgl[0];
}

fbe_sg_element_t * 
fbe_zero_get_bit_bucket_sgl_1MB(void)
{
	return &zero_bit_bucket_sgl_1MB[0];
}

fbe_sg_element_t * 
fbe_invalid_pattern_get_bit_bucket_sgl_1MB(void)
{
	return &invalid_pattern_bit_bucket_sgl_1MB[0];
}

fbe_sg_element_t * 
fbe_unmap_get_bit_bucket_sgl(void)
{
	return &unmap_bit_bucket_sgl[0];
}

fbe_status_t
fbe_memory_unmap_bit_bucket_initialize_with_zeroed_blocks(void)
{
    /* zero out the buffer. */
    fbe_zero_memory(unmap_bit_bucket_address, FBE_4K_BYTES_PER_BLOCK);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_generate_valid_zero_buffer(void)
{
    fbe_u16_t           metadata_size = 0;
    fbe_u64_t           zero_metadata = 0x7fff5eed;
    fbe_u8_t *          data_buffer_p = NULL;
    fbe_block_count_t   block_count ;

    /*!@note zero bit bucket address is already zeroed buffer. */
    data_buffer_p = zero_bit_bucket_address;
    if(data_buffer_p == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get the number of blocks from the zero bit bucket size. */
    block_count = FBE_MEMORY_BLOCKS_PER_BUCKET;
    metadata_size = 8;

    /* Fill out the data buffer with valid zero buffer. */
    while(block_count != 0)
    {
        if(data_buffer_p == NULL)
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }

        data_buffer_p += FBE_BYTES_PER_BLOCK;
        fbe_copy_memory(data_buffer_p, (void *) &zero_metadata, metadata_size);
        data_buffer_p += metadata_size;
        block_count--;
    }

    /* return good status. */
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_memory_zero_bit_bucket_initialize_with_zeroed_blocks(void)
{
    fbe_status_t status;

    /* zero out the buffer. */
    fbe_zero_memory(zero_bit_bucket_address, FBE_MEMORY_ZERO_BIT_BUCKET_SIZE);
    status = fbe_memory_generate_valid_zero_buffer();
    return status;
}

fbe_status_t
fbe_memory_get_zero_bit_bucket(fbe_u8_t ** zero_bit_bucket_p, fbe_u32_t * zero_bit_bucket_size_in_blocks_p)
{
    /* if zero bit bucket address is not initialized then return error. */
    if(zero_bit_bucket_address == NULL) 
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* return address of zero bit bucket and its size. */
    *zero_bit_bucket_p = zero_bit_bucket_address;
    *zero_bit_bucket_size_in_blocks_p = FBE_MEMORY_BLOCKS_PER_BUCKET;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_get_zero_bit_bucket_size_in_blocks(fbe_u32_t * zero_bit_bucket_size_in_blocks_p)
{
    *zero_bit_bucket_size_in_blocks_p = FBE_MEMORY_BLOCKS_PER_BUCKET;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_get_invalid_pattern_bit_bucket(fbe_u8_t ** invalid_pattern_bit_bucket_p, fbe_block_count_t * invalid_pattern_bit_bucket_size_in_blocks_p)
{
    /* if invalid pattern bit bucket address is not initialized then return error. */
    if(invalid_pattern_bit_bucket_address == NULL) 
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* return address of invalid pattern bit bucket and its size. */
    *invalid_pattern_bit_bucket_p = invalid_pattern_bit_bucket_address;
    *invalid_pattern_bit_bucket_size_in_blocks_p = FBE_MEMORY_INVALID_PATTERN_BIT_BUCKET_SIZE / FBE_BE_BYTES_PER_BLOCK;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_memory_get_invalid_pattern_bit_bucket_size_in_blocks(fbe_u32_t * invalid_pattern_bit_bucket_size_in_blocks_p)
{
    *invalid_pattern_bit_bucket_size_in_blocks_p = FBE_MEMORY_INVALID_PATTERN_BIT_BUCKET_SIZE / FBE_BE_BYTES_PER_BLOCK;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_memory_set_memory_functions(fbe_memory_allocation_function_t allocation_function,
                                             fbe_memory_release_function_t release_function)
{
    memory_allocation_function = allocation_function;
    memory_release_function = release_function;
    return FBE_STATUS_OK;
}

fbe_memory_allocation_function_t fbe_memory_get_allocation_function(void)
{
    return memory_allocation_function;
}

fbe_memory_release_function_t fbe_memory_get_release_function(void)
{
    return memory_release_function;
}

void fbe_memory_set_env_limits(fbe_environment_limits_t* env_limits)
{
    fbe_copy_memory(&memory_env_limits, env_limits, sizeof(fbe_environment_limits_t));
}

void fbe_memory_get_env_limits(fbe_environment_limits_t* env_limits)
{
    fbe_copy_memory(env_limits, &memory_env_limits, sizeof(fbe_environment_limits_t));
}

/*!***************************************************************************
 *          fbe_memory_control_code_get_env_limits()
 ***************************************************************************** 
 * 
 * @brief   The function first fill the env_limits and
 *          return this structure in a control buffer.
 *
 * @param   packet       -packet.
 *
 * @return  fbe_status_t
 *
 * @author
 *  06/28/2012  Vera Wang - Created
 *
 *****************************************************************************/
static fbe_status_t
fbe_memory_control_code_get_env_limits(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_environment_limits_t * env_limits = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_u32_t*)&env_limits);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_environment_limits_t)) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(env_limits, sizeof(fbe_environment_limits_t));
    fbe_memory_get_env_limits(env_limits);
    
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

void * fbe_memory_allocate_required(fbe_u32_t allocation_size_in_bytes)
{
    void * pmemory = NULL;
    fbe_memory_allocation_function_t memory_allocation_function = NULL;

    memory_allocation_function = fbe_memory_get_allocation_function();
    if(memory_allocation_function != NULL) 
    {
        pmemory = memory_allocation_function(allocation_size_in_bytes);
    } 
    else 
    {
        pmemory = fbe_memory_native_allocate(allocation_size_in_bytes);
    }

    return pmemory;
}

void fbe_memory_release_required(void * ptr)
{
    fbe_memory_release_function_t memory_release_function = NULL;

    memory_release_function = fbe_memory_get_release_function();
    if(memory_release_function != NULL)
    {
        memory_release_function(ptr);
    } 
    else 
    {
        fbe_memory_native_release(ptr);
    }

    return;
}


