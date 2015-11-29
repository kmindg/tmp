/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file monkey_king_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for update LUN.
 *
 * @version
 *   03/11/2011 - Created. guov
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
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"


/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* monkey_king_short_desc = "LUN update operation";
char* monkey_king_long_desc =
    "\n"
    "\n"
    "The Monkey King scenario tests updating the wwn to an existing LUN,\n"
    "And the change is persisted thru reboot.\n"
    "\n"
    "Dependencies:\n"
    "    - Job Control Service (JCS) API solidified\n"
    "    - JCS creation queue operational\n"
    "    - Configuration (Config) Service able to persist database both locally and on the peer\n"
    "    - Ability to get lun info using fbe_api\n"
    "    - Ability to change lun info using fbe_api\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 3 SAS drives (PDOs)\n"
    "    [PP] 3 logical drives (LDs)\n"
    "    [SEP] 3 provision drives (PVDs)\n"
    "    [SEP] 3 virtual drives (VDs)\n"
    "    [SEP] 1 RAID object (RAID-5)\n"
    "    [SEP] 1 LUN object \n"

    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - Create a RAID object(RG 0) on 3 drives\n"
    "    - Create a LUN object(LUN 0) on RG 0\n"
    "\n" 
    "STEP 2: Send a LUN update request using fbe_api\n"
    "    - Verify that wwn is modified in memory thru get_lun_info.\n"
    "\n"
    "STEP 3: Boot peer SP to test peer synch\n"
    "    - Verify the lun has the wwn set in Step 2, thru get_lun_info.\n"
    "\n"
    "STEP 4: Send a LUN update request using fbe_api\n"
    "    - Verify the lun has the new wwn on SPA.\n"
    "    - Verify the lun has the new wwn on SPB.\n"
    "\n"
    "STEP 5: Reboot SPA and SPB\n"
    "    - Verify the lun has the wwn set in Step 4 on SPA.\n"
    "    - Verify the lun has the wwn set in Step 4 on SPB.\n"
    "\n";        

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/
typedef struct monkey_king_lun_user_config_data_s{
    /*! World-Wide Name associated with the LUN
     */
    fbe_assigned_wwid_t      world_wide_name;

    /*! User-Defined Name for the LUN
     */
    fbe_user_defined_lun_name_t          user_defined_name;
}monkey_king_lun_user_config_data_t;

/*!*******************************************************************
 * @def MONKEY_KING_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define MONKEY_KING_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def MONKEY_KING_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define MONKEY_KING_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def MONKEY_KING_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define MONKEY_KING_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def MONKEY_KING_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define MONKEY_KING_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def MONKEY_KING_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define MONKEY_KING_TEST_RAID_GROUP_ID          0

#define MONKEY_KING_INVALID_TEST_RAID_GROUP_ID  5


/*!*******************************************************************
 * @var monkey_king_disk_set_520
 *********************************************************************
 * @brief This is the disk set for the 520 RG (520 byte per block SAS)
 *
 *********************************************************************/
static fbe_u32_t monkey_king_disk_set_520[MONKEY_KING_TEST_RAID_GROUP_COUNT][MONKEY_KING_TEST_MAX_RG_WIDTH][3] = 
{
    /* width = 3 */
    { {0,0,3}, {0,0,4}, {0,0,5}}
};


/*!*******************************************************************
 * @def MONKEY_KING_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define MONKEY_KING_TEST_LUN_CONFIG_COUNT        3


/*!*******************************************************************
 * @def MONKEY_KING_TEST_LUN_NUMBER
 *********************************************************************
 * @brief  Lun number used by this test
 *
 *********************************************************************/
static fbe_lun_number_t monkey_king_lun_set[MONKEY_KING_TEST_LUN_CONFIG_COUNT] = {123, 234, 456};

/*!*******************************************************************
 * @def MONKEY_KING_TEST_LUN_USER_CONFIG_DATA
 *********************************************************************
 * @brief  Lun wwn and user_defined_name used by this test
 *
 *********************************************************************/
static monkey_king_lun_user_config_data_t monkey_king_lun_user_config_data_set[MONKEY_KING_TEST_LUN_CONFIG_COUNT] = 
{   {{01,02,03,04,05,06,07,00},{0xD3,0xE4,0xF0,0x00}},
    {{01,02,03,04,05,06,07,01},{0xD3,0xE4,0xF1,0x00}},
    {{01,02,03,04,05,06,07,02},{0xD3,0xE4,0xF2,0x00}},
};

