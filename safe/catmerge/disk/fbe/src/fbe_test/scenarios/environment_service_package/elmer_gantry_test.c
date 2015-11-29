/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file elmer_gantry_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify miscellaneous sps_mgmt & ps_mgmt processing.
 * 
 * @version
 *   12/06/2011 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"
/*
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_sps_info.h"
*/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * elmer_gantry_short_desc = "Verify miscellaneous sps_mgmt & ps_mgmt processing";
char * elmer_gantry_long_desc ="\
elmer_gantry\n\
        -lead character in the movie ELMER GANTRY\n\
        -famous quote 'Sin, sin, sin!  You're all sinners!  You're all doomed to perdition!'\n\
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
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. SPS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Validate event notification for SPS Status changes\n\
        - Verify debouncing of SPS BatteryOnline status\n\
        - Verify debouncing of SPS faults\n\
";

static fbe_u64_t                    expected_device_mask = FBE_DEVICE_TYPE_INVALID;
//static fbe_object_id_t	                    sps_object_id = FBE_OBJECT_ID_INVALID;
static fbe_notification_registration_id_t   reg_id;


static void elmer_gantry_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*callbackSem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback,  expDevMak 0x%llx, devMsk 0x%llx ===",
        expected_device_mask,
        update_object_msg->notification_info.notification_data.data_change_info.device_mask);

    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == expected_device_mask)
    {
        mut_printf(MUT_LOG_LOW, "%s, release semaphore\n", __FUNCTION__);
        fbe_semaphore_release(callbackSem, 0, 1 ,FALSE);
    }

}

/*
 * SPS related functions
 */
void elmer_gantry_waitForInitialSpsEvents(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         sem;
    DWORD 									dwWaitResult;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
    fbe_bool_t                              spsTestingComplete = FALSE;

    fbe_semaphore_init(&sem, 0, 1);

    // Initialize to get event notification
    expected_device_mask = FBE_DEVICE_TYPE_SPS;
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  elmer_gantry_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    mut_printf(MUT_LOG_LOW, "  %s, wait for Initial SPS Testing to complete\n", __FUNCTION__);
    while (!spsTestingComplete)
    {
        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
        mut_printf(MUT_LOG_LOW, "  %s, SpsStatus %d, cabling %d, status 0x%x\n", 
                   __FUNCTION__,
                   spsStatusInfo.spsStatus, spsStatusInfo.spsCablingStatus, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if ((spsStatusInfo.spsStatus == SPS_STATE_AVAILABLE) &&
            (spsStatusInfo.spsCablingStatus != FBE_SPS_CABLING_UNKNOWN))
        {
            spsTestingComplete = TRUE;
        }
        else
        {
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
            MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
        }
    }
    mut_printf(MUT_LOG_LOW, "  %s, Initial SPS Testing Completed\n", __FUNCTION__);

    /*
     * cleanup
     */
    status = fbe_api_notification_interface_unregister_notification(elmer_gantry_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

}   // end of elmer_gantry_waitForInitialSpsEvents

void elmer_gantry_updateSpsStatus(SPS_STATE spsState)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, set SpsStatus to %d\n", 
               __FUNCTION__, spsState);

    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.state = spsState;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of elmer_gantry_updateSpsStatus

void elmer_gantry_updateSpsBatteryEol(fbe_bool_t batteryEol)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, set batteryEol to %d\n", 
               __FUNCTION__, batteryEol);

    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.batteryEOL = batteryEol;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of elmer_gantry_updateSpsBatteryEol

