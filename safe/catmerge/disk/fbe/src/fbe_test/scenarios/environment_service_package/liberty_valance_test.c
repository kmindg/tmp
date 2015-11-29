/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file liberty_valance_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the suppressing of PS Faults/Removals during PS FW update.
 * 
 * @version
 *   01/16/2012 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_base_environment_debug.h"

#include "fbe_test_configurations.h"

#include "physical_package_tests.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_eses.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * liberty_valance_short_desc = "Verify the suppressing of PS Faults/Removals during PS FW update";
char * liberty_valance_long_desc ="\
Liberty Valance\n\
        -lead character in the movie THE MAN WHO SHOT LIBERTY VALANCE\n\
        -famous quote 'You heard him Dude.  Pick it up.'\n\
                      'What's the matter Mr. Marshal?  Someone have an accident?'\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 2: Validate that the periodic SPS Test time is detected\n\
        - Verify that PS OverTemp conditions are detected\n\
        - Verify that PE Shutdown condition is detected\n\
        - Verify that DAE Shutdown condition is detected\n\
";

static fbe_object_id_t              expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_u64_t                    expected_device_type = FBE_DEVICE_TYPE_INVALID;
// notification was not beinbg used so comment out
// static fbe_notification_registration_id_t   reg_id;

typedef void (update_fault_function_t) (fbe_device_physical_location_t *locationPtr, fbe_bool_t fault);

#define RM_RETRY_COUNT      3


static void liberty_valance_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
    fbe_semaphore_t         *sem = (fbe_semaphore_t *)context;

//  mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===",update_object_msg->object_id, expected_object_id);
    if (update_object_msg->object_id == expected_object_id) 
    {
//      mut_printf(MUT_LOG_LOW, "callback, device_mask: actual 0x%llx, expected 0x%llx ===",
//                 update_object_msg->notification_info.notification_data.data_change_info.device_mask, expected_device_type);
        if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == expected_device_type)
        {
            mut_printf(MUT_LOG_LOW, "%s, release semaphore===", __FUNCTION__);
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
}

