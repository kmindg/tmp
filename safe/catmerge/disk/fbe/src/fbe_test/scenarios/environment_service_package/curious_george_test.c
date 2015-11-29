/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file curious_george_test.c
 ***************************************************************************
 *
 * @brief
 *  This file verifies the PS firmware upgrade status in various cases. 
 *
 * @version
 *   05-Aug-2010 PHE - Created.
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
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * curious_george_short_desc = "Test PS firmware upgrade initiation and status reporting in various scenarios.";
char * curious_george_long_desc =
    "\n"
    "\n"
    "The curious george scenario verifies the PS firmware upgrade status reporting in various scenarios.\n"
    "It includes: \n"
    "    - The upgrade status reporting for missing registry image path;\n"
    "    - The upgrade status reporting for missing image file;\n"
    "    - The upgrade initiation triggered by PS insertion;\n"
    "    - The upgrade status reporting for PS removal and re-insertion during PS upgrade;\n"
    "    - The upgrade status reporting for containing enclosure removal and re-insertion during PS upgrade;\n"
    "    - The upgrade status reporting when the peer PS is inserted but faulted;\n"
    "    - The upgrade status reporting when the peer PS is not present;\n"
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
    "    - Initiate the firmware upgrade for the Power Supply (0_0_0).\n"
    "    - Verify the upgrade failure reason is FAIL_REAG_REG_IMAGE_PATH.\n"
    "    - Create the registry image path.\n"
    "    - Verify the event message is logged for the PS(0_0_0).\n"
    "\n"
    "STEP 3: Test upgrade status reporting for missing image file.\n"
    "    - Initiate the firmware upgrade for the Power Supply (0_0_0).\n"
    "    - Verify the upgrade failure reason is FAIL_READ_IMAGE_HEADER.\n"
    "    - Create the image file.\n"
    "\n"
    "STEP 4: Test PS firmware upgrade by inserting the PS.\n"
    "    - Remove PS(0_0_0) and verify the PS is removed.\n"
    "    - Insert PS(0_0_0) and verify the PS is re-inserted.\n"
    "    - Verify PS(0_0_0) upgrade is in progress.\n"
    "    - Verify the upgrade completion status for the PS(0_0_0).\n"
    "    - Verify the event message is logged for the PS(0_0_0).\n"
    "\n"
    "STEP 5: Test PS firmware upgrade by removing and reinserting the PS.\n"
    "    - Initiate the firmware upgrade for the Power Supply (0_1_0).\n"
    "    - Verify PS(0_1_0) upgrade is in progress.\n"
    "    - Remove PS(0_1_0) and verify the PS is removed.\n"
    "    - Verify the upgrade completion status for the PS(0_1_0).\n"
    "    - Re-inserted PS(0_1_0) and verify the PS is re-inserted.\n"
    "    - Verify PS(0_1_0) upgrade is in progress.\n"
    "    - Verify the upgrade completion status for the PS(0_1_0).\n"
    "\n"
    "STEP 6: Test PS firmware upgrade by removing and reinserting the containg Enclosure.\n"
    "    - Initiate the firmware upgrade for the Power Supply (0_2_0).\n"
    "    - Verify PS(0_2_0) upgrade is in progress.\n"
    "    - Remove Enclosure (0_2).\n"
    "    - Verify the upgrade completion status for the PS(0_2_0).\n"
    "    - Reinsert the Enclosure (0_2).\n"
    "    - Verify PS(0_2_0) upgrade is in progress.\n"
    "    - Verify the upgrade completion status for the PS(0_2_0).\n"
    "\n"
    "STEP 6: Test upgrade status reporting when the peer PS is inserted but faulted.\n"
    "    - Inject the fault to PS(0_0_1) and verify the PS is faulted.\n"
    "    - Initiate the firmware upgrade for the Power Supply (0_0_0).\n"
    "    - Verify the upgrade completion status for the PS(0_0_0).\n"
    "\n"
    "STEP 7: Test upgrade status reporting when the peer PS is not present.\n"
    "    - Remove PS(0_0_1) and verify the PS is removed.\n"
    "    - Initiate the firmware upgrade for the Power Supply (0_0_0).\n"
    "    - Verify the upgrade completion status for the PS(0_0_0).\n"
    "\n"
    "STEP 9: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the power supply firmware image file.\n"
    ;


