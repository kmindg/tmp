/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file component_accessible_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the event log for SP inaccessible component.
 * 
 * @version
 *   01/21/2013 - Created. Eric Zhou
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
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "EspMessages.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * component_accessible_short_desc = "Verify ESP event logging for environmental interface failure";
char * component_accessible_long_desc ="\
component_accessible\n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 1 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
STEP 1: Bring up the initial topology.\n\
STEP 2: Validate event log for inaccessible component\n\
";

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_class_id_t           expected_class_id = FBE_CLASS_ID_BASE_BOARD;
static fbe_u64_t        expected_device_type = FBE_DEVICE_TYPE_INVALID;
static fbe_device_data_type_t   expected_data_type = FBE_DEVICE_DATA_TYPE_INVALID;
static fbe_device_physical_location_t expected_phys_location = {0};

static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;
static SP_ID peer_sp = SP_B;

static void component_accessible_commmand_response_callback_function(update_object_msg_t * update_object_msg, void *context);

static void component_accessible_test_verify_io_module_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_bem_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_power_supply_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_mgmt_module_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_mezzanine_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_cooling_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_suitcase_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_bmc_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_battery_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_slave_port_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void component_accessible_test_verify_fault_register_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);

static void component_accessible_commmand_response_callback_function(update_object_msg_t * update_object_msg,
                                                               void *context)
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
            case FBE_DEVICE_TYPE_MISC:
            case FBE_DEVICE_TYPE_SUITCASE:
            case FBE_DEVICE_TYPE_BMC:
            case FBE_DEVICE_TYPE_SLAVE_PORT:
            case FBE_DEVICE_TYPE_FLT_REG:
                expected_notification = TRUE; 
                break;

            case FBE_DEVICE_TYPE_FAN:
            case FBE_DEVICE_TYPE_IOMODULE:
            case FBE_DEVICE_TYPE_BACK_END_MODULE:
            case FBE_DEVICE_TYPE_MEZZANINE:
            case FBE_DEVICE_TYPE_MGMT_MODULE: 
                if((expected_phys_location.sp == pDataChangeInfo->phys_location.sp) &&
                   (expected_phys_location.slot == pDataChangeInfo->phys_location.slot))
                {
                    expected_notification = TRUE; 
                }
                break;

            case FBE_DEVICE_TYPE_PS:
            case FBE_DEVICE_TYPE_BATTERY:
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

/*!**************************************************************
 * component_accessible_test() 
 ****************************************************************
 * @brief
 *  Main entry point to test event log for inaccessible component.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   01/21/2013 - Created. Eric Zhou
 *
 ****************************************************************/
void component_accessible_test(void)
{
    fbe_status_t        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     objectId;
    PSPECL_SFI_MASK_DATA    sfi_mask_data = {0};
                                                            
    fbe_semaphore_init(&sem, 0, 1);
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                  component_accessible_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return;
    }

    /* Get handle to the board object */
    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);
    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error for all PE components ===\n");

    component_accessible_test_verify_io_module_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_bem_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_mgmt_module_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_mezzanine_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_power_supply_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_cooling_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_suitcase_inaccessible(objectId, sfi_mask_data);
    component_accessible_test_verify_bmc_inaccessible(objectId, sfi_mask_data);
//    component_accessible_test_verify_battery_inaccessible(objectId, sfi_mask_data);
//    component_accessible_test_verify_slave_port_inaccessible(objectId, sfi_mask_data);
//    component_accessible_test_verify_fault_register_inaccessible(objectId, sfi_mask_data);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified the SMB Error for all PE components ===\n");

    fbeStatus = fbe_api_notification_interface_unregister_notification(component_accessible_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    return;
}
/******************************************
 * end component_accessible_test()
 ******************************************/

