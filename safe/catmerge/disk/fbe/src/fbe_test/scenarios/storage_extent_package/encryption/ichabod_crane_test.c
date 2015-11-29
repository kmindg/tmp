/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file ichabod_crane_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an encryption I/O test.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
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
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_encryption_interface.h"
#include "sep_hook.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_zeroing_utils.h"
#include "fbe_test.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_trace_interface.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "neit_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * ichabod_short_desc = "This scenario will drive I/O during encryption re-key.";
char * ichabod_long_desc ="\
The scenario drives I/O during encryption re-key.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
\n\
Description last updated: 10/25/2013.\n";


/*!*******************************************************************
 * @var ichabod_use_fixed_sys_disk_config
 *********************************************************************
 * @brief
 *   TRUE to use drives on the system disk.
 *********************************************************************/
static fbe_bool_t ichabod_use_fixed_sys_disk_config = FBE_TRUE;

/*!*******************************************************************
 * @def ICHABOD_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define ICHABOD_TEST_LUNS_PER_RAID_GROUP 2

/*!*******************************************************************
 * @def ICHABOD_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ICHABOD_CHUNKS_PER_LUN 15


/*!*******************************************************************
 * @def ICHABOD_TEST_LUNS_PER_RAID_GROUP_UNBIND_TEST
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define ICHABOD_TEST_LUNS_PER_RAID_GROUP_UNBIND_TEST ICHABOD_TEST_LUNS_PER_RAID_GROUP *4

/*!*******************************************************************
 * @def ICHABOD_CHUNKS_PER_LUN_UNBIND_TEST
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ICHABOD_CHUNKS_PER_LUN_UNBIND_TEST ICHABOD_CHUNKS_PER_LUN / 4

/*!*******************************************************************
 * @def ICHABOD_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ICHABOD_SMALL_IO_SIZE_MAX_BLOCKS 1024
/*!*******************************************************************
 * @def ICHABOD_LIGHT_THREAD_COUNT
 *********************************************************************
 * @brief Lightly loaded test to allow rekey to proceed.
 *
 *********************************************************************/
#define ICHABOD_LIGHT_THREAD_COUNT 1

/*!*******************************************************************
 * @def ICHABOD_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ICHABOD_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE //4*0x400*6

/*!*******************************************************************
 * @var ichabod_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ichabod_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG,       0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TAG,  FBE_TEST_RG_CONFIG_RANDOM_TAG,   520,  7,  0,
        { {0,0,0}, {0,0,1}, {0,0,2}, {0,0,3} } }, 

    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,          1,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          0},

    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,   520,  2,  FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var ichabod_raid_group_config_system
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ichabod_raid_group_config_system[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0,
       { {0,0,0}, {0,0,1}, {0,0,2}, {0,0,3} } },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var ichabod_raid_group_config_system_1
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ichabod_raid_group_config_system_1[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0,
       { {0,0,0}, {0,0,1}, {0,0,2}, {0,0,3} } },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/***************************************** 
 * FORWARD FUNCTION DECLARATIONS
 *****************************************/
static void ichabod_crane_read_pattern(fbe_api_rdgen_context_t *read_context_p,
                                       fbe_u32_t num_luns);
static void ichabod_zeroing_interaction_test(fbe_test_rg_configuration_t * const rg_config_p);
static void ichabod_zeroing_drive_failure_test(fbe_test_rg_configuration_t * const rg_config_p);
static void ichabod_crane_del_zero_debug_hook(fbe_object_id_t object_id);
static void ichabod_crane_add_zero_debug_hook(fbe_object_id_t object_id);
static fbe_status_t ichabod_crane_test_validate_pvd_user_rg_mp_handle(fbe_test_rg_configuration_t *rg_config_p);

static void ichabod_vault_set_rg_options(void)
{
    #define VAULT_ENCRYPT_WAIT_TIME_MS 1000
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = VAULT_ENCRYPT_WAIT_TIME_MS;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}

static void ichabod_crane_del_zero_debug_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    mut_printf(MUT_LOG_TEST_STATUS, "Removing debug hooks for PVD:0x%08x..\n", object_id);
    status = fbe_api_scheduler_del_debug_hook(object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to set the debug hook\n");

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_scheduler_del_debug_hook(object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                                  NULL,
                                                  NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to set the debug hook\n");
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    
}

static void ichabod_crane_add_zero_debug_hook(fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    mut_printf(MUT_LOG_TEST_STATUS, "Adding debug hooks for PVD:0x%08x..\n", object_id);
    status = fbe_api_scheduler_add_debug_hook(object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to set the debug hook\n");

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_api_sim_transport_set_target_server(other_sp);
        status = fbe_api_scheduler_add_debug_hook(object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION, 
                                                  NULL,
                                                  NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to set the debug hook\n");
        fbe_api_sim_transport_set_target_server(this_sp);
    }
    
}

static fbe_status_t ichabod_crane_test_validate_pvd_user_rg_mp_handle(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                   rg_count = 0;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   disk_index;
    fbe_object_id_t             pvd_id;
    fbe_provision_drive_control_get_mp_key_handle_t mp_key_handle;
    fbe_status_t status;
    fbe_u8_t user_edge_index = 0;
    current_rg_config_p = rg_config_p;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                    &pvd_id);
            if(fbe_private_space_layout_object_id_is_system_pvd(pvd_id))
            {
                user_edge_index = (pvd_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_3) ? 2 : 4;
            }
            
            mp_key_handle.index = user_edge_index; 
            mp_key_handle.mp_handle_1 = 0;
            mp_key_handle.mp_handle_2 = 0;
            status = fbe_api_provision_drive_get_miniport_key_handle(pvd_id, &mp_key_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            if(mp_key_handle.mp_handle_1 == 0 && mp_key_handle.mp_handle_2 == 0) {
                MUT_FAIL_MSG("Both Keys handles are NULL\n");
            }
        }
        current_rg_config_p++;
    }
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * ichabod_read_only_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run read only I/O during encryption.
 *  Check pattern after encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_read_only_test(fbe_test_rg_configuration_t * const rg_config_p,
                                fbe_api_rdgen_peer_options_t peer_options,
                                fbe_data_pattern_t background_pattern,
                                fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);

    /* Make sure the patterns are still there.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_reinit(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change all contexts to continually read.
     */
    for (index = 0; index < num_luns; index++) {
        MUT_ASSERT_INT_EQUAL(read_context_p[index].start_io.specification.max_passes, 1);
        fbe_api_rdgen_io_specification_set_max_passes(&read_context_p[index].start_io.specification, 0);
    }

    /* Kick off the continual read, this runs all during the rest of the test while we remove and then reinsert drives.
     */
    status = fbe_api_rdgen_start_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* kick off a rekey.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey completed successfully.");
    /* Stop our continual read stream.
     */
    status = fbe_api_rdgen_stop_tests(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    /* Can this fail if no I/O runs before we stop?  */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_read_only_test()
 ******************************************/

/*!**************************************************************
 * ichabod_check_pattern_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run encryption.
 *  Check pattern after encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_check_pattern_test(fbe_test_rg_configuration_t * const rg_config_p,
                                fbe_api_rdgen_peer_options_t peer_options,
                                fbe_data_pattern_t background_pattern,
                                fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    /* kick off a rekey.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Encryption rekey completed successfully.");

    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_check_pattern_test()
 ******************************************/
/*!**************************************************************
 * ichabod_system_drive_test()
 ****************************************************************
 * @brief
 *  Create user rg/luns on system drives.
 *  Make sure it was zeroed properly.
 *  Run I/O.
 *  Destroy the rg/luns.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_system_drive_test(fbe_test_rg_configuration_t * const rg_config_p,
                               fbe_api_rdgen_peer_options_t peer_options,
                               fbe_data_pattern_t background_pattern,
                               fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);

    elmo_create_raid_groups(rg_config_p, ICHABOD_TEST_LUNS_PER_RAID_GROUP, ICHABOD_CHUNKS_PER_LUN);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, FBE_LBA_INVALID, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of bind failed\n");
    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_rg_validate_lbas_encrypted(rg_config_p, 
                                        0,/* lba */ 
                                        64, /* Blocks to check */ 
                                        64 /* Blocks encrypted */);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);
    return;
}
/******************************************
 * end ichabod_system_drive_test()
 ******************************************/
/*!**************************************************************
 * ichabod_system_drive_createrg_encrypt_reboot_test()
 ****************************************************************
 * @brief
 *  Create user rg/luns on system drives.
 *  Run I/O
 *  Enable encryption
 *  Reboot the SPs.
 *  Make sure the LUN data is the same.
 *  Destroy the rg/luns.  
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/

void ichabod_system_drive_createrg_encrypt_reboot_test(fbe_test_rg_configuration_t * const rg_config_p,
                                      fbe_api_rdgen_peer_options_t peer_options,
                                      fbe_data_pattern_t background_pattern,
                                      fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_u32_t                   rg_count = 0;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_index;
    fbe_object_id_t rg_object_id;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    // Create RG
    elmo_create_raid_groups(rg_config_p, ICHABOD_TEST_LUNS_PER_RAID_GROUP, ICHABOD_CHUNKS_PER_LUN);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Verify that it created and in ready state
    mut_printf(MUT_LOG_TEST_STATUS, "== Verify that RG %d is in RDY state ==", rg_config_p->raid_group_id);
    status = fbe_test_util_wait_for_rg_object_id_by_number(rg_config_p->raid_group_id, &rg_object_id, 18000);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    // Wait for RDY State
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 120000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== RG %d (0x%x) is in RDY state ==", rg_config_p->raid_group_id, rg_object_id);

    // Start IO
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, FBE_LBA_INVALID, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of bind failed\n");

    // We use a zero background pattern, and then later we put random other I/Os into this.
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);

    // We do the pattern writes now prior to degrading.
    // This gives us a different patterns across the entire range. 
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    // Make sure the patterns are still there.
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    ichabod_crane_read_pattern(read_context_p, num_luns);

    mut_pause();

    // enable encryption
    mut_printf(MUT_LOG_TEST_STATUS, "Enable Encryption...Start");
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Enabled Encryption...Complete");

    // Wait for Vault to be encrypted
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for vault to be encrypted before reboot ==");
    fbe_test_sep_util_wait_object_encryption_mode(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED, FBE_TEST_WAIT_TIMEOUT_MS);

    // Make sure that the user RG still in RDY state
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 120000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== RG %d (0x%x) is in RDY state ==", rg_config_p->raid_group_id, rg_object_id);

    /* Validate that Keys are getting pushed to all the PVDs */
    status = ichabod_crane_test_validate_pvd_user_rg_mp_handle(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_pause();

    // Reboot SP
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(rg_config_p, active_sp);

    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);
    mut_pause();

    // Wait for LUNs to be in RDY state
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs to be ready ==");

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < ICHABOD_TEST_LUNS_PER_RAID_GROUP; lun_index++){
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUN %d (0x%x) to be ready ==", current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                       lun_object_id);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Validate that keys are pushed again to all the PVDs on reboot. Since the PVDs will still be Zeroing
     * at this time, the mode would not have changed but still we need to make sure valid keys are being pushed down */
    status = ichabod_crane_test_validate_pvd_user_rg_mp_handle(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    //  Make sure no errors occurred.
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);
    return;
}
/******************************************
 * end ichabod_system_drive_createrg_encrypt_reboot_test()
 ******************************************/

/*!**************************************************************
 * ichabod_system_drive_reboot_test()
 ****************************************************************
 * @brief
 *  Create user rg/luns on system drives.
 *  Make sure it was zeroed properly.
 *  Run I/O.
 *  Reboot the SPs.
 *  Make sure the LUN data is the same.
 *  Destroy the rg/luns.  
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_system_drive_reboot_test(fbe_test_rg_configuration_t * const rg_config_p,
                                      fbe_api_rdgen_peer_options_t peer_options,
                                      fbe_data_pattern_t background_pattern,
                                      fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   rg_count = 0;
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    elmo_create_raid_groups(rg_config_p, ICHABOD_TEST_LUNS_PER_RAID_GROUP, ICHABOD_CHUNKS_PER_LUN);

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, FBE_LBA_INVALID, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Validation of bind failed\n");
    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    ichabod_crane_read_pattern(read_context_p, num_luns);

    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(rg_config_p, active_sp);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);
    mut_pause();
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUNs to be ready");

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++){
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < ICHABOD_TEST_LUNS_PER_RAID_GROUP; lun_index++){
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, 1);
    return;
}
/******************************************
 * end ichabod_system_drive_reboot_test()
 ******************************************/
/*!**************************************************************
 * ichabod_cache_disabled_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run encryption with cache off.
 *  Make sure encryption does not proceed.
 *  Enable cache.
 *  Make sure encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/15/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_cache_disabled_test(fbe_test_rg_configuration_t * const rg_config_p,
                                fbe_api_rdgen_peer_options_t peer_options,
                                fbe_data_pattern_t background_pattern,
                                fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_persistent_memory_set_params_t params;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    mut_printf(MUT_LOG_TEST_STATUS, "Add hooks to pause encryption");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_ADD);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption to hit chunk 0.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);
    mut_printf(MUT_LOG_TEST_STATUS, "Make sure there was no progress.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PIN_CLEAN,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Simulate disabling cache.");

    /* Always force rdgen to unpin so that it simulates cache behavior of allowing another 
     * pin for the same LBA.  Normally on unpin flush, rdgen waits for a manual call to flush. 
     */

    fbe_api_persistent_memory_init_params(&params);
    params.b_always_unlock = FBE_TRUE;
    params.b_force_dirty = FBE_FALSE;
    params.force_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_PINNED_NOT_PERSISTENT;
    status = fbe_api_persistent_memory_set_params(&params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Enable encryption to run and validate it does not make progress.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_DELETE);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PIN_CLEAN,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PIN_CLEAN,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);

    mut_printf(MUT_LOG_TEST_STATUS, "Simulate enabling cache.");

    fbe_api_persistent_memory_init_params(&params);
    params.b_always_unlock = FBE_FALSE;
    params.b_force_dirty = FBE_FALSE;
    params.force_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INVALID;
    status = fbe_api_persistent_memory_set_params(&params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Make sure encryption proceeds.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_DELETE);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Encryption completed successfully.");

    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_cache_disabled_test()
 ******************************************/

