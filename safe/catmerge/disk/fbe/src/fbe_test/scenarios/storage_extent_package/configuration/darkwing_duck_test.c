/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file darkwing_duck_test.c
 ***************************************************************************
 *
 * @brief
 *  This test validates the proper progression of the peer join
 *  state machine as it is interrupted by various events.
 *
 * @version
 *  08/08/2011  Deanna Heng  - Created
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
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"
#include "fbe_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * darkwing_duck_short_desc = "test various events during peer join";
char * darkwing_duck_long_desc ="\
The Darkwing Duck Test performs various operations, such as rebuild or SP death, during peer join.\n\
\n\
Dependencies:\n\
        - Rebuilds during SP failures (zoidberg Test).\n\
        - Dual SP \n\
        - Debug Hooks for Join.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] 15 SAS drives\n\
        [PP] 15 SATA drives\n\
        [PP] 30 logical drive\n\
        [SEP] 12 provision drive\n\
        [SEP] 12 virtual drive\n\
        [SEP] x LUNs\n\
\n\
STEP 1: Bring 1 SP up (SPA).\n\
STEP 2: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 3: Insert debug hooks into various points of peer join.\n\
        - bring down active SP\n\
        - bring down passive SP\n\
        - rebuild\n\
STEP 4: Bring SPB up.\n\
STEP 5: Perform hook operation during various states of peer join.\n\
        - REQUEST\n\
        - STARTED\n\
        - DONE\n\
STEP 6: Verify that all flags are cleared and operations are completed successfully.\n\
STEP 7: Cleanup\n\
        - Destroy objects\n";

/*!*******************************************************************
 * @def DARKWING_DUCK_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define DARKWING_DUCK_RAID_TYPE_COUNT 5

/*!*******************************************************************
 * @def DARKWING_DUCK_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define DARKWING_DUCK_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def DARKWING_DUCK_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define DARKWING_DUCK_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def DARKWING_DUCK_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for join state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define DARKWING_DUCK_WAIT_MSEC 1000 * 120

/*!*******************************************************************
 * @def FBE_API_POLLING_INTERVAL
 *********************************************************************
 * @brief Number of milliseconds we should wait to poll the SP
 *
 *********************************************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def DARKWING_DUCK_TC_COUNT
 *********************************************************************
 * @brief The number of total test cases for darkwing duck
 *
 *********************************************************************/
#define DARKWING_DUCK_TC_COUNT 15

/*!*******************************************************************
 * @def DARKWING_DUCK_DIVISOR
 *********************************************************************
 * @brief The number to use as a divisor when choosing test cases
 *
 *********************************************************************/
#define DARKWING_DUCK_DIVISOR 3

/*!*******************************************************************
 * @def DARKWING_DUCK_ACTIVE_SP
 *********************************************************************
 * @brief The active SP 
 *
 *********************************************************************/
#define DARKWING_DUCK_ACTIVE_SP FBE_SIM_SP_A

/*!*******************************************************************
 * @def DARKWING_DUCK_PASSIVE_SP
 *********************************************************************
 * @brief The passive SP 
 *
 *********************************************************************/
#define DARKWING_DUCK_PASSIVE_SP FBE_SIM_SP_B

/*!*******************************************************************
 * @def DARKWING_DUCK_REBOOT_FLAG
 *********************************************************************
 * @brief Number of milliseconds we should wait to poll the SP
 *
 *********************************************************************/
typedef enum darkwing_duck_reboot_flag_e
{
    DARKWING_DUCK_REBOOT = 0,
    DARKWING_DUCK_STARTUP_ONLY,
    DARKWING_DUCK_SHUTDOWN_ONLY
} darkwing_duck_reboot_flag_t;

/*!*******************************************************************
 * @def sep_rebuild_utils_number_physical_objects_g
 *********************************************************************
 * @brief number of physical objects
 *
 *********************************************************************/
extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

/*!*************************************************
 * @struct darkwing_duck_test_case_s
 ***************************************************
 * @brief darkwing duck test case structure
 *
 ***************************************************/
typedef fbe_monitor_test_case_t darkwing_duck_test_case_t;

/**************************************
 * end darkwing_duck_test_case_s
 **************************************/


/*!*******************************************************************
 * @var darkwing_duck_test_cases
 *********************************************************************
 * @brief This is the set of test cases to be run
 *
 *********************************************************************/
darkwing_duck_test_case_t darkwing_duck_test_cases[DARKWING_DUCK_TC_COUNT] = 
{
    {"Rebuild during join REQUEST no reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Rebuild during join STARTED no reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Rebuild during join DONE no reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_DONE,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Broken during join REQUEST no reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Broken during join STARTED no reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Broken during join DONE no reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_DONE,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_FALSE,
     {NULL, NULL, NULL},
    },
    {"Peer SP reboot during join REQUEST",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     0,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Peer SP reboot during join STARTED",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     0,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Peer SP reboot during join DONE",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_DONE,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     0,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Rebuild during join REQUEST with peer SP reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Rebuild during join STARTED with peer SP reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Rebuild during join DONE with peer SP reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_DONE,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     1,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Broken during join REQUEST with peer SP reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Broken during join STARTED with peer SP reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    },
    {"Broken during join DONE with peer SP reboot",
     FBE_OBJECT_ID_INVALID, 
     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
     FBE_RAID_GROUP_SUBSTATE_JOIN_DONE,
     FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED,
     FBE_RAID_GROUP_LOCAL_STATE_INVALID,
     2,
     {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
     FBE_TRUE,
     {NULL, NULL, NULL},
    }
};
/**************************************
 * end darkwing_duck_test_cases()
 **************************************/

/*!*******************************************************************
 * @var darkwing_duck_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t darkwing_duck_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t darkwing_duck_raid_group_config_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/        
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,     520,            2,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 3 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 6 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            2,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 10 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end darkwing_duck_raid_group_config_qual()
 **************************************/

/*!*******************************************************************
 * @var darkwing_duck_raid_group_config_random
 *********************************************************************
 * @brief This is the array of random configurations made from above configs
 *
 *********************************************************************/

fbe_test_rg_configuration_t darkwing_duck_raid_group_config_random[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];

/*!*******************************************************************
 * @var darkwing_duck_test_cases_to_run
 *********************************************************************
 * @brief This is the array of test cases from the lot of darkwing
 *        duck test cases.
 *
 *********************************************************************/

darkwing_duck_test_case_t darkwing_duck_random_test_cases[DARKWING_DUCK_TC_COUNT];

/*!*******************************************************************
 * @var darkwing_duck_num_test_cases_to_run
 *********************************************************************
 * @brief This is the array of test cases from the lot of darkwing
 *        duck test cases.
 *
 *********************************************************************/

fbe_u32_t darkwing_duck_num_test_cases_to_run = DARKWING_DUCK_TC_COUNT;

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:

void darkwing_duck_choose_test_cases(darkwing_duck_test_case_t * test_cases_to_run_p);

void darkwing_duck_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void darkwing_duck_run_join_tests_in_parallel(fbe_test_rg_configuration_t *rg_config_p, 
                                              darkwing_duck_test_case_t *test_case_p,
                                              fbe_sim_transport_connection_target_t target_sp);

void darkwing_duck_start_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                  darkwing_duck_test_case_t *test_cases_p);
void darkwing_duck_remove_hooks_wait_flags_cleared(fbe_test_rg_configuration_t *rg_config_p,
                                                   darkwing_duck_test_case_t *test_cases_p);
