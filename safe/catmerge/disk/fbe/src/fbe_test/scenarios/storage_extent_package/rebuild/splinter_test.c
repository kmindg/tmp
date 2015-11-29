/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

 /*!**************************************************************************
 * @file splinter_test.c
 ***************************************************************************
 *
 * @brief
 *  This test validates the proper progression of the clear rl
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

/*************************
 *   TEST DESCRIPTIONS
 *************************/
char * splinter_short_desc = "Test various events during clear rebuild logging.";
char * splinter_long_desc ="\
The Splinter Test performs SP death, during various states of clear rebuild logging.\n\
\n\
Dependencies:\n\
        - Dual SP \n\
        - Debug Hooks for Clear Rebuild Logging.\n\
\n\
STEP 1: Bring SPs up (SPA = Active SP).\n\
STEP 2: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 3: Insert debug hooks into various points of clear rebuild logging.\n\
STEP 4: Remove drive(s) to mark drives rebuild logging\n\
STEP 5: Reinsert drive(s)\n\
STEP 6: Clear Rebuild Logging hook will be hit. Perform necessary action(s)\n\
    i.e. Peer SP reboot, Dual SP reboot, Remove another drive in the raid group\n\
STEP 7: Remove hook to allow clear rb logging to continue.\n\
STEP 8: Check appropriate drives are or are not marked as rebuild logging.\n\
\n\
Description Last Updated: 9/27/2011\n\
\n";

/*!*******************************************************************
 * @def SPLINTER_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define SPLINTER_RAID_TYPE_COUNT 5

/*!*******************************************************************
 * @def SPLINTER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define SPLINTER_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def SPLINTER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SPLINTER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def SPLINTER_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SPLINTER_WAIT_MSEC 1000 * 120

/*!*******************************************************************
 * @def FBE_API_POLLING_INTERVAL
 *********************************************************************
 * @brief Number of milliseconds we should wait to poll the SP
 *
 *********************************************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def SPLINTER_TC_COUNT
 *********************************************************************
 * @brief The number of total test cases for darkwing duck
 *
 *********************************************************************/
#define SPLINTER_TC_COUNT 6

/*!*******************************************************************
 * @def SPLINTER_ACTIVE_SP
 *********************************************************************
 * @brief The active SP 
 *
 *********************************************************************/
#define SPLINTER_ACTIVE_SP FBE_SIM_SP_A

/*!*******************************************************************
 * @def SPLINTER_PASSIVE_SP
 *********************************************************************
 * @brief The passive SP 
 *
 *********************************************************************/
#define SPLINTER_PASSIVE_SP FBE_SIM_SP_B

/*!*******************************************************************
 * @def sep_rebuild_utils_number_physical_objects_g
 *********************************************************************
 * @brief number of physical objects
 *
 *********************************************************************/
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

/*!*******************************************************************
 * @var splinter_random_test_cases
 *********************************************************************
 * @brief This is the array of test cases from the lot of splinter
 *        test cases.
 *
 *********************************************************************/

fbe_monitor_test_case_t splinter_random_test_cases[SPLINTER_TC_COUNT];

/*!*******************************************************************
 * @var splinter_test_cases
 *********************************************************************
 * @brief This is the set of test cases to be run
 *
 *********************************************************************/
fbe_monitor_test_case_t splinter_test_cases[SPLINTER_TC_COUNT] = 
{
    {"REBOOT during CLEAR RL REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
     FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during CLEAR RL STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
     FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"REBOOT during CLEAR RL DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
     FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during CLEAR RL REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
     FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during CLEAR RL STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
     FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    },
    {"Remove drive during CLEAR RL DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
     FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL}
    }
};

/**************************************
 * end splinter_test_cases()
 **************************************/

/*!*******************************************************************
 * @var splinter_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/

fbe_test_rg_configuration_t splinter_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];


void splinter_run_tests_in_parallel(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_monitor_test_case_t *test_cases_p,
                                    fbe_sim_transport_connection_target_t target_sp); 
void splinter_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void splinter_remove_failed_drives(fbe_test_rg_configuration_t * rg_config_p, fbe_monitor_test_case_t *test_cases_p);
void splinter_choose_test_cases(fbe_monitor_test_case_t * test_cases_to_run_p, fbe_u32_t * num_test_cases);


/*!**************************************************************
 * splinter_run_test_config()
 ****************************************************************
 * @brief
 *  Run clear rebuild logging test cases for rg config
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

void splinter_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_monitor_test_case_t   *test_cases_p = NULL;

    test_cases_p = (fbe_monitor_test_case_t*) context_p;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Splinter Test on Active SP=====\n");

    splinter_run_tests_in_parallel(rg_config_p, test_cases_p,FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Splinter Test on Passive SP=====\n");

    splinter_run_tests_in_parallel(rg_config_p, test_cases_p, FBE_SIM_SP_B);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Splinter Test with dual SP crash=====\n");

    shredder_dual_sp_crash_test(rg_config_p, test_cases_p);

    return;
}
/******************************************
 * end splinter_run_test_config()
 ******************************************/

