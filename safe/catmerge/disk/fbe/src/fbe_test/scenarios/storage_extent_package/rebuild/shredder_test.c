
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

 /*!**************************************************************************
 * @file shredder_test.c
 ***************************************************************************
 *
 * @brief
 *  This test validates the proper progression of the mark nr
 *  state machine as it is interrupted by various events.
 *
 * @version
 *  09/28/2011  Deanna Heng  - Created
 *
 ***************************************************************************/

 /*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_raid_group.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"        
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_testability.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "fbe_test.h"

/*************************
 *   TEST DESCRIPTIONS
 *************************/
char * shredder_short_desc = "Test various events during eval mark nr.";
char * shredder_long_desc ="\
The Shredder Test performs SP death during various states of eval mark nr.\n\
\n\
Dependencies:\n\
        - Dual SP \n\
        - Debug Hooks for Eval Mark Needs Rebuild.\n\
\n\
STEP 1: Bring SPs up (SPA = Active SP).\n\
STEP 2: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 3: Insert debug hooks into various points of eval mark nr.\n\
STEP 4: Remove drive(s) to mark drives rebuild logging\n\
STEP 5: Reinsert drive(s)\n\
STEP 6: Eval Mark NR hook will be hit. Perform necessary action(s)\n\
    i.e. Peer SP reboot, dual SP reboot, removing another drive in the rg\n\
STEP 7: Remove hook to allow eval mark nr to continue.\n\
STEP 8: Check appropriate drives are or are not marked as rebuild logging.\n\
STEP 9: Check all rebuilds complete as necessary.\n\
TODO: Add test cases for inserting a different drive and starting a full rebuild\n\
\n\
Description Last Updated: 9/27/2011\n\
\n";

/*!*******************************************************************
 * @def SHREDDER_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define SHREDDER_RAID_TYPE_COUNT 5

/*!*******************************************************************
 * @def SHREDDER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define SHREDDER_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def SHREDDER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SHREDDER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def SHREDDER_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SHREDDER_WAIT_MSEC 1000 * 120

/*!*******************************************************************
 * @def FBE_API_POLLING_INTERVAL
 *********************************************************************
 * @brief Number of milliseconds we should wait to poll the SP
 *
 *********************************************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def SHREDDER_TC_COUNT
 *********************************************************************
 * @brief The number of total test cases for darkwing duck
 *
 *********************************************************************/
#define SHREDDER_TC_COUNT 6

/*!*******************************************************************
 * @def SHREDDER_ACTIVE_SP
 *********************************************************************
 * @brief The active SP 
 *
 *********************************************************************/
#define SHREDDER_ACTIVE_SP FBE_SIM_SP_A

/*!*******************************************************************
 * @def SHREDDER_PASSIVE_SP
 *********************************************************************
 * @brief The passive SP 
 *
 *********************************************************************/
#define SHREDDER_PASSIVE_SP FBE_SIM_SP_B

/*!*******************************************************************
 * @def sep_rebuild_utils_number_physical_objects_g
 *********************************************************************
 * @brief number of physical objects
 *
 *********************************************************************/
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

/*!*******************************************************************
 * @var shredder_random_test_cases
 *********************************************************************
 * @brief This is the array of test cases from the lot of shredder
 *        test cases.
 *
 *********************************************************************/

fbe_monitor_test_case_t shredder_random_test_cases[SHREDDER_TC_COUNT];

/*!*******************************************************************
 * @var shredder_test_cases
 *********************************************************************
 * @brief This is the set of test cases to be run
 *
 *********************************************************************/
