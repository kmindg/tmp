/***************************************************************************
 * Copyright (C) EMC Corporation 2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                fbe_memory_test_main.c
 ***************************************************************************
 *
 * DESCRIPTION: Unit test suite for fbe implementation of memory management
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/18/2007   Buckley, Martin  initial version
 *    10/29/2007   Alexander Gorlov (gorloa)  update according to reworked version of test controller
 *                                            new assert 
 *                                            void test(void) function is used 
 *                                            comments in doxygen style has been created
 *
 *    07/24/2008   Peter Puhov  Resolved memory initialization issue.          
 *								Fixeddouble allocation test.
 **************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "mut.h"
#include "fbe_base_object.h"
#include "fbe/fbe_queue.h"
#include "fbe_base_service.h"
#include "fbe/fbe_emcutil_shell_include.h"

#include "fbe_memory_private.h"

typedef enum monitor_thread_flag_e{
    MONITOR_THREAD_RUN,
    MONITOR_THREAD_STOP,
    MONITOR_THREAD_DONE
} monitor_thread_flag_t;

static void monitor_thread_func(void * context);
static void free_memory_request(fbe_memory_request_t *memory_request);

/* We tetsting memory service, so it is OK to directly access the methods */
extern fbe_service_methods_t fbe_memory_service_methods;

/*
 * memory_get_statistics is not declared in fbe_memory.h
 */
void memory_get_statistics(fbe_memory_statistics_t * stat);
static fbe_status_t memory_service_init_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t memory_service_destroy_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static 	fbe_memory_dps_init_parameters_t sep_dps_init_parameters;
static 	fbe_memory_dps_init_parameters_t sep_dps_init_data_parameters;

static void check_stack_size(void);

