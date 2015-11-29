#ifndef FBE_MEMORY_H
#define FBE_MEMORY_H

/*#include "fbe_base_service.h"*/
#include "csx_ext.h"
#include "fbe_queue.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_platform.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_environment_limit.h"

/*
C S X  - PLEASE, DO not change the size of this structure - Ever without consulting the MCR group (Peter Puhov) this has big effect on us
*/
#ifdef CSX_BV_ENVIRONMENT_LINUX
#define FBE_MEMORY_CHUNK_SIZE (4288)
#else
//#define FBE_MEMORY_CHUNK_SIZE (4096)
/* To support 4K drives we want to align with 520 * 8 = 4160 */
#define FBE_MEMORY_CHUNK_SIZE (4160)
#endif

/* The chunk size should always be aligned to 16 bytes per Peter Puhov. */
#define FBE_MEMORY_CHUNK_ALIGN_BYTES 16

#define FBE_MEMORY_MAX_CHUNKS_PER_REQUEST 16

typedef enum fbe_memory_chunk_size_e{
	FBE_MEMORY_CHUNK_SIZE_FOR_PACKET = FBE_MEMORY_CHUNK_SIZE, /* 4K */	
	FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO = 520 * (64 + 1), /* + 1 : See: fbe_raid_buffer_32kb_pool_entry_t when RAID_DBG_MEMORY_ENABLED is set */ 
    FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO = 520 + 24 + 48 + 48 , /* 520 + 2 SG elements + DCA arg structure + padding to be 64 bytes aligned*/

	/* The following sizes would describe both data and control memory */
	FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET = 1, 
	FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET = 2, 
	FBE_MEMORY_CHUNK_SIZE_PACKET_64_BLOCKS = 3, 
	FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS = 4, 
}fbe_memory_chunk_size_t;

typedef enum fbe_memory_chunk_size_block_count{
    FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET = 4,
    FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 = 64,
    FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_1 = 1
}fbe_memory_chunk_size_block_count_t;


enum fbe_memory_dps_constants_e{
	FBE_MEMORY_DPS_HEADER_SIZE = 64, /* In fact it is only 40  (34 headers in one FBE_MEMORY_CHUNK_SIZE_FOR_PACKET)*/
	FBE_MEMORY_DPS_64_BLOCKS_IO_PER_MAIN_CHUNK = 31,
	
	FBE_MEMORY_DPS_MAX_PRIORITY = 50, /* see fbe_memory_dps_object_base_priority_e */	
};


typedef enum fbe_memory_request_state_e {
    FBE_MEMORY_REQUEST_STATE_INVALID,
	FBE_MEMORY_REQUEST_STATE_INITIALIZED,
    FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET,
    FBE_MEMORY_REQUEST_STATE_ALLOCATE_OBJECT,
    FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER,
	FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK,
	FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_SYNC, /* Do not call completion function if completed immediatelly */
	FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED,
	FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY,
	FBE_MEMORY_REQUEST_STATE_ABORTED,
	FBE_MEMORY_REQUEST_STATE_DESTROYED,
    FBE_MEMORY_REQUEST_STATE_ERROR,

    FBE_MEMORY_REQUEST_STATE_LAST
}fbe_memory_request_state_t;

enum fbe_memory_dps_request_flags_e{
	FBE_MEMORY_REQUEST_FLAG_FAST_POOL		= 0x00000001,
	FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL	= 0x00000002,
	FBE_MEMORY_REQUEST_FLAG_DATA_POOL   	= 0x00000004,

	FBE_MEMORY_REQUEST_FLAG_BALANCED	   	= 0x00000100, /* This flag indicates that memory come from different pool
															balance counter should be increased and flag needs to be cleared */

};

typedef fbe_u32_t fbe_memory_dps_request_flags_t;

/* Number assigned to each enum indicates resource allocation base priority for
 * corresponding objects. Number of priorities reserved for each object is based 
 * on number of discreet memory allocations that can happen in the context of 
 * that object. For example, Raid and VD both have 8 priorities reserved.   
 */
