/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file snakeskin_renew_test.c
 ***************************************************************************
 *
 * @brief
 *  This test is to test the feature about non-paged metadata and metadata memory
 *  versioning. It covers the following feature:
 *  1) Check whether the size information along with the non-paged metadata
 *     is initialized and persist correctly;
 *  2) Check whether the new condition of base_config can detect the version
 *     inconsistency of non-paged metadata and upgrade it correctly
 *  3) Check whether the size information of metadata memory is initialized correctly
 *
 * @version
 *   4/24/2012:  Created. Jingcheng Zhang
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
#include "fbe/fbe_api_job_service_interface.h"


#define SNAKESKIN_PVD_METADATA_MEMORY_SIZE 0x40
#define SNAKESKIN_TEST_RAID_GROUP_NUMBER 21


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void snakeskin_check_nonpaged_metadata_version_persist(void);
static void snakeskin_test_old_version_data(void);
static void snakeskin_renew_test_metadata_memory_version(void);
void snakeskin_test_nonpaged_multi_version_cmi_update(void);


static fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
static fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;

/*************************
 *   TEST DESCRIPTION
 *************************/
char * snakeskin_short_desc = "Test Database/Metadata Versioning";
char * snakeskin_long_desc =
"Versioning tests the ability of the infrastructure to perform the upgrade correctly. \n"
"	Part 1.		Check non-paged metadata initialized and persist with correct size.\n"
"	Part 2.		Check non-paged metadata CMI handle the version correctly.\n"
"	Part 3.		Check metadata memory initialized with correct size.\n"
"	Last Udapte: 4/24/12\n"
;

void snakeskin_test_init_single_sp(void)
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
        fbe_test_pp_util_insert_sas_drive(0, 0, i, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
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

void snakeskin_test_dualsp_init(void)
{
	fbe_status_t status;
	mut_printf(MUT_LOG_LOW, "=== Init SPB ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	snakeskin_test_init_single_sp();
	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	mut_printf(MUT_LOG_LOW, "=== Init SPA ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	snakeskin_test_init_single_sp();
	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

void snakeskin_test_destroy_single_sp(void)
{
    
    fbe_test_sep_util_destroy_sep_physical();
    return;
}

void snakeskin_test_dualsp_destroy(void)
{
	mut_printf(MUT_LOG_LOW, "=== Snakeskin Test destroy SPB ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	snakeskin_test_destroy_single_sp();

	mut_printf(MUT_LOG_LOW, "=== Snakeskin Test destroy SPA ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	snakeskin_test_destroy_single_sp();

    return;
}


void snakeskin_dualsp_test(void)
{
	fbe_status_t status;
	fbe_object_id_t pvd_object_id;	
	fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;


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

    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();

	snakeskin_check_nonpaged_metadata_version_persist();

    snakeskin_test_old_version_data();

    snakeskin_renew_test_metadata_memory_version();

    snakeskin_test_nonpaged_multi_version_cmi_update();

    return;
}

void snakeskin_create_raid_group(fbe_object_id_t raid_group_id,
                                 fbe_lba_t raid_group_capacity,
                                 fbe_u32_t drive_count, 
                                 fbe_raid_group_type_t raid_group_type,
                                 fbe_object_id_t *raid_obj_id)
{
    fbe_status_t status;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request;
    fbe_u32_t                                         index = 0;
    fbe_object_id_t 					              raid_group_id1 = FBE_OBJECT_ID_INVALID;
	fbe_job_service_error_type_t 					  job_error_code;
    fbe_status_t 									  job_status;
  
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__, raid_group_type);

    fbe_zero_memory(&fbe_raid_group_create_request,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_raid_group_create_request.drive_count = drive_count;
    fbe_raid_group_create_request.raid_group_id = raid_group_id;

    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        fbe_raid_group_create_request.disk_array[index].bus       = 0;
        fbe_raid_group_create_request.disk_array[index].enclosure = 0;
        fbe_raid_group_create_request.disk_array[index].slot      = index + 6;
    }

    /* b_bandwidth = true indicates 
     * FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 1024 for element_size 
     */
    fbe_raid_group_create_request.b_bandwidth           = FBE_FALSE; 
    fbe_raid_group_create_request.capacity              = raid_group_capacity; 
    fbe_raid_group_create_request.raid_type             = raid_group_type;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.job_number            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = 60000;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n", (unsigned long long)fbe_raid_group_create_request.job_number);

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
										 60000,
										 &job_error_code,
										 &job_status,
										 NULL);

	MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
	MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, 
                                                               &raid_group_id1);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id1);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id1, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__,
            raid_group_type);
    if (raid_obj_id) 
        *raid_obj_id = raid_group_id1;
}

void snakeskin_test_system_reboot(void)
{
    fbe_status_t status;
    fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;

    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_sep();
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_sep();
   
    sep_config_load_sep_with_time_save(FBE_TRUE);

    status = fbe_test_sep_util_wait_for_database_service(30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

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

}


/*Test the version and size information is initialized and persist correctly*/
void snakeskin_check_nonpaged_metadata_version_persist(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_metadata_nonpaged_data_t nonpaged_data;
    fbe_u32_t version_size = 0;
    fbe_object_id_t object_id = 0;

    mut_printf(MUT_LOG_LOW, "Non Paged Metadata Size Initialization and Persist Test Started ...\n");
    /*test if the non-paged metadata is initialized with correct size*/
    status = fbe_api_base_config_metadata_get_data(0x1, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_provision_drive_nonpaged_metadata_t));

    /*create a LUN and test if its size of non-paged metadata is initialized correctly*/
    snakeskin_create_raid_group(SNAKESKIN_TEST_RAID_GROUP_NUMBER, FBE_LBA_INVALID, 4, FBE_RAID_GROUP_TYPE_RAID5, &object_id);
    status = fbe_api_base_config_metadata_get_data(object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_raid_group_nonpaged_metadata_t));

    /*Test persist with correct size*/
    /* Persist */
	fbe_api_base_config_metadata_nonpaged_persist(object_id);

    /* Load */
	fbe_api_metadata_nonpaged_load();
    status = fbe_api_base_config_metadata_get_data(object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_raid_group_nonpaged_metadata_t));

    /* Load system non-paged metadata and check the size*/
	fbe_api_metadata_nonpaged_system_load();
    status = fbe_api_base_config_metadata_get_data(0x1, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_provision_drive_nonpaged_metadata_t));

    /*reboot system and test the object version can be initialized correctly*/
    mut_printf(MUT_LOG_LOW, "Reboot System and Test object version initialized correctly ...\n");
    snakeskin_test_system_reboot();
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();


    status = fbe_api_base_config_metadata_get_data(0x1, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_provision_drive_nonpaged_metadata_t));

    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(SNAKESKIN_TEST_RAID_GROUP_NUMBER, 
                                                          &object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_base_config_metadata_get_data(object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_raid_group_nonpaged_metadata_t));

    mut_printf(MUT_LOG_LOW, "Non Paged Metadata Size Initialization and Persist Test Completed ...\n");
}

