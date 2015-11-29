/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_sep_encryption_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains Encryption related test functions.
 *
 * @version
 *   10/25/2013:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "sep_tests.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe_spare.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_terminator_drive_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_random.h"
#include "pp_utils.h"
#include "neit_utils.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_database.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe_raid_library.h"
#include "fbe_test.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "sep_hook.h"

static fbe_u32_t fbe_test_current_generation = 0;
void fbe_test_encryption_init(void)
{
    fbe_test_current_generation = 0;
}
static fbe_u32_t fbe_test_encryption_get_generation(void)
{
    if (fbe_test_current_generation == 0) {
        /* First generation means we are not encrypted. */
        return FBE_U32_MAX;
    }
    return fbe_test_current_generation;
}
static fbe_u32_t fbe_test_encryption_get_next_generation(void)
{
    return fbe_test_current_generation + 1;
}
static void fbe_test_encryption_inc_generation(void)
{
    fbe_test_current_generation++;
}
void fbe_test_encryption_set_rg_keys(fbe_test_rg_configuration_t *current_rg_config_p) 
{
    fbe_test_sep_util_set_rg_keys(current_rg_config_p, 
                                  0, fbe_test_encryption_get_generation(), 
                                  1, fbe_test_encryption_get_next_generation(),
                                  FBE_TRUE    /* Yes start */);
}

void fbe_test_encryption_wait_rg_rekey(fbe_test_rg_configuration_t * const rg_config_p, fbe_bool_t is_kms_enabled)
{
    fbe_status_t status;
    fbe_bool_t b_dualsp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_WAIT_CURRENT);

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 
    /* Wait for the encryption rekey to finish 
     */
    if (b_dualsp) {
        fbe_api_sim_transport_set_target_server(other_sp);
		/* When KMS is enabled we can miss this state */
		if(!is_kms_enabled){
			status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE,
																	FBE_TEST_WAIT_TIMEOUT_MS,
																	FBE_TRUE /* yes check top level */);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);		
			mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey state complete.");
		}

        fbe_api_sim_transport_set_target_server(this_sp);
    }
	/* When KMS is enabled we can miss this state */
	if(!is_kms_enabled){
		status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE,
																FBE_TEST_WAIT_TIMEOUT_MS,
																FBE_TRUE /* yes check top level */);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey state complete.");
	}

    /* Make the "new key" the current key.
     */
    fbe_test_encryption_inc_generation();

    /* Key Index 1 has no key since we are completing.
     * Key Index 0 has the new key (key 1).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption finalize rekey.");
	if(!is_kms_enabled){
        fbe_test_sep_util_complete_rg_encryption(rg_config_p);
	}

    if (b_dualsp) {
        fbe_api_sim_transport_set_target_server(other_sp); 
        status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED,
                                                                FBE_TEST_WAIT_TIMEOUT_MS,
                                                                FBE_FALSE /* do not check top level */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey finished successfully.");
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    status = fbe_test_sep_util_wait_all_rg_encryption_state(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED,
                                                            FBE_TEST_WAIT_TIMEOUT_MS,
                                                            FBE_FALSE /* do not check top level */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey finished successfully.");
}

/*!**************************************************************
 * fbe_test_sep_util_setup_kek()
 ****************************************************************
 * @brief
 *  Make sure the key encryption key was setup properly for a RG.
 *
 * @param rg_config_ptr - RG to validate for.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void fbe_test_sep_util_setup_kek(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_database_control_setup_kek_t encryption_info;
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_status_t        status;
    fbe_u32_t           port_idx;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_u32_t get_total_sp_count;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that KEK are setup correctly via the topology  ===\n");

    get_total_sp_count = (fbe_test_sep_util_get_dualsp_test_mode() == FBE_TRUE) ? 2 : 1;

    fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
    be_port_info.sp_id = FBE_CMI_SP_ID_A;
    for(sp_count = 0; sp_count < get_total_sp_count; sp_count++) 
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Validate that the key is sent over to each of the port correctly and validate
         * the keys valus are correct */
        for (port_idx = 0; port_idx < be_port_info.num_be_ports; port_idx++) 
        {
            fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);
            fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_kek_t));
            mut_printf(MUT_LOG_TEST_STATUS, "=== Setting up KEK for port num:%d  ===\n", port_idx);
            encryption_info.sp_id = be_port_info.sp_id;
            encryption_info.port_object_id = be_port_info.port_info[port_idx].port_object_id;
            _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBEKEK%d\n",port_idx);
            fbe_copy_memory(&encryption_info.kek, keys, FBE_ENCRYPTION_KEY_SIZE);
            encryption_info.key_size = FBE_ENCRYPTION_KEY_SIZE;

            /*  Send this key over to the topology to be sent to miniport */
            status = fbe_api_database_setup_kek(&encryption_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
        fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== Completed setup of KEKs  ===\n");
    return;
}
/******************************************
 * end fbe_test_sep_util_setup_kek()
 ******************************************/ 