/*!*******************************************************************
 * @def MONKEY_KING_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define MONKEY_KING_TEST_NS_TIMEOUT        5000 /*wait maximum of 5 seconds*/


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void monkey_king_test_load_physical_config(void);
static void monkey_king_test_create_logical_config(void);
static void monkey_king_test_calculate_imported_capacity(void);
static void monkey_king_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_assigned_wwid_t   *in_fbe_lun_update_req_p);
static void monkey_king_start_sep_on_node(fbe_bool_t wait, fbe_sim_transport_connection_target_t target);
static void monkey_king_stop_sep_on_node(fbe_sim_transport_connection_target_t target);


/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  monkey_king_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Monkey King test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void monkey_king_test(void)
{
    fbe_status_t                        status;
    fbe_assigned_wwid_t                 fbe_lun_wwn_update_req;
    fbe_object_id_t                     lun0_id; //, lun1_id, lun2_id;
    fbe_sim_transport_connection_target_t   active_connection_target    = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   passive_connection_target   = FBE_SIM_INVALID_SERVER;
    fbe_job_service_error_type_t    job_error_type;


    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Monkey King Test ===\n");

    /* Get the active SP */
    fbe_test_sep_get_active_target_id(&active_connection_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);

    mut_printf(MUT_LOG_LOW, "=== ACTIVE side (%s) ===", active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP */
    passive_connection_target = (active_connection_target == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* find the object id of the lun on the active side */
    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[0], &lun0_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun0_id);

    /*
     *  Test 1: update a LUN and make sure both SPs agree on the configuration
     */
    fbe_copy_memory(&fbe_lun_wwn_update_req,
                    &monkey_king_lun_user_config_data_set[1].world_wide_name,
                    sizeof(fbe_lun_wwn_update_req));
    status = fbe_api_update_lun_wwn(lun0_id, &fbe_lun_wwn_update_req, &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== fbe_api_update_lun successfully! ===\n");

    /* find the object id of the lun on the active side */
    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[0], &lun0_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun0_id);

    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN Updated successfully! ===\n");
    monkey_king_verify_lun_info(lun0_id, &fbe_lun_wwn_update_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");


    /* make sure the passive side agrees with the configuration */
    fbe_api_sim_transport_set_target_server(passive_connection_target);

    /* find the object id of the lun on the passive side */
    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[0], &lun0_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun0_id);

    /* TODO: enable this check when dual SP are working.  verify the lun get to ready state in reasonable time */
//  status = fbe_api_wait_for_object_lifecycle_state(lun0_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
//  MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    monkey_king_verify_lun_info(lun0_id, &fbe_lun_wwn_update_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");

    /* reset active side */
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /*
     *  Test 2: Verify the change is persisted
     */

    monkey_king_stop_sep_on_node(active_connection_target);
    monkey_king_stop_sep_on_node(passive_connection_target);

    monkey_king_start_sep_on_node(FBE_TRUE, active_connection_target);

    /* find the object id of the lun */
    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[0], &lun0_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun0_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun0_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    monkey_king_verify_lun_info(lun0_id, &fbe_lun_wwn_update_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");

//    /* create second lun on active side */
//    fbe_lun_create_req.lun_number = monkey_king_lun_set[1];;
//    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
//    fbe_copy_memory(&fbe_lun_create_req.world_wide_name,
//                    &monkey_king_lun_user_config_data_set[1].world_wide_name,
//                    sizeof(fbe_lun_create_req.world_wide_name));
//    fbe_copy_memory(&fbe_lun_create_req.user_defined_name,
//                    &monkey_king_lun_user_config_data_set[1].user_defined_name,
//                    sizeof(fbe_lun_create_req.user_defined_name));
//
//    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
//    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n", fbe_lun_create_req.job_number);
//
//    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
//                     MONKEY_KING_TEST_NS_TIMEOUT,
//                     &job_error_code,
//                     &job_status);
//
//    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
//    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN creation failed");
//
//
//    /* check the Job Service error code */
//    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);
//
//    /* now lookup the object and check it's state */
//    /* find the object id of the lun*/
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[1], &lun1_id);
//    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun1_id);
//
//    /* verify the lun get to ready state in reasonable time */
//    status = fbe_api_wait_for_object_lifecycle_state(lun1_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Second LUN created successfully! ===\n");
//    monkey_king_verify_lun_info(lun1_id, &fbe_lun_create_req);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");
//
//
//    /* make sure the passive side agrees with the configuration */
//    fbe_api_sim_transport_set_target_server(passive_connection_target);
//
//    /* find the object id of the lun on the passive side */
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[1], &lun1_id);
//    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun1_id);
//
//    /* verify the lun get to ready state in reasonable time */
//    status = fbe_api_wait_for_object_lifecycle_state(lun1_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Second LUN created successfully! ===\n");
//    monkey_king_verify_lun_info(lun1_id, &fbe_lun_create_req);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");
//
//    /* get the number of LUNs in the system for verification use */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_before_create);
//    fbe_api_free_memory(lun_object_id);
//
//    /* reset active side */
//    fbe_api_sim_transport_set_target_server(active_connection_target);
//
//    /*
//     *  !@TODO: Test 3: Create a LUN with duplicate LUN number
//     */
//
//    //fbe_lun_create_req.lun_number = monkey_king_lun_set[2];
////  fbe_copy_memory(&fbe_lun_create_req.world_wide_name,
////                  &monkey_king_lun_user_config_data_set[2].world_wide_name,
////                  sizeof(fbe_lun_create_req.world_wide_name));
////  fbe_copy_memory(&fbe_lun_create_req.user_defined_name,
////                  &monkey_king_lun_user_config_data_set[2].user_defined_name,
////                  sizeof(fbe_lun_create_req.user_defined_name));
//    //
//    //status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
//    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    //mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request with dup LUN_NUM sent successfully! ===\n");
//
//    /* wait for notification */
//    //ns_context.expected_job_status = FBE_STATUS_GENERIC_FAILURE;
//    //fbe_test_sep_util_wait_job_status_notification(&ns_context);
//
//    /* check the Job Service error code */
//    //MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE, ns_context.error_code);
//
//    /* get the number of LUNs in the system and compare with before create */
//    //status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    //fbe_api_free_memory(lun_object_id);
//
//    //MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
//    //mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 2 successfully ===\n");
//
//    /*
//     *  Test 4: Try to create a LUN with a too big capacity
//     */
//
//    /* First get the rg object ID */
//    fbe_api_database_lookup_raid_group_by_number(MONKEY_KING_TEST_RAID_GROUP_ID, &rg_object_id);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
//
//    /* Now get the RG capacity */
//    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    rg_capacity = rg_info.capacity;
//
//    /* Try to create lun */
//    fbe_lun_create_req.lun_number = monkey_king_lun_set[2];
//    fbe_lun_create_req.capacity = rg_capacity*2;
//    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
//    fbe_copy_memory(&fbe_lun_create_req.world_wide_name,
//                    &monkey_king_lun_user_config_data_set[2].world_wide_name,
//                    sizeof(fbe_lun_create_req.world_wide_name));
//    fbe_copy_memory(&fbe_lun_create_req.user_defined_name,
//                    &monkey_king_lun_user_config_data_set[2].user_defined_name,
//                    sizeof(fbe_lun_create_req.user_defined_name));
//
//    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request with large capacity sent successfully! ===\n");
//
//    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
//                                         MONKEY_KING_TEST_NS_TIMEOUT,
//                     &job_error_code,
//                     &job_status);
//
//    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
//
//    /* check the Job Service error code */
//    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY, job_error_code);
//
//    /* now lookup the object and check it's state */
//    /* find the object id of the lun*/
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[2], &lun2_id);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 3 successfully ===\n");
//
//    /*
//     *  Test 5: Try to create a LUN with an invalid RG ID
//     */
//
//    fbe_lun_create_req.raid_group_id = MONKEY_KING_INVALID_TEST_RAID_GROUP_ID;
//    fbe_lun_create_req.lun_number = monkey_king_lun_set[2];
//    fbe_lun_create_req.capacity = 0x1000;
//    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
//
//    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
//    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n", fbe_lun_create_req.job_number);
//
//    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
//                     MONKEY_KING_TEST_NS_TIMEOUT,
//                     &job_error_code,
//                     &job_status);
//
//    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
//
//    /* check the Job Service error code */
//    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_INVALID_ID, job_error_code);
//
//    /* now lookup the object and check it's state */
//    /* find the object id of the lun*/
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[2], &lun2_id);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 4 successfully ===\n");
//
//    /*
//     *  Test 6: Try to create a LUN with an invalid capacity
//     */
//
//    fbe_lun_create_req.raid_group_id = MONKEY_KING_TEST_RAID_GROUP_ID;
//    fbe_lun_create_req.lun_number = monkey_king_lun_set[2];
//    fbe_lun_create_req.capacity = FBE_LBA_INVALID;
//    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
//
//    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
//    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n", fbe_lun_create_req.job_number);
//
//    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
//                     MONKEY_KING_TEST_NS_TIMEOUT,
//                     &job_error_code,
//                     &job_status);
//
//    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
//
//    /* check the Job Service error code */
//    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY, job_error_code);
//
//    /* now lookup the object and check it's state */
//    /* find the object id of the lun*/
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[2], &lun2_id);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 5 successfully ===\n");
//
//    /*
//     *  Test 7: Create a LUN with only one SP and then bring up the other SP
//     */
//
//    /* Destroy the configuration on the passive side; simulating SP failure */
//    fbe_api_sim_transport_set_target_server(passive_connection_target);
//    fbe_test_sep_util_destroy_sep_physical();
//
//    /* Reset the active SP */
//    fbe_api_sim_transport_set_target_server(active_connection_target);
//
//    /* Create a third LUN on the active SP */
//    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
//    fbe_lun_create_req.raid_group_id = MONKEY_KING_TEST_RAID_GROUP_ID;
//    fbe_lun_create_req.lun_number = monkey_king_lun_set[2];
//    fbe_lun_create_req.capacity = 0x1000;
//    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
//    fbe_lun_create_req.ndb_b = FBE_FALSE;
//    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
//    fbe_lun_create_req.addroffset = FBE_LBA_INVALID;  /* set to a valid offset for NDB */
//    fbe_copy_memory(&fbe_lun_create_req.world_wide_name,
//                    &monkey_king_lun_user_config_data_set[2].world_wide_name,
//                    sizeof(fbe_lun_create_req.world_wide_name));
//    fbe_copy_memory(&fbe_lun_create_req.user_defined_name,
//                    &monkey_king_lun_user_config_data_set[2].user_defined_name,
//                    sizeof(fbe_lun_create_req.user_defined_name));
//
//    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
//    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n", fbe_lun_create_req.job_number);
//
//    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
//                                         MONKEY_KING_TEST_NS_TIMEOUT,
//                                         &job_error_code,
//                                         &job_status);
//
//    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
//    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN creation failed");
//
//
//    /* check the Job Service error code */
//    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);
//
//    /* now lookup the object and check it's state */
//    /* find the object id of the lun*/
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[2], &lun2_id);
//    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
//
//    /* verify the lun get to ready state in reasonable time */
//    status = fbe_api_wait_for_object_lifecycle_state(lun2_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Third LUN created successfully! ===\n");
//    monkey_king_verify_lun_info(lun2_id, &fbe_lun_create_req);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");
//
//    /* Now bring back the passive SP */
//    fbe_api_sim_transport_set_target_server(passive_connection_target);
//
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
//               passive_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
//
//    /* Load the physical config on the target server
//     */
//    monkey_king_test_load_physical_config();
//
//    /* Load the SEP package on the target server
//     */
//    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Storage Extent Package(SEP) on %s ===\n",
//               passive_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
//    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
//
//    mut_printf(MUT_LOG_TEST_STATUS, "=== set SEP entries on %s ===\n",
//               passive_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
//    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
//
//    /* Make sure the new LUN is visable on the passive side too */
//
//    /* find the object id of the lun on the passive side */
//    status = fbe_api_database_lookup_lun_by_number(monkey_king_lun_set[2], &lun2_id);
//    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
//
//    /* verify the lun get to ready state in reasonable time */
//    status = fbe_api_wait_for_object_lifecycle_state(lun2_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//
//    /* get the number of LUNs in the system and compare with before create */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
//    fbe_api_free_memory(lun_object_id);
//
//    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
//    mut_printf(MUT_LOG_TEST_STATUS, "=== Third LUN created successfully! ===\n");
//    monkey_king_verify_lun_info(lun2_id, &fbe_lun_create_req);
//    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");
//
//    /* get the number of LUNs in the system for verification use */
//    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_before_create);
//    fbe_api_free_memory(lun_object_id);

