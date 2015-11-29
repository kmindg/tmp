/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file edward_the_scissorhands_test.c
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
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_base_object_trace.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * edward_the_scissorhands_short_desc = "Test firmware upgrade abort and resume.";
char * edward_the_scissorhands_long_desc =
    "\n"
    "\n"
    "The edward the scissorhands scenario tests firmware upgrade abort and resume.\n"
    "It includes: \n"
    "    - Test PS firmare upgrade abort and resume;\n"
    "    - Test LCC firmare upgrade abort and resume;\n"
    "    - Test PS and LCC firmare upgrade abort and resume;\n"
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
    "STEP 3: Test PS firmware upgrade abort and resume.\n"
    "    - Initiate the firmware upgrade for PS(0_0_0).\n"
    "    - Initiate the firmware upgrade for PS(0_1_0).\n"
    "    - Abort the PS firmware upgrade.\n"
    "    - Verify the upgrade completion status for PS(0_0_0) and PS(0_1_0).\n"
    "    - Resume the PS firmware upgrade.\n"
    "    - Verify the upgrade completion status for PS(0_0_0) and PS(0_1_0).\n"
    "\n"
    "STEP 4: Set up the LCC firmware upgrade test environment.\n"
    "    - Create LCC firmware image file.\n"
    "    - Create LCC image path registry entry.\n"
    "\n"
    "STEP 5: Test LCC firmware upgrade abort and resume.\n"
    "    - Initiate the firmware upgrade for LCC(0_0_0).\n"
    "    - Initiate the firmware upgrade for LCC(0_1_0).\n"
    "    - Abort the LCC firmware upgrade.\n"
    "    - Verify the upgrade completion status for LCC(0_0_0) and LCC(0_1_0).\n"
    "    - Resume the LCC firmware upgrade.\n"
    "    - Verify the upgrade completion status for LCC(0_0_0) and LCC(0_1_0).\n"
    "\n"
    "STEP 5: Test PS & LCC firmware upgrade abort and resume.\n"
    "    - Initiate the firmware upgrade for PS(0_1_0) & PS(0_2_0).\n"
    "    - Initiate the firmware upgrade for LCC(0_1_0) & LCC(0_2_0).\n"
    "    - Abort the firmware upgrade for all the devices.\n"
    "    - Verify the upgrade completion status for PS(0_1_0) & PS(0_2_0).\n"
    "    - Verify the upgrade completion status for LCC(0_1_0) & LCC(0_2_0).\n"
    "    - Resume the firmware upgrade for all the devices.\n"
    "    - Verify upgrade in progress for PS(0_1_0) & PS(0_2_0).\n"
    "    - Verify upgrade in progress for LCC(0_1_0) & LCC(0_2_0).\n"
    "    - Verify the upgrade completion status for PS(0_1_0) & PS(0_2_0).\n"
    "    - Verify the upgrade completion status for LCC(0_1_0) & LCC(0_2_0).\n"
    "\n"
    "STEP 5: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the PS firmware image file.\n"
    "    - Delete the LCC firmware image file.\n"
    ;



/*!**************************************************************
 * edward_the_scissorhands_test_fup_abort_and_resume() 
 ****************************************************************
 * @brief
 *  Tests firmware upgrade abort and resume for the specified device type.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  01-Sept-2010 PHE - Created. 
 *
 ****************************************************************/
