/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file ernie_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a degraded raid 6 test.
 *
 * @version
 *   9/10/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "sep_rebuild_utils.h"                              //  .h shared between Dora and other NR/rebuild tests
#include "sep_hook.h"
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * ernie_short_desc = "raid 6 degraded read and write I/O test.";
/* uses big bird long description since we are based on big bird. */
char * ernie_long_desc = NULL;

enum ernie_defines_e
{
    ERNIE_TEST_LUNS_PER_RAID_GROUP = 3,
    ERNIE_CHUNKS_PER_LUN = 3,
    ERNIE_MAX_IO_SIZE_BLOCKS = (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE,
};


/*!*******************************************************************
 * @var ernie_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ernie_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,              block size  RAID-id.    bandwidth.*/
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        1,          0},
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        2,          0},
    {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          0},
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        4,          1},
    {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        5,          1},
    {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        6,          1},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var ernie_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t ernie_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/

    {6,       0xF000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * ernie_single_degraded_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_single_degraded_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1, /* drives to remove */ big_bird_normal_degraded_io_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_single_degraded_test()
 ******************************************/

/*!**************************************************************
 * ernie_single_degraded_zero_abort_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_single_degraded_zero_abort_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1, /* drives to remove */ big_bird_zero_and_abort_io_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_single_degraded_zero_abort_test()
 ******************************************/
/*!**************************************************************
 * ernie_double_degraded_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_double_degraded_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)2, /* drives to remove */ big_bird_normal_degraded_io_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_double_degraded_test()
 ******************************************/

/*!*******************************************************************
 * @var ernie_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t ernie_test_contexts[ERNIE_TEST_LUNS_PER_RAID_GROUP * 2];

static void ernie_add_error_record(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_protocol_error_injection_error_record_t     record;
    fbe_status_t                                    status;
    fbe_object_id_t                                 pdo_object_id;
    fbe_protocol_error_injection_record_handle_t    out_record_handle_p;
    fbe_api_raid_group_get_info_t info;
    fbe_raid_group_map_info_t     map_info;
    fbe_object_id_t               rg_object_id;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));
    map_info.lba += info.paged_metadata_start_lba;

    status = fbe_api_raid_group_map_lba(rg_object_id, &map_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_neit_utils_init_error_injection_record(&record);

    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[map_info.data_pos].bus,
                                                              rg_config_p->rg_disk_set[map_info.data_pos].enclosure,
                                                              rg_config_p->rg_disk_set[map_info.data_pos].slot,
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error record for rg object: 0x%x pos: %u pdo: 0x%x %u_%u_%u lba: 0x%llx",
               rg_object_id, map_info.data_pos, pdo_object_id, 
               rg_config_p->rg_disk_set[map_info.data_pos].bus,
               rg_config_p->rg_disk_set[map_info.data_pos].enclosure,
               rg_config_p->rg_disk_set[map_info.data_pos].slot,
               map_info.pba);

    record.object_id                = pdo_object_id; 
    record.lba_start                = map_info.pba;
    record.lba_end                  = map_info.pba;
    record.num_of_times_to_insert   = 1;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        fbe_test_sep_util_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    fbe_test_sep_util_set_scsi_error_from_error_type(&record.protocol_error_injection_error.protocol_error_injection_scsi_error,
                                                     FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE);
    //  Add the error injection record to the service, which also returns the record handle for later use
    status = fbe_api_protocol_error_injection_add_record(&record, &out_record_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}
static void ernie_remove_drive(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                    status;
    fbe_api_raid_group_get_info_t info;
    fbe_raid_group_map_info_t     map_info;
    fbe_object_id_t               rg_object_id;
    fbe_u32_t drives_to_remove[] = {0, FBE_U32_MAX};

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));
    map_info.lba += info.paged_metadata_start_lba;

    status = fbe_api_raid_group_map_lba(rg_object_id, &map_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    drives_to_remove[0] = (map_info.data_pos == 0) ? 1 : 0;
    fbe_test_set_specific_drives_to_remove(rg_config_p, drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, 1, /*rg count */ 1, /* Drive count */
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SPECIFIC);

}
/*!**************************************************************
 * ernie_run_simultaneous_insert_and_remove()
 ****************************************************************
 * @brief
 *  In this test we pull one drive on a RAID-6.
 *  Then we simulaneously insert that first drive and pull another drive.
 *
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  5/16/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void ernie_run_simultaneous_insert_and_remove(fbe_test_rg_configuration_t * rg_config_p,
                                                                void * drive_to_remove_p)
{
    fbe_api_rdgen_context_t *context_p = &ernie_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t num_contexts = 1;
    fbe_element_size_t element_size;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    if (rg_config_p->b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_zero_background_pattern();

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Remove the first drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Pull one drive for each raid group. ==");

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)){
            current_rg_config_p++;
            continue;
        }
        ernie_remove_drive(current_rg_config_p);
        current_rg_config_p++;
    }

    /* Sleep a random time from 1msec to 2 sec.
     * Half of the time we will not do a sleep here. 
     */
    if ((fbe_random() % 2) == 1){
        fbe_api_sleep((fbe_random() % 2000) + 1);
    }
    /* This code tests that the rg get info code does the right things to return 
     * the rebuild logging status for these drive(s). 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging ==");
    big_bird_wait_for_rgs_rb_logging(rg_config_p, raid_group_count, 1 /* Drives to wait for */);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add error records. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)){
            current_rg_config_p++;
            continue;
        }
        ernie_add_error_record(current_rg_config_p);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks to pause in clear and eval rb logging.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                       FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        current_rg_config_p++;
    }

    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Reinsert the first drive.");
    big_bird_insert_all_drives(rg_config_p, raid_group_count, 1, /* Number of drives to insert */
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hook that we are clear rb logging");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                       FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Enable error injection.");
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Send one I/O so the raid group hits an error. ");

    fbe_api_rdgen_io_params_init(&params);
    params.object_id = FBE_OBJECT_ID_INVALID;
    params.class_id = FBE_CLASS_ID_LUN;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    params.b_async = FBE_TRUE;
    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    status = fbe_api_rdgen_send_one_io_params(&context_p[0], &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hook that we are eval rb logging");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hook so eval rb logging continues.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                       FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hook that eval RL is done.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for I/O to complete.");
    
    status = fbe_api_rdgen_wait_for_ios(&context_p[0], FBE_PACKAGE_ID_SEP_0, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_api_rdgen_test_context_destroy(&context_p[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuild logging set on second failed drive.==");
    big_bird_wait_for_rgs_rb_logging(rg_config_p, raid_group_count, 1 /* Drives to wait for */);

    mut_printf(MUT_LOG_TEST_STATUS, "== Read back and check pattern.");
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = FBE_OBJECT_ID_INVALID;
    params.class_id = FBE_CLASS_ID_LUN;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    params.b_async = FBE_TRUE;
    params.rdgen_operation = FBE_RDGEN_OPERATION_READ_CHECK;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    status = fbe_api_rdgen_send_one_io_params(&context_p[0], &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
     
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for I/O to complete.");
    status = fbe_api_rdgen_wait_for_ios(&context_p[0], FBE_PACKAGE_ID_SEP_0, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_test_context_destroy(&context_p[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**********************************************************
 * end ernie_run_simultaneous_insert_and_remove()
 **********************************************************/

/*!**************************************************************
 * ernie_setup_520()
 ****************************************************************
 * @brief
 *  Setup for a raid 6 degraded I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/4/2014 - Created. Rob Foley
 *
 ****************************************************************/
void ernie_setup_520(void)
{
    /* disable sector tracing debug for now, Rally DE120 */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        rg_config_p = &ernie_raid_group_config_extended[0];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        /* Note that we do not randomize the block size on purpose.
         * This test injects errors for specific pbas and currently this is only working for 520. 
         */
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will use while running test
            in simulation and on hardware */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive
     * with a different serial number in the same slot. 
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();
    
    return;
}
/**************************************
 * end ernie_setup_520()
 **************************************/
/*!**************************************************************
 * ernie_simultaneous_insert_and_remove_test()
 ****************************************************************
 * @brief
 *  Run a test where we fail one drive and insert another at the same time.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/16/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_simultaneous_insert_and_remove_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)2, /* drives to remove */ ernie_run_simultaneous_insert_and_remove,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_simultaneous_insert_and_remove_test()
 ******************************************/

/*!**************************************************************
 * ernie_double_degraded_zero_abort_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_double_degraded_zero_abort_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)2, /* drives to remove */ big_bird_zero_and_abort_io_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_double_degraded_zero_abort_test()
 ******************************************/

/*!**************************************************************
 * ernie_shutdown_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_shutdown_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1, /* drives to remove */ big_bird_run_shutdown_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_shutdown_test()
 ******************************************/

/*!**************************************************************
 * ernie_single_degraded_encryption_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_single_degraded_encryption_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1, /* drives to remove */ big_bird_normal_degraded_encryption_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_single_degraded_encryption_test()
 ******************************************/
/*!**************************************************************
 * ernie_double_degraded_encryption_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 6 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_double_degraded_encryption_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &ernie_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &ernie_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)2, /* drives to remove */ big_bird_normal_degraded_encryption_test,
                                                   ERNIE_TEST_LUNS_PER_RAID_GROUP,
                                                   ERNIE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end ernie_double_degraded_encryption_test()
 ******************************************/
/*!**************************************************************
 * ernie_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 6 degraded I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void ernie_setup(void)
{
    /* disable sector tracing debug for now, Rally DE120 */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &ernie_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &ernie_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will use while running test
            in simulation and on hardware */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive
     * with a different serial number in the same slot. 
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();
    
    return;
}
/**************************************
 * end ernie_setup()
 **************************************/

/*!**************************************************************
 * ernie_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ernie test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore sector tracing debug, Rally DE111 */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    big_bird_cleanup();
    return;
}
/******************************************
 * end ernie_cleanup()
 ******************************************/
void ernie_dualsp_single_degraded_test(void)
{
    ernie_single_degraded_test();
}
void ernie_dualsp_single_degraded_zero_abort_test(void)
{
    ernie_single_degraded_zero_abort_test();
}
void ernie_dualsp_double_degraded_test(void)
{
    ernie_double_degraded_test();
}
void ernie_dualsp_double_degraded_zero_abort_test(void)
{
    ernie_double_degraded_zero_abort_test();
}       
void ernie_dualsp_shutdown_test(void)
{
    ernie_shutdown_test();
    return;
}
void ernie_dualsp_single_degraded_encryption_test(void)
{
    ernie_single_degraded_encryption_test();
}
void ernie_dualsp_double_degraded_encryption_test(void)
{
    ernie_double_degraded_encryption_test();
}
/*!**************************************************************
 * ernie_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 6 degraded I/O test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  4/27/2011 - Created. Rob Foley
 *
 ****************************************************************/
void ernie_dualsp_setup(void)
{
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* disable sector tracing debug for now, Rally DE120 */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    if (fbe_test_util_is_simulation())
    { 
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &ernie_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &ernie_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
           simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

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
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms_both_sps(NULL);
        }

    }

    /* This test will pull a drive out and insert a new drive
     * with a different serial number in the same slot. 
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();
 
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* The shutdown test causes lots of inconsistency errors, which we need to suppress.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end ernie_dualsp_setup()
 ******************************************/

void ernie_dualsp_encryption_setup(void)
{
    fbe_status_t status;
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    ernie_dualsp_setup();
    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
void ernie_encryption_setup(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    ernie_setup();
}
/*!**************************************************************
 * ernie_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ernie test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/27/2011 - Created. Rob Foley
 *
 ****************************************************************/

void ernie_dualsp_cleanup(void)
{
    /* restore sector tracing debug, Rally DE120 */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    big_bird_dualsp_cleanup();

    return;
}
/******************************************
 * end ernie_dualsp_cleanup()
 ******************************************/
/*************************
 * end file ernie_test.c
 *************************/


