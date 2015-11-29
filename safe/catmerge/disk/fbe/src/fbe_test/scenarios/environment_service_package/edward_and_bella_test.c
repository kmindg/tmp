/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file edward_and_bella_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the funcctions to test Single SP Power Supply firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @version
 *   07/22/2010 PHE - Created.
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
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"


#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * edward_and_bella_short_desc = "Test Single SP PS firmware upgrade state transition and queuing logic.";
char * edward_and_bella_long_desc =
    "\n"
    "\n"
    "The edward and bella scenario tests Single SP power supply firmware upgrade state transition and queuing logic.\n"
    "It includes: \n"
    "    - Test single power supply firmare upgrade;\n"
    "    - Test multiple power supply firmware upgrades;\n"
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
    "STEP 2: Set up the test environment.\n"
    "    - Create the power supply firmware image file.\n"
    "    - Create the power supply image path registry entry.\n"
    "\n"
    "STEP 3: Test single power supply firmare upgrade.\n"
    "    - Initiate the firmware upgrade for the PS(0_2_0).\n"
    "    - Verify the upgrade work state for the PS(0_2_0).\n"
    "    - Verify the upgrade completion status for the PS(0_2_0).\n"
    "\n"
    "STEP 4: Test multiple power supply firmware upgrades.\n"
    "    - Initiate the firmware upgrade for the PS(0_0_0).\n"
    "    - Initiate the firmware upgrade for the PS(0_1_0).\n"
    "    - Verify the upgrade completion status for the PS(0_0_0).\n"
    "    - Verify the upgrade completion status for the PS(0_1_0).\n"
    "\n"
    "STEP 5: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the power supply firmware image file.\n"
    ;

/*!**************************************************************
 * edward_and_bella_get_device_fw_rev() 
 ****************************************************************
 * @brief
 *  Gets the device firmware rev.
 *
 * @param deviceType.               
 * @param pLocation.
 * @param pFirmwareRev.
 * @param fwRevSize.
 * 
 * @return fbe_status_t.
 *
 * @author
 *  20-Jan-2012 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t edward_and_bella_get_device_fw_rev(fbe_u64_t deviceType,
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_u8_t * pFirmwareRev,
                                        fbe_u32_t fwRevSize)
{
    fbe_esp_encl_mgmt_lcc_info_t         lccInfo = {0};
    fbe_esp_ps_mgmt_ps_info_t            getPsInfo = {0};
    fbe_esp_cooling_mgmt_fan_info_t      fanInfo = {0};
    fbe_esp_module_io_module_info_t  io_module_info = {0};
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_status_t                         status = FBE_STATUS_OK;

    switch(deviceType) 
    {
        case FBE_DEVICE_TYPE_LCC:
            status = fbe_api_esp_encl_mgmt_get_lcc_info(pLocation, &lccInfo);
            fbe_copy_memory(pFirmwareRev, 
                            &lccInfo.firmwareRev[0],
                            fwRevSize);
            break;
    
        case FBE_DEVICE_TYPE_PS:
            getPsInfo.location = *pLocation;
            status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);

            MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.bInserted, TRUE);
            fbe_copy_memory(pFirmwareRev, 
                            &getPsInfo.psInfo.firmwareRev[0],
                            fwRevSize);
            break;
    
        case FBE_DEVICE_TYPE_FAN:
            status = fbe_api_esp_cooling_mgmt_get_fan_info(pLocation, &fanInfo);
            fbe_copy_memory(pFirmwareRev, 
                            &fanInfo.firmwareRev[0],
                            fwRevSize);
            break;

        case FBE_DEVICE_TYPE_BACK_END_MODULE:
            io_module_info.header.sp = pLocation->sp;
            io_module_info.header.type = deviceType;
            io_module_info.header.slot = pLocation->slot;
            status = fbe_api_esp_module_mgmt_getIOModuleInfo(&io_module_info);
            fbe_copy_memory(pFirmwareRev, 
                            &io_module_info.io_module_phy_info.lccInfo.firmwareRev[0],
                            fwRevSize);
            break;

        case FBE_DEVICE_TYPE_SP:
            status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
            fbe_copy_memory(pFirmwareRev, 
                            &boardInfo.lccInfo[pLocation->sp].firmwareRev[0],
                            fwRevSize);
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!**************************************************************
 * edward_and_bella_test_single_device_fup() 
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
void edward_and_bella_test_single_device_fup(fbe_u64_t deviceType, fbe_device_physical_location_t *pLocation)
{
    fbe_u32_t                            forceFlags = 0;
    //fbe_base_env_fup_work_state_t        expectedWorkState;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_u8_t                             firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1] = {0}; 

    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    mut_printf(MUT_LOG_LOW, "=== Test single %s upgrade. ===", 
               fbe_base_env_decode_device_type(deviceType));

    status = edward_and_bella_get_device_fw_rev(deviceType, 
                                       pLocation, 
                                       &firmwareRev[0], 
                                       FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get device firmware rev!");   

    mut_printf(MUT_LOG_LOW, "=== %s rev %s BEFORE upgrade. ===", 
               fbe_base_env_decode_device_type(deviceType), &firmwareRev[0]);

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(pLocation, 0, 0, TRUE);

    /* Initiate the upgrade. 
     */
    fbe_test_esp_initiate_upgrade(deviceType, pLocation, forceFlags);
