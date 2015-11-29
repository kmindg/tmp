/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file tom_and_jerry.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the physical package PE component info change notification test. 
 *
 * @version
 *   12/01/2009 PHE - Created.
 *
 ***************************************************************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe_notification.h"

char * tom_and_jerry_short_desc = "Test processor enclosure environmental data change notification.";
char * tom_and_jerry_long_desc =
    "\n"
    "\n"
    "The tom and jerry scenario tests the physical package processor enclosure component data change notification.\n"
    "It includes: \n"
    "    - The state change notification for the IO module;\n"
    "    - The state change notification for the power supply;\n"
    "    - The state change notification for the management module;\n"
    "\n"
    "Dependencies:\n"
    "    - The SPECL should be able to work in the user space to read the status from the terminator.\n"
    "    - The terminator should be able to provide the interface function to insert/remove the components in the processor enclosure.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board with processor enclosure data\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"  
    "STEP 2: Verify the Misc data change notification received.\n"
    "    - Get the processor enclosure fault LED status.\n"
    "    - Verify the enclosure fault LED status is LED_BLINK_OFF.\n"
    "    - Set the enclosure fault LED status to LED_BLINK_ON.\n"
    "    - Wait for the Misc data change notificatioin.\n"
    "\n"
    "STEP 3: Verify the IO module data change notification received.\n"
    "    - Get the IO module info.\n"
    "    - Verify the IO module fault LED status is LED_BLINK_OFF.\n"
    "    - Set the IO module fault LED status to LED_BLINK_ON.\n"
    "    - Wait for the IO module data change notification.\n"
    "\n"
    "STEP 4: Verify the IO Port data change notification received.\n"
    "    - Get the IO Port info.\n"
    "    - Verify the IO Port fault LED status is LED_BLINK_OFF.\n"
    "    - Set the IO Port fault LED status to LED_BLINK_ON.\n"
    "    - Wait for the IO Port data change notification.\n"
    "\n"
    "STEP 5: Verify the management module data change notification received.\n"
    "    - Get the management module Info.\n"
    "    - Verify the management module vLan configuration mode is CLARiiON_VLAN_MODE.\n"
    "    - SEt the management module vLan configuration mode to CUSTOM_MGMT_ONLY_VLAN_MODE.\n"
    "    - Wait for the management module data change notification.\n"
    "\n"   
    "STEP 6: Shutdown the Terminator/Physical package.\n"
    ;

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_class_id_t           expected_class_id = FBE_CLASS_ID_BASE_BOARD;
static fbe_u64_t        expected_device_type = FBE_DEVICE_TYPE_INVALID;
static fbe_device_data_type_t   expected_data_type = FBE_DEVICE_DATA_TYPE_INVALID;
static fbe_device_physical_location_t expected_phys_location = {0};

static fbe_semaphore_t          sem;
static fbe_semaphore_t          cmd_sem;
static fbe_notification_registration_id_t          reg_id;

static void tom_and_jerry_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);

static void tom_and_jerry_test_verify_misc_data_change(fbe_object_id_t objectId);
static void tom_and_jerry_test_verify_io_module_data_change(fbe_object_id_t objectId);
static void tom_and_jerry_test_verify_io_port_data_change(fbe_object_id_t objectId);
static void tom_and_jerry_test_verify_mgmt_module_data_change(fbe_object_id_t objectId);
static void tom_and_jerry_test_verify_mgmt_module_vlan_mode_completion(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *command_context);
static void tom_and_jerry_test_verify_set_mgmt_port_completion(fbe_board_mgmt_set_mgmt_port_async_context_t *command_context);
static void tom_and_jerry_set_PostAndOrReset_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context);


void tom_and_jerry(void)
{
    fbe_status_t        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     objectId;
                                                            
    fbe_semaphore_init(&sem, 0, 1);
    fbe_semaphore_init(&cmd_sem, 0, 1);
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                  tom_and_jerry_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Get handle to the board object */
    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    tom_and_jerry_test_verify_misc_data_change(objectId);
    tom_and_jerry_test_verify_io_module_data_change(objectId);
    //tom_and_jerry_test_verify_io_port_data_change(objectId);
    tom_and_jerry_test_verify_mgmt_module_data_change(objectId);
        
    fbeStatus = fbe_api_notification_interface_unregister_notification(tom_and_jerry_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);
    fbe_semaphore_destroy(&cmd_sem); /* SAFEBUG - needs destroy */

    return;
}

