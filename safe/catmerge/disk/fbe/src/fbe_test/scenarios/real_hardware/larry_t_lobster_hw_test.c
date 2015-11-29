
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file larry_t_lobster_hw_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for binding new LUNs.
 *
 * @version
 *   12/15/2009 - Created. Susan Rundbaken (rundbs)
 *   12/02/2010 - Migrated. Bo Gao
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_user_server.h"
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

char* larry_t_lobster_hw_short_desc = "LUN Bind operation";
char* larry_t_lobster_hw_long_desc =
    "\n"
    "\n"
    "The Larry T Lobster scenario tests binding new LUNs to an already\n"
    "existing RAID Group.\n"
    "\n"
    "\n"
    "*** Larry T Lobster Phase 1 **************************************************************\n"
    "\n"    
    "Phase 1 covers the following cases:\n"
    "\n"
    "   - LUN bind, start and complete\n"
    "   - LUN change parameters\n"
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
    "    [SEP] 1 Virtual RAID object (VR)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - For each of the drives:(PDO1-PDO2-PDO3)\n"
    "        - Create a virtual drive (VD) with an I/O edge to PVD\n"
    "        - Verify that PVD and VD are both in the READY state\n"
    "    - Create a RAID object(RO1) and attach edges to the virtual drives\n"
    "\n" 
    "STEP 2: Send a LUN Bind request using fbe_api\n"
    "    - Verify that JCS gets request and puts it on its creation queue for processing\n"
    "    - Verify that JCS processes request and new LUN Object is created successfully by\n"
    "      verifying the following:\n"
    "       - LUN Object in the READY state\n" 
    "       - Config Service has record of object in its tables:\n" 
    "           - LUN ID\n" 
    "           - FBE Object ID\n"
    "           - LUN name\n"
    "           - RG ID\n"
    "           - LUN count\n"
    "           - imported capacity\n"
    "   - Test with aligned and non-aligned capacity as input\n"
    "\n"
    "STEP 3: Change LUN parameters using fbe_api\n"
    "    - The LUN Name is currently the only parameter that can change\n"
    "    - Verify that JCS gets request and puts it on its creation queue for processing\n"
    "    - Verify that JCS processes request and new LUN Object is changed successfully by\n"
    "      verifying the following:\n"
    "       - LUN Object in the READY state\n" 
    "       - Config Service has record of object in its tables:\n" 
    "           - LUN ID\n" 
    "           - FBE Object ID\n"
    "           - New LUN Name\n"
    "           - RG ID\n"
    "           - imported capacity\n"
    "\n"
    "*** Larry T Lobster Post-Phase 1 ********************************************************* \n"
    "\n"
    "The Larry T Lobster Scenario also needs to cover the following cases in follow-on phases:\n"
    "    - Verify that multiple LUNs can be successfully created using first-fit option\n" 
    "    - Verify that multiple LUNs can be successfully created using best-fit option\n"   
    "    - Verify that LUN Object metadata is recorded on both SPs in memory and persistently\n"
    "      for both binding a new LUN and changing LUN parameters\n"
    "    - Verify that appropriate error handling is performed for the following cases:\n" 
    "      - invalid RG ID\n" 
    "      - invalid LUN capacity\n" 
    "      - not enough space in RG\n"
    "      - Config Service cannot update LUN Object metadata in memory\n" 
    "      - Config Service cannot update LUN Object metadata persistently\n"   
    "      - Config Service cannot update LUN Object metadata on peer\n" 
    "\n";        
    

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/
typedef struct larry_t_lobster_lun_user_config_data_s{
    /*! World-Wide Name associated with the LUN
     */
    fbe_assigned_wwid_t      world_wide_name;

    /*! User-Defined Name for the LUN
     */
    fbe_user_defined_lun_name_t          user_defined_name;
}larry_t_lobster_lun_user_config_data_t;

/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_RAID_GROUP_COUNT           1


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY     (0x1bee6000  /* fbe_private_space_get_minimum_system_drive_size() + 0x40000*/)


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_RAID_GROUP_ID          0

#define LARRY_T_LOBSTER_INVALID_TEST_RAID_GROUP_ID  5


/*!*******************************************************************
 * @var larry_t_lobster_disk_set_520
 *********************************************************************
 * @brief This is the disk set for the 520 RG (520 byte per block SAS)
 *
 *********************************************************************/
static fbe_u32_t larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_COUNT][LARRY_T_LOBSTER_TEST_MAX_RG_WIDTH][3] = 
{
    /* width = 3 */
    { {0,0,8}, {0,0,9}, {0,0,10}}
};


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_LUN_COUNT        3