//  /* Reset the active SP */
//  fbe_api_sim_transport_set_target_server(active_connection_target);
   
    return;

}/* End monkey_king_test() */


/*!****************************************************************************
 *  monkey_king_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Monkey King test. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void monkey_king_test_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Monkey King test ===\n");

    /* Set the target server
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Make sure target set correctly
     */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
               sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Load the physical config on the target server
     */
    monkey_king_test_load_physical_config();

   
    /* !@TODO: revisit the need for this sleep and refactor code here and for Test 7 accordingly */    
    //fbe_api_sleep(20000);

    /* Set the target server
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Make sure target set correctly
     */
    sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
               sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Load the physical config on the target server
     */    
    monkey_king_test_load_physical_config();

    /* Load the SEP package on both SPs
     */
    sep_config_load_sep_with_time_save(FBE_TRUE);


    /* Create a test logical configuration; will be created on active side by this function */
    monkey_king_test_create_logical_config();

    return;

} /* End monkey_king_test_setup() */


/*!****************************************************************************
 *  monkey_king_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Monkey King test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void monkey_king_test_cleanup(void)
{  
    fbe_sim_transport_connection_target_t   active_connection_target    = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   passive_connection_target   = FBE_SIM_INVALID_SERVER;
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Monkey King test ===\n");

    /* Destroy the test configuration on both SPs */
    /* Get the active SP */
    fbe_test_sep_get_active_target_id(&active_connection_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);
    /* set the passive SP */
    passive_connection_target = (active_connection_target == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup cleanup on SP %d ===\n", active_connection_target);
    fbe_api_sim_transport_set_target_server(active_connection_target);
    fbe_test_sep_util_destroy_sep_physical();

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup cleanup on SP %d ===\n", passive_connection_target);
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    fbe_test_pp_common_destroy();
    return;

} /* End monkey_king_test_cleanup() */