fbe_monitor_test_case_t shredder_test_cases[SHREDDER_TC_COUNT] = 
{
    {"REBOOT during EVAL MARK NR REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"REBOOT during EVAL MARK NR STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"REBOOT during EVAL MARK NR DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during EVAL MARK NR REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during EVAL MARK NR STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during EVAL MARK NR DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
     FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
};

/**************************************
 * end shredder_test_cases()
 **************************************/

/*!*******************************************************************
 * @var shredder_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/

fbe_test_rg_configuration_t shredder_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];


void shredder_run_tests_in_parallel(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_monitor_test_case_t *test_cases_p,
                                    fbe_sim_transport_connection_target_t target_sp); 
void shredder_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void shredder_remove_failed_drives(fbe_test_rg_configuration_t *rg_config_p, fbe_monitor_test_case_t *test_cases_p);


/*!**************************************************************
 * shredder_run_test_config()
 ****************************************************************
 * @brief
 *  Run eval mark nr test cases on the rg config
 *
 * @param rg_config_p - the raid group configuration to test
 *        context_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_monitor_test_case_t   *test_cases_p = NULL;

    test_cases_p = (fbe_monitor_test_case_t*) context_p;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Shredder Test on Active SP=====\n");

    shredder_run_tests_in_parallel(rg_config_p, test_cases_p, FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Shredder Test on Passive SP=====\n");

    shredder_run_tests_in_parallel(rg_config_p, test_cases_p, FBE_SIM_SP_B);

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Shredder Test with dual SP crash=====\n");

    shredder_dual_sp_crash_test(rg_config_p, test_cases_p);


    return;
}
/******************************************
 * end shredder_run_test_config()
 ******************************************/

/*!**************************************************************
 * shredder_run_tests_in_parallel()
 ****************************************************************
 * @brief
 *  Run eval mark nr test cases on rg config
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_cases_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  11/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_run_tests_in_parallel(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_monitor_test_case_t *test_cases_p,
                                    fbe_sim_transport_connection_target_t target_sp) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;
    fbe_u32_t i = 0;
    fbe_sim_transport_connection_target_t peer_sp;
    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);
    fbe_api_sim_transport_set_target_server(target_sp);

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(SHREDDER_ACTIVE_SP);
        /* setup hooks on active SP */
    for (index = 0; index < raid_group_count; index++) {
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove>1) {
            test_cases_p[index].num_to_remove++;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        sep_rebuild_utils_get_random_drives(&rg_config_p[index], test_cases_p[index].num_to_remove, test_cases_p[index].drive_slots_to_remove);
        captain_planet_set_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
    }

    /* Remove a drive to enter eval rebuild logging*/
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Remove a drive for rgid: %d\n",rg_config_p[index].raid_group_id);
        sep_rebuild_utils_number_physical_objects_g -= 1;
        /*  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
	        is marked. */
        sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[index], 
                                                  test_cases_p[index].drive_slots_to_remove[0], 
                                                  sep_rebuild_utils_number_physical_objects_g, 
                                                  test_cases_p[index].drive_info);
        mut_printf(MUT_LOG_TEST_STATUS, "Verify rebuild logging for test case: %s", test_cases_p[index].test_case_name);
        sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_check_for_reb_restart(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
    }

    /* Reinsert the drive, and we should hit the eval mark nr hook
     */
    for (index = 0; index < raid_group_count; index++) {
        sep_rebuild_utils_number_physical_objects_g += 1;
        /*fbe_test_pp_util_reinsert_drive(rg_config_p[index].rg_disk_set[test_cases_p[index].drive_slots_to_remove[0]].bus,
                                        rg_config_p[index].rg_disk_set[test_cases_p[index].drive_slots_to_remove[0]].enclosure,
                                        rg_config_p[index].rg_disk_set[test_cases_p[index].drive_slots_to_remove[0]].slot, 
                                        test_cases_p[index].drive_info[0]);*/
        sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0], 
                                                        sep_rebuild_utils_number_physical_objects_g, &(test_cases_p[index].drive_info[0]));
        captain_planet_wait_for_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
        captain_planet_wait_for_local_state(&rg_config_p[index], &test_cases_p[index]);
    }

    /* Remove a drive to break rg */
    for (index = 0; index < raid_group_count; index++) {
        if (test_cases_p[index].num_to_remove > 1) {
            mut_printf(MUT_LOG_TEST_STATUS, "Remove another drive(s) in rgid to break rg %d\n",rg_config_p[index].raid_group_id);
            sep_rebuild_utils_number_physical_objects_g -= 1;
            /*  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
    	        is marked. */
            sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[index], 
                                                      test_cases_p[index].drive_slots_to_remove[1], 
                                                      sep_rebuild_utils_number_physical_objects_g, 
                                                      &(test_cases_p[index].drive_info[1]));
            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
                sep_rebuild_utils_number_physical_objects_g -= 1;
                sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[index], 
                                                      test_cases_p[index].drive_slots_to_remove[2], 
                                                      sep_rebuild_utils_number_physical_objects_g, 
                                                      &(test_cases_p[index].drive_info[2]));
            }
            /* verify the raid group is now broken */
            status = fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_FAIL, SHREDDER_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        }
    }

    if (target_sp == FBE_SIM_SP_A) {
        /* Reboot the other SP */
        darkwing_duck_shutdown_peer_sp();
        fbe_api_sleep(2000);
        mut_printf(MUT_LOG_TEST_STATUS, "1now startup the peer sp and call shredder remove failed drives");
        darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, (fbe_test_rg_config_test)shredder_remove_failed_drives);
        fbe_api_sleep(SHREDDER_WAIT_MSEC);

        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /*for (index = 0; index < raid_group_count; index++) {
            if (test_cases_p[index].num_to_remove == 1) {
                // verify the raid group get to ready state in reasonable time 
                fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                        FBE_LIFECYCLE_STATE_READY, SHREDDER_WAIT_MSEC,
                                                        FBE_PACKAGE_ID_SEP_0);
            }
        }*/
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* remove hooks on target SP */
        for (index = 0; index < raid_group_count; index++) {
            mut_printf(MUT_LOG_TEST_STATUS, "Remove hook for test case: %s", test_cases_p[index].test_case_name);
            captain_planet_remove_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
        }
    } else {
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Reboot the other SP */
        darkwing_duck_shutdown_peer_sp();
        fbe_api_sleep(2000);
        mut_printf(MUT_LOG_TEST_STATUS, "now startup the peer sp and call shredder remove failed drives");
        darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, (fbe_test_rg_config_test)shredder_remove_failed_drives);
        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        for (index = 0; index < raid_group_count; index++) {
            if (test_cases_p[index].num_to_remove == 1) {
                /* verify the raid group get to ready state in reasonable time */
                fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                        FBE_LIFECYCLE_STATE_READY, SHREDDER_WAIT_MSEC,
                                                        FBE_PACKAGE_ID_SEP_0);
            }
        }
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    for (index = 0; index < raid_group_count; index++) {
        for (i=1;i<test_cases_p[index].num_to_remove;i++) {
            sep_rebuild_utils_number_physical_objects_g += 1;
            sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[i], 
                                                        sep_rebuild_utils_number_physical_objects_g, &(test_cases_p[index].drive_info[i]));
        }
    }
    /* Verify the rebuild checkpoint is non-zero
     * We will need to update this for the case where a drives really does need to be rebuilt
     */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Check rebuild logging is cleared on rgid %d", test_cases_p[index].rg_object_id);
        sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_logging_clear(&rg_config_p[index]);
        mut_printf(MUT_LOG_TEST_STATUS, "Check rebuild has started and completed for rgid %d", test_cases_p[index].rg_object_id);
		sep_rebuild_utils_wait_for_rb_to_start(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_verify_reinserted_drive(&rg_config_p[index],
                                                  test_cases_p[index].drive_slots_to_remove[0],
                                                  sep_rebuild_utils_number_physical_objects_g,
                                                  test_cases_p[index].drive_info);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove >1) {
            test_cases_p[index].num_to_remove--;
        }
    }
}
/******************************************
 * end shredder_run_tests_in_parallel()
 ******************************************/

