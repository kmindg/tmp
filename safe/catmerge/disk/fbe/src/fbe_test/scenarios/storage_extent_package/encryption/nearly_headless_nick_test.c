/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file nearly_headless_nick_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test for pausing background encryption on demand.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_hook.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_encryption_interface.h"

#define NEARLY_HEADLESS_NICK_LUNS_PER_RAID_GROUP 3
#define NEARLY_HEADLESS_NICK_CHUNKS_PER_LUN      1

fbe_test_rg_configuration_t nearly_headless_nick_raid_group_config[] =  
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,  520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};

char * nearly_headless_nick_short_desc = "Test pausing background encryption";
char * nearly_headless_nick_long_desc =
    "\n"
    "\n"
    "The Nearly Headless Nick test pauses and resumes background encryption\n"
    "Description last updated: 03/18/2014.\n"
    ;

void nearly_headless_nick_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_encryption_init();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &nearly_headless_nick_raid_group_config[0];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, 
                         NEARLY_HEADLESS_NICK_LUNS_PER_RAID_GROUP, 
                         NEARLY_HEADLESS_NICK_CHUNKS_PER_LUN);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);

        sep_config_load_kms(NULL);
    }

    /* The following utility function does some setup for hardware; it's a noop for simulation. */
    fbe_test_common_util_test_setup_init();

    return;
}

void nearly_headless_nick_dualsp_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_encryption_init();

    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t  raid_group_count = 0;
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &nearly_headless_nick_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);
        
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
        sep_config_load_kms_both_sps(NULL);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* The following utility function does some setup for hardware; it's a noop for simulation. */
    fbe_test_common_util_test_setup_init();

    return;

}

void nearly_headless_nick_run_tests_on_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t status;
    fbe_bool_t paused;
    fbe_bool_t dual_sp = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_object_id_t rg_object_ids[16];
    //fbe_scheduler_debug_hook_t hook;
    fbe_sep_package_load_params_t sep_params_p;
    fbe_object_id_t pvd_object_id;

    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    // Wait for zeroing to complete before enabling encryption
    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for zeroing to complete...");
    fbe_test_zero_wait_for_rg_zeroing_complete(rg_config_p);

    big_bird_write_zero_background_pattern();

    // Verify encryption is not paused
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying encryption is not paused on local SP...");
    status = fbe_api_database_get_encryption_paused(&paused);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_FALSE(paused);

    if(dual_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Verifying encryption is not paused on peer SP...");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_database_get_encryption_paused(&paused);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_FALSE(paused);
    }

    // Pause background encryption
    mut_printf(MUT_LOG_TEST_STATUS, "Pausing encryption...");
    status = fbe_api_job_service_set_encryption_paused(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Verify encryption is paused
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying encryption is paused on local SP...");
    status = fbe_api_database_get_encryption_paused(&paused);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(paused);

    if(dual_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Verifying encryption is paused on peer SP...");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_database_get_encryption_paused(&paused);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(paused);
    }

    fbe_api_sleep(1000);

    // Reboot SPs
    mut_printf(MUT_LOG_TEST_STATUS, "Rebooting SPs...");
    if(dual_sp)
    {
        fbe_test_sep_util_destroy_neit_sep_kms_both_sps();
        sep_config_load_sep_and_neit_both_sps();
        sep_config_load_kms_both_sps(NULL);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    else
    {
        fbe_test_common_reboot_sp_neit_sep_kms(FBE_SIM_SP_A, NULL, NULL, NULL);
    }

    // Verify encryption is still paused
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying encryption is paused after reboot on local SP...");
    status = fbe_api_database_get_encryption_paused(&paused);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(paused);

    if(dual_sp)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Verifying encryption is paused after reboot on peer SP...");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_database_get_encryption_paused(&paused);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(paused);
    }

    fbe_test_set_vault_wait_time();

    mut_printf(MUT_LOG_TEST_STATUS, "Adding debug hooks...");

    status = fbe_test_add_debug_hook_active(rg_object_ids[0],
                            SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                            FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAUSED,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_CONTINUE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_add_debug_hook_active(rg_object_ids[0],
                            SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                            FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPAUSED,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_CONTINUE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Enabling Encryption...");
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for PAUSED hook...");
    status = fbe_test_wait_for_debug_hook_active(rg_object_ids[0],
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                 FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAUSED,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_CONTINUE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_add_debug_hook_active(rg_object_ids[0],
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                            FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                            NULL,
                                            NULL,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Resume background encryption
    mut_printf(MUT_LOG_TEST_STATUS, "Resuming encryption...");
    status = fbe_api_job_service_set_encryption_paused(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for UNPAUSED hook...");
    status = fbe_test_wait_for_debug_hook_active(rg_object_ids[0],
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                 FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPAUSED,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_CONTINUE,
                                                 0, 0);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the encryption to ALMOST complete */
    status = fbe_test_wait_for_debug_hook_active(rg_object_ids[0],
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                 FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if(!dual_sp)
    {
         /* Get the pvd object id */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                                rg_config_p->rg_disk_set[0].enclosure,
                                                                rg_config_p->rg_disk_set[0].slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

        /* Initialize the scheduler hook */
        sep_params_p.scheduler_hooks[0].object_id = pvd_object_id;
        sep_params_p.scheduler_hooks[0].monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION;
        sep_params_p.scheduler_hooks[0].monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_REGISTER_KEYS;
        sep_params_p.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
        sep_params_p.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_CONTINUE;
        sep_params_p.scheduler_hooks[0].val1 = NULL;
        sep_params_p.scheduler_hooks[0].val2 = NULL;

        /* and this will be our signal to stop */
        sep_params_p.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;

        mut_printf(MUT_LOG_TEST_STATUS, "Rebooting SP to validate that keys are registered just once ...");
        /* Reboot the SP */
        fbe_test_common_reboot_this_sp(&sep_params_p, NULL);
        /* Get the counter for number of times the register key was sent. Make sure we register only once */
        status = fbe_test_wait_for_hook_counter(pvd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                                 FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_REGISTER_KEYS,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE,
                                                 0, 0, 1);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unexpected hook counter value\n");

    } else {
        status = fbe_test_del_debug_hook_active(rg_object_ids[0],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                                NULL,
                                                NULL,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    }
}

void nearly_headless_nick_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    rg_config_p = &nearly_headless_nick_raid_group_config[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p,
                                   NULL,
                                   nearly_headless_nick_run_tests_on_rg_config,
                                   NEARLY_HEADLESS_NICK_LUNS_PER_RAID_GROUP,
                                   NEARLY_HEADLESS_NICK_CHUNKS_PER_LUN);
}

void nearly_headless_nick_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    rg_config_p = &nearly_headless_nick_raid_group_config[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    fbe_test_run_test_on_rg_config(rg_config_p,
                                   NULL,
                                   nearly_headless_nick_run_tests_on_rg_config,
                                   NEARLY_HEADLESS_NICK_LUNS_PER_RAID_GROUP,
                                   NEARLY_HEADLESS_NICK_CHUNKS_PER_LUN);
}

void nearly_headless_nick_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

void nearly_headless_nick_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms_both_sps();
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;
}
