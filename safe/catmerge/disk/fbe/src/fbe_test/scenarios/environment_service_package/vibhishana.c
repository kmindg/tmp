/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file vibhishana_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify ESP module_mgmt object Mgmt port speed change. 
 * 
 * @version
 *   02-Jun-2011 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "esp_tests.h"
#include "sep_tests.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_object.h"
#include "fbe_test_esp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fp_test.h"
#include "sep_tests.h"

char * vibhishana_short_desc = "Test management port speed persistence across reboot";
char * vibhishana_long_desc ="\
This tests validates the module management object mgmt port speed get configured using persistence data \n\
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
STEP 2: During first initialization of ESP; port get auto configured \n\
STEP 3: Validate the mgmt port speed change.\n\
STEP 4: Restart the ESP check the port speed";


#define FBE_API_POLLING_INTERVAL 100 /*ms*/
#define VIBHISHANA_RAMAN_MGMTID_0   0
fbe_semaphore_t vibhishana_sem;

/*******************************
 *   local function prototypes
 *******************************/
void vibhishana_esp_restart(void);
static void vibhishana_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                                void *context);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!*************************************************************
 * vibhishana_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test Mgmt port speed change
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2-June-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void vibhishana_test(void)
{
    DWORD           dwWaitResult;
    fbe_bool_t      is_msg_present = FBE_FALSE;
    fbe_status_t    status;
    fbe_notification_registration_id_t          reg_id;
    fbe_esp_module_mgmt_get_mgmt_comp_info_t    mgmt_comp_info;
    fbe_esp_module_mgmt_set_mgmt_port_config_t   mgmt_port_config;

    mut_printf(MUT_LOG_LOW, "Vibhishana test: started...\n");

    /* Check for event message */
    status = fbe_test_esp_check_mgmt_config_completion_event();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_init(&vibhishana_sem, 0, 1);
    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  vibhishana_esp_object_data_change_callback_function,
                                                                  &vibhishana_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Init the info header */
    mgmt_comp_info.phys_location.sp = SP_A;
    mgmt_comp_info.phys_location.slot = VIBHISHANA_RAMAN_MGMTID_0;
     /* Init MGMT ID*/
    mgmt_port_config.phys_location.slot = VIBHISHANA_RAMAN_MGMTID_0;

    /* Verify the mgmt port speed change */
    mgmt_port_config.revert = FBE_TRUE;
    mgmt_port_config.mgmtPortRequest.mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_OFF;
    mgmt_port_config.mgmtPortRequest.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_100MBPS;
    mgmt_port_config.mgmtPortRequest.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_HALF;
    status = fbe_api_esp_module_mgmt_set_mgmt_port_config(&mgmt_port_config);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check for event message */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_MGMT_PORT_CONFIG_COMMAND_RECEIVED,
                                         "SPA management module 0",
                                         "OFF",
                                         "100Mbps",
                                         "HALF DUPLEX");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "Vibhishana test: user requested management port speed change...\n");

    mut_printf(MUT_LOG_LOW, "Waiting for port speed get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&vibhishana_sem, 30000);   

    /* Verify the port speed */
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortSpeed == FBE_MGMT_PORT_SPEED_100MBPS);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortDuplex == FBE_PORT_DUPLEX_MODE_HALF);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_OFF);
    /* Check for event message */
    status = fbe_test_esp_check_mgmt_config_completion_event();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Restart ESP */
    mut_printf(MUT_LOG_LOW, "Vibhishana test: ***** Restart ESP ***** \n");
    fbe_test_esp_restart();
    
    /* Check for event message */
    status = fbe_test_esp_check_mgmt_config_completion_event();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Verify the port speed, it should same as previouly configured*/
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortSpeed == FBE_MGMT_PORT_SPEED_100MBPS);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortDuplex == FBE_PORT_DUPLEX_MODE_HALF);
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_OFF);

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(vibhishana_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&vibhishana_sem);    
    mut_printf(MUT_LOG_LOW, "Vibhishana test: completed successfully...\n");
    return;
}
/******************************
 *  end of vibhishana_test
 *****************************/
/*!*************************************************************
 *  vibhishana_setup() 
 ****************************************************************
 * @brief
 *  Initiate the setup required to run the test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2-June-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void vibhishana_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_PROMETHEUS_M1_HW_TYPE,
                                        NULL,
                                        NULL);
}
/******************************
 *  end of vibhishana_setup
 *****************************/
/*!*************************************************************
 *  vibhishana_destroy() 
 ****************************************************************
 * @brief
 *  Unload the packages.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2-June-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
void vibhishana_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************
 *  end of vibhishana_destroy
 *****************************/

 /*!*************************************************************
 *  vibhishana_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  This is notification function for module_mgmt data change
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  2-June-2011 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/ 
static void 
vibhishana_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
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
 *  end of vibhishana_esp_object_data_change_callback_function
 *****************************************************************/
