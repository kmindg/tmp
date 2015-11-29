/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file zoe_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid 1 objects.
 *
 * @version
 *  09/17/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * zoe_short_desc = "raid-1 - Read, Write, Verify-Write I/O Tests";
char * zoe_long_desc =
    "\n"
    "\n"
    "The Zoe scenario tests raid-1 non-degraded I/O.  This includes 2 and 3 drive raid-1.\n"
    "   - raid_test_level 0 and 1\n"
    "   - We test additional combinations of raid group widths and element sizes at -rtl 1.\n"
    "\n"
    "STEP 1: configure all raid groups and LUNs.\n"
    "   - We always test at least one rg with 512 block size drives.\n"
    "   - All raid groups have 3 LUNs each\n"
    "\n"
    "STEP 2: write a background pattern to all luns and then read it back and make sure it \n"
    "        was written successfully\n"
    "\n"
    "STEP 3: Peform the standard I/O test sequence.\n"
    "   - Perform write/read/compare for small I/os.\n"
    "   - Perform verify-write/read/compare for small I/Os.\n"
    "   - Perform verify-write/read/compare for large I/Os.\n"
    "   - Perform write/read/compare for large I/Os.\n"
    "\n"
    "STEP 4: Peform the standard I/O test sequence above with aborts.\n"
    "\n"
    "STEP 5: Peform the caterpillar I/O sequence.\n"
    "   - Caterpillar is multi-threaded sequential I/O that causes\n"
    "     stripe lock contention at the raid group.\n"
    "   - Perform write/read/compare for small I/os.\n"
    "   - Perform verify-write/read/compare for small I/Os.\n"
    "   - Perform verify-write/read/compare for large I/Os.\n"
    "   - Perform write/read/compare for large I/Os.\n"
    "\n"
    "STEP 6: Run the block operation usurper test.\n"
    "   - Issues single small I/Os for every LUN and every operation type.\n"
    "     - Operations tested on the LUN include:\n"
    "       - write, corrupt data, corrupt crc, verify, zero.\n"
    "     - Operations tested on the raid group include:\n"
    "       - verify, zero.\n"
    "\n"
    "STEP 7: Run the block operation usurper test (above) with large I/Os.\n"
    "\n"
    "STEP 8: Destroy raid groups and LUNs.\n"
    "\n"
    "Test Specific Switches:\n"
    "   -abort_cases_only - Only run the abort tests.\n"
    "\n"
    "Description last updated: 10/03/2011.\n";

/*!*******************************************************************
 * @def ZOE_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define ZOE_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def ZOE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ZOE_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def ZOE_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ZOE_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def ZOE_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ZOE_LARGE_IO_SIZE_MAX_BLOCKS ZOE_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def ZOE_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define ZOE_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def ZOE_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define ZOE_TEST_BLOCK_OP_MAX_BLOCK 4096

/*!*******************************************************************
 * @var zoe_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t zoe_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/*!*******************************************************************
 * @var zoe_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t zoe_raid_group_config_extended[] = 
{
    /* width, capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},

    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0},

    {2,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            4,          1},
    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            5,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * zoe_io_during_verify_test()
 ****************************************************************
 * @brief
 *  Add scheduler hook for verify.
 *  Kick off a verify.
 *  Run Read I/O to our mirrors.
 *  Validate that I/O only went to the primary.
 *
 * @param element_size - size in blocks of element.   
 * @param rdgen_operation - type of operation to run with rdgen.             
 *
 * @return None.
 *
 * @author
 *  3/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_io_during_verify_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_contexts = 1;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t lun_index;
    fbe_object_id_t lun_object_id;
    fbe_api_rdgen_context_t context;
    fbe_lun_performance_counters_t stats;
    fbe_perfstats_lun_sum_t        summed_stats; 
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Mark as needing a verify.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate verify on all raid groups. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* we need to set hooks on the active SP */
        status = fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_sim_transport_set_target_server(active_sp);
         
        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }
    // Restore original target.
    fbe_api_sim_transport_set_target_server(current_target);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate verify on all raid groups. (successful)==", __FUNCTION__);

    /* Enable performance stats on all LUNs.  We'll be using the stats to tell if we did reads to the secondary.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < ZOE_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_lun_enable_peformance_stats(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /*! @note Currently this test doesn't support peer I/O
     */
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_perfstats_zero_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't zero stat container!");

    /* Setup the test context.
     */
    fbe_test_sep_io_setup_standard_rdgen_test_context(&context,
                                                      FBE_OBJECT_ID_INVALID,
                                                      FBE_CLASS_ID_LUN,
                                                      FBE_RDGEN_OPERATION_READ_ONLY,
                                                      FBE_LBA_INVALID,    /* use capacity */
                                                      100,    /* passes to run */ 
                                                      5,    /* threads per lun */
                                                      ZOE_SMALL_IO_SIZE_MAX_BLOCKS,
                                                      FBE_FALSE,    /* Don't inject aborts */
                                                      FBE_FALSE    /* Peer I/O isn't enabled */);
    /* Run our I/O test.
     * Just reads so we can see if the reads hit the secondary. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate read I/O on all raid groups. ==", __FUNCTION__);
    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate read I/O on all raid groups (successful).==", __FUNCTION__);

    /* Now make sure that our reads only went to the primary.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate reads went to primary position only. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < ZOE_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_lun_get_performance_stats(lun_object_id,
                                              &stats);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


            //sum stats
            status = fbe_api_perfstats_get_summed_lun_stats(&summed_stats,
                                                            &stats);

            if ((summed_stats.disk_reads[1] != 0) || (summed_stats.disk_blocks_read[1] != 0))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%d reads (%lld blocks) to pos 1 ", (int)summed_stats.disk_reads[1], (long long)summed_stats.disk_blocks_read[1]);
                MUT_FAIL_MSG("unexpected read count");
            }
            if (current_rg_config_p->width > 2)
            {
                if ((summed_stats.disk_reads[2] != 0) || (summed_stats.disk_blocks_read[2] != 0))
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "%d reads (%lld blocks) to pos 1 ", 
                               (int)summed_stats.disk_reads[2], (long long)summed_stats.disk_blocks_read[2]);
                    MUT_FAIL_MSG("unexpected read count");
                }
            }

            status = fbe_api_lun_disable_peformance_stats(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate reads went to primary position only (successful). ==", __FUNCTION__);

    current_rg_config_p = rg_config_p;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    for ( index = 0; index < raid_group_count; index++)
    {
         if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* we need to delete hooks on the active SP */
        status = fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_sim_transport_set_target_server(active_sp);

        
        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    // Restore original target.
    fbe_api_sim_transport_set_target_server(current_target);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end zoe_io_during_verify_test()
 ******************************************/