fbe_status_t memory_test_send_init_command(void)
{
	fbe_packet_t * packet = NULL;
	fbe_semaphore_t sem;
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;

    sep_dps_init_parameters.number_of_main_chunks = 100;
    sep_dps_init_parameters.packet_number_of_chunks = 10;
    sep_dps_init_parameters.block_64_number_of_chunks = 10;
    sep_dps_init_parameters.block_1_number_of_chunks = 10;

    sep_dps_init_parameters.fast_packet_number_of_chunks = 1;
    sep_dps_init_parameters.fast_block_64_number_of_chunks = 2;
    sep_dps_init_parameters.fast_block_1_number_of_chunks = 1;

    sep_dps_init_parameters.reserved_packet_number_of_chunks = 4;
    sep_dps_init_parameters.reserved_block_64_number_of_chunks = 56;
    sep_dps_init_parameters.reserved_block_1_number_of_chunks = 1;
	sep_dps_init_parameters.memory_ptr = NULL;


    sep_dps_init_data_parameters.number_of_main_chunks = 100;
    sep_dps_init_data_parameters.packet_number_of_chunks = 10;
    sep_dps_init_data_parameters.block_64_number_of_chunks = 10;
    sep_dps_init_data_parameters.block_1_number_of_chunks = 0;

    sep_dps_init_data_parameters.fast_packet_number_of_chunks = 1;
    sep_dps_init_data_parameters.fast_block_64_number_of_chunks = 2;
    sep_dps_init_data_parameters.fast_block_1_number_of_chunks = 0;

    sep_dps_init_data_parameters.reserved_packet_number_of_chunks = 4;
    sep_dps_init_data_parameters.reserved_block_64_number_of_chunks = 56;
    sep_dps_init_data_parameters.reserved_block_1_number_of_chunks = 0;

	sep_dps_init_data_parameters.memory_ptr = 
							fbe_allocate_nonpaged_pool_with_tag (sep_dps_init_data_parameters.number_of_main_chunks * 1024 * 1024, 'pfbe');

	fbe_memory_dps_init_number_of_chunks(&sep_dps_init_parameters);
	fbe_memory_dps_init_number_of_data_chunks(&sep_dps_init_data_parameters);

	fbe_memory_init_number_of_chunks(50);

	/* The memory service is not available yet */
	packet = fbe_allocate_nonpaged_pool_with_tag (sizeof (fbe_packet_t), 'pfbe');
	if(packet == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

    fbe_semaphore_init(&sem, 0, 1);

	/* Allocate packet */
	status = fbe_transport_initialize_packet(packet);
	if(status != FBE_STATUS_OK){
		return status;
	}

	status = fbe_transport_build_control_packet(packet, 
												FBE_BASE_SERVICE_CONTROL_CODE_INIT,
												NULL,
												0,
												0,
												memory_service_init_command_completion,
												&sem);
	if(status != FBE_STATUS_OK){
		return status;
	}


		/* Set packet address */
	status = fbe_transport_set_address(	packet,
										FBE_PACKAGE_ID_PHYSICAL,
										FBE_SERVICE_ID_MEMORY,
										FBE_CLASS_ID_INVALID,
										FBE_OBJECT_ID_INVALID);

	payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_increment_control_operation_level(payload);
	status = fbe_memory_service_methods.control_entry(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	fbe_release_nonpaged_pool(packet);

	return status;
}

static fbe_status_t 
memory_service_init_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}


fbe_status_t memory_test_send_destroy_command(void)
{
	fbe_packet_t * packet = NULL;
	fbe_semaphore_t sem;
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;

	/* The memory service is not available yet */
	packet = fbe_allocate_nonpaged_pool_with_tag (sizeof (fbe_packet_t), 'pfbe');
	if(packet == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

    fbe_semaphore_init(&sem, 0, 1);

	/* Allocate packet */
	status = fbe_transport_initialize_packet(packet);
	if(status != FBE_STATUS_OK){
		return status;
	}

	status = fbe_transport_build_control_packet(packet, 
												FBE_BASE_SERVICE_CONTROL_CODE_DESTROY,
												NULL,
												0,
												0,
												memory_service_destroy_command_completion,
												&sem);
	if(status != FBE_STATUS_OK){
		return status;
	}


		/* Set packet address */
	status = fbe_transport_set_address(	packet,
										FBE_PACKAGE_ID_PHYSICAL,
										FBE_SERVICE_ID_MEMORY,
										FBE_CLASS_ID_INVALID,
										FBE_OBJECT_ID_INVALID);

	payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_increment_control_operation_level(payload);
	status = fbe_memory_service_methods.control_entry(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	fbe_release_nonpaged_pool(packet);

	fbe_release_nonpaged_pool(sep_dps_init_data_parameters.memory_ptr);
	return status;
}

static fbe_status_t 
memory_service_destroy_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

void get_memory_statistics(int print) {
	fbe_memory_statistics_t memory_statistics;

	/*
	 * First, invoke with NULL pointer, to cover argument processing
	 */
	memory_get_statistics(NULL);

	memory_get_statistics(&memory_statistics);
	if(print) {
		mut_printf(MUT_LOG_HIGH, "Total Chunks: %lld", memory_statistics.total_chunks);
		mut_printf(MUT_LOG_HIGH, "Free  Chunks: %lld", memory_statistics.free_chunks);
		mut_printf(MUT_LOG_HIGH, "Total Allocated Chunks: %lld", memory_statistics.total_alloc_count);
		mut_printf(MUT_LOG_HIGH, "Total Released  Chunks: %lld", memory_statistics.total_release_count);
		mut_printf(MUT_LOG_HIGH, "Total Pushed    Chunks: %lld", memory_statistics.total_push_count);
		mut_printf(MUT_LOG_HIGH, "Total Popped    Chunks: %lld", memory_statistics.total_pop_count);
		mut_printf(MUT_LOG_HIGH, "Current queue depth: %lld", memory_statistics.current_queue_depth);
		mut_printf(MUT_LOG_HIGH, "Max queue depth: %lld", memory_statistics.max_queue_depth);
	}
}

static void init_destroy_test(void)
{
	memory_test_send_init_command();

	memory_test_send_destroy_command();
}

static void 
allocate_data_and_control_buffer_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
	if(context != NULL){
		fbe_semaphore_release((fbe_semaphore_t *)context, 0, 1, FALSE);
	}
}

static void allocate_data_and_control_buffer_p_p(void)
{
	fbe_status_t status;
	fbe_memory_request_t request;
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;

	/* Init memory service */
	memory_test_send_init_command();
	
	number_of_objects.split.data_objects = 3;
	number_of_objects.split.control_objects = 4;

	status = fbe_memory_build_dc_request_sync(&request, 
											FBE_MEMORY_CHUNK_SIZE_PACKET_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t) allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request.ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request.data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	status = fbe_memory_free_request_entry(&request);

	memory_test_send_destroy_command();
}

static void allocate_data_and_control_buffer_64_p(void)
{
	fbe_status_t status;
	fbe_memory_request_t request;
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;

	/* Init memory service */
	memory_test_send_init_command();
	
	number_of_objects.split.data_objects = 3;
	number_of_objects.split.control_objects = 5;

	status = fbe_memory_build_dc_request_sync(&request, 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request.ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request.data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	status = fbe_memory_free_request_entry(&request);

	memory_test_send_destroy_command();
}

static void allocate_data_and_control_buffer_64_p0(void)
{
	fbe_status_t status;
	fbe_memory_request_t request;
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;

	/* Init memory service */
	memory_test_send_init_command();
	
	number_of_objects.split.data_objects = 3;
	number_of_objects.split.control_objects = 0;

	status = fbe_memory_build_dc_request_sync(&request, 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */ 
											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request.ptr;
	MUT_ASSERT_TRUE(master_memory_header == NULL);

	/* Data memory test */
	master_memory_header = request.data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	status = fbe_memory_free_request_entry(&request);

	memory_test_send_destroy_command();
}
static void allocate_data_and_control_priority(void)
{
	fbe_status_t status;
	fbe_memory_request_t request;
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;
	fbe_memory_dps_statistics_t dps_pool_stats;

	/* Init memory service */
	memory_test_send_init_command();

	fbe_memory_fill_dps_statistics(&dps_pool_stats);

	number_of_objects.split.data_objects = dps_pool_stats.fast_pool_number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO][0] + 2;
	number_of_objects.split.control_objects = 0;

	status = fbe_memory_build_dc_request_sync(&request, 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */

											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request.ptr;
	MUT_ASSERT_TRUE(master_memory_header == NULL);

	/* Data memory test */
	master_memory_header = request.data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	status = fbe_memory_free_request_entry(&request);

	memory_test_send_destroy_command();
}

static void allocate_data_and_control_buffer_deferred_64_p(void)
{
	fbe_status_t status;
	fbe_memory_request_t request[2];
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;
	fbe_memory_dps_statistics_t dps_pool_stats;
    fbe_semaphore_t             sem;
	fbe_status_t                wait_status;


	/* Init memory service */
	memory_test_send_init_command();

	fbe_memory_fill_dps_statistics(&dps_pool_stats);

	number_of_objects.split.data_objects = (fbe_u16_t)(dps_pool_stats.number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] - 1);
	number_of_objects.split.control_objects = 5;

	status = fbe_memory_build_dc_request_sync(&request[0], 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */

											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request[0]);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[0]) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request[0].ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request[0].data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	fbe_semaphore_init(&sem, 0, 1);

	/* The next allocation should return pending status */
	number_of_objects.split.data_objects = 10;
	number_of_objects.split.control_objects = 5;

	status = fbe_memory_build_dc_request_sync(&request[1], 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */

											&sem);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request[1]);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_PENDING);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[1]) == FBE_FALSE);

    wait_status = fbe_semaphore_wait_ms(&sem, 100);
	MUT_ASSERT_TRUE(wait_status == FBE_STATUS_TIMEOUT);

	/* Now release the memory */
	status = fbe_memory_free_request_entry(&request[0]);

	/* Wait for second request to complete */
    wait_status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_TRUE(wait_status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[1]) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request[1].ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request[1].data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	fbe_semaphore_destroy(&sem);

	/* Now release the memory */
	status = fbe_memory_free_request_entry(&request[1]);

	memory_test_send_destroy_command();
}


