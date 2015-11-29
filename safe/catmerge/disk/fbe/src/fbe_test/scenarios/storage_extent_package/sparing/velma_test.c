/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file velma_test.c
 ***************************************************************************
 *
 * @brief
 *  Test for copy sparing rules.
 *
 * @author
 *  6/5/2014 - Created. Rob Foley
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
#include "sep_test_background_ops.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * velma_short_desc = "Test of the sparing rules for Proactive Copy and User Copy.";
char * velma_long_desc ="\
This test will test the various sparing rules for PACO and User Copy.\n\
\n\
* Test the copy of one 520 drive to another 520 drive.\n\
* Test the copy of one 4k drive to another 4k drive.\n\
* Test the copy of one 520 drive to a 4k drive.\n\
* Test the copy of one 4k drive to a 520 drive.\n";

enum velma_defines_e {
    VELMA_TEST_LUNS_PER_RAID_GROUP = 1,

    /* A few extra chunks beyond what we absoltely need for the LUNs. */
    VELMA_EXTRA_CHUNKS = 32,

    /* Blocks needed for our test cases.  To test all the transition cases we need at least 4
     * blocks worth of paged. 
     */  
    VELMA_CHUNKS_PER_LUN = 3,
    
    VELMA_BLOCKS_PER_RG = ((VELMA_CHUNKS_PER_LUN + VELMA_EXTRA_CHUNKS) * 0x800) + FBE_TEST_EXTRA_CAPACITY_FOR_METADATA,

    VELMA_NUM_EXTRA_DRIVES_PER_RG = 2,
};


/*!*******************************************************************
 * @var velma_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t velma_raid_group_config[] = 
{
    {
        /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/

        {3,       VELMA_BLOCKS_PER_RG,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,  FBE_BLOCK_SIZES_520,            2,          0},
        {2,       VELMA_BLOCKS_PER_RG,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  FBE_BLOCK_SIZES_4160,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

/***************************************** 
 * FORWARD FUNCTION DECLARATIONS
 *****************************************/
/*!**************************************************************
 * velma_get_unused_drive_by_block_size()
 ****************************************************************
 * @brief
 *  Send an operation to the entire capacity of all spares in the system.
 *   
 * @param operation - Rdgen operation to start.
 * @param pattern - Rdgen pattern to use.        
 *
 * @return None.
 *
 * @author
 *  6/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void velma_get_unused_drive_by_block_size(fbe_test_block_size_t block_size,
                                          fbe_test_raid_group_disk_set_t *found_spare_info_p)
{
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_test_discovered_drive_locations_t unused_pvd_locations;
    fbe_drive_type_t drive_type_index;
    fbe_bool_t b_found = FBE_FALSE;

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);
    fbe_test_sep_util_get_all_unused_pvd_location(&drive_locations, &unused_pvd_locations);

    for (drive_type_index = 0; drive_type_index < FBE_DRIVE_TYPE_LAST; drive_type_index++) {
        if (unused_pvd_locations.drive_counts[block_size][drive_type_index] > 0) {
            *found_spare_info_p = unused_pvd_locations.disk_set[block_size][drive_type_index][0];
            b_found = FBE_TRUE;
            break;
        }
    }

    if (!b_found) {
        mut_printf(MUT_LOG_TEST_STATUS, "== Could not find unused drive with block size %d", block_size);
        MUT_FAIL();
    }
}
/******************************************
 * end velma_get_unused_drive_by_block_size()
 ******************************************/
