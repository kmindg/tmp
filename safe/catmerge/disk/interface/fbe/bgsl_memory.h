#ifndef BGSL_MEMORY_H
#define BGSL_MEMORY_H

/*#include "bgsl_base_service.h"*/
#include "bgsl_queue.h"

#define BGSL_MEMORY_CHUNK_SIZE 1536
#define BGSL_MEMORY_MAX_CHUNKS_PER_REQUEST 16
/*
typedef struct bgsl_memory_s{
	bgsl_base_service_t bgsl_base_service;

}bgsl_memory_t;
*/
typedef enum bgsl_memory_request_state_e {
	BGSL_MEMORY_REQUEST_STATE_INVALID,
	BGSL_MEMORY_REQUEST_STATE_ALLOCATE_PACKET,
	BGSL_MEMORY_REQUEST_STATE_ALLOCATE_OBJECT,
	BGSL_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER,

	BGSL_MEMORY_REQUEST_STATE_ERROR,

	BGSL_MEMORY_REQUEST_STATE_LAST
}bgsl_memory_request_state_t;

struct bgsl_memory_request_s;
typedef void * bgsl_memory_completion_context_t;
typedef void (* bgsl_memory_completion_function_t)(struct bgsl_memory_request_s * memory_request, bgsl_memory_completion_context_t context);
typedef unsigned int bgsl_memory_number_of_objects_t;
typedef void * bgsl_memory_ptr_t;

typedef struct bgsl_memory_request_s{
	bgsl_queue_element_t					queue_element;
	bgsl_memory_request_state_t			request_state;
	bgsl_memory_number_of_objects_t		number_of_objects;
	bgsl_memory_ptr_t					ptr;
	bgsl_memory_completion_function_t	completion_function;
	bgsl_memory_completion_context_t		completion_context;
}bgsl_memory_request_t;

typedef struct bgsl_memory_statistics_s{
    bgsl_u64_t total_chunks;
    bgsl_u64_t free_chunks;
    bgsl_u64_t total_alloc_count;
    bgsl_u64_t total_release_count;
    bgsl_u64_t total_push_count;
    bgsl_u64_t total_pop_count;
    bgsl_u64_t current_queue_depth;
    bgsl_u64_t max_queue_depth;    
    bgsl_u64_t native_alloc_count;
    bgsl_u64_t native_release_count;    
} bgsl_memory_statistics_t;

bgsl_status_t bgsl_memory_init_number_of_chunks(bgsl_memory_number_of_objects_t number_of_chunks);

void bgsl_memory_allocate(bgsl_memory_request_t * memory_request);
void bgsl_memory_release(void * ptr);

void * bgsl_memory_native_allocate(bgsl_u32_t allocation_size_in_bytes);
void bgsl_memory_native_release(void * ptr);

typedef enum bgsl_memory_bit_bucket_sgl_type_e{ /*!< Used as an index to internal bit_bucket_sgl_array */
	BGSL_MEMORY_BIT_BUCKET_SGL_TYPE_512,
	BGSL_MEMORY_BIT_BUCKET_SGL_TYPE_520,

	BGSL_MEMORY_BIT_BUCKET_SGL_TYPE_LAST
}bgsl_memory_bit_bucket_sgl_type_t;



#endif /* BGSL_MEMORY_H */
