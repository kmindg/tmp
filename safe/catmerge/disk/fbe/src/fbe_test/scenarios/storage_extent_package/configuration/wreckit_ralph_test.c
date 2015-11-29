
/***************************************************************************
 * Copyright (C) EMC Corporation 2011 -2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

 /*!**************************************************************************
 * @file wreckit_ralph_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for validating the
 *   function of the Database Service
 *
 * @version
 *  11/23/2012  Wenxuan Yin  - Created
 *
 ***************************************************************************/

 /*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe_test.h"
#include "fbe_test_configurations.h"
#include "fbe_test_common_utils.h"

/*-----------------------------------------------------------------------------
     TEST DESCRIPTION:
 */
 
 char * wreckit_ralph_short_desc = 
     "Tests the consistency of configuration data during incomplete job handling.";
 char * wreckit_ralph_long_desc =
     "\n"
     "\n"
     "The Wreckit Ralph test tests if the database service works well during incomplete job handling.\n"
     "\n"   
     "Starting Config:\n"
     "\t [PP] armada board\n"
     "\t [PP] SAS PMC port\n"
     "\t [PP] viper enclosure\n"
     "\t [PP] 3 SAS drives (PDO)\n"
     "\t [PP] 3 logical drives (LDO)\n"
     "\t [SEP] 3 provision drives (PVD)\n"
     "\t [SEP] 3 virtual drives (VD)\n"
     "\t [SEP] 1 raid object (RAID)\n"   
     "\n"
     "CASE 1: SP panics when job is updating in-memory configuration.\n"
     "CASE 2: SP panics just before persisting operation.\n"
     "CASE 3: SP panics when job is persisting in journal region.\n"
     "CASE 4: SP panics just before job marks journal header as valid.\n"
     "CASE 5: SP panics when job is persisting in live data region.\n"
     "CASE 6: SP panics just before job invalidates the journal header.\n"
     "For above cases, follow steps:\n"
     "\t- (1). Send the raid group create or destroy job.\n"
     "\t- (2). Simulate active SP panics during different job handling point.\n"    
     "\t- (3). Verify behavior of incomplete job handling.\n"
     "\t- (4). Verify the raid group is created or destroyed after both SPs are reset\n"
     "CASE 7: Job returns failed transaction in persist service.\n"
     "For Case 7, follow steps:\n"
     "\t- (1). Send the raid group create or destroy job.\n"
     "\t- (2). Simulate job returns failed transaction.\n"
     "\t- (3). Verify the failed job notification.\n"
     "\t- (4). Verify the raid group is existed or not in rollback behavior.\n"
     "\n";
 
 /*-----------------------------------------------------------------------------
     DEFINITIONS:
 */

 /*!*******************************************************************
  * @def WRECKIT_RALPH_TEST_RAID_GROUP_COUNT
  *********************************************************************
  * @brief Max number of RGs to create
  *
  *********************************************************************/
#define WRECKIT_RALPH_TEST_RAID_GROUP_COUNT           2

 /*!*******************************************************************
  * @def WRECKIT_RALPH_TEST_NUMBER_OF_PHYSICAL_OBJECTS
  *********************************************************************
  * @brief Number of objects to create
  *        (1 board + 1 port + 1 encl + 15 pdo) 
  *
  *********************************************************************/
#define WRECKIT_RALPH_TEST_NUMBER_OF_PHYSICAL_OBJECTS   18 
    
 /*!*******************************************************************
  * @def WRECKIT_RALPH_TEST_DRIVE_CAPACITY
  *********************************************************************
  * @brief Number of blocks in the physical drive
  *
  *********************************************************************/
#define WRECKIT_RALPH_TEST_DRIVE_CAPACITY   TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY
     
 /*!*******************************************************************
  * @def WRECKIT_RALPH_TEST_RAID_GROUP_ID
  *********************************************************************
  * @brief RAID Group id used by this test
  *
  *********************************************************************/
#define WRECKIT_RALPH_TEST_RAID_GROUP_ID0        10
#define WRECKIT_RALPH_TEST_RAID_GROUP_ID1        11
     
     
 /*!*******************************************************************
  * @def WRECKIT_RALPH_DATABASE_WAIT_FOR_READY_MS
  *********************************************************************
  * @brief  Notification to timeout value in milliseconds 
  *
  *********************************************************************/
#define WRECKIT_RALPH_DATABASE_WAIT_FOR_READY_MS    10000 /*wait maximum of 10 seconds*/

 /*!*******************************************************************
  * @def WRECKIT_RALPH_TEST_NS_TIMEOUT
  *********************************************************************
  * @brief  Notification to timeout value in milliseconds 
  *
  *********************************************************************/
#define WRECKIT_RALPH_TEST_NS_TIMEOUT        60000 /*wait maximum of 60 seconds*/
     
     
 /*!*******************************************************************
  * @var WRECKIT_RALPH_abort_rg_configuration
  *********************************************************************
  * @brief This is rg_configuration
  *
  *********************************************************************/
 static fbe_test_rg_configuration_t wreckit_ralph_rg_configuration[WRECKIT_RALPH_TEST_RAID_GROUP_COUNT + 1] = 
 {
 /* First configuration is for destroy RG job */
    {/* width,  capacity,  raid type, class */
     3, FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,
     /*  block size, RAID-id, bandwidth*/
     512, WRECKIT_RALPH_TEST_RAID_GROUP_ID0, 0,
     /* rg_disk_set */
     {{0,0,5}, {0,0,6}, {0,0,8}}
    },
    
 /* Second configuration is for destroy RG job */
    {/* width,  capacity,  raid type, class */
     3, FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,
     /*  block size, RAID-id, bandwidth*/
     512, WRECKIT_RALPH_TEST_RAID_GROUP_ID1, 0,
     /* rg_disk_set */
     {{0,0,4}, {0,0,7}, {0,0,9}}
    },
    
 /* Terminator */   
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, },
 };

 /************************************************************************************
  * Components to support Table driven test methodology.
  * Following this approach, the parameters for the test will be stored
  * as rows in a table.
  * The test will then be executed for each row(i.e. set of parameters)
  * in the table.
  ***********************************************************************************/
 
 /* The different test numbers.
  */
 typedef enum wreckit_ralph_test_enclosure_num_e
 {
     WRECKIT_RALPH_TEST_ENCL1_WITH_SAS_DRIVES = 0,
 } wreckit_ralph_test_enclosure_num_t;
 
 /* The different drive types.
  */
 typedef enum wreckit_ralph_test_drive_type_e
 {
     SAS_DRIVE
 } wreckit_ralph_test_drive_type_t;
 
 /* This is a set of parameters for the wreckit_ralph test.
  */
 typedef struct enclosure_drive_details_s
 {
     wreckit_ralph_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
     fbe_u32_t port_number;                    /* The port on which current configuration exists */
     wreckit_ralph_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
     fbe_block_size_t block_size;
 } enclosure_drive_details_t;

 /* Table containing actual enclosure and drive details to be inserted.
  */
 static enclosure_drive_details_t encl_table[] = 
 {
     {   SAS_DRIVE,
         0,
         WRECKIT_RALPH_TEST_ENCL1_WITH_SAS_DRIVES,
         520
     },
 };

/* Count of rows in the table.
 */
#define WRECKIT_RALPH_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

/*-----------------------------------------------------------------------------
 FORWARD DECLARATIONS:
 */
