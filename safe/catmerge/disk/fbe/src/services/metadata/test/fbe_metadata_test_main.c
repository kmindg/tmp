/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"

#include "fbe/fbe_transport.h"
#include "fbe_cmi_sim.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe_metadata_test_service_table.h"

#include "fbe_base_config_private.h"
#include "../src/fbe_metadata_private.h"

#include "mut.h"
#include "mut_assert.h"
#include "fbe/fbe_emcutil_shell_include.h"

#define FBE_SEP_NUMBER_OF_CHUNKS                10448    /* Approximately   40 MB */

static fbe_u32_t metadata_stripe_lock_packet_completion_counter;

static fbe_status_t terminator_api_get_sp_id(terminator_sp_id_t *sp_id)
{
	*sp_id = TERMINATOR_SP_A;
	return FBE_STATUS_OK;
}

void metadata_stripe_lock_init(void)
{
    fbe_status_t status;

	fbe_cmi_sim_set_get_sp_id_func(terminator_api_get_sp_id);

	fbe_transport_init();

    /* Initialize service manager */
    status = fbe_service_manager_init_service_table(metadata_test_service_table);
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);

    /* Initialize the memory */
    status =  fbe_memory_init_number_of_chunks(FBE_SEP_NUMBER_OF_CHUNKS);
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);

    /* Initialize topology */
    status = fbe_topology_init_class_table(NULL/*sep_class_table*/);
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TOPOLOGY);


    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_CMI);

    /* Initialize metadata service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_METADATA);
    	
    /*initialize the drive trace service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRACE);
}

void metadata_stripe_lock_destroy(void)
{
    fbe_status_t status;

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRACE);
	
    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_METADATA);
 

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TOPOLOGY);

 		
    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
 	
	
    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);

	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_CMI);

	fbe_transport_destroy();

}

static fbe_status_t 
metadata_stripe_lock_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	metadata_stripe_lock_packet_completion_counter++;
    return FBE_STATUS_OK;
}

void metadata_stripe_lock_range_test(fbe_u64_t write_start,
									 fbe_u64_t write_count,
									 fbe_u64_t read_start,
									 fbe_u64_t read_count)
{
	fbe_packet_t * packet_array[3];
	fbe_u32_t i;
	fbe_semaphore_t sem;
	fbe_payload_ex_t * payload;
	fbe_payload_stripe_lock_operation_t * stripe_lock_operation;
	fbe_base_config_t base_config;
	fbe_status_t status;
	fbe_payload_metadata_operation_t * metadata_operation = NULL;

	mut_printf(MUT_LOG_TEST_STATUS, "%llu %llu %llu %llu\n",
		   (unsigned long long)write_start,
		   (unsigned long long)write_count,
		   (unsigned long long)read_start,
		   (unsigned long long)read_count);

	base_config.base_object.object_id = 0;
	fbe_semaphore_init(&sem, 0, 3);

	for(i = 0; i < 3; i++){
		packet_array[i] = malloc(sizeof(fbe_packet_t));
		fbe_transport_initialize_sep_packet(packet_array[i]);
	}

	fbe_base_config_init(&base_config);
    payload = fbe_transport_get_payload_ex(packet_array[0]);
    metadata_operation = fbe_payload_ex_allocate_metadata_operation(payload);
    base_config.metadata_element.metadata_memory.memory_ptr = NULL;
    base_config.metadata_element.metadata_memory.memory_size = 0;
    base_config.metadata_element.metadata_memory_peer.memory_ptr = NULL;
    base_config.metadata_element.metadata_memory_peer.memory_size = 0;
    fbe_payload_metadata_build_init_memory(metadata_operation, &base_config.metadata_element);
    fbe_transport_set_completion_function(packet_array[0], metadata_stripe_lock_packet_completion, &sem);
    fbe_payload_ex_increment_metadata_operation_level(payload);
    status = fbe_metadata_operation_entry(packet_array[0]);
	fbe_semaphore_wait_ms(&sem, 0);
	fbe_payload_ex_release_metadata_operation(payload, metadata_operation);

	/* Write then read */
	metadata_stripe_lock_packet_completion_counter = 0;
	/* Write */
	payload = fbe_transport_get_payload_ex(packet_array[0]);
	stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation, &base_config.metadata_element, write_start, (fbe_u32_t)write_count);
	fbe_payload_ex_increment_stripe_lock_operation_level(payload);
	fbe_transport_set_completion_function(packet_array[0], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[0]);
	fbe_semaphore_wait_ms(&sem, 0);
	/* Read */
	metadata_stripe_lock_packet_completion_counter = 0;
	payload = fbe_transport_get_payload_ex(packet_array[1]);
	stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation, &base_config.metadata_element, read_start, (fbe_u32_t)read_count);
	fbe_payload_ex_increment_stripe_lock_operation_level(payload);
	fbe_transport_set_completion_function(packet_array[1], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[1]);
	status = fbe_semaphore_wait_ms(&sem, 100); /* 100 ms should be enough */
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);
	MUT_ASSERT_INT_EQUAL(metadata_stripe_lock_packet_completion_counter, 0);

	/* Unlock the write lock */
	payload = fbe_transport_get_payload_ex(packet_array[0]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation);
	fbe_transport_set_completion_function(packet_array[0], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[0]);
	fbe_semaphore_wait_ms(&sem, 0); /* For write completion */
	fbe_semaphore_wait_ms(&sem, 0); /* For read completion */
	MUT_ASSERT_INT_EQUAL(metadata_stripe_lock_packet_completion_counter, 2);

	/* Release write operation */
	payload = fbe_transport_get_payload_ex(packet_array[0]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

	/* Unlock the read lock */
	payload = fbe_transport_get_payload_ex(packet_array[1]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation);
	fbe_transport_set_completion_function(packet_array[1], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[1]);
	fbe_semaphore_wait_ms(&sem, 0); /* For write completion */

	/* Release read operation */
	payload = fbe_transport_get_payload_ex(packet_array[1]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

	fbe_semaphore_destroy(&sem);

	for(i = 0; i < 3; i++){
		fbe_transport_destroy_packet(packet_array[i]);
		free(packet_array[i]);		
	}

}

void metadata_stripe_lock_packet_test(void)
{
	metadata_stripe_lock_range_test( 0,1, 0,1);
	metadata_stripe_lock_range_test( 256 + 4, 128 + 4, 256 + 4, 1);
	metadata_stripe_lock_range_test( 256 + 4, 128 + 4, 256 + 5, 1);
	metadata_stripe_lock_range_test( 256 + 4, 128 + 4, 256 + 4 + 64, 1);
	metadata_stripe_lock_range_test( 256 + 4, 128 + 4, 256 + 4 + 128 + 3, 1);

	metadata_stripe_lock_range_test( 256 + 4, 1, 256 + 4 , 128 + 4);
	metadata_stripe_lock_range_test( 256 + 5, 1, 256 + 4 , 128 + 4);
	metadata_stripe_lock_range_test( 256 + 4 + 64, 1, 256 + 4 , 128 + 4);
	metadata_stripe_lock_range_test( 256 + 4 + 128 + 3, 1, 256 + 4 , 128 + 4);

}

void metadata_stripe_lock_packet_abort_test(void)
{
	fbe_u64_t write_start = 0;
	fbe_u64_t write_count = 1;
	fbe_u64_t read_start = 0;
	fbe_u64_t read_count = 1;
	fbe_packet_t * packet_array[3];
	fbe_u32_t i;
	fbe_semaphore_t sem;
	fbe_payload_ex_t * payload;
	fbe_payload_stripe_lock_operation_t * stripe_lock_operation;
	fbe_base_config_t base_config;
	fbe_status_t status;
	fbe_payload_metadata_operation_t * metadata_operation = NULL;

	mut_printf(MUT_LOG_TEST_STATUS, "%llu %llu %llu %llu\n",
		   (unsigned long long)write_start,
		   (unsigned long long)write_count,
		   (unsigned long long)read_start,
		   (unsigned long long)read_count);

	base_config.base_object.object_id = 0;
	fbe_semaphore_init(&sem, 0, 3);

	for(i = 0; i < 3; i++){
		packet_array[i] = malloc(sizeof(fbe_packet_t));
		fbe_transport_initialize_sep_packet(packet_array[i]);
	}

	fbe_base_config_init(&base_config);
    payload = fbe_transport_get_payload_ex(packet_array[0]);
    metadata_operation = fbe_payload_ex_allocate_metadata_operation(payload);
    base_config.metadata_element.metadata_memory.memory_ptr = NULL;
    base_config.metadata_element.metadata_memory.memory_size = 0;
    base_config.metadata_element.metadata_memory_peer.memory_ptr = NULL;
    base_config.metadata_element.metadata_memory_peer.memory_size = 0;
    fbe_payload_metadata_build_init_memory(metadata_operation, &base_config.metadata_element);
    fbe_transport_set_completion_function(packet_array[0], metadata_stripe_lock_packet_completion, &sem);
    fbe_payload_ex_increment_metadata_operation_level(payload);
    status = fbe_metadata_operation_entry(packet_array[0]);
	fbe_semaphore_wait_ms(&sem, 0);
	fbe_payload_ex_release_metadata_operation(payload, metadata_operation);

	/* Write then read */
	metadata_stripe_lock_packet_completion_counter = 0;
	/* Write */
	payload = fbe_transport_get_payload_ex(packet_array[0]);
	stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation, &base_config.metadata_element, write_start, (fbe_u32_t)write_count);
	fbe_payload_ex_increment_stripe_lock_operation_level(payload);
	fbe_transport_set_completion_function(packet_array[0], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[0]);
	fbe_semaphore_wait_ms(&sem, 0);

	/* Read */
	metadata_stripe_lock_packet_completion_counter = 0;
	payload = fbe_transport_get_payload_ex(packet_array[1]);
	stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation, &base_config.metadata_element, read_start, (fbe_u32_t)read_count);
	stripe_lock_operation->flags = 0;
	fbe_payload_ex_increment_stripe_lock_operation_level(payload);
	fbe_transport_set_completion_function(packet_array[1], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[1]);
	status = fbe_semaphore_wait_ms(&sem, 100); /* 100 ms should be enough */
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);
	MUT_ASSERT_INT_EQUAL(metadata_stripe_lock_packet_completion_counter, 0);

	/* Read suppose to be on a wait queue. Let's abort that packet it */
	fbe_transport_cancel_packet(packet_array[1]);

	fbe_semaphore_wait_ms(&sem, 0); /* For read completion */
	MUT_ASSERT_INT_EQUAL(metadata_stripe_lock_packet_completion_counter, 1);
	status = fbe_transport_get_status_code(packet_array[1]);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_CANCELED);

	/* Unlock the write lock */
	payload = fbe_transport_get_payload_ex(packet_array[0]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation);
	fbe_transport_set_completion_function(packet_array[0], metadata_stripe_lock_packet_completion, &sem);
	fbe_stripe_lock_operation_entry(packet_array[0]);
	fbe_semaphore_wait_ms(&sem, 0); /* For write completion */
	
	MUT_ASSERT_INT_EQUAL(metadata_stripe_lock_packet_completion_counter, 2);

	/* Release write operation */
	payload = fbe_transport_get_payload_ex(packet_array[0]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

	/* Release read operation */
	payload = fbe_transport_get_payload_ex(packet_array[1]);
	stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
	fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

	fbe_semaphore_destroy(&sem);

	for(i = 0; i < 3; i++){
		fbe_transport_destroy_packet(packet_array[i]);
		free(packet_array[i]);		
	}
}

int __cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *suite_p;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

    suite_p = MUT_CREATE_TESTSUITE("fbe_metadata_unit_test_suite");

	MUT_ADD_TEST(suite_p, metadata_stripe_lock_packet_test, metadata_stripe_lock_init, metadata_stripe_lock_destroy);	
	
	MUT_ADD_TEST(suite_p, metadata_stripe_lock_packet_abort_test, metadata_stripe_lock_init, metadata_stripe_lock_destroy);

    exit(0);
}


fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_SEP_0;
    return FBE_STATUS_OK;
}

