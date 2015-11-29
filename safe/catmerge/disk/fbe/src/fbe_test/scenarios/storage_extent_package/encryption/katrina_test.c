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
 *  This file contains a test of priority between encryption and background
 *  operations.  The current background operations tested are:
 *      o copy
 * 
 *  This test execercises both degraded and non-degraded raid groups.
 *
 * @version
 *  01/23/2014  Ron Proulx  - Created.
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

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * katrina_short_desc = "Interaction of encyrption with other operations.";
char * katrina_long_desc = "\
The katrina scenario tests the priority between a raid group that is degraded/shutdown/rebuilding \n\
and other background operations (sniff, copy and zeroing).\n\
There are also other tests that test copy priority versus sniff and zeroing.\n\
\n\
   1) Test that a encryption does not make progress if copy starts first.\n\
   2) Test that copy does not make progress if encryption is already starting.\n\
   3) Test that encryption does not start if copy is already running.\n\
                                 \n\
Description last updated: 1/23/2014.\n";


/*!*******************************************************************
 * @def KATRINA_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define KATRINA_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def KATRINA_MAX_WAIT_SEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define KATRINA_MAX_WAIT_SEC 120

/*!*******************************************************************
 * @def KATRINA_MAX_WAIT_MSEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define KATRINA_MAX_WAIT_MSEC KATRINA_MAX_WAIT_SEC * 1000

/*!*******************************************************************
 * @def KATRINA_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define KATRINA_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def KATRINA_DEFAULT_OFFSET
 *********************************************************************
 * @brief The default offset on the disk where we create rg.
 *
 *********************************************************************/
#define KATRINA_DEFAULT_OFFSET 0x10000

/*!*******************************************************************
 * @def KATRINA_COPY_COUNT
 *********************************************************************
 * @brief This is the number of proactive copies we will do.
 *        This is also the number of extra drives we need for each rg.
 *
 *********************************************************************/
#define KATRINA_COPY_COUNT 7

/*!*******************************************************************
 * @def KATRINA_RAID_10_CONFIG_INDEX
 *********************************************************************
 * @brief The index of the raid 10 index in the below table.
 *
 *********************************************************************/
#define KATRINA_RAID_10_CONFIG_INDEX 0

/*!*******************************************************************
 * @def KATRINA_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define KATRINA_MAX_IO_SIZE_BLOCKS (128)

/*!*******************************************************************
 * @var KATRINA_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 * 
 * @todo random parity is not enabled since copy has issues with R6.
 *
 *********************************************************************/
fbe_test_rg_configuration_t katrina_raid_group_config[] = 
{
    /* width,                        capacity    raid type,                  class,             block size   RAID-id.  bandwidth. */

    /* Raid  10 must be at a particular index, don't change without modifying the above  KATRINA_RAID_10_CONFIG_INDEX */
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,   520,            1,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,   520,            2,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY,   FBE_CLASS_ID_PARITY,   520,            1,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,   520,            3, 0}, 

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

void katrina_get_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_object_id_t *rg_object_ids_p);
extern void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                               fbe_edge_index_t spare_edge_index,
                                                               fbe_test_raid_group_disk_set_t * spare_location_p);

/*!**************************************************************
 * katrina_get_rg_object_ids()
 ****************************************************************
 * @brief
 *  Fetch the array of rg object ids for this test.
 *
 * @param rg_config_p - Configuration under test.
 * @param rg_object_ids_p - Ptr to array of object ids to return.
 *
 * @return None   
 *
 * @author
 *  4/9/2012 - Created. Rob Foley
 *
 ****************************************************************/
void katrina_get_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_object_id_t *rg_object_ids_p)
{   
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_object_id_t striper_object_id;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &striper_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            
            fbe_test_sep_util_get_ds_object_list(striper_object_id, &downstream_object_list);
            MUT_ASSERT_INT_NOT_EQUAL(downstream_object_list.number_of_downstream_objects, 0);

            /*! @note For encryption we use the RAID-10 object id
             */
            rg_object_ids_p[rg_index] = striper_object_id;
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_ids_p[rg_index]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            //rg_object_ids_p[rg_index] = downstream_object_list.downstream_object_list[0];
        }
        else
        {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_ids_p[rg_index]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_ids_p[rg_index], FBE_OBJECT_ID_INVALID);
        current_rg_config_p++;
    }
}
/**************************************
 * end katrina_get_rg_object_ids
 **************************************/

