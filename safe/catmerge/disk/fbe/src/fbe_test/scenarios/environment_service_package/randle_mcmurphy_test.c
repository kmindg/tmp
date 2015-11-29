/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file randle_mcmurphy_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify Shutcase & MCU status in ESP.
 * 
 * @version
 *   12/12/2011 - Created. Joe Perry
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
#include "fp_test.h"
#include "sep_tests.h"

#include "fbe_test_configurations.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * randle_mcmurphy_short_desc = "Verify Suitcase & MCU status processing in ESP";
char * randle_mcmurphy_long_desc ="\
Randle McMurphy\n\
        -lead character in the movie ONE FLEW OVER THE CUCKOO's NEST\n\
        -famous quote '5 fights, huh? Rocky Marciano's got 40 and he's a millionaire'\n\
                      'Hit me, Chief, I got the moves! '\n\
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

#define RM_RETRY_COUNT      5


static void randle_mcmurphy_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
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

/*
 * Suitcase Functions
 */
void randle_mcmurphy_updateSuitcaseShutdownWarning(SP_ID spid, fbe_bool_t shutdownWarning)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, force shutdownWarning %d\n",
               __FUNCTION__, spid, shutdownWarning);
    sfi_mask_data.structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.suitcaseSummary[spid].suitcaseStatus[0].shutdownWarning = shutdownWarning;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of randle_mcmurphy_updateSuitcaseShutdownWarning

void randle_mcmurphy_updateSuitcaseAmbientOverTemp(SP_ID spid, fbe_bool_t ambientOverTemp)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
    fbe_u32_t                               suitcaseIndex;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, force ambientOverTemp %d\n",
               __FUNCTION__, spid, ambientOverTemp);
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
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.suitcaseSummary[spid].suitcaseStatus[0].ambientOvertempFault = ambientOverTemp;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of randle_mcmurphy_updateSuitcaseAmbientOverTemp

void randle_mcmurphy_testSuitcaseStatus(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_esp_board_mgmt_suitcaseInfo_t       suitcaseInfo;
//    fbe_esp_ps_mgmt_ps_info_t               psInfo;
    fbe_bool_t                              isMsgPresent = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_u32_t                               retryIndex;
    fbe_semaphore_t                         sem;

    fbe_semaphore_init(&sem, 0, 1);

    /*
     * Initialize to get event notification for both SP's
     */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  randle_mcmurphy_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    expected_device_type = FBE_DEVICE_TYPE_SUITCASE;
    status = fbe_api_esp_common_get_object_id_by_device_type(expected_device_type, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    /*
     * Test Suitcase ShutdownWarning
     */
    // set Suitcase ShutdownWarning
    mut_printf(MUT_LOG_LOW, "=== %s, Test Suitcase ShutdownWarning ===\n", __FUNCTION__);
    randle_mcmurphy_updateSuitcaseShutdownWarning(SP_A, TRUE);

    mut_printf(MUT_LOG_LOW, "%s, set ShutdownWarning & wait for notification\n", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
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
    for (retryIndex = 0; retryIndex < RM_RETRY_COUNT; retryIndex++)
    {
        isMsgPresent = FALSE;
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_ERROR_ENCL_SHUTDOWN_DETECTED,
                                             &deviceStr[0], "Suitcase");
    
        if ((status == FBE_STATUS_OK) && isMsgPresent)
        {
            break;
        }
        fbe_api_sleep(2000);
    }
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, 
                             "Suitcase AmbientOverTemp Detected Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s Shutdown Detected Event log msg found\n", &deviceStr[0]);

#if FALSE
    // check for Suitcase Shutdown (PS OkToShutdown set)
    psInfo.location.bus = 0;
    psInfo.location.enclosure = 0;
    psInfo.location.slot = 0;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "ps %d, psInserted %d, fault %d\n", 
        0,
        psInfo.psInfo.bInserted,
        psInfo.psInfo.generalFault);
