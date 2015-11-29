/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file tintin_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the funcctions to test firmware upgrade abort and resume. 
 *
 * @version
 *   01-Sept-2010 PHE - Created.
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
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"


char * tintin_short_desc = "Test firmare upgrade coordination and prioritization.";
char * tintin_long_desc =
    "\n"
    "\n"
    "The tintin test scenario tests firmare upgrade coordination and prioritization.\n"
    "It includes: \n"
    "    - Test PS firmare download command can not be sent successfully before LCC upgrade is successful;\n"
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
    "STEP 2: Set up the PS firmware upgrade test environment.\n"
    "    - Create PS firmware image file.\n"
    "    - Create PS image path registry entry.\n"
    "\n"
    "STEP 3: Set up the LCC firmware upgrade test environment.\n"
    "    - Create LCC firmware image file.\n"
    "    - Create LCC image path registry entry.\n"
    "\n"
    "STEP 4: Test PS firmware download command can not be sent successfully.\n"
    "    - Initiate the firmware upgrade for PS(0_0_0).\n"
    "    - Waiting for 30 seconds.\n"
    "    - Verify that the PS(0_0_0) firmware download commadn can not be sent successfuly.\n"
    "\n"
    "STEP 5: Test PS firmware upgrade completes after LCC firmware upgrade is successful.\n"
    "    - Initiate the firmware upgrade for LCC(0_0_0).\n"
    "    - Verify the LCC(0_0_0) firmware upgrade completes successfully.\n"
    "    - Verify the PS(0_0_0) firmware upgrade completes successfully.\n"
    "\n"
    "STEP 6: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the PS firmware image file.\n"
    "    - Delete the LCC firmware image file.\n"
    ;



/*!**************************************************************
 * tintin_test_ps_and_lcc_coordination() 
 ****************************************************************
 * @brief
 *  Verify the LCC upgrade has to be done before the PS upgrade.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  26-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
void tintin_test_ps_and_lcc_coordination(void)
{
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_work_state_t        expectedWorkState;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;
   
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE;

    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Initiate PS firmware upgrade.
     */
    fbe_test_esp_initiate_upgrade(FBE_DEVICE_TYPE_PS, &location, forceFlags);
    
    /* Waiting for 15 seconds to verify the download cmd for PS can not be sent successfully.
     */
    mut_printf(MUT_LOG_LOW, "Waiting for 15 seconds to verify download cmd can not be sent successfully");
    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT;

    status = fbe_test_esp_wait_for_fup_work_state(FBE_DEVICE_TYPE_PS, 
                                             &location,
                                             expectedWorkState,
                                             15000);

    MUT_ASSERT_TRUE_MSG(status != FBE_STATUS_OK, 
        "Work state should not be FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT!");

    mut_printf(MUT_LOG_LOW, "Verified that Download cmd can not be sent successfully!");

    fbe_test_esp_initiate_upgrade(FBE_DEVICE_TYPE_LCC, &location, forceFlags);

    /* The LCC upgrade has to been done before the PS upgrade. */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    fbe_test_esp_verify_fup_completion_status(FBE_DEVICE_TYPE_LCC, &location, expectedCompletionStatus);
 
    fbe_test_esp_verify_fup_completion_status(FBE_DEVICE_TYPE_PS, &location, expectedCompletionStatus);

    return;
}


/*!**************************************************************
 * tintin_test() 
 ****************************************************************
 * @brief
 *  Main entry point for the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  26-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
void tintin_test(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);

    tintin_test_ps_and_lcc_coordination();

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}


void tintin_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void tintin_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file tintin_test.c
 *************************/