void darkwing_duck_check_rb_join_complete(fbe_test_rg_configuration_t *rg_config_p,
                                          darkwing_duck_test_case_t *test_cases_p, 
                                          fbe_sim_transport_connection_target_t target_sp);

fbe_bool_t darkwing_duck_check_rebuild_hook(fbe_sim_transport_connection_target_t target_sp,
                                            darkwing_duck_test_case_t * test_case_p); 
fbe_status_t darkwing_duck_check_rebuild_hook_counter(fbe_object_id_t rg_object_id); 
fbe_status_t darkwing_duck_remove_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p); 
void darkwing_duck_rebuild_drives(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t num_to_remove, fbe_u32_t *drive_slots_to_be_removed); 
fbe_status_t darkwing_duck_wait_for_join_state(fbe_object_id_t object_id, 
                                               fbe_base_config_clustered_flags_t expected_clustered_state,
                                               fbe_u32_t timeout_ms);
fbe_status_t darkwing_duck_check_rg_marked_joined(fbe_object_id_t rg_object_id, fbe_bool_t check_both);
fbe_status_t darkwing_duck_wait_for_peer_join_cleared(fbe_object_id_t object_id, fbe_u32_t timeout_ms);
fbe_status_t darkwing_duck_wait_for_join_local_cleared(fbe_object_id_t object_id, 
                                                       fbe_u32_t timeout_ms);

void darkwing_duck_setup_active_sp_hooks(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_monitor_test_case_t *test_cases_p); 
void darkwing_duck_wait_for_all_rg_hooks(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_monitor_test_case_t *test_cases_p);

void darkwing_duck_reboot_peer_sp(fbe_test_rg_configuration_t * rg_config_p, 
                                  fbe_monitor_test_case_t * test_cases_p);
fbe_status_t darkwing_duck_restart_peer_SP(darkwing_duck_reboot_flag_t reboot_flag); 

/* The below functions aren't currently used, but they are there in case
 * we would want to run the test cases serially. This takes a really long
 * time to do on both SPs due to the "restarting" of the SPs
 */
void darkwing_duck_run_tests_serially(fbe_test_rg_configuration_t *rg_config_p, darkwing_duck_test_case_t *test_case_p);
void darkwing_duck_run_test_case(fbe_test_rg_configuration_t *rg_config_p, darkwing_duck_test_case_t *test_case_p);