static void tom_and_jerry_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_bool_t           expected_notification = FALSE; 
    fbe_notification_data_changed_info_t * pDataChangeInfo = NULL;

    pDataChangeInfo = &update_object_msg->notification_info.notification_data.data_change_info;

    if((expected_object_id == update_object_msg->object_id) && 
       (expected_device_type == pDataChangeInfo->device_mask) &&
       (expected_data_type == pDataChangeInfo->data_type) &&
       (expected_class_id == FBE_CLASS_ID_BASE_BOARD))
    {
        switch(expected_device_type)
        {
            case FBE_DEVICE_TYPE_PLATFORM:
            case FBE_DEVICE_TYPE_MISC:
                expected_notification = TRUE; 
                break;

            case FBE_DEVICE_TYPE_IOMODULE:
            case FBE_DEVICE_TYPE_MEZZANINE:
                if(pDataChangeInfo->data_type == FBE_DEVICE_DATA_TYPE_GENERIC_INFO)
                {
                    if((expected_phys_location.sp == pDataChangeInfo->phys_location.sp) &&
                       (expected_phys_location.slot == pDataChangeInfo->phys_location.slot))
                    {
                        expected_notification = TRUE; 
                    }
                }
                else if(pDataChangeInfo->data_type == FBE_DEVICE_DATA_TYPE_PORT_INFO)
                {
                    if((expected_phys_location.sp == pDataChangeInfo->phys_location.sp) &&
                       (expected_phys_location.slot == pDataChangeInfo->phys_location.slot)&&
                       (expected_phys_location.port == pDataChangeInfo->phys_location.port))
                    {
                        expected_notification = TRUE; 
                    }
                }
                break;

            case FBE_DEVICE_TYPE_BACK_END_MODULE:
            case FBE_DEVICE_TYPE_MGMT_MODULE: 
            case FBE_DEVICE_TYPE_SLAVE_PORT:
            case FBE_DEVICE_TYPE_SUITCASE:
            case FBE_DEVICE_TYPE_BMC:
                if((expected_phys_location.sp == pDataChangeInfo->phys_location.sp) &&
                   (expected_phys_location.slot == pDataChangeInfo->phys_location.slot))
                {
                    expected_notification = TRUE; 
                }
                break;

            case FBE_DEVICE_TYPE_PS:
            case FBE_DEVICE_TYPE_FAN:
                if(expected_phys_location.slot == pDataChangeInfo->phys_location.slot)
                {
                    expected_notification = TRUE; 
                }
                break;

            default:
                break;
        }
    }

    if(expected_notification)
    {
        fbe_semaphore_release(sem, 0, 1 ,FALSE);
    }

    return;
}