/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_LUN_NUMBER
 *********************************************************************
 * @brief  Lun number used by this test
 *
 *********************************************************************/
static fbe_lun_number_t larry_t_lobster_lun_set[LARRY_T_LOBSTER_TEST_LUN_COUNT] = {123, 234, 456};

/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_LUN_USER_CONFIG_DATA
 *********************************************************************
 * @brief  Lun wwn and user_defined_name used by this test
 *
 *********************************************************************/
static larry_t_lobster_lun_user_config_data_t larry_t_lobster_lun_user_config_data_set[LARRY_T_LOBSTER_TEST_LUN_COUNT] = 
{   {{01,02,03,04,05,06,07,00},{0xD3,0xE4,0xF0,0x00}},
    {{01,02,03,04,05,06,07,01},{0xD3,0xE4,0xF1,0x00}},
    {{01,02,03,04,05,06,07,02},{0xD3,0xE4,0xF2,0x00}},
};

/*!*******************************************************************
 * @def LARRY_T_LOBSTER_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define LARRY_T_LOBSTER_TEST_NS_TIMEOUT        60000 /*wait maximum of 5 seconds*/


/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
*/

static void larry_t_lobster_hw_test_load_physical_config(void);
static void larry_t_lobster_hw_test_create_logical_config(void);
static void larry_t_lobster_hw_test_calculate_imported_capacity(void);
static void larry_t_lobster_hw_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_job_service_lun_create_t  *in_fbe_lun_create_req_p);


/*-----------------------------------------------------------------------------
    FUNCTIONS:
*/

