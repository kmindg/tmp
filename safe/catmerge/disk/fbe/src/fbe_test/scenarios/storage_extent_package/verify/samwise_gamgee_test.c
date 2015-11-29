/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file samwise_gamgee_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of priority between rebuild operations
 *  and sniff, zero, copy operations.
 * 
 *  This test also degrades/shuts down raid groups and tests the priority
 *  of the raid group state versus other operations like zero, sniff and copy.
 * 
 *  This test also has a few tasks for copy priority and zero or sniff.
 *  These tests did not belong in any other test so we added them here.
 *
 * @version
 *  4/18/2012 - Created. Rob Foley
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

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * samwise_gamgee_short_desc = "Priority of rg rebuild/degraded/shutdown versus other operations.";
char * samwise_gamgee_long_desc = "\
The samwise_gamgee scenario tests the priority between a raid group that is degraded/shutdown/rebuilding \n\
and other background operations (sniff, copy and zeroing).\n\
There are also other tests that test copy priority versus sniff and zeroing.\n\
\n\
   1) Test that a copy does not make progress on a shutdown raid group. \n\
   2) Test that zero does not make progress on a shutdown raid group. \n\
   3) Test that sniff does not make progress on a shutdown raid group. \n\
   4) Test that a copy does make progress on a degraded raid group. \n\
   5) Test that zero does not make progress on a degraded raid group. \n\
   6) Test that sniff does not make progress on a degraded raid group. \n\
   7) Test that a copy does not make progress on a rebuilding raid group. \n\
   8) Test that zero does not make progress on a rebuilding raid group. \n\
   9) Test that sniff does not make progress on a rebuilding raid group. \n\
   10) Test that sniff does not proceed during a copy. \n\
   11) Test that zero does not proceed during a copy \n\
                                 \n\
Description last updated: 4/18/2012.\n";


/*!*******************************************************************
 * @def SAMWISE_GAMGEE_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def SAMWISE_GAMGEE_MAX_WAIT_SEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_MAX_WAIT_SEC 120

/*!*******************************************************************
 * @def SAMWISE_GAMGEE_MAX_WAIT_MSEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_MAX_WAIT_MSEC SAMWISE_GAMGEE_MAX_WAIT_SEC * 1000

/*!*******************************************************************
 * @def SAMWISE_GAMGEE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def SAMWISE_GAMGEE_DEFAULT_OFFSET
 *********************************************************************
 * @brief The default offset on the disk where we create rg.
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_DEFAULT_OFFSET 0x10000

/*!*******************************************************************
 * @def SAMWISE_GAMGEE_COPY_COUNT
 *********************************************************************
 * @brief This is the number of proactive copies we will do.
 *        This is also the number of extra drives we need for each rg.
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_COPY_COUNT 7

/*!*******************************************************************
 * @def SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX
 *********************************************************************
 * @brief The index of the raid 10 index in the below table.
 *
 *********************************************************************/
#define SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX 0

/*!*******************************************************************
 * @var SAMWISE_GAMGEE_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 * 
 * @todo random parity is not enabled since copy has issues with R6.
 *
 *********************************************************************/
