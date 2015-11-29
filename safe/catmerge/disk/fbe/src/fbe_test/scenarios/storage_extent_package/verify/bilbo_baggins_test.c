/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bilbo_baggins_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of priority between verify operations and
 *  other operations (sniff, zero, paco).
 *
 * @version
 *  3/28/2012 - Created. Rob Foley
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
#include "sep_utils.h"
#include "sep_hook.h"
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

char * bilbo_baggins_short_desc = "Verify operations priority test..";
char * bilbo_baggins_long_desc = "\
The bilbo_baggins scenario tests the priority between verify operations and\n\
other background operations (sniff, Copy and zeroing).\n\
                                 \n\
   1) Run these verifies: error, lun dirty, user read only and user rw verify. \n\
      - ensure that sniff does not proceed.\n\
   2) Run these verifies: user read only and user rw verify. \n\
      - ensure that these verifies do not proceed when zeroing is needed.\n\
      - once zeroing is done, make sure these verifies proceed.\n\
   3) Run these verifies: error, lun dirty. \n\
      - ensure that these verifies do proceed when zeroing is in progress.\n\
      - Make sure zeroing does not proceed when verify is in progress.\n\
      - Once verify is done, make sure zeroing proceeds.\n"
"   4) Run these verifies: error, lun dirty. \n\
      - Kick off a copy and make sure copy does not progress when these verifies are in progress.\n\
      - Once verify is done, make sure copy proceeds.\n\
   5) Run these verifies: user read only and user rw verify. \n\
      - Kick off a copy and make sure copy does  progress when these verifies are in progress.\n\
      - Once copy is done, make sure verify proceeds.\n\
   6) Run these verifies: error, lun dirty, user read only and user rw verify. \n\
      - ensure that the verifies proceed in this order.\n\
      - Make sure error verify runs first, followed by lun dirty verify, user ro and then user rw.\n\
                                 \n\
Description last updated: 4/10/2012.\n";


/*!*******************************************************************
 * @def BILBO_BAGGINS_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define BILBO_BAGGINS_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def BILBO_BAGGINS_MAX_WAIT_MSEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define BILBO_BAGGINS_MAX_WAIT_MSEC FBE_TEST_WAIT_TIMEOUT_MS

/*!*******************************************************************
 * @def BILBO_BAGGINS_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BILBO_BAGGINS_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def BILBO_BAGGINS_DEFAULT_OFFSET
 *********************************************************************
 * @brief The default offset on the disk where we create rg.
 *
 *********************************************************************/
#define BILBO_BAGGINS_DEFAULT_OFFSET 0x10000

/*!*******************************************************************
 * @def BILBO_BAGGINS_COPY_COUNT
 *********************************************************************
 * @brief This is the number of proactive copies we will do.
 *        This is also the number of extra drives we need for each rg.
 *
 *********************************************************************/
#define BILBO_BAGGINS_COPY_COUNT 7

/*!*******************************************************************
 * @var bilbo_baggins_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 * 
 * @todo random parity is not enabled since copy has issues with R6.
 *
 *********************************************************************/
fbe_test_rg_configuration_t bilbo_baggins_raid_group_config[] = 
{
    /* width,                        capacity    raid type,                  class,             block size   RAID-id.  bandwidth. */
    
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,   520,            1,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,   520,            2,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY,   FBE_CLASS_ID_PARITY,   520,            5,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,   520,            3, 0}, 
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID0,   FBE_CLASS_ID_STRIPER,  520,            4, 0}, 

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);

void bilbo_baggins_multiple_verify_test(fbe_test_rg_configuration_t *rg_config_p);