/*!**************************************************************
 * fbe_test_sep_util_validate_and_destroy_kek_setup()
 ****************************************************************
 * @brief
 *  Make sure the key encryption key was setup properly for a RG.
 *
 * @param rg_config_ptr - RG to validate for.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void fbe_test_sep_util_validate_and_destroy_kek_setup(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_status_t        status;
    fbe_api_terminator_device_handle_t port_handle;
    fbe_u8_t            terminator_key[FBE_ENCRYPTION_KEY_SIZE];
    fbe_bool_t          key_match = 1;
    fbe_key_handle_t    kek_handle;
    fbe_u32_t           port_idx;
    fbe_database_control_destroy_kek_t kek_encryption_info;
    fbe_database_control_get_be_port_info_t be_port_info;
    fbe_u32_t sp_count = 0;
    fbe_sim_transport_connection_target_t  current_sp;
    fbe_sim_transport_connection_target_t  target_sp;
    fbe_u32_t get_total_sp_count;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that KEK were sent correctly via the topology  ===\n");

    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);
    fbe_zero_memory(terminator_key, FBE_ENCRYPTION_KEY_SIZE);


    fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
    be_port_info.sp_id = FBE_CMI_SP_ID_A;

    current_sp = fbe_api_sim_transport_get_target_server();
    target_sp = FBE_SIM_SP_A; 

    get_total_sp_count = (fbe_test_sep_util_get_dualsp_test_mode() == FBE_TRUE) ? 2 : 1;
    for(sp_count = 0; sp_count < get_total_sp_count; sp_count++) 
    {
        status = fbe_api_database_get_be_port_info(&be_port_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Validate that the key is sent over to each of the port correctly and validate
         * the keys valus are correct */
        for (port_idx = 0; port_idx < be_port_info.num_be_ports; port_idx++) 
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that KEK for port num:%d  ===\n", port_idx);

            /* Get the local SP ID */
            
            fbe_api_sim_transport_set_target_server(target_sp); 
            _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBEKEK%d\n",port_idx);
            status = fbe_api_port_get_kek_handle(be_port_info.port_info[port_idx].port_object_id, &kek_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_terminator_get_port_handle(port_idx,
                                                        &port_handle);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* From the key handle get the key value and make sure they match */
            fbe_api_terminator_get_encryption_keys(port_handle, kek_handle, terminator_key);
            key_match = fbe_equal_memory(&keys[0], terminator_key, FBE_ENCRYPTION_KEY_SIZE);
            MUT_ASSERT_TRUE_MSG((key_match == 1), "Key values does NOT match\n");

            kek_encryption_info.sp_id = be_port_info.sp_id;
            kek_encryption_info.port_object_id = be_port_info.port_info[port_idx].port_object_id;

            /* We want to always target one SP to test the CMI path */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            status = fbe_api_database_destroy_kek(&kek_encryption_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
        fbe_zero_memory(&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));
        be_port_info.sp_id = FBE_CMI_SP_ID_B;
        target_sp = FBE_SIM_SP_B;
    }

    fbe_api_sim_transport_set_target_server(current_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Completed Validation of KEKs  ===\n");
    return;

}
/******************************************
 * end fbe_test_sep_util_validate_kek_setup()
 ******************************************/ 