/*!**************************************************************
 * darkwing_duck_run_test_config()
 ****************************************************************
 * @brief
 *  Run peer join tests for each raid group in the config
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/

void darkwing_duck_run_test_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    darkwing_duck_test_case_t   *test_cases_p = NULL;

    test_cases_p = (darkwing_duck_test_case_t*) context_p;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Darkwing Duck Test on Active SP=====\n");

    darkwing_duck_run_join_tests_in_parallel(rg_config_p, test_cases_p, DARKWING_DUCK_ACTIVE_SP);

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Darkwing Duck Test on Passive SP=====\n");

    darkwing_duck_run_join_tests_in_parallel(rg_config_p, test_cases_p, DARKWING_DUCK_PASSIVE_SP);


    return;
}
/******************************************
 * end darkwing_duck_run_test_config()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_run_join_tests_in_parallel()
 ****************************************************************
 * @brief
 *  Run a peer join tests in parallel
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run
 *        target_sp - the SP we are going to insert the hooks 
 *                    on. (passive or active)
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_run_join_tests_in_parallel(fbe_test_rg_configuration_t *rg_config_p, 
                                              darkwing_duck_test_case_t *test_cases_p,
                                              fbe_sim_transport_connection_target_t target_sp) {

    fbe_status_t status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_INVALID_SERVER;
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* STEP 1: Set up the hooks on the target SP (SP we are going to insert
     * the hooks on, either passive or active. 
     */
    if (target_sp == DARKWING_DUCK_ACTIVE_SP) {
        darkwing_duck_setup_active_sp_hooks(rg_config_p, test_cases_p);
    } else {
        darkwing_duck_setup_passive_sp_hooks(rg_config_p, test_cases_p);
    }

    /* Sanity check - let's make sure we're running the test against the right SP 
     */
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* STEP 2: Wait for the hook to be hit and make sure the target SP
     * is in the correct state.
     */
    darkwing_duck_wait_for_all_rg_hooks(rg_config_p, test_cases_p);

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* STEP 3: Initiate the rebuilds for applicable test cases. For test
     * cases with no reboot, remove the hooks and check join is complete 
     */
    darkwing_duck_start_rebuilds(rg_config_p, test_cases_p);

    /* STEP 4: Shut down the peer SP, remove the hook, check the peer join flag is cleared, and
     * check the local flag is cleared
     */
    fbe_api_sleep (1000);
    darkwing_duck_shutdown_peer_sp();
    darkwing_duck_remove_hooks_wait_flags_cleared(rg_config_p, test_cases_p);
    
    /* STEP 5: Start up the peer SP. Check that we are joined. Check that we did the rebuild and 
     * that the rebuild is done. 
     */
    darkwing_duck_startup_peer_sp(rg_config_p, NULL, NULL);
    darkwing_duck_check_rb_join_complete(rg_config_p, test_cases_p, target_sp);
    
    return;
}
/******************************************
 * end darkwing_duck_run_join_tests_in_parallel()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_setup_passive_sp_hooks()
 ****************************************************************
 * @brief
 *  Set the test cases hook on each rg on the passive SP
 *  Start sep and neit with sep params in order to hit the hooks.
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run      
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_setup_passive_sp_hooks(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_monitor_test_case_t *test_cases_p) 
{
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_sep_package_load_params_t sep_params_p;
    fbe_u32_t       i = 0;
    fbe_u32_t       index = 0;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    /* Get the local SP ID */
    status = fbe_api_sim_transport_set_target_server(DARKWING_DUCK_PASSIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "=== Setup Hooks on PASSIVE/TARGET SP (%s) ===", DARKWING_DUCK_PASSIVE_SP == FBE_SIM_SP_A ? "SP A" : "SP B");

    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));

    /* set up the debug hooks for the passive SP */
    for (i =0; i < raid_group_count; i++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[i]))
        {
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p[i].raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s rg %d rgobjectid: %d", 
                   test_cases_p[i].test_case_name, rg_config_p[i].raid_group_id, rg_object_id);
        /* let's set the test case object id now */
        test_cases_p[i].rg_object_id = rg_object_id;
        sep_params_p.scheduler_hooks[index].object_id = rg_object_id;
        sep_params_p.scheduler_hooks[index].monitor_state = test_cases_p[i].state;
        sep_params_p.scheduler_hooks[index].monitor_substate = test_cases_p[i].substate;
        sep_params_p.scheduler_hooks[index].check_type = SCHEDULER_CHECK_STATES;
        sep_params_p.scheduler_hooks[index].action = SCHEDULER_DEBUG_ACTION_PAUSE;
        sep_params_p.scheduler_hooks[index].val1 = NULL;
        sep_params_p.scheduler_hooks[index].val2 = NULL;
        index++;

        /* let's insert the hook on the passive SP for rebuilds here
         * Do not add the hook for other test cases where 
         * there is no rebuild or the rg goes broken since no rebuild occurs
         * We are neglecting test cases where there is no peer reboot since the Active
         * side might have already done the rebuild before the reboot.
         * Also the Passive side only takes over the rebuild if it hasn't 
         * occurred yet. The Active side will have already done the rebuild if
         * we are holding the passive side in the join request state
         */
        if (darkwing_duck_check_rebuild_hook(DARKWING_DUCK_PASSIVE_SP, &test_cases_p[i])) {
            mut_printf(MUT_LOG_TEST_STATUS, "Insert rebuild hook rg %d rgobjectid: %d", rg_config_p[i].raid_group_id, rg_object_id);
            sep_params_p.scheduler_hooks[index].object_id = rg_object_id;
            sep_params_p.scheduler_hooks[index].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
            sep_params_p.scheduler_hooks[index].monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
            sep_params_p.scheduler_hooks[index].check_type = SCHEDULER_CHECK_VALS_LT;
            sep_params_p.scheduler_hooks[index].action = SCHEDULER_DEBUG_ACTION_LOG;
            sep_params_p.scheduler_hooks[index].val1 = 0;
            sep_params_p.scheduler_hooks[index].val2 = NULL;
            index++;
        }
    }
    /* and this will be our signal to stop */
    sep_params_p.scheduler_hooks[index].object_id = FBE_OBJECT_ID_INVALID;

    /* Let's make sure we're shutting down the passive SP */
    status = fbe_api_sim_transport_set_target_server(DARKWING_DUCK_ACTIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    darkwing_duck_restart_peer_SP(DARKWING_DUCK_SHUTDOWN_ONLY);

    // let's wait awhile before bringing sp b back up
    fbe_api_sleep(2000);

    /* load sep and neit with startup params to hit the hooks */
    status = fbe_api_sim_transport_set_target_server(DARKWING_DUCK_PASSIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);

    for (index = 0; index < raid_group_count; index++) {
        if (test_cases_p[index].num_to_remove == 1) {
            /* verify the raid group get to activate state in reasonable time */
            fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                    FBE_LIFECYCLE_STATE_ACTIVATE, DARKWING_DUCK_WAIT_MSEC,
                                                    FBE_PACKAGE_ID_SEP_0);
        }
    }
}
/******************************************
 * end darkwing_duck_setup_passive_sp_hooks()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_setup_active_sp_hooks()
 ****************************************************************
 * @brief
 *  For all raid groups, insert the hooks on the active SP side
 *  then reboot the peer SP to hit the hooks. 
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run      
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_setup_active_sp_hooks(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_monitor_test_case_t *test_cases_p) 
{
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       index = 0;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    /* Get the local SP ID */
    status = fbe_api_sim_transport_set_target_server(DARKWING_DUCK_ACTIVE_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, "=== Setup Hooks on ACTIVE/TARGET SP (%s) ===", DARKWING_DUCK_ACTIVE_SP == FBE_SIM_SP_A ? "SP A" : "SP B");

    for (index = 0; index < raid_group_count; index++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[index]))
        {
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p[index].raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "Insert hook for test case: %s", test_cases_p[index].test_case_name);
        darkwing_duck_set_target_SP_hook(&rg_config_p[index], 
                                         &test_cases_p[index]);

        /* Let's insert the rebuild hooks now. Do not add the hook for other test cases where 
         * there is no rebuild or the rg goes broken since no rebuild occurs
         */
        if (darkwing_duck_check_rebuild_hook(DARKWING_DUCK_ACTIVE_SP, &test_cases_p[index])) {
            status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      0,
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_LOG);
        }

    }

    /* restart the passive SP so we can hit the hook
     */
    darkwing_duck_reboot_peer_sp(rg_config_p, NULL);
}
/******************************************
 * end darkwing_duck_setup_active_sp_hooks()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_wait_for_all_rg_hooks()
 ****************************************************************
 * @brief
 *  Wait for the test case hook to be hit for each raid group.
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run      
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_wait_for_all_rg_hooks(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_monitor_test_case_t *test_cases_p) {
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       i = 0;
    fbe_status_t    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== Wait for all Hooks Hit ===");

    for (i=0; i<raid_group_count; i++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[i]))
        {
            continue;
        }

        status =  darkwing_duck_wait_for_target_SP_hook(test_cases_p[i].rg_object_id, test_cases_p[i].state, test_cases_p[i].substate);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "Wait for join state rgid: %d objid: %d clustered state: 0x%llx", 
                   rg_config_p[i].raid_group_id, test_cases_p[i].rg_object_id,
		   (unsigned long long)test_cases_p[i].clustered_flag);
        status = darkwing_duck_wait_for_join_state(test_cases_p[i].rg_object_id,  
                                                   test_cases_p[i].clustered_flag, 
                                                   DARKWING_DUCK_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/******************************************
 * end darkwing_duck_wait_for_all_rg_hooks()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_start_rebuilds()
 ****************************************************************
 * @brief
 *  For all applicable raid groups, start the rebuild(s)
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run      
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_start_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                  darkwing_duck_test_case_t *test_cases_p) {

    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       index = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Run rebuild for applicable test cases ===");
    for (index = 0; index < raid_group_count; index++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[index]))
        {
            continue;
        }

        if (test_cases_p[index].num_to_remove > 0) {
            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
                test_cases_p[index].num_to_remove++;
            }
            darkwing_duck_rebuild_drives(&rg_config_p[index], 
                                         test_cases_p[index].num_to_remove,
                                         test_cases_p[index].drive_slots_to_remove);
        }

        /* Remove the hook for applicable test cases
         * and check that the flags are cleared
         */
        if (!test_cases_p[index].peer_reboot) {
            mut_printf(MUT_LOG_TEST_STATUS, "No Shutdown. Remove the hook for rgid: %d objid: %d ", 
                       rg_config_p[index].raid_group_id, test_cases_p[index].rg_object_id);
            darkwing_duck_remove_target_SP_hook(&rg_config_p[index], 
                                                test_cases_p[index].state, 
                                                test_cases_p[index].substate);
            darkwing_duck_check_rg_marked_joined(test_cases_p[index].rg_object_id, FBE_TRUE);
        }

    }
}
/******************************************
 * end darkwing_duck_start_rebuilds()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_wait_flags_cleared()
 ****************************************************************
 * @brief
 *  For all raid groups, remove the hooks and check 
 *  that the flags are cleared
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run      
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_remove_hooks_wait_flags_cleared(fbe_test_rg_configuration_t *rg_config_p,
                                                   darkwing_duck_test_case_t *test_cases_p) {

    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       index = 0;
    fbe_status_t    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Remove the hook and wait for the flags to be cleared ===\n");
    for (index = 0; index < raid_group_count; index++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[index]))
        {
            continue;
        }
        if (test_cases_p[index].peer_reboot) {
            darkwing_duck_remove_target_SP_hook(&rg_config_p[index], 
                                                test_cases_p[index].state, 
                                                test_cases_p[index].substate);
        }

        /* verify the raid group get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, DARKWING_DUCK_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = darkwing_duck_wait_for_peer_join_cleared(test_cases_p[index].rg_object_id, DARKWING_DUCK_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = darkwing_duck_wait_for_join_local_cleared(test_cases_p[index].rg_object_id,  
                                                   DARKWING_DUCK_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/******************************************
 * end darkwing_duck_remove_hooks_wait_flags_cleared()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_check_rb_join_complete()
 ****************************************************************
 * @brief
 *  For all raid groups, check that the rebuilds are complete if
 *  applicable. Then check that we have successfully joined
 *
 * @param rg_config_p - pointer to the rg configs being tested
 *        test_cases_p - pointer to the test cases we will run
 *        target_sp - the SP we are going to insert the hooks 
 *                    on. (passive or active)           
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_check_rb_join_complete(fbe_test_rg_configuration_t *rg_config_p,
                                          darkwing_duck_test_case_t *test_cases_p, 
                                          fbe_sim_transport_connection_target_t target_sp) {
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       index = 0;
    fbe_u32_t       j = 0;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t                     raid_group_id;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_INVALID_SERVER;
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Check that all Rebuilds and JOINs are complete ===");

    for (index = 0; index < raid_group_count; index++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[index]))
        {
            continue;
        }

        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* verify the raid group get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(test_cases_p[index].rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, DARKWING_DUCK_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);

        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        darkwing_duck_check_rg_marked_joined(test_cases_p[index].rg_object_id, FBE_TRUE);

        if (test_cases_p[index].num_to_remove > 0) {
        
            mut_printf(MUT_LOG_TEST_STATUS, "Wait for the rebuild(s) to be finished for rg id %d.\n", test_cases_p[index].rg_object_id);
            for(j=0;j<test_cases_p[index].num_to_remove;j++)
        	{
        		//  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding.
        		sep_rebuild_utils_wait_for_rb_comp(&rg_config_p[index], test_cases_p[index].drive_slots_to_remove[j]);
        	}
            if (rg_config_p[index].raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
                test_cases_p[index].num_to_remove--;
            }

            sep_rebuild_utils_check_bits(raid_group_id);

            // check bits changes the target sp to SPA -- let's change it back
            status = fbe_api_sim_transport_set_target_server(target_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            /* We want to check that the rebuild finished
             * No rebuilds occur when the rg goes broken.
             */
            if (darkwing_duck_check_rebuild_hook(target_sp, &test_cases_p[index])) {
                /* TODO: reenable check for rebuild hook
                 * There are some issues with checking the rebuild hook counter
                 */
                //darkwing_duck_check_rebuild_hook_counter(test_cases_p[index].rg_object_id);
                darkwing_duck_remove_rebuild_hook(&rg_config_p[index]);
            }
        }
    }
}
/******************************************
 * end darkwing_duck_check_rb_join_complete()
 ******************************************/