/*!**************************************************************
 * ichabod_cache_dirty_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run encryption with cache telling us that the pages are dirty.
 *  Make sure encryption does proceed.
 *  Make sure RAID is seeing the pages were dirty.
 *  Make sure encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_cache_dirty_test(fbe_test_rg_configuration_t * const rg_config_p,
                                fbe_api_rdgen_peer_options_t peer_options,
                                fbe_data_pattern_t background_pattern,
                                fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_persistent_memory_set_params_t params;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    mut_printf(MUT_LOG_TEST_STATUS, "Add hooks to pause encryption");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_ADD);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption to hit chunk 0.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);
    mut_printf(MUT_LOG_TEST_STATUS, "Make sure there was no progress.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPIN_DIRTY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Simulate cache returning dirty.");

    /* Always force rdgen to unpin so that it simulates cache behavior of allowing another 
     * pin for the same LBA.  Normally on unpin flush, rdgen waits for a manual call to flush. 
     */
    fbe_api_persistent_memory_init_params(&params);
    params.b_always_unlock = FBE_TRUE;
    params.b_force_dirty = FBE_TRUE;
    params.force_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INVALID;
    status = fbe_api_persistent_memory_set_params(&params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Enable encryption to run and validate it does make progress.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_DELETE);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_WAIT);

    /* Since a dirty is getting passed back, make sure that RAID does detect the dirty 
     * and causes a dirty flush. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPIN_DIRTY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPIN_DIRTY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Make sure encryption proceeds.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x800, FBE_TEST_HOOK_ACTION_DELETE);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "Encryption completed successfully.");

    mut_printf(MUT_LOG_TEST_STATUS, "Reset cache parameters.");

    fbe_api_persistent_memory_init_params(&params);
    params.b_always_unlock = FBE_FALSE;
    params.b_force_dirty = FBE_FALSE;
    params.force_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INVALID;
    status = fbe_api_persistent_memory_set_params(&params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_cache_dirty_test()
 ******************************************/

/*!**************************************************************
 * ichabod_degraded_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run encryption with cache telling us that the pages are dirty.
 *  Make sure encryption does proceed.
 *  Make sure RAID is seeing the pages were dirty.
 *  Make sure encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_degraded_test(fbe_test_rg_configuration_t * const rg_config_p,
                           fbe_api_rdgen_peer_options_t peer_options,
                           fbe_data_pattern_t background_pattern,
                           fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern....Success");

    /* Disable non-redundant types.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) {
            current_rg_config_p->b_create_this_pass = FBE_FALSE;
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, " == Degrade all raid groups");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", current_rg_config_p->raid_group_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, " == Add hooks to detect encryption");

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x0, FBE_TEST_HOOK_ACTION_ADD);

    mut_printf(MUT_LOG_TEST_STATUS, " == Enable encryption to run.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, " == Validate Encryption does make progress when degraded.");
    /* Since we finished rebuild we should expect encryption to start.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* No ds objects */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x0, FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);

    /* Add a hook so that rebuild does not run.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* 1 ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                       FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, " == restore all raid groups");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", current_rg_config_p->raid_group_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, " == Validate Encryption does make progress when Rebuild finishes");
    /* Since we finished rebuild we should expect encryption to start.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* 1 ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                       FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x0, FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);

    mut_printf(MUT_LOG_TEST_STATUS, "== Allow rebuild to run.");

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* 1 ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                       FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, num_raid_groups, 1);
    mut_printf(MUT_LOG_TEST_STATUS, " == Make sure encryption proceeds.");

    /* Since we finished rebuild we should expect encryption to start.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX,    /* no ds objects, just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x0, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0x0, FBE_TEST_HOOK_ACTION_DELETE);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, " == Encryption completed successfully.");

    mut_printf(MUT_LOG_TEST_STATUS, " == Validate pattern ==");
    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    /* Enable non-redundant types.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) {
            current_rg_config_p->b_create_this_pass = FBE_TRUE;
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end ichabod_degraded_test()
 ******************************************/