/*!**************************************************************
 * shredder_dual_sp_crash_test()
 ****************************************************************
 * @brief
 *  Run eval mark nr tests with dual sp reboot on rg config
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_cases_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  11/28/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_dual_sp_crash_test(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_monitor_test_case_t *test_cases_p) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;
    fbe_u32_t i = 0;

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_sim_transport_set_target_server(SHREDDER_ACTIVE_SP);
    /* setup hooks on active SP */
    for (index = 0; index < raid_group_count; index++) {
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove>1) {
            test_cases_p[index].num_to_remove++;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        sep_rebuild_utils_get_random_drives(&rg_config_p[index], test_cases_p[index].num_to_remove, test_cases_p[index].drive_slots_to_remove);
        fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                FBE_LIFECYCLE_STATE_READY, SHREDDER_WAIT_MSEC,
                                                FBE_PACKAGE_ID_SEP_0);
        captain_planet_set_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
    }

    /* Remove a drive to enter eval rebuild logging*/
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Remove a drive for rgid: %d\n",rg_config_p[index].raid_group_id);
        sep_rebuild_utils_number_physical_objects_g -= 1;
        /*  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
	        is marked. */
        sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[index], 
                                                  test_cases_p[index].drive_slots_to_remove[0], 
                                                  sep_rebuild_utils_number_physical_objects_g, 
                                                  test_cases_p[index].drive_info);
        mut_printf(MUT_LOG_TEST_STATUS, "Verify rebuild logging for test case: %s", test_cases_p[index].test_case_name);
        sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_check_for_reb_restart(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
    }

    /* Reinsert the drive, and we should hit the eval mark nr hook
     */
    for (index = 0; index < raid_group_count; index++) {
        sep_rebuild_utils_number_physical_objects_g += 1;
        sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0], 
                                                        sep_rebuild_utils_number_physical_objects_g, &(test_cases_p[index].drive_info[0]));
        captain_planet_wait_for_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
        captain_planet_wait_for_local_state(&rg_config_p[index], &test_cases_p[index]);
    }

    /* Remove a drive to enter eval rebuild logging*/
    for (index = 0; index < raid_group_count; index++) {
        if (test_cases_p[index].num_to_remove > 1) {
            mut_printf(MUT_LOG_TEST_STATUS, "Remove another drive(s) in rgid to break rg %d\n",rg_config_p[index].raid_group_id);
            sep_rebuild_utils_number_physical_objects_g -= 1;
            /*  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
    	        is marked. */
            sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[index], 
                                                      test_cases_p[index].drive_slots_to_remove[1], 
                                                      sep_rebuild_utils_number_physical_objects_g, 
                                                      &(test_cases_p[index].drive_info[1]));
            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
                sep_rebuild_utils_number_physical_objects_g -= 1;
                sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[index], 
                                                      test_cases_p[index].drive_slots_to_remove[2], 
                                                      sep_rebuild_utils_number_physical_objects_g, 
                                                      &(test_cases_p[index].drive_info[2]));
            }
            /* verify the raid group is now broken */
            status = fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_FAIL, SHREDDER_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        }
    }

    shredder_crash_both_sps(rg_config_p, test_cases_p, (fbe_test_rg_config_test)shredder_remove_failed_drives);
    for (index = 0; index < raid_group_count; index++) {
        if (test_cases_p[index].num_to_remove == 1) {
                /* verify the raid group get to ready state in reasonable time */
            fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                    FBE_LIFECYCLE_STATE_READY, SHREDDER_WAIT_MSEC,
                                                    FBE_PACKAGE_ID_SEP_0);
        }
    }

    for (index = 0; index < raid_group_count; index++) {
        for (i=1;i<test_cases_p[index].num_to_remove;i++) {
            sep_rebuild_utils_number_physical_objects_g += 1;
            sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[i], 
                                                        sep_rebuild_utils_number_physical_objects_g, &(test_cases_p[index].drive_info[i]));
        }
    }
    /* Verify the rebuild checkpoint is non-zero
     * We will need to update this for the case where a drives really does need to be rebuilt
     */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Check rebuild logging is cleared on rgid %d", test_cases_p[index].rg_object_id);
        sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_logging_clear(&rg_config_p[index]);
        mut_printf(MUT_LOG_TEST_STATUS, "Check rebuild has started and completed for rgid %d", test_cases_p[index].rg_object_id);
		sep_rebuild_utils_wait_for_rb_to_start(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_verify_reinserted_drive(&rg_config_p[index],
                                                  test_cases_p[index].drive_slots_to_remove[0],
                                                  sep_rebuild_utils_number_physical_objects_g,
                                                  test_cases_p[index].drive_info);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove >1) {
            test_cases_p[index].num_to_remove--;
        }
    }
}
/******************************************
 * end shredder_dual_sp_crash_test()
 ******************************************/

