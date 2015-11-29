/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file captain_planet_test.c
 ***************************************************************************
 *
 * @brief
 *  This test validates the proper progression of eval rebuild logging
 *
 * @version
 *  09/21/2011  Deanna Heng  - Created
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
#include "fbe_test.h"
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"

/*************************
 *   TEST DESCRIPTIONS
 *************************/
char * captain_planet_short_desc = "test various events during peer join";
char * captain_planet_long_desc ="\
The Captain Planet Test performs SP death, during various states of eval rebuild logging.\n\
\n\
Dependencies:\n\
        - Dual SP \n\
        - Debug Hooks for EVAL RL.\n\
\n\
STEP 1: Bring 1 SP up (SPA).\n\
STEP 2: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 3: Insert debug hooks into various points of eval rebuild logging.\n\
        - bring down active SP\n\
        - bring down passive SP\n\
STEP 4: Bring active/passive SP up.\n\
STEP 5: Perform hook operation during various states of eval rebuild logging.\n\
        - REQUEST\n\
        - STARTED\n\
        - DONE\n\
STEP 6: Verify that all flags are cleared and operations are completed successfully.\n\
STEP 7: Cleanup\n\
        - Destroy objects\n";

/*!*******************************************************************
 * @def CAPTAIN_PLANET_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define CAPTAIN_PLANET_RAID_TYPE_COUNT 5

/*!*******************************************************************
 * @def CAPTAIN_PLANET_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define CAPTAIN_PLANET_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def CAPTAIN_PLANET_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define CAPTAIN_PLANET_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def CAPTAIN_PLANET_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for join state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define CAPTAIN_PLANET_WAIT_MSEC 1000 * 120

/*!*******************************************************************
 * @def FBE_API_POLLING_INTERVAL
 *********************************************************************
 * @brief Number of milliseconds we should wait to poll the SP
 *
 *********************************************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def CAPTAIN_PLANET_TC_COUNT
 *********************************************************************
 * @brief The number of total test cases for darkwing duck
 *
 *********************************************************************/
#define CAPTAIN_PLANET_TC_COUNT 8

/*!*******************************************************************
 * @def CAPTAIN_PLANET_DIVISOR
 *********************************************************************
 * @brief The number to use as a divisor when choosing test cases
 *
 *********************************************************************/
#define CAPTAIN_PLANET_DIVISOR 3

/*!*******************************************************************
 * @def CAPTAIN_PLANET_ACTIVE_SP
 *********************************************************************
 * @brief The active SP 
 *
 *********************************************************************/
#define CAPTAIN_PLANET_ACTIVE_SP FBE_SIM_SP_A

/*!*******************************************************************
 * @def CAPTAIN_PLANET_PASSIVE_SP
 *********************************************************************
 * @brief The passive SP 
 *
 *********************************************************************/
#define CAPTAIN_PLANET_PASSIVE_SP FBE_SIM_SP_B

/*!*******************************************************************
 * @def sep_rebuild_utils_number_physical_objects_g
 *********************************************************************
 * @brief number of physical objects
 *
 *********************************************************************/
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

/*!*******************************************************************
 * @var captain_planet_test_cases
 *********************************************************************
 * @brief This is the set of test cases to be run
 *
 *********************************************************************/
fbe_monitor_test_case_t captain_planet_test_cases[CAPTAIN_PLANET_TC_COUNT] = 
{
    {"REBOOT during EVAL RL REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"REBOOT during EVAL RL STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"REBOOT during EVAL RL STARTED2",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"REBOOT during EVAL RL DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Remove drive during EVAL RL REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Remove drive during EVAL RL STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Remove drive during EVAL RL STARTED2",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Remove drive during EVAL RL DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
     FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
     FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP,
     FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
};

/**************************************
 * end captain_planet_test_cases()
 **************************************/

/*!*******************************************************************
 * @var captain_planet_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/

fbe_test_rg_configuration_t captain_planet_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];

/*!*******************************************************************
 * @var captain_planet_random_test_cases
 *********************************************************************
 * @brief This is the array of test cases from the lot of captain
 *        planet test cases.
 *
 *********************************************************************/

fbe_monitor_test_case_t captain_planet_random_test_cases[CAPTAIN_PLANET_TC_COUNT];

/*!*******************************************************************
 * @var captain_planet_num_test_cases_to_run
 *********************************************************************
 * @brief This is the array of test cases from the lot of darkwing
 *        duck test cases.
 *
 *********************************************************************/

fbe_u32_t captain_planet_num_test_cases_to_run = CAPTAIN_PLANET_TC_COUNT;

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
void captain_planet_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void captain_planet_reboot_passive_SP_during_eval_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_monitor_test_case_t *test_cases_p);
void captain_planet_reboot_active_SP_during_eval_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_monitor_test_case_t *test_cases_p);
void captain_planet_reboot_both_SPs_during_eval_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_monitor_test_case_t *test_cases_p);
fbe_status_t captain_planet_wait_for_local_state_by_obj_id(fbe_object_id_t object_id, 
                                                           fbe_raid_group_local_state_t expected_local_state,
                                                           fbe_u32_t timeout_ms);
