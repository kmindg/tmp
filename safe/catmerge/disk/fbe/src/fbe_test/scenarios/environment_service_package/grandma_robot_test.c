/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file grandma_robot_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the funcctions to test Fan firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @version
 *   22-Apr-2011 PHE - Created.
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
#include "fbe/fbe_api_sim_server.h"
#include "fbe_base_environment_debug.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * grandma_robot_short_desc = "Test Single SP FAN firmware upgrade state transition and queuing logic.";
char * grandma_robot_long_desc =
    "\n"
    "\n"
    "The edward and bella scenario tests Single SP Fan firmware upgrade state transition and queuing logic.\n"
    "It includes: \n"
    "    - Test single Fan firmare upgrade;\n"
    "    - Test multiple Fan firmware upgrades;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 3 voyager enclosures\n"
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
    "STEP 2: Set up the test environment.\n"
    "    - Create the Fan firmware image file.\n"
    "    - Create the Fan image path registry entry.\n"
    "\n"
    "STEP 3: Test single power supply firmare upgrade.\n"
    "    - Initiate the firmware upgrade for the FAN(0_2_0).\n"
    "    - Verify the upgrade work state for the FAN(0_2_0).\n"
    "    - Verify the upgrade completion status for the FAN(0_2_0).\n"
    "\n"
    "STEP 4: Test multiple power supply firmware upgrades.\n"
    "    - Initiate the firmware upgrade for the FAN(0_0_0).\n"
    "    - Initiate the firmware upgrade for the FAN(0_1_0).\n"
    "    - Verify the upgrade completion status for the FAN(0_0_0).\n"
    "    - Verify the upgrade completion status for the FAN(0_1_0).\n"
    "\n"
    "STEP 5: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the FAN firmware image file.\n"
    ;

extern fbe_test_params_t naxos_table[];

/*!**************************************************************
 * grandma_robot_test_multiple_fan_fup_in_same_enclosure() 
 ****************************************************************
 * @brief
 *  Tests multiple device firmware upgrade in the same enclosure for the specified device type.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
void grandma_robot_test_multiple_fan_fup_in_same_enclosure(void)
{
    fbe_u8_t                             slot = 0; // used for the for loop.
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_u64_t                    deviceType;
   
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    deviceType = FBE_DEVICE_TYPE_FAN;

    location.bus = 0;
    location.enclosure = 0;
   
    mut_printf(MUT_LOG_LOW, "=== Test multiple FAN upgrade in same enclosure. ===");

    for(slot = 1; slot < 2; slot ++)
    {
        location.slot = slot;
    
        /* Set up the upgrade environment. */
        fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

        /* Initiate power supply firmware upgrade.
         */
        fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
    }

    /* We don't do the activation in parallel for the same device type even though
     * the device is in different enclosure in case the image is bad.
     * It is not certain that which PS will do the activation first.
     */
   
    for(slot = 1; slot < 2; slot ++)
    {
        location.slot = slot;

        /* Verify the completion status. 
         */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
        fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
    }

    return;
}


/*!**************************************************************
 * @fn grandma_robot_test_peer_not_present_device_fup(fbe_u64_t deviceType)
 ****************************************************************
 * @brief
 *  Tests single device firmware upgrade for the specified device type.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
void grandma_robot_test_peer_not_present_device_fup(fbe_u64_t deviceType)
{
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_work_state_t        expectedWorkState;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;
    
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 1;

    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    mut_printf(MUT_LOG_LOW, "=== Test peer not present %s upgrade. ===", 
               fbe_base_env_decode_device_type(deviceType));

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);

    /* Wait for the work state in the ESP to verify that the download command was sent
     * from the ESP to the physical package. 
     */ 
    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT;

    status = fbe_test_esp_wait_for_fup_work_state(deviceType, 
                                             &location,
                                             expectedWorkState,
                                             30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT failed!");   

    /* Wait for the work state in the ESP to verify that the activate command was sent
     * from the ESP to the physical package. 
     */ 
    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT;

    status = fbe_test_esp_wait_for_fup_work_state(deviceType, 
                                             &location,
                                             expectedWorkState,
                                             30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT failed!");

    /* Verify the completion status. 
     */
    if((deviceType == FBE_DEVICE_TYPE_PS) || 
       (deviceType == FBE_DEVICE_TYPE_FAN))
    {
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    }
    else if(deviceType == FBE_DEVICE_TYPE_LCC) 
    {
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION;
    }

    fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);

    return;
}

/*!**************************************************************
 * grandma_robot_test() 
 ****************************************************************
 * @brief
 *  Main entry point for the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Apr-2011 PHE - Created. 
 *
 ****************************************************************/
void grandma_robot_test(void)
{
    fbe_status_t status;
    fbe_device_physical_location_t       location = {0};

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    status = fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_FAN);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to create FAN image file!");

    status = fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_FAN);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to create FAN registry image path!");

    // set location.slot = 1 for FBE_SIM_SP_B
    edward_and_bella_test_single_device_fup(FBE_DEVICE_TYPE_FAN, &location);
    edward_and_bella_test_multiple_enclosure_fup(FBE_DEVICE_TYPE_FAN);
    grandma_robot_test_multiple_fan_fup_in_same_enclosure();
    grandma_robot_test_peer_not_present_device_fup(FBE_DEVICE_TYPE_FAN);

    status = fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_FAN);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to delete FAN image file!");

    return;
}


void grandma_robot_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VOYAGER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void grandma_robot_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy();
    return;
}


/*************************
 * end file grandma_robot_test.c
 *************************/
