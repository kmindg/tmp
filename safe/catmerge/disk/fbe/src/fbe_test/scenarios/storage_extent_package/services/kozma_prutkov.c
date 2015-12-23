/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file kozma_prutkov_test.c
 ***************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *   9/14/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"

#include "fbe/fbe_api_terminator_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_utils.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "sep_tests.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void kozma_prutkov_memory_metadata_test(void);
static void kozma_prutkov_paged_metadata_operation_test(void);
static void kozma_prutkov_nonpaged_metadata_operation_test(void);
static void kozma_prutkov_stripe_lock_test(void);
static fbe_status_t kozma_prutkov_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context);
static void kozma_prutkov_zero_on_demand_test(void);
static void kozma_prutkov_passive_request_test(void);

static fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
static fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;

/*************************
 *   TEST DESCRIPTION
 *************************/
char * kozma_prutkov_short_desc = "Infrastructure related set of scenarios";
char * kozma_prutkov_long_desc =
"Infrastructure related set of scenarios tests the ability of the infrastructure to perform it's job correctly. \n"
"	Part 1.		Metadata service API.\n"
"	Part 2.		Stripe locking API.\n"
"	Part 3.		Memory service API.\n"
"	Part 4.		Metadata persistence API.\n"
"	Part 5.		Stripe locking on local SP.\n"
"	Part 6.		Memory service usage by objects.\n"
"	Part 7.		Zero on demand.\n"
"	Last Udapte: 11/10/11\n"
;