void liberty_valance_updatePsFault(fbe_device_physical_location_t *locationPtr, fbe_bool_t fault)
{
    ses_stat_elem_ps_struct              ps_struct;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t   encl_device_id;

    mut_printf(MUT_LOG_LOW, "%s, %d_%d Fault PS %d, fault %d\n",
               __FUNCTION__, locationPtr->bus, locationPtr->enclosure, locationPtr->slot, fault);
    status = fbe_api_terminator_get_enclosure_handle(locationPtr->bus, locationPtr->enclosure, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_ps_eses_status(encl_device_id, locationPtr->slot, &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (fault)
    {
        ps_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    }
    else
    {
        ps_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    status = fbe_api_terminator_set_ps_eses_status(encl_device_id, locationPtr->slot, ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of liberty_valance_updatePsFault

void liberty_valance_updateLccFault(fbe_device_physical_location_t *locationPtr, fbe_bool_t fault)
{
    ses_stat_elem_encl_struct            lcc_struct;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t   encl_device_id;

    mut_printf(MUT_LOG_LOW, "%s, %d_%d Fault LCC %d, fault %d\n",
               __FUNCTION__, locationPtr->bus, locationPtr->enclosure, locationPtr->slot, fault);
    status = fbe_api_terminator_get_enclosure_handle(locationPtr->bus, locationPtr->enclosure, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_lcc_eses_status(encl_device_id, &lcc_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (fault)
    {
        lcc_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    }
    else
    {
        lcc_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    status = fbe_api_terminator_set_lcc_eses_status(encl_device_id, lcc_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of liberty_valance_updateLccFault

void liberty_valance_updateFanFault(fbe_device_physical_location_t *locationPtr, fbe_bool_t fault)
{
    ses_stat_elem_cooling_struct         cooling_stat;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t   encl_device_id;

    mut_printf(MUT_LOG_LOW, "%s, %d_%d Fault Fan %d, fault %d\n",
               __FUNCTION__, locationPtr->bus, locationPtr->enclosure, locationPtr->slot, fault);
    status = fbe_api_terminator_get_enclosure_handle(locationPtr->bus, locationPtr->enclosure, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_cooling_eses_status(encl_device_id, locationPtr->slot, &cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (fault)
    {
        cooling_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    }
    else
    {
        cooling_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    status = fbe_api_terminator_set_cooling_eses_status(encl_device_id, locationPtr->slot, cooling_stat);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of liberty_valance_updateFanFault

void liberty_valance_verifyFaultSuppressedDuringFup(fbe_u64_t deviceType)
{
    fbe_device_physical_location_t       location = {0};
    fbe_u32_t                            forceFlags = 0;
    fbe_api_terminator_device_handle_t   terminatorEnclHandle = NULL;
    fbe_base_env_fup_work_state_t        expectedWorkState;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_semaphore_t                      sem;
    fbe_bool_t                           isMsgPresent = FALSE;
    fbe_u8_t                             deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    update_fault_function_t              *faultFunctionPtr;

    fbe_semaphore_init(&sem, 0, 1);
    /*
     * Initialize to get event notification for both SP's
     */
#if 0 // notification is not being used so this is commented out
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                    FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                    FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                    liberty_valance_commmand_response_callback_function,
                                                                    &sem,
                                                                    &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
#endif
    /* 
     * Initialize for FUP
     */
    fbe_test_esp_create_registry_image_path(deviceType);
    fbe_test_esp_create_image_file(deviceType);

    /*
     * Perform Test (start FUP, force fault, verify that fault suppressed
     */
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    forceFlags = FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE | 
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK |
                 FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK;

    switch (deviceType)
    {
    case FBE_DEVICE_TYPE_PS:
        mut_printf(MUT_LOG_LOW, "LIBERTY_VALANCE: Verify no PS faults during a PS FW update\n");
        faultFunctionPtr = &liberty_valance_updatePsFault;
        break;
    case FBE_DEVICE_TYPE_LCC:
        mut_printf(MUT_LOG_LOW, "LIBERTY_VALANCE: Verify no LCC faults during a LCC FW update\n");
        faultFunctionPtr = &liberty_valance_updateLccFault;
        break;
    case FBE_DEVICE_TYPE_FAN:
        mut_printf(MUT_LOG_LOW, "LIBERTY_VALANCE: Verify no Fan faults during a Fan FW update\n");
        faultFunctionPtr = &liberty_valance_updateFanFault;
        break;
    default:
        mut_printf(MUT_LOG_LOW, "LIBERTY_VALANCE: deviceType %lld not supported\n", deviceType);
        MUT_FAIL();
        break;
    }

    // setup expected notificaiton info
    expected_device_type = deviceType;
    status = fbe_api_esp_common_get_object_id_by_device_type(expected_device_type, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s, expected_device_type 0x%llx, expected_object_id 0x%x\n",
               __FUNCTION__, expected_device_type, expected_object_id);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    /* Set up the upgrade environment. */
    fbe_test_esp_setup_terminator_upgrade_env(&location, 0, 0, TRUE);

    /* Get the terminator enclosure handle. */
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &terminatorEnclHandle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Initiate power supply firmware upgrade.
     */
    fbe_test_esp_initiate_upgrade(deviceType, &location, forceFlags);
   
    /* Wait for Activate to start.
     */
    expectedWorkState = FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT;
    mut_printf(MUT_LOG_LOW, "%s, waiting for workstate %d\n",
               __FUNCTION__,FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT);
    status = fbe_test_esp_wait_for_fup_work_state(deviceType, 
                                                  &location,
                                                  expectedWorkState,
                                                  10000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
        "Wait for FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT failed!");
    mut_printf(MUT_LOG_LOW, "%s, %d_%d PS %d, ActivateImageSent detected\n",
               __FUNCTION__, location.bus, location.enclosure, location.slot);

    /* Force the appropriate Fault during Activate
     */
    faultFunctionPtr(&location, TRUE);

    fbe_api_sleep(500);

    /* Clear PS Fault during Activate
     */
    faultFunctionPtr(&location, FALSE);

    /* Wait for FUP to complete
     */
    fbe_test_esp_verify_fup_completion_status(deviceType, 
                                              &location, 
                                              FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED);

    /* Verify that no PS Fault event generated
     */
    status = fbe_base_env_create_device_string(deviceType, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;

    switch (deviceType)
    {
    case FBE_DEVICE_TYPE_PS:
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_ERROR_PS_FAULTED,
                                             &deviceStr[0], "GenFlt");
        break;
    case FBE_DEVICE_TYPE_LCC:
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_ERROR_LCC_FAULTED,
                                             &deviceStr[0], "Not Available", "Not Available");
        break;
    case FBE_DEVICE_TYPE_FAN:
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_ERROR_FAN_FAULTED,
                                             &deviceStr[0], "");
        break;
    default:
        mut_printf(MUT_LOG_LOW, "LIBERTY_VALANCE: deviceType %lld not supported\n", deviceType);
        MUT_FAIL();
        break;
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FALSE, isMsgPresent, 
                             "Fault Detected Event log msg present!");
    mut_printf(MUT_LOG_LOW, "%s Fault Detected Event log msg not found\n", &deviceStr[0]);

    /* 
     * Cleanup from FUP
     */
    fbe_test_esp_delete_image_file(deviceType);

    // cleanup
#if 0 // notification is not being used so this is commented out
    status = fbe_api_notification_interface_unregister_notification(liberty_valance_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
#endif
    fbe_semaphore_destroy(&sem);

}   // end of liberty_valance_verifyFaultSuppressedDuringFup

/*!**************************************************************
 * liberty_valance_viper_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Viper FW updated & suppression
 *  of errors. 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   01/16/2012 - Created. Joe Perry
 *
 ****************************************************************/
void liberty_valance_viper_test(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VIPER) - testing on Viper Enclosures\n");

    /*
     * Verify that PS Faults are suppressed during PS FW Update
     */
    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VIPER) - test suppressing PS faults during PW FUP\n");
    liberty_valance_verifyFaultSuppressedDuringFup(FBE_DEVICE_TYPE_PS);

#if TRUE
    /*
     * Verify that LCC Faults are suppressed during LCC FW Update
     */
    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VIPER) - test suppressing LCC faults during LCC FUP\n");
    liberty_valance_verifyFaultSuppressedDuringFup(FBE_DEVICE_TYPE_LCC);
#endif

    return;

}   // end of liberty_valance_viper_test

/*!**************************************************************
 * liberty_valance_voyager_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Voyager FW updated & suppression
 *  of errors. 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   01/16/2012 - Created. Joe Perry
 *
 ****************************************************************/
void liberty_valance_voyager_test(void)
{
    fbe_status_t status;
//    fbe_device_physical_location_t  location;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VOYAGER) - testing on Voyager Enclosures\n");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

// test
#if FALSE
    fbe_api_sleep(5000);

    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    liberty_valance_updateFanFault(&location, TRUE);
    fbe_api_sleep(5000);

    location.slot = 1;
    liberty_valance_updateFanFault(&location, TRUE);
    fbe_api_sleep(5000);

    location.slot = 2;
    liberty_valance_updateFanFault(&location, TRUE);
    fbe_api_sleep(5000);
#endif

#if TRUE
    /*
     * Verify that PS Faults are suppressed during PS FW Update
     */
    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VOYAGER) - test suppressing PS faults during PW FUP\n");
    liberty_valance_verifyFaultSuppressedDuringFup(FBE_DEVICE_TYPE_PS);

    /*
     * Verify that LCC Faults are suppressed during LCC FW Update
     */
    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VOYAGER) - test suppressing LCC faults during LCC FUP\n");
    liberty_valance_verifyFaultSuppressedDuringFup(FBE_DEVICE_TYPE_LCC);

    /*
     * Verify that Fan Faults are suppressed during Fan FW Update
     */
    mut_printf(MUT_LOG_LOW, "LIBERTY VALANCE(VOYAGER) - test suppressing Fan faults during Fan FUP\n");
    liberty_valance_verifyFaultSuppressedDuringFup(FBE_DEVICE_TYPE_FAN);
#endif

    return;

}   // end of liberty_valance_voyager_test

/*
 * Setup functions
 */
void liberty_valance_viper_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VIPER_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}   // end of liberty_valance_viper_setup

void liberty_valance_voyager_setup(void)
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
        if(pNaxosTest->encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)
        {
            status = naxos_load_and_verify_table_driven(pNaxosTest);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        }
    }
    
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);

}   // end of liberty_valance_voyager_setup

void liberty_valance_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}

/*************************
 * end file liberty_valance_test.c
 *************************/