void captain_planet_choose_test_cases(fbe_monitor_test_case_t * test_cases_to_run_p, fbe_u32_t * num_test_cases);



/*!**************************************************************
 * captain_planet_run_test_config()
 ****************************************************************
 * @brief
 *  Run the tests for eval rebuild logging
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

void captain_planet_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_monitor_test_case_t   *test_cases_p = NULL;

    test_cases_p = (fbe_monitor_test_case_t*) context_p;

    /*mut_printf(MUT_LOG_TEST_STATUS, "=====Running Captain Planet Test on Active SP=====\n");
    captain_planet_reboot_passive_SP_during_eval_rb_logging(rg_config_p, test_cases_p);

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Captain Planet Test on Passive SP=====\n");
    captain_planet_reboot_active_SP_during_eval_rb_logging(rg_config_p, test_cases_p);*/

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Captain Planet Test with Dual SP hard reboot=====\n");
    captain_planet_reboot_both_SPs_during_eval_rb_logging(rg_config_p, test_cases_p);

    return;
}
/******************************************
 * end darkwing_duck_run_test_config()
 ******************************************/

/*!**************************************************************
 * captain_planet_reboot_passive_SP_during_eval_rb_logging()
 ****************************************************************
 * @brief
 *  Reboot the passive SP during various parts of eval rebuild
 *  logging.
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_cases_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void captain_planet_reboot_passive_SP_during_eval_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_monitor_test_case_t *test_cases_p) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;
    fbe_u32_t i = 0;

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* setup hooks on active SP */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove>1) {
            test_cases_p[index].num_to_remove++;
        }
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
        /* After we check for rebuild logging we should be hitting the eval rebuild hook? */
        captain_planet_wait_for_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
        captain_planet_wait_for_local_state(&rg_config_p[index], &test_cases_p[index]);

        if (test_cases_p[index].num_to_remove > 1) {
            sep_rebuild_utils_number_physical_objects_g -= 1;
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
        }
    }

    /* Reboot the other SP */
    darkwing_duck_shutdown_peer_sp();
    
    /* remove hooks on target SP */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Remove hook for test case: %s", test_cases_p[index].test_case_name);
        captain_planet_remove_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
    }

    /* The surviving SP should have continued through the eval rb logging rotart and marked rebuild logging
     * The checkpoint should go to 0 
     */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Verify rebuild logging for test case: %s", test_cases_p[index].test_case_name);
        if (test_cases_p[index].num_to_remove == 1) {
            sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
            sep_rebuild_utils_check_for_reb_restart(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        } else if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            if (test_cases_p[index].drive_slots_to_remove[0]/2 == test_cases_p[index].drive_slots_to_remove[1]/2) {
                if (test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE ||
                    test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED) {
                    sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
                } else {
                    sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
                }
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            } else {
                sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
                sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            }
        } else {
            if (test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE ||
                test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED) {
                sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
            } else {
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
            }

            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[2]);
            } else {
                    sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            } 
        }

    }

    /* Bring the peer back up */
    darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, NULL);
    fbe_api_sleep (1000*5);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    for (index = 0; index < raid_group_count; index++) {
        if (test_cases_p[index].num_to_remove == 1) {
            /* verify the raid group get to ready state in reasonable time */
            fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                FBE_LIFECYCLE_STATE_READY, CAPTAIN_PLANET_WAIT_MSEC,
                                                FBE_PACKAGE_ID_SEP_0);
        }
    }
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Reinsert the drive, and the rebuild should start and then complete
     */
    for (index = 0; index < raid_group_count; index++) {
        for (i=0; i<test_cases_p[index].num_to_remove; i++) {
            sep_rebuild_utils_number_physical_objects_g += 1;
            mut_printf(MUT_LOG_TEST_STATUS, "Inserting slot %d for rg %d", 
                       test_cases_p[index].drive_slots_to_remove[i], rg_config_p[index].raid_group_id);
            sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[i], 
                                                        sep_rebuild_utils_number_physical_objects_g, &(test_cases_p[index].drive_info[i]));
        }
    }

    for (index = 0; index < raid_group_count; index++) {
        sep_rebuild_utils_wait_for_rb_to_start(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove > 1) {
            sep_rebuild_utils_wait_for_rb_to_start(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            test_cases_p[index].num_to_remove--;
        }
    }
}
/***************************************************************
 * end captain_planet_reboot_passive_SP_during_eval_rb_logging()
 ***************************************************************/