/*!**************************************************************
 * bilbo_baggins_sniff_during_bg_verify_test()
 ****************************************************************
 * @brief
 *  - Bind a LUN with no initial verify set.
 *  - Wait for zeroing to complete.
 *  - Make sure that no verifies have occurred.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void bilbo_baggins_sniff_during_bg_verify_test(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t monitor_substate;
    fbe_provision_drive_get_verify_status_t verify_status;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);

    fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting for verify type: 0x%x==", __FUNCTION__, verify_type);

    /* Setup the test, we need to make sure the objects have come ready first. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Add debug hook that pauses verify.  We don't want verify to finish so we can hold up sniffing.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* We need to wait for this since if it is not done, sniffing will not occur. 
         * If we start bg verify before we wait for zeroing, then zeroing will never finish and sniffing will not start.
         */
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);
        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, verify_type);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Set checkpoint to 0 so that the sniff is guaranteed to ask soon about sniffing on our raid group.
         * Otherwise it can take a long time for sniffing to advance to this rg. 
         */
        status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_set_verify_checkpoint(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, BILBO_BAGGINS_DEFAULT_OFFSET);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != BILBO_BAGGINS_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }
    
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s started verify type: 0x%x on all rgs ==", __FUNCTION__, verify_type);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
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
        if (verify_status.verify_checkpoint != BILBO_BAGGINS_DEFAULT_OFFSET)
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
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        /* Make sure sniff is not running since BV should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Make sure sniff does not proceed on pvd_obj_id: 0x%x ==", __FUNCTION__, pvd_object_id);
        fbe_test_sniff_validate_no_progress(pvd_object_id);

        /* Make sure the sniff checkpoint has not moved.
         */
        status = fbe_api_provision_drive_get_verify_status(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (verify_status.verify_checkpoint != BILBO_BAGGINS_DEFAULT_OFFSET)
        {
            MUT_FAIL_MSG("checkpoint was not set");
        }

        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Wait for the raid group verify to finish.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for verify on rg_obj_id: 0x%x ==", __FUNCTION__, rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, verify_type);

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
 * end bilbo_baggins_sniff_during_bg_verify_test()
 **************************************/

/*!**************************************************************
 * bilbo_baggins_low_priority_verify_during_zero_test()
 ****************************************************************
 * @brief
 *  - Bind a LUN with no initial verify set.
 *  - Wait for zeroing to complete.
 *  - Make sure that no verifies have occurred.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 * @param verify_type - Type of verify to start.
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void bilbo_baggins_low_priority_verify_during_zero_test(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t monitor_substate;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting for verify type: 0x%x==", __FUNCTION__, verify_type);

    current_rg_config_p = rg_config_p;
    /* First start the Zero operation */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure our objects are in the ready state. */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        status = fbe_test_add_debug_hook_active(pvd_object_id,
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start the zero. */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting zero on rg: 0x%x==",  rg_object_id);
        fbe_test_sep_util_zero_object_capacity(rg_object_id, 0 /* start lba to zero */);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    /* Wait for Zero hook to hit before starting the low priority verify. */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Wait for the Zero hook to hit. This is to make sure the Zero operation is in progress. */
        status = fbe_test_wait_for_debug_hook_active(pvd_object_id, 
                                             SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                             FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                             SCHEDULER_CHECK_STATES, 
                                             SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now, Kick off the verify. */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting verify type: 0x%x rg: 0x%x==",  verify_type, rg_object_id);
        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, verify_type);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        /* Make sure verify is not running since BV should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate no verify progress type: 0x%x rg: 0x%x==",  verify_type, rg_object_id);
        fbe_test_verify_validate_no_progress(rg_object_id, verify_type);
 
        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Delete the pvd hook to allow zero to complete.
         */
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
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Wait for the zeroing to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== wait for zero to complete on pvd: 0x%x==",  pvd_object_id);
        fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);
        
        /* Make sure BV ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             monitor_substate,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== wait for verify to finish type: 0x%x rg: 0x%x==",  verify_type, rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, verify_type);

        current_rg_config_p++;
    }
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end bilbo_baggins_low_priority_verify_during_zero_test()
 **************************************/

/*!**************************************************************
 * bilbo_baggins_high_priority_verify_during_zero_test()
 ****************************************************************
 * @brief
 *  - Bind a LUN with no initial verify set.
 *  - Wait for zeroing to complete.
 *  - Make sure that no verifies have occurred.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 * @param verify_type - Type of verify to start.
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void bilbo_baggins_high_priority_verify_during_zero_test(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t monitor_substate;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t chkpt;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting for verify type: 0x%x==", __FUNCTION__, verify_type);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Add debug hook that pauses verify.  We don't want verify to finish so we can hold up sniffing.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_add_debug_hook_active(pvd_object_id,
                                                  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Kick off the background verify.  This will not run because of debug hook.
         */
        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, verify_type);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Start zeroing.  This will not run because of debug hook.
         */
        fbe_test_sep_util_zero_object_capacity(rg_object_id, 0 /* start lba to zero */);

        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Add hooks that allow us to know that zeroing is not getting permission to run.
         */
        fbe_test_zero_add_hooks(pvd_object_id);

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
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];
        /* Zeroing should not make progress since verify is running and will deny the zero.
         */
        fbe_test_zero_validate_no_progress(pvd_object_id);
        fbe_test_zero_del_hooks(pvd_object_id);

        /* Make sure verify is not running since debug hook should block it.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_verify_checkpoint(&rg_info, verify_type, &chkpt);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (chkpt != 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "chkpt is 0x%llx not 0\n", (unsigned long long)chkpt);
            MUT_FAIL_MSG("verify checkpoint should be 0");
        }

        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        rg_object_id = rg_object_ids[rg_index]; 
        pvd_object_id = pvd_object_ids[rg_index];

        /* Wait for verify to complete.  It should be free to run since it is higher priority than zeroing.
         */
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, verify_type);
        
        /* Make sure BV ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             monitor_substate,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for the zeroing to complete.  When verify is done zeroing should run freely.
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
 * end bilbo_baggins_high_priority_verify_during_zero_test()
 **************************************/