static void allocate_data_and_control_buffer_reserve_64_p(void)
{
	fbe_status_t status;
	fbe_memory_request_t request[3];
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;
	fbe_memory_dps_statistics_t dps_pool_stats;
    fbe_semaphore_t             sem;
	fbe_status_t                wait_status;
	fbe_memory_io_master_t		memory_io_master;

	/* Init memory service */
	memory_test_send_init_command();

	fbe_memory_fill_dps_statistics(&dps_pool_stats);

	number_of_objects.split.data_objects = (fbe_u16_t)(dps_pool_stats.number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] - 1);
	number_of_objects.split.control_objects = 5;

	status = fbe_memory_build_dc_request_sync(&request[0], 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */

											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request[0]);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[0]) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request[0].ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request[0].data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	fbe_semaphore_init(&sem, 0, 1);

	/* The next allocation should return pending status */
	number_of_objects.split.data_objects = 10;
	number_of_objects.split.control_objects = 5;

	status = fbe_memory_build_dc_request_sync(&request[1], 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            1, /* Should be greater than zero */
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											(fbe_memory_completion_function_t)allocate_data_and_control_buffer_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */

											&sem);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_zero_memory(&memory_io_master, sizeof(fbe_memory_io_master_t));		
	request[1].memory_io_master = &memory_io_master;

    status = fbe_memory_request_entry(&request[1]);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_PENDING);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[1]) == FBE_FALSE);

    wait_status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_TRUE(wait_status == FBE_STATUS_OK);

	/* Now release the memory */
	status = fbe_memory_free_request_entry(&request[0]);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[1]) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request[1].ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request[1].data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);

	/* Verify reserved flag */
	MUT_ASSERT_TRUE(memory_io_master.flags & FBE_MEMORY_REQUEST_FLAG_RESERVED_POOL);

	fbe_semaphore_destroy(&sem);

	/* Now release the memory */
	status = fbe_memory_free_request_entry(&request[1]);

	memory_test_send_destroy_command();

	check_stack_size();
}