fbe_test_rg_configuration_t samwise_gamgee_raid_group_config[] = 
{
    /* width,                        capacity    raid type,                  class,             block size   RAID-id.  bandwidth. */

    /* Raid  10 must be at a particular index, don't change without modifying the above  SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX */
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,   520,            1,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,   520,            2,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY,   FBE_CLASS_ID_PARITY,   520,            1,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,   520,            3, 0}, 

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);
/*!**************************************************************
 * samwise_gamgee_validate_rb_chkpt_is_zero()
 ****************************************************************
 * @brief
 *  Validate that we have at least and only one non-invalid chkpt.
 *
 * @param  rg_object_id - RG object to check the rb checkpoints of.
 *
 * @return None.
 *
 * @author
 *  4/19/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_validate_rb_chkpt_is_zero(fbe_object_id_t rg_object_id,
                                              fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_lba_t                       chkpt = FBE_LBA_INVALID;
    fbe_u32_t                       num_found = 0;
    fbe_u32_t                       rg_position;
    fbe_u32_t                       width = rg_config_p->width;
    fbe_u32_t                       wait_interval_ms = 100;
    fbe_u32_t                       total_wait_time_ms = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_WAIT_TIMEOUT_MS;
    fbe_bool_t                      b_checkpoint_zero = FBE_FALSE;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* We're actually looking at just one mirror.
         */
        width = 2;
    }

    /* While we either validated that we are degraded or the timeout is reached.
     */
    while ((b_checkpoint_zero == FBE_FALSE)   &&
           (total_wait_time_ms <= timeout_ms)     )
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get the minimum rebuild checkpoint.
         */
        num_found = 0;
        chkpt = FBE_LBA_INVALID;
        for (rg_position = 0; rg_position < width; rg_position++)
        {
            if (rg_info.rebuild_checkpoint[rg_position] != FBE_LBA_INVALID)
            {
                chkpt = rg_info.rebuild_checkpoint[rg_position];
                num_found++;
            }
        }

        /* If there were any found, validate exactly (1) found.
         */
        if (num_found > 0)
        {
            if (num_found > 1)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "found %d chkpts\n", num_found);
                MUT_FAIL_MSG("We should only find one checkpoint");
                return;
            }

            if (chkpt != 0)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "chkpt is 0x%llx not 0\n", chkpt);
                MUT_FAIL_MSG("rebuild checkpoint should be 0");
                return;
            }

            /* Just break
             */
            b_checkpoint_zero = FBE_TRUE;
            break;
        }

        /* Always wait a second time */
        fbe_api_sleep(wait_interval_ms);
        total_wait_time_ms += wait_interval_ms;

    } /* while the checkpoint isn't 0 or timeout*/

    /* If the not found timeout.
     */
    if (b_checkpoint_zero == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "timeout chkpt is 0x%llx not 0\n", chkpt);
        MUT_FAIL_MSG("rebuild checkpoint should be 0");
        return;
    }

    /* Success*/
    return;
}
/******************************************
 * end samwise_gamgee_validate_rb_chkpt_is_zero()
 ******************************************/
/*!**************************************************************
 * samwise_gamgee_setup_for_degraded_op()
 ****************************************************************
 * @brief
 *  Randomly decide which position to monitor when another
 *  position is already degraded.
 *
 * @param  rg_config_p - current test config.
 * @param vd_object_ids_p - array of vd object ids one per rg.
 *
 * @return None.
 *
 * @author
 *  4/16/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_setup_for_degraded_op(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_object_id_t *rg_object_id_p,
                                           fbe_object_id_t *vd_object_id_p,
                                           fbe_object_id_t *pvd_object_id_p)
{   
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t top_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t rg_position = 0;
    fbe_u32_t disk_index = 0;

    /* Get another random position to use for the background op.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t mirror_index;
        /* We need to monitor the other half of the mirror to make the mirror degraded.
         */
        if (rg_config_p->drives_removed_array[0] % 2)
        {
            rg_position = rg_config_p->drives_removed_array[0] - 1;
        }
        else
        {
            rg_position = rg_config_p->drives_removed_array[0] + 1;
        }
        /* We need to calculate our rg object id as the mirror object id 
         * for the position that was removed. 
         */
        mirror_index = rg_config_p->drives_removed_array[0] / 2;
        fbe_test_get_rg_ds_object_id(rg_config_p, &rg_object_id, mirror_index); 
        *rg_object_id_p = rg_object_id;
    }
    else
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
        *rg_object_id_p = rg_object_id;
        rg_position = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    }
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[rg_position].bus,
                                                            rg_config_p->rg_disk_set[rg_position].enclosure,
                                                            rg_config_p->rg_disk_set[rg_position].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(pvd_object_id, FBE_OBJECT_ID_INVALID);
    pvd_object_id_p[disk_index] = pvd_object_id;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &top_rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(top_rg_object_id, rg_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    *vd_object_id_p = vd_object_id;
}
/******************************************
 * end samwise_gamgee_setup_for_degraded_op()
 ******************************************/

/*!**************************************************************
 * samwise_gamgee_sniff_during_copy_test()
 ****************************************************************
 * @brief
 *  Perform a copy and make sure sniff does not proceed.
 * 
 * @param rg_config_p - Config array to use.  This is null terminated.
 * 
 * @return None.
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_sniff_during_copy_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_provision_drive_get_verify_status_t verify_status;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);
        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);

        /* Add debug hook to prevent copy from proceeding.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start the copy.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy rg: 0x%x pvd: 0x%x==", rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        /* Set checkpoint to 0 so that the sniff is guaranteed to ask soon about sniffing on our raid group.
         * Otherwise it can take a long time for sniffing to advance to this rg. 
         */
        status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_set_verify_checkpoint(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, SAMWISE_GAMGEE_DEFAULT_OFFSET);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        /* Add the hooks we will use below.
         */
        fbe_test_sniff_add_hooks(pvd_object_id);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        
        /* Make sure verify is not running since copy should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate sniff did not make progress pvd: 0x%x==", pvd_object_id);
        fbe_test_sniff_validate_no_progress(pvd_object_id);

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        /* Delete the vd hook to allow copy to run.
         */
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Wait for the copy to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does continue on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_validate_sniff_progress(pvd_object_id);
        fbe_test_sniff_del_hooks(pvd_object_id);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/*********************************************
 * end samwise_gamgee_sniff_during_copy_test()
 *********************************************/