/*!**************************************************************
 * fbe_test_kms_corrupt_rg_keys()
 ****************************************************************
 * @brief
 *  Corrupt the keys on a raid group in KMS.
 *
 * @param rg_config_p - config to corrupt keys in.
 * @param ds_list_p - Downstream list for R10.
 * @param inject_position - Position to inject on or FBE_U32_MAX to inject on all.
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_kms_corrupt_rg_keys(fbe_test_rg_configuration_t * rg_config_p,
                                          fbe_api_base_config_downstream_object_list_t *ds_list_p,
                                          fbe_u32_t inject_position)
{
    fbe_status_t status;
    fbe_u32_t disk_index;
    fbe_kms_control_get_keys_t get_keys;
    fbe_kms_control_set_keys_t set_keys;
    fbe_object_id_t rg_object_id;
    fbe_u32_t i;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_u32_t position = 0;
        for (disk_index = 0; 
            disk_index < rg_config_p->width / 2; 
            disk_index++) {

            /* Get keys to get the serial numbers. 
             */
            get_keys.object_id = ds_list_p->downstream_object_list[disk_index];
            get_keys.control_number = rg_config_p->raid_group_id;
            fbe_test_sep_util_kms_get_rg_keys(&get_keys);
            
            fbe_zero_memory(&set_keys, sizeof(fbe_kms_control_set_keys_t));
            set_keys.object_id = ds_list_p->downstream_object_list[disk_index];
            set_keys.control_number = rg_config_p->raid_group_id;
            /* Corrupt the keys.
             */
            for (i = 0; i <= 2; i++) {
                if ((inject_position == FBE_U32_MAX) ||
                    (inject_position == position)) {
                    set_keys.b_set_key[i] = FBE_TRUE;
                    set_keys.drive[i] = get_keys.drive[i];
                    fbe_set_memory(&set_keys.key1[i][0], 'E', FBE_DEK_SIZE);
                    fbe_set_memory(&set_keys.key2[i][0], 'E', FBE_DEK_SIZE);
                }
                position++;
            }
            mut_printf(MUT_LOG_TEST_STATUS, "== Corrupt keys for raid group %d object id: 0x%x",
                       rg_config_p->raid_group_id, ds_list_p->downstream_object_list[disk_index]);
            fbe_test_sep_util_kms_set_rg_keys(&set_keys);
        }
    } else {
        /* Get keys to get the serial numbers. 
         */
        get_keys.object_id = rg_object_id;
        get_keys.control_number = rg_config_p->raid_group_id;
        fbe_test_sep_util_kms_get_rg_keys(&get_keys);

        fbe_zero_memory(&set_keys, sizeof(fbe_kms_control_set_keys_t));
        set_keys.object_id = rg_object_id;
        set_keys.control_number = rg_config_p->raid_group_id;

        /* Corrupt the keys.
         */
        for (i = 0; i <= FBE_RAID_MAX_DISK_ARRAY_WIDTH; i++) {
            if ((inject_position == FBE_U32_MAX) ||
                (inject_position == i)){
                set_keys.b_set_key[i] = FBE_TRUE;
                set_keys.drive[i] = get_keys.drive[i];
                fbe_set_memory(&set_keys.key1[i][0], 'E', FBE_DEK_SIZE);
                fbe_set_memory(&set_keys.key2[i][0], 'E', FBE_DEK_SIZE);
            }
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== Corrupt keys for raid group %d object id: 0x%x",
                   rg_config_p->raid_group_id, rg_object_id);
        fbe_test_sep_util_kms_set_rg_keys(&set_keys);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_kms_corrupt_rg_keys()
 ******************************************/
/*!**************************************************************
 * fbe_test_kms_set_rg_keys()
 ****************************************************************
 * @brief
 *  Set a new set of keys on a raid group.
 *
 * @param rg_config_p - config to corrupt keys in.
 * @param ds_list_p - Downstream list for R10.
 * @param inject_position - Position to inject on or FBE_U32_MAX to inject on all.
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/5/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_kms_set_rg_keys(fbe_test_rg_configuration_t * rg_config_p,
                                      fbe_api_base_config_downstream_object_list_t *ds_list_p,
                                      fbe_kms_control_set_keys_t *set_keys_p)
{
    fbe_status_t status;
    fbe_u32_t disk_index;
    fbe_object_id_t rg_object_id;
    fbe_u32_t i;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        for (disk_index = 0; 
            disk_index < rg_config_p->width / 2; 
            disk_index++) {
            for (i = 0; i < 2; i++) {
                set_keys_p->b_set_key[i] = FBE_TRUE;
            }
            fbe_test_sep_util_kms_set_rg_keys(set_keys_p);
            fbe_test_sep_util_kms_push_rg_keys(set_keys_p);
            set_keys_p++;
        }
    } else {

        for (i = 0; i < rg_config_p->width; i++) {
            set_keys_p->b_set_key[i] = FBE_TRUE;
        }
        fbe_test_sep_util_kms_set_rg_keys(set_keys_p);
        fbe_test_sep_util_kms_push_rg_keys(set_keys_p);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_kms_set_rg_keys()
 ******************************************/

/*!**************************************************************
 * fbe_test_kms_get_rg_keys()
 ****************************************************************
 * @brief
 *  Corrupt the keys on a raid group in KMS.
 *
 * @param rg_config_p - config to corrupt keys in.
 * @param ds_list_p - Downstream list for R10.
 * @param inject_position - Position to inject on or FBE_U32_MAX to inject on all.
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/5/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_kms_get_rg_keys(fbe_test_rg_configuration_t * rg_config_p,
                                      fbe_api_base_config_downstream_object_list_t *ds_list_p,
                                      fbe_kms_control_set_keys_t *set_keys_p)
{
    fbe_status_t status;
    fbe_u32_t disk_index;
    fbe_kms_control_get_keys_t get_keys;
    fbe_object_id_t rg_object_id;
    fbe_u32_t i;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        for (disk_index = 0; 
            disk_index < rg_config_p->width / 2; 
            disk_index++) {

            /* Get keys to get the serial numbers. 
             */
            get_keys.object_id = ds_list_p->downstream_object_list[disk_index];
            get_keys.control_number = rg_config_p->raid_group_id;
            fbe_test_sep_util_kms_get_rg_keys(&get_keys);

            set_keys_p->object_id = get_keys.object_id;
            set_keys_p->control_number = get_keys.control_number;
            set_keys_p->width = 2;

            for (i = 0; i <= 2; i++) {
                set_keys_p->drive[i].key_mask = get_keys.drive[i].key_mask;
                fbe_copy_memory(set_keys_p->drive[i].serial_num,
                                get_keys.drive[i].serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
                fbe_copy_memory(&set_keys_p->key1[i][0], &get_keys.key1[i][0], FBE_DEK_SIZE);
                fbe_copy_memory(&set_keys_p->key2[i][0], &get_keys.key2[i][0], FBE_DEK_SIZE);
            }
            set_keys_p++;
        }
    } else {
        /* Get keys to get the serial numbers. 
         */
        get_keys.object_id = rg_object_id;
        get_keys.control_number = rg_config_p->raid_group_id;
        fbe_test_sep_util_kms_get_rg_keys(&get_keys);
        set_keys_p->object_id = get_keys.object_id;
        set_keys_p->control_number = get_keys.control_number;
        set_keys_p->width = rg_config_p->width;

        for (i = 0; i <= FBE_RAID_MAX_DISK_ARRAY_WIDTH; i++) {
            set_keys_p->drive[i].key_mask = get_keys.drive[i].key_mask;
            fbe_copy_memory(set_keys_p->drive[i].serial_num,
                            get_keys.drive[i].serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
            fbe_copy_memory(&set_keys_p->key1[i][0], &get_keys.key1[i][0], FBE_DEK_SIZE);
            fbe_copy_memory(&set_keys_p->key2[i][0], &get_keys.key2[i][0], FBE_DEK_SIZE);
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_kms_get_rg_keys()
 ******************************************/
/*!****************************************************************************
 *  ichabod_check_incorrect_key_log_present
 ******************************************************************************
 * @brief
 *  Check that the event log around incorrect keys is present.
 *
 * @param pvd_object_id - lun object id
 *
 * @author
 *  3/13/2014 - Created. Rob Foley
 *
 *****************************************************************************/
static void ichabod_check_incorrect_key_log_present(fbe_object_id_t pvd_object_id,
                                                    fbe_u32_t bus,
                                                    fbe_u32_t enc,
                                                    fbe_u32_t slot,
                                                    fbe_u8_t* serial_num_p)
{
    fbe_status_t                        status;               
    fbe_bool_t                          is_msg_present = FBE_FALSE;  

    /* check that given event message is logged correctly */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                         &is_msg_present, 
                                         SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY, 
                                         bus, enc, slot, pvd_object_id, serial_num_p);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (!is_msg_present) {
        mut_printf(MUT_LOG_TEST_STATUS, "== event log not found for %d_%d_%d object_id: 0x%x sn: %s",
                   bus, enc, slot, pvd_object_id, serial_num_p);
        MUT_FAIL_MSG(" not found as expected");
    } else {
        mut_printf(MUT_LOG_TEST_STATUS, "== event log found for %d_%d_%d object_id: 0x%x sn: %s",
                   bus, enc, slot, pvd_object_id, serial_num_p);
    }
}
/*******************************************************
 * end ichabod_check_incorrect_key_log_present()
 *******************************************************/
/*!**************************************************************
 * ichabod_incorrect_key_test()
 ****************************************************************
 * @brief
 *  The raid group is already encrypted.
 *  Reboot.
 *  Hook KMS.
 *  Corrupt keys in KMS.  
 *  Remove hook so wrong keys are pushed.
 *  Make sure PVD detects incorrect keys.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/3/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_incorrect_key_test(fbe_test_rg_configuration_t * rg_config_p,
                                fbe_api_rdgen_peer_options_t peer_options,
                                fbe_data_pattern_t background_pattern,
                                fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t object_id_array[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_kms_control_set_keys_t *keys[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_api_base_config_downstream_object_list_t *ds_list_p[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_kms_package_load_params_t kms_params;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_api_provision_drive_info_t pvd_info[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    
    fbe_test_sep_get_active_target_id(&active_sp);

    /* @todo Skip over the first rg for now. 
     * We don't handle system drive key validation yet. 
     */
    //rg_config_p++;
    fbe_test_get_top_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern....Success");
    
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t pvd_id;
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        keys[rg_index] = fbe_api_allocate_memory(sizeof(fbe_kms_control_set_keys_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            ds_list_p[rg_index] = fbe_api_allocate_memory(sizeof(fbe_api_base_config_downstream_object_list_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

            fbe_test_sep_util_get_ds_object_list(rg_object_ids[rg_index], ds_list_p[rg_index]);
        } 
        
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            object_id_array[rg_index][disk_index] = pvd_id;
            status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info[rg_index][disk_index]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Reboot the SP.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(&ichabod_raid_group_config_qual[0], active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add debug hooks in PVDs to log when incorrect keys are detected ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            
            status = fbe_api_scheduler_add_debug_hook(object_id_array[rg_index][disk_index],
                                                      SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                                      FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Using hook at load time in KMS ==");
    kms_params.flags = 0;
    fbe_zero_memory(&kms_params, sizeof(fbe_kms_package_load_params_t));
    kms_params.hooks[0].state = FBE_KMS_HOOK_STATE_LOAD;
    sep_config_load_kms(&kms_params);

    /* Wait until we hit the hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for kms hook ==.");
    fbe_test_util_wait_for_kms_hook(FBE_KMS_HOOK_STATE_LOAD, FBE_TEST_WAIT_TIMEOUT_MS);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_test_kms_get_rg_keys(current_rg_config_p, ds_list_p[rg_index], keys[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Corrupt the KMS Keys so the PVDs find incorrect keys ==.");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_test_kms_corrupt_rg_keys(current_rg_config_p, ds_list_p[rg_index], FBE_U32_MAX);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs to be specialize. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to specialize object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to specialize object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Clear KMS load hook ==");
    fbe_test_sep_util_clear_kms_hook(FBE_KMS_HOOK_STATE_LOAD);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for PVDs to detect incorrect keys. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_test_wait_for_debug_hook(object_id_array[rg_index][disk_index],
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS, 
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                                  0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_scheduler_del_debug_hook(object_id_array[rg_index][disk_index],
                                                      SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                                      FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            ichabod_check_incorrect_key_log_present(object_id_array[rg_index][disk_index],
                                                    current_rg_config_p->rg_disk_set[disk_index].bus,
                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                    &pvd_info[rg_index][disk_index].serial_num[0]);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Push old keys, so raid group comes ready==");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_test_kms_set_rg_keys(current_rg_config_p, ds_list_p[rg_index], keys[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs and RGs to be READY. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        fbe_u32_t lun_index;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++) {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to ready object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, " == Validate pattern ==");
    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_api_free_memory( keys[rg_index]);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_free_memory(ds_list_p[rg_index]);
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end ichabod_incorrect_key_test()
 ******************************************/

/*!**************************************************************
 * ichabod_incorrect_key_one_drive_test()
 ****************************************************************
 * @brief
 *  The raid group is already encrypted.
 *  Reboot.
 *  Hook KMS.
 *  Corrupt 1 key per rg in KMS.
 *  Remove hook so wrong keys are pushed.
 *  Make sure PVD detects incorrect keys.
 *  Make sure RG is degraded.
 *  Reboot SP
 *  Make sure RG rebuild finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_incorrect_key_one_drive_test(fbe_test_rg_configuration_t * rg_config_p,
                                          fbe_api_rdgen_peer_options_t peer_options,
                                          fbe_data_pattern_t background_pattern,
                                          fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t object_id_array[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_api_base_config_downstream_object_list_t *ds_list_p[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_kms_package_load_params_t kms_params;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    
    fbe_test_sep_get_active_target_id(&active_sp);

    /* @todo Skip over the first rg for now. 
     * We don't handle system drive key validation yet. 
     */
    //rg_config_p++;
    fbe_test_get_top_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern....Success");
    
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t pvd_id;
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            ds_list_p[rg_index] = fbe_api_allocate_memory(sizeof(fbe_api_base_config_downstream_object_list_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

            fbe_test_sep_util_get_ds_object_list(rg_object_ids[rg_index], ds_list_p[rg_index]);
        } 
        
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            object_id_array[rg_index][disk_index] = pvd_id;
        }
        current_rg_config_p++;
    }

    /* Reboot the SP.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(&ichabod_raid_group_config_qual[0], active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add debug hooks in PVDs to log when incorrect keys are detected ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        disk_index = 0;
        status = fbe_api_scheduler_add_debug_hook(object_id_array[rg_index][disk_index],
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Using hook at load time in KMS ==");
    kms_params.flags = 0;
    fbe_zero_memory(&kms_params, sizeof(fbe_kms_package_load_params_t));
    kms_params.hooks[0].state = FBE_KMS_HOOK_STATE_LOAD;
    sep_config_load_kms(&kms_params);

    /* Wait until we hit the hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for kms hook ==.");
    fbe_test_util_wait_for_kms_hook(FBE_KMS_HOOK_STATE_LOAD, FBE_TEST_WAIT_TIMEOUT_MS);

    mut_printf(MUT_LOG_TEST_STATUS, "== Corrupt the KMS Keys so the PVDs find incorrect keys ==.");

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_test_kms_corrupt_rg_keys(current_rg_config_p, ds_list_p[rg_index], 0);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs to be specialize. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to specialize object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to specialize object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Clear KMS load hook ==");
    fbe_test_sep_util_clear_kms_hook(FBE_KMS_HOOK_STATE_LOAD);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for PVDs to detect incorrect keys. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        disk_index = 0;
        status = fbe_test_wait_for_debug_hook(object_id_array[rg_index][disk_index],
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                              FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS, 
                                              SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                              0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_scheduler_del_debug_hook(object_id_array[rg_index][disk_index],
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs to be ready. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to specialize object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to specialize object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        fbe_test_sep_rebuild_validate_rg_is_degraded(current_rg_config_p, 1, 0/* pos */);
        current_rg_config_p++;
    }

    /* Reboot the SP so we read back in the correct keys.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(&ichabod_raid_group_config_qual[0], active_sp);
    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs and RGs to be READY. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        fbe_u32_t lun_index;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++) {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to ready object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_test_sep_util_add_removed_position(current_rg_config_p, 0);
        fbe_test_sep_util_removed_position_inserted(current_rg_config_p, 0);
        current_rg_config_p++;
    }
    /* Wait for rebuild to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, num_raid_groups, 1);
    mut_printf(MUT_LOG_TEST_STATUS, " == Make sure encryption proceeds.");

    mut_printf(MUT_LOG_TEST_STATUS, " == Validate pattern ==");
    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_incorrect_key_one_drive_test()
 ******************************************/

void ichabod_enable_key_error_injection(fbe_test_protocol_error_type_t err_type,
                                        fbe_payload_block_operation_opcode_t opcode,
                                        fbe_object_id_t pdo_id,
                                        fbe_bool_t b_user_area,
                                        fbe_object_id_t first_pvd_id,
                                        fbe_protocol_error_injection_error_record_t     *record,
                                        fbe_protocol_error_injection_record_handle_t *record_handle)
{
    fbe_status_t status;
    fbe_port_request_status_t port_status;
    fbe_lba_t lba;
    fbe_lba_t end_lba;
    fbe_api_provision_drive_info_t pvd_info;

     if (b_user_area || (first_pvd_id > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST)) {
        status = fbe_private_space_layout_get_start_of_user_space(FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, &lba);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } else {
        /* Inject an error at the beginning of the validation area, since this is where we will try to read/write.
         */
        status = fbe_api_provision_drive_get_info(first_pvd_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        lba = pvd_info.paged_metadata_lba + pvd_info.paged_mirror_offset;
    }

    /* Convert the lba if required.
     */
    status = fbe_test_neit_utils_convert_to_physical_drive_range(pdo_id,
                                                                 lba,
                                                                 (lba + 0x40),
                                                                 &lba,
                                                                 &end_lba);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set up the error injection */
    record->object_id = pdo_id;
    record->lba_start = lba;
    record->lba_end = FBE_LBA_INVALID;  /* Inject to entire consumed range. */
    record->num_of_times_to_insert = (fbe_u32_t)FBE_LBA_INVALID;

    record->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT;
    record->protocol_error_injection_error.protocol_error_injection_port_error.check_ranges = FBE_TRUE;
    record->protocol_error_injection_error.protocol_error_injection_port_error.scsi_command[0] = 
    fbe_test_sep_util_get_scsi_command(opcode);
    switch(err_type) {
         case FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE:
                port_status = FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE;
                break;
            case FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR:
                port_status = FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR;
                break;
            case FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED:
                port_status = FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED;
                break;
        default:
            MUT_FAIL();
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Enable error injection pdo obj: 0x%x lba: 0x%llx==", pdo_id, lba);
    record ->protocol_error_injection_error.protocol_error_injection_port_error.port_status = port_status;
    status = fbe_api_protocol_error_injection_add_record(record, record_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

void ichabod_disable_injection(fbe_object_id_t pdo_id, 
                               fbe_protocol_error_injection_record_handle_t *record_handle)
{
    fbe_status_t status;
    
    status = fbe_api_protocol_error_injection_remove_record(record_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_remove_object(pdo_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

void ichabod_create_rg(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t num_raid_groups)
{ 
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t    rg_index = 0;

    /* go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC) {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(rg_config_p);
        }
        rg_config_p->b_create_this_pass = FBE_TRUE;
        if (!fbe_test_rg_config_is_enabled(rg_config_p)) {
            rg_config_p++;
            continue;
        }
        fbe_test_sep_util_wait_for_pvd_creation(rg_config_p->rg_disk_set,rg_config_p->width,10000);

        /* Create a RAID group with user provided configuration. */
        status = fbe_test_sep_util_create_raid_group(rg_config_p);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of RAID group failed.");

        rg_config_p++;
    }
}

fbe_u32_t ichabod_get_monitor_substate(fbe_test_protocol_error_type_t err_type)
{
    fbe_u32_t monitor_substate = 0;
    switch (err_type) {
        case FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE:
            monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_BAD_KEY_HANDLE;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR:
            monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_KEY_WRAP_ERROR;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED:
            monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_NOT_ENABLED;
            break;
        default:
            MUT_FAIL_MSG("unknown error type");
            break;
    };
    return monitor_substate;
}
/*!**************************************************************
 * ichabod_key_error_create_test_rg()
 ****************************************************************
 * @brief
 *  Create an encrypted raid group.
 *  Inject a error related to encryption for all drives.
 *  Ensure that the error is hit during RG create
 *  Force keys to get pushed again. (this causes the PVD encryption state to get cleared)
 *  Ensure the RG comes online.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/5/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_key_error_create_test_rg(fbe_test_rg_configuration_t * rg_config_p,
                                      fbe_test_protocol_error_type_t err_type,
                                      fbe_u32_t num_raid_groups)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t object_id_array[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_kms_control_set_keys_t *keys[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_api_base_config_downstream_object_list_t *ds_list_p[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t monitor_substate;
    fbe_protocol_error_injection_error_record_t     record[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_protocol_error_injection_record_handle_t record_handle[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_object_id_t pdo_id;

    fbe_test_sep_get_active_target_id(&active_sp);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting error type %d==", __FUNCTION__, err_type);

    monitor_substate = ichabod_get_monitor_substate(err_type);
    fbe_api_protocol_error_injection_start();

    current_rg_config_p = rg_config_p;

    if (err_type == FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {

        status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                               FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                               1,    /* Num errors. */
                                               FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_util_reduce_sector_trace_level();
        fbe_test_sep_util_disable_trace_limits();
        status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t pvd_id;
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        keys[rg_index] = fbe_api_allocate_memory(sizeof(fbe_kms_control_set_keys_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);
        
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                    &pvd_id);
            status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                      current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                      current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            object_id_array[rg_index][disk_index] = pvd_id;
            /* Initialize the error injection record.
             */
            fbe_test_neit_utils_init_error_injection_record(&record[rg_index][disk_index]);

            ichabod_enable_key_error_injection(err_type, 
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                               pdo_id,
                                               FBE_FALSE,
                                               object_id_array[0][0],
                                               &record[rg_index][disk_index],
                                               &record_handle[rg_index][disk_index]);
            mut_printf(MUT_LOG_TEST_STATUS, "Enabled injection on %d_%d_%d. PDO:0x%08x. Handle:0x%llx\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot,
                       pdo_id,
                      (unsigned long long) record_handle[rg_index][disk_index]);

        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Add debug hooks in PVDs to log when incorrect keys are detected ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        fbe_u32_t monitor_state;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        if (current_rg_config_p->rg_disk_set[0].bus == 0 &&
            current_rg_config_p->rg_disk_set[0].enclosure == 0 && 
            current_rg_config_p->rg_disk_set[0].slot == 0) {
            monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_INIT_VALIDATION_AREA;
        } else {
            monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            
            status = fbe_api_scheduler_add_debug_hook(object_id_array[rg_index][disk_index],
                                                      monitor_state,
                                                      monitor_substate,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Create RGs. ==");
    ichabod_create_rg(rg_config_p, num_raid_groups);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RGs to be in specialize. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t rg_object_id;
        fbe_u32_t total_wait_ms = 0;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        /* Wait for RGs to exist. */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        while (status != FBE_STATUS_OK) {
            fbe_api_sleep(1000);
            total_wait_ms += 1000;
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            if (total_wait_ms > FBE_TEST_WAIT_TIMEOUT_MS) {
                mut_printf(MUT_LOG_TEST_STATUS, "!! raid group still does not exist in %d msec", FBE_TEST_WAIT_TIMEOUT_MS);
                MUT_FAIL();
            }
        }
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
        rg_object_ids[rg_index] = rg_object_id;

        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to specialize object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            ds_list_p[rg_index] = fbe_api_allocate_memory(sizeof(fbe_api_base_config_downstream_object_list_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

            fbe_test_sep_util_get_ds_object_list(rg_object_ids[rg_index], ds_list_p[rg_index]);
        } 
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for PVDs to detect incorrect keys. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;fbe_u32_t monitor_state;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        if (current_rg_config_p->rg_disk_set[0].bus == 0 &&
            current_rg_config_p->rg_disk_set[0].enclosure == 0 && 
            current_rg_config_p->rg_disk_set[0].slot == 0) {
            /* System drives use a different code path. 
             * They remain ready and init the validation area on the key push.
             */
            monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_INIT_VALIDATION_AREA;
        } else {
            monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_test_wait_for_debug_hook(object_id_array[rg_index][disk_index],
                                                  monitor_state,
                                                  monitor_substate,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                                  0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_scheduler_del_debug_hook(object_id_array[rg_index][disk_index],
                                                      monitor_state,
                                                      monitor_substate,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    if (err_type == FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {
        fbe_api_trace_error_counters_t error_counters;
        fbe_test_wait_for_critical_error();
        status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
        mut_printf(MUT_LOG_TEST_STATUS, "== Found %d critical errors ==", 
                   error_counters.trace_critical_error_counter);
        MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_critical_error_counter, 0);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Disable injection ==");
    current_rg_config_p = rg_config_p;
     for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                      current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                      current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "Disable injection on %d_%d_%d. PDO:0x%08x. Handle:0x%llx\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot,
                       pdo_id,
                       (unsigned long long)record_handle[rg_index][disk_index]);
            ichabod_disable_injection(pdo_id, record_handle[rg_index][disk_index]);
        }
        current_rg_config_p++;
     }
    
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if ( (err_type != FBE_API_LOGICAL_ERROR_INJECTION_TYPE_KEY_ERROR) &&
         (rg_config_p->rg_disk_set[0].bus == 0) &&
         (rg_config_p->rg_disk_set[0].enclosure == 0) && 
         (rg_config_p->rg_disk_set[0].slot == 0) ) {
        /* When we use system drives the PVDs will not come back. 
         * This is as designed and either a reboot or destroy should clear the state.  
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Skip to destroy phase. ");
    }else {
        mut_printf(MUT_LOG_TEST_STATUS, "== Push old keys, so raid group comes ready==");
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {
    
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
                current_rg_config_p++;
                continue;
            }
            fbe_test_kms_get_rg_keys(current_rg_config_p, ds_list_p[rg_index], keys[rg_index]);
            fbe_test_kms_set_rg_keys(current_rg_config_p, ds_list_p[rg_index], keys[rg_index]);
            current_rg_config_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs and RGs to be READY. == ");
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            fbe_object_id_t rg_object_id;

            if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
                current_rg_config_p++;
                continue;
            }

            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to be ready object id: 0x%x", __FUNCTION__, rg_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            current_rg_config_p++;
        }
    }

    /* Make sure no errors occurred.
     */
    if (err_type != FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {
        fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    }
    if (err_type == FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {
        /* clear out the error counters since we intentionally injected critical errors.
         */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_NEIT);

        /* Enable error handling so that we panic on unload if needed.
         */
        status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                               FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                               1,    /* Num errors. */
                                               FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Destroy raid groups.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_sep_util_destroy_raid_group(current_rg_config_p->raid_group_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy RAID group failed.");
        current_rg_config_p++;
    }
    /* Wait for PVDs to be ready again.  Since next test will want to create RGs on these.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            mut_printf(MUT_LOG_TEST_STATUS, "Wait for pvd object 0x%x (%d_%d_%d) to be ready\n",
                       object_id_array[rg_index][disk_index],
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot);
            status = fbe_api_wait_for_object_lifecycle_state(object_id_array[rg_index][disk_index], FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_api_free_memory( keys[rg_index]);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_free_memory(ds_list_p[rg_index]);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ichabod_key_error_create_test_rg()
 ******************************************/
/*!**************************************************************
 * ichabod_key_error_io_test_case()
 ****************************************************************
 * @brief
 *  Create an encrypted raid group.
 *  Inject a error related to encryption for all drives.
 *  Run I/O.
 *  Ensure that the error is hit.
 *  Force keys to get pushed again.
 *  Ensure the RG comes online.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_key_error_io_test_case(fbe_test_rg_configuration_t * rg_config_p,
                                    fbe_api_logical_error_injection_type_t err_type,
                                    fbe_u32_t num_raid_groups)
{
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t object_id_array[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t monitor_substate;
    fbe_u32_t num_luns;
    fbe_api_trace_error_counters_t error_counters;
    fbe_protocol_error_injection_error_record_t     record[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_protocol_error_injection_record_handle_t record_handle[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_object_id_t pdo_id;

    fbe_test_sep_get_active_target_id(&active_sp);

    monitor_substate = ichabod_get_monitor_substate(err_type);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    fbe_api_protocol_error_injection_start();
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting for err type:%d ==", __FUNCTION__, err_type);
    
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Disable critical errors ==");
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_test_sep_util_disable_trace_limits();
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t pvd_id;
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        
        for (disk_index = 1; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                    &pvd_id);
            status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                      current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                      current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            object_id_array[rg_index][disk_index] = pvd_id;
            mut_printf(MUT_LOG_TEST_STATUS, "Enable injection on %d_%d_%d\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot);
            /* Initialize the error injection record.
             */
            fbe_test_neit_utils_init_error_injection_record(&record[rg_index][disk_index]);

            ichabod_enable_key_error_injection(err_type, 
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                               pdo_id,
                                               FBE_TRUE,
                                               object_id_array[0][0],
                                               &record[rg_index][disk_index],
                                               &record_handle[rg_index][disk_index]);
             mut_printf(MUT_LOG_TEST_STATUS, "Enabled injection on %d_%d_%d. PDO:0x%08x. Handle:0x%llx\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot,
                       pdo_id,
                      (unsigned long long)record_handle[rg_index][disk_index]);
            break;
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Add debug hooks in PVDs to log when incorrect keys are detected ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 1; disk_index < current_rg_config_p->width; disk_index++) {
            
            status = fbe_api_scheduler_add_debug_hook(object_id_array[rg_index][disk_index],
                                                      SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION_IO,
                                                      monitor_substate,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Start I/O ==");

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, 2, /* threads */ 1 /* blocks */, 
                      FBE_RDGEN_OPERATION_READ_ONLY,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, FBE_RDGEN_PEER_OPTIONS_INVALID);
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for PVDs to detect incorrect keys. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 1; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_test_wait_for_debug_hook(object_id_array[rg_index][disk_index],
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION_IO,
                                                  monitor_substate,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                                  0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_scheduler_del_debug_hook(object_id_array[rg_index][disk_index],
                                                      SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION_IO,
                                                      monitor_substate,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
        }
        current_rg_config_p++;
    }
    fbe_test_wait_for_critical_error();
    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_critical_error_counter, 0);

     mut_printf(MUT_LOG_TEST_STATUS, "== Disable injection ==");
     current_rg_config_p = rg_config_p;
     for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 1; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                      current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                      current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
             mut_printf(MUT_LOG_TEST_STATUS, "Disable injection on %d_%d_%d. PDO:0x%08x. Handle:0x%llx\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot,
                       pdo_id,
                      (unsigned long long) record_handle[rg_index][disk_index]);
            ichabod_disable_injection(pdo_id, record_handle[rg_index][disk_index]);
            break;
        }
        current_rg_config_p++;
     }
    
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs and RGs to be READY. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        fbe_u32_t lun_index;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++) {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            status = fbe_api_lun_clear_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to ready object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    /* clear out the error counters since we intentionally injected critical errors.
     */
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_NEIT);

    /* Enable the critical error handling so that we panic on unload if needed.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end ichabod_key_error_io_test_case()
 ******************************************/

/*!**************************************************************
 * ichabod_key_error_io_test_rg()
 ****************************************************************
 * @brief
 *  The raid group is already encrypted.
 *  Enable error injection.
 *  Run I/O.
 *  Make sure PVD detects key errors.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_key_error_io_test_rg(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t status;
    fbe_u32_t num_raid_groups;
    rg_config_p = &ichabod_raid_group_config_qual[0];
    fbe_test_sep_util_reduce_sector_trace_level();

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    rg_config_p->b_create_this_pass = FBE_TRUE;

    num_raid_groups = 1;
    ichabod_key_error_io_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR, num_raid_groups);
    ichabod_key_error_io_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED, num_raid_groups);
    ichabod_key_error_io_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE, num_raid_groups);

    rg_config_p++;
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p) - 1;
    ichabod_key_error_io_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR, num_raid_groups);
    ichabod_key_error_io_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED, num_raid_groups);
    ichabod_key_error_io_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE, num_raid_groups);
}
/**************************************
 * end ichabod_key_error_io_test_rg()
 **************************************/
/*!**************************************************************
 * ichabod_validation_key_error_test_case()
 ****************************************************************
 * @brief
 *  The raid group is already encrypted.
 *  Reboot.
 *  Hook KMS.
 *  Inject errors on PVD.
 *  Remove hook KMS continues.
 *  Make sure PVD detects key errors.
 *
 * @param rg_config_p - config to run test against.
 * @param err_type - error to inject
 *
 * @return None.   
 *
 * @author
 *  3/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_validation_key_error_test_case(fbe_test_rg_configuration_t *rg_config_p, 
                                            fbe_test_protocol_error_type_t err_type,
                                            fbe_u32_t num_raid_groups)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_api_rdgen_peer_options_t peer_options = FBE_RDGEN_PEER_OPTIONS_INVALID;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_object_id_t object_id_array[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_kms_control_set_keys_t *keys[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_api_base_config_downstream_object_list_t *ds_list_p[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_kms_package_load_params_t kms_params;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_data_pattern_t background_pattern = FBE_DATA_PATTERN_LBA_PASS;
    fbe_data_pattern_t pattern = FBE_DATA_PATTERN_LBA_PASS;
    fbe_u32_t monitor_substate;
    fbe_protocol_error_injection_error_record_t     record[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_protocol_error_injection_record_handle_t record_handle[FBE_TEST_MAX_RAID_GROUP_COUNT][FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_object_id_t pdo_id;

    monitor_substate = ichabod_get_monitor_substate(err_type);
    
    fbe_test_sep_get_active_target_id(&active_sp);

    fbe_test_get_top_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    fbe_api_protocol_error_injection_start();

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting error type %d==", __FUNCTION__, err_type);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, " == Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, " == Read pattern....Success");
    
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t pvd_id;
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        keys[rg_index] = fbe_api_allocate_memory(sizeof(fbe_kms_control_set_keys_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            ds_list_p[rg_index] = fbe_api_allocate_memory(sizeof(fbe_api_base_config_downstream_object_list_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

            fbe_test_sep_util_get_ds_object_list(rg_object_ids[rg_index], ds_list_p[rg_index]);
        } 
        
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                    current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                    current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            object_id_array[rg_index][disk_index] = pvd_id;
        }
        current_rg_config_p++;
    }

    /* Reboot the SP.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(&ichabod_raid_group_config_qual[0], active_sp);

    fbe_api_protocol_error_injection_start();

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error injection ==");
    
    if (err_type == FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {

        mut_printf(MUT_LOG_TEST_STATUS, "== Disable critical errors ==");
        status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                               FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                               1,    /* Num errors. */
                                               FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_util_reduce_sector_trace_level();
        fbe_test_sep_util_disable_trace_limits();
        status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Add debug hooks in PVDs to log when incorrect keys are detected ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            
            status = fbe_api_scheduler_add_debug_hook(object_id_array[rg_index][disk_index],
                                                      SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VALIDATE_KEYS,
                                                      monitor_substate,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            
            status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                      current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                      current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
          
            /* Initialize the error injection record.
             */
            fbe_test_neit_utils_init_error_injection_record(&record[rg_index][disk_index]);

            ichabod_enable_key_error_injection(err_type, 
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                               pdo_id,
                                               FBE_FALSE,
                                               object_id_array[0][0],
                                               &record[rg_index][disk_index],
                                               &record_handle[rg_index][disk_index]);
             mut_printf(MUT_LOG_TEST_STATUS, "Enabled injection on %d_%d_%d. PDO:0x%08x. Handle:0x%llx\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot,
                       pdo_id,
                      (unsigned long long) record_handle[rg_index][disk_index]);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Using hook at load time in KMS ==");
    kms_params.flags = 0;
    fbe_zero_memory(&kms_params, sizeof(fbe_kms_package_load_params_t));
    kms_params.hooks[0].state = FBE_KMS_HOOK_STATE_LOAD;
    sep_config_load_kms(&kms_params);

    /* Wait until we hit the hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for kms hook ==.");
    fbe_test_util_wait_for_kms_hook(FBE_KMS_HOOK_STATE_LOAD, FBE_TEST_WAIT_TIMEOUT_MS);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_test_kms_get_rg_keys(current_rg_config_p, ds_list_p[rg_index], keys[rg_index]);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs to be specialize. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to specialize object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to specialize object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_SPECIALIZE,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Clear KMS load hook ==");
    fbe_test_sep_util_clear_kms_hook(FBE_KMS_HOOK_STATE_LOAD);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for PVDs to detect incorrect keys. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_test_wait_for_debug_hook(object_id_array[rg_index][disk_index],
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VALIDATE_KEYS,
                                                  monitor_substate, 
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                                  0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_scheduler_del_debug_hook(object_id_array[rg_index][disk_index],
                                                      SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VALIDATE_KEYS,
                                                      monitor_substate, 
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    if (err_type == FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {
        fbe_api_trace_error_counters_t error_counters;
        fbe_test_wait_for_critical_error();
        status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
        MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_critical_error_counter, 0);
    }

     mut_printf(MUT_LOG_TEST_STATUS, "== Disable injection ==");
     current_rg_config_p = rg_config_p;
     for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_u32_t disk_index;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (disk_index = 0; disk_index < current_rg_config_p->width; disk_index++) {
            status = fbe_api_get_physical_drive_object_id_by_location(current_rg_config_p->rg_disk_set[disk_index].bus,
                                                                      current_rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                      current_rg_config_p->rg_disk_set[disk_index].slot,
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
             mut_printf(MUT_LOG_TEST_STATUS, "Disabling injection on %d_%d_%d. PDO:0x%08x. Handle:0x%llx\n",
                       current_rg_config_p->rg_disk_set[disk_index].bus,
                       current_rg_config_p->rg_disk_set[disk_index].enclosure,
                       current_rg_config_p->rg_disk_set[disk_index].slot,
                       pdo_id,
                       (unsigned long long)record_handle[rg_index][disk_index]);
            ichabod_disable_injection(pdo_id, record_handle[rg_index][disk_index]);
        }
        current_rg_config_p++;
     }
     if ( (err_type != FBE_API_LOGICAL_ERROR_INJECTION_TYPE_KEY_ERROR) &&
         (rg_config_p->rg_disk_set[0].bus == 0) &&
         (rg_config_p->rg_disk_set[0].enclosure == 0) && 
         (rg_config_p->rg_disk_set[0].slot == 0) ) {

        fbe_sim_transport_connection_target_t active_sp;
        fbe_test_sep_get_active_target_id(&active_sp);
        /* We will not come ready in this case, just reboot.
         * The reason is that these error cases are not automatically recoverable, we expect a reboot to clear these. 
         */
        mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        fbe_api_sim_transport_destroy_client(active_sp);
        fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

        mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        fbe_test_boot_sp(rg_config_p, active_sp);

        status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        sep_config_load_kms(NULL);
    }else {
        mut_printf(MUT_LOG_TEST_STATUS, "== Push old keys, so raid group comes ready==");
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < num_raid_groups; rg_index++) {
    
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
                current_rg_config_p++;
                continue;
            }
            fbe_test_kms_set_rg_keys(current_rg_config_p, ds_list_p[rg_index], keys[rg_index]);
            current_rg_config_p++;
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for LUNs and RGs to be READY. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_object_id_t lun_object_id;
        fbe_object_id_t rg_object_id;
        fbe_u32_t lun_index;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++) {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for rg object to ready object id: 0x%x", __FUNCTION__, rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, " == Validate pattern ==");
    ichabod_crane_read_pattern(read_context_p, num_luns);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_api_free_memory( keys[rg_index]);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_free_memory(ds_list_p[rg_index]);
        }
        current_rg_config_p++;
    }
    if (err_type == FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE) {
        /* clear out the error counters since we intentionally injected critical errors.
         */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_NEIT);
        /* Enable error handling so that we panic on unload if needed.
         */
        status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                               FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                               1,    /* Num errors. */
                                               FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return;
}
/******************************************
 * end ichabod_validation_key_error_test_case()
 ******************************************/

/*!**************************************************************
 * ichabod_validation_key_error_test_rg()
 ****************************************************************
 * @brief
 *  The raid group is already encrypted.
 *  Reboot.
 *  Hook KMS.
 *  Inject errors on PVD.
 *  Remove hook KMS continues.
 *  Make sure PVD detects key errors.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_validation_key_error_test_rg(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t status;
    fbe_u32_t num_raid_groups;
    fbe_test_sep_util_reduce_sector_trace_level();

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    ichabod_validation_key_error_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR, num_raid_groups);
    ichabod_validation_key_error_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED, num_raid_groups);
    ichabod_validation_key_error_test_case(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE, num_raid_groups);
}
/**************************************
 * end ichabod_validation_key_error_test_rg()
 **************************************/

static void ichabod_crane_read_pattern(fbe_api_rdgen_context_t *read_context_p,
                                       fbe_u32_t num_luns)
{
    fbe_status_t status;
    fbe_u32_t index;

    status = fbe_api_rdgen_context_reinit(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change all contexts to do one pass.
     * We need to to a single read/check since it is not guaranteed that all tests made it through one pass 
     * before we stopped. 
     */
    for (index = 0; index < num_luns; index++) {
        fbe_api_rdgen_io_specification_set_max_passes(&read_context_p[index].start_io.specification, 1);
    }

    /* Run our read/check I/O test.  This ensures we run the entire read/check after the rekey is complete.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/*!**************************************************************
 * ichabod_check_pattern_during_encryption_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run encryption.
 *  During encryption pause it at different points and read back
 *  the pattern.  This tests various spanning cases.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_check_pattern_during_encryption_test(fbe_test_rg_configuration_t * const rg_config_p,
                                                  fbe_api_rdgen_peer_options_t peer_options,
                                                  fbe_data_pattern_t background_pattern,
                                                  fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;
    fbe_u32_t test_points;
    fbe_lba_t lba;
    fbe_bool_t b_io_active = (peer_options == FBE_RDGEN_PEER_OPTIONS_INVALID);
    #define ICHABOD_CRANE_INCREMENT_CHECKPOINT 0x800
    #define ICHABOD_CRANE_MAX_TEST_POINTS 5

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        ICHABOD_MAX_IO_SIZE_BLOCKS);

    fbe_test_setup_active_region_context(rg_config_p, read_context_p, num_luns, b_io_active);
    fbe_test_setup_active_region_context(rg_config_p, write_context_p, num_luns, b_io_active);
    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.");
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);
    mut_printf(MUT_LOG_TEST_STATUS, "Write pattern.....Success");

    /* Make sure the patterns are still there.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern.");
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Read pattern....Success");

    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);

    /* kick off a rekey.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Start");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Complete");

    /* Allow the rekey to proceed and check the pattern at each step.
     */
    lba = 0;
    for (test_points = 0; test_points < ICHABOD_CRANE_MAX_TEST_POINTS; test_points++) {
        fbe_lba_t prev_lba = 0;
        mut_printf(MUT_LOG_TEST_STATUS, "Run I/O across checkpoint at lba: 0x%llx", lba);
        /* Wait for debug hook to get hit.
         */
        fbe_test_sep_use_pause_rekey_hooks(rg_config_p, lba, FBE_TEST_HOOK_ACTION_WAIT);

        /* Check the pattern.
         */
        ichabod_crane_read_pattern(read_context_p, num_luns);

        /* Setup the next hook and remove the old hook.
         */
        prev_lba = lba;
        lba += ICHABOD_CRANE_INCREMENT_CHECKPOINT;
        if (test_points + 1 < ICHABOD_CRANE_MAX_TEST_POINTS) {
            fbe_test_sep_use_pause_rekey_hooks(rg_config_p, lba, FBE_TEST_HOOK_ACTION_ADD);
        }
        fbe_test_sep_use_pause_rekey_hooks(rg_config_p, prev_lba, FBE_TEST_HOOK_ACTION_DELETE);
    }

    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    /* Check the pattern.
     */
    ichabod_crane_read_pattern(read_context_p, num_luns);
    
    fbe_test_rg_validate_lbas_encrypted(rg_config_p, 
                                        0,/* lba */ 
                                        64, /* Blocks to check */ 
                                        64 /* Blocks encrypted */);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_check_pattern_during_encryption_test()
 ******************************************/

/*!**************************************************************
 * ichabod_random_io_during_encryption_test()
 ****************************************************************
 * @brief
 *  Seed a pattern.
 *  Run encryption.
 *  Check pattern after encryption finishes.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  11/7/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void ichabod_random_io_during_encryption_test(fbe_test_rg_configuration_t * const rg_config_p,
                                                     fbe_api_rdgen_peer_options_t peer_options,
                                                     fbe_data_pattern_t background_pattern,
                                                     fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_u32_t test_points;
    fbe_lba_t lba;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    #define IO_DURING_ENC_INCREMENT_CHECKPOINT 0x1800
    #define IO_DURING_ENC_MAX_TEST_POINTS 2
    #define IO_DURING_ENC_WAIT_IO_COUNT 5

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, ICHABOD_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);

    /* kick off a rekey.
     */
     mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption rekey. ===");
    /* Key Index 0 has no key since we are starting.
     * Key Index 1 has a new key with generation number 1. 
     */
     mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Start");
     status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Complete");

    /* Allow the rekey to proceed and check the pattern at each step.
     */
    lba = 0;
    for (test_points = 0; test_points < IO_DURING_ENC_MAX_TEST_POINTS; test_points++) {
        fbe_lba_t prev_lba = 0;
        mut_printf(MUT_LOG_TEST_STATUS, "Run I/O across checkpoint at lba: 0x%llx", lba);
        /* Wait for debug hook to get hit.
         */
        fbe_test_sep_use_pause_rekey_hooks(rg_config_p, lba, FBE_TEST_HOOK_ACTION_WAIT);

        fbe_api_sleep(1000);

        /* Setup the next hook and remove the old hook.
         */
        prev_lba = lba;
        lba += IO_DURING_ENC_INCREMENT_CHECKPOINT;
        if (test_points + 1 < IO_DURING_ENC_MAX_TEST_POINTS) {
            fbe_test_sep_use_pause_rekey_hooks(rg_config_p, lba, FBE_TEST_HOOK_ACTION_ADD);
        }
        /* Before unpausing the last checkpoint, stop the I/O and restart with light I/O 
         * to allow the rekey to proceed with haste.
         */
        if (test_points + 1 == IO_DURING_ENC_MAX_TEST_POINTS) {

            mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
            status = fbe_api_rdgen_stop_tests(context_p, num_luns);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

            /* Make sure there were no errors.
             */
            fbe_test_sep_io_check_status(context_p, num_luns, FBE_TEST_RANDOM_ABORT_TIME_INVALID);

            /* Start I/O, but not as aggressive so rebuilds will make progress.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting light I/O. ==", __FUNCTION__);
            big_bird_start_io(rg_config_p, &context_p, ICHABOD_LIGHT_THREAD_COUNT, ICHABOD_SMALL_IO_SIZE_MAX_BLOCKS,
                              FBE_RDGEN_OPERATION_WRITE_READ_CHECK, FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
        }
        fbe_test_sep_use_pause_rekey_hooks(rg_config_p, prev_lba, FBE_TEST_HOOK_ACTION_DELETE);
    }

    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    /* Stop I/O and check for errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_random_io_during_encryption_test()
 ******************************************/
/*!**************************************************************
 * ichabod_reboot_both()
 ****************************************************************
 * @brief
 *  Reboot both SPs at the same time.
 *  This is to be used to reboot the SPs during encryption.
 *
 * @param rg_config_p
 *
 * @return None.
 *
 * @author
 *  7/25/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_reboot_both(fbe_test_rg_configuration_t * const rg_config_p)
{

    fbe_sep_package_load_params_t sep_params;
    fbe_neit_package_load_params_t neit_params;
    fbe_u32_t num_raid_groups;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_status_t status;

    ichabod_vault_set_rg_options();

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(peer_sp);
    fbe_test_sp_sim_stop_single_sp(peer_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Now we will bring up the original SP.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_first_sp(&ichabod_raid_group_config_qual[0], active_sp);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    ichabod_vault_set_rg_options();

    /* Bring up passive SP.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(&ichabod_raid_group_config_qual[0], peer_sp);

    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);
    ichabod_vault_set_rg_options();

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end ichabod_reboot_both()
 ******************************************/
/*!**************************************************************
 * ichabod_drive_removal_during_encryption()
 ****************************************************************
 * @brief
 *  Run encryption
 *  Pull a drive.
 *  Insert the drive and let it rebuild.
 *  Wait for encryption to finish.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  7/16/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void ichabod_drive_removal_during_encryption(fbe_test_rg_configuration_t *rg_config_p, void * test_context_p)
{
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t wait_msec;
    fbe_u32_t rg_count;
    fbe_u32_t drives_to_remove = 1;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_api_rdgen_peer_options_t peer_options = FBE_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
    fbe_data_pattern_t background_pattern = FBE_DATA_PATTERN_LBA_PASS;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== starting I/O. %u threads ==", fbe_test_sep_io_get_threads(FBE_U32_MAX));
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, ICHABOD_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % EMCPAL_MINIMUM_TIMEOUT_MSECS) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption rekey. ===");

    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Start");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Complete");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for encryption to start");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_api_sleep(5000);
    
    wait_msec = 10000 + (fbe_random() % 4000);
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hook and then wait %u msec before drive removals", wait_msec);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_DELETE);
    fbe_api_sleep(wait_msec);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives ");

    big_bird_remove_all_drives(rg_config_p, rg_count, drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* This code tests that the rg get info code does the right things to return 
     * the rebuild logging status for these drive(s). 
     */
    big_bird_wait_for_rgs_rb_logging(rg_config_p, rg_count, drives_to_remove);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    big_bird_insert_all_drives(rg_config_p, rg_count, drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);
    //ichabod_reboot_both(rg_config_p);
    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* We need to make sure all encryptions have stopped since if we try to
     * stop rdgen before the pins have cleaned out we will abort a pin. 
     */ 
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rb logging to clear.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = sep_rebuild_utils_wait_for_rb_logging_clear(current_rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* We need to wait for pins to drain so that we do not stop I/O below with pins in progress. 
     * This would fail back pins and cause our encryption to fail. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for pins to drain before we reinsert drives.");
    status = fbe_test_drain_rdgen_pins(FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives.  Complete");

    /* Make sure there were no errors.
     */
    fbe_test_sep_io_check_status(context_p, num_luns, FBE_TEST_RANDOM_ABORT_TIME_INVALID);

    /* Start I/O, but not as aggressive so encryption will make progress.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting light I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, ICHABOD_LIGHT_THREAD_COUNT, ICHABOD_SMALL_IO_SIZE_MAX_BLOCKS,
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK, FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, rg_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    /* Stop I/O and check for errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_drive_removal_during_encryption()
 ******************************************/

/*!**************************************************************
 * ichabod_validate_error_counters()
 ****************************************************************
 * @brief
 *  Make sure we did not take any unexpected errors.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/25/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_validate_error_counters(void)
{
    fbe_status_t status;
    fbe_api_sector_trace_get_counters_t sector_trace_counters;
    fbe_u32_t crc_index;
    fbe_u32_t lba_index;

    status = fbe_api_sector_trace_flag_to_index(FBE_SECTOR_TRACE_ERROR_FLAG_UCRC, &crc_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sector_trace_flag_to_index(FBE_SECTOR_TRACE_ERROR_FLAG_LBA, &lba_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sector_trace_get_counters(&sector_trace_counters);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get sector trace error counters from sep Package!");

    if ((sector_trace_counters.error_type_counters[crc_index] != 0) ||
        (sector_trace_counters.error_type_counters[lba_index] != 0)) {
        mut_printf(MUT_LOG_TEST_STATUS, "found %u crc errors and %u lba stamp errors",
                   sector_trace_counters.error_type_counters[crc_index],
                   sector_trace_counters.error_type_counters[lba_index]);
        MUT_FAIL_MSG("Unexpected errors encountered\n");
    } else {
        mut_printf(MUT_LOG_TEST_STATUS, "%s no unexpected errors found",
                   __FUNCTION__);
    }
}
/******************************************
 * end ichabod_validate_error_counters()
 ******************************************/
/*!**************************************************************
 * ichabod_shutdown_during_encryption()
 ****************************************************************
 * @brief
 *  Run encryption
 *  Pull multiple drives to break RG.
 *  Insert the drive and let it rebuild.
 *  Wait for encryption to finish.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  7/22/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void ichabod_shutdown_during_encryption(fbe_test_rg_configuration_t *rg_config_p, void * test_context_p)
{
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t wait_msec;
    fbe_u32_t rg_count;
    fbe_u32_t drives_to_remove[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_test_drive_removal_mode_t removal_mode[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
    fbe_data_pattern_t background_pattern = FBE_RDGEN_PATTERN_LBA_PASS;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== starting I/O. %u threads ==", fbe_test_sep_io_get_threads(FBE_U32_MAX));
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, ICHABOD_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % EMCPAL_MINIMUM_TIMEOUT_MSECS) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption rekey. ===");

    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Start");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Complete");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for encryption to start");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_api_sleep(5000);

    /* We wait for 10 seconds baseline and then wait for another 4 seconds. 
     * This is arbitrary since encryption will kick off right away, but will be delayed 
     * by the I/O. 
     */
    wait_msec = 10000 + (fbe_random() % 4000);
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hook and then wait %u msec before drive removals", wait_msec);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_DELETE);
    fbe_api_sleep(wait_msec);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives ");

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        switch (current_rg_config_p->raid_type) {
            case FBE_RAID_GROUP_TYPE_RAID0:
                drives_to_remove[rg_index] = 1;
                removal_mode[rg_index] = FBE_DRIVE_REMOVAL_MODE_RANDOM;
                break;
            case FBE_RAID_GROUP_TYPE_RAID10:
                drives_to_remove[rg_index] = 2;
                /* We must remove adjacent drives to break the RG.
                 */
                removal_mode[rg_index] = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;
                break;
            case FBE_RAID_GROUP_TYPE_RAID1:
                drives_to_remove[rg_index] = current_rg_config_p->width;
                removal_mode[rg_index] = FBE_DRIVE_REMOVAL_MODE_RANDOM;
                break;
            case FBE_RAID_GROUP_TYPE_RAID5:
            case FBE_RAID_GROUP_TYPE_RAID3:
                drives_to_remove[rg_index] = 2;
                removal_mode[rg_index] = FBE_DRIVE_REMOVAL_MODE_RANDOM;
                break;
            case FBE_RAID_GROUP_TYPE_RAID6:
                /* We choose to degrade the RG and then break it. 
                 * This limits the amount of errors due to checksum errors in the reconstruct caused 
                 * by coherency errors. 
                 */
                drives_to_remove[rg_index] = 2;
                removal_mode[rg_index] = FBE_DRIVE_REMOVAL_MODE_RANDOM;
                break;
            default:
                MUT_FAIL_MSG("unknown raid type");
                break;
        };
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            big_bird_remove_all_drives(current_rg_config_p, 1, 1,
                                       FBE_TRUE,    /* Yes we are pulling and reinserting the same drive*/
                                       0,    /* do not wait between removals */
                                       removal_mode[rg_index]);

            big_bird_wait_for_rgs_rb_logging(current_rg_config_p, 1, /* RGs */  1 /* Num drives */);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        
        big_bird_remove_all_drives(current_rg_config_p, 1, drives_to_remove[rg_index],
                                   FBE_TRUE,    /* Yes we are pulling and reinserting the same drive*/
                                   0,    /* do not wait between removals */
                                   removal_mode[rg_index]);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives.  Complete");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for raid groups to fail.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++ )
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_FAIL, 
                                                         FBE_TEST_WAIT_TIMEOUT_MS, 
                                                         FBE_PACKAGE_ID_SEP_0);
		if(status != FBE_STATUS_OK){
			fbe_test_common_panic_both_sps();
		}
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            big_bird_insert_all_drives(current_rg_config_p, 1, drives_to_remove[rg_index] + 1,
                                       FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);
        } else {
            big_bird_insert_all_drives(current_rg_config_p, 1, drives_to_remove[rg_index],
                                       FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);
        }
        current_rg_config_p++;
    }
    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* We need to make sure all encryptions have stopped since if we try to
     * stop rdgen before the pins have cleaned out we will abort a pin. 
     */ 
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rb logging to clear.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = sep_rebuild_utils_wait_for_rb_logging_clear(current_rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* Start I/O, but not as aggressive so encryption will make progress.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting light I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, ICHABOD_LIGHT_THREAD_COUNT, ICHABOD_SMALL_IO_SIZE_MAX_BLOCKS,
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK, FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            big_bird_wait_for_rebuilds(current_rg_config_p, 1, drives_to_remove[rg_index] + 1);
        } else {
            big_bird_wait_for_rebuilds(current_rg_config_p, 1, drives_to_remove[rg_index]);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for encryption.  And remove hooks to let encryption proceed.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    /* Stop I/O and check for errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate the error counters");
    ichabod_validate_error_counters();
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate the error counters...Success");

    /* Bringing the RGs back from shutdown we might see COH errors.
     */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_shutdown_during_encryption()
 ******************************************/

/*!**************************************************************
 * ichabod_corrupt_paged()
 ****************************************************************
 * @brief
 *  Corrupt the paged metadata with a pattern rebuild will not like.
 *  This forces the raid group to activate to reconstruct the paged.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None 
 *
 * @author
 *  7/22/2014 - Created. Rob Foley
 ****************************************************************/

void ichabod_corrupt_paged(fbe_test_rg_configuration_t *rg_config_p,
                           fbe_u32_t raid_group_count)
{
    fbe_status_t                                     status;
    fbe_u32_t                                        index;
    fbe_test_rg_configuration_t                     *current_rg_config_p = rg_config_p;
    fbe_object_id_t                                  rg_object_id;
    fbe_api_raid_group_get_paged_info_t              paged_info;
    fbe_sim_transport_connection_target_t            this_sp;
    fbe_sim_transport_connection_target_t            other_sp;
    fbe_api_base_config_metadata_paged_change_bits_t change_bits;
    fbe_raid_group_paged_metadata_t                 *rg_paged_p = NULL;

    /* To make the paged look invalid, set all NR bits.
     */
    rg_paged_p = (fbe_raid_group_paged_metadata_t*)&change_bits.metadata_record_data[0];
    rg_paged_p->needs_rebuild_bits = 0xFF;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for ( index = 0; index < raid_group_count; index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
   
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        change_bits.metadata_offset = 0; /* Write first chunk*/
        change_bits.metadata_repeat_count = 1; /* Write one chunk. */
        change_bits.metadata_record_data_size = (fbe_u32_t)(sizeof(fbe_raid_group_paged_metadata_t));
        change_bits.metadata_repeat_offset = 0;

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            /* For Raid 10, we need to get the object id of the mirror.
             * Issue the control code to RAID group to get its downstream edge list. 
             */  
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* The first and second positions to remove will be considered to land in the first pair.
             */
            
            status = fbe_api_base_config_metadata_paged_set_bits(downstream_object_list.downstream_object_list[0], &change_bits);
        } else {
            /* Fetch the bits for this set of chunks.  It is possible to get errors back
             * if the object is quiescing, so just retry briefly. 
             */
            status = fbe_api_base_config_metadata_paged_set_bits(rg_object_id, &change_bits);
        }

        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end ichabod_corrupt_paged()
 ******************************************/
/*!**************************************************************
 * ichabod_drive_failure_paged_corrupt()
 ****************************************************************
 * @brief
 *  Run encryption
 *  Pull a drive.
 *  Encryption stops.
 *  Corrupt the paged metadata.
 *  Insert the drive and let it rebuild.
 *  Wait for encryption to finish.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  7/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

static void ichabod_drive_failure_paged_corrupt(fbe_test_rg_configuration_t *rg_config_p, void * test_context_p)
{
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_luns;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t wait_msec;
    fbe_u32_t rg_count;
    fbe_u32_t drives_to_remove = 1;
    fbe_test_rg_configuration_t       *current_rg_config_p = NULL;
    fbe_u32_t                          rg_index;
    fbe_api_rdgen_peer_options_t       peer_options = FBE_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
    fbe_data_pattern_t                 background_pattern = FBE_DATA_PATTERN_LBA_PASS;
    fbe_api_sector_trace_get_counters_t sector_trace_counters;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    rg_count = fbe_test_get_rg_array_length(rg_config_p);
    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS) {
        big_bird_write_zero_background_pattern();
    } else {
        big_bird_write_background_pattern();
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== starting I/O. %u threads ==", fbe_test_sep_io_get_threads(FBE_U32_MAX));
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, ICHABOD_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_READ_ONLY,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % EMCPAL_MINIMUM_TIMEOUT_MSECS) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption rekey. ===");

    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Start");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Kick off rekey...Complete");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for encryption to start");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_api_sleep(5000);
    
    wait_msec = 10000 + (fbe_random() % 4000);
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hook and then wait %u msec before drive removals", wait_msec);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_DELETE);
    fbe_api_sleep(wait_msec);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives ");

    big_bird_remove_all_drives(rg_config_p, rg_count, drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* This code tests that the rg get info code does the right things to return 
     * the rebuild logging status for these drive(s). 
     */
    big_bird_wait_for_rgs_rb_logging(rg_config_p, rg_count, drives_to_remove);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* We need to wait for pins to drain so that we do not stop I/O below with pins in progress. 
     * This would fail back pins and cause our encryption to fail. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for pins to drain before we reinsert drives.");
    status = fbe_test_drain_rdgen_pins(FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Stop I/O before corrupting paged.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove drives.  Complete");

    mut_printf(MUT_LOG_TEST_STATUS, "== Write pattern to mark all for rebuild. This guarantees the rebuild hits the error.");
    big_bird_write_background_pattern();
    /* At this point we will corrupt the paged of the raid group.
     * This causes us to invoke the code we are trying to test, which reconstructs the paged 
     * while the raid group is encrypting. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Corrupt paged");
    ichabod_corrupt_paged(rg_config_p, rg_count);
    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert drives");
    big_bird_insert_all_drives(rg_config_p, rg_count, drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);
    
    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* We need to make sure all encryptions have stopped since if we try to
     * stop rdgen before the pins have cleaned out we will abort a pin. 
     */ 
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rb logging to clear.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = sep_rebuild_utils_wait_for_rb_logging_clear(current_rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, rg_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Check pattern 1 ==");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < rg_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* only top object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== Check pattern 2 ==");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_api_sector_trace_get_counters(&sector_trace_counters);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get sector trace error counters from sep Package!");
    if (sector_trace_counters.total_errors_traced != 0) {
        fbe_api_sector_trace_display_counters(FBE_TRUE    /* verbose */);
    }
    MUT_ASSERT_INT_EQUAL(sector_trace_counters.total_errors_traced, 0);
    return;
}
/******************************************
 * end ichabod_drive_failure_paged_corrupt()
 ******************************************/
/*!***************************************************************************
 * fbe_test_sep_rg_create_holes()
 *****************************************************************************
 * @brief
 *  Unbind half the LUNs to create holes in the consumed space.
 *
 * @param rg_config_p - Pointer to array of raid group configurations to get
 *              lun count from.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/12/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_rg_create_holes(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_test_rg_configuration_t *new_rg_config_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   start_index;
    fbe_lun_number_t            lun_number;
    fbe_u32_t                   lun_index;
    fbe_u32_t                   new_lun_index;
    fbe_status_t                status;

    /* Randomize either odd or even.
     */
    if (fbe_random() % 2) {
        start_index = 1;
    } else {
        start_index = 0;
    }
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups.
     * Destroy all even/odd LUN numbers. 
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {

            new_lun_index = 0;
            for (lun_index = 0;
                  lun_index < current_rg_config_p->number_of_luns_per_rg;
                  lun_index ++){
                if ((lun_index % 2) == start_index) {

                    lun_number = current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

                    new_rg_config_p[rg_index].number_of_luns_per_rg--;

                    mut_printf(MUT_LOG_TEST_STATUS, "destroying lun number: %d", lun_number);
                    status = fbe_test_sep_util_destroy_logical_unit(lun_number);
                    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical Unit failed.");
                    /* Mark so that we know this LUN was unbound.
                     */
                    current_rg_config_p->logical_unit_configuration_list[lun_index].job_number = FBE_U32_MAX;
                }else {
                    /* Save this config into the new position in the new rg config p.
                     */
                    new_rg_config_p[rg_index].logical_unit_configuration_list[new_lun_index] = 
                        current_rg_config_p->logical_unit_configuration_list[lun_index];
                    new_lun_index++;
                }
            }
        } 
        /* Goto next raid group
         */
        current_rg_config_p++;
    }
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_sep_rg_create_holes()
 ***************************************************************/

/*!***************************************************************************
 * fbe_test_rg_check_set_unbound_luns()
 *****************************************************************************
 * @brief
 *  Check for patterns in LUNs that were unbound.
 *
 * @param rg_config_p - Pointer to array of raid group configurations to get
 *              lun count from.
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/14/2014 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_rg_check_set_unbound_luns(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_rdgen_operation_t rdgen_operation,
                                                fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_api_rdgen_context_t     context;
    fbe_object_id_t rg_object_id;
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups.
     * Destroy all even/odd LUN numbers. 
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++) {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            fbe_database_lun_info_t lun_info;
            fbe_object_id_t lun_object_id;

            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    
            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            for (lun_index = 0;
                  lun_index < current_rg_config_p->number_of_luns_per_rg;
                  lun_index ++){
                if (current_rg_config_p->logical_unit_configuration_list[lun_index].job_number != FBE_U32_MAX) {

                    status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                                   &lun_object_id);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

                    lun_info.lun_object_id = lun_object_id;
                    status = fbe_api_database_get_lun_info(&lun_info);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    break;
                }
            }
            for (lun_index = 0;
                  lun_index < current_rg_config_p->number_of_luns_per_rg;
                  lun_index ++){
                /* We previously set this to invalid when we unbound the LUN.
                 */
                if (current_rg_config_p->logical_unit_configuration_list[lun_index].job_number == FBE_U32_MAX) {
                    fbe_api_rdgen_io_params_init(&params);
                    params.object_id = rg_object_id;
                    params.class_id = FBE_CLASS_ID_INVALID;
                    params.package_id = FBE_PACKAGE_ID_SEP_0;
                    params.msecs_to_abort = 0;
                    params.msecs_to_expiration = 0;
                    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

                    params.rdgen_operation = rdgen_operation;
                    params.pattern = pattern;
                    params.lba = (lun_index * current_rg_config_p->logical_unit_configuration_list[lun_index].imported_capacity);
                    params.blocks = current_rg_config_p->logical_unit_configuration_list[lun_index].capacity;

                    params.b_async = FBE_FALSE;    /* sync/wait*/
                    mut_printf(MUT_LOG_TEST_STATUS, "send rdgen op 0x%x (object 0x%x) lba 0x%llx bl 0x%llx",
                               rdgen_operation, rg_object_id, params.lba, params.blocks);
                    status = fbe_api_rdgen_send_one_io_params(&context, &params);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        } 
        /* Goto next raid group
         */
        current_rg_config_p++;
    }
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_rg_check_set_unbound_luns()
 ***************************************************************/
/*!**************************************************************
 * ichabod_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 5 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    //fbe_status_t status;

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        ichabod_random_io_during_encryption_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE, 
                                                 FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
        ichabod_random_io_during_encryption_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, 
                                                 FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    } else {
        ichabod_random_io_during_encryption_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_INVALID, 
                                                 FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_test_rg_config()
 ******************************************/
/*!**************************************************************
 * ichabod_read_only_test_rg_config()
 ****************************************************************
 * @brief
 *  Just run reads during encryption.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_read_only_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    //fbe_status_t status;

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        ichabod_read_only_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    } else {
        ichabod_read_only_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_INVALID, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_read_only_test_rg_config()
 ******************************************/
/*!**************************************************************
 * ichabod_check_pattern_test_rg_config()
 ****************************************************************
 * @brief
 *  Just run reads during encryption.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_check_pattern_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    //fbe_status_t status;
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        ichabod_check_pattern_during_encryption_test(rg_config_p, 
                                                     FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, 
                                                     FBE_RDGEN_PATTERN_LBA_PASS, 
                                                     FBE_RDGEN_PATTERN_LBA_PASS);
        ichabod_check_pattern_test(rg_config_p, 
                                   FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, 
                                   FBE_RDGEN_PATTERN_LBA_PASS, 
                                   FBE_RDGEN_PATTERN_LBA_PASS);
    } else {
        ichabod_check_pattern_during_encryption_test(rg_config_p, 
                                                     FBE_API_RDGEN_PEER_OPTIONS_INVALID, 
                                                     FBE_RDGEN_PATTERN_LBA_PASS, 
                                                     FBE_RDGEN_PATTERN_LBA_PASS);
        ichabod_check_pattern_test(rg_config_p, 
                                   FBE_API_RDGEN_PEER_OPTIONS_INVALID, 
                                   FBE_RDGEN_PATTERN_LBA_PASS, 
                                   FBE_RDGEN_PATTERN_LBA_PASS);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_check_pattern_test_rg_config()
 ******************************************/

/*!**************************************************************
 * ichabod_slf_test_rg_config()
 ****************************************************************
 * @brief
 *  Cause SLF on an encrypted raid group and
 *  make sure the data is readable.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  6/17/2014 - Created. Rob Foley
 *  
 ****************************************************************/

void ichabod_slf_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                 status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                    rg_index = 0;
    fbe_u32_t                    num_raid_groups;
    fbe_object_id_t              pvd_object_id;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Write/check pattern.");
    big_bird_write_background_pattern();

    mut_printf(MUT_LOG_TEST_STATUS, " == Degrade all raid groups");

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", current_rg_config_p->raid_group_id);
        status = fbe_test_pp_util_pull_drive(current_rg_config_p->rg_disk_set[0].bus, 
                                             current_rg_config_p->rg_disk_set[0].enclosure, 
                                             current_rg_config_p->rg_disk_set[0].slot,
                                             &current_rg_config_p->drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        } 
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[0].bus, 
                                                                current_rg_config_p->rg_disk_set[0].enclosure, 
                                                                current_rg_config_p->rg_disk_set[0].slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for pvd 0x%x to be in slf", pvd_object_id);
        fozzie_slf_test_wait_pvd_slf_state(pvd_object_id, FBE_TRUE /* yes in slf*/ );
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Read/check pattern.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rg: 0x%x ==", current_rg_config_p->raid_group_id);

        status = fbe_test_pp_util_reinsert_drive(current_rg_config_p->rg_disk_set[0].bus, 
                                                 current_rg_config_p->rg_disk_set[0].enclosure, 
                                                 current_rg_config_p->rg_disk_set[0].slot,
                                                 current_rg_config_p->drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Read/check pattern.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_slf_test_rg_config()
 ******************************************/

/*!**************************************************************
 * ichabod_test_rg_config_skip_unbound()
 ****************************************************************
 * @brief
 *  Run a raid 5 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  12/12/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_test_rg_config_skip_unbound(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    //fbe_status_t status;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_test_rg_configuration_t *new_rg_config_p = NULL;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        } else {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);

        /* Since we are typically just using one thread, we can't use load balance, so we will use random instead 
         * so that some traffic occurrs on both sides. 
         */
        if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
        }
    }
    new_rg_config_p = (fbe_test_rg_configuration_t *)fbe_api_allocate_memory(sizeof(fbe_test_rg_configuration_t) * (rg_count + 1));
    if (new_rg_config_p == NULL) {
        MUT_FAIL_MSG("Could not allocate memory");
    }
    big_bird_write_background_pattern();
    fbe_copy_memory(new_rg_config_p, rg_config_p, sizeof(fbe_test_rg_configuration_t) * (rg_count + 1));
    fbe_test_rg_wait_for_zeroing(rg_config_p);
    
    /* Unbind half the LUNs to create holes.  Also remove these LUNs from the RG configuration.
     */
    fbe_test_sep_rg_create_holes(rg_config_p, new_rg_config_p);

    fbe_test_rg_check_set_unbound_luns(rg_config_p, FBE_RDGEN_OPERATION_WRITE_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
    fbe_test_rg_check_set_unbound_luns(rg_config_p, FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);

    big_bird_write_background_pattern();
    fbe_test_rg_check_set_unbound_luns(rg_config_p, FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
    //fbe_test_sep_util_validate_encryption_mode_setup(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        ichabod_check_pattern_test(new_rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    } else {
        ichabod_check_pattern_test(new_rg_config_p, peer_options, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    }

    fbe_test_rg_check_set_unbound_luns(rg_config_p, FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_ZEROS);

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    fbe_api_free_memory(new_rg_config_p);
    return;
}
/******************************************
 * end ichabod_test_rg_config_skip_unbound()
 ******************************************/

/*!**************************************************************
 * ichabod_test_rg_config_degraded()
 ****************************************************************
 * @brief
 *  Run an encryption test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 ****************************************************************/

void ichabod_test_rg_config_degraded(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    //fbe_status_t status;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        } else {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);

        /* Since we are typically just using one thread, we can't use load balance, so we will use random instead 
         * so that some traffic occurrs on both sides. 
         */
        if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
        }
    }

    fbe_test_rg_wait_for_zeroing(rg_config_p);

    //fbe_test_sep_util_validate_encryption_mode_setup(rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        
    } else {
        ichabod_cache_dirty_test(rg_config_p, peer_options, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
        ichabod_cache_disabled_test(rg_config_p, peer_options, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
        ichabod_degraded_test(rg_config_p, peer_options, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_test_rg_config_degraded()
 ******************************************/

/*!**************************************************************
 * ichabod_test_rg_config_incorrect_key()
 ****************************************************************
 * @brief
 *  Run an encryption test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/3/2014 - Created. Rob Foley
 ****************************************************************/

void ichabod_test_rg_config_incorrect_key(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        } else {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);

        /* Since we are typically just using one thread, we can't use load balance, so we will use random instead 
         * so that some traffic occurrs on both sides. 
         */
        if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for zeroing to finish. ==");
    fbe_test_rg_wait_for_zeroing(rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        
    } else {
        ichabod_incorrect_key_test(rg_config_p, peer_options, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_test_rg_config_incorrect_key()
 ******************************************/

/*!**************************************************************
 * ichabod_test_rg_config_incorrect_key_one_drive()
 ****************************************************************
 * @brief
 *  Run an encryption test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/28/2014 - Created. Rob Foley
 ****************************************************************/

void ichabod_test_rg_config_incorrect_key_one_drive(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        } else {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);

        /* Since we are typically just using one thread, we can't use load balance, so we will use random instead 
         * so that some traffic occurrs on both sides. 
         */
        if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE) {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for zeroing to finish. ==");
    fbe_test_rg_wait_for_zeroing(rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        
    } else {
        ichabod_incorrect_key_one_drive_test(rg_config_p, peer_options, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_test_rg_config_incorrect_key_one_drive()
 ******************************************/

/*!**************************************************************
 * ichabod_test_rg_config_zero()
 ****************************************************************
 * @brief
 *  Run a raid 5 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  1/22/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_test_rg_config_zero(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    //fbe_status_t status;

    fbe_test_rg_wait_for_zeroing(rg_config_p);

    ichabod_zeroing_drive_failure_test(rg_config_p);
    ichabod_zeroing_interaction_test(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_test_rg_config_zero()
 ******************************************/
/*!**************************************************************
 * ichabod_zeroing_drive_failure_test()
 ****************************************************************
 * @brief
 *  This test case validates the interaction between Zeroing, drive
 * failure and enabling encryption *     
 *
 * @param rg_config_p - config to run test against.               
 *
 * @notes
 *      The main purpose of this test is to make sure that if the 
 * encryption was enabled when the drive is not available that the keys
 * gets pushed down when the drive does comes back
 *
 * @return None.   
 *
 * @author
 *  04/29/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void ichabod_zeroing_drive_failure_test(fbe_test_rg_configuration_t * const rg_config_p)
{

    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_u32_t disk_position;
    fbe_object_id_t     pvd_id[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_api_provision_drive_info_t pvd_get_info;
    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_api_terminator_device_handle_t  drive_info[10][1];
    fbe_provision_drive_control_get_mp_key_handle_t mp_key_handle;
    fbe_block_count_t          num_encrypted;
    fbe_u32_t time = 0;

    /* Since we are going to remove drives, we want to prevent HS from swapping in */
    mut_printf(MUT_LOG_TEST_STATUS, "Disable Automatic HS...");
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to turn off HS\n");

    current_rg_config_p++;
    /* Add debug hooks to prevent Zeroing from completing */
    for (rg_index = 1; rg_index < rg_count; rg_index++)
    {
        for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[disk_position].bus,
                                                                     current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                     current_rg_config_p->rg_disk_set[disk_position].slot,
                                                                     &pvd_id[disk_position]);
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to get the PVD Location\n");

            ichabod_crane_add_zero_debug_hook(pvd_id[disk_position]);
        }
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);

        /* Issue a command to zero out a lun */
        status = fbe_api_lun_initiate_zero(lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Remove one drive */
        mut_printf(MUT_LOG_TEST_STATUS, "Remove one drive for RG:%d..\n", rg_index);
        status = fbe_test_pp_util_pull_drive(current_rg_config_p->rg_disk_set[0].bus,
                                             current_rg_config_p->rg_disk_set[0].enclosure,
                                             current_rg_config_p->rg_disk_set[0].slot,
                                             &drive_info[rg_index][0]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    current_rg_config_p++;
    /* Enable Encryption */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "Waiting 10 seconds to make sure Encryption mode does not change\n");
    fbe_api_sleep(10000); /* Sleep for 10 seconds */

    /* Validate that Encryption mode does not change because Zeroing is still runing */
    for (rg_index = 1; rg_index < rg_count; rg_index++)
    {
        for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[disk_position].bus,
                                                                     current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                     current_rg_config_p->rg_disk_set[disk_position].slot,
                                                                     &pvd_id[disk_position]);
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to get the PVD Location\n");

            status = fbe_api_base_config_get_encryption_mode(pvd_id[disk_position], &encryption_mode);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID, "Encryption mode unexpected\n");

            if(disk_position == 0) {
                /* We just need to check this position because this is the drive we removed */
                mp_key_handle.index = 0;
                mp_key_handle.mp_handle_1 = 0;
                mp_key_handle.mp_handle_2 = 0;
                status = fbe_api_provision_drive_get_miniport_key_handle(pvd_id[0], &mp_key_handle);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                MUT_ASSERT_UINT64_EQUAL(mp_key_handle.mp_handle_1, 0);
                MUT_ASSERT_UINT64_EQUAL(mp_key_handle.mp_handle_2, 0);
            }
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_base_config_get_encryption_mode(rg_object_id, &encryption_mode);
        MUT_ASSERT_INT_EQUAL_MSG(encryption_mode, FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID, "Encryption mode unexpected\n");
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    current_rg_config_p++;
    /* Insert the removed Drives to make sure now the handle gets pushed */
    for (rg_index = 1; rg_index < rg_count; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Reinsert one drive for RG:%d..\n", rg_index);
        fbe_test_pp_util_reinsert_drive(current_rg_config_p->rg_disk_set[0].bus,
                                        current_rg_config_p->rg_disk_set[0].enclosure,
                                        current_rg_config_p->rg_disk_set[0].slot,
                                        drive_info[rg_index][0]);

        status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[0].bus,
                                                                     current_rg_config_p->rg_disk_set[0].enclosure,
                                                                     current_rg_config_p->rg_disk_set[0].slot,
                                                                     &pvd_id[0]);
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to get the PVD Location\n");

        /* Now that the drive is inserted, make sure that the keys are getting pushed down */
        mp_key_handle.index = 0;
        mp_key_handle.mp_handle_1 = 0;
        mp_key_handle.mp_handle_2 = 0;
        while(time < 120000) {
            status = fbe_api_provision_drive_get_miniport_key_handle(pvd_id[0], &mp_key_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            if(mp_key_handle.mp_handle_2 != 0) {
                break;
            }
            fbe_api_sleep(100);
            time += 100;
        }
        MUT_ASSERT_UINT64_EQUAL(mp_key_handle.mp_handle_1, 0);
        MUT_ASSERT_UINT64_NOT_EQUAL(mp_key_handle.mp_handle_2, 0);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    current_rg_config_p++;
    /* Delete all the debug hooks */
    for (rg_index = 1; rg_index < rg_count; rg_index++)
    {
        for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[disk_position].bus,
                                                                     current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                     current_rg_config_p->rg_disk_set[disk_position].slot,
                                                                     &pvd_id[disk_position]);
            MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to get the PVD Location\n");

            ichabod_crane_del_zero_debug_hook(pvd_id[disk_position]);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for encryption to complete...\n");
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */ 
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    current_rg_config_p++;
    for (rg_index = 1; rg_index < rg_count; rg_index++)
    {
        for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[disk_position].bus,
                                                                     current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                     current_rg_config_p->rg_disk_set[disk_position].slot,
                                                                     &pvd_id[disk_position]);

            /* Validate that the Paged MD region on the drive is really encrypted */
            status = fbe_api_provision_drive_get_info(pvd_id[disk_position], &pvd_get_info);
            // insure that there were no errors
            MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

            mut_printf(MUT_LOG_TEST_STATUS, "Validate that Paged is encrypted on PVD:0x%08x\n", pvd_id[disk_position]);
            status = fbe_api_encryption_check_block_encrypted(current_rg_config_p->rg_disk_set[disk_position].bus,
                                                              current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                              current_rg_config_p->rg_disk_set[disk_position].slot,
                                                              pvd_get_info.paged_metadata_lba,
                                                              10,
                                                              &num_encrypted);
            MUT_ASSERT_UINT64_EQUAL_MSG(10, num_encrypted, "Incorrect number of encrypted blocks\n");
        }
        current_rg_config_p++;
    }

    /* Since we are going to remove drives, we want to prevent HS from swapping in */
    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Unable to turn on HS\n");
}
/*!**************************************************************
 * ichabod_zeroing_interaction_test()
 ****************************************************************
 * @brief
 *  This test case validates the interaction between Zeroing and
 *  encryption
 *     
 *
 * @param rg_config_p - config to run test against.               
 *
 * @notes
 *      - Validates that if RG's paged is NOT encrypted then zero
 * marking of LUN is errored back
 *      - Encryption has higher priority over zeroing once encryption
 *        starts
 *      - If Encryption has not YET started then Zeroing needs to finish
 *
 * @return None.   
 *
 * @author
 *  01/15/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void ichabod_zeroing_interaction_test(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_u32_t rg_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index;
    fbe_object_id_t lun_object_id;
    fbe_u32_t disk_position;
    fbe_object_id_t     pvd_id[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_lba_t   expect_checkpoint;
    fbe_api_provision_drive_info_t pvd_get_info;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            return;
        }

        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);

        /* Add a debug hook to make sure we hit the check for downstream priority and mode does not change */
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1, /* All ds objects */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Enable Encryption */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");

    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_ADD);

    /* Key Index 0 has no key since we are starting.
     * Key Index 1 has a new key with generation number 1. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for vault to be encrypted before starting zeroing ==");
    fbe_test_sep_util_wait_object_encryption_mode(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED, FBE_TEST_WAIT_TIMEOUT_MS);
    //fbe_test_rg_wait_for_zeroing(rg_config_p);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);

        /* Make sure now we start the encryption process */
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1, /* All ds objects */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Issue a command to zero out a lun */
        status = fbe_api_lun_initiate_zero(lun_object_id);

        /* Validate that zeroing operation will fail */
        MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

        /* Remove the hook */
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1, /* All ds objects */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_WAIT);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);

        /* Now issue the command to validate that we can now initiate the LUN Zeroing operation */
        status = fbe_api_lun_initiate_zero(lun_object_id);

        /* Validate that kicking of zeroing operation will succeed now */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Sleep 10 seconds before validating Zeroing Stops  ==", __FUNCTION__);
    fbe_api_sleep(10000); /* Sleep for 10 seconds */

    /* Validate that the checkpoint is still at default offset */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        /* Validate that Zeroing does not progress until encryption is still outstanding */
        for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location( current_rg_config_p->rg_disk_set[disk_position].bus,
                                                                     current_rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                     current_rg_config_p->rg_disk_set[disk_position].slot,
                                                                     &pvd_id[disk_position]);
            status = fbe_api_provision_drive_get_zero_checkpoint(pvd_id[disk_position], &expect_checkpoint);
            status = fbe_api_provision_drive_get_info(pvd_id[disk_position], &pvd_get_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_UINT64_EQUAL(expect_checkpoint, pvd_get_info.default_offset);
        }
        current_rg_config_p++;
    }


    /* Add the debug hooks.
     */
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, 0, FBE_TEST_HOOK_ACTION_DELETE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for encryption to complete.... ==", __FUNCTION__);
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */ 
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for Zeroing to complete ==", __FUNCTION__);
        for (disk_position = 0; disk_position < current_rg_config_p->width; disk_position++)
        {
            status = fbe_test_zero_wait_for_disk_zeroing_complete(pvd_id[disk_position], FBE_TEST_HOOK_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Now validate that Zeroing is completed now that encryption is also complete */

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/**************************************************
 * end ichabod_random_io_during_encryption_test()
 **************************************************/

/*!**************************************************************
 * ichabod_crane_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_test_rg_config,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    return;
}
/******************************************
 * end ichabod_crane_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_drive_removal_test()
 ****************************************************************
 * @brief
 *  Run an encryption test where drives are removed during DIP encryption.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  7/16/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_drive_removal_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_drive_removal_during_encryption,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    return;
}
/******************************************
 * end ichabod_crane_drive_removal_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_shutdown_test()
 ****************************************************************
 * @brief
 *  Run an encryption test where a RG is shutdown during DIP encryption.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  7/22/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_shutdown_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_shutdown_during_encryption,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    return;
}
/******************************************
 * end ichabod_crane_shutdown_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_drive_removal_paged_corrupt_test()
 ****************************************************************
 * @brief
 *  Run an encryption test where we intentionally
 *  corrupt the paged and make sure it gets reconstructed properly.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  7/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_drive_removal_paged_corrupt_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_drive_failure_paged_corrupt,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Delete encryption completing hooks ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

    return;
}
/******************************************
 * end ichabod_crane_drive_removal_paged_corrupt_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_dualsp_drive_removal_test()
 ****************************************************************
 * @brief
 *  Run an encryption test where drives are removed during DIP encryption.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  7/16/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_dualsp_drive_removal_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_drive_removal_during_encryption,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    return;
}
/******************************************
 * end ichabod_crane_dualsp_drive_removal_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_read_only_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_read_only_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_read_only_test_rg_config,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    return;
}
/******************************************
 * end ichabod_crane_read_only_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_check_pattern_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_check_pattern_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_check_pattern_test_rg_config,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    return;
}
/******************************************
 * end ichabod_crane_check_pattern_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_degraded_test()
 ****************************************************************
 * @brief
 *  Degraded encryption test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  1/17/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_degraded_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_test_rg_config_degraded,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    return;
}
/******************************************
 * end ichabod_crane_degraded_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_incorrect_key_test()
 ****************************************************************
 * @brief
 *  Make sure that if the key is incorrect we detect it.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  2/3/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_incorrect_key_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    /* Before we create the RG, enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_test_rg_config_incorrect_key,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE    /* don't destroy config */);
    /* We will not run encryption, but we will need it already encrypted.
     */
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);

    return;
}
/******************************************
 * end ichabod_crane_incorrect_key_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_incorrect_key_one_drive_test()
 ****************************************************************
 * @brief
 *  Make sure that if the key is incorrect we detect it.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  2/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_incorrect_key_one_drive_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    /* Before we create the RG, enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_test_rg_config_incorrect_key_one_drive,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE    /* don't destroy config */);
    /* We will not run encryption, but we will need it already encrypted.
     */
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end ichabod_crane_incorrect_key_one_drive_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_rg_create_key_error_test()
 ****************************************************************
 * @brief
 *  Test case where we get a key error on RG create.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  3/5/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_crane_rg_create_key_error_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_raid_groups;
    rg_config_p = &ichabod_raid_group_config_qual[0];

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    rg_config_p->b_create_this_pass = FBE_TRUE;

    rg_config_p++; /* skip system drives RG. */
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p) - 1;
    ichabod_key_error_create_test_rg(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE, num_raid_groups);
    ichabod_key_error_create_test_rg(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED, num_raid_groups);
    ichabod_key_error_create_test_rg(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR, num_raid_groups);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    return;
}
/******************************************
 * end ichabod_crane_rg_create_key_error_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_rg_create_system_drives_key_error_test()
 ****************************************************************
 * @brief
 *  Test case where we get a key error on RG create using system drives.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  3/12/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_crane_rg_create_system_drives_key_error_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_status_t status;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    rg_config_p = &ichabod_raid_group_config_qual[0];

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable user RGs, we only test system RGs.
     */
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    for (rg_index = 1; rg_index < num_raid_groups; rg_index++) {
        rg_config_p[rg_index].b_create_this_pass = FBE_FALSE;
        rg_config_p[rg_index].raid_type = FBE_U32_MAX;
        rg_config_p[rg_index].width = FBE_U32_MAX;
        rg_config_p[rg_index].capacity = FBE_U32_MAX;
    }

    num_raid_groups = 1;
    ichabod_key_error_create_test_rg(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE, num_raid_groups);
    ichabod_key_error_create_test_rg(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED, num_raid_groups);
    ichabod_key_error_create_test_rg(rg_config_p, FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR, num_raid_groups);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    return;
}
/******************************************
 * end ichabod_crane_rg_create_system_drives_key_error_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_key_error_io_test()
 ****************************************************************
 * @brief
 *  Test case where we get a key error on I/Os.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  3/7/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_crane_key_error_io_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    /* Before we create the RG, enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_key_error_io_test_rg,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE    /* don't destroy config */);
    /* We will not run encryption, but we will need it already encrypted.
     */
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end ichabod_crane_key_error_io_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_validate_key_error_test()
 ****************************************************************
 * @brief
 *  Test handling a key error on a validation of keys.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  3/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_validate_key_error_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &ichabod_raid_group_config_qual[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    /* Before we create the RG, enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    rg_config_p++; // skip system RGs
    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_validation_key_error_test_rg,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_FALSE    /* destroy config */);
    /* We will not run encryption, but we will need it already encrypted.
     */
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);

    return;
}
/******************************************
 * end ichabod_crane_validate_key_error_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_validate_key_error_system_drive_test()
 ****************************************************************
 * @brief
 *  Test handling a key error on a validation of keys.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  3/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_validate_key_error_system_drive_test(void)
{
    fbe_status_t status;
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t num_raid_groups;
    fbe_u32_t rg_index;
    rg_config_p = &ichabod_raid_group_config_qual[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    /* Before we create the RG, enable encryption */
    status = fbe_test_sep_util_enable_kms_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    /* Disable user RGs, we only test system RGs.
     */
    for (rg_index = 1; rg_index < num_raid_groups; rg_index++) {
        rg_config_p[rg_index].b_create_this_pass = FBE_FALSE;
        rg_config_p[rg_index].raid_type = FBE_U32_MAX;
        rg_config_p[rg_index].width = FBE_U32_MAX;
        rg_config_p[rg_index].capacity = FBE_U32_MAX;
    }
    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_validation_key_error_test_rg,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE    /* don't destroy config */);
    /* We will not run encryption, but we will need it already encrypted.
     */
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);

    return;
}
/******************************************
 * end ichabod_crane_validate_key_error_system_drive_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_zero_test()
 ****************************************************************
 * @brief
 *  Degraded encryption test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  1/22/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_zero_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){ 
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_test_rg_config_zero,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE /* don't destroy config */);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }
    return;
}
/******************************************
 * end ichabod_crane_zero_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_unconsumed_test()
 ****************************************************************
 * @brief
 *  Degraded encryption test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  1/30/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_crane_unconsumed_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_sep_util_set_chunks_per_rebuild(1);
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_test_rg_config_skip_unbound,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP_UNBIND_TEST,
                                                             ICHABOD_CHUNKS_PER_LUN_UNBIND_TEST,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params); 
    }
    return;
}
/******************************************
 * end ichabod_crane_unconsumed_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_system_drive_test()
 ****************************************************************
 * @brief
 *  System drive bind encrypted RG/LUNs test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  2/10/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_crane_system_drive_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_status_t status;
    rg_config_p = &ichabod_raid_group_config_system[0];

    mut_pause();

    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    rg_config_p->b_create_this_pass = FBE_TRUE;
    ichabod_system_drive_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);
    mut_pause();
    mut_printf(MUT_LOG_TEST_STATUS, "== Start test to re-bind the same drives ==");
    rg_config_p = &ichabod_raid_group_config_system_1[0];
    rg_config_p->b_create_this_pass = FBE_TRUE;
    ichabod_system_drive_reboot_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    return;
}
/******************************************
 * end ichabod_crane_system_drive_test()
 ******************************************/
