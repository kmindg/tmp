/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file vulture_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains quiesce/unquiesce test for metadata I/O
 *
 * @version
 *   05/05/2015 - Created. Kang, Jamin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "sep_rebuild_utils.h"
#include "sep_hook.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe/fbe_dest_service.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * vulture_short_desc = "Test quiesce/unquiesce for metadata I/O";
char * vulture_long_desc ="\
The vulture senario tests quiesce/unquiesce for metadata I/O.\n\
\n\
Dependencies:\n\
        - Dual SP \n\
        - Debug Hooks for Write Log Remap.\n\
\n\
Test case 1:\n\
        - For AR 724320\n\
         1. Degraded Raid group\n\
         2. Inject delay errors on metadata\n\
         3. Run I/O and wait for delay errors on metadata to increase\n\
         4. Simulate a drive glitch, wait for drive to unquiesce.\n\
         5. Repeat the glitch.\n\
         6. Remove failure, hook and LEI, wait for I/O to complete\n\
\n";

/*!*******************************************************************
 * @def VULTURE_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test.
 *
 *********************************************************************/
#define VULTURE_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def VULTURE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define VULTURE_CHUNKS_PER_LUN 3

static fbe_test_rg_configuration_t vulture_raid_group_config[] = {
    /* width, capacity     raid type,                  class,                  block size      RAID-id.   bandwidth.*/
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,         0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


static void vulture_init_debug_hook(fbe_scheduler_debug_hook_t *hook)
{
    memset(hook, 0, sizeof(*hook));

    hook->check_type = SCHEDULER_CHECK_STATES;
    hook->action = SCHEDULER_DEBUG_ACTION_PAUSE;
}

static void vulture_add_debug_hook(const fbe_scheduler_debug_hook_t *hook)
{
    fbe_status_t status;

    status = fbe_api_scheduler_add_debug_hook(hook->object_id,
                                              hook->monitor_state,
                                              hook->monitor_substate,
                                              hook->val1, hook->val2,
                                              hook->check_type, hook->action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static void vulture_wait_debug_hook(const fbe_scheduler_debug_hook_t *hook)
{
    fbe_status_t status;

    status = fbe_test_wait_for_debug_hook(hook->object_id,
                                          hook->monitor_state,
                                          hook->monitor_substate,
                                          hook->check_type, hook->action,
                                          hook->val1, hook->val2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static void vulture_delete_debug_hook(const fbe_scheduler_debug_hook_t *hook)
{
    fbe_status_t status;

    status = fbe_api_scheduler_del_debug_hook(hook->object_id,
                                              hook->monitor_state,
                                              hook->monitor_substate,
                                              hook->val1, hook->val2,
                                              hook->check_type, hook->action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}


/*!**************************************************************
 * vulture_get_parity_position_on_metadata()
 ****************************************************************
 * @brief
 *  Get parity position on first pagedmetadata chunk
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return parity position
 *
 * @author
 *  05/06/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_u32_t vulture_get_parity_position_on_metadata(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t rg_object_id;
    fbe_raid_group_map_info_t rg_map;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.lba = rg_info.paged_metadata_start_lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return rg_map.parity_pos;
}

/*!**************************************************************
 * vulture_determine_drive_to_remove()
 ****************************************************************
 * @brief
 *  Setup the position we will remove in test.
 *
 * @param rg_config_p - the raid group configuration to test
 * @which - the role of removal: 0 -> degraded the raidgroup, 1 -> remove second drive
 *
 * @return None.
 *
 * @author
 *  05/06/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_u32_t vulture_determine_drive_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t which)
{
    fbe_u32_t pos = 1;
    fbe_u32_t parity_pos_on_metadata = vulture_get_parity_position_on_metadata(rg_config_p);
    /* Parity pos on on metadata chunk: 2 */
    switch (which) {
    case 0:
        if (pos == parity_pos_on_metadata) {
            pos += 1;
        }
        /* remove a drive the isn't parity on metadata */
        rg_config_p->specific_drives_to_remove[0] = pos;
        break;
    case 1:
    default:
        pos = parity_pos_on_metadata;
        rg_config_p->specific_drives_to_remove[1] = pos;
        break;
    }
    return pos;
}

/*!**************************************************************
 * vulture_inject_delay_io_on_metadata()
 ****************************************************************
 * @brief
 *  Inject delay I/O on specificed LBA.
 *  We use SILENT_DROP to simulate the incomplete write.
 *
 * @param rg_object_id - the raid group object id
 * @param lba - the lba to inject
 *
 * @return None
 *
 * @author
 *  05/06/2015 - Created. Kang, Jamin
 *
 ****************************************************************/
static void vulture_inject_delay_io_on_metadata(fbe_object_id_t rg_object_id,
                                                fbe_u32_t delay_ms)
{
    fbe_status_t status;
    fbe_raid_group_map_info_t rg_map;
    fbe_lba_t pba;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_api_logical_error_injection_record_t error_record = {
        0x1,        /* pos_bitmap */
        0x10,       /* width */
        0x0,        /* lba */
        0x80,       /* blocks */
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT,/* error mode */
        0x0,        /* error count */
        delay_ms,   /* error limit */
        0x0,        /* skip count */
        0x0,        /* skip limit */
        0x1,        /* error adjcency */
        0x0,        /* start bit */
        0x0,        /* number of bits */
        0x0,        /* erroneous bits */
        0x0,        /* crc detectable */
        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
    };

    /* Inject iw on metadata */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Inject delay I/O", __FUNCTION__);

    /* Inject on metadata parity position */
    memset(&rg_map, 0, sizeof(rg_map));
    rg_map.lba = rg_info.paged_metadata_start_lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &rg_map);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    pba = rg_map.pba - rg_map.offset;
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: Inject iw IO on %u offset: 0x%llx, ppos: %u\n",
               __FUNCTION__, rg_map.data_pos, (unsigned long long)pba,
               rg_map.parity_pos);
    error_record.pos_bitmap = 1 << rg_map.parity_pos;
    error_record.lba = pba;
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * vulture_wait_delay_io_match()
 ****************************************************************
 * @brief
 *  Wait unit expected delay write errors hit.
 *
 * @param expected_errors - the number of injected errors.
 *
 * @return None
 *
 * @author
 *  05/06/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void vulture_wait_delay_io_match(fbe_u32_t expected_errors)
{
    fbe_u32_t max_wait_time = 10000;
    fbe_u32_t wait_time = 200;
    fbe_u32_t wait_count = max_wait_time / wait_time;

    for (; wait_count; wait_count -= 1) {
        fbe_api_logical_error_injection_get_stats_t stats;
        fbe_status_t status;

        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK );
        if (stats.num_errors_injected >= expected_errors) {
            mut_printf(MUT_LOG_TEST_STATUS,
                       "%s: en: %u, num injected: 0x%llx, num object: %u, num records: %u\n",
                       __FUNCTION__,
                       stats.b_enabled, stats.num_errors_injected,
                       stats.num_objects_enabled, stats.num_records);
            return;
        }
        fbe_api_sleep(wait_time);
    }
    MUT_FAIL_MSG("Wait LEI failed");
}

/*!**************************************************************
 * vulture_remove_drive()
 ****************************************************************
 * @brief
 *  Removed 1/2 disks to make the raid group degraded.
 *
 * @param rg_config - the raid group
 * @which - which drive to remove
 * @return None
 *
 * @author
 *  05/06/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void vulture_remove_drive(fbe_test_rg_configuration_t *rg_config, fbe_u32_t which)
{
    fbe_status_t status;
    fbe_u32_t physical_objects;
    fbe_u32_t objects_to_remove = 1;
    fbe_u32_t degraded_position;

    vulture_determine_drive_to_remove(rg_config, which);
    status = fbe_api_get_total_objects(&physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting object number");

    physical_objects -= objects_to_remove;
    degraded_position = rg_config->specific_drives_to_remove[which];
    sep_rebuild_utils_remove_drive_and_verify(rg_config, degraded_position,
                                              physical_objects,
                                              &rg_config->drive_handle[which]);
    mut_printf(MUT_LOG_TEST_STATUS, "Drive removed: %d_%d_%d\n",
               rg_config->rg_disk_set[degraded_position].bus,
               rg_config->rg_disk_set[degraded_position].enclosure,
               rg_config->rg_disk_set[degraded_position].slot);
}

/*!**************************************************************
 * vulture_reinsert_drive()
 ****************************************************************
 * @brief
 *  Reinsert the drives we removed
 *
 * @param rg_config_p - the raid group configuration to test
 * @parm which  - role of the drive
 * @return None.
 *
 * @author
 *  05/08/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void vulture_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t which)
{
    fbe_u32_t objects_to_add = 1;
    fbe_u32_t physical_objects;
    fbe_status_t status;

    status = fbe_api_get_total_objects(&physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Error getting object number");

    physical_objects += objects_to_add;
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p,
                                                rg_config_p->specific_drives_to_remove[which],
                                                physical_objects,
                                                &rg_config_p->drive_handle[which]);
}

/*!**************************************************************
 * vulture_degraded_raid_group()
 ****************************************************************
 * @brief
 *  Removed 1 disk to make the raid group degraded.
 *
 * @param rg_config - the raid group
 *
 * @return None
 *
 * @author
 *  05/06/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void vulture_degraded_raid_group(fbe_test_rg_configuration_t *rg_config)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s Entry", __FUNCTION__);
    vulture_remove_drive(rg_config, 0);

    /* Lookup RG object ID and save them locally */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     30000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Succeed\n", __FUNCTION__);
}

static void vulture_reinsert_degraded_drive(fbe_test_rg_configuration_t *rg_config)
{
    vulture_reinsert_drive(rg_config, 0);
}


static void vulture_remove_second_drive(fbe_test_rg_configuration_t *rg_config)
{
    vulture_remove_drive(rg_config, 1);
}

static void vulture_reinsert_second_drive(fbe_test_rg_configuration_t *rg_config_p)
{
    vulture_reinsert_drive(rg_config_p, 1);
}

static fbe_u32_t vulture_get_second_drive_position(fbe_test_rg_configuration_t *rg_config)
{
    return vulture_determine_drive_to_remove(rg_config, 1);
}

/*!*****************************************************************
 * vulture_run_test_case_AR724320
 *******************************************************************
 * @brief
 *  Test for AR 724320
 *       1. Degraded Raid group
 *       2. Inject delay I/O on metatdata
 *       3. Run I/O and wait for delay errors on metadata to increase
 *       4. Simulate a drive glitch, wait for drive to unquiesce.
 *       5. Repeat the glitch.
 *       6. Remove failure, hook and LEI, wait for I/O to complete
 *******************************************************************/
static void vulture_run_test_case_AR724320(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_api_rdgen_context_t context;
    fbe_lba_t user_iw_lba = 0;
    fbe_scheduler_debug_hook_t unquiesce_hook;
    fbe_scheduler_debug_hook_t eval_hook;
    fbe_u32_t parity_pos;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start", __FUNCTION__);
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: degraded raid group", __FUNCTION__);
    vulture_degraded_raid_group(rg_config_p);

    /* Disable LEI and clear all records */
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Inject delay I/O", __FUNCTION__);
    /* Inject on metadata parity position */
    vulture_inject_delay_io_on_metadata(rg_object_id, 20000);

    /* Enable LEI on this raid group */
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_send_one_io_asynch(&context,
                                              rg_object_id,
                                              FBE_CLASS_ID_INVALID,
                                              FBE_PACKAGE_ID_SEP_0,
                                              FBE_RDGEN_OPERATION_WRITE_ONLY,
                                              FBE_RDGEN_LBA_SPEC_FIXED,
                                              user_iw_lba,
                                              1,
                                              FBE_RDGEN_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait LEI", __FUNCTION__);
    vulture_wait_delay_io_match(1);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Add eval hook", __FUNCTION__);
    vulture_init_debug_hook(&eval_hook);
    eval_hook.object_id = rg_object_id;
    eval_hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL;
    eval_hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2;
    vulture_add_debug_hook(&eval_hook);


    parity_pos = vulture_get_second_drive_position(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove second drive (%u) to hit eval RL hook", __FUNCTION__, parity_pos);
    vulture_remove_second_drive(rg_config_p);
    vulture_wait_debug_hook(&eval_hook);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinsert second drive and wait path ready (simulate glitch)", __FUNCTION__);
    vulture_reinsert_second_drive(rg_config_p);
    fbe_api_wait_for_block_edge_path_state(rg_object_id, parity_pos,
                                           FBE_PATH_STATE_ENABLED,
                                           FBE_PACKAGE_ID_SEP_0, 10000/*ms*/);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait unquiesce hook", __FUNCTION__);
    vulture_init_debug_hook(&unquiesce_hook);
    unquiesce_hook.object_id = rg_object_id;
    unquiesce_hook.monitor_state = SCHEDULER_MONITOR_STATE_BASE_CONFIG_UNQUIESCE;
    unquiesce_hook.monitor_substate = FBE_BASE_CONFIG_SUBSTATE_UNQUIESCE_METADATA_MEMORY_UPDATED;
    vulture_add_debug_hook(&unquiesce_hook);
    vulture_delete_debug_hook(&eval_hook);
    vulture_wait_debug_hook(&unquiesce_hook);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove second drive to hit eval RL hook", __FUNCTION__);
    vulture_add_debug_hook(&eval_hook);
    vulture_remove_second_drive(rg_config_p);
    vulture_wait_debug_hook(&eval_hook);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinsert second drive and wait path ready", __FUNCTION__);
    vulture_reinsert_second_drive(rg_config_p);
    fbe_api_wait_for_block_edge_path_state(rg_object_id, parity_pos,
                                           FBE_PATH_STATE_ENABLED,
                                           FBE_PACKAGE_ID_SEP_0, 10000/*ms*/);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove failure, hooks and LEI", __FUNCTION__);
    vulture_reinsert_degraded_drive(rg_config_p);
    vulture_delete_debug_hook(&eval_hook);
    vulture_delete_debug_hook(&unquiesce_hook);
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for I/O to complete", __FUNCTION__);
    fbe_api_rdgen_wait_for_ios(&context, FBE_PACKAGE_ID_SEP_0, 1);
    status = fbe_api_rdgen_test_context_destroy(&context);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Passed\n", __FUNCTION__);
}

/*!**************************************************************
 * vulture_run_test()
 ****************************************************************
 * @brief
 *  Entry of the vulture dual SP test
 *
 * @param rg_config_p - the raid group configuration to test
 *
 * @return None.
 *
 * @author
 *  05/06/2015 - Created. Kang, Jamin
 *
 ****************************************************************/
void vulture_run_test(fbe_test_rg_configuration_t *rg_config_p, void *test_case_in)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Entered\n", __FUNCTION__);
    vulture_run_test_case_AR724320(rg_config_p);
    return;
}
/******************************************
 * End vulture_run_test()
 ******************************************/

/*!**************************************************************
 * vulture_dualsp_test()
 ****************************************************************
 * @brief
 *  Run dual SP quiesce/unquiesce test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/05/2015 - Created. Kang, Jamin
 *
 ****************************************************************/
void vulture_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_status_t                status;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    status = fbe_api_database_stop_all_background_service(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable the recovery queue so that a spare cannot swap-in */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_LOW);

    rg_config_p = &vulture_raid_group_config[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p,
                                                  NULL,
                                                  vulture_run_test,
                                                  VULTURE_LUNS_PER_RAID_GROUP,
                                                  VULTURE_CHUNKS_PER_LUN,
                                                  FBE_FALSE);
    /* Enable the recovery queue */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s PASS. ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end vulture_dualsp_test()
 ******************************************/

/*!**************************************************************
 * vulture_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a dual SP quiesce/unquiesce test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/05/2015 - Created. Kang, Jamin
 *
 ****************************************************************/
void vulture_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation()) {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for vulture test ===\n");

        rg_config_p = &vulture_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/******************************************
 * End vulture_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * vulture_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the vulture dual SP test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  05/05/2015 - Created. Kang, Jamin
 *
 ****************************************************************/
void vulture_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s: === Cleanup for vulture_dualsp_test ===\n", __FUNCTION__);
    if (fbe_test_util_is_simulation()) {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * End vulture_dualsp_cleanup()
 ******************************************/

/*******************************
 * End file vulture_test.c
 *******************************/