void snakeskin_test_old_version_data(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_metadata_nonpaged_data_t nonpaged_data;
	fbe_object_id_t object_id;
    fbe_api_base_config_metadata_nonpaged_change_bits_t change_bits;
    fbe_u32_t org_size;
    fbe_u32_t version_size;
    fbe_u32_t index = 0;

    change_bits.metadata_offset = 0;
    change_bits.metadata_record_data_size = 4;
    change_bits.metadata_repeat_count = 1;
    change_bits.metadata_repeat_offset = 0;


    mut_printf(MUT_LOG_LOW, "Non Paged Metadata Run Old Version Data Test Started ...\n");
    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(SNAKESKIN_TEST_RAID_GROUP_NUMBER, &object_id);
    org_size = sizeof (fbe_raid_group_nonpaged_metadata_t);
    index = 0;
    for (index = 0; !((org_size >> index) & 0x1); index ++) {
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);  
    if (index > 0) {
        * (fbe_u32_t *)change_bits.metadata_record_data = (1 << index) - 1;
        status = fbe_api_base_config_metadata_nonpaged_set_bits(object_id, &change_bits);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    * (fbe_u32_t *)change_bits.metadata_record_data = (1 << index);
    status = fbe_api_base_config_metadata_nonpaged_clear_bits(object_id, &change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_base_config_metadata_get_data(object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_raid_group_nonpaged_metadata_t) - 1);
    /* Persist */
	fbe_api_base_config_metadata_nonpaged_persist(object_id);

    snakeskin_test_system_reboot();
    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(SNAKESKIN_TEST_RAID_GROUP_NUMBER, &object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_base_config_metadata_get_data(object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    version_size = ((fbe_sep_version_header_t *)nonpaged_data.data)->size;
    MUT_ASSERT_INT_EQUAL(version_size, sizeof (fbe_raid_group_nonpaged_metadata_t));

    mut_printf(MUT_LOG_LOW, "Non Paged Metadata Run Old Version Data Test Completed ...\n");
}


void snakeskin_renew_test_metadata_memory_version(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t object_id = 0;
	fbe_provision_drive_control_get_versioned_metadata_memory_t get_metadata_memory;

    status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &object_id );

    mut_printf(MUT_LOG_LOW, "Check Version Size of Metadata Memory On SPA\n");

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_provision_drive_get_versioned_metadata_memory(object_id, &get_metadata_memory);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(get_metadata_memory.ver_header.size, SNAKESKIN_PVD_METADATA_MEMORY_SIZE);
    MUT_ASSERT_INT_EQUAL(get_metadata_memory.peer_ver_header.size, SNAKESKIN_PVD_METADATA_MEMORY_SIZE);

    mut_printf(MUT_LOG_LOW, "Check Version Size of Metadata Memory Completed\n");
}


void snakeskin_test_nonpaged_multi_version_cmi_update(void)
{
    fbe_base_config_control_metadata_set_size_t set_nonpaged_size;
    fbe_object_id_t spa_object_id;
    fbe_object_id_t spb_object_id;
    fbe_status_t status;
    fbe_api_base_config_metadata_nonpaged_change_bits_t change_bits;
    fbe_metadata_nonpaged_data_t nonpaged_data;
    fbe_u8_t   np_flags;
    fbe_u8_t   old_invalid_value;
    fbe_u32_t enlarge_size = 32;
    const fbe_u32_t max_np_size = sizeof(change_bits.metadata_record_data);

    mut_printf(MUT_LOG_LOW, "Non Paged Metadata Multi-Version CMI Update Test Started ...\n");

    /* find the object id of the raid group */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);  
    fbe_api_database_lookup_raid_group_by_number(SNAKESKIN_TEST_RAID_GROUP_NUMBER, &spa_object_id);

    /*change non-paged metadata size in SPA to a larger size then we can simulate a new 
      version non-paged metadata CMI update*/
    if (sizeof(fbe_raid_group_nonpaged_metadata_t) > max_np_size) {
        /* We overflow now. All APIs that update nonpaged will be broken.
         * So failed here.
         */
        mut_printf(MUT_LOG_LOW, "%s: Nonpaged metadata too large for APIs.\n", __FUNCTION__);
        mut_printf(MUT_LOG_LOW, "    sizeof fbe_raid_group_nonpaged_metadata_t: %u, API size: %u\n",
                   (fbe_u32_t)sizeof(fbe_raid_group_nonpaged_metadata_t),
                   (fbe_u32_t)max_np_size);
        MUT_FAIL();
    }

    if (sizeof(fbe_raid_group_nonpaged_metadata_t) + enlarge_size > max_np_size) {
        /* The space left for NDU is too small now. Complain here */
        mut_printf(MUT_LOG_LOW, "%s: WARNING: sizeof nonpaged metadata will overflow soon:\n", __FUNCTION__);
        mut_printf(MUT_LOG_LOW, "    sizeof fbe_raid_group_nonpaged_metadata_t: %u, API size: %u\n",
                   (fbe_u32_t)sizeof(fbe_raid_group_nonpaged_metadata_t),
                   (fbe_u32_t)max_np_size);
        enlarge_size = max_np_size - sizeof(fbe_raid_group_nonpaged_metadata_t);
        mut_printf(MUT_LOG_LOW, "    reduce new size to %u\n", enlarge_size);
    }
    set_nonpaged_size.set_ver_header = 0;
    set_nonpaged_size.new_size = sizeof (fbe_raid_group_nonpaged_metadata_t) + enlarge_size;
    status = fbe_api_base_config_metadata_set_nonpaged_size(spa_object_id, &set_nonpaged_size);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    change_bits.metadata_offset = (fbe_u32_t)&(((fbe_raid_group_nonpaged_metadata_t *)0)->raid_group_np_md_flags);
    change_bits.metadata_record_data_size = 1;
    change_bits.metadata_repeat_count = 5;
    change_bits.metadata_repeat_offset = 0;
    * (fbe_u8_t *)change_bits.metadata_record_data = 0xFF;
    status = fbe_api_base_config_metadata_nonpaged_clear_bits(spa_object_id, &change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_base_config_metadata_get_data(spa_object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    np_flags = ((fbe_raid_group_nonpaged_metadata_t *)nonpaged_data.data)->raid_group_np_md_flags;
    MUT_ASSERT_INT_EQUAL(np_flags, 0);



    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);  
    fbe_api_database_lookup_raid_group_by_number(SNAKESKIN_TEST_RAID_GROUP_NUMBER, &spb_object_id);
    status = fbe_api_base_config_metadata_get_data(spb_object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    old_invalid_value =  ((fbe_raid_group_nonpaged_metadata_t *)nonpaged_data.data)->raid_group_np_md_flags;

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    * (fbe_u8_t *)change_bits.metadata_record_data = 0x30;
    mut_printf(MUT_LOG_LOW, "Non Paged Metadata update: off %d, size %d, total_size %d \n",
               (int)change_bits.metadata_offset, 5, (int)sizeof (fbe_raid_group_nonpaged_metadata_t));
    status = fbe_api_base_config_metadata_nonpaged_set_bits(spa_object_id, &change_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /*Validate the CMI update from SPB side*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_base_config_metadata_get_data(spb_object_id, &nonpaged_data);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    np_flags = ((fbe_raid_group_nonpaged_metadata_t *)nonpaged_data.data)->raid_group_np_md_flags;
    MUT_ASSERT_INT_EQUAL(np_flags, 0x30);
    MUT_ASSERT_INT_NOT_EQUAL(np_flags, old_invalid_value);

    mut_printf(MUT_LOG_LOW, "Non Paged Metadata Multi-Version CMI Update Test Done\n");
}