#if 0
    /* Wait for the work state in the ESP to verify that the download command was sent
     * from the ESP to the physical package. 
     */ 
    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT;

    status = fbe_test_esp_wait_for_fup_work_state(deviceType, 
                                             pLocation,
                                             expectedWorkState,
                                             30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT failed!");   

    /* Wait for the work state in the ESP to verify that the activate command was sent
     * from the ESP to the physical package. 
     */ 
    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT;

    status = fbe_test_esp_wait_for_fup_work_state(deviceType, 
                                             pLocation,
                                             expectedWorkState,
                                             30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT failed!");
#endif
    /* Verify the completion status. 
     */
    expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    fbe_test_esp_verify_fup_completion_status(deviceType, pLocation, expectedCompletionStatus);

    status = edward_and_bella_get_device_fw_rev(deviceType, 
                                       pLocation, 
                                       &firmwareRev[0], 
                                       FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get firmware rev!");   

    mut_printf(MUT_LOG_LOW, "=== %s rev %s AFTER upgrade. ===", 
               fbe_base_env_decode_device_type(deviceType), &firmwareRev[0]);

#if 0  // this needs more merging from rockies to work.

    getResumePromInfo.deviceType = deviceType;
    getResumePromInfo.deviceLocation = location;
    status = fbe_api_esp_common_get_resume_prom_info(&getResumePromInfo);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get device resume prom info!");
    
    mut_printf(MUT_LOG_LOW, "=== %s Resume Prom Programmable rev  %s AFTER upgrade. ===", 
               fbe_base_env_decode_device_type(deviceType), &firmwareRev[0]);

    retValue = memcmp(&firmwareRev[0],
                      &getResumePromInfo.resume_prom_data.prog_details[0].prog_rev[0],
                      RESUME_PROM_PROG_REV_SIZE);

    MUT_ASSERT_TRUE_MSG(retValue == 0, "Resume Prom Programmable rev is not updated!");
#endif 
    return;
}

/*!**************************************************************
 * edward_and_bella_test_multiple_enclosure_fup() 
 ****************************************************************
 * @brief
 *  Tests multiple device firmware upgrade for the specified device type.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
void edward_and_bella_test_multiple_enclosure_fup(fbe_u64_t deviceType)
{
    fbe_u8_t                             enclosure = 0; // used for the for loop.
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_base_env_fup_completion_status_t expectedCompletionStatus;
   
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;
    location.bus = 0;

    if (FBE_SIM_SP_B == fbe_api_sim_transport_get_target_server())
    {
        location.slot = 1;
    }
   
    mut_printf(MUT_LOG_LOW, "=== Test multiple %s upgrade in different enclosure. ===",
               fbe_base_env_decode_device_type(deviceType));

    // initiate upgrade for deviceType on each enclosure 
    // The loop count should be determined by the configuration
    for(enclosure = 1; enclosure < NAXOS_MAX_OB_AND_VOY; enclosure ++)
    {
        location.enclosure = enclosure;
    
        /* Set up the upgrade environment. */
        fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

        fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
    }

    /* We don't do the activation in parallel for the same device type even though
     * the device is in different enclosure in case the image is bad.
     * It is not certain that which PS will do the activation first.
     */
   // The loop count should be determined by the configuration
    for(enclosure = 1; enclosure < NAXOS_MAX_OB_AND_VOY; enclosure ++)
    {
        location.enclosure = enclosure;

        /* Verify the completion status. 
         */
        expectedCompletionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
        fbe_test_esp_verify_fup_completion_status(deviceType, &location, expectedCompletionStatus);
    }

    return;
}

/*!**************************************************************
 * edward_and_bella_test() 
 ****************************************************************
 * @brief
 *  Main entry point for the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
void edward_and_bella_test(void)
{
    fbe_status_t status;
    fbe_device_physical_location_t       location = {0};
    
    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);

    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;

    // set location.slot = 1 for FBE_SIM_SP_B
    edward_and_bella_test_single_device_fup(FBE_DEVICE_TYPE_PS, &location);
    edward_and_bella_test_multiple_enclosure_fup(FBE_DEVICE_TYPE_PS);

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);

    return;
}


void edward_and_bella_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void edward_and_bella_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy();
    return;
}


/*************************
 * end file edward_and_bella_test.c
 *************************/