static fbe_status_t wreckit_ralph_build_job_queue_element(
                            fbe_job_queue_element_t        *job_queue_element,
                            fbe_object_id_t                object_id,
                            fbe_u8_t                       *command_data_p,
                            fbe_u32_t                      command_size);
static fbe_status_t wreckit_ralph_disable_creation_queue_processing(fbe_object_id_t in_object_id);
static fbe_status_t wreckit_ralph_enable_creation_queue_processing(fbe_object_id_t in_object_id);
static void wreckit_ralph_panic_sp(fbe_sim_transport_connection_target_t sp);
static void wreckit_ralph_restore_sp(fbe_sim_transport_connection_target_t sp);
static void wreckit_ralph_shutdown_sp(fbe_sim_transport_connection_target_t sp);
static void wreckit_ralph_powerup_sp(fbe_sim_transport_connection_target_t sp);
static void wreckit_ralph_panic_sp_case1_update_transaction(void);
static void wreckit_ralph_panic_sp_case2_before_persist(void);
static void wreckit_ralph_panic_sp_case3_persist_write_jounal(void);
static void wreckit_ralph_panic_sp_case4_before_validate_jounal_header(void);
static void wreckit_ralph_panic_sp_case5_persist_write_live_data(void);
static void wreckit_ralph_panic_sp_case6_before_invalidate_jounal_header(void);
static void wreckit_ralph_simulate_job_returns_failed_transaction(void);
static void wreckit_ralph_check_database_state(fbe_database_state_t expected_db_state);
static void wreckit_ralph_verify_rollback_destroy_rg(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_replay_destroy_rg(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_rollback_create_rg(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_replay_create_rg(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_rg_destroyed_after_reset(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_rg_created_after_reset(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_rg_destroyed(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_rg_created(fbe_raid_group_number_t rg_number);
static void wreckit_ralph_verify_job_notification_destroy_rg(fbe_u64_t job_number);
static void wreckit_ralph_verify_job_notification_create_rg(fbe_u64_t job_number);
static void wreckit_ralph_issue_job_destroy_rg(fbe_test_rg_configuration_t *rg_configuration_p);
static void wreckit_ralph_issue_job_create_rg(fbe_test_rg_configuration_t *rg_configuration_p);

static void wreckit_ralph_test_case1_destroy_rg(void);
static void wreckit_ralph_test_case2_destroy_rg(void);
static void wreckit_ralph_test_case3_destroy_rg(void);
static void wreckit_ralph_test_case4_destroy_rg(void);
static void wreckit_ralph_test_case5_destroy_rg(void);
static void wreckit_ralph_test_case6_destroy_rg(void);
static void wreckit_ralph_test_case7_destroy_rg(void);

static void wreckit_ralph_test_case1_create_rg(void);
static void wreckit_ralph_test_case2_create_rg(void);
static void wreckit_ralph_test_case3_create_rg(void);
static void wreckit_ralph_test_case4_create_rg(void);
static void wreckit_ralph_test_case5_create_rg(void);
static void wreckit_ralph_test_case6_create_rg(void);
static void wreckit_ralph_test_case7_create_rg(void);

static void wreckit_ralph_test_load_physical_config(void);


 /*-----------------------------------------------------------------------------
     FUNCTIONS:
 */

 /*!**************************************************************
  * wreckit_ralph_build_job_queue_element()
  ****************************************************************
  * @brief
  *  This function builds a job queue element 
  * 
  * @param job_queue_element - element to build 
  * @param object_id           - the ID of the target object
  * @param command_data_p      - pointer to client data
  * @param command_size        - size of client's data
  *
  * @return fbe_status_t
  *  The status of the operation typically FBE_STATUS_OK.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  * 
  ****************************************************************/
 fbe_status_t wreckit_ralph_build_job_queue_element(
         fbe_job_queue_element_t        *job_queue_element,
         fbe_object_id_t                object_id,
         fbe_u8_t                       *command_data_p,
         fbe_u32_t                      command_size)
 {
     /* Context size cannot be greater than job element context size. */
     if (command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
     {
         return FBE_STATUS_INSUFFICIENT_RESOURCES;
     }
 
     job_queue_element->object_id = object_id;
     job_queue_element->job_type = FBE_JOB_TYPE_INVALID;
     job_queue_element->timestamp = 0;
     job_queue_element->previous_state = FBE_JOB_ACTION_STATE_INVALID;
     job_queue_element->current_state  = FBE_JOB_ACTION_STATE_INVALID;
     job_queue_element->queue_access = FBE_FALSE;
     job_queue_element->num_objects = 0;
 
     if (command_data_p != NULL)
     {
         fbe_copy_memory(job_queue_element->command_data, command_data_p, command_size);
     }
 
     return FBE_STATUS_OK;
 }

 /*!**************************************************************
 * wreckit_ralph_disable_creation_queue_processing()
 ****************************************************************
 * @brief
 *  This function sends a request to the job service to disable
 *  creation queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of disable request
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 * 
 ****************************************************************/
fbe_status_t wreckit_ralph_disable_creation_queue_processing(fbe_object_id_t in_object_id)
{
   return fbe_test_sep_util_disable_creation_queue(in_object_id);
}

/*!**************************************************************
 * wreckit_ralph_enable_creation_queue_processing()
 ****************************************************************
 * @brief
 *  This function sends a request to the job service to enable
 *  creation queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of enable request
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 * 
 ****************************************************************/
fbe_status_t wreckit_ralph_enable_creation_queue_processing(fbe_object_id_t in_object_id)
{
    return fbe_test_sep_util_enable_creation_queue(in_object_id);
}

/*!**************************************************************
 * wreckit_ralph_panic_sp()
 ****************************************************************
 * @brief
 *  Panic SP by killing the simulation client and process.
 *
 * @param fbe_sim_transport_connection_target_t              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp(fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t peer_sp;

    /* Get peer */
    peer_sp = (sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* go to sp which need panic */
    status = fbe_api_sim_transport_set_target_server(sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " <<< PANIC! %s >>>\n", sp == FBE_SIM_SP_A ? 
                                                            "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(sp);
    fbe_test_sp_sim_stop_single_sp(sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* we can't destroy fbe_api, because it's shared between two SPs */
    fbe_test_base_suite_startup_single_sp(sp);

    /* Set the target server to peer (not Active) SP */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

/*!**************************************************************
 * wreckit_ralph_restore_sp()
 ****************************************************************
 * @brief
 *  Restore SP.
 *
 * @param fbe_sim_transport_connection_target_t 
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_restore_sp(fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t                    status;

    mut_printf(MUT_LOG_TEST_STATUS, " %s entry\n", __FUNCTION__);

    /* go to sp which need restore */
    status = fbe_api_sim_transport_set_target_server(sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " <<< Restore %s from panic >>>\n", 
                            sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* reload the physical configuration and environment */
    wreckit_ralph_test_load_physical_config();
    sep_config_load_sep_and_neit();

    return;
}

/*!**************************************************************
 * wreckit_ralph_shutdown_sp()
 ****************************************************************
 * @brief
 *  Shutdown SP.
 *
 * @param fbe_sim_transport_connection_target_t              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_shutdown_sp(fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    /* go to sp which need shutdown*/
    status = fbe_api_sim_transport_set_target_server(sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " <<< Shutdown %s >>>\n", sp == FBE_SIM_SP_A ? 
                                                            "SP A" : "SP B");
    fbe_test_sep_util_destroy_neit_sep();

    return;
}

/*!**************************************************************
 * wreckit_ralph_powerup_sp()
 ****************************************************************
 * @brief
 *  Powerup SP.
 *
 * @param fbe_sim_transport_connection_target_t              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_powerup_sp(fbe_sim_transport_connection_target_t sp)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    /* go to sp which need powerup */
    status = fbe_api_sim_transport_set_target_server(sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, " <<< Powerup %s >>>\n", sp == FBE_SIM_SP_A ? 
                                                            "SP A" : "SP B");
    sep_config_load_sep_and_neit();

    return;
}

/*!**************************************************************
 * wreckit_ralph_panic_sp_case1_update_transaction()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics when the job is updating
 *  the database transaction table.
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp_case1_update_transaction()
{    
    fbe_status_t                                status;
    
    /* 1. Add hook to database servcie */
    status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_database_wait_hook(FBE_DATABASE_HOOK_TYPE_WAIT_IN_UPDATE_TRANSACTION, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    /* 4. Disable the job processing on passive SP before panic active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: passive SP Job processing is now disabled\n", __FUNCTION__);

    /* 5. PANIC active SP now! */
    wreckit_ralph_panic_sp(FBE_SIM_SP_A);

    return;
}

/*!**************************************************************
 * wreckit_ralph_panic_sp_case2_before_persist()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics when job completes updating 
 *  in-memory configuration, just before persist
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp_case2_before_persist()
{
    fbe_status_t                                status;
    
    /* 1. Add hook to database servcie */
    status = fbe_api_database_add_hook(FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_database_wait_hook(FBE_DATABASE_HOOK_TYPE_WAIT_BEFORE_TRANSACTION_PERSIST, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    /* 4. Disable the job processing on passive SP before panic active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: passive SP Job processing is now disabled\n", __FUNCTION__);

    /* 5. PANIC active SP now! */
    wreckit_ralph_panic_sp(FBE_SIM_SP_A);

    return;

}

/*!**************************************************************
 * wreckit_ralph_panic_sp_case3_persist_write_jounal()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics when job is persisting in 
 *  journal region.
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp_case3_persist_write_jounal()
{
    fbe_status_t                                status;
    
    /* 1. Add hook to persist servcie */
    status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_persist_wait_hook(FBE_PERSIST_HOOK_TYPE_SHRINK_JOURNAL_AND_WAIT, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    /* 4. Disable the job processing on passive SP before panic active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: passive SP Job processing is now disabled\n", __FUNCTION__);

    /* 5. PANIC active SP now! */
    wreckit_ralph_panic_sp(FBE_SIM_SP_A);

    return;

}

/*!**************************************************************
 * wreckit_ralph_panic_sp_case4_before_validate_jounal_header()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics before job validates the 
 *  journal header.
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp_case4_before_validate_jounal_header()
{
    fbe_status_t                                status;
    
    /* 1. Add hook to persist servcie */
    status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_MARK_JOURNAL_HEADER_VALID, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_persist_wait_hook(FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_MARK_JOURNAL_HEADER_VALID, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    /* 4. Disable the job processing on passive SP before panic active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: passive SP Job processing is now disabled\n", __FUNCTION__);

    /* 5. PANIC active SP now! */
    wreckit_ralph_panic_sp(FBE_SIM_SP_A);

    return;

}


/*!**************************************************************
 * wreckit_ralph_panic_sp_case4_persist_write_live_data()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics when job is persisting in 
 *  live data region.
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp_case5_persist_write_live_data()
{
    fbe_status_t                                status;
    
    /* 1. Add hook to persist servcie */
    status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_persist_wait_hook(FBE_PERSIST_HOOK_TYPE_SHRINK_LIVE_TRANSACTION_AND_WAIT, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    /* 4. Disable the job processing on passive SP before panic active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: passive SP Job processing is now disabled\n", __FUNCTION__);

    /* 5. PANIC active SP now! */
    wreckit_ralph_panic_sp(FBE_SIM_SP_A);

    return;

}

/*!**************************************************************
 * wreckit_ralph_panic_sp_case6_before_invalidate_jounal_header()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics before job invalidates the 
 *  journal header.
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_panic_sp_case6_before_invalidate_jounal_header()
{
    fbe_status_t                                status;
    
    /* 1. Add hook to persist servcie */
    status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_INVALIDATE_JOURNAL_HEADER, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_persist_wait_hook(FBE_PERSIST_HOOK_TYPE_WAIT_BEFORE_INVALIDATE_JOURNAL_HEADER, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    /* 4. Disable the job processing on passive SP before panic active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: passive SP Job processing is now disabled\n", __FUNCTION__);

    /* 5. PANIC active SP now! */
    wreckit_ralph_panic_sp(FBE_SIM_SP_A);

    return;

}

/*!**************************************************************
 * wreckit_ralph_panic_sp_case7_before_invalidate_jounal_header()
 ****************************************************************
 * @brief
 *  Simulate the active SP panics before job invalidates the 
 *  journal header.
 *
 * @param None.             
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_simulate_job_returns_failed_transaction()
{
    fbe_status_t                                status;
    
    /* 1. Add hook to persist servcie */
    status = fbe_api_persist_add_hook(FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to add hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now added\n", __FUNCTION__);

    /* 2. Now we can enable the job */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: active SP Job processing is now enabled\n", __FUNCTION__);

    /* 3. Wait for the hook to be triggered */
    status = fbe_api_persist_wait_hook(FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION, 
                                        WRECKIT_RALPH_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), 
                        "Wait to trigger hook timed out!");
    MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK), "FAIL to trigger hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: hook is now triggered\n", __FUNCTION__);

    return;

}



/*!**************************************************************
 * wreckit_ralph_check_database_state()
 ****************************************************************
 * @brief
 *  Check if the database is ready.
 *
 * @param fbe_database_state_t expected_state.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_check_database_state(fbe_database_state_t expected_db_state)
{
    fbe_status_t    status;

    MUT_ASSERT_INT_EQUAL(FBE_DATABASE_STATE_READY, expected_db_state);
    status = fbe_test_sep_util_wait_for_database_service_ready(WRECKIT_RALPH_DATABASE_WAIT_FOR_READY_MS);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                        "wreckit_ralph_test: FAIL to get db state!");
    return;
    
}

/*!**************************************************************
 * wreckit_ralph_verify_rollback_destroy_rg()
 ****************************************************************
 * @brief
 *  Verify the job rollback behavior.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_verify_rollback_destroy_rg(fbe_raid_group_number_t rg_number)
{
    /* Check if the DB state is ready */
    wreckit_ralph_check_database_state(FBE_DATABASE_STATE_READY);

    /* Verify the object id of the raid group */
    wreckit_ralph_verify_rg_created(rg_number);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Succeed!\n", __FUNCTION__);
    
    return;    
}

/*!**************************************************************
 * wreckit_ralph_verify_replay_destroy_rg()
 ****************************************************************
 * @brief
 *  Verify the job replay behavior.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_verify_replay_destroy_rg(fbe_raid_group_number_t rg_number)
{
    /* Check if the DB state is ready */
    wreckit_ralph_check_database_state(FBE_DATABASE_STATE_READY);

    /* Make sure the RAID object successfully destroyed */
    wreckit_ralph_verify_rg_destroyed(rg_number);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Succeed!\n", __FUNCTION__);
    
    return;    
}

 /*!**************************************************************
  * wreckit_ralph_verify_rollback_create_rg()
  ****************************************************************
  * @brief
  *  Verify the job rollback behavior.
  *
  * @param None.              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
 void wreckit_ralph_verify_rollback_create_rg(fbe_raid_group_number_t rg_number)
 {
     /* Check if the DB state is ready */
     wreckit_ralph_check_database_state(FBE_DATABASE_STATE_READY);
     
     /* Make sure the RAID object successfully destroyed */
     wreckit_ralph_verify_rg_destroyed(rg_number);
     
     mut_printf(MUT_LOG_TEST_STATUS, "%s: Succeed!\n", __FUNCTION__);
     
     return;    
 }
 
 /*!**************************************************************
  * wreckit_ralph_verify_replay_create_rg()
  ****************************************************************
  * @brief
  *  Verify the job replay behavior.
  *
  * @param None.              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
void wreckit_ralph_verify_replay_create_rg(fbe_raid_group_number_t rg_number)
{
     /* Check if the DB state is ready */
     wreckit_ralph_check_database_state(FBE_DATABASE_STATE_READY);
     
     /* Verify the object id of the raid group */
     wreckit_ralph_verify_rg_created(rg_number);
     
     mut_printf(MUT_LOG_TEST_STATUS, "%s: Succeed!\n", __FUNCTION__);
     
     return;    
}

 /*!**************************************************************
  * wreckit_ralph_verify_rg_destroyed_after_reset()
  ****************************************************************
  * @brief
  *  Verify if the rg of the param number is destroyed after
  *  both SPs are reset.
  *
  * @param fbe_raid_group_number_t rg_number              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
void wreckit_ralph_verify_rg_destroyed_after_reset(fbe_raid_group_number_t rg_number)
{
    fbe_status_t                                status;

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Shutdown SPB first */
    wreckit_ralph_shutdown_sp(FBE_SIM_SP_B);

    /* Bring dual sp mode back before reseting both SPs 
        - Restore SPA first, then power up SPB */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    wreckit_ralph_restore_sp(FBE_SIM_SP_A);
    wreckit_ralph_powerup_sp(FBE_SIM_SP_B);

    /* Verify RG status from view of SPB */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the RAID object successfully destroyed */
    mut_printf(MUT_LOG_TEST_STATUS, "SPB: ");
    wreckit_ralph_verify_rg_destroyed(rg_number);

    /* Verify RG status from view of SPA */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure the RAID object successfully destroyed */
    mut_printf(MUT_LOG_TEST_STATUS, "SPA: ");
    wreckit_ralph_verify_rg_destroyed(rg_number);

    return; 
}

 /*!**************************************************************
   * wreckit_ralph_verify_rg_created_after_reset()
   ****************************************************************
   * @brief
   *  Verify if the rg of the param number is created after
   *  both SPs are reset.
   *
   * @param fbe_raid_group_number_t rg_number              
   *
   * @return None.
   *
   * @version
   * 11/23/2012 - Created. Wenxuan Yin
   *
   ****************************************************************/
void wreckit_ralph_verify_rg_created_after_reset(fbe_raid_group_number_t rg_number)
{
     fbe_status_t                                status;

     fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

     /* Shutdown SPB first */
     wreckit_ralph_shutdown_sp(FBE_SIM_SP_B);
     
     /* Bring dual sp mode back before reseting both SPs 
         - Restore SPA first, then power up SPB */
     fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
     wreckit_ralph_restore_sp(FBE_SIM_SP_A);
     wreckit_ralph_powerup_sp(FBE_SIM_SP_B);

     /* Verify RG status from view of SPB */
     status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     /* Make sure the RAID object successfully destroyed */
     mut_printf(MUT_LOG_TEST_STATUS, "SPB: ");
     wreckit_ralph_verify_rg_created(rg_number);

     /* Verify RG status from view of SPA */
     status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     /* Make sure the RAID object successfully destroyed */
     mut_printf(MUT_LOG_TEST_STATUS, "SPA: ");
     wreckit_ralph_verify_rg_created(rg_number);
     
     return; 
}

  /*!**************************************************************
   * wreckit_ralph_verify_rg_destroyed()
   ****************************************************************
   * @brief
   *  Verify raid group is actually destroyed.
   *
   * @param fbe_raid_group_number_t rg_number             
   *
   * @return None.
   *
   * @version
   * 11/23/2012 - Created. Wenxuan Yin
   *
   ****************************************************************/
static void wreckit_ralph_verify_rg_destroyed(fbe_raid_group_number_t rg_number)
{
    fbe_status_t                                status;    
    fbe_object_id_t                             rg_object_id;

    /* Make sure the RAID object successfully destroyed */
    status = fbe_api_database_lookup_raid_group_by_number(rg_number, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, 
        "Raid group (object id: 0x%x, rg mumber: %d) is destroyed.\n", rg_object_id, rg_number);

    return;
}

 /*!**************************************************************
  * wreckit_ralph_verify_rg_created()
  ****************************************************************
  * @brief
  *  Verify raid group is actually created.
  *
  * @param fbe_raid_group_number_t rg_number.              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
static void wreckit_ralph_verify_rg_created(fbe_raid_group_number_t rg_number)
{
    fbe_status_t                                status;    
    fbe_object_id_t                             rg_object_id;

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
                                 rg_number, 
                                 &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                  FBE_LIFECYCLE_STATE_READY, 
                                                  WRECKIT_RALPH_TEST_NS_TIMEOUT,
                                                  FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, 
        "Raid group (object id 0x%x, rg mumber %d) is created.\n", rg_object_id, rg_number);

    return;
}

 /*!**************************************************************
   * wreckit_ralph_verify_job_notification_destroy_rg()
   ****************************************************************
   * @brief
   *  Verify job notification.
   *
   * @param None.              
   *
   * @return None.
   *
   * @version
   * 11/23/2012 - Created. Wenxuan Yin
   *
   ****************************************************************/
static void wreckit_ralph_verify_job_notification_destroy_rg(fbe_u64_t job_number)
{
    fbe_status_t                                    status;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;

    status = fbe_api_common_wait_for_job(job_number,
                                      WRECKIT_RALPH_TEST_NS_TIMEOUT,
                                      &job_error_code,
                                      &job_status,
                                      NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG destruction failed");
    /* RG probably has LUNs in it.. */
    MUT_ASSERT_TRUE_MSG((job_error_code != FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES), 
                 "RG cannot be destroyed for its upstream objects (probably LUNs), Delete those first..");

    return;
}


 /*!**************************************************************
   * wreckit_ralph_verify_job_notification_create_rg()
   ****************************************************************
   * @brief
   *  Verify job notification.
   *
   * @param None.              
   *
   * @return None.
   *
   * @version
   * 11/23/2012 - Created. Wenxuan Yin
   *
   ****************************************************************/
 static void wreckit_ralph_verify_job_notification_create_rg(fbe_u64_t job_number)
 {
    fbe_status_t                                status;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(job_number,
                                      WRECKIT_RALPH_TEST_NS_TIMEOUT,
                                      &job_error_code,
                                      &job_status,
                                      NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    return;
 }

/*!**************************************************************
* wreckit_ralph_issue_job_destroy_rg()
****************************************************************
* @brief
*  wreckit_ralph_test send a job to destroy raid group.
*
* @param None.              
*
* @return None.
*
* @version
* 11/23/2012 - Created. Wenxuan Yin
*
****************************************************************/
static void wreckit_ralph_issue_job_destroy_rg(fbe_test_rg_configuration_t *rg_configuration_p)
{
    fbe_status_t                                    status;
    fbe_api_job_service_raid_group_destroy_t        fbe_raid_group_destroy_request;
    fbe_object_id_t                                 rg_object_id;

    /* Validate that the raid group exist */
    status = fbe_api_database_lookup_raid_group_by_number(
                                 rg_configuration_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    fbe_test_sep_util_print_metadata_statistics(rg_object_id);

    /* Destroy a RAID group */
    fbe_raid_group_destroy_request.raid_group_id = rg_configuration_p->raid_group_id;
    fbe_raid_group_destroy_request.job_number = 0;
    fbe_raid_group_destroy_request.wait_destroy = FBE_FALSE;
    fbe_raid_group_destroy_request.destroy_timeout_msec = WRECKIT_RALPH_TEST_NS_TIMEOUT;

    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "RAID group destroy job sent successfully!\n");
    mut_printf(MUT_LOG_TEST_STATUS, 
            "Raid group id: %d Object Id: 0x%x Job number 0x%llx\n", 
            rg_configuration_p->raid_group_id, rg_object_id, 
            fbe_raid_group_destroy_request.job_number);

    /* Record the  job number */
    rg_configuration_p->job_number = fbe_raid_group_destroy_request.job_number;

    return;
}


/*!**************************************************************
 * wreckit_ralph_issue_job_create_rg()
 ****************************************************************
 * @brief
 *  wreckit_ralph_test send a job to create raid group.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
static void wreckit_ralph_issue_job_create_rg(fbe_test_rg_configuration_t *rg_configuration_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;

    /* Validate that the raid group does NOT exist */
    status = fbe_api_database_lookup_raid_group_by_number(
                                 rg_configuration_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    
    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "Create a RAID Group.\n");
    status = fbe_test_sep_util_create_raid_group(rg_configuration_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    return;
}

/*!**************************************************************
 * wreckit_ralph_test_case1_destroy_rg()
 ****************************************************************
 * @brief
 *  Test case 1: SP panics when job is updating in-memory 
 *  configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case1_destroy_rg()
{
    fbe_status_t                                status;
    fbe_test_rg_configuration_t                 *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[0];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 1 for destroying RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: setup RG
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Set up raid group. ~~~~~\n");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 2:  send the RG destroy job
    * -First disable the job processing on active SP
    * -Then send a job to destroy raid group created just now
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);

    /* Step 3: simulate active SP panics when job is updating in-memory configuration.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3: Panic active SP during transaction update. ~~~~~\n");
    wreckit_ralph_panic_sp_case1_update_transaction();

    /* Step 4:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify rollback: RG 10 is still there
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is destroyed
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 4 - 6: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 5: Verify that the database rolls-back the destroy raid group job.
     */
    wreckit_ralph_verify_rollback_destroy_rg(wreckit_ralph_rg_configuration[0].raid_group_id);
    
    /* Step 6: Verify that the raid group destroy job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 7: Verify the RG is destroyed after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 7: Verify the RG is destroyed after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_destroyed_after_reset(wreckit_ralph_rg_configuration[0].raid_group_id);

    return;
}

/*!**************************************************************
 * wreckit_ralph_test_case2_destroy_rg()
 ****************************************************************
 * @brief
 *  Test case 2: SP panics just after job complete updating 
 *  in-memory configuration and before persist operation.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case2_destroy_rg()
{
    fbe_status_t                                status;
    fbe_test_rg_configuration_t                 *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[0];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 2 for destroying RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: setup RG
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Set up raid group. ~~~~~\n");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 2:  send the RG destroy job
    * -First disable the job processing on active SP
    * -Then send a job to destroy raid group created just now
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);

    /* Step 3: simulate active SP panics when job complete updating 
    *  in-memory configuration and before persist operation.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3: Panic active SP before persist operation. ~~~~~\n");
    wreckit_ralph_panic_sp_case2_before_persist();

    /* Step 4:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is destroyed
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is destroyed
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 4 - 6: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 5: Verify that the database rolls-back the destroy raid group job.
     */
    wreckit_ralph_verify_rollback_destroy_rg(wreckit_ralph_rg_configuration[0].raid_group_id);
    
    /* Step 6: Verify that the raid group destroy job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 7: Verify the RG is destroyed after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 7: Verify the RG is destroyed after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_destroyed_after_reset(wreckit_ralph_rg_configuration[0].raid_group_id);

    return;

}

/*!**************************************************************
 * wreckit_ralph_test_case3_destroy_rg()
 ****************************************************************
 * @brief
 *  Test case 3: SP panics when job is persisting journal region.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case3_destroy_rg()
{
    fbe_status_t                                status;
    fbe_test_rg_configuration_t                 *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[0];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 3 for destroying RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: setup RG
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Set up raid group. ~~~~~\n");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 2:  send the RG destroy job
    * -First disable the job processing on active SP
    * -Then send a job to destroy raid group created just now
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);

    /* Step 3: simulate active SP panics when job is is persisting journal region.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3: Panic active SP when persisting in journal region. ~~~~~\n");
    wreckit_ralph_panic_sp_case3_persist_write_jounal();

    /* Step 4:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is destroyed
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is destroyed
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 4 - 6: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 5: Verify that the database rolls-back the destroy raid group job.
     */
    wreckit_ralph_verify_rollback_destroy_rg(wreckit_ralph_rg_configuration[0].raid_group_id);
    
    /* Step 6: Verify that the raid group destroy job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 7: Verify the RG is destroyed after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 7: Verify the RG is destroyed after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_destroyed_after_reset(wreckit_ralph_rg_configuration[0].raid_group_id);

    return;

}

/*!**************************************************************
 * wreckit_ralph_test_case4_destroy_rg()
 ****************************************************************
 * @brief
 *  Test case 4: SP panics before job validates the journal header.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case4_destroy_rg()
{
    fbe_status_t                                status;
    fbe_test_rg_configuration_t                 *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[0];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 4 for destroying RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: setup RG
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Set up raid group. ~~~~~\n");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 2:  send the RG destroy job
    * -First disable the job processing on active SP
    * -Then send a job to destroy raid group created just now
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);

    /* Step 3: simulate active SP panics before validate the journal header.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
        "~~~~~ Step 3: Panic active SP before validate the journal header. ~~~~~\n");
    wreckit_ralph_panic_sp_case4_before_validate_jounal_header();

    /* Step 4:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is destroyed
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is destroyed
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 4 - 6: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 5: Verify that the database rolls-back the destroy raid group job.
     */
    wreckit_ralph_verify_rollback_destroy_rg(wreckit_ralph_rg_configuration[0].raid_group_id);
    
    /* Step 6: Verify that the raid group destroy job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 7: Verify the RG is destroyed after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 7: Verify the RG is destroyed after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_destroyed_after_reset(wreckit_ralph_rg_configuration[0].raid_group_id);

    return;

}


/*!**************************************************************
 * wreckit_ralph_test_case5_destroy_rg()
 ****************************************************************
 * @brief
 *  Test case 5: SP panics when job is persisting live data region.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case5_destroy_rg()
{
    fbe_status_t                                status;
    fbe_test_rg_configuration_t                 *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[0];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 5 for destroying RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: setup RG
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Set up raid group. ~~~~~\n");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 2:  send the RG destroy job
    * -First disable the job processing on active SP
    * -Then send a job to destroy raid group created just now
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);

    /* Step 3: simulate active SP panics when job is persisting live data region.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3: Panic active SP during persisting live data region. ~~~~~\n");
    wreckit_ralph_panic_sp_case5_persist_write_live_data();

    /* Step 4:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is destroyed
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is destroyed
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 4 - 6: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 5: Verify that the database rolls-back the destroy raid group job.
     */
    wreckit_ralph_verify_rollback_destroy_rg(wreckit_ralph_rg_configuration[0].raid_group_id);
    
    /* Step 6: Verify that the raid group destroy job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);

    /* Step 7: Verify the RG is destroyed after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 7: Verify the RG is destroyed after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_destroyed_after_reset(wreckit_ralph_rg_configuration[0].raid_group_id);

    return;

}

 /*!**************************************************************
  * wreckit_ralph_test_case6_destroy_rg()
  ****************************************************************
  * @brief
  *  Test case 6: SP panics before job invalidates the journal header.
  *
  * @param None.              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
 void wreckit_ralph_test_case6_destroy_rg()
 {
     fbe_status_t                                status;
     fbe_test_rg_configuration_t                 *rg_configuration_p;

     rg_configuration_p = &wreckit_ralph_rg_configuration[0];
 
     mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 6 for destroying RG =====\n");
 
     /* Start from the active SP */
     status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
     /* Step 1: setup RG
     * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 1: Set up raid group. ~~~~~\n");
     wreckit_ralph_issue_job_create_rg(rg_configuration_p);
     wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
     wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);
 
     /* Step 2:  send the RG destroy job
     * -First disable the job processing on active SP
     * -Then send a job to destroy raid group created just now
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
     status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
     MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
     wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
 
     /* Step 3: simulate active SP panics before invalidate journal header.
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
         "~~~~~ Step 3: Panic active SP before invalidate the journal header. ~~~~~\n");
     wreckit_ralph_panic_sp_case6_before_invalidate_jounal_header();
 
     /* Step 4:  Verify behavior of incomplete job handling
     * -Go to single sp mode since active SP is down
     * -Verify replay: RG 10 is destroyed
     * -Enable job processing on passive SP
     * -Verify the job is successful and RG 10 is destroyed
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 4 - 6: Verify behavior of incomplete job handling. ~~~~~\n");
     status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
     
    /* Step 5: Verify that the database rolls-back the destroy raid group job.
     */
     wreckit_ralph_verify_rollback_destroy_rg(wreckit_ralph_rg_configuration[0].raid_group_id);
     
    /* Step 6: Verify that the raid group destroy job is restarted and completes.
     */
     status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
     MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
     wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
     wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);
 
     /* Step 7: Verify the RG is destroyed after both SPs are reset.
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
              "~~~~~ Step 7: Verify the RG is destroyed after both SPs are reset. ~~~~~\n");
     wreckit_ralph_verify_rg_destroyed_after_reset(wreckit_ralph_rg_configuration[0].raid_group_id);
 
     return;
 
 }

 /*!**************************************************************
  * wreckit_ralph_test_case7_destroy_rg()
  ****************************************************************
  * @brief
  *  Test case 7: Job returns failed transaction in persist service.
  *
  * @param None.              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
 void wreckit_ralph_test_case7_destroy_rg()
 {
     fbe_status_t                                    status;
     fbe_job_service_error_type_t                    job_error_code;
     fbe_status_t                                    job_status;
     fbe_test_rg_configuration_t                     *rg_configuration_p;

     rg_configuration_p = &wreckit_ralph_rg_configuration[0];
     
     mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 7 for destroying RG =====\n");
     
     /* Start from the active SP */
     status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
     
     /* Step 1: setup RG
     * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 1: Set up raid group. ~~~~~\n");
     wreckit_ralph_issue_job_create_rg(rg_configuration_p);
     wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[0].job_number);
     wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);
     
     /* Step 2:  send the RG destroy job
     * -First disable the job processing on active SP
     * -Then send a job to destroy raid group created just now
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 2: Send RG destroy job. ~~~~~\n");
     status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
     MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
     wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);

     /* Step 3: simulate Job returns failed transaction in persist service.
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 3: Simulate Job returns failed transaction in persist service. ~~~~~\n");
     wreckit_ralph_simulate_job_returns_failed_transaction();

     /* Step 4: verify the job processing returns failed
     */    
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 4: Verify the job processing returns failed. ~~~~~\n");
     status = fbe_api_common_wait_for_job(wreckit_ralph_rg_configuration[0].job_number,
                                       WRECKIT_RALPH_TEST_NS_TIMEOUT,
                                       &job_error_code,
                                       &job_status,
                                       NULL);
     MUT_ASSERT_TRUE_MSG((job_status != FBE_STATUS_OK), 
        "Job doesn't return failed transaction in persist service!");

     /* Step 5: verify the RG is still there 
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                 "~~~~~ Step 5: Verify the RG is STILL there . ~~~~~\n");
     wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[0].raid_group_id);
     
     /* Step 6: Destroy the RG at last 
     */
     mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 6: Destroy the raid group. ~~~~~\n");

     /* We need to clean the hook first! */
     status = fbe_api_persist_remove_hook(FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION, FBE_PACKAGE_ID_SEP_0);
     MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to remove hook!");
     mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now removed\n", __FUNCTION__);
     
     wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
     wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[0].job_number);
     wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[0].raid_group_id);
     
     return;

 }


 /*!**************************************************************
 * wreckit_ralph_test_case1_create_rg()
 ****************************************************************
 * @brief
 *  Test case 1: SP panics when job is updating in-memory 
 *  configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case1_create_rg()
{
    fbe_status_t                                status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 1 for creating RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate active SP panics when job is updating in-memory configuration.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Panic active SP during transaction update. ~~~~~\n");
    wreckit_ralph_panic_sp_case1_update_transaction();

    /* Step 3:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify rollback: RG 10 is NOT there
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is created
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3 - 5: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 4: Verify that the database rolls-back the create raid group job.
     */
    wreckit_ralph_verify_rollback_create_rg(wreckit_ralph_rg_configuration[1].raid_group_id);
    
    /* Step 5: Verify that the raid group create job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: Verify the RG is created after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 6: Verify the RG is created after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_created_after_reset(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 5: destroy RG at last
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 7: Destroy the raid group. ~~~~~\n");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    return;
}

/*!**************************************************************
 * wreckit_ralph_test_case2_create_rg()
 ****************************************************************
 * @brief
 *  Test case 2: SP panics just after job complete updating 
 *  in-memory configuration and before persist operation.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case2_create_rg()
{
    fbe_status_t                                    status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 2 for creating RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate active SP panics before persist operation.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Panic active SP before persist operation. ~~~~~\n");
    wreckit_ralph_panic_sp_case2_before_persist();

    /* Step 3:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is created
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is created
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3 - 5: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 4: Verify that the database rolls-back the create raid group job.
     */
    wreckit_ralph_verify_rollback_create_rg(wreckit_ralph_rg_configuration[1].raid_group_id);
    
    /* Step 5: Verify that the raid group create job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: Verify the RG is created after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 6: Verify the RG is created after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_created_after_reset(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 5: destroy RG at last
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 5: Destroy the raid group. ~~~~~\n");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    return;

}

/*!**************************************************************
 * wreckit_ralph_test_case3_create_rg()
 ****************************************************************
 * @brief
 *  Test case 3: SP panics when job is persisting journal region.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case3_create_rg()
{
    fbe_status_t                                    status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 3 for creating RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate active SP panics when persisting in journal region.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Panic active SP when persisting in journal region. ~~~~~\n");
    wreckit_ralph_panic_sp_case3_persist_write_jounal();

    /* Step 3:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is created
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is created
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3 - 5: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 4: Verify that the database rolls-back the create raid group job.
     */
    wreckit_ralph_verify_rollback_create_rg(wreckit_ralph_rg_configuration[1].raid_group_id);
    
    /* Step 5: Verify that the raid group create job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: Verify the RG is created after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 6: Verify the RG is created after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_created_after_reset(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 5: destroy RG at last
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 5: Destroy the raid group. ~~~~~\n");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    return;
    
}

/*!**************************************************************
 * wreckit_ralph_test_case4_create_rg()
 ****************************************************************
 * @brief
 *  Test case 4: SP panics before job validates the journal header.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case4_create_rg()
{
    fbe_status_t                                    status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 4 for creating RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate active SP panics before validate the journal header.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Panic active SP before validate the journal header. ~~~~~\n");
    wreckit_ralph_panic_sp_case4_before_validate_jounal_header();

    /* Step 3:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is created
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is created
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3 - 5: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 4: Verify that the database rolls-back the create raid group job.
     */
    wreckit_ralph_verify_rollback_create_rg(wreckit_ralph_rg_configuration[1].raid_group_id);
    
    /* Step 5: Verify that the raid group create job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: Verify the RG is created after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 6: Verify the RG is created after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_created_after_reset(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: destroy RG at last
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 7: Destroy the raid group. ~~~~~\n");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    return;
    
}

/*!**************************************************************
 * wreckit_ralph_test_case5_create_rg()
 ****************************************************************
 * @brief
 *  Test case 5: SP panics when job is persisting live data region.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case5_create_rg()
{
    fbe_status_t                                    status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 5 for creating RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate active SP panics during persisting live data region.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Panic active SP during persisting live data region. ~~~~~\n");
    wreckit_ralph_panic_sp_case5_persist_write_live_data();

    /* Step 3:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is created
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is created
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3 - 5: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 4: Verify that the database rolls-back the create raid group job.
     */
    wreckit_ralph_verify_rollback_create_rg(wreckit_ralph_rg_configuration[1].raid_group_id);
    
    /* Step 5: Verify that the raid group create job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: Verify the RG is created after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 6: Verify the RG is created after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_created_after_reset(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 7: destroy RG at last
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 7: Destroy the raid group. ~~~~~\n");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    return;
    
}

/*!**************************************************************
 * wreckit_ralph_test_case6_create_rg()
 ****************************************************************
 * @brief
 *  Test case 6: SP panics before job invalidates the journal header.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case6_create_rg()
{
    fbe_status_t                                    status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];

    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 6 for creating RG =====\n");

    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate active SP panics before invalidate the journal header.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Panic active SP before invalidate the journal header. ~~~~~\n");
    wreckit_ralph_panic_sp_case6_before_invalidate_jounal_header();

    /* Step 3:  Verify behavior of incomplete job handling
    * -Go to single sp mode since active SP is down
    * -Verify replay: RG 10 is created
    * -Enable job processing on passive SP
    * -Verify the job is successful and RG 10 is created
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3 - 5: Verify behavior of incomplete job handling. ~~~~~\n");
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    /* Step 4: Verify that the database rolls-back the create raid group job.
     */
    wreckit_ralph_verify_rollback_create_rg(wreckit_ralph_rg_configuration[1].raid_group_id);
    
    /* Step 5: Verify that the raid group create job is restarted and completes.
     */
    status = wreckit_ralph_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to enable the job processing!");
    wreckit_ralph_verify_job_notification_create_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_created(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 6: Verify the RG is created after both SPs are reset.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
             "~~~~~ Step 6: Verify the RG is created after both SPs are reset. ~~~~~\n");
    wreckit_ralph_verify_rg_created_after_reset(wreckit_ralph_rg_configuration[1].raid_group_id);

    /* Step 7: destroy RG at last
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 7: Destroy the raid group. ~~~~~\n");
    wreckit_ralph_issue_job_destroy_rg(rg_configuration_p);
    wreckit_ralph_verify_job_notification_destroy_rg(wreckit_ralph_rg_configuration[1].job_number);
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    return;
 
 }

/*!**************************************************************
 * wreckit_ralph_test_case7_create_rg()
 ****************************************************************
 * @brief
 *  Test case 7: Job returns failed transaction in persist service.
 *
 * @param None.              
 *
 * @return None.
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 ****************************************************************/
void wreckit_ralph_test_case7_create_rg()
{
    fbe_status_t                                    status;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;
    fbe_test_rg_configuration_t                     *rg_configuration_p;

    rg_configuration_p = &wreckit_ralph_rg_configuration[1];
    
    mut_printf(MUT_LOG_TEST_STATUS, "===== Wreckit Ralph Test Case 7 for creating RG =====\n");
    
    /* Start from the active SP */
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Step 1:  send the RG create job
    * -First disable the job processing on active SP
    * -Then send a job to create raid group
    * -Use 0_0_5, 0_0_6, 0_0_8 to create one R5 with RG number 10 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 1: Send RG create job. ~~~~~\n");
    status = wreckit_ralph_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to disable the job processing!");
    wreckit_ralph_issue_job_create_rg(rg_configuration_p);

    /* Step 2: simulate Job returns failed transaction in persist service.
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 2: Simulate Job returns failed transaction in persist service. ~~~~~\n");
    wreckit_ralph_simulate_job_returns_failed_transaction();

    /* Step 3: verify the job processing returns failed
    */    
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 3: Verify the job processing returns failed. ~~~~~\n");
    status = fbe_api_common_wait_for_job(wreckit_ralph_rg_configuration[1].job_number,
                                      WRECKIT_RALPH_TEST_NS_TIMEOUT,
                                      &job_error_code,
                                      &job_status,
                                      NULL);
    MUT_ASSERT_TRUE_MSG((job_status != FBE_STATUS_OK), 
       "Job doesn't return failed transaction in persist service!");
    
    /* Step 4: verify the RG is NOT there 
    */
    mut_printf(MUT_LOG_TEST_STATUS, 
                "~~~~~ Step 4: Verify the RG is NOT there . ~~~~~\n");
    wreckit_ralph_verify_rg_destroyed(wreckit_ralph_rg_configuration[1].raid_group_id);

    
    /* We need to clean the hook */
    status = fbe_api_persist_remove_hook(FBE_PERSIST_HOOK_TYPE_RETURN_FAILED_TRANSACTION, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "FAIL to remove hook!");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Debug hook is now removed\n", __FUNCTION__);

    return;

}


