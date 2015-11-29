#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_enclosure.h"
#include "fbe_enclosure_data_access_public.h"
#include "fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_object_map_interface.h"

void fbe_api_enclosure_bypass_drive_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_base_enclosure_drive_bypass_command_t drive_bypass_cmd;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);//test for enclosure no 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_bypass_drive(object_handle_p, &drive_bypass_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);//test for enclosure no 1
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_bypass_drive(object_handle_p, &drive_bypass_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_bypass_drive successfully!!! ===\n");
    return;
}
void fbe_api_enclosure_unbypass_drive_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_base_enclosure_drive_unbypass_command_t drive_unbypass_cmd;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);//test for enclosure no 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    /* Set up the input here */
    /* uncomment this when unbypass is ready //drive_unbypass_cmd.drive_unbypass_control = 0;*/
    status = fbe_api_enclosure_unbypass_drive(object_handle_p, &drive_unbypass_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set up the input here */
    /* uncomment this when unbypass is ready //drive_unbypass_cmd.drive_unbypass_control = 1;*/
    status = fbe_api_enclosure_unbypass_drive(object_handle_p, &drive_unbypass_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_unbypass_drive successfully!!! ===\n");
    return;
}
void fbe_api_enclosure_firmware_download_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_enclosure_mgmt_download_op_t download_firmware_test;

    // init the status struct
    fbe_zero_memory(&download_firmware_test, sizeof(fbe_enclosure_mgmt_download_op_t));

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);//test for enclosure no 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_firmware_download(object_handle_p ,&download_firmware_test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);//test for enclosure no 1
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    download_firmware_test.op_code = FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD;
    status = fbe_api_enclosure_firmware_download(object_handle_p ,&download_firmware_test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_firmware_download successfully!!! ===\n");
    return;

}
void fbe_api_enclosure_firmware_download_status_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_enclosure_mgmt_download_op_t firmware_download_status_test;
    
    // init the status struct
    fbe_zero_memory(&firmware_download_status_test, sizeof(fbe_enclosure_mgmt_download_op_t));

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);//test for enclosure no 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_firmware_download_status(object_handle_p,&firmware_download_status_test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);//test for enclosure no 1
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    firmware_download_status_test.op_code = FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS;
    status = fbe_api_enclosure_firmware_download_status(object_handle_p,&firmware_download_status_test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_firmware_download_status successfully!!! ===\n");
    return;
}

