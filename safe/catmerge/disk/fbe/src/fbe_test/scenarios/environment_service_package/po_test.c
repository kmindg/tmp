/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file po_test.c
 ***************************************************************************
 *
 * @brief
 *  Remove and insert management module and power supply from the peer SP
 * 
 * @version
 *   27-Feb-2012 - Created. Feng Ling
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
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fp_test.h"
#include "sep_tests.h"

char * po_short_desc = "remove management module and power supply from peer SP";
char * po_long_desc ="\
po is a clumsy panda in the movie Kung Fu Panda, he is a kung fu fanatic and trained by shifu to protect the valley\n\
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
STEP 2: Validate the insert mgmt config mode setup\n\
";


fbe_semaphore_t po_sem;
static void po_esp_ps_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                                  void *context);
static void po_esp_mgmt_module_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                                  void *context);

/*!*************************************************************
 *  po_test() 
 ****************************************************************
 * @brief
 *  Entry function for po test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  27-Feb-2012- Created. Feng Ling
 *
 ****************************************************************/ 
void po_test(void)
{
    DWORD                                       dwWaitResult;
    fbe_bool_t                                  is_msg_present = FBE_FALSE;
    fbe_status_t                                status;
    fbe_notification_registration_id_t          reg_id;
    fbe_esp_module_mgmt_get_mgmt_comp_info_t    mgmt_comp_info;
	fbe_esp_ps_mgmt_ps_info_t                   getPsInfo;
	SPECL_SFI_MASK_DATA                         sfi_mask_data;
    PORT_SPEED                                  initial_port_speed;

    mut_printf(MUT_LOG_LOW, "po test: started...\n");
    mut_printf(MUT_LOG_LOW, "po test: power supply test started...\n");
   
	// test for power supply
    // Check for event message
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    fbe_semaphore_init(&po_sem, 0, 1);
    // Register to get event notification from ESP 
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  po_esp_ps_data_change_callback_function,
                                                                  &po_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   
	// sfi get sfi data
    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // modify sfi data to remove power supply
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[SP_B].psStatus[0].inserted = FALSE;
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Get data and verify it
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[SP_B].psStatus[0].inserted == FALSE);

    // send sfi data
	status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // wait for semaphore
	mut_printf(MUT_LOG_LOW, "Waiting for semaphore get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&po_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    // get power supply status from ESP
    getPsInfo.location.slot = 2;
    getPsInfo.location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    getPsInfo.location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // check that power supply is not inserted
    MUT_ASSERT_TRUE(getPsInfo.psInfo.bInserted == FALSE);

	// Check for event message 
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_PS_REMOVED,
                                         "xPE Power Supply 2");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "po test: ps removed successfully...\n");

	is_msg_present = FBE_FALSE;

    // sfi get sfi data
    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // modify sfi data to insert power supply
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[SP_B].psStatus[0].inserted = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Get data and verify it
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[SP_B].psStatus[0].inserted == TRUE);

    // send sfi data
	status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // wait for semaphore
	mut_printf(MUT_LOG_LOW, "Waiting for semaphore get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&po_sem, 30000);

    // get power supply status from ESP
    getPsInfo.location.slot = 2;
    getPsInfo.location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    getPsInfo.location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // check that power supply is inserted
	MUT_ASSERT_TRUE(getPsInfo.psInfo.bInserted == TRUE);
	

    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_PS_INSERTED,
                                         "xPE Power Supply 2");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "po test: ps inserted successfully...\n");

    // Unregistered the notification
    status = fbe_api_notification_interface_unregister_notification(po_esp_ps_data_change_callback_function,
                                                                   reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&po_sem);

    mut_printf(MUT_LOG_LOW, "po test: power supply completed successfully...\n");
	
	mut_printf(MUT_LOG_LOW, "po test: mgmt module test started...\n");

	// test for mgmt module
	is_msg_present = FBE_FALSE;

    // get mgmt module status from ESP
	mgmt_comp_info.phys_location.sp = SP_A;
    mgmt_comp_info.phys_location.slot = 0;
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    initial_port_speed = mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortSpeed;

    // Check for event message
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    fbe_semaphore_init(&po_sem, 0, 1);
    // Register to get event notification from ESP 
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  po_esp_mgmt_module_data_change_callback_function,
                                                                  &po_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    // sfi get sfi data
    sfi_mask_data.structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // modify sfi data to remove mgmt module
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].inserted = FALSE;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].inserted = FALSE;
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Get data and verify it
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
     // wait for semaphore
	mut_printf(MUT_LOG_LOW, "Waiting for semaphore get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&po_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].inserted == FALSE);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].inserted == FALSE);

    // send sfi data
	status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // wait for semaphore
	mut_printf(MUT_LOG_LOW, "Waiting for semaphore get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&po_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    // get mgmt module status from ESP
	mgmt_comp_info.phys_location.sp = SP_A;
    mgmt_comp_info.phys_location.slot = 0;
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // check that mgmt module is not inserted
	MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.bInserted == FALSE);

	// Check for event message 
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_MGMT_FRU_REMOVED,
                                         "SPB management module 0");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "po test: mgmt module removed successfully...\n");

	is_msg_present = FBE_FALSE;

    // sfi get sfi data
    sfi_mask_data.structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // modify sfi data to insert mgmt module
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].inserted = TRUE;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].inserted = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Get data and verify it
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].inserted == TRUE);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].inserted == TRUE);

    // send sfi data
	status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // wait for semaphore
	mut_printf(MUT_LOG_LOW, "Waiting for semaphore get update to ESP module_mgmt...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&po_sem, 30000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    // get mgmt module status from ESP
	mgmt_comp_info.phys_location.sp = SP_A;
    mgmt_comp_info.phys_location.slot = 0;
    status = fbe_api_esp_module_mgmt_get_mgmt_comp_info(&mgmt_comp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // check that mgmt module is inserted
	MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.bInserted == TRUE);
    // check that the mgmt module speed setting was restored on insertion at runtime
    MUT_ASSERT_TRUE(mgmt_comp_info.mgmt_module_comp_info.managementPort.mgmtPortSpeed == initial_port_speed);

    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_MGMT_FRU_INSERTED,
                                         "SPB management module 0");
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "po test: mgmt module inserted successfully...\n");

	// Unregistered the notification
    status = fbe_api_notification_interface_unregister_notification(po_esp_mgmt_module_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&po_sem);

    fbe_test_esp_check_mgmt_config_completion_event();

	mut_printf(MUT_LOG_LOW, "po test: mgmt module test completed successfully...\n");

    mut_printf(MUT_LOG_LOW, "po test: completed successfully...\n");
	return;
}