/*!**************************************************************
 * bilbo_baggins_high_priority_verify_during_copy_test()
 ****************************************************************
 * @brief
 *  - Bind a LUN with no initial verify set.
 *  - Wait for zeroing to complete.
 *  - Make sure that no verifies have occurred.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 * @param verify_type - Type of verify to start.
 * @return None.
 *
 * @author
 *  4/4/2012 - Created. Rob Foley
 *
 ****************************************************************/

void bilbo_baggins_high_priority_verify_during_copy_test(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t monitor_substate;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t chkpt;
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting for verify type: 0x%x==", __FUNCTION__, verify_type);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);
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

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Add debug hook that pauses verify.  We don't want verify to finish so we can hold up sniffing.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add hook that pauses the copy.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Kick off the background verify.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting verify type: 0x%x rg: 0x%x==", verify_type, rg_object_id);
        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, verify_type);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

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

        /* Make sure verify is not running since debug hook should block it.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_verify_checkpoint(&rg_info, verify_type, &chkpt);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (chkpt != 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "chkpt is 0x%llx not 0\n", (unsigned long long)chkpt);
            MUT_FAIL_MSG("verify checkpoint should be 0");
        }

        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
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
        vd_object_id = vd_object_ids[rg_index];

        /* Wait for verify to complete.  It should be free to run since it is higher priority than zeroing.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for verify type %d rg: 0x%x==", verify_type, rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, verify_type);
        
        /* Make sure BV ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             monitor_substate,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for the copy to complete.  When verify is done copy should run freely.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end bilbo_baggins_high_priority_verify_during_copy_test()
 **************************************/