void fbe_api_enclosure_get_info_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_base_object_mgmt_get_enclosure_info_t enclosure_info;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_get_info(object_handle_p, &enclosure_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

//    fbe_edal_printAllComponentData((void *)&enclosure_info);

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_get_info(object_handle_p, &enclosure_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

//    fbe_edal_printAllComponentData((void *)&enclosure_info);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_get_info successfully!!! ===\n");
    return;
}

void fbe_api_enclosure_get_setup_info_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_base_object_mgmt_get_enclosure_setup_info_t enclosureSetupInfo;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_getSetupInfo(object_handle_p, &enclosureSetupInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    MUT_ASSERT_TRUE(enclosureSetupInfo.enclosureBufferSize == VIPER_ENCL_TOTAL_DATA_SIZE);

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_getSetupInfo(object_handle_p, &enclosureSetupInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

    MUT_ASSERT_TRUE(enclosureSetupInfo.enclosureBufferSize == VIPER_ENCL_TOTAL_DATA_SIZE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_getSetupInfo successfully!!! ===\n");
    return;

}

#define MAX_REQUEST_STATUS_COUNT    10
void fbe_api_enclosure_set_control_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_edal_status_t edalStatus;
    fbe_base_object_mgmt_set_enclosure_control_t enclosureControlInfoBuffer;
    fbe_edal_block_handle_t                  enclosureControlInfo = &enclosureControlInfoBuffer;
    fbe_bool_t driveFaultLedOn;
    fbe_bool_t enclFaultLedOn;
    fbe_bool_t lccFaultLedOn;
    fbe_bool_t driveMarked;
    fbe_u32_t                                       originalStateChangeCount;
    fbe_u32_t                                       newStateChangeCount;
    fbe_bool_t                                      statusChangeFound = FALSE;
    fbe_u8_t                                        requestCount = 0;
    fbe_bool_t                                      turnOn, markOn;
    fbe_u16_t                                       index;

    mut_printf(MUT_LOG_TEST_STATUS, "=== TEST ENCLOSURE SET CONTROL PROCESSING ===\n");

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    /*
     * Turn on Drive Fault LED
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive Fault LED Test ===\n");
    for (index = 0; index < 2; index++)
    {
        if (index == 0)
        {
            turnOn = TRUE;
        }
        else
        {
            turnOn = FALSE;
        }

        // get current enclosure (formatted correctly)
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Save State change count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                 &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "**** Encl %d , originalStateChangeCount %d****\n",
            0, originalStateChangeCount);

        // set some write data
        edalStatus = fbe_edal_setDriveBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                      FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                      3,
                                      turnOn);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                      FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                      FBE_ENCL_ENCLOSURE,
                                      0,
                                      turnOn);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                      FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                      FBE_ENCL_LCC,
                                      0,
                                      turnOn);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
// Display fields
        edalStatus = fbe_edal_setU8(enclosureControlInfo,
                                    FBE_DISPLAY_MODE,
                                    FBE_ENCL_DISPLAY,
                                    0,
                                    SES_DISPLAY_MODE_DISPLAY_CHAR);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setU8(enclosureControlInfo,
                                    FBE_DISPLAY_CHARACTER,
                                    FBE_ENCL_DISPLAY,
                                    0,
                                    1);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setU8(enclosureControlInfo,
                                    FBE_DISPLAY_MODE,
                                    FBE_ENCL_DISPLAY,
                                    1,
                                    SES_DISPLAY_MODE_DISPLAY_CHAR);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setU8(enclosureControlInfo,
                                    FBE_DISPLAY_CHARACTER,
                                    FBE_ENCL_DISPLAY,
                                    1,
                                    0);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setU8(enclosureControlInfo,
                                    FBE_DISPLAY_MODE,
                                    FBE_ENCL_DISPLAY,
                                    2,
                                    SES_DISPLAY_MODE_DISPLAY_CHAR);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        edalStatus = fbe_edal_setU8(enclosureControlInfo,
                                    FBE_DISPLAY_CHARACTER,
                                    FBE_ENCL_DISPLAY,
                                    2,
                                    2);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

        // send to enclosure
        status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify that Drive LED is ON
        mut_printf(MUT_LOG_TEST_STATUS, "=== Verify Drive Fault LED ===\n");
        statusChangeFound = FALSE;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                        &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                0, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                //State Change detected, so time to check if Drive is bypassed
                edalStatus = fbe_edal_getDriveBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_FAULT_LED_ON,
                                              3,
                                              &driveFaultLedOn);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                              FBE_ENCL_ENCLOSURE,
                                              0,
                                              &enclFaultLedOn);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);

                edalStatus = fbe_edal_getBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                              FBE_ENCL_LCC,
                                              0,
                                              &lccFaultLedOn);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if ((driveFaultLedOn == turnOn) && (enclFaultLedOn == turnOn) && (lccFaultLedOn == turnOn))
//                if ((driveFaultLedOn == turnOn) && (enclFaultLedOn == turnOn))
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "=== Verified Drive, Enclosure, LCC LED ON ===\n");
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

    }

    /*
     * Mark Drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Mark Drive LED Test ===\n");
    for (index = 0; index < 2; index++)
    {
        if (index == 0)
        {
            markOn = TRUE;
        }
        else
        {
            markOn = FALSE;
        }

        // get current enclosure (formatted correctly)
        fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
        status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Save State change count
        edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                 &originalStateChangeCount);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "**** Encl %d , originalStateChangeCount %d****\n",
            0, originalStateChangeCount);

        // set some write data
        edalStatus = fbe_edal_setDriveBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                      FBE_ENCL_COMP_MARK_COMPONENT,
                                      10,
                                      markOn);
        MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
        // send to enclosure
        status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        // Verify that Drive LED is ON
        mut_printf(MUT_LOG_TEST_STATUS, "=== Verify Drive LED Marked ===\n");
        statusChangeFound = FALSE;
        while (requestCount < MAX_REQUEST_STATUS_COUNT)
        {
            // Request current enclosure data
            requestCount++;
            fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
            status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            // Check for State Change
            edalStatus = fbe_edal_getOverallStateChangeCount(enclosureControlInfo,
                                                        &newStateChangeCount);
            MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "**** Encl %d , requestCount %d, newStateChangeCount %d****\n",
                0, requestCount, newStateChangeCount);
            if (newStateChangeCount > originalStateChangeCount)
            {
                //State Change detected, so time to check if Drive is bypassed
                edalStatus = fbe_edal_getDriveBool(enclosureControlInfo,
                                              FBE_ENCL_COMP_MARKED,
                                              10,
                                              &driveMarked);
                MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
                if (driveMarked == markOn)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "=== Verified Drive LED Marked/Unmarked ===\n");
                    statusChangeFound = TRUE;
                    break;
                }
                else
                {
                    originalStateChangeCount = newStateChangeCount;
                }
            }

            // Delay before requesting Enclosure status again
            fbe_api_sleep (3000);

        }   // end of while loop
        MUT_ASSERT_TRUE(statusChangeFound == TRUE);

    }

#if FALSE
    // delay before sending next Control info
    fbe_api_sleep (10000);

    /*
     * Re-set values & send Control
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Verify Drive LED Control ===\n");
    // get current enclosure (formatted correctly
    fbe_zero_memory(&enclosureControlInfoBuffer, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
    status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // verify previous write
    edalStatus = fbe_edal_getDriveBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                  FBE_ENCL_COMP_FAULT_LED_ON,
                                  3,
                                  &driveFaultLedOn);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    MUT_ASSERT_TRUE(driveFaultLedOn == TRUE);
    edalStatus = fbe_edal_getDriveBool(enclosureControlInfo,             // mark drive 10 (flash LED)
                                  FBE_ENCL_COMP_MARKED,
                                  10,
                                  &driveMarked);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    MUT_ASSERT_TRUE(driveMarked == TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive LED Control verified ===\n");

    // set some write data
    mut_printf(MUT_LOG_TEST_STATUS, "=== Reset Drive LED Control ===\n");
    edalStatus = fbe_edal_setDriveBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                  FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                  3,
                                  FALSE);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    edalStatus = fbe_edal_setDriveBool(enclosureControlInfo,             // mark drive 10 (flash LED)
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  10,
                                  FALSE);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    // send to enclosure
    status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // delay before sending next Control info
    fbe_api_sleep (10000);

    // verify previous write
    edalStatus = fbe_edal_getDriveBool(enclosureControlInfo,             // turn on drive 3 fault LED
                                  FBE_ENCL_COMP_FAULT_LED_ON,
                                  3,
                                  &driveFaultLedOn);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    MUT_ASSERT_TRUE(driveFaultLedOn == FALSE);
    edalStatus = fbe_edal_getDriveBool(enclosureControlInfo,             // mark drive 10 (flash LED)
                                  FBE_ENCL_COMP_MARKED,
                                  10,
                                  &driveMarked);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    MUT_ASSERT_TRUE(driveMarked == FALSE);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Drive LED Control reset ===\n");

    // delay before sending next Control info
    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Wait for 10 secs & see if Status changes!!! ===\n");
    fbe_api_sleep (10000);

    // get current enclosure (formatted correctly
    status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
// jap
    fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);

    // delay before sending next Control info
    fbe_api_sleep (10000);

//    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    /*
     * Send some write data for the enclosure
     */
    // clear buffer that was previously used
    memset(&enclosureControlInfoBuffer, 0, sizeof(fbe_base_object_mgmt_set_enclosure_control_t));
    // get current enclosure (formatted correctly
    status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set some write data
    edalStatus = fbe_edal_setDrivePhyBool(enclosureControlInfo,
                                          FBE_ENCL_EXP_PHY_DISABLE,
                                          1,
                                          TRUE);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    // send to enclosure
    status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

    // delay before sending next Control info
    fbe_api_sleep (5000);

    // get current enclosure (formatted correctly
    status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set some write data
    edalStatus = fbe_edal_setDrivePhyBool(enclosureControlInfo,
                                          FBE_ENCL_EXP_PHY_DISABLE,
                                          14,
                                          TRUE);
    MUT_ASSERT_TRUE(edalStatus == FBE_EDAL_STATUS_OK);
    // send to enclosure
    status = fbe_api_enclosure_setControl(object_handle_p, &enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
#endif

    // delay before sending next Control info
    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Wait for 10 secs & see if Status changes!!! ===\n");
    fbe_api_sleep (10000);

    // get current enclosure (formatted correctly
    status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
// jap
    fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_setControl successfully!!! ===\n");
    return;

}

void fbe_api_enclosure_set_leds_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_base_object_mgmt_set_enclosure_led_t enclosureLedInfo;
    fbe_base_object_mgmt_set_enclosure_control_t enclosureControlInfoBuffer;
    fbe_edal_block_handle_t                  enclosureControlInfo = &enclosureControlInfoBuffer;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    // set Flash LED command to turn on flashing
    mut_printf(MUT_LOG_TEST_STATUS, "=== Flash Enclosure ON ===\n");
    enclosureLedInfo.flashLedsActive = TRUE;
    enclosureLedInfo.flashLedsOn = TRUE;
    status = fbe_api_enclosure_setLeds(object_handle_p, &enclosureLedInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

#if TRUE
    // delay before sending next Control info
    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Wait for 10 secs & see if Status changes!!! ===\n");
    fbe_api_sleep (10000);
    status = fbe_api_enclosure_get_info(object_handle_p, 
                                        (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
// jap
    fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
#endif

//    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    // set Flash LED command to turn off flashing
    mut_printf(MUT_LOG_TEST_STATUS, "=== Flash Enclosure OFF ===\n");
    enclosureLedInfo.flashLedsActive = TRUE;
    enclosureLedInfo.flashLedsOn = FALSE;
    status = fbe_api_enclosure_setLeds(object_handle_p, &enclosureLedInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   

#if TRUE
    // delay before sending next Control info
    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Wait for 10 secs & see if Status changes!!! ===\n");
    fbe_api_sleep (10000);
    status = fbe_api_enclosure_get_info(object_handle_p, (fbe_base_object_mgmt_get_enclosure_info_t *)&enclosureControlInfoBuffer);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
// jap
    fbe_edal_printAllComponentData(enclosureControlInfo, fbe_edal_trace_func);
#endif

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_setLeds successfully!!! ===\n");
    return;

}

void fbe_api_enclosure_firmware_update_test(void)
{
    fbe_u32_t object_handle_p;
    fbe_status_t status;
    fbe_enclosure_mgmt_download_op_t activate_firmware_test;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);//test for enclosure no 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_firmware_update(object_handle_p ,&activate_firmware_test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);//test for enclosure no 1
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);

    activate_firmware_test.op_code = FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE;
    status = fbe_api_enclosure_firmware_update(object_handle_p ,&activate_firmware_test);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute enclosure_firmware_update successfully!!! ===\n");
    return;

}

void fbe_api_enclosure_get_slot_to_phy_mapping_test(void)
{
    fbe_object_id_t               object_id;
    fbe_status_t                  status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_ctrl_op_t  slot_to_phy_mapping_op;
    fbe_u8_t                    * response_buf_p = NULL;
    fbe_u32_t                     response_buf_size = 0;
    fbe_enclosure_status_t       enclosure_status = FBE_ENCLOSURE_STATUS_OK;

    status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_id);//test for enclosure no 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);

    response_buf_size = 4 * sizeof(fbe_enclosure_mgmt_slot_to_phy_mapping_info_t);
    /* Allocate the memory. */
    response_buf_p = (fbe_u8_t *)malloc(response_buf_size);
    
    /* Fill in the struct fbe_enclosure_mgmt_ctrl_op_t with slot_num_start(slot 0), slot_num_end(slot 4),
     * response_buf_p, response_buf_size.
     */
    status = fbe_api_enclosure_build_slot_to_phy_mapping_cmd(&slot_to_phy_mapping_op, 0, 3,
                                                    response_buf_p, response_buf_size);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Send the operation to the physical package. */
    status = fbe_api_enclosure_get_slot_to_phy_mapping(object_id, &slot_to_phy_mapping_op, &enclosure_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(enclosure_status == FBE_ENCLOSURE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute get_slot_to_phy_mapping successfully!!! ===\n");
    return;

}