/*!**************************************************************
 * fbe_test_sep_util_validate_key_setup()
 ****************************************************************
 * @brief
 *  Make sure this raid group has keys setup properly.
 *
 * @param rg_config_ptr - RG to validate for.
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void fbe_test_sep_util_validate_key_setup(fbe_test_rg_configuration_t *rg_config_ptr)
{
    fbe_u32_t           index = 0;
    fbe_object_id_t     rg_object_id;              
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_status_t        status;
    fbe_provision_drive_control_get_mp_key_handle_t mp_key_handle;
    fbe_api_terminator_device_handle_t port_handle;
    fbe_u8_t            terminator_key[FBE_ENCRYPTION_KEY_SIZE];
    fbe_object_id_t     pvd_id;
    fbe_bool_t          key_match = 1;
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_api_provision_drive_info_t        provision_drive_info;
    fbe_block_count_t          num_encrypted;

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_ptr->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that DEKs are sent correctly via the topology  ===\n");

    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));
    fbe_test_sep_util_generate_encryption_key(rg_object_id, &encryption_info);

    for(index = 0; index < rg_config_ptr->width; index++)
    {
        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that DEKs are sent correctly via the topology for index %d  ===\n", index);

        // get id of a PVD object associated with this RAID group
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_ptr->rg_disk_set[index].bus,
                                                                 rg_config_ptr->rg_disk_set[index].enclosure,
                                                                 rg_config_ptr->rg_disk_set[index].slot,
                                                                 &pvd_id);
        // insure that there were no errors
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_api_base_config_get_encryption_mode(pvd_id, &encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");

            MUT_ASSERT_INT_EQUAL_MSG(FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED, encryption_mode, "Unexpected Encryption Mode value\n");

        mp_key_handle.index = 0; /* we have only one RG per drive and so only one edge above */
        status = fbe_api_provision_drive_get_miniport_key_handle(pvd_id, &mp_key_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_terminator_get_port_handle(rg_config_ptr->rg_disk_set[index].bus,
                                                    &port_handle);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_api_terminator_get_encryption_keys(port_handle, mp_key_handle.mp_handle_1, terminator_key);
        key_match = fbe_equal_memory(encryption_info.key_setup.key_table_entry[0].encryption_keys[index].key1, terminator_key, FBE_ENCRYPTION_KEY_SIZE);

        MUT_ASSERT_TRUE_MSG((key_match == 1), "Key values does NOT match\n");

        /* Validate that the Paged MD region on the drive is really encrypted */
        status = fbe_api_provision_drive_get_info(pvd_id, &provision_drive_info);
        // insure that there were no errors
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_api_encryption_check_block_encrypted(rg_config_ptr->rg_disk_set[index].bus,
                                                          rg_config_ptr->rg_disk_set[index].enclosure,
                                                          rg_config_ptr->rg_disk_set[index].slot,
                                                          provision_drive_info.paged_metadata_lba,
                                                          10,
                                                          &num_encrypted);
        // insure that there were no errors
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        MUT_ASSERT_INT_EQUAL_MSG( (fbe_u32_t)num_encrypted, 10, "Unexpected encryption state\n" );
    }

    return;

}
/**************************************
 * end fbe_test_sep_util_validate_key_setup
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_validate_encryption_mode_setup()
 ****************************************************************
 * @brief
 *  Make sure the mode of encryption is properly setup.
 *    
 * @param rg_config_ptr - RG to validate for.
 * @param expected_encryption_mode - The mode of encryption that
 *                                   should be setup.
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

void fbe_test_sep_util_validate_encryption_mode_setup(fbe_test_rg_configuration_t *rg_config_ptr, 
                                                      fbe_base_config_encryption_mode_t   expected_encryption_mode)
{
    fbe_u32_t           index = 0;
    fbe_object_id_t     rg_object_id;              
    fbe_status_t        status;
    fbe_object_id_t     pvd_id;
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_sim_transport_connection_target_t sp_target[2]= {FBE_SIM_SP_A, FBE_SIM_SP_B};
    fbe_u32_t sp_id;
    fbe_object_id_t     vd_object_id;
	fbe_time_t			timeout = 0;
	fbe_base_config_encryption_state_t encryption_state;

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_ptr->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    for(sp_id = 0; sp_id < 1; sp_id++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Validate encryption mode setup for SP:%d  rg 0x%x ===", sp_id, rg_object_id);


        status = fbe_api_base_config_get_encryption_mode(rg_object_id, &encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting RG encryption mode failed\n");
		while((expected_encryption_mode != encryption_mode) && (timeout < 60 * 1000)){ /* 1 min timeout */
			fbe_api_base_config_get_encryption_state(rg_object_id,  &encryption_state);

			mut_printf(MUT_LOG_TEST_STATUS, "rg 0x%x Encryption expected mode %d mode %d state %d",rg_object_id, 
																										expected_encryption_mode, 
																										encryption_mode,
																										encryption_state);

			fbe_api_sleep(1000); /* Wait a sec. and try again */
			timeout += 1000;
	        status = fbe_api_base_config_get_encryption_mode(rg_object_id, &encryption_mode);
			MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting RG encryption mode failed");
		}


        MUT_ASSERT_INT_EQUAL_MSG(expected_encryption_mode, encryption_mode, "Unexpected RG Encryption Mode value");

        for(index = 0; index < rg_config_ptr->width; index++)
        {
            fbe_api_sim_transport_set_target_server(sp_target[sp_id]);
    
             // get id of a PVD object associated with this RAID group
            status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_ptr->rg_disk_set[index].bus,
                                                                     rg_config_ptr->rg_disk_set[index].enclosure,
                                                                     rg_config_ptr->rg_disk_set[index].slot,
                                                                     &pvd_id);
            // insure that there were no errors
            MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
            status = fbe_api_base_config_get_encryption_mode(pvd_id, &encryption_mode);
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting PVD encryption mode failed");
    
            MUT_ASSERT_INT_EQUAL_MSG(expected_encryption_mode, encryption_mode, "Unexpected PVD Encryption Mode value");

             status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, index, &vd_object_id);
             MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
            status = fbe_api_base_config_get_encryption_mode(vd_object_id, &encryption_mode);
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting VD encryption mode failed");
    
            MUT_ASSERT_INT_EQUAL_MSG(expected_encryption_mode, encryption_mode, "Unexpected VD Encryption Mode value");
        }
    }
    return;

}
/******************************************
 * end fbe_test_sep_util_validate_encryption_mode_setup()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_start_rg_encryption()
 ****************************************************************
 * @brief
 *  For a raid group that is already created, push in the keys.
 *
 * @param rg_config_p - Current RG.               
 *
 * @return None.  
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_start_rg_encryption(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_test_sep_util_set_rg_keys(current_rg_config_p, 
                                      0, fbe_test_encryption_get_generation(), 
                                      1, fbe_test_encryption_get_next_generation(),
                                      FBE_TRUE /* Yes start */);
        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_sep_util_start_rg_encryption()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_complete_rg_encryption()
 ****************************************************************
 * @brief
 *  Finish the encryption of the raid group.
 *
 * @param rg_config_p - Current RG.               
 *
 * @return None.  
 *
 * @author
 *  11/1/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_complete_rg_encryption(fbe_test_rg_configuration_t * const rg_config_p) {
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t rg_object_id;
    fbe_bool_t b_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* Make the first key the same as the second key.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Encryption set both keys to the same value.");
        fbe_test_sep_util_set_rg_keys(current_rg_config_p, 0, fbe_test_encryption_get_generation(), 1, fbe_test_encryption_get_generation(),
                                      FBE_FALSE /* Not starting */);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#if 0
        if (b_dualsp) {
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_sep_wait_all_levels_rg_encryption_state(current_rg_config_p, 
                                                                      FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE, 
                                                                      FBE_TEST_WAIT_TIMEOUT_MS);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
        status = fbe_test_sep_wait_all_levels_rg_encryption_state(current_rg_config_p, 
                                                                  FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE, 
                                                                  FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif
        /* Only done on one SP since it it clustered by database.*/
		/* This should be done by scheduler encryption plugin 
			status = fbe_api_change_rg_encryption_mode(rg_object_id, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		*/


        if (b_dualsp) {
            fbe_api_sim_transport_set_target_server(other_sp);
            fbe_test_sep_util_validate_encryption_mode_setup(current_rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
        fbe_test_sep_util_validate_encryption_mode_setup(current_rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED);

        fbe_api_sim_transport_set_target_server(this_sp);
        /* Wait for the raid group to update np and complete encryption.
         */
        if (b_dualsp) {
            fbe_api_sim_transport_set_target_server(other_sp);
            fbe_test_sep_util_wait_top_rg_encryption_complete(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
        fbe_test_sep_util_wait_top_rg_encryption_complete(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* The final step is removing the second key.  Set the second key to invalid and leave the first key valid.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Encryption remove second key.");
        fbe_test_sep_util_set_rg_keys(current_rg_config_p, 1, FBE_U32_MAX, 
                                      0, fbe_test_encryption_get_generation(),
                                      FBE_FALSE /* Not starting */);

        //status = fbe_api_raid_group_set_encryption_state(rg_object_id, &set_enc);
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }
}
/******************************************
 * end fbe_test_sep_util_complete_rg_encryption()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_wait_rg_encryption_state()
 ****************************************************************
 * @brief
 *  Wait for a raid group encryption rekey to complete.
 *
 * @param rg_object_id - Object id to wait on
 * @param expected state - state to wait for.
 * @param wait_ms - max ms to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_wait_rg_encryption_state(fbe_object_id_t rg_object_id,
                                                        fbe_base_config_encryption_state_t expected_state,
                                                        fbe_u32_t wait_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_ms;

    /* We arbitrarily decide to wait 2 seconds in between displaying.
     */
    #define REKEY_WAIT_MSEC 2000

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec) {
        status = fbe_api_base_config_get_encryption_state(rg_object_id, &encryption_state);
        if (status != FBE_STATUS_OK){
            return status;
        }
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK){
            return status;
        }
        if (encryption_state == expected_state){
            /* Only display complete if we waited below.
             */
            if (elapsed_msec > 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "=== %s rekey complete rg: 0x%x", 
                           __FUNCTION__, rg_object_id);
            }
            return status;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % REKEY_WAIT_MSEC) == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s waiting for rekey rg: 0x%x expected_state: %d  state: %d chkpt: 0x%llx", 
                       __FUNCTION__, rg_object_id, expected_state, encryption_state, (unsigned long long)rg_info.rekey_checkpoint);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= wait_ms) {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_rg_encryption_state()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_all_rg_encryption_state()
 ****************************************************************
 * @brief
 *  Wait for encryption to finish on all raid groups.
 *
 * @param rg_config_p - Current RG.
 * @param expected_state - State we will wait for
 * @param wait_ms - Milliseconds to wait max before timeout.
 * @param b_check_top_level - TRUE to check the top level RG,
 *                            only checked for RAID-10.
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_wait_all_rg_encryption_state(fbe_test_rg_configuration_t * const rg_config_p,
                                                            fbe_base_config_encryption_state_t expected_state,
                                                            fbe_u32_t wait_ms,
                                                            fbe_bool_t b_check_top_level)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t rg_object_id;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* For each mirrored pair wait for the state to change..
             */
            for (index = 0; 
                index < downstream_object_list.number_of_downstream_objects; 
                index++) {

                status = fbe_test_sep_util_wait_rg_encryption_state(downstream_object_list.downstream_object_list[index], 
                                                                    expected_state, wait_ms);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }    /* end for each mirrored pair. */
            if (b_check_top_level) {
                status = fbe_test_sep_util_wait_rg_encryption_state(rg_object_id, expected_state, wait_ms);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        } else {
            status = fbe_test_sep_util_wait_rg_encryption_state(rg_object_id, expected_state, wait_ms);

	        if ((FBE_STATUS_OK != status) && fbe_test_sep_util_get_cmd_option("-panic_sp")) {
				fbe_api_panic_sp();
			}

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_all_rg_encryption_state()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_wait_all_levels_rg_encryption_state()
 ****************************************************************
 * @brief
 *  Wait for encryption to finish on all raid groups.
 *
 * @param rg_config_p - Current RG.
 * @param wait_ms - Milliseconds to wait max before timeout.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_wait_all_levels_rg_encryption_state(fbe_test_rg_configuration_t * const rg_config_p,
                                                              fbe_base_config_encryption_state_t expected_state,
                                                              fbe_u32_t wait_ms)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_object_id_t rg_object_id;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair wait for the state to change..
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++) {

            status = fbe_test_sep_util_wait_rg_encryption_state(downstream_object_list.downstream_object_list[index], 
                                                                expected_state, wait_ms);
            if (status != FBE_STATUS_OK) {
                return status; 
            }
        }    /* end for each mirrored pair. */
    } else {
        status = fbe_test_sep_util_wait_rg_encryption_state(rg_object_id, expected_state, wait_ms);
        if (status != FBE_STATUS_OK) {
            return status; 
        }
    }
    return status;
}
/******************************************
 * end fbe_test_sep_wait_all_levels_rg_encryption_state()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_wait_rg_encryption_complete()
 ****************************************************************
 * @brief
 *  Wait for a raid group encryption rekey to complete.
 *
 * @param rg_object_id - Object id to wait on
 * @param expected state - state to wait for.
 * @param wait_ms - max ms to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  11/11/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_wait_rg_encryption_complete(fbe_object_id_t rg_object_id,
                                                        fbe_u32_t wait_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_ms;

    /* We arbitrarily decide to wait 2 seconds in between displaying.
     */
    #define REKEY_WAIT_MSEC 2000

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec) {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK){
            return status;
        }
        if (rg_info.rekey_checkpoint == FBE_LBA_INVALID){
            /* Only display complete if we waited below.
             */
            if (elapsed_msec > 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "=== %s rekey complete rg: 0x%x", __FUNCTION__, rg_object_id);
            }
            return status;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % REKEY_WAIT_MSEC) == 0) {
            status = fbe_api_base_config_get_encryption_state(rg_object_id, &encryption_state);
            if (status != FBE_STATUS_OK){
                return status;
            }
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s waiting for rekey completion rg: 0x%x state: 0x%x, chkpt: 0x%llx", 
                       __FUNCTION__, rg_object_id, encryption_state, 
                       (unsigned long long)rg_info.rekey_checkpoint);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= wait_ms) {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_rg_encryption_complete()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_top_rg_encryption_complete()
 ****************************************************************
 * @brief
 *  Wait for a raid group encryption rekey to complete.
 *
 * @param rg_object_id - Object id to wait on
 * @param expected state - state to wait for.
 * @param wait_ms - max ms to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  11/15/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_wait_top_rg_encryption_complete(fbe_object_id_t rg_object_id,
                                                               fbe_u32_t wait_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (rg_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair wait for the state to change..
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++) {
            status = fbe_test_sep_util_wait_rg_encryption_complete(downstream_object_list.downstream_object_list[index], 
                                                                   wait_ms);
            if (status != FBE_STATUS_OK) {
                return status; 
            }
        }    /* end for each mirrored pair. */
    }
    status = fbe_test_sep_util_wait_rg_encryption_complete(rg_object_id, wait_ms);
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_top_rg_encryption_complete()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_wait_all_rg_encryption_complete()
 ****************************************************************
 * @brief
 *  Wait for encryption to finish on all raid groups.
 *
 * @param rg_config_p - Current RG.
 * @param wait_ms - Milliseconds to wait max before timeout.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/11/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_util_wait_all_rg_encryption_complete(fbe_test_rg_configuration_t * const rg_config_p,
                                                               fbe_u32_t wait_ms)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t rg_object_id;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            break;
        }
        
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_wait_top_rg_encryption_complete(rg_object_id, wait_ms);
        if (status != FBE_STATUS_OK) { 
            return status; 
        }

        current_rg_config_p++;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_all_rg_encryption_complete()
 ******************************************/