enum fbe_memory_dps_object_base_priority_e{
    FBE_MEMORY_DPS_RDGEN_BASE_PRIORITY = 0,
    FBE_MEMORY_DPS_BVD_BASE_PRIORITY = 0,
    FBE_MEMORY_DPS_LUN_BASE_PRIORITY = 0,
    FBE_MEMORY_DPS_STRIPER_ABOVE_MIRROR_BASE_PRIORITY = 6,
    FBE_MEMORY_DPS_RAID_BASE_PRIORITY = 14,
    FBE_MEMORY_DPS_VD_BASE_PRIORITY = 25,
    FBE_MEMORY_DPS_PVD_BASE_PRIORITY = 33,
    FBE_MEMORY_DPS_UNDEFINED_PRIORITY = 0xFFFFFFFF,
};
typedef enum fbe_memory_dps_queue_id_e{
	FBE_MEMORY_DPS_QUEUE_ID_MAIN,
	FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET,
	FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO,
    FBE_MEMORY_DPS_QUEUE_ID_FOR_1_BLOCK_IO,
	FBE_MEMORY_DPS_QUEUE_ID_LAST,
	FBE_MEMORY_DPS_QUEUE_ID_INVALID,
}fbe_memory_dps_queue_id_t;

typedef fbe_u32_t fbe_memory_request_priority_t;

typedef fbe_u64_t fbe_memory_io_stamp_t;
#define FBE_MEMORY_REQUEST_IO_STAMP_INVALID 0x00FFFFFFFFFFFFFF

struct fbe_memory_request_s;
typedef void * fbe_memory_completion_context_t;
typedef void (* fbe_memory_completion_function_t)(struct fbe_memory_request_s * memory_request, fbe_memory_completion_context_t context);
typedef unsigned int fbe_memory_number_of_objects_t;

#pragma pack(1)
typedef struct fbe_memory_number_of_objects_split_s{
	fbe_u16_t control_objects;
	fbe_u16_t data_objects;
}fbe_memory_number_of_objects_split_t;

typedef union fbe_memory_number_of_objects_dc_s{
	fbe_memory_number_of_objects_split_t	split;
	fbe_memory_number_of_objects_t			number_of_objects;
}fbe_memory_number_of_objects_dc_t;

#pragma pack()

typedef void * fbe_memory_ptr_t;

typedef enum fbe_memory_service_control_code_e {
    FBE_MEMORY_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_MEMORY),

    FBE_MEMORY_SERVICE_CONTROL_CODE_GET_DPS_STATS,
    FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_ADD_MEMORY,
    FBE_MEMORY_SERVICE_CONTROL_CODE_GET_ENV_LIMITS,
    FBE_MEMORY_SERVICE_CONTROL_CODE_PERSIST_SET_PARAMS,
	FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_REDUCE_SIZE,


    FBE_MEMORY_SERVICE_CONTROL_CODE_LAST
} fbe_memory_service_control_code_t;

typedef struct fbe_memory_statistics_s{
    fbe_u64_t total_chunks;
    fbe_u64_t free_chunks;
    fbe_u64_t total_alloc_count;
    fbe_u64_t total_release_count;
    fbe_u64_t total_push_count;
    fbe_u64_t total_pop_count;
    fbe_u64_t current_queue_depth;
    fbe_u64_t max_queue_depth;    
    fbe_u64_t native_alloc_count;
    fbe_u64_t native_release_count;    
	fbe_u64_t false_release_count;	
} fbe_memory_statistics_t;

/*!*******************************************************************
 * @struct fbe_memory_dps_statistics_t
 *********************************************************************
 * @brief
 *  This structure contains the DPS memory statistics couners
 *
 *********************************************************************/
typedef struct fbe_memory_dps_statistics_s{
    fbe_u64_t number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t number_of_free_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t number_of_data_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t number_of_free_data_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];

	/* How many requests we received */
	fbe_u64_t request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];
	fbe_u64_t request_data_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];

	/* How many requests we deferred (queued) */
    fbe_u64_t deferred_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];
    fbe_u64_t deferred_data_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];

    /* How many requests we ran out of memory completely on. */
    fbe_u64_t deadlock_count[FBE_CPU_ID_MAX];

	/* How many requests are currently on the queue */
	fbe_u64_t  request_queue_count[FBE_MEMORY_DPS_QUEUE_ID_LAST];

	/* Fast pools */
	fbe_u32_t fast_pool_number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];
	fbe_u32_t fast_pool_number_of_free_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX]; /* Not accurate */

	fbe_u32_t fast_pool_number_of_data_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];
	fbe_u32_t fast_pool_number_of_free_data_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX]; /* Not accurate */

	fbe_u64_t fast_pool_request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];
	fbe_u64_t fast_pool_data_request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_CPU_ID_MAX];

} fbe_memory_dps_statistics_t;

typedef void * (* fbe_memory_allocation_function_t)(fbe_u32_t allocation_size_in_bytes);
typedef void   (* fbe_memory_release_function_t)(void * ptr);

fbe_memory_allocation_function_t fbe_memory_get_allocation_function(void);
fbe_memory_release_function_t fbe_memory_get_release_function(void);

