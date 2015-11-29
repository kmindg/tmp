/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file tenali_raman_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify ESP module_mgmt control command
 * 
 * @version
 *   25-Mar-2011 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "esp_tests.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "specl_types.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_test_esp_utils.h"
#include "fp_test.h"
#include "sep_tests.h"

char * tenali_raman_short_desc = "module_mgmt vLan config test and mgmt port speed set test";
char * tenali_raman_long_desc ="\
This tests validates the module management object vLan config and mgmt port speed change \n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper drive\n\
        [PP] 3 SAS drives/drive\n\
        [PP] 3 logical drives/drive\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 2: Validate the vLan config mode setup\n\
STEP 3: Validate the mgmt port speed change";

#define TENALI_RAMAN_MGMTID_0   0
fbe_semaphore_t tenali_raman_sem;
static void tenali_raman_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                                  void *context);
static fbe_status_t tenali_raman_specl_setup(void);

/*!*************************************************************
 *  tenali_raman_test() 
 ****************************************************************
 * @brief
 *  Entry function for Tenali Raman test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Mar-2011- Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void tenali_raman_test()
{
    DWORD           dwWaitResult;
    fbe_bool_t      is_msg_present = FBE_FALSE;
    fbe_status_t    status;
    fbe_notification_registration_id_t          reg_id;
    fbe_esp_module_mgmt_get_mgmt_comp_info_t    mgmt_comp_info;
    fbe_esp_module_mgmt_set_mgmt_port_config_t   mgmt_port_config;

    mut_printf(MUT_LOG_LOW, "Tenali Raman test: started...\n");

    /* Check for event message */
    status = fbe_test_esp_check_mgmt_config_completion_event();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    fbe_semaphore_init(&tenali_raman_sem, 0, 1);
    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  tenali_raman_esp_object_data_change_callback_function,
                                                                  &tenali_raman_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Init the info header */
    mgmt_comp_info.phys_location.sp = SP_A;
    mgmt_comp_info.phys_location.slot = TENALI_RAMAN_MGMTID_0;
     /* Init MGMT ID*/
    mgmt_port_config.phys_location.slot = TENALI_RAMAN_MGMTID_0;

    /* AutoNegotiate mode should be enable */
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_ON);

    /* Verify the mgmt port speed change */
    mgmt_port_config.revert = FBE_TRUE;
    mgmt_port_config.mgmtPortRequest.mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_OFF;
    mgmt_port_config.mgmtPortRequest.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_10MBPS;
    mgmt_port_config.mgmtPortRequest.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_HALF;
    status = fbe_api_esp_module_mgmt_set_mgmt_port_config(&mgmt_port_config);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Waiting for port speed get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tenali_raman_sem, 30000);
    /* Verify the port speed */
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortSpeed == FBE_MGMT_PORT_SPEED_10MBPS);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortDuplex == FBE_PORT_DUPLEX_MODE_HALF);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_OFF);
    /* Check for event message */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_MGMT_PORT_CONFIG_COMPLETED,
                                         "SPA management module 0");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "Tenali Raman test: mgmt port speed change verified successfully...\n");

    /* Verify vLan config mode set to CLARiiON_VLAN_MODE when esp loaded */
    mgmt_comp_info.phys_location.sp = SP_A;
    mgmt_comp_info.phys_location.slot = TENALI_RAMAN_MGMTID_0;
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.vLanConfigMode == CLARiiON_VLAN_MODE);
    mut_printf(MUT_LOG_LOW, "Tenali Raman test: Set vLan config mode to CLARiiON_VLAN_MODE completed successfully\n");

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(tenali_raman_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&tenali_raman_sem);

    mut_printf(MUT_LOG_LOW, "Tenali Raman test: completed successfully...\n");
    return;
}
/***********************************
 *  end of tenali_raman_test()
 ***********************************/
/*!*************************************************************
 *  tenali_raman_setup() 
 ****************************************************************
 * @brief
 *  Initiate the setup required to run the test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Mar-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void tenali_raman_setup(void)
{

    fbe_status_t status;

    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    fp_test_start_physical_package();

    tenali_raman_specl_setup();
    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
    
    return;
}
/***********************************
 *  end of tenali_raman_setup()
 **********************************/

static fbe_status_t tenali_raman_specl_setup(void)
{
    fbe_status_t status;
    SPECL_SFI_MASK_DATA sfi_mask_data;

    /* Set the vLan config mode to CUSTOM_MGMT_ONLY_VLAN_MODE
     * so when ESP loaded; verify it set to CLARiiON_VLAN_MODE mode
     */
    sfi_mask_data.structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the vLan mode */
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[TENALI_RAMAN_MGMTID_0].mgmtStatus[SP_A].vLanConfigMode = CUSTOM_MGMT_ONLY_VLAN_MODE;
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get data and verify it*/
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[TENALI_RAMAN_MGMTID_0].mgmtStatus[SP_A].vLanConfigMode == CUSTOM_MGMT_ONLY_VLAN_MODE);

    return FBE_STATUS_OK;

}
/*!*************************************************************
 *  tenali_raman_destroy() 
 ****************************************************************
 * @brief
 *  Unload the packages.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  25-Mar-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void tenali_raman_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/*************************************************
 *  end of tenali_raman_destroy()
 ************************************************/
/*!*************************************************************
 *  tenali_raman_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  This is notification function for module_mgmt data change
 *
 * @param update_object_msg - object message.
 * @param context - notification context
 *
 * @return None.
 *
 * @author
 *  25-Mar-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
static void 
tenali_raman_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                      void *context)
{
    fbe_status_t    status;
    fbe_object_id_t expected_object_id = FBE_OBJECT_ID_INVALID;
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t   device_mask;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    if (update_object_msg->object_id == expected_object_id &&
        (device_mask & FBE_DEVICE_TYPE_MGMT_MODULE) == FBE_DEVICE_TYPE_MGMT_MODULE) 
    {
        if((update_object_msg->notification_info.notification_data.data_change_info.phys_location.sp == SP_A) &&
           (update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot == 0))
        {
           fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
    return;
}
/******************************************************************
 *  end of tenali_raman_esp_object_data_change_callback_function
 *****************************************************************/