/*!**************************************************************
  * wreckit_ralph_test_load_physical_config()
  ****************************************************************
  * @brief
  *  Load the wreckit_ralph test physical configuration.
  *
  * @param None.              
  *
  * @return None.
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  ****************************************************************/
 static void wreckit_ralph_test_load_physical_config()
 {
 
     fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
     fbe_u32_t                               total_objects = 0;
     fbe_class_id_t                          class_id;
     fbe_api_terminator_device_handle_t      port0_handle;
     fbe_api_terminator_device_handle_t      port0_encl_handle[WRECKIT_RALPH_TEST_ENCL_MAX];
     fbe_api_terminator_device_handle_t      drive_handle;
     fbe_u32_t                               slot = 0;
     fbe_u32_t                               enclosure = 0;
 
     mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");
 
     mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
     /* before loading the physical package we initialize the terminator */
     status = fbe_api_terminator_init();
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
     /* Insert a board
      */
     status = fbe_test_pp_util_insert_armada_board();
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
     /* Insert a port
      */
     fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                             2, /* portal */
                                             0, /* backend number */ 
                                             &port0_handle);
 
     /* Insert enclosures to port 0
      */
     for ( enclosure = 0; enclosure < WRECKIT_RALPH_TEST_ENCL_MAX; enclosure++)
     { 
         fbe_test_pp_util_insert_viper_enclosure(0,
                                                 enclosure,
                                                 port0_handle,
                                                 &port0_encl_handle[enclosure]);
     }
 
     /* Insert drives for enclosures.
      */
     for ( enclosure = 0; enclosure < WRECKIT_RALPH_TEST_ENCL_MAX; enclosure++)
     {
         for ( slot = 0; slot < FBE_TEST_DRIVES_PER_ENCL; slot++)
         {
             fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number, 
                                               encl_table[enclosure].encl_number,   
                                               slot,                                
                                               encl_table[enclosure].block_size,    
                                               TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,      
                                               &drive_handle);
         }
     }
 
     mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
     status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
     mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
     status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
     /* Init fbe api on server */
     mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
     status = fbe_api_sim_server_init_fbe_api();
     MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
     /* Wait for the expected number of objects */
     status = fbe_api_wait_for_number_of_objects(WRECKIT_RALPH_TEST_NUMBER_OF_PHYSICAL_OBJECTS, WRECKIT_RALPH_TEST_NS_TIMEOUT, FBE_PACKAGE_ID_PHYSICAL);
     MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
 
     mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");
 
     /* Inserted a armada board so we check for it;
      * board is always assumed to be object id 0
      */
     status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
     MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
     mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");
 
     /* Make sure we have the expected number of objects.
      */
     status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
     MUT_ASSERT_TRUE(total_objects == WRECKIT_RALPH_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
     mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");
 
     return;
 
 }


 /*!****************************************************************************
  *  wreckit_ralph_dualsp_setup()
  ******************************************************************************
  *
  * @brief
  *  This is the setup function for wreckit_ralph_test in dualsp mode. It is responsible
  *  for loading the physical configuration for this test suite on both SPs.
  *
  * @param   None
  *
  * @return  None
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  *****************************************************************************/
 void wreckit_ralph_dualsp_setup()
 {
     fbe_sim_transport_connection_target_t   sp;
     fbe_status_t status = FBE_STATUS_OK;
 
     if (fbe_test_util_is_simulation())
     {
         mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for wreckit ralph test ===\n");
         /* Set the target server */
         status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
         /* Make sure target set correctly */
         sp = fbe_api_sim_transport_get_target_server();
         MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
 
         mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                    sp == FBE_SIM_SP_A ? "SP A" : "SP B");
 
         /* Load the physical config on the target server */
         wreckit_ralph_test_load_physical_config();
 
         /* Set the target server */
         status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
 
         /* Make sure target set correctly */
         sp = fbe_api_sim_transport_get_target_server();
         MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
 
         mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                    sp == FBE_SIM_SP_A ? "SP A" : "SP B");
 
         /* Load the physical config on the target server */
         wreckit_ralph_test_load_physical_config();
 
         /* We will work primarily with SPA.  The configuration is automatically
          * instantiated on both SPs.  We no longer create the raid groups during
          * the setup.
          */
         status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
         sep_config_load_sep_and_neit_both_sps();
         fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
     }
 
     return;
 
 }

 
 /*!****************************************************************************
  *  wreckit_ralph_dualsp_cleanup()
  ******************************************************************************
  *
  * @brief
  *  This is the cleanup function for the wreckit_ralph_dualsp_test.
  *
  * @param   None
  *
  * @return  None
  *
  * @version
  * 11/23/2012 - Created. Wenxuan Yin
  *
  *****************************************************************************/
 void wreckit_ralph_dualsp_cleanup()
 {
     fbe_status_t status = FBE_STATUS_OK;
     
     mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for wreckit_ralph_dualsp_test ===\n");
 
     /* Always clear dualsp mode */
     fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
 
     if (fbe_test_util_is_simulation())
     {
         /* First execute teardown on SP B then on SP A */
         status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
         /* clear the error counter */
         fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
         fbe_test_sep_util_destroy_neit_sep_physical();
 
         /* First execute teardown on SP A */
         status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
         /* clear the error counter */
         fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
         fbe_test_sep_util_destroy_neit_sep_physical();
     }
     return;
 
 }

 /*!****************************************************************************
   *  wreckit_ralph_dualsp_test_job_destroy_rg_test1()
   ******************************************************************************
   *
   * @brief
   *  This test focus on the destroying RG job, main entry.  
   *
   * @param   None
   *
   * @return  None 
   *
   * @version
   * 11/23/2012 - Created. Wenxuan Yin
   *
   *****************************************************************************/
  void wreckit_ralph_dualsp_test_job_destroy_rg_test1()
 {
     /* Run the cases sequentially */
     wreckit_ralph_test_case1_destroy_rg();
     wreckit_ralph_test_case2_destroy_rg();
     wreckit_ralph_test_case3_destroy_rg();
     wreckit_ralph_test_case4_destroy_rg();
     return;
 }