/*!**************************************************************
 * velma_copy_mixed_test()
 ****************************************************************
 * @brief
 *  Test sparing rules.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void velma_copy_mixed_test(fbe_test_rg_configuration_t *rg_config_p, 
                     fbe_block_size_t copy_to_block_size,
                     fbe_spare_swap_command_t copy_type)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t spare_info;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t rg_object_id;

    velma_get_unused_drive_by_block_size(copy_to_block_size, &spare_info);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hook to pause request event.");
    status = fbe_test_use_rg_hooks(rg_config_p,
                                   1,    /* one ds object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EVENT,
                                   FBE_RAID_GROUP_SUBSTATE_EVENT_COPY_BL_REFRESH_NEEDED,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hook to pause refresh of bl/s.");
    status = fbe_test_use_rg_hooks(rg_config_p,
                                   1,    /* one ds object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                   FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hook to detect refresh of bl/s.");
    status = fbe_test_use_rg_hooks(rg_config_p,
                                   1,    /* one ds object */
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                   FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED,
                                   0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                   FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== rg block size: %d bitmask_4k: 0x%x",
               rg_config_p->block_size, rg_info.bitmask_4k);

    if (rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
        if (rg_info.bitmask_4k != (1 << rg_config_p->width) - 1) {
            MUT_FAIL_MSG("4k RG bitmask does not match width");
        }
    } else {
        if (rg_info.bitmask_4k != 0) {
            MUT_FAIL_MSG("4k RG bitmask is not 0 for 520 RG");
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting user copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              1, /* Num RGs */
                                                              0, /* Position to copy */
                                                              copy_type,
                                                              &spare_info /* Specify the destination drives */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__);

    if (rg_config_p->block_size == FBE_BLOCK_SIZES_520) {
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hook to pause request event.");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVENT,
                                       FBE_RAID_GROUP_SUBSTATE_EVENT_COPY_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hook to pause refresh of bl/s.");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Remove hooks to let block size get refreshed");

        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for refresh of bl/s.");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Validate bl/s is refreshed properly.");

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== rg block size: %d bitmask_4k: 0x%x",
                   rg_config_p->block_size, rg_info.bitmask_4k);

        /* For 4k->520 VD it should still report worst case (source).
         */
        if (rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
            fbe_raid_position_bitmask_t all_bits = ((1 << rg_config_p->width) - 1);
            if (rg_info.bitmask_4k != all_bits) {
                MUT_FAIL_MSG("4k RG bitmask not set for all positions");
            }
        } else {
            /* For 520->4k RG, it should already report worst case (destination)
             */
            if (rg_info.bitmask_4k != 1) {
                MUT_FAIL_MSG("4k RG bitmask is not 1 for 520 RG");
            }
        }

        mut_printf(MUT_LOG_TEST_STATUS, "== Remove hooks to let COPY run.");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVENT,
                                       FBE_RAID_GROUP_SUBSTATE_EVENT_COPY_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Confirm that bl/s is still old size for RG.");

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          1, /* Num RGs */
                                                                          0, /* Position to copy */
                                                                          copy_type,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
        
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for hook to pause refresh of bl/s.");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Remove hooks to let block size get refreshed");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVENT,
                                       FBE_RAID_GROUP_SUBSTATE_EVENT_COPY_BL_REFRESH_NEEDED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for refresh of bl/s.");
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
                                       FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== rg block size: %d bitmask_4k: 0x%x",
               rg_config_p->block_size, rg_info.bitmask_4k);

    /* Make sure the appropriate bit was added or removed for the drive we just swapped in.
     */
    if (rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
        fbe_raid_position_bitmask_t all_bits = ((1 << rg_config_p->width) - 1);
        if (rg_info.bitmask_4k != (all_bits & (~1))) {
            MUT_FAIL_MSG("4k RG bitmask not cleared for position 0");
        }
    } else {
        if (rg_info.bitmask_4k != 1) {
            MUT_FAIL_MSG("4k RG bitmask is not 1 for 520 RG");
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);
}
/**************************************
 * end velma_copy_mixed_test()
 **************************************/
/*!**************************************************************
 * velma_copy_test()
 ****************************************************************
 * @brief
 *  Test sparing rules.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void velma_copy_test(fbe_test_rg_configuration_t *rg_config_p, 
                     fbe_block_size_t copy_to_block_size,
                     fbe_spare_swap_command_t copy_type)
{
    fbe_status_t status;
    fbe_test_raid_group_disk_set_t spare_info;
    fbe_test_raid_group_disk_set_t other_block_size_spare_info;
    fbe_block_size_t other_block_size_copy_to;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t rg_object_id;

    velma_get_unused_drive_by_block_size(copy_to_block_size, &spare_info);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    if(copy_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND) {

        /* The purpose of this is to make sure there are drives on the same type and then drives of the different type
         * are available. We need to make sure the drive of the same block size is picked 
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting Proactive copy ==", __FUNCTION__);

        /* Make sure the other block size drive is available */
        other_block_size_copy_to = (copy_to_block_size == FBE_TEST_BLOCK_SIZE_4160) ? FBE_TEST_BLOCK_SIZE_520 : FBE_TEST_BLOCK_SIZE_4160;
        velma_get_unused_drive_by_block_size(other_block_size_copy_to, &other_block_size_spare_info);
        status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                                  1, /* Num RGs */
                                                                  0, /* Position to copy */
                                                                  copy_type,
                                                                  NULL);
    } else {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting User copy ==", __FUNCTION__);
        status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                                  1, /* Num RGs */
                                                                  0, /* Position to copy */
                                                                  copy_type,
                                                                  &spare_info /* Specify the destination drives */);
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          1, /* Num RGs */
                                                                          0, /* Position to copy */
                                                                          copy_type,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Since the block size for this copy operation is of the same drives, the block size of the RG should not have changed */
    if (rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
        if (rg_info.bitmask_4k != (1 << rg_config_p->width) - 1) {
            MUT_FAIL_MSG("4k RG bitmask does not match width");
        }
    } else {
        if (rg_info.bitmask_4k != 0) {
            MUT_FAIL_MSG("4k RG bitmask is not 0 for 520 RG");
        }
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);
}
/**************************************
 * end velma_copy_test()
 **************************************/