/*!**************************************************************
 * curious_george_test_ps_fup_missing_registry_image_path() 
 ****************************************************************
 * @brief
 *  Tests the upgrade status report when the PS image path in the registry is missing.
 *
 * @param None.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_missing_registry_image_path(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_esp_ps_mgmt_ps_info_t            getPsInfo = {0};
    fbe_bool_t                           isMsgPresent = FALSE;
    fbe_status_t                         status = FBE_STATUS_OK;
    char                                 deviceStr[FBE_DEVICE_STRING_LENGTH];
   
    mut_printf(MUT_LOG_LOW, "=== Test upgrade status report for missing registry image path ===");

    mut_printf(MUT_LOG_LOW, "Clear ESP event log.");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to clear ESP event log!");

    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    status = fbe_base_env_create_device_string(deviceType, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to creat device string.");

    status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get PS info for its rev and subenclProdId.");

    mut_printf(MUT_LOG_LOW, "PS(0_0_0) rev %s, subenclProdId %s", 
                            &getPsInfo.psInfo.firmwareRev[0], 
                            &getPsInfo.psInfo.subenclProductId[0]);

    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
    
    /* Check event logging.
     */
    status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_INFO_FUP_STARTED,
                                         &deviceStr[0],
                                         &getPsInfo.psInfo.firmwareRev[0],
                                         &getPsInfo.psInfo.subenclProductId[0]);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to check event log msg!");

    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "Event log msg is not present!");

    mut_printf(MUT_LOG_LOW, "Event log message check passed.");

    /* Verify the completion status. 
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_REG_IMAGE_PATH;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);

    /* Check event logging
     */
    status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_FUP_FAILED,
                                         &deviceStr[0],
                                         &getPsInfo.psInfo.firmwareRev[0],
                                         fbe_base_env_decode_fup_completion_status(expectedCompletionStatus));

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to check event log msg!");

    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "Event log msg is not present!");

    mut_printf(MUT_LOG_LOW, "Event log message check passed.");

    return;
}


/*!**************************************************************
 * curious_george_test_ps_fup_missing_image_file() 
 ****************************************************************
 * @brief
 *  Tests the upgrade status report when the PS image file is missing.
 *
 * @param None.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_missing_image_file(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
   
    mut_printf(MUT_LOG_LOW, "=== Test upgrade status report for missing image file ===");

    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);

    /* Verify the completion status. 
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_IMAGE_HEADER;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);

    return;
}

/*!**************************************************************
 * curious_george_test_complete_lcc_upgrade_before_ps() 
 ****************************************************************
 * @brief
 *  This functions initiates the LCC upgrade for all the enclosures
 *  so that the PS upgrade would not be affected by the priority check.
 *  Because the LCC upgrade has the higher priority than the PS upgrade.
 *  
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  26-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_complete_lcc_upgrade_before_ps(void)
{
    fbe_u32_t                      enclosure = 0;
    fbe_device_physical_location_t location = {0};
    fbe_u32_t                      forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;

    location.bus = 0;
    location.slot = 0;

    for(enclosure = 0; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;

        forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE;

        fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

        /* Initiate LCC firmware upgrade.
         */
        fbe_test_esp_initiate_upgrade(FBE_DEVICE_TYPE_LCC, &location, forceFlags);

        /* Verify the completion status for all the device types.
         */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
        fbe_test_esp_verify_fup_completion_status(FBE_DEVICE_TYPE_LCC, &location, expectedCompletionStatus);
    }

    return;
}

