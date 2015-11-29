/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file defiler_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test to validate the 4K handling after commit 
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe_test.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_trace_interface.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"
#include "ndu_toc_common.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_hook.h"
#include "fbe/fbe_api_cmi_interface.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char *defiler_short_desc = "This scenario will test c4mirror raid group functions";
char *defiler_long_desc ="\
This scenario will test c4mirror raid group functions\n\
\n\
    1. Test all raid groups are ready.\n\
    2. Remove a system drive and reinsert.\n\
         Test all raid groups aren't degraded.\n\
    3. Remove a drive and insert a new drive.\n\
         Test all raid group are marking NR.\n\
         Test all raid groups aren't degraded.\n\
\n\
\n";


typedef struct defiler_test_case_s {
    fbe_api_terminator_device_handle_t dh_spa;
    fbe_api_terminator_device_handle_t dh_spb;
    fbe_u32_t port, encl, slot;
    fbe_object_id_t rg_spa;
    fbe_object_id_t rg_spb;
    fbe_u32_t fault_position;
} defiler_test_case_t;


#define DEFILER_SPA_RG          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPA
#define DEFILER_SPB_RG          FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPB

#define DEFILER_TEST_DRIVE_CAPACITY ((fbe_lba_t)(1.5*TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY))
/*!*******************************************************************
 * @var defiler_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t defiler_raid_group_config[] =
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


extern void sabbath_pull_drive(fbe_u32_t port_number,
                               fbe_u32_t enclosure_number,
                               fbe_u32_t slot_number,
                               fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                               fbe_api_terminator_device_handle_t *drive_handle_p_spb);
extern void sabbath_reinsert_drive(fbe_u32_t port_number,
                                   fbe_u32_t enclosure_number,
                                   fbe_u32_t slot_number,
                                   fbe_api_terminator_device_handle_t drive_handle_p_spa,
                                   fbe_api_terminator_device_handle_t drive_handle_p_spb);
extern fbe_bool_t  sabbath_wait_for_sep_object_state(fbe_object_id_t object_id,
                                                     fbe_lifecycle_state_t  expected_lifecycle_state,
                                                     fbe_bool_t wait_spa, fbe_bool_t wait_spb,
                                                     fbe_u32_t wait_sec);
extern void sabbath_insert_blank_new_drive(fbe_u32_t port_number, 
                                           fbe_u32_t enclosure_number, 
                                           fbe_u32_t slot_number,
                                           fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                                           fbe_api_terminator_device_handle_t *drive_handle_p_spb);


/*!**************************************************************************
 *  defiler_get_activesp
 ***************************************************************************
 * @brief
 *   This function return active SP. (Reinit can only be done on active sp)
 *
 *
 * @return active SP
 *
 *   2015-04-21. Created. Jamin Kang
 *
 ***************************************************************************/
static fbe_sim_transport_connection_target_t defiler_get_activesp(void)
{
	fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;
	fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
	fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;
    fbe_u32_t i, max_retries;

    max_retries = 6000;
    for (i = 0; i < max_retries; i += 1) {
		/* We have to figure out whitch SP is ACTIVE */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		fbe_api_cmi_service_get_info(&spa_cmi_info);
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		fbe_api_cmi_service_get_info(&spb_cmi_info);

		if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_A;
			passive_connection_target = FBE_SIM_SP_B;
            break;
		}

		if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_B;
			passive_connection_target = FBE_SIM_SP_A;
            break;
        }
        fbe_api_sleep(100);
	}
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: ACTIVE side is %s",
               __FUNCTION__, active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    return active_connection_target;
}

/*!**************************************************************************
 *  defiler_verify_not_rb_logging
 ***************************************************************************
 * @brief
 *   This function verifies that rebuild logging is not set.
 *
 * @param rg_object_id            - raid group object id
 * @param in_position             - disk position in the RG
 *
 * @return void
 *
 *   03/24/2015. Created. Kang Jamin
 *
 ***************************************************************************/