/*!***************************************************************************
 *          katrina_test_copy_during_encryption_test1()
 *****************************************************************************
 *
 * @brief   Start and encryption (encryption will be in various
 *          states) and attempt a proactive copy.  The copy
 *          should not start.
 *
 * @param   rg_config_p - Config array to use.  This is null terminated.
 * @param   pattern - The I/O pattern (should not be zeros)
 * @param   peer_options - (only valid in dualsp)
 * 
 * @return  none
 *
 * @note    We don't need to test every encryption state.  All we care about
 *          is encryption mode transitions:
 *          1. FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED ==>
 *          2. FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS ==>
 *              FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED
 *          3. FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS ==>
 *              FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED
 *
 * @author
 *  01/23/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void katrina_test_copy_during_encryption_test1(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_data_pattern_t pattern,
                                         fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t num_luns;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_u32_t index;

    #define IO_DURING_ENC_INCREMENT_CHECKPOINT 0x1800
    #define IO_DURING_ENC_MAX_TEST_POINTS 2
    #define IO_DURING_ENC_WAIT_IO_COUNT 5

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s starting ===", __FUNCTION__);

    katrina_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    /* Test 1:  Validate that if the proactive copy job runs prior to change mode the copy succeeds.
     *          Step  1: Write a zero pattern to all luns
     *          Step  2: Set a log hook to validate the raid group request a start encryption
     *                   Set a pause hook to stop the raid group before it gets the downstream encryption status
     *                   Set a log hook to validate that encryption does not start
     *                   Set a pause hook to hold the copy the moment the job starts
     *                   Set a pause hook when the user copy job completes
     *                   Set a pause hook when the copy complete job runs
     *          Step  3: Enable system level encryption
     *          Step  4: Start I/O (w/r/c)
     *          Step  5: Start encryption re-key
     *          Step  6: Validate encryption mode change log hook
     *          Step  7: Wait for get downstream encryption status hook
     *          Step  8: Start a user copy
     *          Step  9: Wait for copy start hook
     *          Step 10: Remove get downstream encryption status hook
     *          Step 11: Remove the copy start pause hook
     *          Step 11: Wait for copy complete hook
     *          Step 12: Validate encrypt start log hook is not encountered
     *          Step 13: Remove copy complete pause hook
     *          Step 14: Validate copy completes
     *          Step 15: Validate raid group encryption starts and completes
     *          Step 16: Stop I/O and validate no errors
     *          
     */
    /* Test 1:  Step  1: Write a zero pattern to all luns.
     */
    big_bird_write_zero_background_pattern();

    /* Test 1:  Step  2: Set a log hook to validate the raid group request encryption start
     *                   Set a pause hook to stop the raid group before it gets the downstream encryption status
     *                   Set a log hook to validate that encrypt re-key does not start
     *                   Set a pause hook to hold the copy the moment the job starts
     *                   Set a pause hook when the user copy job completes
     *                   Set a pause hook when the copy complete job runs
     */
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

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Set a pause hook to stop the raid group request encryption start */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_test_use_rg_hooks(current_rg_config_p, 2, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                              FBE_TEST_HOOK_ACTION_ADD);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#if 0
        /* Set a pause hook to hold the copy the moment the job starts */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set a pause hook when the copy complete job runs */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif

        current_rg_config_p++;
    }       

    /* Test 1: Step  3: Enable system level encryption
     */
    // Add hook for encryption to complete and enable KMS
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS started ==");
    status = fbe_test_sep_encryption_add_hook_and_enable_kms(rg_config_p, fbe_test_sep_util_get_dualsp_test_mode());
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS completed; status: 0x%x. ==", status);

    /* Test 1:  Step  4: Start I/O (w/r/c)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, KATRINA_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Test 1:  Step  4: Start encryption
     *          Step  5: Validate encryption mode change log hook
     *          Step  6: Wait for get downstream encryption status hook
     *          Step  7: Start a user copy
     *          Step  8: Wait for copy start hook
     *          Step  9: Remove get downstream encryption status hook
     */
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

        /* Step  4: Start encryption */
        //mut_printf(MUT_LOG_TEST_STATUS, "== %s Request encryption rekey rg: 0x%x ==", __FUNCTION__, rg_object_id);
        //fbe_test_sep_util_start_encryption_on_rg(current_rg_config_p); 

        /* Step  5: Validate encryption mode change pause hook */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                                     FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step  6: Setup virtual drive keys */
        //status = fbe_test_sep_util_setup_vd_keys(vd_object_id);
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step  7: Start a user copy */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Start user copy rg: 0x%x dest pvd: 0x%x==", __FUNCTION__, rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_UPSTREAM_RAID_GROUP_DENIED, copy_status);

        /* Step  8: Remove get downstream encryption status hook*/
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

         fbe_test_use_rg_hooks(current_rg_config_p, 2, 
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                              FBE_TEST_HOOK_ACTION_WAIT);

        /* Step  7: Start a user copy */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Start user copy rg: 0x%x dest pvd: 0x%x==", __FUNCTION__, rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_UPSTREAM_RAID_GROUP_DENIED, copy_status);
        

        fbe_test_use_rg_hooks(current_rg_config_p, 2, 
                             SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                               FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,
                                               0, 0, 
                                               SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                             FBE_TEST_HOOK_ACTION_DELETE);

         if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
         {
             fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
             for (index = 0; 
                   index < downstream_object_list.number_of_downstream_objects; 
                   index++) {
                    status = fbe_test_util_wait_for_encryption_state(downstream_object_list.downstream_object_list[index], 
                                                FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED, 
                                                30000);
             }
         }
         else
         {
                 status = fbe_test_util_wait_for_encryption_state(rg_object_id, 
                                                FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED, 
                                                30000);
         }
        MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Encryption state unexpected\n");

        /* Start user copy again */
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