static void tom_and_jerry_test_verify_misc_data_change(fbe_object_id_t objectId)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_misc_info_t miscInfo = {0};
    fbe_board_mgmt_set_PostAndOrReset_t post_reset = {0};
    fbe_board_mgmt_set_PostAndOrReset_async_context_t *context;
    fbe_api_board_enclFaultLedInfo_t            enclFaultLedInfo;

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Misc Data Change ===\n");

    /* Get the management module  B0 info. */
    fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.EnclosureFaultLED == LED_BLINK_OFF);
    MUT_ASSERT_TRUE(miscInfo.SPFaultLED == LED_BLINK_OFF);
    MUT_ASSERT_TRUE(miscInfo.UnsafeToRemoveLED == LED_BLINK_OFF);
    MUT_ASSERT_TRUE(post_reset.sp_id == SP_A);
    MUT_ASSERT_TRUE(post_reset.holdInPost == FALSE);
    MUT_ASSERT_TRUE(post_reset.holdInReset == FALSE);
    MUT_ASSERT_TRUE(post_reset.rebootBlade == FALSE);

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_MISC;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Change the enclosure fault LED status. */
    enclFaultLedInfo.blink_rate = LED_BLINK_ON;
    fbeStatus = fbe_api_board_set_enclFaultLED(objectId, &enclFaultLedInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.EnclosureFaultLED == LED_BLINK_ON);

    /* Change the SP fault LED status. */
    fbeStatus = fbe_api_board_set_spFaultLED(objectId, LED_BLINK_ON, FAULT_CONDITION);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.SPFaultLED == LED_BLINK_ON);

    /* Change the Unsafe to Remove LED status. */
    fbeStatus = fbe_api_board_set_UnsafetoRemoveLED(objectId, LED_BLINK_ON);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.UnsafeToRemoveLED == LED_BLINK_ON);

    context = malloc(sizeof(fbe_board_mgmt_set_PostAndOrReset_async_context_t));

    context->command.sp_id = SP_B;
    context->command.holdInPost = TRUE;
    context->command.holdInReset = FALSE;
    context->command.rebootBlade = TRUE;
    context->completion_function = tom_and_jerry_set_PostAndOrReset_completion;

    /* Set/Clear Hold in Post or Reset. */
    fbeStatus = fbe_api_board_set_PostAndOrReset_async(objectId, context);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_PENDING);

    fbe_semaphore_wait(&cmd_sem, NULL);
    MUT_ASSERT_TRUE(context->status == FBE_STATUS_OK);
    free(context);

    /* Wait 1 minute for misc data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.peerHoldInPost == TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified Misc Data Change ===\n");

    return;
}

static void tom_and_jerry_set_PostAndOrReset_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context)
{
    fbe_semaphore_release(&cmd_sem, 0, 1 ,FALSE);
}


static void tom_and_jerry_test_verify_io_module_data_change(fbe_object_id_t objectId)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t ioModuleInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying IO Module Data Change ===\n");

    /* Get the IO module  B0 info. */
    ioModuleInfo.associatedSp = SP_B;
    ioModuleInfo.slotNumOnBlade = 0;
    fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.faultLEDStatus == LED_BLINK_OFF);

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_IOMODULE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;
    expected_phys_location.sp = SP_B;
    expected_phys_location.slot = 0;

    /* Change the io module led blink rate. */
    fbeStatus = fbe_api_board_set_IOModuleFaultLED(objectId, SP_B, 0, FBE_DEVICE_TYPE_IOMODULE, LED_BLINK_ON);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 1 minute for power supply data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.associatedSp == SP_B);
    MUT_ASSERT_TRUE(ioModuleInfo.slotNumOnBlade == 0);
    /* Re-enable this after SPECL fixes its dummyspeclGetMCUBlinkRate */
    MUT_ASSERT_TRUE(ioModuleInfo.faultLEDStatus == LED_BLINK_ON);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified IO Module Data Change ===\n");

    return;
}

static void tom_and_jerry_test_verify_io_port_data_change(fbe_object_id_t objectId)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_port_info_t ioPortInfo = {0};
    fbe_board_mgmt_set_iom_port_LED_t ioPortLED = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying IO Port Data Change ===\n");

    /* Get the IO module 0 Port 0 info. */
    ioPortInfo.associatedSp = SP_A;
    ioPortInfo.slotNumOnBlade = 0;
    ioPortInfo.portNumOnModule = 3;
    ioPortInfo.deviceType = FBE_DEVICE_TYPE_IOMODULE;
    fbeStatus = fbe_api_board_get_io_port_info(objectId, &ioPortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioPortInfo.ioPortLED == LED_BLINK_OFF);

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_IOMODULE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_PORT_INFO;
    expected_phys_location.sp = SP_B;
    expected_phys_location.slot = 0;
    expected_phys_location.port = 3;

    /* Pass on the SP, Slot and Port information for which the LED has to be changed */
    ioPortLED.iom_LED.sp_id = SP_B;
    ioPortLED.iom_LED.slot = 0;
    ioPortLED.iom_LED.device_type = FBE_DEVICE_TYPE_IOMODULE;
    ioPortLED.io_port = 3;
    ioPortLED.iom_LED.blink_rate = LED_BLINK_ON;
    ioPortLED.led_color = LED_COLOR_TYPE_BLUE;

    /* Change the IO Port led blink rate. */
    fbeStatus = fbe_api_board_set_iom_port_LED(objectId, &ioPortLED);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 1 minute for IO Port data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_io_port_info(objectId, &ioPortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioPortInfo.associatedSp == SP_B);
    MUT_ASSERT_TRUE(ioPortInfo.slotNumOnBlade == 0);
    MUT_ASSERT_TRUE(ioPortInfo.portNumOnModule == 3);
    /* Re-enable this after SPECL fixes its dummyspeclGetMCUBlinkRate */
    //MUT_ASSERT_TRUE(ioPortInfo.ioPortLED == LED_BLINK_ON);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified IO Port Data Change ===\n");

    return;
}