/*!**************************************************************
 * shredder_crash_both_sps()
 ****************************************************************
 * @brief
 *  Crash both SPs and then bring them back up with the correct config
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_cases_p - list of test cases to run     
 *        func_p - pointer to the function to remove drives        
 *
 * @return None.   
 *
 * @author
 *  11/28/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_crash_both_sps(fbe_test_rg_configuration_t * rg_config_p, 
                             fbe_monitor_test_case_t *test_cases_p,
                             fbe_test_rg_config_test func_p) {
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = 0;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    if (func_p == NULL) {
        func_p = (fbe_test_rg_config_test)shredder_remove_failed_drives;
    }

    mut_printf(MUT_LOG_LOW, "Crash both SPs");
    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    // Destroy the SPs and its current session 
    fbe_test_base_suite_destroy_sp();

    // Start the SPs with a new session 
    fbe_test_base_suite_startup_post_powercycle();

    fbe_test_sep_duplicate_config(rg_config_p, temp_config);

    // load the physical config
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    elmo_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    elmo_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    // remove appropriate drives
    if (test_cases_p != NULL) {
        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        func_p(rg_config_p, test_cases_p);

        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        func_p(rg_config_p, test_cases_p);
    }

    sep_config_load_sep_and_neit_both_sps();
  
}
/******************************************
 * end shredder_crash_both_sps()
 ******************************************/