void fbe_test_encryption_display_key(fbe_u8_t *key, fbe_u32_t length)
{
    fbe_u32_t index;
    fbe_char_t key_string[FBE_ENCRYPTION_KEY_SIZE * 2 + 1];
    fbe_char_t partial_string[5];
    key_string[0] = 0;
    for(index = 0; index < length; index++) {
        _snprintf(partial_string, 5, "%02x", key[index]);
        csx_p_strncat(key_string, partial_string, FBE_ENCRYPTION_KEY_SIZE * 2 + 1);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "          %s", key_string);
}
void fbe_test_encryption_display_keys(fbe_encryption_setup_encryption_keys_t *keys_p)
{
    fbe_u32_t index;
    mut_printf(MUT_LOG_TEST_STATUS, "**********************************************************************");
    mut_printf(MUT_LOG_TEST_STATUS, "object      :   %d (0x%x)", 
               keys_p->object_id, keys_p->object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "generation  :   0x%llx", keys_p->generation_number);
    mut_printf(MUT_LOG_TEST_STATUS, "num_of_keys :   %d", keys_p->num_of_keys);
    for(index = 0; index < keys_p->num_of_keys; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "%2d) key1: %s", index, &keys_p->encryption_keys[index].key1[0]);
        fbe_test_encryption_display_key( &keys_p->encryption_keys[index].key1[0], 
                                         FBE_ENCRYPTION_KEY_SIZE);
        mut_printf(MUT_LOG_TEST_STATUS, "    key2: %s", &keys_p->encryption_keys[index].key2[0]);
        fbe_test_encryption_display_key( &keys_p->encryption_keys[index].key2[0], 
                                         FBE_ENCRYPTION_KEY_SIZE);
        mut_printf(MUT_LOG_TEST_STATUS, "    mask: 0x%llx", keys_p->encryption_keys[index].key_mask);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "**********************************************************************");
}
fbe_status_t fbe_test_sep_util_set_rg_keys(fbe_test_rg_configuration_t * rg_config_p,
                                           fbe_u32_t current_index,
                                           fbe_u32_t current_generation,
                                           fbe_u32_t next_index,
                                           fbe_u32_t next_generation,
                                           fbe_bool_t b_start)
{
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;              
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_status_t        status;
    fbe_u32_t timeout = 0;
    fbe_sim_transport_connection_target_t this_sp, other_sp;

    while(timeout < 1800)
    {
        /* First get the Object ID for the Raid Group */
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
     //   MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if(rg_object_id != FBE_OBJECT_ID_INVALID)
        {
            break;
        }
        fbe_api_sleep(100);
        timeout += 100;
    }

    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));

    status = fbe_test_sep_util_generate_encryption_key(rg_object_id, &encryption_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (current_generation == FBE_U32_MAX) {
        /* Encryption start or end.  Keep key 0 invalid, add key 1 as valid.
         */
        fbe_test_sep_util_invalidate_key(rg_object_id, &encryption_info, current_index, 
                                         FBE_ENCRYPTION_KEY_MASK_UPDATE);
        if (b_start) {
            /* When we start we are adding a key and it needs to get updated.
             */
            fbe_test_sep_util_generate_next_key(rg_object_id, &encryption_info, next_index, next_generation, 
                                                (FBE_ENCRYPTION_KEY_MASK_UPDATE | FBE_ENCRYPTION_KEY_MASK_VALID));
        } else {
            /* When we end we are removing a key and the other key remains valid.
             */
            fbe_test_sep_util_generate_next_key(rg_object_id, &encryption_info, next_index, next_generation, 
                                                FBE_ENCRYPTION_KEY_MASK_VALID);
        }
    } else if (b_start) {
        /* Normal rekey start case.  Both keys are valid, but we only need to update the second one.
         */
        fbe_test_sep_util_generate_next_key(rg_object_id, &encryption_info, current_index, current_generation, 
                                            FBE_ENCRYPTION_KEY_MASK_VALID);
        fbe_test_sep_util_generate_next_key(rg_object_id, &encryption_info, next_index, next_generation, 
                                            (FBE_ENCRYPTION_KEY_MASK_UPDATE | FBE_ENCRYPTION_KEY_MASK_VALID));
    } else {
        /* Normal rekey/encryption finish case.  Both keys are valid, but we only need to update the first one.
         */
        fbe_test_sep_util_generate_next_key(rg_object_id, &encryption_info, current_index, current_generation, 
                                             (FBE_ENCRYPTION_KEY_MASK_UPDATE | FBE_ENCRYPTION_KEY_MASK_VALID));
        fbe_test_sep_util_generate_next_key(rg_object_id, &encryption_info, next_index, next_generation, 
                                            FBE_ENCRYPTION_KEY_MASK_VALID);
    }
    //fbe_test_encryption_display_keys(&encryption_info.key_setup.key_table_entry[0]);

    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair set the keys.  */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++) {
            encryption_info.key_setup.key_table_entry[0].num_of_keys = 2;
            encryption_info.key_setup.key_table_entry[0].object_id = downstream_object_list.downstream_object_list[index];

            status = fbe_api_database_setup_encryption_rekey(&encryption_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    } else {
        /* Push it down */
		mut_printf(MUT_LOG_TEST_STATUS,"fbe_api_database_setup_encryption_rekey obj 0x%x SP: %s\n", 
                   rg_object_id, (this_sp == FBE_SIM_SP_A) ? "SPA" : "SPB");
        status = fbe_api_database_setup_encryption_rekey(&encryption_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return FBE_STATUS_OK;
}
/*************************
 * end file fbe_test_sep_encryption_utils.c
 *************************/


/*!**************************************************************
 * fbe_test_sep_wait_all_levels_rg_encryption_state_passed()
 ****************************************************************
 * @brief
 *  Wait for encryption to finish on all raid groups.
 *
 * @param rg_config_p - Current RG.
 * @param wait_ms - Milliseconds to wait max before timeout.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/17/2013 - Created. Peter Puhov
 *
 ****************************************************************/

fbe_status_t fbe_test_sep_wait_all_levels_rg_encryption_state_passed(fbe_test_rg_configuration_t * const rg_config_p,
                                                              fbe_base_config_encryption_state_t expected_state,
                                                              fbe_u32_t wait_ms)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_object_id_t rg_object_id;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair wait for the state to change..
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++) {

            status = fbe_test_sep_util_wait_rg_encryption_state_passed(downstream_object_list.downstream_object_list[index], 
                                                                expected_state, wait_ms);
            if (status != FBE_STATUS_OK) {
                return status; 
            }
        }    /* end for each mirrored pair. */
    } else {
        status = fbe_test_sep_util_wait_rg_encryption_state_passed(rg_object_id, expected_state, wait_ms);
        if (status != FBE_STATUS_OK) {
            return status; 
        }
    }
    return status;
}
/******************************************
 * end fbe_test_sep_wait_all_levels_rg_encryption_state_passed()
 ******************************************/