typedef struct fbe_memory_dps_init_parameters_s{
    fbe_memory_number_of_objects_t number_of_main_chunks;
    fbe_memory_number_of_objects_t packet_number_of_chunks;
    fbe_memory_number_of_objects_t block_64_number_of_chunks;
    fbe_memory_number_of_objects_t block_1_number_of_chunks;

    fbe_memory_number_of_objects_t fast_packet_number_of_chunks;
    fbe_memory_number_of_objects_t fast_block_64_number_of_chunks;
    fbe_memory_number_of_objects_t fast_block_1_number_of_chunks;

    fbe_memory_number_of_objects_t reserved_packet_number_of_chunks;
    fbe_memory_number_of_objects_t reserved_block_64_number_of_chunks;
    fbe_memory_number_of_objects_t reserved_block_1_number_of_chunks;

	fbe_u8_t * memory_ptr; /* If not NULL has preallocated memory address */
} fbe_memory_dps_init_parameters_t;

fbe_status_t fbe_memory_init_number_of_chunks(fbe_memory_number_of_objects_t number_of_chunks);

fbe_status_t fbe_memory_dps_init_number_of_chunks(fbe_memory_dps_init_parameters_t *num_chunks_p);
fbe_status_t fbe_memory_dps_init_number_of_data_chunks(fbe_memory_dps_init_parameters_t *num_chunks_p);

fbe_status_t fbe_memory_set_memory_functions(fbe_memory_allocation_function_t allocation_function,
											 fbe_memory_release_function_t release_function);
fbe_status_t fbe_memory_dps_set_memory_functions(fbe_memory_allocation_function_t allocation_function,
                                                 fbe_memory_release_function_t release_function);

void * fbe_memory_native_allocate(fbe_u32_t allocation_size_in_bytes);
void fbe_memory_native_release(void * ptr);
/* AJTODO: Temporary hack. Remove...  This routine will zero out the memroy it created */
void * fbe_memory_ex_allocate(fbe_u32_t allocation_size_in_bytes);
void fbe_memory_ex_release(void * ptr);
void fbe_memory_get_mem_mb_for_chunk_count(fbe_u32_t number_of_chunks, fbe_u32_t *memory_use_mb_p);
void * fbe_memory_allocate_required(fbe_u32_t allocation_size_in_bytes);
void fbe_memory_release_required(void * ptr);

typedef void*(*fbe_memory_native_allocate_mock_t)(fbe_u32_t);
typedef void(*fbe_memory_native_release_mock_t)(void*);
void fbe_memory_native_set_allocate_mock(fbe_memory_native_allocate_mock_t);
void fbe_memory_native_set_release_mock(fbe_memory_native_release_mock_t);

typedef enum fbe_memory_bit_bucket_sgl_type_e{ /*!< Used as an index to internal bit_bucket_sgl_array */
    FBE_MEMORY_BIT_BUCKET_SGL_TYPE_512,
    FBE_MEMORY_BIT_BUCKET_SGL_TYPE_520,

    FBE_MEMORY_BIT_BUCKET_SGL_TYPE_LAST
}fbe_memory_bit_bucket_sgl_type_t;

fbe_sg_element_t * fbe_memory_get_bit_bucket_sgl(fbe_memory_bit_bucket_sgl_type_t memory_bit_bucket_sgl_type);

typedef enum fbe_invalid_pattern_bit_bucket_sgl_type_e { /*!< Used as an index to internal bit_bucket_sgl_array */
	FBE_INVALID_PATTERN_BIT_BUCKET_SGL_TYPE_520,
	FBE_INVALID_PATTERN_BIT_BUCKET_SGL_TYPE_LAST
}fbe_invalid_pattern_bit_bucket_sgl_type_t;

fbe_sg_element_t * fbe_zero_get_bit_bucket_sgl(void);
fbe_sg_element_t * fbe_invalid_pattern_get_bit_bucket_sgl(void);

fbe_sg_element_t * fbe_zero_get_bit_bucket_sgl_1MB(void);
fbe_sg_element_t * fbe_invalid_pattern_get_bit_bucket_sgl_1MB(void);

fbe_sg_element_t * fbe_unmap_get_bit_bucket_sgl(void);

/* This bitbucket you can read into multiple times.  We never write with this bitbucket. */
fbe_u8_t * fbe_memory_get_bit_bucket_addr(void);
fbe_u32_t fbe_memory_get_bit_bucket_size(void);

