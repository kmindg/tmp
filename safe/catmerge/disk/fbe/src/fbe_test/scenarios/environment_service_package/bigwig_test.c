/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bigwig_test.c
 ***************************************************************************
 *
 * @brief
 *  This file verifies the DAE degrade cable reporting 
 *
 * @version
 *   26-Aug-2011 PHE - Created.
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

char * bigwig_short_desc = "Test DAE cable status.";
char * bigwig_long_desc =
    "\n"
    "\n"
    "The bigwig test verifies DAE cable status.\n"
    "It includes: \n"
    "    - The DEGRADED cable status;\n"
    "    - The DISABLED cable status;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the commands to updat the sas connector status.\n"
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
    "STEP 2: Test DEGRADE cable.\n"
    "    - Get bus 0 enclosure 1 connector 0 cable status to verify the cable status is GOOD.\n"
    "    - Inject the sas connector status error to degrade the cable status in terminator.\n"
    "    - Wait for the notification of the change for the ESP connector info.\n"
    "    - Verify the new status of the connector is DEGRADED.\n"
    "    - Clear the sas connector status error in terminator.\n"
    "\n"
    "STEP 3: Test DISABLED cable.\n"
    "    - Get bus 0 enclosure 1 connector 0 cable status to verify the cable status is GOOD.\n"
    "    - Inject the sas connector status error to degrade the cable status in terminator.\n"
    "    - Wait for the notification of the change for the ESP connector info.\n"
    "    - Verify the new status of the connector is DISABLED.\n"
    "    - Clear the sas connector status error in terminator.\n"
    "\n"
    "STEP 4: Shutdown the Terminator/Physical package/ESP.\n"
    ;


static fbe_semaphore_t sem;
static void bigwig_test_dae_cable_status(fbe_cable_status_t cableStatus);
static void 
bigwig_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                void *context);

/*!**************************************************************
 * bigwig_test_dae_cable_status() 
 ****************************************************************
 * @brief
 *  Tests the DAE cable status.
 *
 * @param None.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  26-Aug-2011 PHE - Created. 
 *
 ****************************************************************/
