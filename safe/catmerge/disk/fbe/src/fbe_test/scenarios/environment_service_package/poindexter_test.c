/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file poindexter_test.c
 ***************************************************************************
 *
 * @brief
 *  This file verifies the LCC firmware upgrade status in various cases. 
 *
 * @version
 *   30-Aug-2010 PHE - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"


char * poindexter_short_desc = "Test LCC firmware upgrade initiation and status reporting in various scenarios.";
char * poindexter_long_desc =
    "\n"
    "\n"
    "The curious george scenario tests the LCC firmware upgrade initiation and status reporting in various scenarios.\n"
    "It includes: \n"
    "    - The upgrade status reporting for missing registry image path;\n"
    "    - The upgrade status reporting for missing image file;\n"
    "    - The upgrade initiation triggered by LCC insertion;\n"
    "    - The upgrade status reporting for LCC removal and re-insertion during LCC upgrade;\n"
    "    - The upgrade status reporting for containing enclosure removal and re-insertion during LCC upgrade;\n"
    "    - The upgrade status reporting when the peer LCC is inserted but faulted;\n"
    "    - The upgrade status reporting when the peer LCC is not present;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 3 viper enclosures\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "    - Wait until all the upgrade initiated at power up completes.\n"
    "\n"  
    "STEP 2: Test upgrade status reporting for missing registry image path.\n"
    "    - Initiate the firmware upgrade for the LCC (0_0_0).\n"
    "    - Verify the upgrade failure reason is FAIL_REAG_REG_IMAGE_PATH.\n"
    "    - Create the registry image path.\n"
    "    - Verify the event message is logged for the LCC(0_0_0).\n"
    "\n"
    "STEP 3: Test upgrade status reporting for missing image file.\n"
    "    - Initiate the firmware upgrade for the LCC (0_0_0).\n"
    "    - Verify the upgrade failure reason is FAIL_READ_IMAGE_HEADER.\n"
    "    - Create the image file.\n"
    "\n"
    "STEP 4: Test LCC firmware upgrade by inserting the LCC.\n"
    "    - Remove LCC(0_0_0) and verify the LCC is removed.\n"
    "    - Insert LCC(0_0_0) and verify the LCC is re-inserted.\n"
    "    - Verify LCC(0_0_0) upgrade is in progress.\n"
    "    - Verify the upgrade completion status for the LCC(0_0_0).\n"
    "    - Verify the event message is logged for the LCC(0_0_0).\n"
    "\n"
    "STEP 5: Test LCC firmware upgrade by removing and reinserting the LCC.\n"
    "    - Initiate the firmware upgrade for the LCC (0_1_0).\n"
    "    - Verify LCC(0_1_0) upgrade is in progress.\n"
    "    - Remove LCC(0_1_0) and verify the PS is removed.\n"
    "    - Verify the upgrade completion status for the LCC(0_1_0).\n"
    "    - Re-inserted LCC(0_1_0) and verify the PS is re-inserted.\n"
    "    - Verify LCC(0_1_0) upgrade is in progress.\n"
    "    - Verify the upgrade completion status for the LCC(0_1_0).\n"
    "\n"
    "STEP 6: Test PS firmware upgrade by removing and reinserting the containg Enclosure.\n"
    "    - Initiate the firmware upgrade for the LCC (0_2_0).\n"
    "    - Verify LCC(0_2_0) upgrade is in progress.\n"
    "    - Remove Enclosure (0_2).\n"
    "    - Verify the upgrade completion status for the LCC(0_2_0).\n"
    "    - Reinsert the Enclosure (0_2).\n"
    "    - Verify LCC(0_2_0) upgrade is in progress.\n"
    "    - Verify the upgrade completion status for the LCC(0_2_0).\n"
    "\n"
    "STEP 6: Test upgrade status reporting when the peer LCC is inserted but faulted.\n"
    "    - Inject the fault to LCC(0_0_1) and verify the LCC is faulted.\n"
    "    - Initiate the firmware upgrade for the LCC (0_0_0).\n"
    "    - Verify the upgrade completion status for the LCC(0_0_0).\n"
    "\n"
    "STEP 7: Test upgrade status reporting when the peer LCC is not present.\n"
    "    - Remove LCC(0_0_1) and verify the PS is removed.\n"
    "    - Initiate the firmware upgrade for the LCC (0_0_0).\n"
    "    - Verify the upgrade completion status for the LCC(0_0_0).\n"
    "\n"
    "STEP 9: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the LCC firmware image file.\n"
    ;