void ichabod_crane_dualsp_system_drive_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== starting %s ==", __FUNCTION__);

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    ichabod_crane_system_drive_test();

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
}
/*!**************************************************************
 * ichabod_crane_dualsp_unconsumed_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  1/30/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_dualsp_unconsumed_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &ichabod_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &ichabod_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_test_rg_config_skip_unbound,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end ichabod_crane_dualsp_unconsumed_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();


    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_test_rg_config,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end ichabod_crane_dualsp_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_dualsp_read_only_test()
 ****************************************************************
 * @brief
 *  Run read only I/O during an encryption.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_crane_dualsp_read_only_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();


    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_read_only_test_rg_config,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end ichabod_crane_dualsp_read_only_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_dualsp_check_pattern_test()
 ****************************************************************
 * @brief
 *  Run encryption test where we check patterns at various points
 *  of the encryption.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  2/20/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_dualsp_check_pattern_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();


    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_check_pattern_test_rg_config,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end ichabod_crane_dualsp_check_pattern_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_dualsp_slf_test()
 ****************************************************************
 * @brief
 *  Run an encryption test with SLF.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  6/17/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_dualsp_slf_test(void)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;

    rg_config_p = &ichabod_raid_group_config_qual[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    mut_printf(MUT_LOG_TEST_STATUS, "== Enable encryption ==");
    status = fbe_test_sep_util_enable_kms_encryption();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL, ichabod_slf_test_rg_config,
                                                         ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                         ICHABOD_CHUNKS_PER_LUN,
                                                         FBE_TRUE);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end ichabod_crane_dualsp_slf_test()
 ******************************************/