#if 0
        /* Step  7: Wait for copy start hook*/
        status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                                     FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step  9: Remove the edge connected hook*/
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif

        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        current_rg_config_p++;
    }  

    /* Test 1:  Step 11: Wait for copy complete hook
     *          Step 12: Validate encrypt start log hook is not encountered
     *          Step 13: Remove copy complete pause hook
     *          Step 14: Validate copy completes
     */
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

#if 0
        /* Step 11: Wait for copy complete hook*/
        status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                     FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step 12: Validate encrypt start log hook is not encountered */
        fbe_test_encryption_validate_no_progress(rg_object_id);

        /* Step 13: Remove copy complete pause hook */
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif

        /* Step 14: Validate copy completes */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    } 

    /* Test 1:  Step 16: Validate raid group encryption starts and completes
     */
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
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

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/*************************************************
 * end katrina_test_copy_during_encryption_test1()
 *************************************************/

#if 0
/*!***************************************************************************
 *          katrina_test_copy_during_encryption_test2()
 *****************************************************************************
 *
 * @brief   Start and encryption (encryption will be in various
 *          states) and attempt a proactive copy.  The copy
 *          should not start. A copy operation is attempted at the
 *          following phases of encryption:
 *          
 *          2. The encrypt job has set the state to `encryption started'
 *             The copy job starts and is `denied'.
 *
 * @param   rg_config_p - Config array to use.  This is null terminated.
 * @param   pattern - The I/O pattern (should not be zeros)
 * @param   peer_options - (only valid in dualsp)
 * 
 * @return  none
 *
 * @note    We don't need to test every encryption state.  All we care about
 *          is encryption mode transitions:
 *          1. FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED ==>
 *          2. FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS ==>
 *              FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED
 *          3. FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS ==>
 *              FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED
 *
 * @author
 *  01/27/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void katrina_test_copy_during_encryption_test2(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_data_pattern_t pattern,
                                         fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t num_luns;
    #define IO_DURING_ENC_INCREMENT_CHECKPOINT 0x1800
    #define IO_DURING_ENC_MAX_TEST_POINTS 2
    #define IO_DURING_ENC_WAIT_IO_COUNT 5

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s starting ===", __FUNCTION__);

    katrina_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    /* Test 1:  Validate that if the proactive copy job runs prior to change mode the copy succeeds.
     *          Step  1: Write a zero pattern to all luns
     *          Step  2: Set a log hook to validate the raid group request a start encryption
     *                   Set a pause hook to stop the raid group before it gets the downstream encryption status
     *                   Set a log hook to validate that encryption does not start
     *                   Set a pause hook to hold the recovery queue
     *                   Set a pause hook when the user copy job completes
     *                   Set a pause hook when the copy complete job runs
     *          Step  3: Start I/O (w/r/c)
     *          Step  4: Start encryption
     *          Step  5: Validate encryption mode change log hook
     *          Step  6: Wait for get downstream encryption status hook
     *          Step  7: Disable the recovery queue 
     *          Step  8: Start a user copy
     *          Step  9: Remove get downstream encryption status hook
     *          Step 10: Validate raid group encryption starts
     *          Step 11: Remove encryption start hooks
     *          Step 12: Validate raid group encryption starts and completes
     *          Step 13: Enable the recovery queue
     *          Step 14: Wait for copy start hook
     *          Step 15: Wait for copy complete hook
     *          Step 16: Remove copy complete pause hook
     *          Step 17: Validate copy completes
     *          Step 18: Stop I/O and validate no errors
     *          
     */
    /* Test 1:  Step  1: Write a zero pattern to all luns.
     */
    big_bird_write_zero_background_pattern();

    /* Test 1:  Step  2: Set a log hook to validate the raid group request encryption start
     *                   Set a pause hook to stop the raid group before it gets the downstream encryption status
     *                   Set a log hook to validate that encrypt re-key does not start
     *                   Set a pause hook to hold the copy the moment the job starts
     *                   Set a pause hook when the user copy job completes
     *                   Set a pause hook when the copy complete job runs
     */
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

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Set a log hook to validate the raid group request encryption start */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_REQUEST_ENCRYPT_START_MODE,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set a pause hook to stop the raid group before it gets the downstream encryption status */
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_GET_ENCRYPTION_STATUS,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set a log hooks to validate that encrypt does not start */
        fbe_test_encryption_add_hooks(rg_object_id);

        /* Set a pause hook when the copy complete job runs */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }       

    /* Test 2: Step  3: Enable system level encryption
     */
    status = fbe_api_enable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    /* Test 1:  Step  3: Start I/O (w/r/c)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, KATRINA_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Test 1:  Step  4: Start encryption
     *          Step  5: Validate encryption mode change log hook
     *          Step  6: Wait for get downstream encryption status hook
     *          Step  7: Start a user copy
     *          Step  8: Wait for copy start hook
     *          Step  9: Remove get downstream encryption status hook
     */
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

        /* Step  4: Start encryption */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Request encryption rekey rg: 0x%x ==", __FUNCTION__, rg_object_id);
        fbe_test_sep_util_start_encryption_on_rg(current_rg_config_p); 

        /* Step  5: Validate encryption mode change log hook */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                                     FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_REQUEST_ENCRYPT_START_MODE,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_LOG,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step  6: Wait for get downstream encryption status hook */
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                                     FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_GET_ENCRYPTION_STATUS,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step  7: Disable the recovery queue */
        fbe_test_sep_util_disable_recovery_queue(vd_object_id);

        /* Step  8: Start a user copy */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Start user copy rg: 0x%x dest pvd: 0x%x==", __FUNCTION__, rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Step  9: Remove get downstream encryption status hook*/
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                                FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_GET_ENCRYPTION_STATUS,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }  

    /* Test 1:  Step 10: Validate raid group encryption starts
     */
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

        /* Step 10: Validate rekey starts */
        fbe_test_sep_util_validate_encryption_mode_setup(current_rg_config_p, FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS);

        /* Step 11: Remove encryption start hooks */
        fbe_test_encryption_del_hooks(rg_object_id);

        current_rg_config_p++;
    } 

    /* Test 1:  Step 12: Validate raid group encryption starts and completes
     */
    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_FALSE);

    /* Test 1:  Step 13: Enable the recovery queue
     *          Step 14: Wait for copy start hook
     */
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

        /* Step  13: Enable the recovery queue */
        fbe_test_sep_util_enable_recovery_queue(vd_object_id);

        /* Step 14: Wait for copy to start */
        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        current_rg_config_p++;
    }  

    /* Test 1:  Step 15: Wait for copy complete hook
     *          Step 16: Remove copy complete pause hook
     *          Step 17: Validate copy completes
     */
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

        /* Step 15: Wait for copy complete hook*/
        status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                     FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step 16: Remove copy complete pause hook */
        status = fbe_test_del_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Step 17: Validate copy completes */
        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    } 

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

    /* Test 1:  Step 20: After the test are complete disable system level encryption
     */