void edward_the_scissorhands_test_fup_abort_and_resume(fbe_u64_t deviceType)
{
    fbe_u8_t                             enclosure = 0; // used for the for loop.
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_bool_t                           is_msg_present = FBE_FALSE;
    fbe_class_id_t                       class_id = FBE_CLASS_ID_INVALID;
   
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;
    location.bus = 0;
    location.slot = 0;
    
    status = fbe_base_env_map_devicetype_to_classid(deviceType, &class_id);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get class id by device type!");
        
    mut_printf(MUT_LOG_LOW, "=== Test %s upgrade abort and resume ===", 
               fbe_base_env_decode_device_type(deviceType));

    for(enclosure = 0; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;

        /* Set up the upgrade environment. */
        fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, FALSE);
    
        /* Initiate firmware upgrade.
         */
        fbe_test_esp_initiate_upgrade(deviceType, &location,forceFlags);
    }

    mut_printf(MUT_LOG_LOW, "Aborting upgrade for all %s", 
               fbe_base_env_decode_device_type(deviceType));

    status = fbe_api_esp_common_abort_upgrade_by_class_id(class_id);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to abort the upgrade!");

    status = fbe_api_wait_for_event_log_msg(30000,
            FBE_PACKAGE_ID_ESP,
            &is_msg_present,
            ESP_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED,
            fbe_base_env_decode_class_id(class_id));
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    
    for(enclosure = 0; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;
    
        /* Verify that the upgrade is aborted.
         */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED;
        fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
    }

    mut_printf(MUT_LOG_LOW, "Upgrade for all %s is aborted", 
               fbe_base_env_decode_device_type(deviceType));

    mut_printf(MUT_LOG_LOW, "Resuming upgrade for all %s", 
               fbe_base_env_decode_device_type(deviceType));

    status = fbe_api_esp_common_resume_upgrade_by_class_id(class_id);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to resume the upgrade!");

    status = fbe_api_wait_for_event_log_msg(30000,
            FBE_PACKAGE_ID_ESP,
            &is_msg_present,
            ESP_INFO_ENV_RESUME_UPGRADE_COMMAND_RECEIVED,
            fbe_base_env_decode_class_id(class_id));
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    
    for(enclosure = 0; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;

        /* Verify the completion status.
         */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_REV_CHANGE;
        fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
    }

    mut_printf(MUT_LOG_LOW, "Upgrade for all %s is resumed", 
               fbe_base_env_decode_device_type(deviceType));

    return;
}


/*!**************************************************************
 * edward_the_scissorhands_test_all_fup_abort_and_resume() 
 ****************************************************************
 * @brief
 *  Tests firmware upgrade abort and resume for all the device types.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
void edward_the_scissorhands_test_fup_abort_and_resume_for_all(void)
{
    fbe_u8_t                             enclosure = 0; // used for the for loop.
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;

    deviceType = FBE_DEVICE_TYPE_LCC;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;
    location.bus = 0;
    location.slot = 0;

    mut_printf(MUT_LOG_LOW, "=== Test ALL upgrade abort and resume ===");

    for(enclosure = 1; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;
    
        /* Set up the upgrade environment. */
        fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);
    
        /* Initiate firmware upgrade all the device types.
         */
        fbe_test_esp_initiate_upgrade_for_all(&location, forceFlags);
    }

    mut_printf(MUT_LOG_LOW, "Aborting upgrade...");
    status = fbe_api_esp_common_abort_upgrade();
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to abort the upgrade!");

    for(enclosure = 1; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;
    
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED;
        fbe_test_esp_verify_fup_completion_status_for_all(&location, expectedCompletionStatus);
    }

    mut_printf(MUT_LOG_LOW, "Upgrade for devices is aborted.");

    mut_printf(MUT_LOG_LOW, "Resuming upgrade...");
    status = fbe_api_esp_common_resume_upgrade();
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to resume the upgrade!");

    for(enclosure = 1; enclosure < 3; enclosure ++)
    {
        location.enclosure = enclosure;

        /* Verify the completion status for all the device types.
         */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
        fbe_test_esp_verify_fup_completion_status_for_all(&location, expectedCompletionStatus);
    }

    mut_printf(MUT_LOG_LOW, "Upgrade for devices is resumed.");
    return;
}


/*!**************************************************************
 * edward_the_scissorhands_test() 
 ****************************************************************
 * @brief
 *  Main entry point for the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  01-Sept-2010 PHE - Created. 
 *
 ****************************************************************/
void edward_the_scissorhands_test(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);

    edward_the_scissorhands_test_fup_abort_and_resume(FBE_DEVICE_TYPE_PS);

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);

    edward_the_scissorhands_test_fup_abort_and_resume(FBE_DEVICE_TYPE_LCC);

    edward_the_scissorhands_test_fup_abort_and_resume_for_all();

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}


void edward_the_scissorhands_setup(void)
{
    fbe_status_t status;
    fbe_u32_t index = 0;
    fbe_u32_t maxEntries = 0;
    fbe_test_params_t * pNaxosTest = NULL;

    /* Load the terminator, the physical package with the naxos config
     * and verify the objects in the physical package.
     */
    maxEntries = fbe_test_get_naxos_num_of_tests() ;
    for(index = 0; index < maxEntries; index++)
    {
        /* Load the configuration for test */
        pNaxosTest =  fbe_test_get_naxos_test_table(index);
        if(pNaxosTest->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
        {
            status = naxos_load_and_verify_table_driven(pNaxosTest);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        }
    }
    
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_esp_sep_and_neit();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);

}

void edward_the_scissorhands_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file edward_the_scissorhands_test.c
 *************************/