/*!**************************************************************
 * bilbo_baggins_low_priority_verify_during_copy_test()
 ****************************************************************
 * @brief
 *  Perform a verify and copy at the same time.  Make sure copy
 *  proceeds before the verify.
 * @param rg_config_p - Config array to use.  This is null terminated.
 * @param verify_type - Type of verify to start.
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void bilbo_baggins_low_priority_verify_during_copy_test(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t monitor_substate;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t chkpt;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting for verify type: 0x%x==", __FUNCTION__, verify_type);

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);
    fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);

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

        /* Add debug hook to prevent copy from proceeding.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add hook to prevent raid group from proceeding with verify.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Start the copy.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting copy rg: 0x%x pvd: 0x%x==", rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        /* Kick off the background verify.  The background verify should get held up by the copy.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting verify type: 0x%x rg: 0x%x==", verify_type, rg_object_id);
        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, verify_type);
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

        /* We let verify proceed once it has seen it cannot run due to copy.
         * If we let verify go too soon, it will proceed because copy has not started, 
         * and thus has not bubbled up the notification that verify should not run.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Allow verify to proceed.  Copy should block it.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /* Make sure verify is not running since copy should block it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== validate verify does not make progress verify type: 0x%x rg: 0x%x==",  
                   verify_type, rg_object_id);
        fbe_test_verify_validate_no_progress(rg_object_id, verify_type);
 
        /* Add rg hook to catch verify in progress.  We'll check this later when zeroing is allowed to proceed.
         */
        fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  monitor_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_verify_checkpoint(&rg_info, verify_type, &chkpt);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (chkpt != 0)
        {
            MUT_FAIL_MSG("checkpoint is not zero");
        }
        /* Delete the pvd hook to allow copy to run.
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
        
        /* Make sure BV ran.
         */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             monitor_substate,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for verify type %d rg: 0x%x==", verify_type, rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, verify_type);

        current_rg_config_p++;
    }

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end bilbo_baggins_low_priority_verify_during_copy_test()
 **************************************/