/*!****************************************************************************
 *  larry_t_lobster_hw_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Larry T Lobster test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_hw_test(void)
{
    fbe_status_t                        status;
    fbe_api_job_service_lun_create_t    fbe_lun_create_req;
    fbe_object_id_t                     lun0_id, lun1_id, lun2_id;
    fbe_object_id_t                     *lun_object_id;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_object_id_t                     rg_object_id;
    fbe_u32_t                           lun_count_before_create, lun_count_after_create;
    fbe_lba_t                           rg_capacity;
    fbe_job_service_error_type_t    job_error_code;
    fbe_status_t            job_status;
    fbe_sim_transport_connection_target_t   active_connection_target    = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   passive_connection_target   = FBE_SIM_INVALID_SERVER;


    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Larry T Lobster Test ===\n");
    
    /* get the number of LUNs in the system for verification use */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_before_create);
    fbe_api_free_memory(lun_object_id);

    /* Get the active SP */
    fbe_test_sep_get_active_target_id(&active_connection_target);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);

    mut_printf(MUT_LOG_LOW, "=== ACTIVE side (%s) ===", active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP */
    passive_connection_target = (active_connection_target == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* 
     *  Test 1: Create a LUN and make sure both SPs agree on the configuration
     */

    /* create first lun on active side */
    fbe_api_sim_transport_set_target_server(active_connection_target);
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = LARRY_T_LOBSTER_TEST_RAID_GROUP_ID; 
    fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[0];
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID;  /* set to a valid offset for NDB */
    fbe_lun_create_req.wait_ready = FBE_FALSE;
    fbe_lun_create_req.ready_timeout_msec = LARRY_T_LOBSTER_TEST_NS_TIMEOUT;
    fbe_copy_memory(&fbe_lun_create_req.world_wide_name, 
                    &larry_t_lobster_lun_user_config_data_set[0].world_wide_name,
                    sizeof(fbe_lun_create_req.world_wide_name));
    fbe_copy_memory(&fbe_lun_create_req.user_defined_name, 
                    &larry_t_lobster_lun_user_config_data_set[0].user_defined_name,
                    sizeof(fbe_lun_create_req.user_defined_name));

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_lun_create_req.job_number);

    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
                                         LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                     &job_error_code,
                     &job_status,
                     NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created LUN");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN creation failed");

    /* check the Job Service error code */
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);

    /* find the object id of the lun on the active side */
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[0], &lun0_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun0_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun0_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== First LUN created successfully! ===\n");
    larry_t_lobster_hw_verify_lun_info(lun0_id, &fbe_lun_create_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");


    /* make sure the passive side agrees with the configuration */
    fbe_api_sim_transport_set_target_server(passive_connection_target);

    /* find the object id of the lun on the passive side */
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[0], &lun0_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun0_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun0_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== First LUN created successfully! ===\n");
    larry_t_lobster_hw_verify_lun_info(lun0_id, &fbe_lun_create_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");

    /* get the number of LUNs in the system for verification use */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_before_create);
    fbe_api_free_memory(lun_object_id);

    /* reset active side */
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /* 
     *  Test 2: Create another LUN 
     */

    /* create second lun on active side */
    fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[1];;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_copy_memory(&fbe_lun_create_req.world_wide_name, 
                    &larry_t_lobster_lun_user_config_data_set[1].world_wide_name,
                    sizeof(fbe_lun_create_req.world_wide_name));
    fbe_copy_memory(&fbe_lun_create_req.user_defined_name, 
                    &larry_t_lobster_lun_user_config_data_set[1].user_defined_name,
                    sizeof(fbe_lun_create_req.user_defined_name));

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_lun_create_req.job_number);

    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
                     LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                     &job_error_code,
                     &job_status,
                     NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN creation failed");


    /* check the Job Service error code */
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);

    /* now lookup the object and check it's state */
    /* find the object id of the lun*/
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[1], &lun1_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun1_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun1_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Second LUN created successfully! ===\n");
    larry_t_lobster_hw_verify_lun_info(lun1_id, &fbe_lun_create_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");


    /* make sure the passive side agrees with the configuration */
    fbe_api_sim_transport_set_target_server(passive_connection_target);

    /* find the object id of the lun on the passive side */
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[1], &lun1_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun1_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun1_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Second LUN created successfully! ===\n");
    larry_t_lobster_hw_verify_lun_info(lun1_id, &fbe_lun_create_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");

    /* get the number of LUNs in the system for verification use */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_before_create);
    fbe_api_free_memory(lun_object_id);

    /* reset active side */
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /* 
     *  !@TODO: Test 3: Create a LUN with duplicate LUN number
     */

    //fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[2];
//  fbe_copy_memory(&fbe_lun_create_req.world_wide_name,
//                  &larry_t_lobster_lun_user_config_data_set[2].world_wide_name,
//                  sizeof(fbe_lun_create_req.world_wide_name));
//  fbe_copy_memory(&fbe_lun_create_req.user_defined_name,
//                  &larry_t_lobster_lun_user_config_data_set[2].user_defined_name,
//                  sizeof(fbe_lun_create_req.user_defined_name));
    // 
    //status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    //mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request with dup LUN_NUM sent successfully! ===\n");

    /* wait for notification */
    //ns_context.expected_job_status = FBE_STATUS_GENERIC_FAILURE;
    //fbe_test_sep_util_wait_job_status_notification(&ns_context);

    /* check the Job Service error code */
    //MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE, ns_context.error_code);

    /* get the number of LUNs in the system and compare with before create */
    //status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    //fbe_api_free_memory(lun_object_id);

    //MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
    //mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 2 successfully ===\n");

    /* 
     *  Test 4: Try to create a LUN with a too big capacity
     */

    /* First get the rg object ID */
    fbe_api_database_lookup_raid_group_by_number(LARRY_T_LOBSTER_TEST_RAID_GROUP_ID, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Now get the RG capacity */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    rg_capacity = rg_info.capacity;

    /* Try to create lun */
    fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[2];
    fbe_lun_create_req.capacity = rg_capacity*2;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_copy_memory(&fbe_lun_create_req.world_wide_name, 
                    &larry_t_lobster_lun_user_config_data_set[2].world_wide_name,
                    sizeof(fbe_lun_create_req.world_wide_name));
    fbe_copy_memory(&fbe_lun_create_req.user_defined_name, 
                    &larry_t_lobster_lun_user_config_data_set[2].user_defined_name,
                    sizeof(fbe_lun_create_req.user_defined_name));

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request with large capacity sent successfully! ===\n");

    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
                                         LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                     &job_error_code,
                     &job_status,
                     NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
    
    /* check the Job Service error code */
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY, job_error_code);

    /* now lookup the object and check it's state */
    /* find the object id of the lun*/
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[2], &lun2_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 3 successfully ===\n");

    /* 
     *  Test 5: Try to create a LUN with an invalid RG ID
     */

    fbe_lun_create_req.raid_group_id = LARRY_T_LOBSTER_INVALID_TEST_RAID_GROUP_ID; 
    fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[2];
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_lun_create_req.job_number);

    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
                     LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                     &job_error_code,
                     &job_status,
                     NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");

    /* check the Job Service error code */
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_INVALID_ID, job_error_code);

    /* now lookup the object and check it's state */
    /* find the object id of the lun*/
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[2], &lun2_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 4 successfully ===\n");

    /* 
     *  Test 6: Try to create a LUN with an invalid capacity
     */

    fbe_lun_create_req.raid_group_id = LARRY_T_LOBSTER_TEST_RAID_GROUP_ID; 
    fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[2];
    fbe_lun_create_req.capacity = FBE_LBA_INVALID;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_lun_create_req.job_number);

    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
                     LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                     &job_error_code,
                     &job_status,
                     NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");

    /* check the Job Service error code */
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY, job_error_code);

    /* now lookup the object and check it's state */
    /* find the object id of the lun*/
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[2], &lun2_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);
    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== System rejected creation of LUN 5 successfully ===\n");

    /* 
     *  Test 7: Create a LUN with only one SP and then bring up the other SP 
     */

    /* Destroy the configuration on the passive side; simulating SP failure */
    fbe_api_sim_transport_set_target_server(passive_connection_target);
    fbe_test_sep_util_destroy_sep_physical();

    /* Reset the active SP */
    fbe_api_sim_transport_set_target_server(active_connection_target);

    /* Create a third LUN on the active SP */
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = LARRY_T_LOBSTER_TEST_RAID_GROUP_ID; 
    fbe_lun_create_req.lun_number = larry_t_lobster_lun_set[2];
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID;  /* set to a valid offset for NDB */
    fbe_copy_memory(&fbe_lun_create_req.world_wide_name, 
                    &larry_t_lobster_lun_user_config_data_set[2].world_wide_name,
                    sizeof(fbe_lun_create_req.world_wide_name));
    fbe_copy_memory(&fbe_lun_create_req.user_defined_name, 
                    &larry_t_lobster_lun_user_config_data_set[2].user_defined_name,
                    sizeof(fbe_lun_create_req.user_defined_name));

    status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_lun_create_req.job_number);

    status = fbe_api_common_wait_for_job(fbe_lun_create_req.job_number,
                                         LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create LUN");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN creation failed");


    /* check the Job Service error code */
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);

    /* now lookup the object and check it's state */
    /* find the object id of the lun*/
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[2], &lun2_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun2_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Third LUN created successfully! ===\n");
    larry_t_lobster_hw_verify_lun_info(lun2_id, &fbe_lun_create_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");

    /* Now bring back the passive SP */
    fbe_api_sim_transport_set_target_server(passive_connection_target);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
               passive_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* Load the physical config on the target server
     */
    larry_t_lobster_hw_test_load_physical_config();

    /* Load the SEP package on the target server
     */
//    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Storage Extent Package(SEP) on %s ===\n",
//               passive_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
//    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
//
//    mut_printf(MUT_LOG_TEST_STATUS, "=== set SEP entries on %s ===\n",
//               passive_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
//    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);

    /* Make sure the new LUN is visable on the passive side too */

    /* find the object id of the lun on the passive side */
    status = fbe_api_database_lookup_lun_by_number(larry_t_lobster_lun_set[2], &lun2_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun2_id);

    /* verify the lun get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(lun2_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the number of LUNs in the system and compare with before create */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_after_create);
    fbe_api_free_memory(lun_object_id);

    MUT_ASSERT_INT_EQUAL_MSG(lun_count_after_create, lun_count_before_create+1, "Found unexpected number of LUNs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Third LUN created successfully! ===\n");
    larry_t_lobster_hw_verify_lun_info(lun2_id, &fbe_lun_create_req);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN info verified successfully! ===\n");

    /* get the number of LUNs in the system for verification use */
    status = fbe_api_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &lun_object_id, &lun_count_before_create);
    fbe_api_free_memory(lun_object_id);

    /* Reset the active SP */
    fbe_api_sim_transport_set_target_server(active_connection_target);
   
    return;

}/* End larry_t_lobster_hw_test() */