/*!**************************************************************
 * samwise_gamgee_zero_during_copy_test()
 ****************************************************************
 * @brief
 *  Perform a copy and make sure sniff does not proceed.
 * 
 * @param rg_config_p - Config array to use.  This is null terminated.
 * 
 * @return None.
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_zero_during_copy_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);
        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);

        /* Add debug hook to prevent copy from proceeding.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add hook when zero starts.
         */
        status = fbe_test_add_debug_hook_active(pvd_object_id,
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start the copy.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy rg: 0x%x pvd: 0x%x==", rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        /* Add hooks that allow us to know that zeroing is not getting permission to run.
         */
        fbe_test_zero_add_hooks(pvd_object_id);

        /* Start zeroing.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting zero of rg: 0x%x==", rg_object_id);
        fbe_test_sep_util_zero_object_capacity(rg_object_id, 0 /* start lba to zero */);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        status = fbe_test_del_debug_hook_active(pvd_object_id,
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        
        /* Make sure zeroing is not running since copy should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate zeroing did not make progress pvd: 0x%x==", pvd_object_id);
        /* Zeroing should not make progress since verify is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);
        fbe_test_zero_del_hooks(pvd_object_id);

        /* Delete the vd hook to allow copy to run.
         */
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Wait for the copy to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy to complete rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure zeroing does continue on pvd_obj_id: 0x%x ==", 
                   __FUNCTION__, pvd_object_id);

        /* Wait for the zeroing to complete.  When verify is done zeroing should run freely.
         */
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_zero_during_copy_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_rebuild_during_copy_test()
 ****************************************************************
 * @brief
 *  - Start the copy.
 *  - Degrade the raid group.
 *  - Re-insert the drive.  A debug hook prevents it from running.
 *  - Make sure the copy does not proceed.
 *  - Delete the rebuild debug hook, and make sure it finishes.
 *  - Make sure the copy finishes.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_rebuild_during_copy_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 1 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 1);

    /* Determine our destination index.
     */
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
    
        /* Add hook that pauses the copy.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start Copy.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy rg: 0x%x pvd: 0x%x==", rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", rg_object_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */
                               removal_mode);

        samwise_gamgee_setup_for_degraded_op(current_rg_config_p, &rg_object_ids[rg_index],
                                            &vd_object_ids[rg_index], &pvd_object_ids[rg_index]);

        /* Add debug hook that pauses rebuild.  We don't want revuild to finish so we can hold up copy.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
    
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", rg_object_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add hooks that allow us to know that copying is not getting permission to run.
         */
        fbe_test_copy_add_hooks(vd_object_id);

        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Copy should not make progress since verify is running and will deny the zero.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate copy does not progress rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        fbe_test_copy_validate_no_progress(vd_object_id);
        fbe_test_copy_del_hooks(vd_object_id);

        /* Make sure rebuild is not running since debug hook should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate rebuild does not progress rg: 0x%x ==", rg_object_id);
        samwise_gamgee_validate_rb_chkpt_is_zero(rg_object_id, current_rg_config_p);

        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        
        /* Make sure RB ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for the copy to complete.  When rebuild is done copy should run freely.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_rebuild_during_copy_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_rebuild_during_zero_test()
 ****************************************************************
 * @brief
 *  - Degrade the raid group.
 *  - Re-insert the drive.  A debug hook prevents it from running.
 *  - Start the copy.
 *  - Make sure the zero does not proceed.
 *  - Delete the rebuild debug hook, and make sure it finishes.
 *  - Make sure the zero continues.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/16/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_rebuild_during_zero_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_rg_zeroing_complete(current_rg_config_p);

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", rg_object_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */
                               removal_mode);

        samwise_gamgee_setup_for_degraded_op(current_rg_config_p, &rg_object_ids[rg_index],
                                            &vd_object_ids[rg_index], &pvd_object_ids[rg_index]);

        /* Add debug hook that pauses rebuild.  We don't want revuild to finish so we can hold up copy.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
    
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", rg_object_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        mut_printf(MUT_LOG_TEST_STATUS, "== wait for rb logging to get cleared rg: 0x%x==", rg_object_id);
        status = fbe_test_wait_for_debug_hook_active(rg_object_id,
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY, 
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Add hooks that allow us to know that zeroing is not getting permission to run.
         */
        fbe_test_zero_add_hooks(pvd_object_id);

        /* If raid10, we don't want to zero the r10 metadata area. Don't use the mirror under striper id. 
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id); 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Start zeroing.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting zero of rg: 0x%x==", rg_object_id);
        fbe_test_sep_util_zero_object_capacity(rg_object_id, 0 /* start lba to zero */);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure zeroing is not running since rebuild should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate zeroing did not make progress pvd: 0x%x==", pvd_object_id);
        /* Zeroing should not make progress since rebuild is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure rebuild is not running since debug hook should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate rebuild does not progress rg: 0x%x ==", rg_object_id);
        samwise_gamgee_validate_rb_chkpt_is_zero(rg_object_id, current_rg_config_p);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== validate zeroing did not make progress pvd: 0x%x==", pvd_object_id);
        /* Zeroing should not make progress since rebuild is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);
        fbe_test_zero_del_hooks(pvd_object_id);

        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure RB ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure zeroing does continue on pvd_obj_id: 0x%x ==", 
                   __FUNCTION__, pvd_object_id);

        /* Wait for the zeroing to complete.  When rebuild is done zeroing should run freely.
         */
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_rebuild_during_zero_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_rebuild_during_sniff_test()
 ****************************************************************
 * @brief
 *  - Degrade the raid group.
 *  - Re-insert the drive.  A debug hook prevents it from running.
 *  - Make sure the sniff does not proceed.
 *  - Delete the rebuild debug hook, and make sure it finishes.
 *  - Make sure the sniff continues.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/16/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_rebuild_during_sniff_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_get_verify_status_t verify_status;
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_rg_zeroing_complete(current_rg_config_p);

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", rg_object_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */
                               removal_mode);

        samwise_gamgee_setup_for_degraded_op(current_rg_config_p, &rg_object_ids[rg_index],
                                            &vd_object_ids[rg_index], &pvd_object_ids[rg_index]);

        /* Add debug hook that pauses rebuild.  We don't want revuild to finish so we can hold up copy.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
    
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", rg_object_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);


        /* Set checkpoint to 0 so that the sniff is guaranteed to ask soon about sniffing on our raid group.
         * Otherwise it can take a long time for sniffing to advance to this rg. 
         */
        status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_set_verify_checkpoint(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, SAMWISE_GAMGEE_DEFAULT_OFFSET);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        /* Add the hooks we will use below.
         */
        fbe_test_sniff_add_hooks(pvd_object_id);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Make sure sniff is not running since BV should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does not proceed on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_sniff_validate_no_progress(pvd_object_id);

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure rebuild is not running since debug hook should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate rebuild does not progress rg: 0x%x ==", rg_object_id);
        samwise_gamgee_validate_rb_chkpt_is_zero(rg_object_id, current_rg_config_p);

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure RB ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Make sure sniff is running again.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does continue on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_validate_sniff_progress(pvd_object_id);
        fbe_test_sniff_del_hooks(pvd_object_id);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_rebuild_during_sniff_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_degraded_during_sniff_test()
 ****************************************************************
 * @brief
 *  - Degrade the raid group.
 *  - Make sure the sniff does not proceed.
 *  - Delete the rebuild debug hook, and make sure it finishes.
 *  - Make sure the sniff continues.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/16/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_degraded_during_sniff_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_get_verify_status_t verify_status;
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_rg_zeroing_complete(current_rg_config_p);

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", rg_object_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */
                               removal_mode);

        samwise_gamgee_setup_for_degraded_op(current_rg_config_p, &rg_object_ids[rg_index],
                                            &vd_object_ids[rg_index], &pvd_object_ids[rg_index]);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Set checkpoint to 0 so that the sniff is guaranteed to ask soon about sniffing on our raid group.
         * Otherwise it can take a long time for sniffing to advance to this rg. 
         */
        status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_set_verify_checkpoint(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, SAMWISE_GAMGEE_DEFAULT_OFFSET);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        /* Add the hooks we will use below.
         */
        fbe_test_sniff_add_hooks(pvd_object_id);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Make sure sniff is not running since BV should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does not proceed on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_sniff_validate_no_progress(pvd_object_id);

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure rebuild is not running since debug hook should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate rg is degraded rg: 0x%x ==", rg_object_id);
        samwise_gamgee_validate_rb_chkpt_is_zero(rg_object_id, current_rg_config_p);

        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", rg_object_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);
        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure RB ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Make sure sniff is running again.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does continue on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_validate_sniff_progress(pvd_object_id);
        fbe_test_sniff_del_hooks(pvd_object_id);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_degraded_during_sniff_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_degraded_during_zero_test()
 ****************************************************************
 * @brief
 *  - Degrade the raid group.
 *  - Start the zero.
 *  - Make sure the zero does not proceed. 
 *  - Reinsert drives, make sure rebuild completes.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_degraded_during_zero_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", rg_object_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */
                               removal_mode);

        samwise_gamgee_setup_for_degraded_op(current_rg_config_p, &rg_object_ids[rg_index],
                                            &vd_object_ids[rg_index], &pvd_object_ids[rg_index]);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure rebuild is not running since debug hook should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate rg is still degraded rg: 0x%x ==", rg_object_id);
        samwise_gamgee_validate_rb_chkpt_is_zero(rg_object_id, current_rg_config_p);

        /* If raid10, we don't want to zero the r10 metadata area
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start zeroing.  This will be allowed to run since zeroing during a degraded, not rebuilding
         * raid group is allowed. 
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting zero of rg: 0x%x==", rg_object_id);
        fbe_test_sep_util_zero_object_capacity(rg_object_id, 0 /* start lba to zero */);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        /* Add hooks that allow us to know that zeroing is not getting permission to run.
         */
        fbe_test_zero_add_hooks(pvd_object_id);
        
        /* Make sure zeroing is not running since rebuild should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate zeroing did not make progress pvd: 0x%x==", pvd_object_id);
        /* Zeroing should not make progress since rebuild is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);

        /* Add rg hook to catch rebuild in progress.  We'll check this later when rebuild is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
    
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", rg_object_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);

        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure RB ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure zeroing is allowed to run even though the raid group is degraded.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for zero to complete pvd[0]: 0x%x==", pvd_object_id);
        
        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_rg_zeroing_complete(current_rg_config_p);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_degraded_during_zero_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_degraded_during_copy_test()
 ****************************************************************
 * @brief
 *  - Start the copy.
 *  - Degrade the raid group.
 *  - Make sure the copy does not proceed
 *  - Re-insert drives and make sure rebuild finishes
 *  - Make sure the copy proceeds and completes
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_degraded_during_copy_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 1 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 1);

    /* Determine our destination index.
     */
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Add debug hook to prevent copy from proceeding.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start Copy.  This will be allowed to run even though we are degraded.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy rg: 0x%x pvd: 0x%x==", rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drive for rg: 0x%x ==", rg_object_id);
        big_bird_remove_drives(current_rg_config_p, 1, /* 1 raid group */ FBE_FALSE,
                               FBE_TRUE, /* yes reinsert same drive */ removal_mode);

        samwise_gamgee_setup_for_degraded_op(current_rg_config_p, &rg_object_ids[rg_index],
                                            &vd_object_ids[rg_index], &pvd_object_ids[rg_index]);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add hooks to validate permission isn't granted.*/
        fbe_test_copy_add_hooks(vd_object_id);

        /* Delete the hook to allow the copy to proceed. */
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Copy should not make progress since verify is running and will deny the zero.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate copy does not progress rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        fbe_test_copy_validate_no_progress(vd_object_id);
        fbe_test_copy_del_hooks(vd_object_id);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 

        /* Add rg hook to catch rebuild in progress.  We'll check this later when copy is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rgs: 0x%x ==", rg_object_id);
        big_bird_insert_drives(current_rg_config_p, 1, FBE_FALSE);

        current_rg_config_p++;
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        vd_object_id = vd_object_ids[rg_index];
        
        /* Make sure RB ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                             FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Copy should not make progress since verify is running and will deny the zero.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_degraded_during_copy_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_shutdown_during_zero_test()
 ****************************************************************
 * @brief
 *  - Start the zero with a hook so it does not proceed.
 *  - Shutdown the raid group.
 *  - Make sure the zero does not proceed.
 *  - Restore the raid group, wait for it to rebuild.
 *  - Make sure the zero continues.
 *
 * We do not test R0 since it is non-redundant.
 * We do not test R1 since if it is shutdown none of the PVDs 
 * are present to even do a zero.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_shutdown_during_zero_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], FBE_U32_MAX /* last position */);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);
        /* Add debug hook so zeroing does not proceed.
         */
        status = fbe_test_add_debug_hook_active(pvd_object_id,
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start zeroing.  This will not run because of debug hook.
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
        mut_printf(MUT_LOG_TEST_STATUS, "== starting zero of rg: 0x%x==", rg_object_id);
        fbe_test_sep_util_zero_object_capacity(rg_object_id, 0 /* start lba to zero */);

        /* Wait until PVD starts the zero
        */
        status = fbe_test_wait_for_debug_hook_active(pvd_object_id, 
                                             SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                             FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                             0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t num_drives_to_remove;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drives to shutdown for rg: 0x%x ==", rg_object_id);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            num_drives_to_remove = 3;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            num_drives_to_remove = 1;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Raid 10 goes sequential to insure a shutdown.
             */
            num_drives_to_remove = 2;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            num_drives_to_remove = current_rg_config_p->width;
        }
        else
        {
            num_drives_to_remove = 2;
        }
        big_bird_remove_all_drives(current_rg_config_p, 1, num_drives_to_remove,
                                   FBE_TRUE,    /* We are pulling and reinserting same drive */
                                   0,    /* msec wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        /* Validate raid group is shutdown.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate state is FAIL for rg: 0x%x ==", rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                         SAMWISE_GAMGEE_MAX_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Remove debug hook so zeroing can proceed.
         */
        status = fbe_test_del_debug_hook_active(pvd_object_id,
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Add hooks that allow us to know that zeroing is not getting permission to run.
         */
        fbe_test_zero_add_hooks(pvd_object_id);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        
        /* Make sure zeroing is not running since rebuild should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate zeroing did not make progress pvd: 0x%x==", pvd_object_id);
        /* Zeroing should not make progress since rebuild is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        mut_printf(MUT_LOG_TEST_STATUS, "== validate zeroing did not make progress pvd: 0x%x==", pvd_object_id);

        /* Zeroing should not make progress since rebuild is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);
        fbe_test_zero_del_hooks(pvd_object_id);
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rg: 0x%x ==", rg_object_id);
        big_bird_insert_all_drives(current_rg_config_p, 1, 0, /* not used*/
                                   FBE_TRUE    /* We are pulling and reinserting same drive */);
        current_rg_config_p++;
    }
    
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds rg: 0x%x ==", rg_object_id);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1,/* num rg */ 1 /* drives rebuilding */);

        mut_printf(MUT_LOG_TEST_STATUS, "== wait for all objects ready for rg: 0x%x ==", rg_object_id);
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure zeroing does continue on pvd_obj_id: 0x%x ==", 
                   __FUNCTION__, pvd_object_id);

        /* Wait for the zeroing to complete.  When rebuild is done zeroing should run freely.
         */
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_shutdown_during_zero_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_shutdown_during_sniff_test()
 ****************************************************************
 * @brief
 *  - Shutdown the raid group.
 *  - Make sure the sniff does not proceed for one drive in the rg.
 *  - Restore the raid group, wait for it to rebuild.
 *  - Make sure the sniff continues.
 *
 * We do not test R0 since it is non-redundant.
 * We do not test R1 since if it is shutdown none of the PVDs 
 * are present to even do a sniff.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_shutdown_during_sniff_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_get_verify_status_t verify_status;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], FBE_U32_MAX /* last position */);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t num_drives_to_remove;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Remove the drives.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drives to shutdown for rg: 0x%x ==", rg_object_id);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            num_drives_to_remove = 3;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            num_drives_to_remove = 1;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Raid 10 goes sequential to insure a shutdown.
             */
            num_drives_to_remove = 2;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            num_drives_to_remove = current_rg_config_p->width;
        }
        else
        {
            num_drives_to_remove = 2;
        }
        big_bird_remove_all_drives(current_rg_config_p, 1, num_drives_to_remove,
                                   FBE_TRUE,    /* We are pulling and reinserting same drive */
                                   0,    /* msec wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        /* Validate raid group is shutdown.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate state is FAIL for rg: 0x%x ==", rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                         SAMWISE_GAMGEE_MAX_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Set checkpoint to 0 so that the sniff is guaranteed to ask soon about sniffing on our raid group.
         * Otherwise it can take a long time for sniffing to advance to this rg. 
         */
        status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_set_verify_checkpoint(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, SAMWISE_GAMGEE_DEFAULT_OFFSET);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }
        /* Add the hooks we will use below.
         */
        fbe_test_sniff_add_hooks(pvd_object_id);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        /* Make sure sniff is not running since rg should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does not proceed on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_sniff_validate_no_progress(pvd_object_id);

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != SAMWISE_GAMGEE_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rg: 0x%x ==", rg_object_id);
        big_bird_insert_all_drives(current_rg_config_p, 1, 0, /* not used*/
                                   FBE_TRUE    /* We are pulling and reinserting same drive */);
        current_rg_config_p++;
    }
    
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];

        mut_printf(MUT_LOG_TEST_STATUS, "== wait for all objects ready for rg: 0x%x ==", rg_object_id);
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds rg: 0x%x ==", rg_object_id);
        big_bird_wait_for_rebuilds(current_rg_config_p, 1,/* num rg */ 1 /* drives rebuilding */);

        /* Make sure sniff is running again.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does continue on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_validate_sniff_progress(pvd_object_id);
        fbe_test_sniff_del_hooks(pvd_object_id);
        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_shutdown_during_sniff_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_shutdown_during_copy_test()
 ****************************************************************
 * @brief
 *  - Start the copy with a hook so it does not proceed.
 *  - Shutdown the raid group.
 *  - Make sure the copy does not proceed.
 *  - Restore the raid group, wait for it to rebuild.
 *  - Make sure the copy completes.
 *
 * We do not test R0 since it is non-redundant.
 * We do not test R1 since if it is shutdown none of the PVDs 
 * are present to even do a copy.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  4/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_shutdown_during_copy_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};

    mut_printf(MUT_LOG_TEST_STATUS, "\n== %s starting ==\n", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], FBE_U32_MAX /* last position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], FBE_U32_MAX /* last position */);
    /* Determine our destination index.
     */
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Start Copy.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy rg: 0x%x pvd: 0x%x==", rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t num_drives_to_remove;
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Remove the drives.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drives to shutdown for rg: 0x%x ==", rg_object_id);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            num_drives_to_remove = 3;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            num_drives_to_remove = 1;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Raid 10 goes sequential to insure a shutdown.
             */
            num_drives_to_remove = 2;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            num_drives_to_remove = current_rg_config_p->width;
        }
        else
        {
            num_drives_to_remove = 2;
        }
        big_bird_remove_all_drives(current_rg_config_p, 1, num_drives_to_remove,
                                   FBE_TRUE,    /* We are pulling and reinserting same drive */
                                   0,    /* msec wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];
        /* Validate raid group is shutdown.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate state is FAIL for rg: 0x%x ==", rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                         SAMWISE_GAMGEE_MAX_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Add hooks that allow us to know that copying is not getting permission to run.
         */
        fbe_test_copy_add_hooks(vd_object_id);
        /* Remove debug hook.  Copy will not proceed since RG is shutdown.
         */
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        /* Copy should not make progress since verify is running and will deny the zero.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Validate copy does not progress rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        fbe_test_copy_validate_no_progress(vd_object_id);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        fbe_test_copy_validate_no_progress(vd_object_id);
        fbe_test_copy_del_hooks(vd_object_id);

        mut_printf(MUT_LOG_TEST_STATUS, "== insert drives for rg: 0x%x ==", rg_object_id);
        big_bird_insert_all_drives(current_rg_config_p, 1, 0, /* not used*/
                                   FBE_TRUE    /* We are pulling and reinserting same drive */);
        current_rg_config_p++;
    }
    
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p) || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) ||
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index];
        pvd_object_id = pvd_object_ids[rg_index];
        vd_object_id = vd_object_ids[rg_index];

        mut_printf(MUT_LOG_TEST_STATUS, "== wait for all objects ready for rg: 0x%x ==", rg_object_id);
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds rg: 0x%x ==", rg_object_id);

        big_bird_wait_for_rebuilds(current_rg_config_p, 1,/* num rg */ 1 /* drives rebuilding */);

        /* Wait for the copy to complete.  When rg is no longer shutdown copy should run freely.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end samwise_gamgee_shutdown_during_copy_test()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_run_tests()
 ****************************************************************
 * @brief
 *  Run the set of tests for this scenario.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{

    /* Test that copy does not proceed during shutdown
     */
    samwise_gamgee_shutdown_during_copy_test(rg_config_p);

    /* Test that zero does not proceed when the rg is shutdown.
     */ 
    samwise_gamgee_shutdown_during_zero_test(rg_config_p);

    /* Test that sniff does not proceed during a shutdown.
     */ 
    samwise_gamgee_shutdown_during_sniff_test(rg_config_p);

    if (fbe_test_sep_util_get_cmd_option("-shutdown_only"))
    {
        return;
    }
    /* Make sure that for a degraded raid group, copy does proceed.
     */
    samwise_gamgee_degraded_during_copy_test(rg_config_p);
    /* Make sure that for a degraded raid group, zero does not proceed.
     */
    samwise_gamgee_degraded_during_zero_test(rg_config_p);
    /* Make sure that for a degraded raid group, sniff does not proceed.
     */
    samwise_gamgee_degraded_during_sniff_test(rg_config_p);

    /* Test that copy does not proceed during rebuild
     */
    samwise_gamgee_rebuild_during_copy_test(rg_config_p);

    /* Test that zero does not proceed during a rebuild.
     */ 
    samwise_gamgee_rebuild_during_zero_test(rg_config_p);

    /* Test that sniff does not proceed during a rebuild.
     */ 
    samwise_gamgee_rebuild_during_sniff_test(rg_config_p);

    /* Test that sniff does not proceed during copy.
     */
    samwise_gamgee_sniff_during_copy_test(rg_config_p);

    /* Make sure zeroing does not proceed during copy.
     */
    samwise_gamgee_zero_during_copy_test(rg_config_p);
    return;
}
/**************************************
 * end samwise_gamgee_run_tests()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_test()
 ****************************************************************
 * @brief
 *  Run background operations priority test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &samwise_gamgee_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, samwise_gamgee_run_tests,
                                   SAMWISE_GAMGEE_TEST_LUNS_PER_RAID_GROUP,
                                   SAMWISE_GAMGEE_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end samwise_gamgee_test()
 ******************************************/