/*!**************************************************************
 * fbe_test_sep_util_wait_rg_encryption_state()
 ****************************************************************
 * @brief
 *  Wait for a raid group encryption rekey to complete.
 *
 * @param rg_object_id - Object id to wait on
 * @param expected state - state to wait for.
 * @param wait_ms - max ms to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  12/17/2013 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_util_wait_rg_encryption_state_passed(fbe_object_id_t rg_object_id,
                                                        fbe_base_config_encryption_state_t expected_state,
                                                        fbe_u32_t wait_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_ms;

    /* We arbitrarily decide to wait 2 seconds in between displaying.
     */
    #define REKEY_WAIT_MSEC 2000

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec) {
        status = fbe_api_base_config_get_encryption_state(rg_object_id, &encryption_state);
        if (status != FBE_STATUS_OK){
            return status;
        }
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK){
            return status;
        }
        if (encryption_state >= expected_state){
            /* Only display complete if we waited below.
             */
            if (elapsed_msec > 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "=== %s PASSED rg: 0x%x", __FUNCTION__, rg_object_id);
            }
            return status;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % REKEY_WAIT_MSEC) == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s waiting for rekey rg: 0x%x expected_state: %d  state: %d chkpt: 0x%llx", 
                       __FUNCTION__, rg_object_id, expected_state, encryption_state, (unsigned long long)rg_info.rekey_checkpoint);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= wait_ms) {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end fbe_test_sep_util_wait_rg_encryption_state()
 ******************************************/


void fbe_test_sep_use_pause_rekey_hooks(fbe_test_rg_configuration_t * const rg_config_p,
                                        fbe_lba_t checkpoint,
                                        fbe_test_hook_action_t hook_action) 
{   
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;
    fbe_u32_t width = current_rg_config_p->width;

    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        width = current_rg_config_p->width;

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            /* We will add hooks at mirror level so actual width is 2.
             */
            width = 2;
        }

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* All ds objects */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       checkpoint, 0, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       hook_action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

}