/*!**************************************************************
 * splinter_run_tests_in_parallel()
 ****************************************************************
 * @brief
 *  SP reboot during various stages of clear rebuild logging
 *
 * @param rg_config_p - the raid group configuration to test
 *        context_p - list of test cases to run   
 *        target_sp - SP to run the test against (reboot it's peer)          
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void splinter_run_tests_in_parallel(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_monitor_test_case_t *test_cases_p, 
                                    fbe_sim_transport_connection_target_t target_sp) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;
    fbe_u32_t i =0;
    fbe_sim_transport_connection_target_t peer_sp;
    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (index = 0; index < raid_group_count; index++) {
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove>1) {
            test_cases_p[index].num_to_remove++;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        sep_rebuild_utils_get_random_drives(&rg_config_p[index], test_cases_p[index].num_to_remove, test_cases_p[index].drive_slots_to_remove);
        fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                FBE_LIFECYCLE_STATE_READY, SPLINTER_WAIT_MSEC,
                                                FBE_PACKAGE_ID_SEP_0);
        captain_planet_set_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
    }


    /* Remove a drive to mark rebuild logging*/
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

    /* Reinsert the drive, and we should hit the clear rebuild logging hook
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
                                                         FBE_LIFECYCLE_STATE_FAIL, SPLINTER_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        }
    }


    if (target_sp == FBE_SIM_SP_A) {
        /* Reboot the other SP */
        darkwing_duck_shutdown_peer_sp();
        fbe_api_sleep(2000);
        mut_printf(MUT_LOG_TEST_STATUS, "1now startup the peer sp and call splinter remove failed drives");
        darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, (fbe_test_rg_config_test)splinter_remove_failed_drives);
        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sleep(SPLINTER_WAIT_MSEC);

        /*for (index = 0; index < raid_group_count; index++) {
            if (test_cases_p[index].num_to_remove == 1) {
                // verify the raid group get to ready state in reasonable time 
                fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                        FBE_LIFECYCLE_STATE_READY, SPLINTER_WAIT_MSEC,
                                                        FBE_PACKAGE_ID_SEP_0);
            }
        }*/
        fbe_api_sim_transport_set_target_server(target_sp);
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
        mut_printf(MUT_LOG_TEST_STATUS, "now startup the peer sp and call splinter remove failed drives");
        darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, (fbe_test_rg_config_test)splinter_remove_failed_drives);
        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        for (index = 0; index < raid_group_count; index++) {
            if (test_cases_p[index].num_to_remove == 1) {
                /* verify the raid group get to ready state in reasonable time */
                fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                        FBE_LIFECYCLE_STATE_READY, SPLINTER_WAIT_MSEC,
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

    /* Verify the rebuild checkpoint is non-zero and rebuild logging is cleared successfully
     */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Check rebuild logging is cleared for rgid %d", test_cases_p[index].rg_object_id);
        sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_logging_clear(&rg_config_p[index]);
        mut_printf(MUT_LOG_TEST_STATUS, "Check rebuild checkpoint is complete %d", test_cases_p[index].rg_object_id);
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
 * end splinter_run_tests_in_parallel()
 ******************************************/

/*!**************************************************************
 * splinter_remove_failed_drives()
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
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void splinter_remove_failed_drives(fbe_test_rg_configuration_t * rg_config_p, fbe_monitor_test_case_t *test_cases_p) {
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_status_t status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "we are in splinter_remove_failed_drives");

    // Remove drives other than the first removed drive since we reinserted that drive before the hard reboot
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
 * end splinter_remove_failed_drives()
 ******************************************/

/*!**************************************************************
 * splinter_choose_test_cases
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
void splinter_choose_test_cases(fbe_monitor_test_case_t * test_cases_to_run_p, fbe_u32_t * num_test_cases) {
    fbe_u32_t random_num = 0;
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;    

    for (i=0; i < SPLINTER_TC_COUNT; i++) {
        // We're aiming to select ~1/3 test cases...
        random_num = fbe_random() % 3; 
        if (random_num == 0) {
            test_cases_to_run_p[j] = splinter_test_cases[i];
            j++;
        }
    }

    // if we failed to select any test cases, select one now
    if (j==0) {
        test_cases_to_run_p[0] = splinter_test_cases[fbe_random() % SPLINTER_TC_COUNT];
        j++;
    }

    *num_test_cases = j;
}
/******************************************
 * end splinter_choose_test_cases()
 ******************************************/

/*!**************************************************************
 * splinter_test()
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
void splinter_dualsp_test(void)
{
    fbe_u32_t                   testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* rtl 1 and rtl 0 are the same for now */
    if (testing_level > 0) {
        fbe_test_run_test_on_rg_config(&splinter_raid_group_config_random[0], 
                                       &splinter_test_cases[0], 
                                       splinter_run_test_config,
                                       SPLINTER_LUNS_PER_RAID_GROUP,
                                       SPLINTER_CHUNKS_PER_LUN);
    } else {
        fbe_test_run_test_on_rg_config(&splinter_raid_group_config_random[0], 
                                       &splinter_random_test_cases[0], 
                                       splinter_run_test_config,
                                       SPLINTER_LUNS_PER_RAID_GROUP,
                                       SPLINTER_CHUNKS_PER_LUN);
    }

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end splinter_dualsp_test()
 ******************************************/


/*!**************************************************************
 * splinter_setup()
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
void splinter_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t raid_group_count = 0;
    fbe_u32_t num_test_cases = SPLINTER_TC_COUNT;

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
        splinter_choose_test_cases(&splinter_random_test_cases[0], &num_test_cases);
    }

    darkwing_duck_create_random_config(&splinter_raid_group_config_random[0], num_test_cases);

    if (fbe_test_util_is_simulation()) {
        raid_group_count = fbe_test_get_rg_array_length(&splinter_raid_group_config_random[0]);
        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&splinter_raid_group_config_random[0]);

        /* Setup the physical config for the raid groups on both SPs
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(&splinter_raid_group_config_random[0], raid_group_count);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(&splinter_raid_group_config_random[0], raid_group_count);

        sep_config_load_sep_and_neit_both_sps();
        /* Set the default target server back to A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************
 * end splinter_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * splinter_cleanup()
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

void splinter_dualsp_cleanup(void)
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
 * end splinter_dual_sp_cleanup()
 ******************************************/

 /*************************
 * end file splinter_test.c
 *************************/