/*!**************************************************************
 * curious_george_test_ps_fup_by_ps_insertion() 
 ****************************************************************
 * @brief
 *  Tests the power supply firmware upgrade when the power supply gets inserted.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_by_ps_insertion(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    ses_stat_elem_ps_struct              psStruct;
    fbe_esp_ps_mgmt_ps_info_t            getPsInfo = {0};
    fbe_status_t                         status = FBE_STATUS_OK;
    char                                 deviceStr[FBE_DEVICE_STRING_LENGTH];

    mut_printf(MUT_LOG_LOW, "=== Test PS upgrade triggered by PS insertion ===");

    mut_printf(MUT_LOG_LOW, "Clear ESP event log.");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to clear ESP event log!");

    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;

    status = fbe_base_env_create_device_string(deviceType, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to creat device string.");

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_ps_eses_status(terminatorEnclHandle, 
                                                   PS_0, 
                                                   &psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the power supply.
     */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_0, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS removal failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_%d_0) is removed.", location.enclosure);

    /* Insert the power supply.
     */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_0, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS insertion failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_%d_0) is inserted.", location.enclosure);

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);  

    status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get PS info for its rev and subenclProdId.");

    mut_printf(MUT_LOG_LOW, "PS(0_0_0) rev %s, subenclProdId %s", 
                            &getPsInfo.psInfo.firmwareRev[0], 
                            &getPsInfo.psInfo.subenclProductId[0]);
    return;
}


/*!**************************************************************
 * curious_george_test_ps_fup_by_ps_removal_insertion() 
 ****************************************************************
 * @brief
 *  Tests the power supply firmware upgrade status
 *  when the power supply gets removed during the upgrade.
 *  It also tests the upgrade status right after the power supply gets inserted back.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_by_ps_removal_insertion(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    ses_stat_elem_ps_struct              psStruct;
    fbe_status_t                         status = FBE_STATUS_OK;
    ses_ver_desc_struct                  ps_ver_desc;

    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    mut_printf(MUT_LOG_LOW, "=== Test PS upgrade by PS removal and insertion ===");

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_ps_eses_status(terminatorEnclHandle, 
                                                   PS_0, 
                                                   &psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Initiate power supply firmware upgrade.
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
   
    /* Verify the upgrade is in progress.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 

    /* Remove the power supply.
     */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_0, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&location, FALSE /* Removed */, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS removal failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_%d_0) is removed.", location.enclosure);

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 


    /* 
     * Change the revision of the removed PS in config page, in case the
     * previous upgrade already sent an activate. IF an activate is sent earlier,
     * then inserted PS will have the same revision as the PS image file and so
     * no new upgrade will be initiated once the PS is inserted.
     */
    status = fbe_api_terminator_eses_get_ver_desc(terminatorEnclHandle,
                                                  SES_SUBENCL_TYPE_PS,
                                                  FBE_ESES_SUBENCL_SIDE_ID_A,
                                                  SES_COMP_TYPE_PS_FW,
                                                  &ps_ver_desc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

//    ps_ver_desc.comp_rev_level[4] -= 2;
    ps_ver_desc.cdes_rev.cdes_2_0_rev.comp_rev_level[8] -= 2;

    status = fbe_api_terminator_eses_set_ver_desc(terminatorEnclHandle,
                                                  SES_SUBENCL_TYPE_PS,
                                                  FBE_ESES_SUBENCL_SIDE_ID_A,
                                                  SES_COMP_TYPE_PS_FW,
                                                  ps_ver_desc);  
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
      


    /* Re-insert the power supply.
     */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_0, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&location, TRUE /* Inserted */, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for PS insertion failed!");

    mut_printf(MUT_LOG_LOW, "PS(0_%d_0) is re-inserted", location.enclosure);

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 
  
    return;
}

/*!**************************************************************
 * curious_george_test_ps_fup_by_encl_removal_insertion() 
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
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_by_encl_removal_insertion(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_api_terminator_device_handle_t   terminatorPortHandle = NULL;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;
   
    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 2;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    mut_printf(MUT_LOG_LOW, "=== Test PS upgrade by encl removal and insertion ===");

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Initiate power supply firmware upgrade.
     */
    status = fbe_api_esp_common_initiate_upgrade(deviceType, &location, forceFlags);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to initiate the upgrade!");

    mut_printf(MUT_LOG_LOW, "PS(0_%d_0) upgrade is initiated", location.enclosure);
   
    /* Verify the upgrade is in progress.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 

    /* Remove the containing enclosure.
     */
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
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 

    return;
}