/*!**************************************************************
 * captain_planet_reboot_active_SP_during_eval_rb_logging()
 ****************************************************************
 * @brief
 *  Reboot the active SP during various states of eval rebuild 
 *  logging.
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
void captain_planet_reboot_active_SP_during_eval_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_monitor_test_case_t *test_cases_p) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;
    fbe_u32_t i =0; 

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* setup hooks on active SP */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove>1) {
            test_cases_p[index].num_to_remove++;
        }
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
        captain_planet_wait_for_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
        /* After we check for rebuild logging we should be hitting the eval rebuild hook? */
        captain_planet_wait_for_local_state(&rg_config_p[index], &test_cases_p[index]);
        if (test_cases_p[index].num_to_remove > 1) {
            sep_rebuild_utils_number_physical_objects_g -= 1;
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
                                                         FBE_LIFECYCLE_STATE_FAIL, CAPTAIN_PLANET_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        }
    }

    // restart the active SP
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Reboot the other SP */
    darkwing_duck_shutdown_peer_sp();

    /* The surviving SP should have continued through the eval rb logging rotart and marked rebuild logging
     * The checkpoint should go to 0 
     */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Verify rebuild logging for test case: %s", test_cases_p[index].test_case_name);
        if (test_cases_p[index].num_to_remove == 1) {
            sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
            sep_rebuild_utils_check_for_reb_restart(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        } else if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            if (test_cases_p[index].drive_slots_to_remove[0]/2 == test_cases_p[index].drive_slots_to_remove[1]/2) {
                if (test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE ||
                    test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED) {
                    sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
                } else {
                    sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
                }
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            } else {
                sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
                sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            }
        } else {
            if (test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE ||
                test_cases_p[index].substate == FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED) {
                sep_rebuild_utils_verify_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
            } else {
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
            }

            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
                sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[2]);
            } else {
                    sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            } 
        }
    }

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Bring the peer back up */
    darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, NULL);
    fbe_api_sleep (1000*5);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    for (index = 0; index < raid_group_count; index++) {
        if (test_cases_p[index].num_to_remove == 1) {
            /* verify the raid group get to ready state in reasonable time */
            fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                FBE_LIFECYCLE_STATE_READY, CAPTAIN_PLANET_WAIT_MSEC,
                                                FBE_PACKAGE_ID_SEP_0);
        }
    }
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Reinsert the drive, and the rebuild should start and then complete
     */
    for (index = 0; index < raid_group_count; index++) {
        for (i=0; i<test_cases_p[index].num_to_remove; i++) {
            sep_rebuild_utils_number_physical_objects_g += 1;
            mut_printf(MUT_LOG_TEST_STATUS, "Inserting slot %d for rg %d", 
                       test_cases_p[index].drive_slots_to_remove[i], rg_config_p[index].raid_group_id);
            sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[i], 
                                                        sep_rebuild_utils_number_physical_objects_g, &(test_cases_p[index].drive_info[i]));
        }
    }

    for (index = 0; index < raid_group_count; index++) {
        sep_rebuild_utils_wait_for_rb_to_start(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[0]);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove > 1) {
            sep_rebuild_utils_wait_for_rb_to_start(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[1]);
            test_cases_p[index].num_to_remove--;
        }
    }

}
/******************************************
 * end captain_planet_reboot_active_SP_during_eval_rb_logging()
 ******************************************/