/*!**************************************************************
 * ichabod_crane_dualsp_zero_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_crane_dualsp_zero_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    if (test_level > 0)
    {
        rg_config_p = &ichabod_raid_group_config_qual[0];  //extended[0];
    }
    else
    {
        rg_config_p = &ichabod_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p)){
        fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                             NULL, ichabod_test_rg_config_zero,
                                                             ICHABOD_TEST_LUNS_PER_RAID_GROUP,
                                                             ICHABOD_CHUNKS_PER_LUN,
                                                             FBE_TRUE);
        params.b_encrypted = FBE_FALSE;
        fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    }

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end ichabod_crane_dualsp_zero_test()
 ******************************************/

/*!**************************************************************
 * ichabod_crane_system_drive_createrg_encrypt_reboot_test()
 ****************************************************************
 * @brief
 *  System drive binds RG/LUNs, encryption, and reboot test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  04/10/2014 - Created. 
 *
 ****************************************************************/
void ichabod_crane_system_drive_createrg_encrypt_reboot_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_pause();
    mut_printf(MUT_LOG_TEST_STATUS, "== Start %s ==", __FUNCTION__);

    rg_config_p = &ichabod_raid_group_config_system_1[0];
    rg_config_p->b_create_this_pass = FBE_TRUE;

    ichabod_system_drive_createrg_encrypt_reboot_test(rg_config_p, FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, FBE_RDGEN_PATTERN_LBA_PASS, FBE_RDGEN_PATTERN_LBA_PASS);

    // Make sure no errors occurred.
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end ichabod_crane_system_drive_createrg_encrypt_reboot_test()
 ******************************************/