static void defiler_verify_not_rb_logging(fbe_object_id_t rg_obj, fbe_u32_t in_position)
{

    fbe_status_t                        status;                         //  fbe status
    fbe_u32_t                           count;                          //  loop count
    fbe_u32_t                           max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t       raid_group_info;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify rebuild logging NOT set on position %d in the RG 0x%x\n",
               __FUNCTION__, in_position, rg_obj);

    //  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)
    max_retries = 600;

    //  Loop until the rebuild logging has been cleared
    for (count = 0; count < max_retries; count++) {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(rg_obj, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  Check if rebuild logging is clear
        if (raid_group_info.b_rb_logging[in_position] == FBE_FALSE) {
            return;
        }

        //  Wait before trying again
        fbe_api_sleep(100);
    }

    //  rebuild logging was not cleared within the time limit - assert
    MUT_ASSERT_TRUE(0);

    //  Return (should never get here)
    return;

}   // End defiler_verify_not_rb_logging()


static void defiler_pull_drive(fbe_u32_t port_number,
                               fbe_u32_t enclosure_number,
                               fbe_u32_t slot_number,
                               fbe_api_terminator_device_handle_t *drive_handle_p_spa,
                               fbe_api_terminator_device_handle_t *drive_handle_p_spb)
{
    fbe_object_id_t pvd_id;
    fbe_status_t status;
    fbe_bool_t wait_lifecycle;


    status = fbe_api_provision_drive_get_obj_id_by_location(port_number,
                                                            enclosure_number,
                                                            slot_number, &pvd_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Failed to get pvd object id");
    mut_printf(MUT_LOG_TEST_STATUS, "Start to remove pvd 0x%x, %u_%u_%u\n",
               pvd_id, port_number, enclosure_number, slot_number);
    sabbath_pull_drive(port_number, enclosure_number, slot_number,
                       drive_handle_p_spb, drive_handle_p_spb);
    wait_lifecycle = sabbath_wait_for_sep_object_state(pvd_id,
                                                       FBE_LIFECYCLE_STATE_FAIL,
                                                       FBE_TRUE, FBE_TRUE,
                                                       10000);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, wait_lifecycle, "Failed to wait pvd failed");
}

static void defiler_reinsert_drive(fbe_u32_t port_number,
                                   fbe_u32_t enclosure_number,
                                   fbe_u32_t slot_number,
                                   fbe_api_terminator_device_handle_t drive_handle_p_spa,
                                   fbe_api_terminator_device_handle_t drive_handle_p_spb)
{
    sabbath_reinsert_drive(port_number, enclosure_number, slot_number,
                           drive_handle_p_spb, drive_handle_p_spb);
}

static void defiler_shutdown_sps(void)
{
    fbe_sim_transport_connection_target_t   original_target;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Shutdown sps\n", __FUNCTION__);
    original_target = fbe_api_sim_transport_get_target_server();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(original_target);
}

static void defiler_boot_sps(fbe_sep_package_load_params_t *spa_params,
                             fbe_sep_package_load_params_t *spb_params)
{
    fbe_sim_transport_connection_target_t   original_target;

    original_target = fbe_api_sim_transport_get_target_server();

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Boot sps\n",
               __FUNCTION__);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit_params(spa_params, NULL);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit_params(spb_params, NULL);

    fbe_api_sim_transport_set_target_server(original_target);
}



static void defiler_init_test_case(defiler_test_case_t *test)
{
    memset(test, 0, sizeof(*test));
    test->port = 0;
    test->encl = 0;
    test->slot = 0;

    test->rg_spa = DEFILER_SPA_RG;
    test->rg_spb = DEFILER_SPB_RG;
    test->fault_position = 0;
}


static void defiler_set_bg_ops_on_object(fbe_object_id_t obj, fbe_bool_t is_en)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: %s background service for obj: 0x%x\n",
               __FUNCTION__, is_en? "Enable":"Disable", obj);

    status = fbe_api_wait_for_object_lifecycle_state(obj, FBE_LIFECYCLE_STATE_READY,
                                                     FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (is_en) {
        status = fbe_api_base_config_enable_background_operation(obj,
                                                                  FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    } else {
        status = fbe_api_base_config_disable_background_operation(obj,
                                                                  FBE_RAID_GROUP_BACKGROUND_OP_ALL);
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void defiler_test_set_bg_ops(defiler_test_case_t *test, fbe_bool_t is_en)
{

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    defiler_set_bg_ops_on_object(test->rg_spa, is_en);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    defiler_set_bg_ops_on_object(test->rg_spb, is_en);
}

static void defiler_test_verify_rb_logging(defiler_test_case_t *test)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify RL is set on RG 0x%x\n",
               __FUNCTION__, test->rg_spa);
    sep_rebuild_utils_verify_rb_logging_by_object_id(test->rg_spa, test->fault_position);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify RL is set on RG 0x%x\n",
               __FUNCTION__, test->rg_spb);
    sep_rebuild_utils_verify_rb_logging_by_object_id(test->rg_spb, test->fault_position);
}