void bilbo_validate_rg_no_verify_progress(fbe_object_id_t rg_object_id, fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t total_ms = 0;

    while (FBE_TRUE)
    {
        /* Validate all verifies have not made progress.  Make sure incomplete write verify has not finished.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                   rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint,
                   (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint,
                   (unsigned long long)rg_info.error_verify_checkpoint);

        if (total_ms > timeout_ms) {
            MUT_FAIL_MSG("checkpoint is not 0 in alloted time");
        }
        if ((rg_info.ro_verify_checkpoint != 0) ||
            (rg_info.rw_verify_checkpoint != 0) ||
            (rg_info.error_verify_checkpoint != 0) ||
            (rg_info.system_verify_checkpoint != 0) ||            
            (rg_info.incomplete_write_verify_checkpoint != 0)){
            fbe_api_sleep(200);
            total_ms += 200;
        }
        else {
            break;
        }
    }
}
/*!**************************************************************
 * bilbo_baggins_multiple_verify_test()
 ****************************************************************
 * @brief
 *  - Start all verify types.
 *  - Using hooks allow each one to proceed in priority order.
 *
 * @param rg_config_p - Config array to use.  This is null terminated.
 * @return None.
 *
 * @author
 *  4/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void bilbo_baggins_multiple_verify_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

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
        
        /* Validate there are no verifies in progress.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.system_verify_checkpoint != FBE_LBA_INVALID) ||            
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint,
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint,
                       (unsigned long long)rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("checkpoint is not FBE_LBA_INVALID");
        }

        /* Add debug hook that pauses verify.  We don't want verify to finish so we can hold up all verifies.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Kick off the background verify.  This will not run because of debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== starting all verify types on rg: 0x%x==", rg_object_id);
        /* Start user verify first since it causes metadata verify.
         */
        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RW);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, FBE_LUN_VERIFY_TYPE_ERROR);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, FBE_LUN_VERIFY_TYPE_SYSTEM);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_verify_start_on_all_rg_luns(current_rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RO);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        bilbo_validate_rg_no_verify_progress(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS);

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

        /* Validate all verifies have not made progress.  Make sure incomplete write verify has not finished.
         */
        bilbo_validate_rg_no_verify_progress(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS);

        /* Prevent all verifies from running.
         */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Allow metadata verify to complete.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY,
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

        /* Make sure incomplet write verify wanted to run.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for incomplete write verify to start rg: 0x%x==", rg_object_id);
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate all verifies have not made progress.  Make sure incomplete write verify has not finished.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != 0) ||
            (rg_info.rw_verify_checkpoint != 0) ||
            (rg_info.system_verify_checkpoint != 0) ||            
            (rg_info.error_verify_checkpoint != 0) ||
            (rg_info.incomplete_write_verify_checkpoint == FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint, 
                       (unsigned long long)rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("checkpoint is not 0");
        }

        /* Allow incomplete write verify to run.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
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

        /* Wait for incomplete write verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for incomplete write verify to complete rg: 0x%x==", rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);
        
        
        /* Make sure error verify wanted to run.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for error verify to start rg: 0x%x==", rg_object_id);
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate all verifies have not made progress.  Make sure error verify has not finished.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != 0) ||
            (rg_info.rw_verify_checkpoint != 0) ||
            (rg_info.system_verify_checkpoint != 0) ||
            (rg_info.error_verify_checkpoint == FBE_LBA_INVALID) ||
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint, 
                       (unsigned long long)rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("checkpoint is not 0");
        }

        /* Allow Error verify to run.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
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

        /* Wait for error verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for error verify to complete rg: 0x%x==", rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, FBE_LUN_VERIFY_TYPE_ERROR);

        /* Make sure system verify wanted to run.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for system verify to start rg: 0x%x==", rg_object_id);
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that err verify is done. Validate that user rw and user ro have not started.
         * Make sure system verify has not finished.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != 0) ||
            (rg_info.rw_verify_checkpoint != 0) ||
            (rg_info.system_verify_checkpoint == FBE_LBA_INVALID) ||            
            (rg_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint,
                       (unsigned long long)rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("checkpoints are not expected");
        }

        /* Allow system verify to run.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,
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

        /* Wait for system verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for ld verify to complete rg: 0x%x==", rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, FBE_LUN_VERIFY_TYPE_SYSTEM);

        /* Make sure uwer rw verify wanted to run.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for user rw verify to start rg: 0x%x==", rg_object_id);
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that err verify, ld verify are done.
         * Validate that user ro have not started. 
         * Make sure user rw verify has not finished.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != 0) ||
            (rg_info.rw_verify_checkpoint == FBE_LBA_INVALID) ||
            (rg_info.system_verify_checkpoint != FBE_LBA_INVALID) ||            
            (rg_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint, 
                       (unsigned long long)rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("checkpoints are not expected");
        }

        /* Allow user rw verify to run.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START,
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

        /* Wait for lun dirty verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for user rw verify to complete rg: 0x%x==", rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, FBE_LUN_VERIFY_TYPE_USER_RW);

        /* Make sure uwer ro verify wanted to run.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for user ro verify to start rg: 0x%x==", rg_object_id);
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                             FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that err verify, ld, and user rw verify are done.
         * Validate that user ro have not completed. 
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint == FBE_LBA_INVALID) ||
            (rg_info.system_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx sys: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint, 
                       (unsigned long long)rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("checkpoints are not expected");
        }

        /* Allow user ro verify to run.
         */
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START,
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

        /* Wait for user ro verify to complete.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for user ro verify to complete rg: 0x%x==", rg_object_id);
        fbe_test_verify_wait_for_verify_completion(rg_object_id, BILBO_BAGGINS_MAX_WAIT_MSEC, FBE_LUN_VERIFY_TYPE_USER_RO);

        /* Validate that all verifies are done.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.system_verify_checkpoint != FBE_LBA_INVALID) ||            
            (rg_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "rg: 0x%x rw: 0x%llx ro: 0x%llx iw: 0x%llx ld: 0x%llx err: 0x%llx",
                       rg_object_id, (unsigned long long)rg_info.rw_verify_checkpoint, (unsigned long long)rg_info.ro_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint, 
                       rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("all verifies are not complete");
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end bilbo_baggins_multiple_verify_test()
 **************************************/