void ichabod_setup_rg_config(fbe_test_rg_configuration_t *rg_config_p)
{
    /* We no longer create the raid groups during the setup
     */
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    fbe_test_rg_setup_random_block_sizes(rg_config_p);
    fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
#if 1
    if (rg_config_p[0].width > 4) {
        /* Limit our system RG to 4 drives so it does not exceed the system drives. 
         */
        rg_config_p[0].width = 4;
    }
    /* First config is on system drives, it uses a fixed disk config that we specified 
     * for the system drives. 
     */
    rg_config_p->b_use_fixed_disks = ichabod_use_fixed_sys_disk_config;
#endif
    fbe_test_sep_util_update_extra_chunk_size(rg_config_p, ICHABOD_CHUNKS_PER_LUN * 2);
}
/*!**************************************************************
 * ichabod_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &ichabod_raid_group_config_qual[0];

        ichabod_setup_rg_config(rg_config_p);

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

        elmo_load_config(rg_config_p, 
                           ICHABOD_TEST_LUNS_PER_RAID_GROUP, 
                           ICHABOD_CHUNKS_PER_LUN);
    }
    ichabod_vault_set_rg_options();

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_test_disable_background_ops_system_drives();
    
    /* load KMS */
    sep_config_load_kms(NULL);
    fbe_test_set_rg_vault_encrypt_wait_time(1000);
    /* It can take a few minutes for the vault to encrypt itself. 
     * So waits for hooks, etc might need to be longer. 
     */
    fbe_test_set_timeout_ms(10 * 60000);

    return;
}
/**************************************
 * end ichabod_setup()
 **************************************/