/*!****************************************************************************
 *  larry_t_lobster_hw_test_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Larry T Lobster test. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_hw_test_setup(void)
{
//    fbe_sim_transport_connection_target_t   sp;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Larry T Lobster test ===\n");

    /* set control entry for drivers on SP server side */
    status = fbe_api_user_server_set_driver_entries();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* register notification elements on SP server side */
    status = fbe_api_user_server_register_notifications();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Create a test logical database; will be created on active side by this function */
    larry_t_lobster_hw_test_create_logical_config();

    return;

} /* End larry_t_lobster_hw_test_setup() */


/*!****************************************************************************
 *  larry_t_lobster_hw_test_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Larry T Lobster test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void larry_t_lobster_hw_test_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Larry T Lobster test ===\n");

    /* Destroy the test configuration on both SPs */
//  fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
//  fbe_test_sep_util_destroy_sep_physical();
//
//  fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
//  fbe_test_sep_util_destroy_sep_physical();

    return;

} /* End larry_t_lobster_hw_test_cleanup() */


/*
 *  The following functions are Larry T Lobster test functions. 
 *  They test various features of the LUN Object focusing on LUN creation (bind).
 */


/*!****************************************************************************
 *  larry_t_lobster_hw_test_calculate_imported_capacity
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
static void larry_t_lobster_hw_test_calculate_imported_capacity(void)
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

}  /* End larry_t_lobster_hw_test_calculate_imported_capacity() */