/*
    status = fbe_api_disable_system_encryption();
    mut_printf(MUT_LOG_TEST_STATUS, "Encryption disable status %d", status);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "disabling of encryption failed\n");
*/
    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/*************************************************
 * end katrina_test_copy_during_encryption_test2()
 *************************************************/
#endif

/*!***************************************************************************
 *          katrina_test_encryption_during_copy_test1()
 *****************************************************************************
 *
 * @brief   Start a copy operation (prior to starting encryption).  Then
 *          start encryption.  The encryption mode should never transition to
 *          `re-key in progress'.
 *          
 *          1. The raid group is already encrypted and has retrieve the keys.
 *             We set a hook after the mode is changed to encryption started.
 *             Start a user copy.  Encryption should not make progress until
 *             the copy is complete.
 *
 * @param   rg_config_p - Config array to use.  This is null terminated.
 * @param   pattern - The I/O pattern (should not be zeros)
 * @param   peer_options - (only valid in dualsp)
 * 
 * @return  none
 *
 * @note    We don't need to test every encryption state.  All we care about
 *          is encryption mode transitions:
 *          1. FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED ==>
 *          2. FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS ==>
 *              FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED
 *          3. FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS ==>
 *              FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED
 *
 * @author
 *  01/27/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void katrina_test_encryption_during_copy_test1(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_data_pattern_t pattern,
                                               fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index = 0;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_object_id_t pvd_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_u32_t dest_index[FBE_TEST_MAX_RAID_GROUP_COUNT] = {0};
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_test_raid_group_disk_set_t spare_drive_location;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t num_luns, i;
    fbe_base_config_encryption_mode_t encryption_mode;
    #define IO_DURING_ENC_INCREMENT_CHECKPOINT 0x1800
    #define IO_DURING_ENC_MAX_TEST_POINTS 2
    #define IO_DURING_ENC_WAIT_IO_COUNT 5

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s starting ===", __FUNCTION__);

    katrina_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);
    fbe_test_get_pvd_object_ids(rg_config_p, &pvd_object_ids[0], 0 /* position */);
    fbe_test_get_vd_object_ids(rg_config_p, &vd_object_ids[0], 0);
    fbe_test_copy_determine_dest_index(&vd_object_ids[0], raid_group_count, &dest_index[0]);

    /* Test 1:  Validate that if the proactive copy job runs prior to change mode the copy succeeds.
     *          Step  1: Write a zero pattern to all luns
     *          Step  2: Set a log hook to validate the raid group request a start encryption
     *                   Set a pause hook to stop the raid group before it gets the downstream encryption status
     *                   Set a log hook to validate that encryption does not start
     *                   Set a pause hook to hold the copy just prior to the copy job completing
     *                   Set a pause hook when the user copy job completes
     *                   Set a pause hook when the copy complete job runs
     *          Step  3: Start I/O (w/r/c)
     *          Step  4: Start encryption
     *          Step  5: Validate encryption mode change log hook
     *          Step  6: Wait for get downstream encryption status hook
     *          Step  7: Start a user copy
     *          Step  8: Wait for copy start hook
     *          Step 10: Remove the copy start pause hook
     *          Step 11: Wait for copy complete hook
     *          Step 12: Validate encrypt start log hook is not encountered
     *          Step 13: Remove copy complete pause hook
     *          Step 14: Validate copy completes
     *          Step  9: Remove get downstream encryption status hook
     *          Step 15: Validate raid group encryption starts and completes
     *          Step 16: Stop I/O and validate no errors
     *          
     */
    /* Test 1:  Step  1: Write a zero pattern to all luns.
     */
    big_bird_write_zero_background_pattern();

    /* Test 1:  Step  2: Set a log hook to validate the raid group request encryption start
     *                   Set a pause hook to stop the raid group before it gets the downstream encryption status
     *                   Set a log hook to validate that encrypt re-key does not start
     *                   Set a pause hook to hold the copy the moment the job starts
     *                   Set a pause hook when the user copy job completes
     *                   Set a pause hook when the copy complete job runs
     */
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

        /* Make sure our objects are in the ready state.*/
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);


        mut_printf(MUT_LOG_TEST_STATUS, "%s: vd obj: 0x%x \n",
                   __FUNCTION__, vd_object_id);

        /* Set the debug hook
         */
        status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0,
                                                  0,
                                                  SCHEDULER_CHECK_VALS_GT,
                                                  SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }       
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Test 1:  Step  3: Start I/O (w/r/c)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, KATRINA_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
    fbe_test_io_wait_for_io_count(context_p, num_luns, IO_DURING_ENC_WAIT_IO_COUNT, FBE_TEST_WAIT_TIMEOUT_MS);
    fbe_api_sim_transport_set_target_server(this_sp);

    /* Test 1:  Step  4: Start encryption
     *          Step  5: Validate encryption mode change log hook
     *          Step  6: Wait for get downstream encryption status hook
     *          Step  7: Start a user copy
     *          Step  8: Wait for copy start hook
     *          Step  9: Remove get downstream encryption status hook
     */
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

        /* Step  7: Start a user copy */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Start user copy rg: 0x%x dest pvd: 0x%x==", __FUNCTION__, rg_object_id, pvd_object_id);
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

        diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_index[rg_index], &spare_drive_location);

        status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              SCHEDULER_CHECK_VALS_GT, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE, 
                                              0,
                                              0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }  


    /* Test 3: Step  3: start rekey
     */
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 1:  Step 11: Wait for copy complete hook
     *          Step 12: Validate encrypt start log hook is not encountered
     *          Step 13: Remove copy complete pause hook
     *          Step 14: Validate copy completes
     */
    for (i = 0; i < 10; i++)
	{
		fbe_api_sleep(1000);

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

            /* Validate encryption not in progress */
            fbe_api_base_config_get_encryption_mode(rg_object_id, &encryption_mode);
            MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS, encryption_mode, "Unexpected RG Encryption Mode value");

            current_rg_config_p++;
        }
	}

    /* Test 1:  Step 15: Validate raid group encryption starts and completes
     */
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

        /* Remove rebuild hooks */
        status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                  0, 
                                                  0, 
                                                  SCHEDULER_CHECK_VALS_GT, 
                                                  SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy rg: 0x%x vd: 0x%x==", rg_object_id, vd_object_id);
        status = fbe_test_copy_wait_for_completion(vd_object_id, dest_index[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    } 

	fbe_api_sleep(2000);

    /* This does all the logic of dealing with completing the raid group including 
     * the keys and the encryption mode. 
     */
    status = fbe_test_sep_util_kms_start_rekey();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
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

    /* Refresh the locations of the all extra drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/*************************************************
 * end katrina_encryption_during_copy_test1()
 *************************************************/

/*!**************************************************************
 * katrina_run_tests()
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
 *  01/24/2014  Ron Proulx  - Created.
 *
 ****************************************************************/

void katrina_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    /* Standard peer options
     */
     if (fbe_test_sep_util_get_dualsp_test_mode()) {
         peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
     }
    /* All pvds must be zeroed before enabling encryption
     */
    fbe_test_zero_wait_for_rg_zeroing_complete(rg_config_p);

    /* Test 2:  If copy is started (destination edge swapped in) before
     *          we change the encryption mode to `encryption started' then
     *          copy wins and encryption must wait for the copy to complete.
     */
    katrina_test_copy_during_encryption_test1(rg_config_p, FBE_DATA_PATTERN_LBA_PASS, peer_options);

    /* Test 3:  Test that encryption does not proceed during copy.
     */
    katrina_test_encryption_during_copy_test1(rg_config_p, FBE_DATA_PATTERN_LBA_PASS, peer_options);

#if 0
    /* Test 2:  If encryption changes the mode to `encryption started' before
     *          the copy job starts (before the validation function) then
     *          encryption wins and copy will not start until encryption is
     *          complete.
     */
    katrina_test_copy_during_encryption_test2(rg_config_p, FBE_DATA_PATTERN_LBA_PASS, peer_options);
#endif

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/**************************************
 * end katrina_run_tests()
 **************************************/
/*!**************************************************************
 * katrina_test()
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

void katrina_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &katrina_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, katrina_run_tests,
                                   KATRINA_TEST_LUNS_PER_RAID_GROUP,
                                   KATRINA_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end katrina_test()
 ******************************************/
/*!**************************************************************
 * katrina_setup()
 ****************************************************************
 * @brief
 *  Setup the katrina test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  03/28/2012 - Created. Rob Foley
 *
 ****************************************************************/
void katrina_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &katrina_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
          
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* Some of the tests here can't use a 2 drive raid 10. 
         * If the above random function chose a 2 drive r10, change it to a 4 drive. 
         */
        MUT_ASSERT_INT_EQUAL(rg_config_p[KATRINA_RAID_10_CONFIG_INDEX].raid_type, FBE_RAID_GROUP_TYPE_RAID10);
        if (rg_config_p[KATRINA_RAID_10_CONFIG_INDEX].width == 2)
        {
            rg_config_p[KATRINA_RAID_10_CONFIG_INDEX].width = 4;
        }

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, KATRINA_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, KATRINA_COPY_COUNT);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
        
        /* load kms */
        sep_config_load_kms(NULL);
    }
    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end katrina_setup()
 **************************************/