void kozma_prutkov_test_init_single_sp(void)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
	fbe_u32_t i;

    /* before loading the physical package we need to initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert port (0) */
    fbe_test_pp_util_insert_sas_pmc_port(1, 2, 0, &port_handle); /* io port, portal, backend number */ 											

    /* Insert an enclosure (0,0) */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port_handle, &encl_handle);

	/* Insert a SAS drives (0,0,i) */
    for (i = 0; i < 10; i++){
        fbe_test_pp_util_insert_sas_drive(0, 0, i, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY * 2, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(13, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Load Storage Extent Package */
	sep_config_load_sep();

	return;
}

void kozma_prutkov_test_dualsp_init(void)
{
	fbe_status_t status;
	mut_printf(MUT_LOG_LOW, "=== Init SPB ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	kozma_prutkov_test_init_single_sp();
	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	mut_printf(MUT_LOG_LOW, "=== Init SPA ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	kozma_prutkov_test_init_single_sp();
	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

void kozma_prutkov_test_destroy_single_sp(void)
{
    
    fbe_test_sep_util_destroy_sep_physical();
    return;
}

void kozma_prutkov_test_dualsp_destroy(void)
{
	mut_printf(MUT_LOG_LOW, "=== Kozma Prutkov destroy SPB ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	kozma_prutkov_test_destroy_single_sp();

	mut_printf(MUT_LOG_LOW, "=== Kozma Prutkov destroy SPA ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	kozma_prutkov_test_destroy_single_sp();

    return;
}

fbe_status_t kozma_prutkov_timer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t *)completion_context;
	fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

void kozma_prutkov_timer_test(void)
{
	fbe_packet_t * packet = NULL;
	fbe_semaphore_t sem;
	fbe_status_t status;

	mut_printf(MUT_LOG_LOW, "=== starting Timer test ===\n");
	fbe_semaphore_init(&sem, 0, 1);
	//fbe_transport_timer_init();  fbe test already initializes the transport.
	packet = malloc(sizeof(fbe_packet_t));
	fbe_transport_initialize_packet(packet);

	/* Timer test */
	fbe_transport_set_timer(packet,1000,kozma_prutkov_timer_completion,&sem);

	status = fbe_semaphore_wait_ms(&sem, 500);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);

	status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* Cancelation test */
	fbe_transport_set_timer(packet,1000,kozma_prutkov_timer_completion,&sem);

	status = fbe_semaphore_wait_ms(&sem, 500);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);

	fbe_transport_cancel_timer(packet); /* This will complete packet immediatelly */

	status = fbe_semaphore_wait_ms(&sem, 100);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	fbe_semaphore_destroy(&sem);

	fbe_transport_destroy_packet(packet);
	free(packet);
	//fbe_transport_timer_destroy();
	mut_printf(MUT_LOG_LOW, "=== Timer test completed successfully ===\n");
}

void kozma_prutkov_dualsp_test(void)
{
	fbe_status_t status;
	fbe_object_id_t pvd_object_id;	
	fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;

    fbe_test_wait_for_all_pvds_ready();

    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();


	kozma_prutkov_timer_test();

	do {
		fbe_api_sleep(100);
		/* We have to figure out whitch SP is ACTIVE */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		fbe_api_cmi_service_get_info(&spa_cmi_info);
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		fbe_api_cmi_service_get_info(&spb_cmi_info);

		if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_A;
			passive_connection_target = FBE_SIM_SP_B;
			mut_printf(MUT_LOG_LOW, "SPA is ACTIVE\n");
		}

		if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_B;
			passive_connection_target = FBE_SIM_SP_A;
			mut_printf(MUT_LOG_LOW, "SPB is ACTIVE\n");
		}
	}while(active_connection_target != FBE_SIM_SP_A && active_connection_target != FBE_SIM_SP_B);

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Wait for PVD object to be ready on ACTIVE side. \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Wait for PVD object to be ready on PASSIVE side. \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

//	kozma_prutkov_passive_request_test();

//	kozma_prutkov_memory_metadata_test();

//	kozma_prutkov_paged_metadata_operation_test();

//	kozma_prutkov_nonpaged_metadata_operation_test();

	kozma_prutkov_stripe_lock_test();

	kozma_prutkov_zero_on_demand_test();
    return;
}

static void kozma_prutkov_memory_metadata_test(void)
{
	fbe_status_t                    status;
	fbe_object_id_t                 object_id;
	fbe_provision_drive_control_get_metadata_memory_t provision_drive_control_get_metadata_memory;
    fbe_bool_t                      b_is_enabled = FBE_FALSE;
    fbe_api_provision_drive_info_t  provision_drive_info;
	fbe_lba_t                       zero_checkpoint = 0;
	fbe_lba_t                       current_zero_checkpoint = 0;

	mut_printf(MUT_LOG_LOW, "Memory Metadata Test Start ... \n");

    status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &object_id );

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

	mut_printf(MUT_LOG_LOW, "Memory Metadata: On SPA\n");
	status = fbe_api_provision_drive_get_metadata_memory(object_id, &provision_drive_control_get_metadata_memory);
	mut_printf(MUT_LOG_LOW, "Memory Metadata:  lifecycle_state = %d\n",
												provision_drive_control_get_metadata_memory.lifecycle_state);

	MUT_ASSERT_INT_EQUAL(provision_drive_control_get_metadata_memory.lifecycle_state, FBE_LIFECYCLE_STATE_READY);

	mut_printf(MUT_LOG_LOW, "Peer Memory Metadata:  lifecycle_state = %d\n",
												provision_drive_control_get_metadata_memory.lifecycle_state_peer);

	MUT_ASSERT_INT_EQUAL(provision_drive_control_get_metadata_memory.lifecycle_state_peer, FBE_LIFECYCLE_STATE_READY);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

	mut_printf(MUT_LOG_LOW, "Memory Metadata: On SPB\n");
	status = fbe_api_provision_drive_get_metadata_memory(object_id, &provision_drive_control_get_metadata_memory);
	mut_printf(MUT_LOG_LOW, "Memory Metadata:  lifecycle_state = %d\n",
												provision_drive_control_get_metadata_memory.lifecycle_state);

	MUT_ASSERT_INT_EQUAL(provision_drive_control_get_metadata_memory.lifecycle_state, FBE_LIFECYCLE_STATE_READY);

	mut_printf(MUT_LOG_LOW, "Peer Memory Metadata:  lifecycle_state = %d\n",
												provision_drive_control_get_metadata_memory.lifecycle_state_peer);

	MUT_ASSERT_INT_EQUAL(provision_drive_control_get_metadata_memory.lifecycle_state_peer, FBE_LIFECYCLE_STATE_READY);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Nonpaged Metadata Test Start ... \n");
	mut_printf(MUT_LOG_LOW, "Set target server to SPA\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Set the check point to the second chunk.  The checkpoint must be aligned
     * to the provision drive chunk size.
     */
    status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable disk zeroing background operation 
     */
    status = fbe_api_base_config_disable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the lowest allowable background zero checkpoint plus (1) chunk.
     */
    status = fbe_api_provision_drive_get_info(object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    zero_checkpoint = provision_drive_info.default_offset + provision_drive_info.default_chunk_size;
    
    /* Now set the checkpoint
     */ 
	status = fbe_api_provision_drive_set_zero_checkpoint(object_id, zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* confirm that the checkpoint hasn't changed*/
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id,  &current_zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(current_zero_checkpoint == zero_checkpoint);

	mut_printf(MUT_LOG_LOW, "Get zero checkpoint on SPB\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

	current_zero_checkpoint = 0;
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &current_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(current_zero_checkpoint == zero_checkpoint);

    /* Re-enable background zeroing if required
     */
    if (b_is_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
	mut_printf(MUT_LOG_LOW, "Nonpaged Metadata Test Completed\n");

}

static void 
kozma_prutkov_paged_metadata_operation_test(void)
{
    fbe_status_t status;
	fbe_object_id_t object_id;
    fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
	fbe_u64_t 	metadata_offset;
	fbe_u32_t i;

	mut_printf(MUT_LOG_LOW, "Paged Metadata Test Start ... \n");

	status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &object_id );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	fbe_api_base_config_disable_background_operation(object_id , FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);

    paged_change_bits.metadata_offset = 0;
    paged_change_bits.metadata_record_data_size = 4;
    paged_change_bits.metadata_repeat_count = 1;
    paged_change_bits.metadata_repeat_offset = 0;

	mut_printf(MUT_LOG_LOW, "Step 1 \n");

    * (fbe_u32_t *)paged_change_bits.metadata_record_data = 0xFFFFFFFF;
    status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 2 \n");
    paged_get_bits.metadata_offset = 0;
    paged_get_bits.metadata_record_data_size = 8;
    paged_get_bits.get_bits_flags = 0;
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(* (fbe_u32_t *)paged_get_bits.metadata_record_data, 0);

	mut_printf(MUT_LOG_LOW, "Step 3 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0xF0E0F0E0F0E0F0E0;
    status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 4 \n");
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(* (fbe_u64_t *)paged_get_bits.metadata_record_data, 0xF0E0F0E0F0E0F0E0);

	mut_printf(MUT_LOG_LOW, "Step 5 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0x0B000B000B000B00;
    status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 6 \n");
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(* (fbe_u64_t *)paged_get_bits.metadata_record_data, 0xFBE0FBE0FBE0FBE0);

	mut_printf(MUT_LOG_LOW, "Step 7 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0xF0E0F0E0F0E0F0E0;
    status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 8 \n");
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(* (fbe_u32_t *)paged_get_bits.metadata_record_data, 0x0B000B000B000B00);


    /* Offset test */
    paged_change_bits.metadata_offset = 512 + 64;
    paged_change_bits.metadata_record_data_size = 8;
    paged_change_bits.metadata_repeat_count = 1;
    paged_change_bits.metadata_repeat_offset = 0;

	mut_printf(MUT_LOG_LOW, "Step 9 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0xFFFFFFFFFFFFFFFF;
    status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 10 \n");
    paged_get_bits.metadata_offset = 512 + 64;
    paged_get_bits.metadata_record_data_size = 8;
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(* (fbe_u32_t *)paged_get_bits.metadata_record_data, 0);

	mut_printf(MUT_LOG_LOW, "Step 11 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0xF0E0F0E0F0E0F0E0;
    status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 12 \n");
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(* (fbe_u64_t *)paged_get_bits.metadata_record_data, 0xF0E0F0E0F0E0F0E0);

	mut_printf(MUT_LOG_LOW, "Step 13 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0x0B000B000B000B00;
    status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 14 \n");
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(* (fbe_u64_t *)paged_get_bits.metadata_record_data, 0xFBE0FBE0FBE0FBE0);

	mut_printf(MUT_LOG_LOW, "Step 15 \n");
    * (fbe_u64_t *)paged_change_bits.metadata_record_data = 0xF0E0F0E0F0E0F0E0;
    status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 16 \n");
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(* (fbe_u64_t *)paged_get_bits.metadata_record_data, 0x0B000B000B000B00);


    /* Large size get test */
    /* Paged metadata record is 64 blocks and we want to read 64 bytes on the border of two blocks */
    metadata_offset = 5*(512 * 64) - 32; /* 32 bytes from block 5 and 32 bytes from block 6 */
    paged_change_bits.metadata_record_data_size = sizeof(fbe_api_provisional_drive_paged_bits_t);
    paged_change_bits.metadata_repeat_count = 1;
    paged_change_bits.metadata_repeat_offset = 0;

    paged_get_bits.metadata_record_data_size = sizeof(fbe_api_provisional_drive_paged_bits_t); /* test is operating on a PVD, should use paged md size of PVD */

	mut_printf(MUT_LOG_LOW, "Step 17 \n");
	/* Clear 64 bytes */
	for(i = 0; i < 64; i++){
		paged_change_bits.metadata_offset = metadata_offset + i*paged_change_bits.metadata_record_data_size;
		paged_change_bits.metadata_record_data[0] = 0xFF;
		status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	    paged_get_bits.metadata_offset = metadata_offset + i*paged_get_bits.metadata_record_data_size;
		status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[0], 0);
	}

	mut_printf(MUT_LOG_LOW, "Step 18 \n");
	/* Fill 64 bytes with incremental data */
	for(i = 0; i < 64; i++){
		paged_change_bits.metadata_offset = metadata_offset + i*paged_change_bits.metadata_record_data_size;
		paged_change_bits.metadata_record_data[0] = i;
		status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		paged_get_bits.metadata_offset = metadata_offset + i*paged_get_bits.metadata_record_data_size;
		status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[0], i);
	}

#if 0 /* This operation is illegal */
	mut_printf(MUT_LOG_LOW, "Step 19 \n");
	/* Read all 64 bytes at once */
	paged_get_bits.metadata_offset = metadata_offset;
    paged_get_bits.metadata_record_data_size = 64;
	status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	for(i = 0; i < 64; i++){
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i], i);
	}
#endif

	mut_printf(MUT_LOG_LOW, "Step 20 \n");
	metadata_offset = 512 * 64 - 4; /* 64 blocks */

	/* Clear 8 bytes */
	for(i = 0; i < 8; i++){
		paged_change_bits.metadata_offset = metadata_offset + i*paged_change_bits.metadata_record_data_size;
		paged_change_bits.metadata_record_data[0] = 0xFF;
		status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	    paged_get_bits.metadata_offset = metadata_offset + i*paged_get_bits.metadata_record_data_size;
		status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[0], 0);
	}

	mut_printf(MUT_LOG_LOW, "Step 21 \n");
	/* Fill 8 bytes with incremental data */
	for(i = 0; i < 8; i++){
		paged_change_bits.metadata_offset = metadata_offset + i*paged_change_bits.metadata_record_data_size;
		paged_change_bits.metadata_record_data[0] = i;
		status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	    paged_get_bits.metadata_offset = metadata_offset + i*paged_get_bits.metadata_record_data_size;
		status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[0], i);
	}


	mut_printf(MUT_LOG_LOW, "Step 22 \n");
	/* Read on the boundaries */
	paged_get_bits.metadata_offset = metadata_offset;
	paged_get_bits.metadata_record_data_size = 8*paged_change_bits.metadata_record_data_size;
	status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	for(i = 0; i < 8; i++){
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i*paged_change_bits.metadata_record_data_size], i);
	}


	mut_printf(MUT_LOG_LOW, "Step 23 \n");
	metadata_offset = 512 * 128 - 4; /* 64 blocks */
	paged_get_bits.metadata_record_data_size = paged_change_bits.metadata_record_data_size;

	/* Clear 8 bytes */
	for(i = 0; i < 8; i++){
		paged_change_bits.metadata_offset = metadata_offset + i*paged_change_bits.metadata_record_data_size;
		paged_change_bits.metadata_record_data[0] = 0xFF;
		status = fbe_api_base_config_metadata_paged_clear_bits(object_id, &paged_change_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		paged_get_bits.metadata_offset = metadata_offset + i*paged_get_bits.metadata_record_data_size;
		status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[0], 0);
	}

	mut_printf(MUT_LOG_LOW, "Step 24 \n");
	/* Fill 8 bytes with incremental data */
	for(i = 0; i < 8; i++){
		paged_change_bits.metadata_offset = metadata_offset + i*paged_change_bits.metadata_record_data_size;
		paged_change_bits.metadata_record_data[0] = i;
		status = fbe_api_base_config_metadata_paged_set_bits(object_id, &paged_change_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		paged_get_bits.metadata_offset = metadata_offset + i*paged_get_bits.metadata_record_data_size;
		status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[0], i);
	}


	mut_printf(MUT_LOG_LOW, "Step 25 \n");
	/* Read on the boundaries */
	paged_get_bits.metadata_offset = metadata_offset;
	paged_get_bits.metadata_record_data_size = 8*paged_change_bits.metadata_record_data_size;
	status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	for(i = 0; i < 8; i++){
		MUT_ASSERT_INT_EQUAL(paged_get_bits.metadata_record_data[i*paged_change_bits.metadata_record_data_size], i);
	}

	mut_printf(MUT_LOG_LOW, "Paged Metadata Test Completed\n");

    return;
}

static void 
kozma_prutkov_nonpaged_metadata_operation_test(void)
{
    fbe_status_t status;
	fbe_object_id_t object_id;
    fbe_api_base_config_metadata_nonpaged_change_bits_t change_bits;
	fbe_lba_t                       get_zero_checkpoint = 0;
    fbe_bool_t                      b_is_enabled = FBE_FALSE;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       dchkp = 0;
    fbe_block_count_t               chksz = 0;
    fbe_u64_t                       zero_chkp_offset = 0;
	mut_printf(MUT_LOG_LOW, "NonPaged Metadata Test Start ... \n");

	status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &object_id );
    fbe_api_provision_drive_get_zero_checkpiont_offset(object_id,&zero_chkp_offset);
    change_bits.metadata_offset = zero_chkp_offset;
    change_bits.metadata_record_data_size = 4;
    change_bits.metadata_repeat_count = 2;
    change_bits.metadata_repeat_offset = 0;

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    * (fbe_u32_t *)change_bits.metadata_record_data = 0xFFFFFFFF;
    status = fbe_api_base_config_metadata_nonpaged_clear_bits(object_id, &change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0);


    change_bits.metadata_offset = zero_chkp_offset;
    change_bits.metadata_record_data_size = 1;
    change_bits.metadata_repeat_count = 8;
    change_bits.metadata_repeat_offset = 0;
	* (fbe_u8_t *)change_bits.metadata_record_data = 0xFA;

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_set_bits(object_id, &change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0xFAFAFAFAFAFAFAFA);

    change_bits.metadata_offset = zero_chkp_offset+1;
    change_bits.metadata_record_data_size = 1;
    change_bits.metadata_repeat_count = 6;
    change_bits.metadata_repeat_offset = 0;
	* (fbe_u8_t *)change_bits.metadata_record_data = 0xFA;

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_clear_bits(object_id, &change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0xFA000000000000FA);

	/* Set zero checkpoint to  0 */
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_set_checkpoint(object_id, zero_chkp_offset/*offset*/, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x0);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x0);

	/* Try to set zero checkpoint to  0x10 - it should fail */
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_set_checkpoint(object_id, zero_chkp_offset/*offset*/, 0x10);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x0);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x0);

	/* Increment checkpoint by 0x20 */
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_incr_checkpoint(object_id, zero_chkp_offset/*offset*/, 0 /*old_checkpoint*/, 0x20);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x20);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x20);

	/* Try to increment checkpoint with different old checkpoint - it should fail */
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_incr_checkpoint(object_id, zero_chkp_offset/*offset*/, 0 /*old_checkpoint*/, 0x20);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x20);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x20);

	/* Set checkpoint to 0x10 */
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);    
    status = fbe_api_base_config_metadata_nonpaged_set_checkpoint(object_id, zero_chkp_offset/*offset*/, 0x10);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x10);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);    
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == 0x10);

	mut_printf(MUT_LOG_LOW, "Non Paged Metadata Persist Test Start ...\n");
    /* Set the check point to the second chunk.  The checkpoint must be aligned
     * to the provision drive chunk size.
     */
    status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable disk zeroing background operation 
     */
    status = fbe_api_base_config_disable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the lowest allowable background zero checkpoint plus (1) chunk.
     */
    status = fbe_api_provision_drive_get_info(object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    dchkp = provision_drive_info.default_offset;
    chksz = provision_drive_info.default_chunk_size;

	status = fbe_api_provision_drive_set_zero_checkpoint(object_id, (dchkp + (chksz * 1)));
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 1)));

	/* Persist */
	fbe_api_metadata_nonpaged_persist();

	/* Set checkpoint to different value */
	status = fbe_api_provision_drive_set_zero_checkpoint(object_id, (dchkp + (chksz * 2)));
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 2)));

	/* Load */
	fbe_api_metadata_nonpaged_load();

	/* Check that we have the first value */
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 1)));

	mut_printf(MUT_LOG_LOW, "Non Paged Metadata Persist Test Completed\n");
	mut_printf(MUT_LOG_LOW, "Non Paged Metadata Single Persist Test Started ...\n");
	/* Test single object persist functionality */
	/* Set some checkpoint */
	status = fbe_api_provision_drive_set_zero_checkpoint(object_id, (dchkp + (chksz * 3)));
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 3)));

	/* Persist */
	fbe_api_base_config_metadata_nonpaged_persist(object_id);

	/* Set checkpoint to different value */
	status = fbe_api_provision_drive_set_zero_checkpoint(object_id, (dchkp + (chksz * 4)));
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 4)));

	/* Load */
	fbe_api_metadata_nonpaged_load();

	/* Check that we have the first value */
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 3)));

    /* Re-enable background zeroing if required
     */
    if (b_is_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }


	mut_printf(MUT_LOG_LOW, "Non Paged Metadata Single Persist Test Completed\n");

	mut_printf(MUT_LOG_LOW, "Non Paged Metadata System Persist Test Started ...\n");
	/* Test single object persist functionality */
	object_id = 1; /* Hardcoded system PVD */

    /* Set the check point to the second chunk.  The checkpoint must be aligned
     * to the provision drive chunk size.
     */
    status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable disk zeroing background operation 
     */
    status = fbe_api_base_config_disable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the lowest allowable background zero checkpoint plus (1) chunk.
     */
    status = fbe_api_provision_drive_get_info(object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    dchkp = provision_drive_info.default_offset;
    chksz = provision_drive_info.default_chunk_size;

	/* Set some checkpoint */
	status = fbe_api_provision_drive_set_zero_checkpoint(object_id , (dchkp + (chksz * 1)));
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 1)));

	/* Persist */
	fbe_api_base_config_metadata_nonpaged_persist(object_id);

	/* Set checkpoint to different value */
	status = fbe_api_provision_drive_set_zero_checkpoint(object_id, (dchkp + (chksz * 2)));
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 2)));

	/* Load */
	fbe_api_metadata_nonpaged_system_load();

	/* Check that we have the first value */
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &get_zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(get_zero_checkpoint == (dchkp + (chksz * 1)));

    /* Re-enable background zeroing if required
     */
    if (b_is_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
	mut_printf(MUT_LOG_LOW, "Non Paged Metadata System Persist Test Completed\n");

	mut_printf(MUT_LOG_LOW, "Non Paged Metadata Test Completed\n");

    return;
}