/*!**************************************************************
 * darkwing_duck_wait_for_join_state()
 ****************************************************************
 * @brief
 *  Wait for join to be in a certain state
 *
 * @param object_id - the raid group object id
 *        expected_clustered_state - the state we are waiting for
 *        timeout_ms - the amount of time we'll wait for              
 *
 * @return fbe_status_t    
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_wait_for_join_state(fbe_object_id_t object_id, 
                                               //fbe_raid_group_local_state_t expected_local_state,
                                               fbe_base_config_clustered_flags_t expected_clustered_state,
                                               fbe_u32_t timeout_ms)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                       current_time = 0;
    fbe_api_raid_group_get_info_t   raid_group_info_p;
    fbe_raid_group_local_state_t    current_state = FBE_RAID_GROUP_LOCAL_STATE_INVALID;
    fbe_base_config_clustered_flags_t clustered_flag = FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP;


    while (current_time < timeout_ms){
        status = fbe_api_raid_group_get_info(object_id, &raid_group_info_p, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_state = raid_group_info_p.local_state;
        clustered_flag = raid_group_info_p.base_config_clustered_flags;
        if (current_time % 10000 == 0) {
            mut_printf(MUT_LOG_TEST_STATUS,
		       "Current states: clustered flag: 0x%llx local state:0x%llx",
		       (unsigned long long)clustered_flag,
		       (unsigned long long)current_state);
        }

        if ((raid_group_info_p.base_config_clustered_flags & expected_clustered_state) == expected_clustered_state) {
        
            mut_printf(MUT_LOG_TEST_STATUS,
		       "clustered flag is 0x%llx. local state is 0x%llx",
		       (unsigned long long)clustered_flag,
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
		  (unsigned long long)expected_clustered_state,
		  (unsigned long long)clustered_flag, timeout_ms);

    //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end darkwing_duck_wait_for_join_state()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_wait_for_join_local_cleared()
 ****************************************************************
 * @brief
 *  Wait for join to be in a certain state
 *
 * @param object_id - the object id to use to check for the local state 
 *        timeout_ms - the time to wait before throwing an error              
 *
 * @return fbe_status_t    
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_wait_for_join_local_cleared(fbe_object_id_t object_id, 
                                                       fbe_u32_t timeout_ms)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                       current_time = 0;
    fbe_api_raid_group_get_info_t   raid_group_info_p;
    fbe_raid_group_local_state_t    current_state = FBE_RAID_GROUP_LOCAL_STATE_INVALID;

    /* TODO: This can probably be combined with wait for peer join cleared
     */
    while (current_time < timeout_ms) {
        status = fbe_api_raid_group_get_info(object_id, &raid_group_info_p, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_state = raid_group_info_p.local_state;
        if (current_time % 10000 == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "local state is 0x%llx",
		       (unsigned long long)current_state);
        }
        /* Let's make sure the peer join clustered flag is cleared */
        if ((raid_group_info_p.base_config_peer_clustered_flags | ~FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK) == ~FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK) 
        {
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
                  "%s: object %d expected state 0x%x != current state 0x%llx in %d ms!\n", 
                  __FUNCTION__, object_id, 0,
		  (unsigned long long)current_state, timeout_ms);


    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end darkwing_duck_wait_for_join_local_cleared()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_wait_for_join_cleared()
 ****************************************************************
 * @brief
 *  Wait for join to be in a certain state
 *
 * @param None.               
 *
 * @return fbe_status_t    
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_wait_for_peer_join_cleared(fbe_object_id_t object_id, 
                                               fbe_u32_t timeout_ms)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                       current_time = 0;
    fbe_api_raid_group_get_info_t   raid_group_info_p;
    fbe_raid_group_local_state_t    current_state = FBE_RAID_GROUP_LOCAL_STATE_INVALID;
    fbe_base_config_clustered_flags_t clustered_flag = FBE_BASE_CONFIG_METADATA_MEMORY_INVALID_BITMAP;

    while (current_time < timeout_ms) {
        status = fbe_api_raid_group_get_info(object_id, &raid_group_info_p, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_state = raid_group_info_p.local_state;
        clustered_flag = raid_group_info_p.base_config_peer_clustered_flags;
        if (current_time % 10000 == 0) {
            mut_printf(MUT_LOG_TEST_STATUS,
		       "clustered flag for peer is 0x%llx local state is 0x%llx",
		       (unsigned long long)clustered_flag,
		       (unsigned long long)current_state);
        }
        /* Let's make sure the peer join clustered flag is cleared */
        if ((raid_group_info_p.base_config_peer_clustered_flags | ~FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK) == ~FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK) 
        {
            mut_printf(MUT_LOG_TEST_STATUS,
		       "clustered flag for peer is 0x%llx. local state is 0x%llx",
		       (unsigned long long)clustered_flag,
		       (unsigned long long)current_state);
            /* We should be active now */
            MUT_ASSERT_INT_EQUAL(raid_group_info_p.metadata_element_state, FBE_METADATA_ELEMENT_STATE_ACTIVE); 
            return status;
        }

        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "local state is 0x%llx\n",
	       (unsigned long long)current_state);

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d expected state 0x%x != peer current state 0x%llx in %d ms!\n", 
                  __FUNCTION__, object_id, 0,
		 (unsigned long long)clustered_flag, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end darkwing_duck_wait_for_join_cleared()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_check_rg_marked_joined()
 ****************************************************************
 * @brief
 *  Restart the passive SP
 *
 * @param rg_object_id - the object id of the raid group
 *        check_both - flag to check both SPs.              
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_check_rg_marked_joined(fbe_object_id_t rg_object_id, fbe_bool_t check_both)
{
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "Check that the join is complete for rgid %d on %s.", 
               rg_object_id, target_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, DARKWING_DUCK_WAIT_MSEC,
                                                     FBE_PACKAGE_ID_SEP_0);
    /* check that the join flag is cleared */
    status = darkwing_duck_wait_for_join_state(rg_object_id, 
                                               //FBE_RAID_GROUP_LOCAL_STATE_JOINED, 
                                               FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED,
                                               DARKWING_DUCK_WAIT_MSEC);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // check the state of the rg on the peer sp as well 
    if (check_both) {
    
        /* set the peer SP */
        peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);
    
        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "Check that the join is complete for rgid %d on %s.", rg_object_id, peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* verify the raid group get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, DARKWING_DUCK_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
    
        /* check that the join flag is cleared */
        status = darkwing_duck_wait_for_join_state(rg_object_id, 
                                                   //FBE_RAID_GROUP_LOCAL_STATE_JOINED, 
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED,
                                                   DARKWING_DUCK_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/******************************************
 * end darkwing_duck_check_rg_marked_joined()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_set_target_SP_hook()
 ****************************************************************
 * @brief
 *  Add the hook to the active SP
 *
 * @param rg_config_p - the raid config to work on
 *        state - the monitor state
 *        substate - the monitor substate             
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_set_target_SP_hook(fbe_test_rg_configuration_t* rg_config_p, 
                                              fbe_monitor_test_case_t* test_case_p) 
{
    fbe_object_id_t         raid_group_object_id;
    fbe_status_t            status = FBE_STATUS_OK;

    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);
    // let's set this now
    test_case_p->rg_object_id = raid_group_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook for rgid %d object %d", rg_config_p->raid_group_id, raid_group_object_id);

	status = fbe_api_scheduler_add_debug_hook(raid_group_object_id,
	    		test_case_p->state,
	    		test_case_p->substate,
	    		NULL,
	    		NULL,
	    		SCHEDULER_CHECK_STATES,
	    		SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/******************************************
 * end darkwing_duck_set_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = DARKWING_DUCK_WAIT_MSEC;
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
	hook.val2 = NULL;
	hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rgobjid: %d state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook");
            return status;
        }
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state %d substate %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end darkwing_duck_wait_for_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_remove_target_SP_hook()
 ****************************************************************
 * @brief
 *  Remove the hook to the active SP
 *
 * @param              
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_remove_target_SP_hook(fbe_test_rg_configuration_t* rg_config_p,
                                                 fbe_u32_t state, 
                                                 fbe_u32_t substate) 
{
    fbe_object_id_t                     raid_group_object_id;
    fbe_status_t                        status = FBE_STATUS_OK;
    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook. rg: %d objid: %d state: %d substate %d", 
               rg_config_p->raid_group_id, raid_group_object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook(raid_group_object_id,
                                              state,
                                              substate,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/******************************************
 * end darkwing_duck_remove_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_check_rebuild_hook_()
 ****************************************************************
 * @brief
 *  Check whether or not to check the rebuild hook
 *
 * @param target_sp 
 *        test_case_p         
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  9/21/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_bool_t darkwing_duck_check_rebuild_hook(fbe_sim_transport_connection_target_t target_sp,
                                            darkwing_duck_test_case_t * test_case_p) {
    
    if (target_sp == DARKWING_DUCK_ACTIVE_SP) {
        return (test_case_p->num_to_remove == 1);
    } else {
        return (test_case_p->num_to_remove == 1 && test_case_p->peer_reboot &&
                (test_case_p->clustered_flag == FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING ||
                 test_case_p->clustered_flag == FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED));
    }

}
/******************************************
 * end darkwing_duck_check_rebuild_hook()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_check_rebuild_hook_counter()
 ****************************************************************
 * @brief
 *  Verify that the rebuild hook was hit
 *
 * @param rg_object_id - the object id of the raid group             
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_check_rebuild_hook_counter(fbe_object_id_t rg_object_id) 
{
    fbe_scheduler_debug_hook_t hook;
    fbe_status_t            status = FBE_STATUS_OK;
    
    hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
    hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_VALS_LT;
    hook.action = SCHEDULER_DEBUG_ACTION_LOG;
    hook.val1 = 0;
	hook.val2 = NULL;
	hook.counter = 0;

	status = fbe_api_scheduler_get_debug_hook(&hook);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS,
	       "Rebuild Hook counter for object id %d = %llu",
	       rg_object_id, (unsigned long long)hook.counter);
    MUT_ASSERT_UINT64_NOT_EQUAL(0, hook.counter);

    return FBE_STATUS_OK;
}
/******************************************
 * end darkwing_duck_check_rebuild_hook_counter()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_remove_rebuild_hook()
 ****************************************************************
 * @brief
 *  Remove the hook to the target SP
 *
 * @param rg_config_p - the raid group config to remove the hook from            
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_remove_rebuild_hook(fbe_test_rg_configuration_t* rg_config_p) 
{
    fbe_object_id_t                     raid_group_object_id;
    fbe_status_t                        status = FBE_STATUS_OK;
    //  Get the RG's object id.
	fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id);

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Rebuild Debug Hook. rg: %d objid: %d", 
               rg_config_p->raid_group_id, raid_group_object_id);

    status = fbe_api_scheduler_del_debug_hook(raid_group_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              0,
                                              NULL,
                                              SCHEDULER_CHECK_VALS_LT,
                                              SCHEDULER_DEBUG_ACTION_LOG);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/******************************************
 * end darkwing_duck_remove_rebuild_hook()
 ******************************************/


/*!**************************************************************
 * darkwing_duck_rebuild_drives()
 ****************************************************************
 * @brief
 *  Remove specified drive slots in a raid group
 *
 * @param rg_config_p - raid group to remove drives
 *        num_to_remove - number of drives to remove
 *        drive_slots_to_be_removes               
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_rebuild_drives(fbe_test_rg_configuration_t *rg_config_p, 
                                  fbe_u32_t num_to_remove,
                                  fbe_u32_t *drive_slots_to_be_removed) 
{
    fbe_u32_t 			                    i = 0;
    fbe_api_terminator_device_handle_t      drive_info[3];       //  drive info needed for reinsertion


    mut_printf(MUT_LOG_TEST_STATUS, "Initiate the rebuild for the %d drive(s) for rgid: %d\n", num_to_remove, rg_config_p->raid_group_id);
    sep_rebuild_utils_get_random_drives(rg_config_p, num_to_remove, drive_slots_to_be_removed);
    for(i=0;i<num_to_remove;i++)
	{
		sep_rebuild_utils_number_physical_objects_g -= 1;

		/*  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
	        is marked. */
		sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[i], sep_rebuild_utils_number_physical_objects_g, &drive_info[i]);        

        /* TODO: Add checks for rebuild logging. This is only applicable when the rg is ready 
         * For instance the passive SP may automatically be set to joined even though it is being held
         * at the STARTED or JOINED STATE. 
         */ 
        /*status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &obj_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_get_object_lifecycle_state(obj_id, &current_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (current_state == FBE_LIFECYCLE_STATE_READY) {
            sep_rebuild_utils_verify_rb_logging(rg_config_p, drive_slots_to_be_removed[i]);
            //sep_rebuild_utils_check_for_reb_restart(rg_config_p, drive_slots_to_be_removed[i]);
        }*/
	}
    // TODO: For broken test cases, verify that rg goes to failed. 

    /* Reinsert the drives to initiate a rebuild
     */
    for(i=0;i<num_to_remove;i++)
	{
		sep_rebuild_utils_number_physical_objects_g += 1;
		// Reinsert the drive and wait for the rebuild to start
		sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[i], sep_rebuild_utils_number_physical_objects_g, &drive_info[i]);
	}
}
/******************************************
 * end darkwing_duck_rebuild_drives()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_reboot_peer_sp()
 ****************************************************************
 * @brief
 *  Do a hard reboot on the peer sp and recreate the config
 *
 * @param rg_config_p - raid group config to recreate    
 *        test_cases_p - contains the information on drived removed      
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_reboot_peer_sp(fbe_test_rg_configuration_t * rg_config_p, 
                                  fbe_monitor_test_case_t * test_cases_p) 
{
    mut_printf(MUT_LOG_LOW, " == REBOOT PEER SP == ");
    darkwing_duck_shutdown_peer_sp();
    fbe_api_sleep(2000);
    darkwing_duck_startup_peer_sp(rg_config_p, test_cases_p, NULL);
}
/******************************************
 * end darkwing_reboot_peer_sp()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_shutdown_peer_sp()
 ****************************************************************
 * @brief
 *  Do a hard reboot on the peer sp
 *
 * @param  
 *
 * @return 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_shutdown_peer_sp() {
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == SHUTDOWN PEER SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(peer_sp);
    fbe_test_sp_sim_stop_single_sp(peer_sp == FBE_SIM_SP_A? FBE_TEST_SPA: FBE_TEST_SPB);
    /* we can't destroy fbe_api, because it's shared between two SPs */
    fbe_test_base_suite_startup_single_sp(peer_sp);

    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end darkwing_shutdown_peer_sp()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_startup_peer_sp()
 ****************************************************************
 * @brief
 *  Startup the peer sp after a hard reboot and recreate the config
 *
 * @param rg_config_p - raid group config to recreate    
 *        test_cases_p - contains the information on drived removed      
 *
 * @return 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_startup_peer_sp(fbe_test_rg_configuration_t * rg_config_p, 
                                   fbe_monitor_test_case_t * test_cases_p,
                                   fbe_test_rg_config_test func_p) {
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t raid_group_count = 0;
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    if (func_p == NULL) {
        func_p = (fbe_test_rg_config_test)darkwing_duck_remove_failed_drives;
    }

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == STARTUP SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    /* Start the SP with a new session */
    fbe_test_sep_duplicate_config(rg_config_p, temp_config);
    elmo_create_physical_config_for_rg(&temp_config[0], raid_group_count);
    if (test_cases_p != NULL) {
        func_p(rg_config_p, test_cases_p);
    }
    sep_config_load_sep_and_neit();

    fbe_api_database_set_load_balance(FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}
/******************************************
 * end darkwing_startup_peer_sp()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_remove_failed_drives()
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
void darkwing_duck_remove_failed_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                        fbe_monitor_test_case_t * test_cases_p) {
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_status_t status = FBE_STATUS_OK;
    

    for (i=0; i< raid_group_count; i++) {
        for (j=0; j< test_cases_p[i].num_to_remove; j++) {
            status = fbe_test_pp_util_pull_drive(rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].bus,
                                                 rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].enclosure,
                                                 rg_config_p[i].rg_disk_set[test_cases_p[i].drive_slots_to_remove[j]].slot, 
                                                 &test_cases_p[i].drive_info[j]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
}
/******************************************
 * end darkwing_duck_remove_failed_drives()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_run_tests_serially()
 ****************************************************************
 * @brief
 *  Run peer join tests for each raid group in the config. 
 *  This function isn't being used. It can be used for rtl 2
 *  perhaps. The lot of tests take an extremely long time to execute
 *  due the unloading and loading of packages
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/

void darkwing_duck_run_tests_serially(fbe_test_rg_configuration_t *rg_config_p, darkwing_duck_test_case_t   *test_cases_p)
{
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       index = 0;
    fbe_u32_t       test_index = 0;
    //darkwing_duck_test_case_t   *test_cases_p = NULL;

    //test_cases_p = (darkwing_duck_test_case_t*) context_p;

    mut_printf(MUT_LOG_TEST_STATUS, "=====Running Darkwing Duck test=====\n");

    /* For each raid group in the config, run each test case. 
     */
    for (index = 0; index < raid_group_count; index++) {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[index])) {
            continue;
        }

        for (test_index = 0; test_index < DARKWING_DUCK_TC_COUNT; test_index++) {
            darkwing_duck_run_test_case(&rg_config_p[index], &test_cases_p[test_index]);
        }
    }

    return;
}
/******************************************
 * end darkwing_duck_run_tests_serially()
 ******************************************/


