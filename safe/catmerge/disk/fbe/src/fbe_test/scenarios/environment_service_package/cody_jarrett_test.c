/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cody_jarrett_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify OverTemp & enclosure Shutdown in ESP.
 * 
 * @version
 *   10/18/2011 - Created. Joe Perry
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

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * cody_jarrett_short_desc = "Verify OverTemp & enclosure shutdown processing in ESP";
char * cody_jarrett_long_desc ="\
Cody Jarrett\n\
        -lead character in the movie WHITE HEAT\n\
        -famous quote 'Made it Ma!  Top of the world!'\n\
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

static fbe_object_id_t	                    expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_u64_t                    expected_device_type = FBE_DEVICE_TYPE_INVALID;
static fbe_notification_registration_id_t   reg_id;


static void cody_jarrett_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===",
        update_object_msg->object_id, expected_object_id);

	if (update_object_msg->object_id == expected_object_id) 
	{
    	mut_printf(MUT_LOG_LOW, "callback, device_mask: actual 0x%llx, expected 0x%llx ===",
            update_object_msg->notification_info.notification_data.data_change_info.device_mask, expected_device_type);
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == expected_device_type)
	    {
    		fbe_semaphore_release(sem, 0, 1 ,FALSE);
        } 
	}
}

void cody_jarrett_updateSuitcaseShutdownWarning(SP_ID spid, fbe_bool_t shutdownWarning)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
    fbe_u32_t                               suitcaseIndex;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, force shutdownWarning %d\n",
               __FUNCTION__, spid, shutdownWarning);
    if (spid == SP_A)
    {
        suitcaseIndex = 0;
    }
    else
    {
        suitcaseIndex = 1;
    }
    sfi_mask_data.structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.suitcaseSummary[spid].suitcaseStatus[0].shutdownWarning = shutdownWarning;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of cody_jarrett_updateSuitcaseShutdownWarning

void cody_jarrett_updatePePsOverTemp(SP_ID spid, fbe_u32_t psIndex, fbe_bool_t psOverTemp)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, ps %d, force psOverTemp %d\n",
               __FUNCTION__, spid, psIndex, psOverTemp);

    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spid].psStatus[psIndex].ambientOvertempFault = 
        psOverTemp;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of cody_jarrett_updatePePsOverTemp