/***********************************
 *  end of po_test()
 ***********************************/

/*!*************************************************************
 *  po_setup() 
 ****************************************************************
 * @brief
 *  Initiate the setup required to run the test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  27-Feb-2012- Created. Feng Ling
 *
 ****************************************************************/ 
void po_setup(void)
{
    fbe_status_t             status;
    SPECL_SFI_MASK_DATA      sfi_mask_data;

    
    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    fp_test_start_physical_package();

    /* Set the insert ps config mode;
     * when ESP loaded; verify it set to TRUE
     */
    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Set the insert ps mode
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[SP_B].psStatus[0].inserted = TRUE;
    // Set the insert Mgmt mode
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].inserted = TRUE;
    sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].inserted = TRUE;

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // Get data and verify it
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    status = fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[SP_B].psStatus[0].inserted == TRUE);

    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].inserted == TRUE);
    MUT_ASSERT_TRUE(sfi_mask_data.sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].inserted == TRUE);

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
 *  end of po_setup()
 **********************************/

/*!*************************************************************
 *  po_destroy() 
 ****************************************************************
 * @brief
 *  Unload the packages.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  27-Feb-2012- Created. Feng Ling
 *
 ****************************************************************/ 
void po_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/*************************************************
 *  end of po_destroy()
 ************************************************/

/*!*************************************************************
 *  po_esp_ps_data_change_callback_function() 
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
 *  27-Feb-2012- Created. Feng Ling
 *
 ****************************************************************/ 
static void 
po_esp_ps_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                      void *context)
{
    fbe_status_t         status;
    fbe_object_id_t      expected_object_id = FBE_OBJECT_ID_INVALID;
    fbe_semaphore_t      *sem = (fbe_semaphore_t *)context;
    fbe_u64_t    device_mask;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    status = fbe_api_esp_common_get_object_id_by_device_type(FBE_DEVICE_TYPE_PS, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    if ( (update_object_msg->object_id == expected_object_id) &&
        (device_mask && FBE_DEVICE_TYPE_PS)) 
    {
        if(update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot == 2)
        {
           fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
    return;
}
/******************************************************************
 *  end of po_esp_ps_data_change_callback_function
 *****************************************************************/

/*!*************************************************************
 *  po_esp_mgmt_module_data_change_callback_function() 
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
 *  27-Feb-2012- Created. Feng Ling
 *
 ****************************************************************/ 
static void 
po_esp_mgmt_module_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                      void *context)
{
    fbe_status_t         status;
    fbe_object_id_t      expected_object_id = FBE_OBJECT_ID_INVALID;
    fbe_semaphore_t      *sem = (fbe_semaphore_t *)context;
    fbe_u64_t    device_mask;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &expected_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expected_object_id != FBE_OBJECT_ID_INVALID);

    if (update_object_msg->object_id == expected_object_id &&
        (device_mask && FBE_DEVICE_TYPE_MGMT_MODULE)) 
    {
        if((update_object_msg->notification_info.notification_data.data_change_info.phys_location.sp == SP_B) &&
           (update_object_msg->notification_info.notification_data.data_change_info.phys_location.slot == 0))
        {
           fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
    return;
}
/******************************************************************
 *  end of po_esp_mgmt_module_data_change_callback_function
 *****************************************************************/