/*!**************************************************************
 * darkwing_duck_run_test_case()
 ****************************************************************
 * @brief
 *  Run a peer join test.
 *  This function isn't being used. It can be used for rtl 2
 *  perhaps. The lot of tests take an extremely long time to execute
 *  due the unloading and loading of packages. 
 *
 * @param fbe_test_rg_configuration - The rg config to run the test against   
 *        darkwing_duck_test_case_t - The test case to run            
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/

void darkwing_duck_run_test_case(fbe_test_rg_configuration_t *rg_config_p, darkwing_duck_test_case_t *test_case_p)
{
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                   num_to_remove = 0;
    fbe_u32_t                   i = 0;

    
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* let's set the rg_object_id for now
     */
    test_case_p->rg_object_id = rg_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "Darkwing Duck Test: %s RGID: %d OBJID: %d\n", 
               test_case_p->test_case_name, rg_config_p->raid_group_id, rg_object_id);


    mut_printf(MUT_LOG_TEST_STATUS, "Inserting hook state: %d substate: %d\n", 
               test_case_p->state, test_case_p->substate);
    darkwing_duck_set_target_SP_hook(rg_config_p, 
                                     test_case_p);

    /* restart the passive SP so we can hit the hook
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Reboot the passive SP\n");
    darkwing_duck_restart_peer_SP(DARKWING_DUCK_REBOOT);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook rgid: %d state: %d substate: %d.\n", 
                   test_case_p->rg_object_id, test_case_p->state, test_case_p->substate);
    status = darkwing_duck_wait_for_target_SP_hook(test_case_p->rg_object_id,
                                                   test_case_p->state, 
                                                   test_case_p->substate);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
   
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the join clustered flag 0x%llx\n",
	       (unsigned long long)test_case_p->clustered_flag);
    /* wait for the correct state in order to insert an event
     */
    status = darkwing_duck_wait_for_join_state(test_case_p->rg_object_id,  
                                               test_case_p->clustered_flag, 
                                               DARKWING_DUCK_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* book keeping to keep track of how many objects to wait for later */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If num_to_remove > 0, we are running a rebuild test case
     */
    if (test_case_p->num_to_remove > 0) {
        num_to_remove = test_case_p->num_to_remove;
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) {
            num_to_remove++;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Rebuild %d drive(s).\n", num_to_remove);
        darkwing_duck_rebuild_drives(rg_config_p, 
                                     num_to_remove,
                                     test_case_p->drive_slots_to_remove);
    }
        

    /* Remove the hook for applicable test cases
     * and check that the flags are cleared
     */
    if (!test_case_p->peer_reboot) {
        mut_printf(MUT_LOG_TEST_STATUS, "Remove the hook and wait for all rgs are joined.\n");
        darkwing_duck_remove_target_SP_hook(rg_config_p, 
                                            test_case_p->state, 
                                            test_case_p->substate);
        darkwing_duck_check_rg_marked_joined(test_case_p->rg_object_id, FBE_TRUE);
   } else {
        mut_printf(MUT_LOG_TEST_STATUS, "Shutting down passive SP.\n");
        /* shutdown the peer SP to and check that the 
         * flags on the active SP are cleared 
         */
        darkwing_duck_restart_peer_SP(DARKWING_DUCK_SHUTDOWN_ONLY);

        mut_printf(MUT_LOG_TEST_STATUS, "Remove the hook and wait for the flags to be cleared.\n");
        darkwing_duck_remove_target_SP_hook(rg_config_p, 
                                            test_case_p->state, 
                                            test_case_p->substate);
    
        /*status = darkwing_duck_wait_for_join_state(test_case_p->rg_object_id, 
                                                   0, 
                                                   DARKWING_DUCK_WAIT_MSEC);*/
        status = darkwing_duck_wait_for_peer_join_cleared(test_case_p->rg_object_id, DARKWING_DUCK_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    
        /* bring up the peer SP to and check that the 
         * flags on both SPs are cleared 
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Restart the passive SP and make sure all raid groups are joined.\n");
        darkwing_duck_restart_peer_SP(DARKWING_DUCK_STARTUP_ONLY);
        darkwing_duck_check_rg_marked_joined(test_case_p->rg_object_id, FBE_TRUE);

   }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for %d rebuild(s) to be finished for rg id %d.\n", 
               num_to_remove, test_case_p->rg_object_id);
    for(i=0;i<num_to_remove;i++) {
    	//  Wait until the rebuild finishes.  We need to pass the disk position of the disk which is rebuilding.
    	sep_rebuild_utils_wait_for_rb_comp(rg_config_p, test_case_p->drive_slots_to_remove[i]);
    }

    sep_rebuild_utils_check_bits(rg_object_id);

    return;
}
/******************************************
 * end darkwing_duck_run_test_case
 ******************************************/

/*!**************************************************************
 * darkwing_duck_restart_peer_SP()
 ****************************************************************
 * @brief
 *  Restart the passive SP
 *
 * @param reboot_flag - flag to perform desired operation             
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_restart_peer_SP(darkwing_duck_reboot_flag_t reboot_flag) 
{
    fbe_sim_transport_connection_target_t   active_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the local SP ID */
    active_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_sp);

    /* set the passive SP */
    peer_sp = (active_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* TODO: We will need to insert a more desireable way of rebooting the
     * SP. This is because some operations may still be run while we are
     * unloading packages.
     */
    // here we will perform the action on the peer SP as specified
    switch (reboot_flag) 
    {
        case DARKWING_DUCK_REBOOT:
            mut_printf(MUT_LOG_LOW, " == REBOOT PEER SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
            fbe_test_sep_util_destroy_neit_sep();
            fbe_api_sleep (1000*5);
            sep_config_load_sep_and_neit();
            fbe_api_database_set_load_balance(FBE_TRUE);
            break;
        case DARKWING_DUCK_STARTUP_ONLY:
            mut_printf(MUT_LOG_LOW, " == STARTUP SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
            sep_config_load_sep_and_neit();
            fbe_api_database_set_load_balance(FBE_TRUE);
            break;
        case DARKWING_DUCK_SHUTDOWN_ONLY:
            mut_printf(MUT_LOG_LOW, " == SHUTDOWN PEER SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
            fbe_test_sep_util_destroy_neit_sep();
            break;
    }

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************
 * end darkwing_duck_restart_peer_SP()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_create_random_config
 ****************************************************************
 * @brief
 *  Create a random configuration from the rg_config
 *
 * @param rg_config_p pointer to the configuration to configure       
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t darkwing_duck_create_random_config(fbe_test_rg_configuration_t * rg_config_p, fbe_u32_t num_rgs) 
{

    fbe_u32_t rt_index = 0; // raid type index in the initial configuration
    fbe_u32_t rg_index = 0; // raid group index in the raid type configuration
    fbe_u32_t rg_count = 0; // number of raid groups in the configuration
    fbe_u32_t index = 0;
    fbe_test_rg_configuration_t * temp_rg_config_p = NULL;

    

    mut_printf(MUT_LOG_TEST_STATUS, "Creating a random configuration\n");

    /* Using the number of test cases we are going to run (defined in setup)
     * and the number of raid types setup in the config, 
     * randomly select the raid groups. 
     */
    for (index =0; index < num_rgs; index++) {
        rt_index = fbe_random() % DARKWING_DUCK_RAID_TYPE_COUNT; 
        temp_rg_config_p = &darkwing_duck_raid_group_config_qual[rt_index][0];
        rg_count = fbe_test_get_rg_array_length(temp_rg_config_p);
        rg_index = fbe_random() % rg_count; 

        rg_config_p[index] = temp_rg_config_p[rg_index];
        mut_printf(MUT_LOG_TEST_STATUS, "We've selected to run raid type %d raid group %d rg count %d\n", rt_index, rg_index, rg_count);
        rg_config_p[index].raid_group_id = index;
    }

    rg_config_p[index] = temp_rg_config_p[rg_count];
    
    return FBE_STATUS_OK;
}
/******************************************
 * end darkwing_duck_create_random_config()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_choose_test_cases
 ****************************************************************
 * @brief
 *  Create a random configuration from the rg_config
 *
 * @param test_cases_to_run_p - pointer to the test cases we will choose        
 *
 * @return None
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_choose_test_cases(fbe_monitor_test_case_t * test_cases_to_run_p) {
    fbe_u32_t random_num = 0;
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;

    for (i=0; i < DARKWING_DUCK_TC_COUNT; i++) {
        random_num = fbe_random() % DARKWING_DUCK_DIVISOR; 
        if (random_num == 0) {
            test_cases_to_run_p[j] = darkwing_duck_test_cases[i];
            j++;
        }
    }

	// if we failed to select any test cases, select one now
    if (j==0) {
        test_cases_to_run_p[0] = darkwing_duck_test_cases[fbe_random() % DARKWING_DUCK_TC_COUNT];
        j++;
    }

    darkwing_duck_num_test_cases_to_run = j;
}
/******************************************
 * end darkwing_duck_choose_test_cases()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_test()
 ****************************************************************
 * @brief
 *  Run peer join test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/

void darkwing_duck_test(void)
{
    fbe_u32_t                   testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (testing_level > 0) {
        fbe_test_run_test_on_rg_config(&darkwing_duck_raid_group_config_random[0], 
                                       &darkwing_duck_test_cases[0], 
                                       darkwing_duck_run_test_config,
                                       DARKWING_DUCK_LUNS_PER_RAID_GROUP,
                                       DARKWING_DUCK_CHUNKS_PER_LUN);
    } else {
   
        fbe_test_run_test_on_rg_config(&darkwing_duck_raid_group_config_random[0], 
                                       &darkwing_duck_random_test_cases[0], 
                                       darkwing_duck_run_test_config,
                                       //darkwing_duck_run_tests_in_parallel,
                                       //darkwing_duck_run_passive_SP_test,
                                       DARKWING_DUCK_LUNS_PER_RAID_GROUP,
                                       DARKWING_DUCK_CHUNKS_PER_LUN);
    }

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end darkwing_duck_test()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_setup()
 ****************************************************************
 * @brief
 *  Setup for darkwing duck test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void darkwing_duck_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t raid_group_count = 0;

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
        darkwing_duck_choose_test_cases(&darkwing_duck_random_test_cases[0]);
    }

    darkwing_duck_create_random_config(&darkwing_duck_raid_group_config_random[0], darkwing_duck_num_test_cases_to_run);

    if (fbe_test_util_is_simulation()) {
        raid_group_count = fbe_test_get_rg_array_length(&darkwing_duck_raid_group_config_random[0]);
        /* Initialize the raid group configuration 
         */
        fbe_test_sep_util_init_rg_configuration_array(&darkwing_duck_raid_group_config_random[0]);

        /* Setup the physical config for the raid groups on both SPs
         */
        fbe_api_sim_transport_set_target_server(DARKWING_DUCK_ACTIVE_SP);
        elmo_create_physical_config_for_rg(&darkwing_duck_raid_group_config_random[0], raid_group_count);

        fbe_api_sim_transport_set_target_server(DARKWING_DUCK_PASSIVE_SP);
        elmo_create_physical_config_for_rg(&darkwing_duck_raid_group_config_random[0], raid_group_count);

        sep_config_load_sep_and_neit_load_balance_both_sps();

        /* Set the default target server back to A
         */
        fbe_api_sim_transport_set_target_server(DARKWING_DUCK_ACTIVE_SP);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************
 * end darkwing_duck_setup()
 ******************************************/

/*!**************************************************************
 * darkwing_duck_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the darkwing duck test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  8/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/

void darkwing_duck_cleanup(void)
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
 * end file darkwing_duck_test.c
 *************************/
