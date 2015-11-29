/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file grandma_wolf_test.c
 ***************************************************************************
 *
 * @brief
 *  This file verifies the PS firmware upgrade status in various cases. 
 *
 * @version
 *   17-Dec-2010 PHE - Created.
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
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"


char * grandma_wolf_short_desc = "Test firmware upgrade error handling.";
char * grandma_wolf_long_desc =
    "\n"
    "\n"
    "The grandma wolf scenario tests the error handling by injecting different types of errors.\n"
    "It includes: \n"
    "    - The error handling for image checksum error;\n"
    "    - The error handling for bad image;\n"
    "    - The error handling for activate timeout error.\n"
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
    "    - Create the PS firmware upgrade registry setting and the image file.\n"
    "    - Create the LCC firmware upgrade registry setting and the image file.\n"
    "\n" 
    "STEP 2: Test PS upgrade by injecting the image checksum error.\n"
    "    - Initiate PS firmare upgrade on enclosure 0_0.\n"
    "    - Wait until ACTIVATE command is sent.\n"
    "    - Inject the checksum error.\n"
    "    - Verify the completion status.\n"
    "\n" 
    "STEP 3: Test PS upgrade with bad image.\n"
    "    - Initiate PS firmare upgrade on enclosure 0_1, 0_2.\n"
    "    - Verify the PS firmare failure due to the bad image.\n"
    "\n"
    "STEP 4: Test PS upgrade by extending the reset time.\n"
    "    - Set up activate and reset time to make it larger than activate timeout value.\n"
    "    - Initiate PS firmare upgrade on enclosure 0_2.\n"
    "    - Verify the completion status.\n"
    "\n"
    "STEP 5: Repeat the steps 2, 3 and 4 for LCCs.\n"
    "\n"
    "STEP 6: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete PS firmware image file.\n"
    "    - Delete LCC firmware image file.\n"
    ;

#define GRANDMA_WOLF_TEST_DEVICE_TYPE_MAX 2
extern fbe_u64_t test_esp_fup_device_type_table[];
extern fbe_test_params_t naxos_table[];

/*!**************************************************************
 * grandma_wolf_test_upgrade_with_checksum_error() 
 ****************************************************************
 * @brief
 *  Tests the firmware upgrade completion status by injecting the checksum error.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06-Dec-2010 PHE - Created. 
 *
 ****************************************************************/
void grandma_wolf_test_upgrade_with_checksum_error(fbe_u64_t deviceType)
{
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle;
    fbe_base_env_fup_work_state_t        expectedWorkState;
    fbe_download_status_desc_t           downloadStatDesc;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_esp_encl_mgmt_lcc_info_t         lccInfo;
    fbe_esp_ps_mgmt_ps_info_t            psInfo;

    mut_printf(MUT_LOG_LOW, "=== Test %s upgrade with checksum error  ===", 
               fbe_base_env_decode_device_type(deviceType));

    deviceType = deviceType;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 5000, 5000, TRUE);

    
    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Prepare the download status to be set. */
    fbe_zero_memory(&downloadStatDesc, sizeof(fbe_download_status_desc_t));
    downloadStatDesc.status = ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM;

    /* Inject the download status error in terminator. */
    status = fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(terminatorEnclHandle, downloadStatDesc);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    /* Initiate firmware upgrade. */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);

    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT;

    status = fbe_test_esp_wait_for_fup_work_state(deviceType, 
                                             &location,
                                             expectedWorkState,
                                             30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT failed!");  
    
    /* Add the time delay to make sure the ternimator recieved the activate command
     * and update the download status .*/
    fbe_api_sleep(6000);

#if 0 // now automatic retries happen and fail_to_activate is not a final status
    /* Verify the completion status. */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE;
    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
#endif
    /* Add the time delay to make sure the ternimator thread for this activation has terminated. */
    fbe_api_sleep(20000);

    if (deviceType == FBE_DEVICE_TYPE_LCC)
    {
        status = fbe_api_esp_encl_mgmt_get_lcc_info(&location, &lccInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
        MUT_ASSERT_FALSE(lccInfo.fupFailure); 
    }
    else if (deviceType == FBE_DEVICE_TYPE_PS)
    {
        psInfo.location = location;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
        MUT_ASSERT_FALSE(psInfo.psInfo.fupFailure); 
    }
    return;
}


/*!**************************************************************
 * grandma_wolf_test_upgrade_with_bad_image() 
 ****************************************************************
 * @brief
 *  Tests the firmware upgrade with the bad image.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06-Dec-2010 PHE - Created. 
 *
 ****************************************************************/
void grandma_wolf_test_upgrade_with_bad_image(fbe_u64_t deviceType)
{
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_u32_t                            enclosure = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;

    mut_printf(MUT_LOG_LOW, "=== Test %s upgrade with bad image  ===", 
               fbe_base_env_decode_device_type(deviceType));

    deviceType = deviceType;
    location.bus = 0;
    location.slot = 0;
    
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    for(enclosure = 1; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;
    
        /* Set up the upgrade environment. */
        fbe_test_esp_setup_terminator_upgrade_env(&location, 20000, 20000, TRUE);

        /* Initiate power supply firmware upgrade. */
        fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
       
        /* Verify the completion status. */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE;
        fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
    }

    return;
}

/*!**************************************************************
 * grandma_wolf_test_upgrade_with_activate_timeout_error() 
 ****************************************************************
 * @brief
 *  Tests the firmware upgrade completion status by injecting the checksum error.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06-Dec-2010 PHE - Created. 
 *
 ****************************************************************/
void grandma_wolf_test_upgrade_with_activate_timeout_error(fbe_u64_t deviceType)
{
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;

    mut_printf(MUT_LOG_LOW, "=== Test %s upgrade with activate timeout error  ===", 
               fbe_base_env_decode_device_type(deviceType));

    deviceType = deviceType;
    location.bus = 0;
    location.enclosure = 2;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                      FBE_BASE_ENV_FUP_FORCE_FLAG_NO_BAD_IMAGE_CHECK | 
                      FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    /* Set up the upgrade environment.
     * activate time interval is 20 seconds and reset time interval is 40 seconds
     * to create the activation timeout(total 60 seconds) for LCC. 
     * PS activation timeout is 5 minutes. 
     * We don't want to wait for that long fot the test. Will need a interface function to 
     * modify the PS activation timeout for the test purpose. 
     */ 
    fbe_test_esp_setup_terminator_upgrade_env(&location, 20000, 40000, TRUE);

    /* Initiate power supply firmware upgrade. */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
    
    /* Verify the completion status. */
    if(deviceType == FBE_DEVICE_TYPE_LCC) 
    {
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED;
    }
    else
    {
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    }

    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
    
    return;
}

/*!**************************************************************
 * grandma_wolf_test() 
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
void grandma_wolf_test(void)
{
    fbe_u32_t            index = 0;
    fbe_status_t         status = FBE_STATUS_OK;
    fbe_u64_t    deviceType;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);

    for(index = 0; test_esp_fup_device_type_table[index] != FBE_DEVICE_TYPE_INVALID; index ++)
    {
        deviceType = test_esp_fup_device_type_table[index];
        grandma_wolf_test_upgrade_with_checksum_error(deviceType);
        //grandma_wolf_test_upgrade_with_bad_image(deviceType);
        //grandma_wolf_test_upgrade_with_activate_timeout_error(deviceType);
    }

    grandma_wolf_test_upgrade_with_activate_timeout_error(FBE_DEVICE_TYPE_LCC);


    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}

void grandma_wolf_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void grandma_wolf_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file grandma_wolf_test.c
 *************************/