/*!**************************************************************
 * ichabod_drive_removal_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_drive_removal_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Since we are doing drive removal tests, just test with redundant raid types.
     */
    rg_config_p = &ichabod_raid_group_config_qual[0];
    rg_config_p[0].raid_type = FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT;

    /* Do not use the RAID-0 config, use a different config.
     */
    while (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) {
        rg_config_p++;
    }
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) {
        MUT_FAIL_MSG("Did not find RAID-0 config");
    }
    rg_config_p->raid_type = FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT;
    rg_config_p->width = FBE_TEST_RG_CONFIG_RANDOM_TAG;
    ichabod_setup();
    return;
}
/**************************************
 * end ichabod_drive_removal_setup()
 **************************************/
/*!**************************************************************
 * ichabod_shutdown_setup()
 ****************************************************************
 * @brief
 *  Setup for a shutdown during encryption test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  7/22/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_shutdown_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Since we are doing drive removal tests, just test with redundant raid types.
     */
    rg_config_p = &ichabod_raid_group_config_qual[0];
    /* We must skip R6 for now because of the way it handles inconsistencies degraded. 
     * The R6 is degraded when it goes broken. 
     * It logs crc errors for certain cases where it gets inconsistencies 
     * until that is fixed we will skip R6.
     */
    while (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID6) {
        rg_config_p++;
    }
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID6) {
        MUT_FAIL_MSG("Did not find RAID-0 config");
    }
    rg_config_p->raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    rg_config_p->width = FBE_TEST_RG_CONFIG_RANDOM_TAG;

    ichabod_use_fixed_sys_disk_config = FBE_FALSE;
    ichabod_setup();
    return;
}
/**************************************
 * end ichabod_shutdown_setup()
 **************************************/