#endif

    // clear Suitcase ShutdownWarning
    randle_mcmurphy_updateSuitcaseShutdownWarning(SP_A, FALSE);
    mut_printf(MUT_LOG_LOW, "%s, clear ShutdownWarning & wait for notification\n", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    for (retryIndex = 0; retryIndex < RM_RETRY_COUNT; retryIndex++)
    {
        isMsgPresent = FALSE;
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_INFO_ENCL_SHUTDOWN_CLEARED,
                                             &deviceStr[0]);
    
        if ((status == FBE_STATUS_OK) && isMsgPresent)
        {
            break;
        }
        fbe_api_sleep(2000);
    }
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, 
                             "Suitcase Shutdown Cleared Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s Shutdown Cleared Event log msg found\n", &deviceStr[0]);

    /*
     * Test Suitcase AmbientOverTemp
     */
    mut_printf(MUT_LOG_LOW, "=== %s, Test Suitcase AmbientOverTemp ===\n", __FUNCTION__);
    randle_mcmurphy_updateSuitcaseAmbientOverTemp(SP_A, TRUE);

    mut_printf(MUT_LOG_LOW, "%s, set AmbientOverTemp & wait for notification\n", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    suitcaseInfo.location.sp = SP_A;
    suitcaseInfo.location.slot = 0;
    status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(suitcaseInfo.suitcaseInfo.ambientOvertempFault);

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SUITCASE, 
                                               &suitcaseInfo.location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    for (retryIndex = 0; retryIndex < RM_RETRY_COUNT; retryIndex++)
    {
        isMsgPresent = FALSE;
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_ERROR_SUITCASE_FAULT_DETECTED,
                                             &deviceStr[0], "ambientOverTemp");
    
        if ((status == FBE_STATUS_OK) && isMsgPresent)
        {
            break;
        }
        fbe_api_sleep(2000);
    }
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, 
                             "Suitcase AmbientOverTemp Detected Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s AmbientOverTemp Detected Event log msg found\n", &deviceStr[0]);

    randle_mcmurphy_updateSuitcaseAmbientOverTemp(SP_A, FALSE);
    mut_printf(MUT_LOG_LOW, "%s, clear AmbientOverTemp & wait for notification\n", __FUNCTION__);
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    for (retryIndex = 0; retryIndex < RM_RETRY_COUNT; retryIndex++)
    {
        isMsgPresent = FALSE;
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                             &isMsgPresent, 
                                             ESP_INFO_SUITCASE_FAULT_CLEARED,
                                             &deviceStr[0], "ambientOverTemp");
    
        if ((status == FBE_STATUS_OK) && isMsgPresent)
        {
            break;
        }
        fbe_api_sleep(2000);
    }
    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, 
                             "Suitcase AmbientOverTemp Cleared Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s Shutdown AmbientOverTemp Event log msg found\n", &deviceStr[0]);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(randle_mcmurphy_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

}   // end of randle_mcmurphy_testSuitcaseStatus


/*!**************************************************************
 * randle_mcmurphy_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Sentry platform Suitcase 
 *  & MCU fault detection.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   12/12/2011 - Created. Joe Perry
 *
 ****************************************************************/
void randle_mcmurphy_test(void)
{
    fbe_status_t                            status;

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    /*
     * Verify the Suitcase Status reporting
     */
    mut_printf(MUT_LOG_LOW, "MCMURPHY: Test Suitcase Status reporting\n");
    randle_mcmurphy_testSuitcaseStatus();

    return;

}
/******************************************
 * end randle_mcmurphy_test()
 ******************************************/


/*!**************************************************************
 * randle_mcmurphy_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   12/12/2011 - Created. Joe Perry
 *
 ****************************************************************/
void randle_mcmurphy_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_DEFIANT_M1_HW_TYPE,
                                        NULL,
                                        NULL);

}
/**************************************
 * end randle_mcmurphy_setup()
 **************************************/

/*!**************************************************************
 * randle_mcmurphy_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the randle_mcmurphy test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   10/18/2011 - Created. Joe Perry
 *
 ****************************************************************/

void randle_mcmurphy_destroy(void)
{
    fbe_api_sleep(20000);
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end randle_mcmurphy_destroy()
 ******************************************/
/*************************
 * end file randle_mcmurphy_test.c
 *************************/