void cody_jarrett_updateDae0PsOverTemp(fbe_u32_t psIndex, fbe_bool_t psOverTemp)
{
    ses_stat_elem_ps_struct                 ps_struct;
    fbe_api_terminator_device_handle_t      encl_device_id;
    fbe_status_t                            status;

    mut_printf(MUT_LOG_LOW, "  %s, ps %d, force psOverTemp %d\n",
               __FUNCTION__, psIndex, psOverTemp);

    status = fbe_api_terminator_get_enclosure_handle(0, 0, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_ps_eses_status(encl_device_id, psIndex, &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set OverTemp fault
    ps_struct.overtmp_fail = psOverTemp;
    status = fbe_api_terminator_set_ps_eses_status(encl_device_id, psIndex, ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of cody_jarrett_updateDae0PsOverTemp

void cody_jarrett_updateEnclShutdownReason(fbe_device_physical_location_t *location,
                                           fbe_u8_t shutdownReason)
{
    ses_pg_emc_encl_stat_struct             emcEnclStatus;
    fbe_api_terminator_device_handle_t      encl_device_id;
    fbe_status_t                            status;

    mut_printf(MUT_LOG_LOW, "  %s, bus %d, encl %d, force ShutdownReason %d\n",
               __FUNCTION__, location->bus, location->enclosure, shutdownReason);

    status = fbe_api_terminator_get_enclosure_handle(location->bus, 
                                                     location->enclosure, 
                                                     &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_emcEnclStatus(encl_device_id, &emcEnclStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // set ShutdownReason
    emcEnclStatus.shutdown_reason = shutdownReason;
    status = fbe_api_terminator_set_emcEnclStatus(encl_device_id, &emcEnclStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_emcEnclStatus(encl_device_id, &emcEnclStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of cody_jarrett_updateDae0ShutdownReason

void cody_jarrett_testPsOverTemp(fbe_semaphore_t *semPtr,
                                 fbe_bool_t xpeArray,
                                 SP_ID firstPsSp,
                                 fbe_u32_t firstPsIndex,
                                 SP_ID secondPsSp,
                                 fbe_u32_t secondPsIndex)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_esp_ps_mgmt_ps_info_t               psInfo;
    fbe_bool_t                              isMsgPresent = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];

    // setup expected notificaiton info
    expected_device_type = FBE_DEVICE_TYPE_PS;
    status = fbe_api_esp_common_get_object_id_by_device_type(expected_device_type, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    // for first PS OverTemp (DPE) & verify
    cody_jarrett_updatePePsOverTemp(firstPsSp, firstPsIndex, TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, set first PS OverTemp & wait for notification ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    if (xpeArray)
    {
        psInfo.location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        psInfo.location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        psInfo.location.bus = 0;
        psInfo.location.enclosure = 0;
    }
    psInfo.location.slot = firstPsIndex;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    if (!psInfo.psInfo.ambientOvertempFault)
    {
        dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }
    MUT_ASSERT_TRUE(psInfo.psInfo.ambientOvertempFault)

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_PS, 
                                               &psInfo.location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_PS_OVER_TEMP_DETECTED,
                                         &deviceStr[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "PS fault (OverTemp) Detected Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s fault (OverTemp) Detected Event log msg found\n", &deviceStr[0]);

    // for second PS OverTemp (DPE) & verify
    cody_jarrett_updatePePsOverTemp(secondPsSp, secondPsIndex, TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, set second PS OverTemp & wait for notification ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    psInfo.location.slot = secondPsIndex;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.psInfo.ambientOvertempFault)

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_PS, 
                                               &psInfo.location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_PS_OVER_TEMP_DETECTED,
                                         &deviceStr[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "PS fault (OverTemp) Detected Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s fault (OverTemp) Detected Event log msg found\n", &deviceStr[0]);

    // clear PS OverTemp's
    cody_jarrett_updatePePsOverTemp(firstPsSp, firstPsIndex, FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, clear first PS OverTemp & wait for notification ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    cody_jarrett_updatePePsOverTemp(secondPsSp, secondPsIndex, FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, clear second PS OverTemp & wait for notification ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

}   // end of cody_jarrett_testPsOverTemp

void cody_jarrett_testDpeShutdown(fbe_semaphore_t *semPtr)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_esp_board_mgmt_suitcaseInfo_t       suitcaseInfo;
    fbe_bool_t                              isMsgPresent = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];

    expected_device_type = FBE_DEVICE_TYPE_SUITCASE;
    status = fbe_api_esp_common_get_object_id_by_device_type(expected_device_type, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    cody_jarrett_updateSuitcaseShutdownWarning(SP_A, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, set ShutdownWarning & wait for notification ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    suitcaseInfo.location.sp = SP_A;
    suitcaseInfo.location.slot = 0;
    status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(suitcaseInfo.suitcaseInfo.shutdownWarning);

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SUITCASE, 
                                               &suitcaseInfo.location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_ENCL_SHUTDOWN_DETECTED,
                                         &deviceStr[0], "Suitcase");

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, 
                             "Suitcase Shutdown Detected Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s Shutdown Detected Event log msg found\n", &deviceStr[0]);

    cody_jarrett_updateSuitcaseShutdownWarning(SP_A, FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, clear ShutdownWarning & wait for notification ===", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_INFO_ENCL_SHUTDOWN_CLEARED,
                                         &deviceStr[0]);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, 
                             "Suitcase Shutdown Cleared Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s Shutdown Cleared Event log msg found\n", &deviceStr[0]);

}   // end of cody_jarrett_testDpeShutdown

void cody_jarrett_testDaeShutdown(fbe_semaphore_t *semPtr,
                                  fbe_device_physical_location_t *location)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_esp_encl_mgmt_encl_info_t           enclInfo;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = 60 * 1000; /* 1 Minute wait*/

    expected_device_type = FBE_DEVICE_TYPE_ENCLOSURE;
    status = fbe_api_esp_common_get_object_id_by_device_type(expected_device_type, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    cody_jarrett_updateEnclShutdownReason(location, FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT);
    mut_printf(MUT_LOG_LOW, "=== %s, set ShutdownReason & wait for notification ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(semPtr, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Wait for the debouncing logic to run..*/
    while(current_time < timeout_ms)
    {
        status = fbe_api_esp_encl_mgmt_get_encl_info(location, &enclInfo);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if(enclInfo.shutdownReason == FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT )
        {
            break;
        }
        current_time = current_time + 200;
        fbe_api_sleep(200);
        
    }

    /* Check to make sure we get the expected response after the debouncing logic is run */
    MUT_ASSERT_INT_EQUAL(enclInfo.shutdownReason, FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT);

}   // end of cody_jarrett_testDaeShutdown

/*!**************************************************************
 * cody_jarrett_sentry_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Sentry platform OverTemp
 *  & DPE Shutdown detection.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   10/18/2011 - Created. Joe Perry
 *
 ****************************************************************/
void cody_jarrett_sentry_test(void)
{
    fbe_status_t                            status;
	fbe_semaphore_t 						sem;

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    fbe_semaphore_init(&sem, 0, 1);

    /*
     * Initialize to get event notification for both SP's
     */
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  cody_jarrett_commmand_response_callback_function,
																  &sem,
																  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Verify multiple PS's reporting OverTemp
     */
    mut_printf(MUT_LOG_LOW, "CODY JARRETT(SENTRY): Test PS OverTemp reporting\n");
    cody_jarrett_testPsOverTemp(&sem, FALSE, SP_A, 0, SP_B, 0);

    /*
     * Verify the DPE reporting Shutdown
     */
    mut_printf(MUT_LOG_LOW, "CODY JARRETT(SENTRY): Test DPE Shutdown reporting\n");
    cody_jarrett_testDpeShutdown(&sem);

    /*
     * Verify a DAE reporting Shutdown
     */
    mut_printf(MUT_LOG_LOW, "CODY JARRETT(SENTRY): Test DAE Shutdown reporting\n");
    mut_printf(MUT_LOG_LOW, "*****  NOT IMPLEMENTED YET *****\n");

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(cody_jarrett_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;

}
/******************************************
 * end cody_jarrett_sentry_test()
 ******************************************/


/*!**************************************************************
 * cody_jarrett_argonaut_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Argonaut platform OverTemp
 *  & Encl Shutdown detection.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   10/18/2011 - Created. Joe Perry
 *
 ****************************************************************/
void cody_jarrett_argonaut_test(void)
{
    fbe_status_t                            status;
	fbe_semaphore_t 						sem;
    fbe_device_physical_location_t          location;

    /* Wait util there is no firmware upgrade in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

     /* Wait util there is no resume prom read in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    fbe_semaphore_init(&sem, 0, 1);

    /*
     * Initialize to get event notification for both SP's
     */
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  cody_jarrett_commmand_response_callback_function,
																  &sem,
																  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Verify multiple PS's reporting OverTemp
     */
    mut_printf(MUT_LOG_LOW, "CODY JARRETT(ARGONAUT): Test PS OverTemp reporting\n");
    cody_jarrett_testPsOverTemp(&sem, TRUE, SP_A, 0, SP_A, 1);

    /*
     * Verify the xPE reporting Shutdown
     */
    mut_printf(MUT_LOG_LOW, "CODY JARRETT(ARGONAUT): Test xPE Shutdown reporting\n");
    mut_printf(MUT_LOG_LOW, "*****  NOT IMPLEMENTED YET *****\n");


    /*
     * Verify a DAE reporting Shutdown
     */
    mut_printf(MUT_LOG_LOW, "CODY JARRETT(ARGONAUT): Test DAE Shutdown reporting\n");
    location.bus = 0;
    location.enclosure = 0;
    cody_jarrett_testDaeShutdown(&sem, &location);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(cody_jarrett_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;

}
/******************************************
 * end cody_jarrett_argonaut_test()
 ******************************************/

/*!**************************************************************
 * cody_jarrett_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   10/18/2011 - Created. Joe Perry
 *
 ****************************************************************/
void cody_jarrett_setup(SPID_HW_TYPE platform_type)
{
    fbe_status_t    status;
    fbe_bool_t      simulateSps;

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    /*
     * we do the setup on SPA side
     */
    fbe_test_startEspAndSep(FBE_SIM_SP_A, platform_type);

    // simulate SPS to stop testing
    status = fbe_api_esp_sps_mgmt_getSimulateSps(&simulateSps);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}
/**************************************
 * end cody_jarrett_setup()
 **************************************/

void cody_jarrett_sentry_setup()
{
    cody_jarrett_setup(SPID_DEFIANT_M1_HW_TYPE);
}

void cody_jarrett_argonaut_setup()
{
    cody_jarrett_setup(SPID_SPITFIRE_HW_TYPE);
}

/*!**************************************************************
 * cody_jarrett_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the cody_jarrett test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   10/18/2011 - Created. Joe Perry
 *
 ****************************************************************/

void cody_jarrett_destroy(void)
{
    fbe_api_sleep(20000);
    fbe_test_esp_delete_registry_file();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end cody_jarrett_destroy()
 ******************************************/
/*************************
 * end file cody_jarrett_test.c
 *************************/