void fbe_test_sep_use_chkpt_hooks(fbe_test_rg_configuration_t * const rg_config_p,
                                  fbe_lba_t checkpoint,
                                  fbe_u32_t state, 
                                  fbe_u32_t substate,
                                  fbe_test_hook_action_t hook_action) 
{   
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;
    fbe_u32_t width = current_rg_config_p->width;

    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        width = current_rg_config_p->width;

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            /* We will add hooks at mirror level so actual width is 2.
             */
            width = 2;
        }

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* All ds objects */
                                       state, substate, checkpoint, 0, SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       hook_action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

}

/*!**************************************************************
 * fbe_test_sep_encryption_completing_hooks()
 ****************************************************************
 * @brief
 *  Add encryption completing hook for a RG.
 *
 * @param rg_config_ptr - RG to validate for.               
 *
 * @return None.   
 *
 * @author
 *  04-Feb-2014 - Created. 
 *
 ****************************************************************/
void fbe_test_sep_encryption_completing_hooks(fbe_test_rg_configuration_t * const rg_config_p, fbe_test_hook_action_t hook_action)
{   
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;
    fbe_u32_t width = current_rg_config_p->width;

    for ( rg_index = 0; rg_index < rg_count; rg_index++) 
    {
        width = current_rg_config_p->width;

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) 
        {
            // We will add hooks at mirror level so actual width is 2.
            width = 2;
        }

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) 
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       hook_action);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    return;

} /* end of fbe_test_sep_encryption_completing_hooks() */