static void defiler_test_verify_not_rb_logging(defiler_test_case_t *test)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    defiler_verify_not_rb_logging(test->rg_spa, test->fault_position);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    defiler_verify_not_rb_logging(test->rg_spb, test->fault_position);
}

static void defiler_test_remove_drive(defiler_test_case_t *test)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove drive\n",
               __FUNCTION__);

    defiler_pull_drive(test->port, test->encl, test->slot, &test->dh_spa, &test->dh_spb);
    defiler_test_verify_rb_logging(test);
}

static void defiler_test_wait_rb_complete(defiler_test_case_t *test)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rb complete on rg 0x%x\n",
               __FUNCTION__, test->rg_spa);
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(test->rg_spa, test->fault_position);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for rb complete on rg 0x%x\n",
               __FUNCTION__, test->rg_spa);
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(test->rg_spb, test->fault_position);
}

static void defiler_verify_nr_all_chunks(fbe_object_id_t rg_obj, fbe_u32_t position)
{
    fbe_status_t status;
    fbe_api_raid_group_get_paged_info_t     paged_info;

    status = fbe_api_raid_group_get_paged_bits(rg_obj, &paged_info, FBE_TRUE /* Get data from disk*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: RG 0x%x, pos: %u, NR count: %llu, pos NR count:%llu\n",
               __FUNCTION__, rg_obj, position,
               (unsigned long long)paged_info.chunk_count,
               (unsigned long long)paged_info.pos_nr_chunks[position]);
    if (paged_info.num_nr_chunks != paged_info.chunk_count ||
        paged_info.pos_nr_chunks[position] != paged_info.chunk_count) {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s rg obj: 0x%x pos: %u, chunk count: %llu, NR count: %llu, pos NR count: %llu",
                   __FUNCTION__, rg_obj, position,
                   (unsigned long long)paged_info.chunk_count,
                   (unsigned long long)paged_info.num_nr_chunks,
                   (unsigned long long)paged_info.pos_nr_chunks[position]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}

static void defiler_test_setup_pause_param(fbe_sep_package_load_params_t *param, fbe_object_id_t rg_obj)
{
    fbe_scheduler_debug_hook_t *debug_hook_p;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set bgop hook on RG 0x%x==",
               __FUNCTION__, rg_obj);
    sep_config_fill_load_params(param);

    debug_hook_p = &param->scheduler_hooks[0];

    debug_hook_p->object_id = rg_obj;
    debug_hook_p->monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
    debug_hook_p->monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
    debug_hook_p->val1 = 0;
    debug_hook_p->val2 = 0;
    debug_hook_p->check_type = SCHEDULER_CHECK_STATES;
    debug_hook_p->action = SCHEDULER_DEBUG_ACTION_PAUSE;
}


/*!**************************************************************
 * defiler_test_reinsert_drive()
 ****************************************************************
 * @brief
 *  Remove drive from slot 0_0_0 and reinsert.
 *  Verify RG 0x81 and 0x82 finish rebuilding
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_reinsert_drive(void)
{
    defiler_test_case_t test;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);
    defiler_test_remove_drive(&test);

    defiler_reinsert_drive(test.port, test.encl, test.slot,
                           test.dh_spa, test.dh_spb);

    defiler_test_verify_not_rb_logging(&test);
    defiler_test_wait_rb_complete(&test);
}

/*!**************************************************************
 * defiler_test_new_drive()
 ****************************************************************
 * @brief
 *  Remove drive from slot 0_0_0 and reinsert a new drive
 *  Verify all chunks are mark NR on rg 0x81 and 0x82
 *  Verify RG 0x81 and 0x82 finish rebuilding
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_new_drive(void)
{
    defiler_test_case_t test;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);
    defiler_test_remove_drive(&test);

    defiler_test_set_bg_ops(&test, FBE_FALSE);
    sabbath_insert_blank_new_drive(test.port, test.encl, test.slot, &test.dh_spb, &test.dh_spb);

    defiler_test_verify_not_rb_logging(&test);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    defiler_verify_nr_all_chunks(test.rg_spa, test.fault_position);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    defiler_verify_nr_all_chunks(test.rg_spb, test.fault_position);

    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_wait_rb_complete(&test);
}

/*!**************************************************************
 * defiler_test_remove_drive_when_poweroff()
 ****************************************************************
 * @brief
 *  Power down SPA and SPB
 *  Remove drive from slot 0_0_0
 *  Reboot both SPSs
 *  Reinsert the drive
 *  Verify rg 0x81 and 0x82 finished rebuilding
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_remove_drive_when_poweroff(void)
{
    defiler_test_case_t test;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);
    fbe_test_sep_util_destroy_neit_sep_both_sps();
    sabbath_pull_drive(test.port, test.encl, test.slot, &test.dh_spa, &test.dh_spb);
    sep_config_load_sep_and_neit_both_sps();

    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_verify_rb_logging(&test);
    defiler_reinsert_drive(test.port, test.encl, test.slot, test.dh_spa, test.dh_spb);
    defiler_test_verify_not_rb_logging(&test);
    defiler_test_wait_rb_complete(&test);
}

/*!**************************************************************
 * defiler_test_replace_drive_when_poweroff()
 ****************************************************************
 * @brief
 *  Power down SPA and SPB
 *  Replace slot 0_0_0 with a new drive
 *  Verify all chunks are marked NR on rg 0x81 and 0x82
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_replace_drive_when_poweroff(void)
{
    defiler_test_case_t test;
    fbe_sep_package_load_params_t spa_params, spb_params;


    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);

    defiler_shutdown_sps();

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Replace drive on %u_%u_%u\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    sabbath_pull_drive(test.port, test.encl, test.slot, &test.dh_spa, &test.dh_spb);
    sabbath_insert_blank_new_drive(test.port, test.encl, test.slot, &test.dh_spb, &test.dh_spb);

    defiler_test_setup_pause_param(&spa_params, test.rg_spa);
    defiler_test_setup_pause_param(&spb_params, test.rg_spb);

    defiler_boot_sps(&spa_params, &spb_params);

    defiler_test_set_bg_ops(&test, FBE_FALSE);
    defiler_test_verify_not_rb_logging(&test);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    defiler_verify_nr_all_chunks(test.rg_spa, test.fault_position);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    defiler_verify_nr_all_chunks(test.rg_spb, test.fault_position);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_del_scheduler_hook(&spa_params.scheduler_hooks[0]);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_del_scheduler_hook(&spb_params.scheduler_hooks[0]);

    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_wait_rb_complete(&test);
}

/*!**************************************************************
 * defiler_test_replace_drive_when_peer_poweroff()
 ****************************************************************
 * @brief
 *  Power down SPB
 *  Replace slot 0_0_0 with a new drive
 *  Verify all chunks are marked NR on rg 0x81
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_replace_drive_when_peer_poweroff(void)
{
    defiler_test_case_t test;
    fbe_sep_package_load_params_t spb_params;


    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Shutdown SPB\n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Replace drive on %u_%u_%u\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    sabbath_pull_drive(test.port, test.encl, test.slot, &test.dh_spa, &test.dh_spb);
    sabbath_insert_blank_new_drive(test.port, test.encl, test.slot, &test.dh_spb, &test.dh_spb);


    defiler_test_setup_pause_param(&spb_params, test.rg_spb);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Boot SPB\n", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit_params(&spb_params, NULL);


    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify RL cleared on SPB\n", __FUNCTION__);
    defiler_test_set_bg_ops(&test, FBE_FALSE);
    defiler_test_verify_not_rb_logging(&test);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify all chunks are NR on RG 0x%x\n",
               __FUNCTION__, test.rg_spb);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    defiler_verify_nr_all_chunks(test.rg_spb, test.fault_position);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_del_scheduler_hook(&spb_params.scheduler_hooks[0]);

    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_wait_rb_complete(&test);
}


/*!**************************************************************
 * defiler_test_slf_when_poweroff()
 ****************************************************************
 * @brief
 *  Power down both SP
 *  SLF slot 0_0_0 on SPA
 *  Wait for rebuild complete
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-04-21 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_slf_when_poweroff(void)
{
    defiler_test_case_t test;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);

    defiler_shutdown_sps();

    mut_printf(MUT_LOG_TEST_STATUS, "%s: SLF drive %u_%u_%u on SPA\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_pull_drive(test.port, test.encl, test.slot, &test.dh_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to pull drive from SPA");

    defiler_boot_sps(NULL, NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify RL cleared on both SPs\n", __FUNCTION__);
    defiler_test_verify_not_rb_logging(&test);

    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_wait_rb_complete(&test);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: reinsert drive %u_%u_%u on SPA\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(test.port, test.encl, test.slot, test.dh_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to reinsert drive from SPA");
    fbe_api_sleep(2000);
    defiler_test_wait_rb_complete(&test);
}

/*!**************************************************************
 * defiler_test_slf_when_poweron()
 ****************************************************************
 * @brief
 *  Power down both SP
 *  SLF slot 0_0_0 on SPA
 *  Wait for rebuild complete
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-04-21 - Created. Kang Jamin
 *
 ****************************************************************/
static void defiler_test_slf_when_poweron(void)
{
    defiler_test_case_t test;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Enter\n", __FUNCTION__);
    defiler_init_test_case(&test);

    defiler_shutdown_sps();

    mut_printf(MUT_LOG_TEST_STATUS, "%s: pull drive on %u_%u_%u\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    sabbath_pull_drive(test.port, test.encl, test.slot, &test.dh_spa, &test.dh_spb);

    defiler_boot_sps(NULL, NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: SLF drive %u_%u_%u on SPA\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_reinsert_drive(test.port, test.encl, test.slot, test.dh_spb);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "reinsert_drive: fail to reinsert drive from SPB");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify RL cleared on both SPs\n", __FUNCTION__);
    defiler_test_verify_not_rb_logging(&test);

    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_wait_rb_complete(&test);


    mut_printf(MUT_LOG_TEST_STATUS, "%s: reinsert drive %u_%u_%u on SPA\n",
               __FUNCTION__, test.port, test.encl, test.slot);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(test.port, test.encl, test.slot, test.dh_spa);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to reinsert drive from SPA");
    fbe_api_sleep(2000);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify RL cleared on both SPs\n", __FUNCTION__);
    defiler_test_verify_not_rb_logging(&test);
    defiler_test_set_bg_ops(&test, FBE_TRUE);
    defiler_test_wait_rb_complete(&test);
}


/*!**************************************************************
 * defiler_dualsp_test()
 ****************************************************************
 * @brief
 *      start defiler test
 * @param  None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/
void defiler_dualsp_test(void)
{
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test reinsert drive\n", __FUNCTION__);
    defiler_test_reinsert_drive();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test new drive\n", __FUNCTION__);
    defiler_test_new_drive();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test replace drive when poweroff\n", __FUNCTION__);
    defiler_test_replace_drive_when_poweroff();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test replace drive when peer poweroff\n", __FUNCTION__);
    defiler_test_replace_drive_when_peer_poweroff();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test SLF when poweroff\n", __FUNCTION__);
    defiler_test_slf_when_poweroff();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Test SLF when poweron\n", __FUNCTION__);
    defiler_test_slf_when_poweron();
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Done\n", __FUNCTION__);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end defiler_dualsp_test()
 ******************************************/

/*!**************************************************************
 * defiler_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a defiler c4mirror raid group test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *    2015-03-23. Created. Kang Jamin
 ****************************************************************/
void defiler_dualsp_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        rg_config_p = &defiler_raid_group_config[0];

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }



    return;
}
/**************************************
 * end defiler_dualsp_setup()
 **************************************/

/*!**************************************************************
 * defiler_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the defiler test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2015-03-23 - Created. Kang Jamin
 *
 ****************************************************************/

void defiler_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end defiler_dualsp_cleanup()
 ******************************************/