/*!**************************************************************
 * captain_planet_reboot_both_SPs_during_eval_rb_logging()
 ****************************************************************
 * @brief
 *  Reboot the both SPs during various parts of eval rebuild
 *  logging.
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_cases_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void captain_planet_reboot_both_SPs_during_eval_rb_logging(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_monitor_test_case_t *test_cases_p) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t index = 0;
    fbe_u32_t i = 0;

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* setup hooks on active SP */
    for (index = 0; index < raid_group_count; index++) {
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove>1) {
            test_cases_p[index].num_to_remove++;
        }
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
        /* After we check for rebuild logging we should be hitting the eval rebuild hook? */
        captain_planet_wait_for_target_SP_hook(&rg_config_p[index], &test_cases_p[index]);
        captain_planet_wait_for_local_state(&rg_config_p[index], &test_cases_p[index]);

        if (test_cases_p[index].num_to_remove > 1) {
            sep_rebuild_utils_number_physical_objects_g -= 1;
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
        }
    }

    /* Reboot the both SPs */
    shredder_crash_both_sps(rg_config_p, NULL, NULL);
    for (index = 0; index < raid_group_count; index++) {
        /* verify the raid group get to ready state in reasonable time */
        fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                FBE_LIFECYCLE_STATE_READY, CAPTAIN_PLANET_WAIT_MSEC,
                                                FBE_PACKAGE_ID_SEP_0);

    }



    // Verify the checkpoints for all previously removed drives in the rg and check rl is not marked
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    for (index = 0; index < raid_group_count; index++) {
        for (i=0;i<test_cases_p[index].num_to_remove; i++) {
            sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[i]);
            sep_rebuild_utils_verify_not_rb_logging(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[i]);
        }
        if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_cases_p[index].num_to_remove > 1) {
            test_cases_p[index].num_to_remove--;
        }
    }
}
/***************************************************************
 * end captain_planet_reboot_both_SPs_during_eval_rb_logging()
 ***************************************************************/


/*!**************************************************************
 * captain_planet_wait_for_local_state()
 ****************************************************************
 * @brief
 *  wait for the local state of a raid group that is specified
 *  in test_case_p. Adjust accordingly for raid10
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_case_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void captain_planet_wait_for_local_state(fbe_test_rg_configuration_t * rg_config_p,
                                         fbe_monitor_test_case_t * test_case_p) {
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               pos = 0;
    fbe_object_id_t         rg_object_id = test_case_p->rg_object_id;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
        fbe_test_sep_util_get_ds_object_list(test_case_p->rg_object_id, &downstream_object_list);

        //  Get the index into the downstream object list for the mirror object corresponding
        //  to the incoming position
        pos = test_case_p->drive_slots_to_remove[0] / 2;
    
        //  Get the object ID of the corresponding mirror object
        rg_object_id = downstream_object_list.downstream_object_list[pos];

    } 

    status = captain_planet_wait_for_local_state_by_obj_id(rg_object_id,
                                                           test_case_p->local_state,
                                                           CAPTAIN_PLANET_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}
/******************************************
 * end captain_planet_wait_for_local_state()
 ******************************************/

/*!**************************************************************
 * captain_planet_wait_for_local_state_by_obj_id()
 ****************************************************************
 * @brief
 *  Wait for a raid group to be in a certain local state by object id
 *
 * @param object_id - the raid group object id
 *        expected_local_state - the state we are waiting for
 *        timeout_ms - the amount of time we'll wait for              
 *
 * @return fbe_status_t    
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t captain_planet_wait_for_local_state_by_obj_id(fbe_object_id_t object_id, 
                                                           fbe_raid_group_local_state_t expected_local_state,
                                                           fbe_u32_t timeout_ms)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                       current_time = 0;
    fbe_api_raid_group_get_info_t   raid_group_info_p;
    fbe_raid_group_local_state_t    current_state = FBE_RAID_GROUP_LOCAL_STATE_INVALID;

    while (current_time < timeout_ms){
        status = fbe_api_raid_group_get_info(object_id, &raid_group_info_p, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_state = raid_group_info_p.local_state;
        if (current_time % 10000 == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "Current local state:0x%llx",
		       (unsigned long long)current_state);
        }

        if ((current_state & expected_local_state) == expected_local_state) {
            mut_printf(MUT_LOG_TEST_STATUS, "local state is 0x%llx",
		       (unsigned long long)current_state);
            return status;
        }

        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "local state is 0x%llx\n",
	       (unsigned long long)current_state);

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d expected state 0x%llx != current state 0x%llx in %d ms!\n", 
                  __FUNCTION__, object_id,
		  (unsigned long long)expected_local_state,
		  (unsigned long long)current_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end captain_planet_wait_for_local_state()
 ******************************************/