/*
 *  The following functions are Monkey King test functions. 
 *  They test various features of the LUN Object focusing on LUN creation (bind).
 */


/*!****************************************************************************
 *  monkey_king_test_calculate_imported_capacity
 ******************************************************************************
 *
 * @brief
 *   This is the test function for calculating the imported capacity of a LUN.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void monkey_king_test_calculate_imported_capacity(void)
{
    fbe_api_lun_calculate_capacity_info_t  capacity_info;
    fbe_status_t                           status;
    

    mut_printf(MUT_LOG_TEST_STATUS, "=== Calculate imported capacity ===\n");

    /* set a random capacity for test purpose */
    capacity_info.exported_capacity = 0x100000;

    /* calculate the imported capacity */
    status = fbe_api_lun_calculate_imported_capacity(&capacity_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== exported capacity: %llu ===\n",
	       (unsigned long long)capacity_info.exported_capacity);
    mut_printf(MUT_LOG_TEST_STATUS, "=== imported capacity: %llu ===\n",
	       (unsigned long long)capacity_info.imported_capacity);

    return;

}  /* End monkey_king_test_calculate_imported_capacity() */


/*
 * The following functions are utility functions used by this test suite
 */


/*!**************************************************************
 * monkey_king_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Monkey King test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void monkey_king_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      encl0_1_handle;
    fbe_api_terminator_device_handle_t      encl0_2_handle;
    fbe_api_terminator_device_handle_t      encl0_3_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;


    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

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
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 3, port0_handle, &encl0_3_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, MONKEY_KING_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, MONKEY_KING_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, 520, MONKEY_KING_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, 520, MONKEY_KING_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(MONKEY_KING_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == MONKEY_KING_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End monkey_king_test_load_physical_config() */