/*!**************************************************************
 * poindexter_test_lcc_fup_missing_registry_image_path() 
 ****************************************************************
 * @brief
 *  Tests the upgrade status report when the LCC image path in the registry is missing.
 *
 * @param None.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  30-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void poindexter_test_lcc_fup_missing_registry_image_path(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_lcc_info_t                       getLccInfo = {0};
    fbe_bool_t                           isMsgPresent = FALSE;
    fbe_status_t                         status = FBE_STATUS_OK;
    char                                 deviceStr[FBE_DEVICE_STRING_LENGTH];
   
    mut_printf(MUT_LOG_LOW, "=== Test upgrade status report for missing registry image path ===");

    mut_printf(MUT_LOG_LOW, "Clear ESP event log.");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to clear ESP event log!");

    mut_printf(MUT_LOG_LOW, "Initiate upgrade for LCC(0_0_0).");

    deviceType = FBE_DEVICE_TYPE_LCC;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    status = fbe_base_env_create_device_string(deviceType, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to creat device string.");

    // As part of the device string ESP is now adding the firmware target, in this case Main
    fbe_sprintf(&deviceStr[0], 
                (FBE_DEVICE_STRING_LENGTH), 
                "%s %s", 
                &deviceStr[0],
                "Main");

    status = fbe_api_enclosure_get_lcc_info(&location, &getLccInfo);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get LCC info for its rev and subenclProdId.");

    mut_printf(MUT_LOG_LOW, "LCC(0_0_0) rev %s, subenclProdId %s", 
                            &getLccInfo.firmwareRev[0], 
                            &getLccInfo.subenclProductId[0]);

    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);


    // check for the event log message

    status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_INFO_FUP_STARTED,
                                         &deviceStr[0],
                                         &getLccInfo.firmwareRev[0],
                                         &getLccInfo.subenclProductId[0]);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to check event log msg!");

    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "Event log msg is not present!");

    mut_printf(MUT_LOG_LOW, "Event log message check passed.");

    mut_printf(MUT_LOG_LOW, "Verifying the completion status.");

    /* Verify the completion status. */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_REG_IMAGE_PATH;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);

    /* Verify the event logging. */
    status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_FUP_FAILED,
                                         &deviceStr[0],
                                         &getLccInfo.firmwareRev[0],
                                         fbe_base_env_decode_fup_completion_status(expectedCompletionStatus));

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to check event log msg!");

    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "Event log msg is not present!");

    mut_printf(MUT_LOG_LOW, "Event log message check passed.");

    return;
}


/*!**************************************************************
 * poindexter_test_lcc_fup_missing_image_file() 
 ****************************************************************
 * @brief
 *  Tests the upgrade status report when the LCC image file is missing.
 *
 * @param None.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  30-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void poindexter_test_lcc_fup_missing_image_file(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
   
    mut_printf(MUT_LOG_LOW, "=== Test upgrade status report for missing image file ===");

    mut_printf(MUT_LOG_LOW, "Initiate upgrade for LCC(0_0_0).");

    deviceType = FBE_DEVICE_TYPE_LCC;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);

    /* verify the completion status. */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_IMAGE_HEADER;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);

    return;
}


/*!**************************************************************
 * poindexter_test_lcc_fup_by_encl_removal_insertion() 
 ****************************************************************
 * @brief
 *  Tests power supply firmware upgrade by removing and re-inserting
 *  the containing enclosure.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  30-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void poindexter_test_lcc_fup_by_encl_removal_insertion(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_api_terminator_device_handle_t   terminatorPortHandle = NULL;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;
   
    deviceType = FBE_DEVICE_TYPE_LCC;
    location.bus = 0;
    location.enclosure = 2;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    mut_printf(MUT_LOG_LOW, "=== Test LCC upgrade by encl removal and insertion ===");

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);
  
    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
   
    /* Verify the upgrade is in progress.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);  

    /* Remove the containing enclosure.
     */
    //fbe_api_sleep(500);

    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_api_terminator_pull_enclosure(terminatorEnclHandle);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to remove the enclosure!");

    mut_printf(MUT_LOG_LOW, "Encl(0_%d) is removed", location.enclosure);

   /* Verifying the completion status.
    */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);  

    /* Re-insert the containing enclosure.
     */
    status = fbe_api_terminator_get_port_handle(0, &terminatorPortHandle);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to get port handle!");

    status = fbe_api_terminator_reinsert_enclosure(terminatorPortHandle, terminatorEnclHandle);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to insert the enclosure!");

    mut_printf(MUT_LOG_LOW, "Encl(0_%d) is re-inserted", location.enclosure);

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);  
   
    return;
}


/*!**************************************************************
 * poindexter_test() 
 ****************************************************************
 * @brief
 *  Main entry point for testing power supply upgrade initiation and upgrade status reporting.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void poindexter_test(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    poindexter_test_lcc_fup_missing_registry_image_path();
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);
    poindexter_test_lcc_fup_missing_image_file();

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    poindexter_test_lcc_fup_by_encl_removal_insertion();

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}

void poindexter_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void poindexter_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file poindexter_test.c
 *************************/