/*!**************************************************************
 * velma_test_rg_config()
 ****************************************************************
 * @brief
 *  Test sparing rules.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

void velma_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== First copy to like block size. ");
        if (current_rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
            mut_printf(MUT_LOG_TEST_STATUS, "== User Copy/Proactive Copy 4k -> 4k ");
            velma_copy_test(current_rg_config_p, 
                            FBE_TEST_BLOCK_SIZE_4160,
                            FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND);
            velma_copy_test(current_rg_config_p,
                            FBE_TEST_BLOCK_SIZE_4160,
                            FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND);
        } else {
            mut_printf(MUT_LOG_TEST_STATUS, "== User Copy/Proactive Copy 520 -> 520 ");
            velma_copy_test(current_rg_config_p, 
                            FBE_TEST_BLOCK_SIZE_520,
                            FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND);
            velma_copy_test(current_rg_config_p,
                            FBE_TEST_BLOCK_SIZE_520,
                            FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND);

        }
        /* Initiate copy to opposite block size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Next copy to different block size. ");
        if (current_rg_config_p->block_size == FBE_BLOCK_SIZES_4160) {
            mut_printf(MUT_LOG_TEST_STATUS, "== Copy 4k -> 520 ");
            velma_copy_mixed_test(current_rg_config_p, FBE_TEST_BLOCK_SIZE_520,
                            FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND);
        } else {
            mut_printf(MUT_LOG_TEST_STATUS, "== Copy 520 -> 4k ");
            velma_copy_mixed_test(current_rg_config_p, FBE_TEST_BLOCK_SIZE_4160, 
                            FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND);
        }
        current_rg_config_p++;
    }
}
/**************************************
 * end velma_test_rg_config()
 **************************************/
/*!**************************************************************
 * velma_populate_num_extra_drives()
 ****************************************************************
 * @brief
 *  Add enough extra drives for the copies we will perform.
 *
 * @param rg_config_p - Raid group config.
 *
 * @return -   
 *
 * @author
 *  6/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void velma_populate_num_extra_drives(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups;
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        current_rg_config_p->num_of_extra_drives = VELMA_NUM_EXTRA_DRIVES_PER_RG;
        current_rg_config_p++;
    }
}
/******************************************
 * end velma_populate_num_extra_drives()
 ******************************************/
/*!**************************************************************
 * velma_setup()
 ****************************************************************
 * @brief
 *  Setup for an encryption test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/5/2014 - Created. Rob Foley
 *
 ****************************************************************/
void velma_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    /* This will automatically setup the seed if it has not already been set.
     */
    fbe_test_init_random_seed();

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;
        
        rg_config_p = &velma_raid_group_config[0][0]; 
        array_p = &velma_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Use simulator PSL so that it does not take forever and a day to zero the system drives.
         */
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);

        velma_populate_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        elmo_load_sep_and_neit();
    }
    /* We inject errors on purpose, so we do not want to stop on any sector trace errors.
     */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

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
 * end velma_setup()
 **************************************/

/*!**************************************************************
 * velma_test()
 ****************************************************************
 * @brief
 *  Test for Copy Sparing Rules.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

void velma_test(void)
{
    fbe_test_create_raid_group_params_t params;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "starting %s", __FUNCTION__);

    rg_config_p = &velma_raid_group_config[0][0];

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "== enable system drive zeroing ==");
    fbe_test_sep_util_enable_system_drive_zeroing();
    fbe_test_enable_rg_background_ops_system_drives();

    fbe_test_create_raid_group_params_init_for_time_save(&params,
                                                         NULL,
                                                         velma_test_rg_config,
                                                         VELMA_TEST_LUNS_PER_RAID_GROUP,
                                                         VELMA_CHUNKS_PER_LUN,
                                                         FBE_TRUE /* don't destroy config */);
    params.b_encrypted = FBE_FALSE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
}
/******************************************
 * end velma_test()
 ******************************************/

/*!**************************************************************
 * velma_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the velma test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/5/2014 - Created. Rob Foley
 *
 ****************************************************************/

void velma_cleanup(void)
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
 * end velma_cleanup()
 ******************************************/


/*************************
 * end file velma_test.c
 *************************/