/*!**************************************************************
 * ichabod_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test to run on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_dualsp_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        rg_config_p = &ichabod_raid_group_config_qual[0]; 

        ichabod_setup_rg_config(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

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

    ichabod_vault_set_rg_options();

    /* Initialize any required fields 
     */
    fbe_test_sep_reset_encryption_enabled();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_test_set_rg_vault_encrypt_wait_time(1000);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_set_rg_vault_encrypt_wait_time(1000);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_disable_background_ops_system_drives();

    /* load KMS */
    sep_config_load_kms_both_sps(NULL);

    /* It can take a few minutes for the vault to encrypt itself. 
     * So waits for hooks, etc might need to be longer. 
     */
    fbe_test_set_timeout_ms(10 * 60000);

    return;
}
/**************************************
 * end ichabod_dualsp_setup()
 **************************************/

/*!**************************************************************
 * ichabod_drive_removal_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a dualsp test where drives are removed.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  7/25/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ichabod_drive_removal_dualsp_setup(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Since we are doing drive removal tests, just test with redundant raid types.
     */
    rg_config_p = &ichabod_raid_group_config_qual[0];
    rg_config_p[0].raid_type = FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT;

    /* Do not use the RAID-0 config, use a different config.
     */
    while (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) {
        rg_config_p++;
    }
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) {
        MUT_FAIL_MSG("Did not find RAID-0 config");
    }
    rg_config_p->raid_type = FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT;
    rg_config_p->width = FBE_TEST_RG_CONFIG_RANDOM_TAG;

    ichabod_dualsp_setup();
    return;
}
/**************************************
 * end ichabod_drive_removal_dualsp_setup()
 **************************************/
/*!**************************************************************
 * ichabod_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ichabod test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end ichabod_cleanup()
 ******************************************/
/*!**************************************************************
 * ichabod_cleanup_with_errs()
 ****************************************************************
 * @brief
 *  Cleanup the ichabod test and expect errors.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_cleanup_with_errs(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_kms();
        fbe_sep_util_destroy_neit_sep_phy_expect_errs();
    }
    return;
}
/******************************************
 * end ichabod_cleanup_with_errs()
 ******************************************/
        

/*!**************************************************************
 * ichabod_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ichabod test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ichabod_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {		
        fbe_test_sep_util_destroy_kms_both_sps();
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end ichabod_dualsp_cleanup()
 ******************************************/
/*************************
 * end file ichabod_crane_test.c
 *************************/