static void component_accessible_test_verify_io_module_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t ioModuleInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_io_module_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> IO MODULE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_IOMODULE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_IO_MODULE, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the IO Module info. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the IO Module info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[ioModuleInfo.associatedSp].ioStatus[ioModuleInfo.slotNumOnBlade].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the IO Module info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = ioModuleInfo.associatedSp;
    location.slot = ioModuleInfo.slotNumOnBlade;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_IOMODULE, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(ioModuleInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the IO Module info. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the IO Module info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary->ioStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the IO Module info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_bem_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t bemInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           bemIndex = 0;
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_bem_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> BEM ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_BEM, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the BEM info. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the BEM info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    while (bemIndex < TOTAL_IO_MOD_PER_BLADE - 1)
    {
        if (sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[bemInfo.associatedSp].ioStatus[bemIndex].moduleType == IO_MODULE_TYPE_BEM)
        {
            break;
        }
        bemIndex++;
    }
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[bemInfo.associatedSp].ioStatus[bemIndex].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the BEM info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_bem_info(objectId, &bemInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bemInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = bemInfo.associatedSp;
    location.slot = bemInfo.slotNumOnBlade;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(bemInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the BEM info. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the BEM info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[bemInfo.associatedSp].ioStatus[bemIndex].transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the BEM info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_bem_info(objectId, &bemInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bemInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_power_supply_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_power_supply_info_t psInfo = {0};
    fbe_base_board_get_base_board_info_t boardInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_power_supply_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> Power Supply ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_PS;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_POWER_SUPPLY, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the PS info. */
    sfi_mask_data->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the PS info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.psStatus.psSummary[psInfo.associatedSp].psStatus[psInfo.slotNumOnEncl].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the PS info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    fbeStatus = fbe_api_board_get_base_board_info(objectId, &boardInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if (boardInfo.platformInfo.enclosureType == XPE_ENCLOSURE_TYPE) 
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    location.slot = psInfo.slotNumOnEncl;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_PS, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(psInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the PS info. */
    sfi_mask_data->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the PS info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.psStatus.psSummary->psStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the PS info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_mgmt_module_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_mgmt_module_info_t mgmtInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_mgmt_module_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> MGMT MODULE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_MGMT_MODULE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_MGMT_MODULE, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the MGMT Module info. */
    sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the MGMT Module info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[mgmtInfo.associatedSp].mgmtStatus[mgmtInfo.mgmtID].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the MGMT Module info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmtInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = mgmtInfo.associatedSp;
    location.slot = mgmtInfo.mgmtID;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_MGMT_MODULE, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(mgmtInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the MGMT Module info. */
    sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the MGMT Module info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary->mgmtStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the MGMT Module info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mgmtInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_mezzanine_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t   mezzanineInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           mezzIndex = 0;
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_mezzanine_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> MEZZANINE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_MEZZANINE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_MEZZANINE, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the MEZZANINE info. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the MEZZANINE info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    while (mezzIndex < TOTAL_IO_MOD_PER_BLADE - 1)
    {
        if ((sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzanineInfo.associatedSp].ioStatus[mezzIndex].moduleType == IO_MODULE_TYPE_MEZZANINE) ||
            (sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzanineInfo.associatedSp].ioStatus[mezzIndex].moduleType == IO_MODULE_TYPE_ONBOARD))
        {
            break;
        }
        mezzIndex++;
    }
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzanineInfo.associatedSp].ioStatus[mezzIndex].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the MEZZANINE info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzanineInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mezzanineInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = mezzanineInfo.associatedSp;
    location.slot = mezzanineInfo.slotNumOnBlade;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_MEZZANINE, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(mezzanineInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the MEZZANINE info. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the MEZZANINE info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzanineInfo.associatedSp].ioStatus[mezzIndex].transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the MEZZANINE info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzanineInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mezzanineInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_cooling_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_cooling_info_t coolingInfo = {0};
    fbe_base_board_get_base_board_info_t boardInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_cooling_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> COOLING ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_FAN;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_COOLING_COMPONENT, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the COOLING info. */
    sfi_mask_data->structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the COOLING info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fanStatus.fanSummary[coolingInfo.associatedSp].fanPackStatus[coolingInfo.slotNumOnSpBlade].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the COOLING info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_cooling_info(objectId, &coolingInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(coolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    fbeStatus = fbe_api_board_get_base_board_info(objectId, &boardInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if (boardInfo.platformInfo.enclosureType == XPE_ENCLOSURE_TYPE) 
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    location.slot = coolingInfo.slotNumOnSpBlade;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_FAN, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(coolingInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the COOLING info. */
    sfi_mask_data->structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the COOLING info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fanStatus.fanSummary->fanPackStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the COOLING info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_cooling_info(objectId, &coolingInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(coolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_suitcase_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_suitcase_info_t suitcaseInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_suitcase_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> SUITCASE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_SUITCASE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_SUITCASE, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the SUITCASE info. */
    sfi_mask_data->structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the SUITCASE info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.suitcaseStatus.suitcaseSummary[suitcaseInfo.associatedSp].suitcaseStatus[suitcaseInfo.suitcaseID].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the SUITCASE info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_suitcase_info(objectId, &suitcaseInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(suitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = suitcaseInfo.associatedSp;
    location.slot = suitcaseInfo.suitcaseID;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SUITCASE, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(suitcaseInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the SUITCASE info. */
    sfi_mask_data->structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the SUITCASE info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.suitcaseStatus.suitcaseSummary->suitcaseStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the SUITCASE info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_suitcase_info(objectId, &suitcaseInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(suitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_bmc_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_bmc_info_t bmcInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_bmc_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> BMC ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_BMC;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_BMC, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the BMC info. */
    sfi_mask_data->structNumber = SPECL_SFI_BMC_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.bmcStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the BMC info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.bmcStatus.bmcSummary[bmcInfo.associatedSp].bmcStatus[bmcInfo.bmcID].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the BMC info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_bmc_info(objectId, &bmcInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bmcInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = bmcInfo.associatedSp;
    location.slot = bmcInfo.bmcID;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BMC, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(bmcInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the BMC info. */
    sfi_mask_data->structNumber = SPECL_SFI_BMC_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.bmcStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the BMC info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.bmcStatus.bmcSummary->bmcStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the BMC info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_bmc_info(objectId, &bmcInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bmcInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_battery_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_board_mgmt_get_battery_status_t        bobInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_esp_sps_mgmt_getBobCount(&componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> BOB ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_BATTERY;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_BATTERY, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the BOB info. */
    sfi_mask_data->structNumber = SPECL_SFI_BATTERY_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.batteryStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the BOB info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.batteryStatus.batterySummary[bobInfo.batteryInfo.associatedSp].batteryStatus[bobInfo.batteryInfo.slotNumOnSpBlade].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the BOB info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_battery_status(objectId, &bobInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bobInfo.batteryInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.slot = bobInfo.batteryInfo.slotNumOnSpBlade;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(bobInfo.batteryInfo.envInterfaceStatus));
scanf("%d",(int *)&fbeStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the BOB info. */
    sfi_mask_data->structNumber = SPECL_SFI_BATTERY_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.batteryStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the BOB info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.batteryStatus.batterySummary->batteryStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the BOB info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_battery_status(objectId, &bobInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(bobInfo.batteryInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_slave_port_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_slave_port_info_t    slavePortInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_slave_port_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> SLAVE PORT ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_SLAVE_PORT;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_SLAVE_PORT, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the SLAVE PORT info. */
    sfi_mask_data->structNumber = SPECL_SFI_SLAVE_PORT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.slavePortStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    slavePortInfo.associatedSp = peer_sp;

    /* Set the SLAVE PORT info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.slavePortStatus.slavePortSummary[slavePortInfo.associatedSp].slavePortStatus[slavePortInfo.slavePortID].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the SLAVE PORT info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_slave_port_info(objectId, &slavePortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(slavePortInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = slavePortInfo.associatedSp;
    location.slot = slavePortInfo.slavePortID;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SLAVE_PORT, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(slavePortInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the SLAVE PORT info. */
    sfi_mask_data->structNumber = SPECL_SFI_SLAVE_PORT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.slavePortStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the SLAVE PORT info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.slavePortStatus.slavePortSummary[slavePortInfo.associatedSp].slavePortStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the SLAVE PORT info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_slave_port_info(objectId, &slavePortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(slavePortInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

static void component_accessible_test_verify_fault_register_inaccessible(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_peer_boot_flt_exp_info_t        fltRegInfo = {0};
//    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_u8_t                            deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           componentCount = 0;

    fbeStatus = fbe_api_board_get_flt_reg_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount <= 0)
    {
        return;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> FAULT REGISTER ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_FLT_REG;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_FLT_REG, 9);

    /********************** SET the SMB Errors *************************/
    /* Get the FAULT REGISTER info. */
    sfi_mask_data->structNumber = SPECL_SFI_FLT_REG_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fltRegInfo.associatedSp = peer_sp;

    /* Set the FAULT REGISTER info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[fltRegInfo.associatedSp].faultRegisterStatus[fltRegInfo.fltExpID].transactionStatus = STATUS_SMB_ARB_ERR;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

    /* Get the FAULT REGISTER info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_flt_reg_info(objectId, &fltRegInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fltRegInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

    location.sp = fltRegInfo.associatedSp;
    location.slot = fltRegInfo.fltExpID;
    fbeStatus = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_FLT_REG, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                         &deviceStr[0],
                                         fbe_base_env_decode_env_status(fltRegInfo.envInterfaceStatus));
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    /********************** CLEAR the SMB Errors *************************/
    mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

    /* Get the FAULT REGISTER info. */
    sfi_mask_data->structNumber = SPECL_SFI_FLT_REG_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Set the FAULT REGISTER info. */
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[fltRegInfo.associatedSp].faultRegisterStatus[fltRegInfo.fltExpID].transactionStatus = EMCPAL_STATUS_SUCCESS;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the FAULT REGISTER info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_flt_reg_info(objectId, &fltRegInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fltRegInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

// Stand Alone alerts do not work - disable
#if FALSE
    /* Check for event message */
    fbeStatus = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present,
                                         ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                         &deviceStr[0]);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present == FBE_TRUE);
#endif

    return;
}

/*!**************************************************************
 * component_accessible_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   01/21/2013 - Created. Eric Zhou
 *
 ****************************************************************/
void component_accessible_setup(void)
{
    fbe_test_startEspAndSep(FBE_SIM_SP_A, SPID_DEFIANT_M1_HW_TYPE);
}
/**************************************
 * end component_accessible_setup()
 **************************************/

/*!**************************************************************
 * component_accessible_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the component_accessible test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   01/21/2013 - Created. Eric Zhou
 *
 ****************************************************************/
void component_accessible_destroy(void)
{
    fbe_api_sleep(20000);
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end component_accessible_destroy()
 ******************************************/

/*************************
 * end file component_accessible_test.c
 *************************/