/*!**************************************************************
 * shredder_remove_failed_drives()
 ****************************************************************
 * @brief
 *  Remove the failed drives after a hard reboot
 *
 * @param rg_config_p - raid group configs recreated    
 *        test_cases_p - contains failed drives to remove   
 *
 * @return
 *
 * @author
 *  11/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_remove_failed_drives(fbe_test_rg_configuration_t * rg_config_p, fbe_monitor_test_case_t *test_cases_p) {
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_status_t status = FBE_STATUS_OK;

    // remove all drives but the first drive since we already reinserted it prior to the hard reboot
    for (i=0; i< raid_group_count; i++) {
        for (j=1; j< test_cases_p[i].num_to_remove; j++) {
            status = fbe_test_pp_util_pull_drive(rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].bus,
                                                 rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].enclosure,
                                                 rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].slot, 
                                                 &test_cases_p[i].drive_info[j]);
            mut_printf(MUT_LOG_TEST_STATUS,"Removed drive %d_%d_%d from raid group id %d",
                       rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].bus,
                       rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].enclosure,
                       rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].slot, 
                       test_cases_p[i].rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
}
/******************************************
 * end shredder_remove_failed_drives()
 ******************************************/

/*!**************************************************************
 * shredder_choose_test_cases
 ****************************************************************
 * @brief
 *  Create a random configuration from the rg_config
 *
 * @param test_cases_to_run_p - pointer to the test cases we will choose    
 *        num_test_cases - the number of test cases we have chosen    
 *
 * @return None
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_choose_test_cases(fbe_monitor_test_case_t * test_cases_to_run_p, fbe_u32_t * num_test_cases) {
    fbe_u32_t random_num = 0;
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;

    for (i=0; i < SHREDDER_TC_COUNT; i++) {
        // We're aiming to select 1/3 test cases...
        random_num = fbe_random() % 3; 
        if (random_num == 0) {
            test_cases_to_run_p[j] = shredder_test_cases[i];
            j++;
        }
    }

    // if we failed to select any test cases, select one now
    if (j==0) {
        test_cases_to_run_p[0] = shredder_test_cases[fbe_random() % SHREDDER_TC_COUNT];
        j++;
    }

    *num_test_cases = j;
}
/******************************************
 * end shredder_choose_test_cases()
 ******************************************/