/*!**************************************************************
 * zoe_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 1 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_bool_t b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");

    fbe_test_raid_group_get_info(rg_config_p);

    /* Run I/O to this set of raid groups with the I/O sizes specified
     * (no aborts and no peer I/Os)
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          ZOE_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ZOE_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                            /* Don't inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Now run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      ZOE_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      ZOE_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_TRUE,                             /* Inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Now run caterpillar I/Os and periodically abort some outstanding requests.
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
                                          ZOE_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ZOE_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                             /* Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

        /* We do not expect any errors.
         */
        fbe_test_sep_util_validate_no_raid_errors_both_sps();

        if (!b_run_dualsp_io)
        {
            zoe_io_during_verify_test(rg_config_p);
        }
    }
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end zoe_test_rg_config()
 ******************************************/

/*!**************************************************************
 * zoe_test_rg_config_block_ops()
 ****************************************************************
 * @brief
 *  Run a raid 1 block operations test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_test_rg_config_block_ops(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
                                      ZOE_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      ZOE_TEST_BLOCK_OP_MAX_BLOCK,
                                      FBE_FALSE,    /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      FBE_FALSE,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID); 
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end zoe_test_rg_config_block_ops()
 ******************************************/
/*!**************************************************************
 * zoe_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 1 objects on a single SP.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &zoe_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &zoe_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, zoe_test_rg_config,
                                   ZOE_TEST_LUNS_PER_RAID_GROUP,
                                   ZOE_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end zoe_test()
 ******************************************/

/*!**************************************************************
 * zoe_block_opcode_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 1 objects on a single SP.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_block_opcode_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &zoe_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &zoe_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, zoe_test_rg_config_block_ops,
                                   ZOE_TEST_LUNS_PER_RAID_GROUP,
                                   ZOE_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end zoe_test()
 ******************************************/

/*!**************************************************************
 * zoe_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 1 objects on both SPs
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_dualsp_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &zoe_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &zoe_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, zoe_test_rg_config,
                                   ZOE_TEST_LUNS_PER_RAID_GROUP,
                                   ZOE_CHUNKS_PER_LUN);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end zoe_dualsp_test()
 ******************************************/

/*!**************************************************************
 * zoe_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test on single sp.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void zoe_setup(void)
{    
    mut_printf(MUT_LOG_TEST_STATUS, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level > 0)
        {
            rg_config_p = &zoe_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &zoe_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p,
                         ZOE_TEST_LUNS_PER_RAID_GROUP,
                         ZOE_CHUNKS_PER_LUN);
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *        the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end zoe_setup()
 **************************************/

/*!**************************************************************
 * zoe_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void zoe_dualsp_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {        
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        if (test_level > 0)
        {
            rg_config_p = &zoe_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &zoe_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
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
        
        sep_config_load_sep_and_neit_load_balance_both_sps();

	}
    
    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *        the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/**************************************
 * end zoe_dualsp_setup()
 **************************************/

/*!**************************************************************
 * zoe_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the zoe test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end zoe_cleanup()
 ******************************************/

/*!**************************************************************
 * zoe_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the zoe test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void zoe_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s entry", __FUNCTION__);

    fbe_test_sep_util_print_dps_statistics();

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
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end zoe_dualsp_cleanup()
 ******************************************/

/*************************
 * end file zoe_test.c
 *************************/