/*!****************************************************************************
 *  wreckit_ralph_dualsp_test_job_destroy_rg_test2()
 ******************************************************************************
 *
 * @brief
 *  This test focus on the destroying RG job, main entry.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @version
 * 11/23/2012 - Created. Wenxuan Yin
 *
 *****************************************************************************/
void wreckit_ralph_dualsp_test_job_destroy_rg_test2()
{
   /* Run the cases sequentially */
   wreckit_ralph_test_case5_destroy_rg();
   wreckit_ralph_test_case6_destroy_rg();
   wreckit_ralph_test_case7_destroy_rg();
   return;
}

 
  /*!****************************************************************************
    *  wreckit_ralph_dualsp_test_job_create_rg_test1()
    ******************************************************************************
    *
    * @brief
    *  This test focus on the creating RG job, main entry.  
    *
    * @param   None
    *
    * @return  None 
    *
    * @version
    * 11/23/2012 - Created. Wenxuan Yin
    *
    *****************************************************************************/
   void wreckit_ralph_dualsp_test_job_create_rg_test1()
  {
      /* Run the cases sequentially */
      wreckit_ralph_test_case1_create_rg();
      wreckit_ralph_test_case2_create_rg();
      wreckit_ralph_test_case3_create_rg();
      wreckit_ralph_test_case4_create_rg();
      return;
  }

 
 /*!****************************************************************************
   wreckit_ralph_dualsp_test_job_create_rg_test2()
   ******************************************************************************
   *
   * @brief
   *  This test focus on the creating RG job, main entry.  
   *
   * @param   None
   *
   * @return  None 
   *
   * @version
   * 11/23/2012 - Created. Wenxuan Yin
   *
   *****************************************************************************/
 void wreckit_ralph_dualsp_test_job_create_rg_test2()
 {
     /* Run the cases sequentially */
     wreckit_ralph_test_case5_create_rg();
     wreckit_ralph_test_case6_create_rg();
     wreckit_ralph_test_case7_create_rg();
     return;
 }