fbe_status_t fbe_memory_get_zero_bit_bucket_size_in_blocks(fbe_u32_t * zero_bit_bucket_size_p);
fbe_status_t fbe_memory_get_zero_bit_bucket(fbe_u8_t ** zero_bit_bucket_p, fbe_u32_t * zero_bit_bucket_size_p);
fbe_status_t fbe_memory_get_invalid_pattern_bit_bucket(fbe_u8_t ** invalid_pattern_bit_bucket_p, fbe_block_count_t * invalid_pattern_bit_bucket_size_p);
fbe_status_t fbe_memory_get_invalid_pattern_bit_bucket_size_in_blocks(fbe_u32_t * invalid_pattern_bit_bucket_size_p);

void memory_service_trace(fbe_trace_level_t trace_level,
				fbe_trace_message_id_t message_id,
				const char * fmt, ...)
				__attribute__((format(__printf_func__,3,4)));

/* Memory interface to DPS */
enum fbe_memory_constants_e {
	FBE_MEMORY_HEADER_SIZE = 64,
	FBE_MEMORY_HEADER_MAGIC_NUMBER				= 0x0000FBE4,
	FBE_MEMORY_HEADER_MAGIC_NUMBER_DATA			= 0x0000FBE5,
	FBE_MEMORY_HEADER_MASK						= 0x00FFFFFF,
	FBE_MEMORY_HEADER_SHIFT						= 24,
};

typedef struct fbe_memory_hptr_s{
	FBE_ALIGN(8)void * master_header; /* Master header will point to himself */
	FBE_ALIGN(8)void * next_header;
}fbe_memory_hptr_t;

typedef struct fbe_memory_header_s{
	fbe_u64_t				magic_number;
	fbe_memory_chunk_size_t memory_chunk_size; /* Valid in master header only */
	fbe_u32_t				number_of_chunks; /* Valid in master header only */
    fbe_memory_request_priority_t priority; /* Priority we originally allocated at. */

	fbe_queue_element_t queue_element;

	union{
		fbe_memory_hptr_t hptr;
		fbe_queue_element_t queue_element;
	}u;

	FBE_ALIGN(8)fbe_u8_t * data;
}fbe_memory_header_t;

/* This set of flags allows us to know what situations we are allowed to abort requests for.
 */
enum fbe_memory_reject_allowed_flag_e{
	FBE_MEMORY_REJECT_ALLOWED_FLAG_INVALID            = 0x00000000,
	FBE_MEMORY_REJECT_ALLOWED_FLAG_NOT_PREFERRED      = 0x00000001,
	FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE = 0x00000002,
	FBE_MEMORY_REJECT_ALLOWED_FLAG_DEGRADED           = 0x00000004,
	FBE_MEMORY_REJECT_ALLOWED_FLAG_QUEUE_DEPTH        = 0x00000008,
	FBE_MEMORY_REJECT_ALLOWED_FLAG_LAST               = 0x00000008,
};

typedef struct fbe_memory_io_master_s { /* One master to rule memory allocations for entire I/O duration */
	fbe_u32_t chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_LAST];
	fbe_u32_t reserved_chunk_counter[FBE_MEMORY_DPS_QUEUE_ID_LAST];
	fbe_memory_request_priority_t       priority; /* Only requests at higher priority will be allowed */
	fbe_u32_t flags;
    fbe_u8_t client_reject_allowed_flags;   /* Flags client gave us. */
    fbe_u8_t arrival_reject_allowed_flags;  /* Flags client knew about on arrival. */
}fbe_memory_io_master_t;


typedef struct fbe_memory_request_s{
	/* The order is important!!! Please do not reorder */
	fbe_queue_element_t             queue_element;
    fbe_u64_t                       magic_number;

	fbe_queue_head_t				chunk_queue; /* queue of allocated memory headers */

	fbe_memory_io_master_t			*	memory_io_master; /* Used to track overall memory allocations for this I/O */
	/*struct fbe_packet_s					* packet;*/

    fbe_memory_request_state_t          request_state;
	fbe_memory_chunk_size_t				chunk_size; /* Valid if request_state is FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK */
    fbe_memory_number_of_objects_t      number_of_objects; /* Can be mapped to fbe_memory_number_of_objects_split_t */
    fbe_memory_request_priority_t       priority; /* This memory request priority */
    fbe_memory_io_stamp_t               io_stamp; /* Used to track memory requests associated with packet */
	fbe_memory_ptr_t					ptr; /* Should be casted to fbe_memory_header_t if request_state is FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK */
	fbe_memory_ptr_t					data_ptr; /* Should be casted to fbe_memory_header_t if request_state is FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK */
	fbe_memory_completion_function_t    completion_function;
    fbe_memory_completion_context_t     completion_context;	
	fbe_memory_dps_request_flags_t		flags;	
}fbe_memory_request_t;