/*!**************************************************************
 * curious_george_test_ps_fup_peer_ps_not_present() 
 ****************************************************************
 * @brief
 *  Tests power supply firmware upgrade status when the peer PS is not present.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_peer_ps_not_present(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_device_physical_location_t       peerLocation = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    ses_stat_elem_ps_struct              psStruct;
    fbe_status_t                         status = FBE_STATUS_OK;
   
    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    peerLocation.bus = 0;
    peerLocation.enclosure = 0;
    peerLocation.slot = 1;

    mut_printf(MUT_LOG_LOW, "=== Test PS upgrade status when peer PS is not present ===");
  
    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the present eses status before inserting the fault to the peer power supply.
     */    
    status = fbe_api_terminator_get_ps_eses_status(terminatorEnclHandle, 
                                                   PS_1, 
                                                   &psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the peer power supply.
     */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_1, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&peerLocation, FALSE, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for peer PS to remove failed!");
 
    mut_printf(MUT_LOG_LOW, "Peer PS(0_%d_1) is removed.\n", peerLocation.enclosure);

    /* Initiate power supply firmware upgrade.
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 

    /* Re-insert the peer power supply.
    */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_1, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&peerLocation, TRUE, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for peer PS to re-insert failed!");
 
    mut_printf(MUT_LOG_LOW, "Peer PS(0_%d_1) is re-inserted.\n", peerLocation.enclosure);

    return;
}


/*!**************************************************************
 * curious_george_test_ps_fup_peer_ps_faulted() 
 ****************************************************************
 * @brief
 *  Tests power supply firmware upgrade status when the peer PS is faulted.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  08-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
void curious_george_test_ps_fup_peer_ps_faulted(void)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_device_physical_location_t       peerLocation = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    ses_stat_elem_ps_struct              psStruct;
    fbe_status_t                         status = FBE_STATUS_OK;
   
    deviceType = FBE_DEVICE_TYPE_PS;
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    peerLocation.bus = 0;
    peerLocation.enclosure = 1;
    peerLocation.slot = 1;

    mut_printf(MUT_LOG_LOW, "=== Test PS upgrade status when peer PS is faulted ===");
    
    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the present eses status before inserting the fault to the peer power supply.
     */    
    status = fbe_api_terminator_get_ps_eses_status(terminatorEnclHandle, 
                                                   PS_1, 
                                                   &psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Fault the peer power supply.
     */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_1, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&peerLocation, TRUE, FBE_MGMT_STATUS_TRUE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for peer PS to become faulted failed!");
 
    mut_printf(MUT_LOG_LOW, "Peer PS(0_%d_1) is faulted.\n", peerLocation.enclosure);

    /* Initiate power supply firmware upgrade.
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);

    /* Verify the completion status.
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus); 

    /* Set the peer power supply status back to OK.
    */
    psStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = fbe_api_terminator_set_ps_eses_status(terminatorEnclHandle, 
                                                   PS_1, 
                                                   psStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_wait_for_ps_status(&peerLocation, TRUE, FBE_MGMT_STATUS_FALSE, 60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for peer PS to become OK failed!");
 
    mut_printf(MUT_LOG_LOW, "Peer PS(0_%d_1) is OK.\n", peerLocation.enclosure);

    return;
}

/*!**************************************************************
 * curious_george_test() 
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
void curious_george_test(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    curious_george_test_ps_fup_missing_registry_image_path();
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    curious_george_test_ps_fup_missing_image_file();
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);

    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);

    curious_george_test_complete_lcc_upgrade_before_ps();
    curious_george_test_ps_fup_by_ps_insertion();
    curious_george_test_ps_fup_by_ps_removal_insertion();
    curious_george_test_ps_fup_by_encl_removal_insertion();
    curious_george_test_ps_fup_peer_ps_not_present();
    curious_george_test_ps_fup_peer_ps_faulted();

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}

void curious_george_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void curious_george_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file curious_george_test.c
 *************************/

