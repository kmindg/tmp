/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file guti_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the event notifications for SP Environment changes (FAN status).
 * 
 * @version
 *   07/23/2012 - Created. Rui Chang
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
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe_test_esp_utils.h"

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

char * guti_short_desc = "Verify event notification for SP Environment changes (FAN status)";
char * guti_long_desc ="\
Guti\n\
        Verify event notification for SP Environment changes (FAN status) on Jetfire and Megatron.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 1 fallback enclosure\n\
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
        - Verify that the fan status are processed correctly forn\
        - 1. Jetfire standalone fan\n\
        - 2. jetfire BM internal fan\n\
        - 3. Megatron standalone fan\n\
        - 4. Voyager standalone fan\n\
STEP 2: Validate event notification for FAN Status changes\n\
";

static fbe_class_id_t                       expected_class_id = FBE_CLASS_ID_INVALID;
static SP_ID                                currentSp;

void guti_startupSp(fbe_sim_transport_connection_target_t targetSp,
                        SPID_HW_TYPE platform_type);


static void guti_commmand_response_callback_function_for_fan (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*callbackSem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, devMsk 0x%llx, classId 0x%x===",
               update_object_msg->object_id,
               update_object_msg->notification_info.notification_data.data_change_info.device_mask,
               update_object_msg->notification_info.class_id);


    if ((update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_FAN) &&
        (update_object_msg->notification_info.class_id == expected_class_id))
    {
        mut_printf(MUT_LOG_LOW, "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE detected, release semaphore\n", __FUNCTION__);
        fbe_semaphore_release(callbackSem, 0, 1 ,FALSE);
    }

}

static void guti_commmand_response_callback_function_for_bem (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*callbackSem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, devMsk 0x%llx, classId 0x%x===",
               update_object_msg->object_id,
               update_object_msg->notification_info.notification_data.data_change_info.device_mask,
               update_object_msg->notification_info.class_id);


    if ((update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_BACK_END_MODULE) &&
        (update_object_msg->notification_info.class_id == expected_class_id))
    {
        mut_printf(MUT_LOG_LOW, "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE detected, release semaphore\n", __FUNCTION__);
        fbe_semaphore_release(callbackSem, 0, 1 ,FALSE);
    }

}

/*
 * FAN related functions
 */
void guti_updateXpeFanFault(SP_ID spid, fbe_u32_t fanIndex, fbe_bool_t fanFault)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, fan %d, force fanFault %d\n",
               __FUNCTION__, spid, fanIndex, fanFault);

    sfi_mask_data.structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, fan %d, fanFault %d\n",
    __FUNCTION__, spid, fanIndex, 
    sfi_mask_data.sfiSummaryUnions.fanStatus.fanSummary[spid].fanPackStatus[fanIndex].fanStatus[0].fanFault);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.fanStatus.fanSummary[spid].fanPackStatus[fanIndex].fanStatus[0].fanFault = 
        fanFault;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of guti_updateXpeFanFault

void guti_updateDpeFanFault(SP_ID spid, fbe_u32_t fanIndex, fbe_bool_t fanFault)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, fan %d, force fanFault %d\n",
               __FUNCTION__, spid, fanIndex, fanFault);

    sfi_mask_data.structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.fanStatus.fanSummary[spid].fanPackStatus[fanIndex].fanStatus[1].fanFault = 
        fanFault;
    
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of guti_updateDpeFanFault