/*!**************************************************************
 * captain_planet_set_target_SP_hook()
 ****************************************************************
 * @brief
 *  Set the hook on the target sp. Adjust accordingly for raid10
 *  since we want to check the rebuild parameters on the mirror
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_case_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void captain_planet_set_target_SP_hook(fbe_test_rg_configuration_t *rg_config_p, 
                                       fbe_monitor_test_case_t *test_case_p) 
{
    fbe_u32_t               index = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         raid_group_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         mirror_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    { 
        //  Get the RG's object id.
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);
        // let's set this now
        test_case_p->rg_object_id = raid_group_object_id;
        //  Get the downstream object list; this is the list of mirror objects for this R10 RG
        fbe_test_sep_util_get_ds_object_list(raid_group_object_id, &downstream_object_list);

        //  Get the index into the downstream object list for the mirror object corresponding
        //  to the incoming position
        index = test_case_p->drive_slots_to_remove[0] / 2;

        //  Get the object ID of the corresponding mirror object
        mirror_rg_object_id = downstream_object_list.downstream_object_list[index];
        
        mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook for rgid %d object %d ds_mirror_id: %d", 
                       rg_config_p->raid_group_id, raid_group_object_id, mirror_rg_object_id);
        
        status = fbe_api_scheduler_add_debug_hook(mirror_rg_object_id,
                                                  test_case_p->state,
                                                  test_case_p->substate,
                                                  NULL,
                                                  NULL,
                                                  SCHEDULER_CHECK_STATES,
                                                  SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    } else {
       darkwing_duck_set_target_SP_hook(rg_config_p, test_case_p);
    }
}
/******************************************
 * end captain_planet_set_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * captain_planet_remove_target_SP_hook()
 ****************************************************************
 * @brief
 *  Remove the hook on the target sp. Adjust accordingly for raid10
 *  since we wanted to check the rebuild parameters on the mirror
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_case_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void captain_planet_remove_target_SP_hook(fbe_test_rg_configuration_t *rg_config_p, 
                                          fbe_monitor_test_case_t *test_case_p) 
{
    fbe_u32_t               pos = 0;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         raid_group_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         mirror_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;


    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        //  Get the RG's object id.
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);
        //  Get the downstream object list; this is the list of mirror objects for this R10 RG
        fbe_test_sep_util_get_ds_object_list(raid_group_object_id, &downstream_object_list);

        //  Get the index into the downstream object list for the mirror object corresponding
        //  to the incoming position
        pos = test_case_p->drive_slots_to_remove[0] / 2;

        //  Get the object ID of the corresponding mirror object
        mirror_rg_object_id = downstream_object_list.downstream_object_list[pos];
        
        mut_printf(MUT_LOG_TEST_STATUS, "Deleteing Debug Hook for rgid %d object %d ds_mirror_id: %d", 
                   rg_config_p->raid_group_id, raid_group_object_id, mirror_rg_object_id);
        
        status = fbe_api_scheduler_del_debug_hook(mirror_rg_object_id,
                                                  test_case_p->state,
                                                  test_case_p->substate,
                                                  NULL,
                                                  NULL,
                                                  SCHEDULER_CHECK_STATES,
                                                  SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    } else {
        darkwing_duck_remove_target_SP_hook(rg_config_p, test_case_p->state, test_case_p->substate);
    }

}
/******************************************
 * end captain_planet_remove_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * captain_planet_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook on the target sp. Adjust accordingly for raid10
 *  since we wanted to check the rebuild parameters on the mirror
 *
 * @param rg_config_p - the raid group configuration to test
 *        test_case_p - list of test cases to run             
 *
 * @return None.   
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void captain_planet_wait_for_target_SP_hook(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_monitor_test_case_t * test_case_p) {
    fbe_u32_t               pos = 0;
    fbe_object_id_t         raid_group_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         mirror_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Wait for Debug Hook for rgid %d object %d downstream mirrors..", 
                       rg_config_p->raid_group_id, raid_group_object_id);
        //  Get the downstream object list; this is the list of mirror objects for this R10 RG
        fbe_test_sep_util_get_ds_object_list(raid_group_object_id, &downstream_object_list);

        //  Get the index into the downstream object list for the mirror object corresponding
        //  to the incoming position
        pos = test_case_p->drive_slots_to_remove[0] / 2;
        //  Get the object ID of the corresponding mirror object
        mirror_rg_object_id = downstream_object_list.downstream_object_list[pos];
        
        darkwing_duck_wait_for_target_SP_hook(mirror_rg_object_id, test_case_p->state, test_case_p->substate);
    } else {
        darkwing_duck_wait_for_target_SP_hook(raid_group_object_id, test_case_p->state, test_case_p->substate);
    }
}
/******************************************
 * end captain_planet_wait_for_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * captain_planet_choose_test_cases
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
void captain_planet_choose_test_cases(fbe_monitor_test_case_t * test_cases_to_run_p, fbe_u32_t * num_test_cases) {
    fbe_u32_t random_num = 0;
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;

    for (i=0; i < CAPTAIN_PLANET_TC_COUNT; i++) {
        mut_printf(MUT_LOG_TEST_STATUS, "selecting test cases %d\n", i);
        // We're aiming to select 1/3 test cases...
        random_num = fbe_random() % 3; 
        if (random_num == 0) {
            test_cases_to_run_p[j] = captain_planet_test_cases[i];
            j++;
        }
    }

	// if we failed to select any test cases, select one now
    if (j==0) {
        test_cases_to_run_p[0] = captain_planet_test_cases[fbe_random() % CAPTAIN_PLANET_TC_COUNT];
        j++;
    }

    *num_test_cases = j;
}
/******************************************
 * end captain_planet_choose_test_cases()
 ******************************************/