/*
 * The following functions are utility functions used by this test suite
 */


/*!**************************************************************
 * larry_t_lobster_hw_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Larry T Lobster test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void larry_t_lobster_hw_test_load_physical_config(void)
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
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, 4160, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, 4160, LARRY_T_LOBSTER_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == LARRY_T_LOBSTER_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End larry_t_lobster_hw_test_load_physical_config() */


/*!**************************************************************
 * larry_t_lobster_hw_test_create_logical_config()
 ****************************************************************
 * @brief
 *  Create the Larry T Lobster test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void larry_t_lobster_hw_test_create_logical_config(void)
{
    fbe_status_t                                        status;
    fbe_object_id_t                                     rg_object_id;
    fbe_u32_t                                           iter;
    fbe_api_job_service_raid_group_create_t             fbe_raid_group_create_req;
    fbe_job_service_error_type_t                        job_error_code;
    fbe_status_t                                        job_status;
    fbe_sim_transport_connection_target_t               active_connection_target = FBE_SIM_SP_A;

    
    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;
    fbe_zero_memory(&fbe_raid_group_create_req, sizeof(fbe_api_job_service_raid_group_create_t));

    mut_printf(MUT_LOG_TEST_STATUS, "=== Create the Logical Configuration ===\n");

    /* Determine which SP is the active side */
//    fbe_test_sep_get_active_target_id(&active_connection_target);
//    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_connection_target);

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
    fbe_raid_group_create_req.raid_group_id = LARRY_T_LOBSTER_TEST_RAID_GROUP_ID;
    fbe_raid_group_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_raid_group_create_req.drive_count = 3;
    fbe_raid_group_create_req.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec    = LARRY_T_LOBSTER_TEST_NS_TIMEOUT;
    for (iter = 0; iter < 3; ++iter)    {
        fbe_raid_group_create_req.disk_array[iter].bus = larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_ID][iter][0];
        fbe_raid_group_create_req.disk_array[iter].enclosure = larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_ID][iter][1];
        fbe_raid_group_create_req.disk_array[iter].slot = larry_t_lobster_disk_set_520[LARRY_T_LOBSTER_TEST_RAID_GROUP_ID][iter][2];
    }
    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_raid group_create request sent successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX\n",
	       (unsigned long long)fbe_raid_group_create_req.job_number);

    EmcutilSleep(2000);

    /* wait for notification */
  status = fbe_api_common_wait_for_job(fbe_raid_group_create_req.job_number,
                LARRY_T_LOBSTER_TEST_NS_TIMEOUT,
                &job_error_code,
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
    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End larry_t_lobster_hw_test_create_logical_config() */

static void larry_t_lobster_hw_verify_lun_info(fbe_object_id_t in_lun_object_id, fbe_api_job_service_lun_create_t  *in_fbe_lun_create_req_p)
{
    fbe_status_t status;
    fbe_database_lun_info_t lun_info;
    fbe_object_id_t rg_obj_id;

    lun_info.lun_object_id = in_lun_object_id;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INTEGER_EQUAL_MSG(status, FBE_STATUS_OK, "fbe_api_lun_get_info failed!");

    /* check rd group obj_id*/
    status = fbe_api_database_lookup_raid_group_by_number(in_fbe_lun_create_req_p->raid_group_id, &rg_obj_id);
    MUT_ASSERT_INTEGER_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INTEGER_EQUAL_MSG(rg_obj_id, lun_info.raid_group_obj_id, "Lun_get_info return mismatch rg_obj_id");

    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p->capacity, lun_info.capacity, "Lun_get_info return mismatch capacity" );
    MUT_ASSERT_INTEGER_EQUAL_MSG(in_fbe_lun_create_req_p->raid_type, lun_info.raid_info.raid_type, "Lun_get_info return mismatch raid_type" );
    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_create_req_p->world_wide_name, 
                                (char *)&lun_info.world_wide_name, 
                                sizeof(lun_info.world_wide_name),
                                "lun_get_info return mismatch WWN!");

    MUT_ASSERT_BUFFER_EQUAL_MSG((char *)&in_fbe_lun_create_req_p->user_defined_name, 
                                (char *)&lun_info.user_defined_name, 
                                sizeof(lun_info.user_defined_name),
                                "lun_get_info return mismatch user_defined_name!");

}