/*!**************************************************************
 * samwise_gamgee_setup()
 ****************************************************************
 * @brief
 *  Setup the samwise_gamgee test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void samwise_gamgee_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &samwise_gamgee_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
          
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* Some of the tests here can't use a 2 drive raid 10. 
         * If the above random function chose a 2 drive r10, change it to a 4 drive. 
         */
        MUT_ASSERT_INT_EQUAL(rg_config_p[SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX].raid_type, FBE_RAID_GROUP_TYPE_RAID10);
        if (rg_config_p[SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX].width == 2)
        {
            rg_config_p[SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX].width = 4;
        }
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, SAMWISE_GAMGEE_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, SAMWISE_GAMGEE_COPY_COUNT);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }
    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end samwise_gamgee_setup()
 **************************************/
/*!**************************************************************
 * samwise_gamgee_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the samwise_gamgee test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void samwise_gamgee_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end samwise_gamgee_cleanup()
 ******************************************/

/*!****************************************************************************
 * samwise_gamgee_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  12/03/2012  Ron Proulx  - Created.
 ******************************************************************************/
void samwise_gamgee_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &samwise_gamgee_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, samwise_gamgee_run_tests,
                                   SAMWISE_GAMGEE_TEST_LUNS_PER_RAID_GROUP,
                                   SAMWISE_GAMGEE_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end samwise_gamgee_dualsp_test()
 ******************************************************************************/

/*!**************************************************************
 * samwise_gamgee_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a Permanent Sparing  test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  12/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
void samwise_gamgee_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &samwise_gamgee_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
          
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* Some of the tests here can't use a 2 drive raid 10. 
         * If the above random function chose a 2 drive r10, change it to a 4 drive. 
         */
        MUT_ASSERT_INT_EQUAL(rg_config_p[SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX].raid_type, FBE_RAID_GROUP_TYPE_RAID10);
        if (rg_config_p[SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX].width == 2)
        {
            rg_config_p[SAMWISE_GAMGEE_RAID_10_CONFIG_INDEX].width = 4;
        }
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, SAMWISE_GAMGEE_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, SAMWISE_GAMGEE_COPY_COUNT);

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

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************
 * end samwise_gamgee_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * samwise_gamgee_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the freddie test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  12/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
void samwise_gamgee_dualsp_cleanup(void)
{

    fbe_test_sep_util_print_dps_statistics();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;
}
/******************************************
 * end samwise_gamgee_dualsp_cleanup()
 ******************************************/

/*********************************
 * end file samwise_gamgee_test.c
 *********************************/