/*!**************************************************************
 * captain_planet_test()
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
void captain_planet_dualsp_test(void)
{
    fbe_u32_t                   testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* rtl 1 and rtl 0 are the same for now */
    if (testing_level > 0) {
        fbe_test_run_test_on_rg_config(&captain_planet_raid_group_config_random[0], 
                                       &captain_planet_test_cases[0],  
                                       captain_planet_run_test_config,
                                       CAPTAIN_PLANET_LUNS_PER_RAID_GROUP,
                                       CAPTAIN_PLANET_CHUNKS_PER_LUN);
    } else {
        fbe_test_run_test_on_rg_config(&captain_planet_raid_group_config_random[0], 
                                       &captain_planet_random_test_cases[0],
                                       captain_planet_run_test_config,
                                       CAPTAIN_PLANET_LUNS_PER_RAID_GROUP,
                                       CAPTAIN_PLANET_CHUNKS_PER_LUN);
    }

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end captain_planet_test()
 ******************************************/


/*!**************************************************************
 * captain_planet_setup()
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
void captain_planet_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t raid_group_count = 0;
    fbe_u32_t num_test_cases = CAPTAIN_PLANET_TC_COUNT;

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
        captain_planet_choose_test_cases(&captain_planet_random_test_cases[0], &num_test_cases);
    }

    darkwing_duck_create_random_config(&captain_planet_raid_group_config_random[0], num_test_cases);

    if (fbe_test_util_is_simulation()) {
        raid_group_count = fbe_test_get_rg_array_length(&captain_planet_raid_group_config_random[0]);
        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&captain_planet_raid_group_config_random[0]);

        /* Setup the physical config for the raid groups on both SPs
         */
        fbe_api_sim_transport_set_target_server(CAPTAIN_PLANET_ACTIVE_SP);
        elmo_create_physical_config_for_rg(&captain_planet_raid_group_config_random[0], raid_group_count);

        fbe_api_sim_transport_set_target_server(CAPTAIN_PLANET_PASSIVE_SP);
        elmo_create_physical_config_for_rg(&captain_planet_raid_group_config_random[0], raid_group_count);

        sep_config_load_sep_and_neit_both_sps();
        /* Set the default target server back to A
         */
        fbe_api_sim_transport_set_target_server(CAPTAIN_PLANET_ACTIVE_SP);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************
 * end captain_planet_setup()
 ******************************************/

/*!**************************************************************
 * captain_planet_cleanup()
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

void captain_planet_dualsp_cleanup(void)
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

 /*************************
 * end file captain_planet_test.c
 *************************/