/*!**************************************************************
 * fbe_test_sep_encryption_add_hook_and_enable_kms()
 ****************************************************************
 * @brief
 *  Add hook and enable kms for a RG.
 *
 * @param rg_config_ptr - RG to validate for.               
 *
 * @return None.   
 *
 * @author
 *  04-Feb-2014 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_encryption_add_hook_and_enable_kms(fbe_test_rg_configuration_t * const rg_config_p, fbe_bool_t b_dualsp) 
{
    fbe_status_t status = FBE_STATUS_OK;

    if (b_dualsp)
    {
        /* Start loading SP A. */
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        // Add hook to wait for encryption to complete
        mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption complete ==");
        fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

        /* Start loading SP B. */
        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        // Add hook to wait for encryption to complete
        mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption complete ==");
        fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

        status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        // Add hook to wait for encryption to complete
        mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption complete ==");
        fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    }

    status = fbe_test_sep_util_enable_kms_encryption();

    return status;
} /* end of fbe_test_sep_encryption_add_hook_and_enable_kms() */


/*!**************************************************************
 * fbe_test_rg_validate_lbas_encrypted()
 ****************************************************************
 * @brief
 *  Make sure the blocks of the raid group are properly encrypted.
 * 
 * @param rg_config_p
 * @param lba
 * @param blocks
 * @param blocks_encrypted
 *
 * @return None.   
 *
 * @author
 *  2/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_rg_validate_lbas_encrypted(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_lba_t lba,
                                         fbe_block_count_t blocks,
                                         fbe_block_count_t blocks_encrypted)
{
    fbe_u32_t           index = 0;
    fbe_object_id_t     rg_object_id; 
    fbe_status_t        status;
    fbe_object_id_t     pvd_id;
    fbe_api_provision_drive_info_t        provision_drive_info;
    fbe_block_count_t          num_encrypted;
    fbe_raid_group_map_info_t map_info;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;
    fbe_object_id_t lun_object_id;

    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
    
        /* Verify the object id of the raid group */
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        for(index = 0; index < rg_config_p->width; index++)
        {
            map_info.lba = lba;
            status = fbe_api_lun_map_lba(lun_object_id, &map_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "=== lba 0x%llx on LUN object 0x%x is at pba 0x%llx at disk %d  ===\n",
                       lba, lun_object_id, map_info.pba, map_info.data_pos);
    
            status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[map_info.data_pos].bus,
                                                                     rg_config_p->rg_disk_set[map_info.data_pos].enclosure,
                                                                     rg_config_p->rg_disk_set[map_info.data_pos].slot,
                                                                     &pvd_id);
            MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    
            /* Validate that the Paged MD region on the drive is really encrypted */
            status = fbe_api_provision_drive_get_info(pvd_id, &provision_drive_info);
            // insure that there were no errors
            MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
            status = fbe_api_encryption_check_block_encrypted(rg_config_p->rg_disk_set[map_info.data_pos].bus,
                                                              rg_config_p->rg_disk_set[map_info.data_pos].enclosure,
                                                              rg_config_p->rg_disk_set[map_info.data_pos].slot,
                                                              map_info.pba,
                                                              blocks,
                                                              &num_encrypted);
            // insure that there were no errors
            MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

            mut_printf(MUT_LOG_TEST_STATUS, "num_encrypted 0x%llx blocks 0x%llx", num_encrypted, blocks_encrypted);
            if (num_encrypted != blocks) {
                MUT_FAIL_MSG("num_encrypted != blocks");
            }
        }
    }

    return;

}
/**************************************
 * end fbe_test_rg_validate_lbas_encrypted
 **************************************/