static void kozma_prutkov_stripe_lock_test(void)
{
	fbe_status_t status;
	fbe_object_id_t a_object_id;
	fbe_object_id_t b_object_id;
	fbe_object_id_t object_id;
	fbe_u32_t i;

	fbe_api_metadata_debug_lock_t debug_lock[4];
	fbe_packet_t * packet[4];
	fbe_semaphore_t sem;

	mut_printf(MUT_LOG_LOW, "Stripe Lock Test Start ... \n");

	fbe_semaphore_init(&sem, 0, 1);

	for(i = 0; i < 4; i++){
		packet[i] = malloc(sizeof(fbe_packet_t));
		fbe_transport_initialize_sep_packet(packet[i]);
	}

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &a_object_id );
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_wait_for_object_lifecycle_state(a_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &b_object_id );
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_wait_for_object_lifecycle_state(b_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	MUT_ASSERT_INT_EQUAL(a_object_id, b_object_id);

	object_id = a_object_id;

	mut_printf(MUT_LOG_LOW, "We will work with PVD %d \n", object_id);
	if(active_connection_target == FBE_SIM_SP_A){
		mut_printf(MUT_LOG_LOW, "Active side is SPA \n");
	}else {
		mut_printf(MUT_LOG_LOW, "Active side is SPB \n");
	}

	fbe_api_sim_transport_set_target_server(active_connection_target);

	for(i = 0 ; i < 4;i++){
		debug_lock[i].object_id = object_id;
		debug_lock[i].b_allow_hold = FBE_TRUE; 
		debug_lock[i].packet_ptr = NULL;
		debug_lock[i].stripe_number = 0;		
		debug_lock[i].stripe_count = 1;
	}

	/* Send start lock command */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START;
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


	mut_printf(MUT_LOG_LOW, "Step 1 (Send write lock on active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	/* Send write lock on active side */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
	
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 2 (Send another write lock to passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	/* Send another write lock and see that it is stuck */
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
	debug_lock[1].stripe_count = 2;

    fbe_transport_set_completion_function(packet[1], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_LOW, "Step 3 (Send write unlock to active side) ... \n");

	fbe_api_sim_transport_set_target_server(active_connection_target);

	/* Send write unlock */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 4 (Wait for completion of second write lock) ... \n");
	/* Wait for completion of second write lock */
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[1].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 5 (Send second write unlock on passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	/* Send second write unlock */
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
    fbe_transport_set_completion_function(packet[1], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

#if 1 /* Shared dual SP lock is temporarily disabled */
	mut_printf(MUT_LOG_LOW, "Step 6 (Send read lock on active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK;		
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);


	mut_printf(MUT_LOG_LOW, "Step 7 (Send another read lock to passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK;			
    fbe_transport_set_completion_function(packet[1], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[1].packet_ptr != NULL);


	mut_printf(MUT_LOG_LOW, "Step 8 (Send read unlock to active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK;
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 9 (Send write lock to active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	/* Send write lock and see that it is stuck */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);


	mut_printf(MUT_LOG_LOW, "Step 10 (Send read unlock to passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK;			
    fbe_transport_set_completion_function(packet[1], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


	mut_printf(MUT_LOG_LOW, "Step 11 (Wait for completion of write lock) ... \n");
	/* Wait for completion of second write lock */
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 12 (Send write unlock to active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;		
    fbe_transport_set_completion_function(packet[0], kozma_prutkov_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif
	for(i = 0; i < 4; i++){
		fbe_transport_destroy_packet(packet[i]);
		free(packet[i]);
	}

    fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */
	
	mut_printf(MUT_LOG_LOW, "Stripe Lock Test Test Completed\n");

}

static fbe_status_t 
kozma_prutkov_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t *)completion_context;
	fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}


static void kozma_prutkov_zero_on_demand_test(void)
{
	fbe_object_id_t object_id;
    fbe_payload_block_operation_t				block_operation;
    fbe_block_transport_block_operation_info_t	block_operation_info;
    fbe_raid_verify_error_counts_t				verify_err_counts={0};
	fbe_sg_element_t * sg_list;
	fbe_lba_t lba;
	fbe_u32_t block_count;
	fbe_u8_t * buffer;
	fbe_status_t status;
	fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
    fbe_u64_t paged_bitmap_data;
    fbe_u64_t paged_bitmap_data_2;
	fbe_lba_t curr_checkpoint;


	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 6, &object_id );
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Zero On Demand Test Start ...obj 0x%x\n", object_id);

	/* Set PVD debug flags */
	fbe_api_provision_drive_set_debug_flag(object_id, 0xffff);

    /* Note
      * When PVD reads paged metadata in ZOD code path and if it found the combination of 
      * NZ: 0, UZ: 0 and CU: 0 then it generate an error trace to prevent data corruption for bound PVD.
      * here we are sending IO directly to the unbound PVD. Since we have disabled background zeroing
      * on all PVDs at the start of this test, there will be NZ bit remain set and there will be no error trace. 
      * but we might need to perform this test diffent way to eliminate paged bitmap combination
      * NZ: 0, UZ: 0 and CU: 0 for unbound PVD.
	*/

	/* Send I/O */

	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &curr_checkpoint);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	lba = 0x7ffba;
	//FBE_PROVISION_DRIVE_CHUNK_SIZE                =    0x800,   /* in blocks (2048 blocks)    */
	if(lba < curr_checkpoint){
		lba = ((curr_checkpoint / 0x800) + 1 ) * 0x800 - 70;
	}

	block_count = 0x4b;
	buffer = fbe_api_allocate_memory(block_count * 520 + 2*sizeof(fbe_sg_element_t));
	sg_list = (fbe_sg_element_t *)buffer;
	sg_list[0].address = buffer + 2*sizeof(fbe_sg_element_t);
    sg_list[0].count = block_count * 520;

    sg_list[1].address = NULL;
    sg_list[1].count = 0;

	fbe_payload_block_build_operation(&block_operation,
									 FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
									 lba,
									 block_count,
									 520,
									 1,
									 NULL);

    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_err_counts;

	//FBE_PROVISION_DRIVE_CHUNK_SIZE                =    0x800,   /* in blocks (2048 blocks)    */
    paged_get_bits.metadata_offset = (lba / 0x800) * sizeof(fbe_provision_drive_paged_metadata_t); /* 8 bytes for each chunk *///0x1fe; /* 510 + 4 = 514 */
    paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t) * 2;
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
   status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

   /* paged bitmap can be one of the following based on background zeroing operation pass through on these chunks
     * VB - 1, NZ -1, UZ -0,  CD - 0 (2 byte - 0x0003) or 
     * VB - 1, NZ -0, UZ -0,  CD - 0 (2 byte - 0x0001)
     * so both values are valid one
     */
    memcpy(&paged_bitmap_data, &paged_get_bits.metadata_record_data[0], sizeof(fbe_u64_t));
    memcpy(&paged_bitmap_data_2, &paged_get_bits.metadata_record_data[8], sizeof(fbe_u64_t));

    mut_printf(MUT_LOG_LOW, "Zero On Demand Test paged bit before write 0x%llx, lba 0x%llx, curr chk 0x%llx\n", paged_bitmap_data, lba, curr_checkpoint);
    mut_printf(MUT_LOG_LOW, "Zero On Demand Test paged bit 2 before write 0x%llx, lba 0x%llx, curr chk 0x%llx\n", paged_bitmap_data_2, lba, curr_checkpoint);

    if(!((paged_bitmap_data  == 0x3) ||
           (paged_bitmap_data == 0x1) ||
           (paged_bitmap_data_2 == 0x3) ||
           (paged_bitmap_data_2 == 0x1)))
    {

        mut_printf(MUT_LOG_TEST_STATUS, "kozma_prutkov ZOD test fail- Received paged bits 0x%x are not 0x00030003 or 0x00010001 ****\n", 
                        * (fbe_u32_t *)paged_get_bits.metadata_record_data);
        MUT_FAIL();
    }

    fbe_api_block_transport_send_block_operation(object_id, FBE_PACKAGE_ID_SEP_0, &block_operation_info, sg_list);

    paged_get_bits.metadata_offset = (lba / 0x800) * sizeof(fbe_provision_drive_paged_metadata_t); /* 2 bytes for each chunk */ //0x1fe; /* 510 + 4 = 514 */
    paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t) * 2;
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* paged bitmap will be VB - 1, NZ -0, UZ -0,  CD - 1 (2 byte - 0x0009)
     */
    memcpy(&paged_bitmap_data, &paged_get_bits.metadata_record_data[0], sizeof(fbe_u64_t));
    memcpy(&paged_bitmap_data_2, &paged_get_bits.metadata_record_data[8], sizeof(fbe_u64_t));
    mut_printf(MUT_LOG_LOW, "Zero On Demand Test paged bit after write 0x%llx\n", paged_bitmap_data);
    MUT_ASSERT_UINT64_EQUAL(paged_bitmap_data, 0x9);
    mut_printf(MUT_LOG_LOW, "Zero On Demand Test paged bit after write 0x%llx\n", paged_bitmap_data_2);
    MUT_ASSERT_UINT64_EQUAL(paged_bitmap_data_2, 0x9);
    fbe_api_block_transport_send_block_operation(object_id, FBE_PACKAGE_ID_SEP_0, &block_operation_info, sg_list);

    fbe_api_free_memory(buffer);

    mut_printf(MUT_LOG_LOW, "Zero On Demand Test Test Completed\n");
    
}

static void kozma_prutkov_passive_request_test(void)
{
	fbe_status_t status;
	fbe_metadata_info_t metadata_info;
	fbe_object_id_t pvd_object_id;
	fbe_u32_t slot;

	for(slot = 5; slot < 9; slot ++){
		status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, slot, &pvd_object_id);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		mut_printf(MUT_LOG_LOW, "Check that PVD slot %d object\n", slot);
		fbe_api_sim_transport_set_target_server(active_connection_target);

		status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		status = fbe_api_base_config_metadata_get_info(pvd_object_id, &metadata_info);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		if(slot % 2){
			MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);
		} else {
			MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);
		}

		mut_printf(MUT_LOG_LOW, "Check that PVD slot %d object\n", slot);
		fbe_api_sim_transport_set_target_server(passive_connection_target);

		status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		status = fbe_api_base_config_metadata_get_info(pvd_object_id, &metadata_info);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		if(slot % 2){
			MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);
		} else {
			MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);
		}
	}


    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 6, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Check that PVD object is ACTIVE on ACTIVE side. \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
    status = fbe_api_base_config_metadata_get_info(pvd_object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);

	mut_printf(MUT_LOG_LOW, "Check that PVD object is PASSIVE on PASSIVE side. \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_base_config_metadata_get_info(pvd_object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);


	mut_printf(MUT_LOG_LOW, "Ask ACTIVE PVD object is to become PASSIVE. \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	status = fbe_api_base_config_passive_request(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sleep(100); /* Sleep a little bit */
    status = fbe_api_base_config_metadata_get_info(pvd_object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);

	mut_printf(MUT_LOG_LOW, "Check that PASSIVE PVD object become ACTIVE. \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
    status = fbe_api_base_config_metadata_get_info(pvd_object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);

}