/*!**************************************************************
 * katrina_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the katrina test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  01/24/2014  Ron Proulx  - Created.
 *
 ****************************************************************/

void katrina_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end katrina_cleanup()
 ******************************************/

/*!****************************************************************************
 * katrina_dualsp_test()
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
void katrina_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &katrina_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, katrina_run_tests,
                                   KATRINA_TEST_LUNS_PER_RAID_GROUP,
                                   KATRINA_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end katrina_dualsp_test()
 ******************************************************************************/

/*!**************************************************************
 * katrina_dualsp_setup()
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
void katrina_dualsp_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &katrina_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
          
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* Some of the tests here can't use a 2 drive raid 10. 
         * If the above random function chose a 2 drive r10, change it to a 4 drive. 
         */
        MUT_ASSERT_INT_EQUAL(rg_config_p[KATRINA_RAID_10_CONFIG_INDEX].raid_type, FBE_RAID_GROUP_TYPE_RAID10);
        if (rg_config_p[KATRINA_RAID_10_CONFIG_INDEX].width == 2)
        {
            rg_config_p[KATRINA_RAID_10_CONFIG_INDEX].width = 4;
        }
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, KATRINA_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        fbe_test_rg_populate_extra_drives(rg_config_p, KATRINA_COPY_COUNT);

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

        /* Load kms */
        sep_config_load_kms_both_sps(NULL);
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************
 * end katrina_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * katrina_dualsp_cleanup()
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
void katrina_dualsp_cleanup(void)
{

    fbe_test_sep_util_print_dps_statistics();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* Destroy kms */
        fbe_test_sep_util_destroy_kms_both_sps();

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
 * end katrina_dualsp_cleanup()
 ******************************************/

/*********************************
 * end file katrina_test.c
 *********************************/