void elmer_gantry_verifySpsBatteryOnlineDebounce(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         sem;
	DWORD 									dwWaitResult;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
    fbe_u32_t                               retryIndex;

    fbe_semaphore_init(&sem, 0, 1);

    // Initialize to get event notification
    expected_device_mask = FBE_DEVICE_TYPE_SPS;
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  elmer_gantry_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    // turn off SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Change SPS Status briefly & verify no change
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY: Change SPS status to BatteryOnline briefly (should ride through)\n");
    elmer_gantry_updateSpsStatus(SPS_STATE_ON_BATTERY);

    fbe_api_sleep (7000);           // delay so sps_mgmt can process the same event

    // Verify that ESP sps_mgmt detects SPS Status change
//	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
//	MUT_ASSERT_TRUE(dwWaitResult == FBE_STATUS_TIMEOUT);

    fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatusInfo.spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    mut_printf(MUT_LOG_LOW, "  ESP SpsStatus 0x%x, status 0x%x\n", 
        spsStatusInfo.spsStatus, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(spsStatusInfo.spsStatus, SPS_STATE_AVAILABLE);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: no SPS status change verified\n");

    mut_printf(MUT_LOG_LOW, "  %s, set SpsStatus to Available\n", __FUNCTION__);
    elmer_gantry_updateSpsStatus(SPS_STATE_AVAILABLE);

    fbe_api_sleep (10000);           // delay so sps_mgmt can process the same event

    /*
     * Change SPS Status & verify change
     */
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: Change SPS status to BatteryOnline\n");
    elmer_gantry_updateSpsStatus(SPS_STATE_ON_BATTERY);

    // Verify that ESP sps_mgmt detects SPS Status change
    for (retryIndex = 0; retryIndex < 5; retryIndex++)
    {
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
//        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
        mut_printf(MUT_LOG_LOW, "  ESP SpsStatus 0x%x, status 0x%x\n", 
            spsStatusInfo.spsStatus, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo.spsStatus == SPS_STATE_ON_BATTERY)
        {
            break;
        }
        fbe_api_sleep (3000);
    }
    MUT_ASSERT_INT_EQUAL(spsStatusInfo.spsStatus, SPS_STATE_ON_BATTERY);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: SPS status change verified\n");

// SPECL does not shutdown SPS yet
#if FALSE
    // Verify that SPS is shutdown
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatusInfo.spsIndex = FBE_LOCAL_SPS;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    mut_printf(MUT_LOG_LOW, "ESP spsInserted %d, SpsStatus 0x%x, status 0x%x\n", 
        spsStatusInfo.spsInserted, spsStatusInfo.spsStatus, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_FALSE(spsStatusInfo.spsInserted);
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY: SPS shutdown verified\n");
#endif

    fbe_api_sleep (10000);           // delay so sps_mgmt can process the same event

    mut_printf(MUT_LOG_LOW, "  %s, set SpsStatus to Available\n", __FUNCTION__);
    elmer_gantry_updateSpsStatus(SPS_STATE_AVAILABLE);

    // turn on SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Verify that ESP sps_mgmt detects SPS Status change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 25000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatusInfo.spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    mut_printf(MUT_LOG_LOW, "  ESP SpsStatus 0x%x, status 0x%x\n", 
        spsStatusInfo.spsStatus, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(spsStatusInfo.spsStatus, SPS_STATE_AVAILABLE);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: SPS status Available\n");

    // turn on SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * cleanup
     */
    status = fbe_api_notification_interface_unregister_notification(elmer_gantry_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

}   // end of elmer_gantry_verifySpsBatteryOnlineDebounce

void elmer_gantry_verifySpsFaultDebounce(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         sem;
	DWORD 									dwWaitResult;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
    fbe_u32_t                               retryIndex;

    fbe_semaphore_init(&sem, 0, 1);

    // Initialize to get event notification
    expected_device_mask = FBE_DEVICE_TYPE_SPS;
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  elmer_gantry_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    // turn off SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Change SPS Status briefly & verify no change
     */
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: Change SPS Faulted to EOL briefly (should ride through)\n");
    elmer_gantry_updateSpsBatteryEol(TRUE);

    fbe_api_sleep (7000);           // delay so sps_mgmt can process the same event

    mut_printf(MUT_LOG_LOW, "  %s, clear SPS Fault\n", __FUNCTION__);
    elmer_gantry_updateSpsBatteryEol(FALSE);

    // Verify that ESP sps_mgmt detects SPS Status change
//	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
//	MUT_ASSERT_TRUE(dwWaitResult == FBE_STATUS_TIMEOUT);

    fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatusInfo.spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    mut_printf(MUT_LOG_LOW, "  ESP spsFaults 0x%x, status 0x%x\n", 
        spsStatusInfo.spsFaultInfo.spsModuleFault, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_FALSE(spsStatusInfo.spsFaultInfo.spsModuleFault);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: no SPS status change verified\n");

    fbe_api_sleep (10000);           // delay so sps_mgmt can process the same event

    /*
     * Change SPS Status & verify change
     */
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: Change SPS to Faulted\n");
    elmer_gantry_updateSpsBatteryEol(TRUE);

    for (retryIndex = 0; retryIndex < 5; retryIndex++)
    {
        // Verify that ESP sps_mgmt detects SPS Status change
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 50000);
//        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
        mut_printf(MUT_LOG_LOW, "  ESP spsFaults 0x%x, status 0x%x\n", 
            spsStatusInfo.spsFaultInfo.spsModuleFault, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo.spsFaultInfo.spsBatteryEOL)
        {
            break;
        }
        fbe_api_sleep (5000);
    }
    MUT_ASSERT_TRUE(spsStatusInfo.spsFaultInfo.spsBatteryEOL);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: SPS Fault verified\n");

    // clear fault
    elmer_gantry_updateSpsBatteryEol(FALSE);

    // turn on SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * cleanup
     */
    status = fbe_api_notification_interface_unregister_notification(elmer_gantry_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

}   // end of elmer_gantry_verifySpsFaultDebounce

/*
 * BBU related functions
 */
void elmer_gantry_updateBbuOnBatteryStatus(fbe_bool_t onBattery)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, set onBattery to %d\n", 
               __FUNCTION__, onBattery);

    sfi_mask_data.structNumber = SPECL_SFI_BATTERY_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.batteryStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.batteryStatus.batterySummary[0].batteryStatus[0].onBattery = onBattery;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of elmer_gantry_updateBbuStatus

void elmer_gantry_verifyBbuOnBatteryDebounce(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         sem;
    DWORD                                   dwWaitResult;
    fbe_esp_sps_mgmt_bobStatus_t            bbuStatusInfo;
    fbe_u32_t                               retryIndex;

    fbe_semaphore_init(&sem, 0, 1);

    // Initialize to get event notification
    expected_device_mask = FBE_DEVICE_TYPE_BATTERY;
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  elmer_gantry_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    // turn off SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Change SPS Status briefly & verify no change
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY: Change BBU status to OnBattery briefly (should ride through)\n");
    elmer_gantry_updateBbuOnBatteryStatus(TRUE);

    fbe_api_sleep (7000);           // delay so sps_mgmt can process the same event

    // Verify that ESP sps_mgmt detects SPS Status change
//	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
//	MUT_ASSERT_TRUE(dwWaitResult == FBE_STATUS_TIMEOUT);

    fbe_set_memory(&bbuStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
    bbuStatusInfo.bobIndex = 0;
    status = fbe_api_esp_sps_mgmt_getBobStatus(&bbuStatusInfo);
    mut_printf(MUT_LOG_LOW, "  ESP BBU batteryState %d, status 0x%x\n", 
        bbuStatusInfo.bobInfo.batteryState, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_FALSE(bbuStatusInfo.bobInfo.batteryState == FBE_BATTERY_STATUS_ON_BATTERY);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: no BBU OnBattery change verified\n");

    mut_printf(MUT_LOG_LOW, "  %s, clear BBU OnBattery\n", __FUNCTION__);
    elmer_gantry_updateBbuOnBatteryStatus(FALSE);

    fbe_api_sleep (10000);           // delay so sps_mgmt can process the same event

    /*
     * Change SPS Status & verify change
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY: Change BBU status to OnBattery\n");
    elmer_gantry_updateBbuOnBatteryStatus(TRUE);

    // Verify that ESP sps_mgmt detects SPS Status change
    for (retryIndex = 0; retryIndex < 5; retryIndex++)
    {
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
//        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
        fbe_set_memory(&bbuStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
        bbuStatusInfo.bobIndex = 0;
        status = fbe_api_esp_sps_mgmt_getBobStatus(&bbuStatusInfo);
        mut_printf(MUT_LOG_LOW, "  ESP BBU batteryState %d, status 0x%x\n", 
            bbuStatusInfo.bobInfo.batteryState, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (bbuStatusInfo.bobInfo.batteryState == FBE_BATTERY_STATUS_ON_BATTERY)
        {
            break;
        }
        fbe_api_sleep (3000);
    }
    MUT_ASSERT_TRUE(bbuStatusInfo.bobInfo.batteryState == FBE_BATTERY_STATUS_ON_BATTERY);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: BBU OnBattery change verified\n");

    fbe_api_sleep (10000);           // delay so sps_mgmt can process the same event

    mut_printf(MUT_LOG_LOW, "  %s, set BBU OnBattery\n", __FUNCTION__);
    elmer_gantry_updateBbuOnBatteryStatus(FALSE);

    // turn on SPS simulation
//    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Verify that ESP sps_mgmt detects SPS Status change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 25000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_set_memory(&bbuStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
    bbuStatusInfo.bobIndex = 0;
    status = fbe_api_esp_sps_mgmt_getBobStatus(&bbuStatusInfo);
    mut_printf(MUT_LOG_LOW, "  ESP BBU batteryState %d, status 0x%x\n", 
        bbuStatusInfo.bobInfo.batteryState, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_FALSE(bbuStatusInfo.bobInfo.batteryState == FBE_BATTERY_STATUS_ON_BATTERY);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: BBU not OnBattery\n");

    // turn on SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * cleanup
     */
    status = fbe_api_notification_interface_unregister_notification(elmer_gantry_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

}   // end of elmer_gantry_verifyBbuOnBatteryDebounce


void elmer_gantry_updateDaePsMarginStatus(fbe_u32_t busIndex,
                                          fbe_u32_t enclIndex,
                                          fbe_u32_t psIndex, 
                                          fbe_bool_t psMarginFaultPs,
                                          fbe_bool_t psMarginFault)
{
    ses_ps_info_elem_struct                 ps_struct;
    fbe_api_terminator_device_handle_t      encl_device_id;
    fbe_status_t                            status;

    mut_printf(MUT_LOG_LOW, "  %s, %d_%d, ps %d, psMarginFaultPs %d, psMarginFault %d\n", 
               __FUNCTION__, busIndex, enclIndex, psIndex, psMarginFaultPs, psMarginFault);


    status = fbe_api_terminator_get_enclosure_handle(busIndex, enclIndex, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_emcPsInfoStatus(encl_device_id, psIndex, &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set PS Margin Results
    ps_struct.margining_test_mode = FBE_ESES_MARGINING_TEST_MODE_STATUS_TEST_SUCCESSFUL;
    if (!psMarginFault)
    {
        ps_struct.margining_test_results = FBE_ESES_MARGINING_TEST_RESULTS_SUCCESS;
    }
    else if (psMarginFaultPs)
    {
        ps_struct.margining_test_results = FBE_ESES_MARGINING_TEST_RESULTS_12V_1_OPEN;
    }
    else
    {
        ps_struct.margining_test_results = FBE_ESES_MARGINING_TEST_RESULTS_12V_1_SHORTED_TO_12_V2;
    }
    status = fbe_api_terminator_set_emcPsInfoStatus(encl_device_id, psIndex, &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of elmer_gantry_updateDaePsMarginStatus

void elmer_gantry_verifyDaePsMarginTestFailures(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         sem;
	DWORD 									dwWaitResult;
    fbe_esp_ps_mgmt_ps_info_t               psInfo;

    fbe_semaphore_init(&sem, 0, 1);

    // Initialize to get event notification
    expected_device_mask = FBE_DEVICE_TYPE_PS;
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  elmer_gantry_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
    psInfo.location.bus = 0;
    psInfo.location.enclosure = 1;
    psInfo.location.slot = 0;

    /*
     * Test PS Margin Test failure indicting a PS
     */
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY:Verify DAE PS Marining Test (faulted PS) failure detection\n");
    elmer_gantry_updateDaePsMarginStatus(psInfo.location.bus, psInfo.location.enclosure, 0, TRUE, TRUE);

    // Verify that ESP ps_mgmt detects PS Status change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "  %s, psMarginTestResults 0x%x, status %d\n", 
        __FUNCTION__, psInfo.psInfo.psMarginTestResults, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(psInfo.psInfo.psMarginTestResults, FBE_ESES_MARGINING_TEST_RESULTS_12V_1_OPEN);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: psMarginTestResults change verified\n");

    fbe_api_sleep (5000);           // delay so sps_mgmt can process the same event

    elmer_gantry_updateDaePsMarginStatus(psInfo.location.bus, psInfo.location.enclosure, 0, TRUE, FALSE);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY:Verify DAE PS Marining Test (faulted PS) failure detection - Success\n");

    // Verify that ESP ps_mgmt detects PS Status change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /*
     * Test PS Margin Test failure indicting multiple CRU's
     */
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY:Verify PE PS Marining Test (multiple CRU's) failure detection\n");
    elmer_gantry_updateDaePsMarginStatus(psInfo.location.bus, psInfo.location.enclosure, 0, FALSE, TRUE);

    // Verify that ESP ps_mgmt detects PS Status change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "  %s, psMarginTestResults 0x%x, status %d\n", 
        __FUNCTION__, psInfo.psInfo.psMarginTestResults, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(psInfo.psInfo.psMarginTestResults, FBE_ESES_MARGINING_TEST_RESULTS_12V_1_SHORTED_TO_12_V2);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY: psMarginTestResults change verified\n");

    fbe_api_sleep (5000);           // delay so sps_mgmt can process the same event

    elmer_gantry_updateDaePsMarginStatus(psInfo.location.bus, psInfo.location.enclosure, 0, FALSE, FALSE);
    mut_printf(MUT_LOG_LOW, "  ELMER GANTRY:Verify DAE PS Marining Test (multiple CRU's) failure detection - Success\n");

    // Verify that ESP ps_mgmt detects PS Status change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 20000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /*
     * cleanup
     */
    status = fbe_api_notification_interface_unregister_notification(elmer_gantry_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

}   // end of elmer_gantry_verifyDaePsMarginTestFailures

/*
 * Telco related functions
 */
void elmer_gantry_updatePePsAcDc(SP_ID spid, fbe_u32_t psIndex, AC_DC_INPUT acDc)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, ps %d, force acDc %d\n",
               __FUNCTION__, spid, psIndex, acDc);

    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spid].psStatus[psIndex].ACDCInput = acDc;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of elmer_gantry_updatePePsAcDc

void elmer_gantry_updateSpsInsertRemove(fbe_bool_t insert)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, SPS Insert %d\n",
               __FUNCTION__, insert);

    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.inserted = insert;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of elmer_gantry_updateSpsInsertRemove

void elmer_gantry_verifyTelcoArray(void)
{
    fbe_u32_t                           psIndex, psCount, realPsIndex;
    fbe_status_t                        status;
    fbe_device_physical_location_t      location = {0};
    fbe_esp_ps_mgmt_ps_info_t           psInfo;

    /*
     * Update PE PS's to report DC
     */
    mut_printf(MUT_LOG_LOW, "%s, Change PS's to DC\n", __FUNCTION__);
    location.bus = 0;
    location.enclosure = 0;
    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psCount <= FBE_ESP_PS_MAX_COUNT_PER_ENCL);

    psInfo.location = location;
    realPsIndex = 0;
    for (psIndex = 0; psIndex < psCount; psIndex++)
    {
        if (psIndex == (psCount / 2))
        {
            realPsIndex = 0;
        }
        if (psIndex < (psCount / 2))
        {
            elmer_gantry_updatePePsAcDc(SP_A, realPsIndex, DC_INPUT);
        }
        else
        {
            elmer_gantry_updatePePsAcDc(SP_B, realPsIndex, DC_INPUT);

        }
        realPsIndex++;

        psInfo.location.slot = psIndex;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "%d_%d: ps %d, psInserted %d, AcDc %d\n", 
            psInfo.location.bus,
            psInfo.location.enclosure,
            psIndex,
            psInfo.psInfo.bInserted,
            psInfo.psInfo.ACDCInput);
        MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
        MUT_ASSERT_TRUE(psInfo.psInfo.ACDCInput == AC_INPUT);
    }

    // unload and re-load ESP without the registry file this time
    mut_printf(MUT_LOG_LOW, "%s, Restart ESP to get PS & SPS changes\n", __FUNCTION__);
    fbe_test_esp_restart();

    fbe_api_sleep (10000);

    // Verify that PS's are now DC
    mut_printf(MUT_LOG_LOW, "%s, Verify PS's are DC\n", __FUNCTION__);
    for (psIndex = 0; psIndex < psCount; psIndex++)
    {
        psInfo.location.slot = psIndex;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "%d_%d: ps %d, psInserted %d, AcDc %d\n", 
            psInfo.location.bus,
            psInfo.location.enclosure,
            psIndex,
            psInfo.psInfo.bInserted,
            psInfo.psInfo.ACDCInput);
        MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
        MUT_ASSERT_TRUE(psInfo.psInfo.ACDCInput == DC_INPUT);
    }

}   // end of elmer_gantry_verifyTelcoArray

/*!**************************************************************
 * elmer_gantry_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   12/06/2011 - Created. Joe Perry
 *
 ****************************************************************/

void elmer_gantry_test(void)
{

//    elmer_gantry_waitForInitialSpsEvents();
    fbe_api_sleep (20000);           // delay so sps_mgmt can process the same event

    /*
     * Verify SPS BatteryOnline Debouncing
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:Verify SPS BatteryOnline Debouncing\n");
    elmer_gantry_verifySpsBatteryOnlineDebounce();

    /*
     * Verify SPS Fault Debouncing
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:Verify SPS Fault Debouncing\n");
    elmer_gantry_verifySpsFaultDebounce();

    /*
     * Verify PS Margining Test failures
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:Verify DAE PS Marining Test failure detection\n");
    elmer_gantry_verifyDaePsMarginTestFailures();

    /*
     * Detect Telco array
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:Verify the detection of Telco array\n");
    elmer_gantry_verifyTelcoArray();

    /*
     *
     */
//    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:\n");

    return;
}
/******************************************
 * end elmer_gantry_test()
 ******************************************/

/*!**************************************************************
 * elmer_gantry_oberon_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   12/06/2011 - Created. Joe Perry
 *
 ****************************************************************/

void elmer_gantry_oberon_test(void)
{

//    elmer_gantry_waitForInitialSpsEvents();
    fbe_api_sleep (20000);           // delay so sps_mgmt can process the same event

    /*
     * Verify BBU OnBattery Debouncing
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:Verify BBU OnBattery Debouncing\n");
    elmer_gantry_verifyBbuOnBatteryDebounce();

    /*
     * Detect Telco array
     */
    mut_printf(MUT_LOG_LOW, "ELMER GANTRY:Verify the detection of Telco array\n");
    elmer_gantry_verifyTelcoArray();

    return;
}
/******************************************
 * end elmer_gantry_oberon_test()
 ******************************************/

/*!**************************************************************
 * elmer_gantry_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/23/2011 - Created. Joe Perry
 *
 ****************************************************************/
void elmer_gantry_setup(void)
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_LAPA_RIOS_CONIG,
                                                    SPID_UNKNOWN_HW_TYPE,
                                                    NULL,
                                                    NULL);
}
/**************************************
 * end elmer_gantry_setup()
 **************************************/

/*!**************************************************************
 * elmer_gantry_oberon_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/23/2011 - Created. Joe Perry
 *
 ****************************************************************/
void elmer_gantry_oberon_setup(void)
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_BASIC_CONIG,
    		                                          SPID_OBERON_1_HW_TYPE,
                                                      NULL,
                                                      NULL);
}
/**************************************
 * end elmer_gantry_oberon_setup()
 **************************************/

/*!**************************************************************
 * elmer_gantry_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the elmer_gantry test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/23/2011 - Created. Joe Perry
 *
 ****************************************************************/

void elmer_gantry_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end elmer_gantry_destroy()
 ******************************************/

/*************************
 * end file elmer_gantry_test.c
 *************************/