static void tom_and_jerry_test_verify_mgmt_module_data_change(fbe_object_id_t objectId)
{
    DWORD                   dwWaitResult;
    fbe_u32_t               componentCountPerBlade = 0;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_mgmt_module_info_t mgmtModuleInfo = {0};
    fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *vlan_config_context;
    fbe_board_mgmt_set_mgmt_port_async_context_t *mgmt_port_context;

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Mgmt Module Data Change ===\n");

    fbeStatus = fbe_api_board_get_mgmt_module_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* For SENTRY_CPU_MODULE, we don't have Mgmt module and Count will be Zero. 
     * We only need to exercise this code only if the component count is > 0
     */
    if(componentCountPerBlade > 0)
    {
    /* Get the management module  B0 info. */
    mgmtModuleInfo.associatedSp = SP_B;
    mgmtModuleInfo.mgmtID = 0;
    fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmtModuleInfo.faultLEDStatus == LED_BLINK_OFF);
    MUT_ASSERT_TRUE(mgmtModuleInfo.vLanConfigMode == CLARiiON_VLAN_MODE);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.linkStatus == FALSE);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortSpeed == SPEED_10_Mbps);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortDuplex == FULL_DUPLEX);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_ON);

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_MGMT_MODULE;
    expected_phys_location.sp = SP_B;
    expected_phys_location.slot = 0;

    /* Change the Mgmt module LED blink rate. */
    fbeStatus = fbe_api_board_set_mgmt_module_fault_LED(objectId, SP_B, LED_BLINK_ON);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 1 minute for power supply data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmtModuleInfo.associatedSp == SP_B);
    /* Re-enable this after SPECL fixes its dummyspeclGetMCUBlinkRate */
    //MUT_ASSERT_TRUE(mgmtModuleInfo.faultLEDStatus == LED_BLINK_ON);

    /* Change the management module vLan config mode. */
    vlan_config_context = malloc(sizeof(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t));
    MUT_ASSERT_TRUE(vlan_config_context != NULL);

    vlan_config_context->command.sp_id = SP_B;
    vlan_config_context->command.vlan_config_mode = CUSTOM_MGMT_ONLY_VLAN_MODE;
    vlan_config_context->completion_function = tom_and_jerry_test_verify_mgmt_module_vlan_mode_completion;
    fbeStatus = fbe_api_board_set_mgmt_module_vlan_config_mode_async(objectId, vlan_config_context);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_PENDING);
    fbe_semaphore_wait(&cmd_sem, NULL);
    MUT_ASSERT_TRUE(vlan_config_context->status == FBE_STATUS_OK);
    free(vlan_config_context);


    /* Wait 1 minute for management module data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmtModuleInfo.associatedSp == SP_B);
    MUT_ASSERT_TRUE(mgmtModuleInfo.vLanConfigMode == CUSTOM_MGMT_ONLY_VLAN_MODE);

    mgmt_port_context = malloc(sizeof(fbe_board_mgmt_set_mgmt_port_async_context_t));
    MUT_ASSERT_TRUE(mgmt_port_context != NULL);

    /* Set the Management port Information */ 
    mgmt_port_context->command.sp_id = SP_B;
    mgmt_port_context->command.portIDType = MANAGEMENT_PORT0;
    mgmt_port_context->command.mgmtPortConfig.mgmtPortAutoNeg = FALSE;
    mgmt_port_context->command.mgmtPortConfig.mgmtPortSpeed = SPEED_100_Mbps;
    mgmt_port_context->command.mgmtPortConfig.mgmtPortDuplex = HALF_DUPLEX;
    mgmt_port_context->completion_function = tom_and_jerry_test_verify_set_mgmt_port_completion;
    
    /* Change the management module Port Information. */
    fbeStatus = fbe_api_board_set_mgmt_module_port_async(objectId, mgmt_port_context);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_PENDING);
    fbe_semaphore_wait(&cmd_sem, NULL);
    MUT_ASSERT_TRUE(mgmt_port_context->status == FBE_STATUS_OK);
    free(mgmt_port_context);


    /* Wait 1 minute for management module data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Verify the status */
    fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmtModuleInfo.associatedSp == SP_B);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_OFF);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortSpeed == SPEED_100_Mbps);
    MUT_ASSERT_TRUE(mgmtModuleInfo.managementPort.mgmtPortDuplex == HALF_DUPLEX);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified Mgmt Module Data Change ===\n");
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO Mgmt Module found for this (SENTRY) CPU module\n");
    }

    return;
}

void tom_and_jerry_test_verify_mgmt_module_vlan_mode_completion(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *command_context)
{
    fbe_semaphore_release(&cmd_sem, 0, 1 ,FALSE);
    return;
}

void tom_and_jerry_test_verify_set_mgmt_port_completion(fbe_board_mgmt_set_mgmt_port_async_context_t *command_context)
{
    fbe_semaphore_release(&cmd_sem, 0, 1 ,FALSE);
    return;
}