/*!**************************************************************
 * bilbo_baggins_run_tests()
 ****************************************************************
 * @brief
 *  We create a config and run the rebuild tests.
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

void bilbo_baggins_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    /* Test that all verifies are higher priority than sniff.
     */
    bilbo_baggins_sniff_during_bg_verify_test(rg_config_p, FBE_LUN_VERIFY_TYPE_ERROR);
    bilbo_baggins_sniff_during_bg_verify_test(rg_config_p, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);
    bilbo_baggins_sniff_during_bg_verify_test(rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RO);
    bilbo_baggins_sniff_during_bg_verify_test(rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RW);

    /* Test that user ro and user rw verifies are lower priority than zeroing.
     */
    bilbo_baggins_low_priority_verify_during_zero_test(rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RO);
    bilbo_baggins_low_priority_verify_during_zero_test(rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RW);

    /* Test that error and lun auto verify are higher priority than zeroing.
     */
    bilbo_baggins_high_priority_verify_during_zero_test(rg_config_p, FBE_LUN_VERIFY_TYPE_ERROR);
    bilbo_baggins_high_priority_verify_during_zero_test(rg_config_p, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);

    /* Test that error and lun auto verify are higher priority than copy.
     */
    bilbo_baggins_low_priority_verify_during_copy_test(rg_config_p, FBE_LUN_VERIFY_TYPE_ERROR);
    bilbo_baggins_low_priority_verify_during_copy_test(rg_config_p, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);

    /* Test that validates that read only and read write verifies do not progress during a copy.
     */
    bilbo_baggins_low_priority_verify_during_copy_test(rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RO);
    bilbo_baggins_low_priority_verify_during_copy_test(rg_config_p, FBE_LUN_VERIFY_TYPE_USER_RW);
    /* Test where we start multiple verifies and make sure they run in the expected order.
     */
    bilbo_baggins_multiple_verify_test(rg_config_p);
    return;
}
/**************************************
 * end bilbo_baggins_run_tests()
 **************************************/
/*!**************************************************************
 * bilbo_baggins_test()
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

void bilbo_baggins_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &bilbo_baggins_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, bilbo_baggins_run_tests,
                                   BILBO_BAGGINS_TEST_LUNS_PER_RAID_GROUP,
                                   BILBO_BAGGINS_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end bilbo_baggins_test()
 ******************************************/
/*!**************************************************************
 * bilbo_baggins_setup()
 ****************************************************************
 * @brief
 *  Setup the bilbo baggins test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void bilbo_baggins_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &bilbo_baggins_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, BILBO_BAGGINS_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, BILBO_BAGGINS_COPY_COUNT);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }
    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end bilbo_baggins_setup()
 **************************************/
/*!**************************************************************
 * bilbo_baggins_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the bilbo_baggins test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void bilbo_baggins_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end bilbo_baggins_cleanup()
 ******************************************/

/*!****************************************************************************
 * bilbo_baggins_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author 
 *  03/28/2012 - Created. Rob Foley
 ******************************************************************************/
void bilbo_baggins_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &bilbo_baggins_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, bilbo_baggins_run_tests,
                                   BILBO_BAGGINS_TEST_LUNS_PER_RAID_GROUP,
                                   BILBO_BAGGINS_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end bilbo_baggins_dualsp_test()
 ******************************************************************************/

/*!**************************************************************
 * bilbo_baggins_dualsp_setup()
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
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void bilbo_baggins_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &bilbo_baggins_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, BILBO_BAGGINS_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, BILBO_BAGGINS_COPY_COUNT);

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
 * end bilbo_baggins_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * bilbo_baggins_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the freddie test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void bilbo_baggins_dualsp_cleanup(void)
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
 * end bilbo_baggins_dualsp_cleanup()
 ******************************************/


/*********************************
 * end file bilbo_baggins_test.c
 ********************************/