/*!**************************************************************
 * monkey_king_test_create_logical_config()
 ****************************************************************
 * @brief
 *  Create the Monkey King test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void monkey_king_test_create_logical_config(void)
{
    fbe_status_t                                        status;
    fbe_object_id_t                                     rg_object_id;
    fbe_u32_t                                           iter;
    fbe_api_job_service_raid_group_create_t             fbe_raid_group_create_req;
    fbe_job_service_error_type_t 			            job_error_type;
    fbe_status_t 					                    job_status;
    fbe_sim_transport_connection_target_t               active_connection_target;
    fbe_api_lun_create_t        	fbe_lun_create_req;
	fbe_object_id_t					lu_id;

    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_zero_memory(&fbe_raid_group_create_req, sizeof(fbe_api_job_service_raid_group_create_t));
    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Create the Logical Configuration ===\n");

    /* Determine which SP is the active side */
    fbe_test_sep_get_active_target_id(&active_connection_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);

    mut_printf(MUT_LOG_LOW, "=== ACTIVE side (%s) ===", active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Set the target server to the active side */
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");

    fbe_raid_group_create_req.b_bandwidth = 0;
    fbe_raid_group_create_req.capacity = FBE_LBA_INVALID; /* not specifying max number of LUNs for test */
    //fbe_raid_group_create_req.explicit_removal = 0;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id = MONKEY_KING_TEST_RAID_GROUP_ID;
    fbe_raid_group_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_raid_group_create_req.drive_count = 3;
    fbe_raid_group_create_req.wait_ready                = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec        = MONKEY_KING_TEST_NS_TIMEOUT*10;

    for (iter = 0; iter < 3; ++iter)    {
        fbe_raid_group_create_req.disk_array[iter].bus = monkey_king_disk_set_520[MONKEY_KING_TEST_RAID_GROUP_ID][iter][0];
        fbe_raid_group_create_req.disk_array[iter].enclosure = monkey_king_disk_set_520[MONKEY_KING_TEST_RAID_GROUP_ID][iter][1];
        fbe_raid_group_create_req.disk_array[iter].slot = monkey_king_disk_set_520[MONKEY_KING_TEST_RAID_GROUP_ID][iter][2];
    }
    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_raid group_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_raid_group_create_req.job_number);

    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_req.job_number,
					MONKEY_KING_TEST_NS_TIMEOUT,
					&job_error_type,
					&job_status,
					NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
            fbe_raid_group_create_req.raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    /* Create a LUN */
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = MONKEY_KING_TEST_RAID_GROUP_ID; 
    fbe_lun_create_req.lun_number = monkey_king_lun_set[0];
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_copy_memory(&fbe_lun_create_req.world_wide_name,
                    &monkey_king_lun_user_config_data_set[0].world_wide_name,
                    sizeof(fbe_lun_create_req.world_wide_name));
    fbe_copy_memory(&fbe_lun_create_req.user_defined_name,
                    &monkey_king_lun_user_config_data_set[0].user_defined_name,
                    sizeof(fbe_lun_create_req.user_defined_name));
    status = fbe_api_create_lun(&fbe_lun_create_req, 
                                FBE_TRUE, 
                                MONKEY_KING_TEST_NS_TIMEOUT, 
                                &lu_id,
                                &job_error_type);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End monkey_king_test_create_logical_config() */