void guti_updateDiskEnclosureFanFault(fbe_u32_t busNum, fbe_u32_t enclNum, fbe_u32_t fanIndex, fbe_bool_t fanFault)
{
    fbe_api_terminator_device_handle_t  encl_device_id;
    ses_stat_elem_cooling_struct        cooling_status;
    fbe_status_t                            status;

    mut_printf(MUT_LOG_LOW, "  %s, bus %d, encl %d, fan %d, force fanFault %d\n",
               __FUNCTION__, busNum, enclNum, fanIndex, fanFault);

    status = fbe_api_terminator_get_enclosure_handle(busNum, enclNum, &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get the Fan Status before introducing the fault */
    status = fbe_api_terminator_get_cooling_eses_status(encl_device_id, fanIndex, &cooling_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Cause/Clear the Fan fault on enclosure*/
    if (fanFault == TRUE)
    {
        cooling_status.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    }
    else
    {
        cooling_status.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    status = fbe_api_terminator_set_cooling_eses_status(encl_device_id, fanIndex, cooling_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of guti_updateXpeFanFault

void guti_testDpeFanFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         psSem;
    fbe_notification_registration_id_t      psRegId;
    fbe_device_physical_location_t  fanLocation;
    fbe_esp_cooling_mgmt_fan_info_t  fanInfo;
    fbe_esp_module_io_module_info_t espIoModuleInfo = {0};

    fbe_semaphore_init(&psSem, 0, 1);

    ///////////////////////////////////
    // Start to test jetfire standalone fan  //
    ///////////////////////////////////
    expected_class_id = FBE_CLASS_ID_BASE_BOARD;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
																  guti_commmand_response_callback_function_for_fan,
																  &psSem,
																  &psRegId);


    // fault one FAN
    guti_updateDpeFanFault(SP_A, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (set FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=0;
    fanLocation.enclosure=0;
    fanLocation.slot=0;

    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, FanFault: %d ===", __FUNCTION__,fanInfo.fanFaulted);

    // clear fault on FAN
    guti_updateDpeFanFault(SP_A, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (clear FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=0;
    fanLocation.enclosure=0;
    fanLocation.slot=0;

    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, FanFault: %d ===", __FUNCTION__,fanInfo.fanFaulted);

    status = fbe_api_notification_interface_unregister_notification(guti_commmand_response_callback_function_for_fan,
                                                                    psRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    ///////////////////////////////////
    // Start to test Voyager standalone fan  //
    ///////////////////////////////////
    expected_class_id = FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE,
																  guti_commmand_response_callback_function_for_fan,
																  &psSem,
																  &psRegId);
    // fault voyager cooling module
    guti_updateDiskEnclosureFanFault(0, 1, 5, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (set FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=0;
    fanLocation.enclosure=1;
    fanLocation.slot=1;

    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, FanFault: %d ===", __FUNCTION__,fanInfo.fanFaulted);

    // clear voyager cooling module fault
    guti_updateDiskEnclosureFanFault(0, 1, 5, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (clear FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=0;
    fanLocation.enclosure=1;
    fanLocation.slot=1;

    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, FanFault: %d ===", __FUNCTION__,fanInfo.fanFaulted);

    status = fbe_api_notification_interface_unregister_notification(guti_commmand_response_callback_function_for_fan,
                                                                    psRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //////////////////////////
    // Test BEM internal fan fault //
    //////////////////////////
    expected_class_id = FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE,
																  guti_commmand_response_callback_function_for_bem,
																  &psSem,
																  &psRegId);

    // fault one fan should not cause BEM internal fan fault event
    guti_updateDiskEnclosureFanFault(0, 0, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (set FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    espIoModuleInfo.header.sp = SP_A;
    espIoModuleInfo.header.slot = 0;
    espIoModuleInfo.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
    status  = fbe_api_esp_module_mgmt_getIOModuleInfo(&espIoModuleInfo);

    MUT_ASSERT_TRUE(espIoModuleInfo.io_module_phy_info.internalFanFault == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, BEM FanFault: %d ===", __FUNCTION__,espIoModuleInfo.io_module_phy_info.internalFanFault);

    // fault second fan which should cause BEM internal fan fault event
    guti_updateDiskEnclosureFanFault(0, 0, 1, TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (set FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_api_sleep (8000);
    status  = fbe_api_esp_module_mgmt_getIOModuleInfo(&espIoModuleInfo);

    MUT_ASSERT_TRUE(espIoModuleInfo.io_module_phy_info.internalFanFault == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, BEM FanFault: %d ===", __FUNCTION__,espIoModuleInfo.io_module_phy_info.internalFanFault);

    // clear fan fault
    guti_updateDiskEnclosureFanFault(0, 0, 0, FALSE);
    guti_updateDiskEnclosureFanFault(0, 0, 1, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (clear FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbe_api_sleep (8000);
    status  = fbe_api_esp_module_mgmt_getIOModuleInfo(&espIoModuleInfo);

    MUT_ASSERT_TRUE(espIoModuleInfo.io_module_phy_info.internalFanFault == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, BEM FanFault: %d ===", __FUNCTION__,espIoModuleInfo.io_module_phy_info.internalFanFault);

    ///////////////////
    // Test finish, clean //
    ///////////////////
    status = fbe_api_notification_interface_unregister_notification(guti_commmand_response_callback_function_for_bem,
                                                                    psRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&psSem);


}   // end of guti_testDpePsFaults

void guti_testXpeFanFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         psSem;
    fbe_notification_registration_id_t      psRegId;
    fbe_device_physical_location_t  fanLocation;
    fbe_esp_cooling_mgmt_fan_info_t  fanInfo;

    fbe_semaphore_init(&psSem, 0, 1);

    expected_class_id = FBE_CLASS_ID_BASE_BOARD;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
																  guti_commmand_response_callback_function_for_fan,
																  &psSem,
																  &psRegId);



    // fault FAN A3 B3
    guti_updateXpeFanFault(SP_A, 3, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (set FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=FBE_XPE_PSEUDO_BUS_NUM;
    fanLocation.enclosure=FBE_XPE_PSEUDO_ENCL_NUM;
    fanLocation.slot=3;


    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, Fan slot %d FanFault: %d ===", __FUNCTION__,fanLocation.slot, fanInfo.fanFaulted);

    guti_updateXpeFanFault(SP_B, 3, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (set FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=FBE_XPE_PSEUDO_BUS_NUM;
    fanLocation.enclosure=FBE_XPE_PSEUDO_ENCL_NUM;
    fanLocation.slot=8;


    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == TRUE);
    mut_printf(MUT_LOG_LOW, "=== %s, Fan slot %d FanFault: %d ===", __FUNCTION__,fanLocation.slot, fanInfo.fanFaulted);

    // clear fault on FAN
    guti_updateXpeFanFault(SP_A, 3, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (clear FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=FBE_XPE_PSEUDO_BUS_NUM;
    fanLocation.enclosure=FBE_XPE_PSEUDO_ENCL_NUM;
    fanLocation.slot=3;


    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, Fan slot %d FanFault: %d ===", __FUNCTION__,fanLocation.slot, fanInfo.fanFaulted);

    guti_updateXpeFanFault(SP_B, 3, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for fan change event (clear FanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    fbe_api_sleep (8000);

    fanLocation.bus=FBE_XPE_PSEUDO_BUS_NUM;
    fanLocation.enclosure=FBE_XPE_PSEUDO_ENCL_NUM;
    fanLocation.slot=8;


    status = fbe_api_esp_cooling_mgmt_get_fan_info(&fanLocation, &fanInfo);

    MUT_ASSERT_TRUE(fanInfo.fanFaulted == FALSE);
    mut_printf(MUT_LOG_LOW, "=== %s, Fan slot %d FanFault: %d ===", __FUNCTION__,fanLocation.slot, fanInfo.fanFaulted);


    status = fbe_api_notification_interface_unregister_notification(guti_commmand_response_callback_function_for_fan,
                                                                    psRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&psSem);


}   // end of guti_testXpePsFaults


/*!**************************************************************
 * guti_jetfire_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test FAN Status on Jetfire array.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/23/2012 - Created. Rui Chang
 *
 ****************************************************************/

void guti_jetfire_test(void)
{

    /*
     * Start up other SP
     */
    mut_printf(MUT_LOG_LOW, "  GUTI(JETFIRE): starting up peer SP\n");
    currentSp = SP_A;
    guti_startupSp(FBE_SIM_SP_A, SPID_DEFIANT_M1_HW_TYPE);

    /*
     * Verify DPE FAN's faulted condition detected
     */
    mut_printf(MUT_LOG_LOW, "GUTI(JETFIRE): SP %d detect DPE FAN faults\n", currentSp);
    guti_testDpeFanFaults();
    mut_printf(MUT_LOG_LOW, "GUTI(JETFIRE): SP %d detect DPE FAN faults - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);

    return;
}
/******************************************
 * end guti_jetfire_test()
 ******************************************/

/*!**************************************************************
 * guti_megatron() 
 ****************************************************************
 * @brief
 *  Main entry point to the test FAN Status on Megatron array.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/23/2012 - Created. Rui Chang
 *
 ****************************************************************/

void guti_megatron_test(void)
{

    /*
     * Start up other SP
     */
    mut_printf(MUT_LOG_LOW, "  GUTI(MEGATRON): starting up peer SP\n");
    currentSp = SP_A;
    guti_startupSp(FBE_SIM_SP_A, SPID_PROMETHEUS_M1_HW_TYPE);

    /*
     * Verify xPE FAN's faulted condition detected
     */
    mut_printf(MUT_LOG_LOW, "GUTI(MEGATRON): SP %d detect xPE FAN faults\n", currentSp);
    guti_testXpeFanFaults();
    mut_printf(MUT_LOG_LOW, "GUTI(MEGATRON): SP %d detect xPE FAN faults - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);


    return;
}
/******************************************
 * end guti_megatron()
 ******************************************/

void guti_startupSp(fbe_sim_transport_connection_target_t targetSp,
                        SPID_HW_TYPE platform_type)
{


    fbe_test_startEspAndSep_with_common_config(targetSp,
                                               FBE_ESP_TEST_SIMPLE_VOYAGER_CONFIG,
                                               platform_type,
                                               NULL,
                                               NULL);
}

/*!**************************************************************
 * guti_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   07/23/2012 - Created. Rui Chang
 *
 ****************************************************************/
void guti_setup(SPID_HW_TYPE platform_type)
{

    currentSp = SP_B;
    guti_startupSp(FBE_SIM_SP_B, platform_type);

//    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

}
/**************************************
 * end guti_setup()
 **************************************/

void guti_jetfire_setup()
{
    guti_setup(SPID_DEFIANT_M1_HW_TYPE);
}

void guti_megatron_setup()
{
    guti_setup(SPID_PROMETHEUS_M1_HW_TYPE);
}

/*!**************************************************************
 * guti_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the guti test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/23/2012 - Created. Rui Chang
 *
 ****************************************************************/

void guti_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end guti_destroy()
 ******************************************/
/*************************
 * end file guti_test.c
 *************************/