/*!**************************************************************
 * shredder_test()
 ****************************************************************
 * @brief
 *  Run eval rebuild logging test
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_dualsp_test(void)
{
    fbe_u32_t                   testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* rtl 1 and rtl 0 are the same for now */
    if (testing_level > 0) {
        fbe_test_run_test_on_rg_config(&shredder_raid_group_config_random[0], 
                                       &shredder_test_cases[0], 
                                       shredder_run_test_config,
                                       SHREDDER_LUNS_PER_RAID_GROUP,
                                       SHREDDER_CHUNKS_PER_LUN);
    } else {
        fbe_test_run_test_on_rg_config(&shredder_raid_group_config_random[0], 
                                       &shredder_random_test_cases[0], 
                                       shredder_run_test_config,
                                       SHREDDER_LUNS_PER_RAID_GROUP,
                                       SHREDDER_CHUNKS_PER_LUN);
    }

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end shredder_test()
 ******************************************/

/*!**************************************************************
 * shredder_setup()
 ****************************************************************
 * @brief
 *  Setup for darkwing duck test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void shredder_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t raid_group_count = 0;
    fbe_u32_t num_test_cases = SHREDDER_TC_COUNT;

    fbe_u32_t random_seed = (fbe_u32_t)fbe_get_time();

    /* The random seed allows us to make tests more random. 
     * We seed the random number generator with the time. 
     * We also allow the user to provide the random seed so that they can 
     * be able to reproduce failures more easily. 
     */
    fbe_test_sep_util_get_cmd_option_int("-test_random_seed", &random_seed);

    mut_printf(MUT_LOG_TEST_STATUS, "random seed is: 0x%x", random_seed);
    fbe_random_set_seed(random_seed);

    if (test_level == 0) {
        shredder_choose_test_cases(&shredder_random_test_cases[0], &num_test_cases);
    }

    darkwing_duck_create_random_config(&shredder_raid_group_config_random[0], num_test_cases);

    if (fbe_test_util_is_simulation()) {
        raid_group_count = fbe_test_get_rg_array_length(&shredder_raid_group_config_random[0]);
        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&shredder_raid_group_config_random[0]);

        /* Setup the physical config for the raid groups on both SPs
         */
        fbe_api_sim_transport_set_target_server(SHREDDER_ACTIVE_SP);
        elmo_create_physical_config_for_rg(&shredder_raid_group_config_random[0], raid_group_count);

        fbe_api_sim_transport_set_target_server(SHREDDER_PASSIVE_SP);
        elmo_create_physical_config_for_rg(&shredder_raid_group_config_random[0], raid_group_count);

        sep_config_load_sep_and_neit_both_sps();
        /* Set the default target server back to A
         */
        fbe_api_sim_transport_set_target_server(SHREDDER_ACTIVE_SP);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************
 * end shredder_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * shredder_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the darkwing duck test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/

void shredder_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
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
 * end shredder_dual_sp_cleanup()
 ******************************************/

 /*************************
 * end file shredder_test.c
 *************************/