static void monkey_king_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_assigned_wwid_t   *in_fbe_lun_update_req_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;

    lun_info.lun_object_id = in_lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INTEGER_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_lun_get_info failed!");

    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_update_req_p->world_wide_name, 
                                (char *)&lun_info.world_wide_name, 
                                sizeof(lun_info.world_wide_name),
                                "lun_get_info return mismatch WWN!");

}

static void monkey_king_start_sep_on_node(fbe_bool_t wait, fbe_sim_transport_connection_target_t target)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, " === Starting Storage Extent Package(SEP) on %d ===", target);
    fbe_api_sim_transport_set_target_server(target);

	fbe_test_util_create_registry_file();

	status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_ESP);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting Storage Extent Package(SEP) ===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set SEP entries ===\n");
    if ( wait == FBE_TRUE )
    {
        status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
    }
    else
    {
        status = fbe_api_sim_server_set_package_entries_no_wait(FBE_PACKAGE_ID_SEP_0);
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_set_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}

static void monkey_king_stop_sep_on_node(fbe_sim_transport_connection_target_t target)
{
    fbe_api_sim_transport_set_target_server(target);
    mut_printf(MUT_LOG_LOW, " === Stopping Storage Extent Package(SEP) on %d ===", target);
    fbe_test_sep_util_destroy_sep();
}