static void bigwig_test_dae_cable_status(fbe_cable_status_t cableStatus)
{
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_esp_encl_mgmt_connector_info_t   connectorInfo = {0};
    fbe_bool_t                           isMsgPresent = FALSE;
    fbe_status_t                         status = FBE_STATUS_OK;
    char                                 deviceStr[FBE_DEVICE_STRING_LENGTH];
    ses_stat_elem_sas_conn_struct        sasConnStruct;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    DWORD                                dwWaitResult;
   
    if(cableStatus == FBE_CABLE_STATUS_DEGRADED) 
    {
        mut_printf(MUT_LOG_LOW, "=== Test DEGRADED cable ===");
    }
    else
    {   
        mut_printf(MUT_LOG_LOW, "=== Test DISABLED cable ===");
    }


    /* Clear the event log.
     */ 
    mut_printf(MUT_LOG_LOW, "Clear ESP event log.");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to clear ESP event log!");

    /* Get the ESP connector status to verify it is good.
     */
    deviceType = FBE_DEVICE_TYPE_CONNECTOR;
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;

    mut_printf(MUT_LOG_LOW, "Verify the initial cable status is good");
    
    status = fbe_api_esp_encl_mgmt_get_connector_info(&location, &connectorInfo);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get ESP connector info.");

    MUT_ASSERT_TRUE_MSG(connectorInfo.cableStatus == FBE_CABLE_STATUS_GOOD, "Initial cable status is not good.");

    mut_printf(MUT_LOG_LOW, "Get terminator SAS connector status.");

    /* Get the terminator enclosure handle.
     */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the present eses status before inserting the fault.
     */    
    status = fbe_api_terminator_get_sas_conn_eses_status(terminatorEnclHandle, 
                                                   LOCAL_ENTIRE_CONNECTOR_0, 
                                                   &sasConnStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the status to NON-CRITICAL/CRITICAL
     * ie. degraded/disabled.
     */
    
    if(cableStatus == FBE_CABLE_STATUS_DEGRADED) 
    {
        mut_printf(MUT_LOG_LOW, "Set terminator SAS connector status to NONCRITICAL.");

        sasConnStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_NONCRITICAL;
    }
    else if(cableStatus == FBE_CABLE_STATUS_DISABLED) 
    {
        mut_printf(MUT_LOG_LOW, "Set terminator SAS connector status to CRITICAL.");

        sasConnStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    }

    status = fbe_api_terminator_set_sas_conn_eses_status(terminatorEnclHandle, 
                                                   LOCAL_ENTIRE_CONNECTOR_0, 
                                                   sasConnStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for notification from ESP for the connector status change.
     */
    mut_printf(MUT_LOG_LOW, "Waiting for ESP connector info change");
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);


    mut_printf(MUT_LOG_LOW, "Get ESP Bus 0 Encl 1 Connector 0 info");

    status = fbe_api_esp_encl_mgmt_get_connector_info(&location, &connectorInfo);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get ESP connector info.");

    MUT_ASSERT_TRUE_MSG(connectorInfo.cableStatus == cableStatus, "cable status is not expected.");

    mut_printf(MUT_LOG_LOW, "Cable status verified");

    location.bank = connectorInfo.connectorType;
    location.componentId = connectorInfo.connectorId;
    status = fbe_base_env_create_device_string(deviceType, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to creat device string.");

    /* Check event logging.
     */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_INFO_CONNECTOR_STATUS_CHANGE,
                                         &deviceStr[0],
                                         fbe_base_env_decode_connector_cable_status(FBE_CABLE_STATUS_GOOD),
                                         fbe_base_env_decode_connector_cable_status(cableStatus));

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to check event log msg!");

    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "Event log msg is not present!");

    mut_printf(MUT_LOG_LOW, "Event log message check passed.");

    /* Set terminator sas connector status back to SES_STAT_CODE_OK.
     */
    mut_printf(MUT_LOG_LOW, "Set terminator sas connector status back to SES_STAT_CODE_OK");

    sasConnStruct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;

    status = fbe_api_terminator_set_sas_conn_eses_status(terminatorEnclHandle, 
                                                   LOCAL_ENTIRE_CONNECTOR_0, 
                                                   sasConnStruct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for notification from ESP for the connector status change.
     */
    mut_printf(MUT_LOG_LOW, "Waiting for ESP connector info change");
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_LOW, "Get ESP Bus 0 Encl 1 Connector 0 info");

    status = fbe_api_esp_encl_mgmt_get_connector_info(&location, &connectorInfo);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed to get ESP connector info.");

    MUT_ASSERT_TRUE_MSG(connectorInfo.cableStatus == FBE_CABLE_STATUS_GOOD, "cable status is not good.");

    mut_printf(MUT_LOG_LOW, "Cable status was set back to good.");

    return;
}

 /*!**************************************************************
 *  bigwig_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  Notification callback function for data change
 *
 * @param update_object_msg: Object message
 * @param context: Callback context.               
 *
 * @return None.
 *
 * @author
 *  26-Aug-2011 Created Vaibhav Gaonkar
 *
 ****************************************************************/
static void 
bigwig_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_u64_t    device_mask;
    fbe_object_id_t      expected_object_id;
    fbe_status_t         status;

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    if (update_object_msg->object_id == expected_object_id) 
    {
        if(device_mask == FBE_DEVICE_TYPE_CONNECTOR) 
        {
            mut_printf(MUT_LOG_LOW, "Bigwig test: Get the notification...");
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
    return;
}
/*************************************************************
 *  end of ravana_esp_object_data_change_callback_function()
 ************************************************************/

/*!**************************************************************
 * bigwig_test() 
 ****************************************************************
 * @brief
 *  Main entry point for testing DAE cable status reporting.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  26-Aug-2011 PHE - Created. 
 *
 ****************************************************************/
void bigwig_test(void)
{
    fbe_notification_registration_id_t reg_id;
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
    
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    /* Init the Semaphore */
    fbe_semaphore_init(&sem, 0, 1);

    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  bigwig_esp_object_data_change_callback_function,
                                                                  &sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    bigwig_test_dae_cable_status(FBE_CABLE_STATUS_DEGRADED);
    bigwig_test_dae_cable_status(FBE_CABLE_STATUS_DISABLED);

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(bigwig_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}

void bigwig_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void bigwig_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file bigwig_test.c
 *************************/