void fbe_memory_allocate(fbe_memory_request_t * memory_request);
void fbe_memory_release(void * ptr);

fbe_status_t fbe_memory_alloc_entry(struct fbe_packet_s * packet);

fbe_status_t fbe_memory_request_init(fbe_memory_request_t * memory_request);

fbe_bool_t fbe_memory_request_is_build_chunk_complete(fbe_memory_request_t * memory_request);

fbe_status_t fbe_memory_request_mark_aborted_complete(fbe_memory_request_t * memory_request);

fbe_bool_t fbe_memory_request_is_in_use(fbe_memory_request_t * memory_request);

fbe_bool_t fbe_memory_request_is_allocation_complete(fbe_memory_request_t * memory_request);

fbe_bool_t fbe_memory_request_is_immediate(fbe_memory_request_t * memory_request);

fbe_bool_t fbe_memory_request_is_aborted(fbe_memory_request_t * memory_request);

fbe_status_t fbe_memory_build_chunk_request(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_t number_of_objects,
                                            fbe_memory_request_priority_t new_priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context);

fbe_status_t fbe_memory_build_chunk_request_sync(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_t number_of_objects,
                                            fbe_memory_request_priority_t new_priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context);


fbe_status_t fbe_memory_build_dc_request(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_dc_t number_of_objects,
                                            fbe_memory_request_priority_t new_priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context);

fbe_status_t fbe_memory_build_dc_request_sync(fbe_memory_request_t * memory_request,
											fbe_memory_chunk_size_t	chunk_size,
											fbe_memory_number_of_objects_dc_t number_of_objects,
                                            fbe_memory_request_priority_t new_priority,
                                            fbe_memory_io_stamp_t io_stamp,
											fbe_memory_completion_function_t completion_function,
											fbe_memory_completion_context_t	completion_context);

fbe_status_t fbe_memory_request_entry(fbe_memory_request_t * memory_request);
fbe_memory_header_t *fbe_memory_dps_chunk_queue_element_to_header(fbe_queue_element_t * queue_element);
fbe_status_t fbe_memory_abort_request(fbe_memory_request_t * memory_request);

//fbe_status_t fbe_memory_request_get_entry_mark_free(fbe_memory_request_t * memory_request, fbe_memory_ptr_t * memory_entry_pp);

//fbe_status_t fbe_memory_free_entry(fbe_memory_header_t * master_memory_header);

fbe_status_t fbe_memory_free_request_entry(fbe_memory_request_t * memory_request);


fbe_status_t fbe_memory_get_next_memory_header(fbe_memory_header_t * current_memory_header,
                                               fbe_memory_header_t ** next_memory_header);

fbe_memory_header_t *fbe_memory_get_first_memory_header(fbe_memory_request_t * memory_request_p);
fbe_memory_header_t *fbe_memory_get_first_data_memory_header(fbe_memory_request_t * memory_request_p);
fbe_memory_hptr_t *fbe_memory_get_next_memory_element(fbe_memory_hptr_t * current_memory_element);

fbe_u32_t fbe_memory_get_queue_length(fbe_memory_request_t * memory_request_p);
fbe_u32_t fbe_memory_get_data_queue_length(fbe_memory_request_t * memory_request_p);
fbe_u8_t *fbe_memory_header_element_to_data(fbe_memory_hptr_t * header_element);

fbe_u8_t *fbe_memory_header_to_data(fbe_memory_header_t * memory_header);

fbe_memory_header_t * fbe_memory_header_data_to_master_header(fbe_u8_t * data);

void fbe_memory_get_env_limits(fbe_environment_limits_t* env_limits);
void fbe_memory_set_env_limits(fbe_environment_limits_t* env_limits);
fbe_status_t fbe_memory_dps_ctrl_data_to_chunk_size(fbe_memory_chunk_size_t ctrl_size,
                                                    fbe_memory_chunk_size_t data_size,
                                                    fbe_memory_chunk_size_t *combined_p);
fbe_status_t fbe_memory_dps_chunk_size_to_data_bytes(fbe_memory_chunk_size_t chunk_size,
                                                     fbe_memory_chunk_size_t *bytes_p);
fbe_status_t fbe_memory_dps_chunk_size_to_ctrl_bytes(fbe_memory_chunk_size_t chunk_size,
                                                     fbe_memory_chunk_size_t *bytes_p);
fbe_bool_t fbe_memory_is_reject_allowed_set(fbe_memory_io_master_t *io_master_p, 
                                            fbe_u8_t flags);

fbe_status_t fbe_memory_check_state(fbe_cpu_id_t cpu_id, fbe_bool_t check_data_memory);

#endif /* FBE_MEMORY_H */