static void check_stack_size(void)
{
	csx_size_t   	used_stack_size_rv;

	csx_p_get_used_stack_size(&used_stack_size_rv); 
	mut_printf(MUT_LOG_TEST_STATUS, "used_stack_size_rv: %llu\n", (csx_u64_t)used_stack_size_rv);
}


static void allocate_data_and_control_64_64_priority(void)
{
	fbe_status_t status;
	fbe_memory_request_t request[3];
	fbe_memory_number_of_objects_dc_t number_of_objects;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;	
	fbe_u32_t count;
	fbe_memory_dps_statistics_t dps_pool_stats;

	/* Init memory service */
	memory_test_send_init_command();

	fbe_memory_fill_dps_statistics(&dps_pool_stats);

	number_of_objects.split.data_objects = (fbe_u16_t)(dps_pool_stats.number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] - 1);
	number_of_objects.split.control_objects = (fbe_u16_t)(dps_pool_stats.number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_FOR_64_BLOCKS_IO] - 1);

	status = fbe_memory_build_dc_request_sync(&request[0], 
											FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_PACKET,
											number_of_objects,
                                            0, /* new_priority*/
                                            FBE_MEMORY_REQUEST_IO_STAMP_INVALID, /* io_stamp */
											allocate_data_and_control_buffer_completion,
											NULL);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_memory_request_entry(&request[0]);	
	/* The sync request should be satisfied */
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(fbe_memory_request_is_allocation_complete(&request[0]) == FBE_TRUE);

	/* Control memory test */
	master_memory_header = request[0].ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.control_objects);

	/* Data memory test */
	master_memory_header = request[0].data_ptr;
	MUT_ASSERT_TRUE(master_memory_header != NULL);
	MUT_ASSERT_TRUE(master_memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);

	count = 1;
    memory_header = master_memory_header->u.hptr.next_header;
	MUT_ASSERT_TRUE(memory_header != NULL);
	MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
	count++;

    while(memory_header->u.hptr.next_header != NULL){
        memory_header = memory_header->u.hptr.next_header;
		MUT_ASSERT_TRUE(memory_header->memory_chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO);
		count++;
    }

	MUT_ASSERT_TRUE(count == number_of_objects.split.data_objects);
	/* Now release the memory */
	status = fbe_memory_free_request_entry(&request[0]);

	memory_test_send_destroy_command();

	check_stack_size();
}


int __cdecl main (int argc , char ** argv)
{ 
    mut_testsuite_t *memorySuite;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

	fbe_transport_run_queue_init();

    memorySuite = MUT_CREATE_TESTSUITE("memorySuite")
    MUT_ADD_TEST (memorySuite, init_destroy_test, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_buffer_p_p, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_buffer_64_p, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_buffer_64_p0, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_priority, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_buffer_deferred_64_p, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_buffer_reserve_64_p, NULL, NULL);
	MUT_ADD_TEST(memorySuite, allocate_data_and_control_64_64_priority, NULL, NULL);

	
	MUT_RUN_TESTSUITE(memorySuite);

	fbe_transport_run_queue_destroy();

	exit(0);
	
}