/*!**************************************************************
 * fbe_test_rg_validate_pvd_paged_encrypted()
 ****************************************************************
 * @brief
 *  Make sure the blocks of the raid group are properly encrypted.
 *
 * @param rg_config_p - RG to validate for.
 *
 * @return None.   
 *
 * @author
 *  2/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_rg_validate_pvd_paged_encrypted(fbe_test_rg_configuration_t *rg_config_p,   
                                              fbe_block_count_t blocks)
{
    fbe_u32_t           index = 0;
    fbe_object_id_t     rg_object_id; 
    fbe_status_t        status;
    fbe_object_id_t     pvd_id;
    fbe_api_provision_drive_info_t        provision_drive_info;
    fbe_block_count_t          num_encrypted;

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    for(index = 0; index < rg_config_p->width; index++)
    {

        mut_printf(MUT_LOG_TEST_STATUS, "=== Validate that DEKs are sent correctly via the topology for index %d  ===\n", index);

        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[index].bus,
                                                                 rg_config_p->rg_disk_set[index].enclosure,
                                                                 rg_config_p->rg_disk_set[index].slot,
                                                                 &pvd_id);
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        /* Validate that the Paged MD region on the drive is really encrypted */
        status = fbe_api_provision_drive_get_info(pvd_id, &provision_drive_info);
        // insure that there were no errors
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_api_encryption_check_block_encrypted(rg_config_p->rg_disk_set[index].bus,
                                                          rg_config_p->rg_disk_set[index].enclosure,
                                                          rg_config_p->rg_disk_set[index].slot,
                                                          provision_drive_info.paged_metadata_lba,
                                                          10,
                                                          &num_encrypted);
        // insure that there were no errors
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        mut_printf(MUT_LOG_TEST_STATUS, "num_encrypted 0x%llx blocks 0x%llx", num_encrypted, blocks);
        if (num_encrypted != blocks) {
            MUT_FAIL_MSG("num_encrypted != blocks");
        }
    }

    return;

}
/**************************************
 * end fbe_test_rg_validate_pvd_paged_encrypted
 **************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_object_encryption_mode()
 ****************************************************************
 * @brief
 *  Wait for this RG to get to this encryption mode.
 *    
 * @param rg_config_ptr - RG to validate for.
 * @param expected_encryption_mode - The mode of encryption that
 *                                   should be setup.
 *
 * @return None.
 *
 * @author
 *  2/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_wait_object_encryption_mode(fbe_object_id_t     rg_object_id,
                                                   fbe_base_config_encryption_mode_t   expected_encryption_mode,
                                                   fbe_u32_t max_timeout_ms)
{
    fbe_status_t        status;
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_base_config_encryption_state_t   encryption_state;
    fbe_time_t          timeout = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait encryption mode %d for rg 0x%x ===", 
               expected_encryption_mode, rg_object_id);

    status = fbe_api_base_config_get_encryption_mode(rg_object_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting RG encryption mode failed\n");
    while ((expected_encryption_mode != encryption_mode) && (timeout < max_timeout_ms)) {    /* 1 min timeout */
        fbe_api_base_config_get_encryption_state(rg_object_id,  &encryption_state);

        mut_printf(MUT_LOG_TEST_STATUS, "rg 0x%x Encryption expected mode %d mode %d state %d",rg_object_id, 
                   expected_encryption_mode, 
                   encryption_mode,
                   encryption_state);

        fbe_api_sleep(1000);    /* Wait a sec. and try again */
        timeout += 1000;
        status = fbe_api_base_config_get_encryption_mode(rg_object_id, &encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting RG encryption mode failed");
    }

    MUT_ASSERT_INT_EQUAL_MSG(expected_encryption_mode, encryption_mode, "Unexpected RG Encryption Mode value");
}
/******************************************
 * end fbe_test_sep_util_wait_object_encryption_mode()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_object_encryption_state()
 ****************************************************************
 * @brief
 *  Wait for this RG to get to this encryption mode.
 *    
 * @param rg_config_ptr - RG to validate for.
 * @param expected_encryption_state - The state of encryption that
 *                                   should be setup.
 *
 * @return None.
 *
 * @author
 *  2/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_wait_object_encryption_state(fbe_object_id_t     rg_object_id,
                                                    fbe_base_config_encryption_state_t   expected_encryption_state,
                                                    fbe_u32_t max_timeout_ms)
{
    fbe_status_t        status;
    fbe_base_config_encryption_mode_t   encryption_mode;
    fbe_base_config_encryption_state_t   encryption_state;
    fbe_time_t          timeout = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait encryption state %d for rg 0x%x ===", 
               expected_encryption_state, rg_object_id);

    status = fbe_api_base_config_get_encryption_state(rg_object_id, &encryption_state);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting RG encryption state failed\n");
    while ((expected_encryption_state != encryption_state) && (timeout < max_timeout_ms)) {    /* 1 min timeout */
        fbe_api_base_config_get_encryption_state(rg_object_id,  &encryption_state);
        fbe_api_base_config_get_encryption_mode(rg_object_id,  &encryption_mode);

        mut_printf(MUT_LOG_TEST_STATUS, "rg 0x%x Encryption expected state %d state %d mode %d",rg_object_id, 
                   expected_encryption_state,
                   encryption_state, 
                   encryption_mode);

        fbe_api_sleep(1000);    /* Wait a sec. and try again */
        timeout += 1000;
        status = fbe_api_base_config_get_encryption_state(rg_object_id, &encryption_state);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting RG encryption state failed");
    }

    MUT_ASSERT_INT_EQUAL_MSG(expected_encryption_state, encryption_state, "Unexpected RG Encryption state value");
}
/******************************************
 * end fbe_test_sep_util_wait_object_encryption_state()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_wait_for_scrubbing_complete()
 ****************************************************************
 * @brief
 *  Wait for scrubbing to finish on a PVD.
 *
 * @param object_id - current pvd.
 * @param max_timeout_ms - time to wait max.
 *
 * @return None   
 *
 * @author
 *  7/9/2014 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_wait_for_scrubbing_complete(fbe_object_id_t object_id,
                                                   fbe_u32_t max_timeout_ms)
{
    fbe_status_t                        status;
    fbe_time_t                          timeout = 0;
	fbe_api_provision_drive_info_t      pvd_info;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Wait scrub complete 0x%x ===", object_id);

    status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    while ((pvd_info.scrubbing_in_progress) && (timeout < max_timeout_ms)) {    /* 1 min timeout */

        mut_printf(MUT_LOG_TEST_STATUS, "scrub still in progress on pvd object: 0x%x", object_id);

        fbe_api_sleep(1000);    /* Wait a sec. and try again */
        timeout += 1000;
        status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    if (pvd_info.scrubbing_in_progress) {
        MUT_FAIL_MSG("wait for scrubbing to finish timed out\n");
    }
    return;
}
/******************************************
 * end fbe_test_sep_util_wait_for_scrubbing_complete()
 ******************************************/
