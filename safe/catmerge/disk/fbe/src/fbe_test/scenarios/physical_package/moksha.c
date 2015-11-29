/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file moksha.c
 ***************************************************************************
 *
 * @brief
 *  This file contains handling of STALE data for the PE components        *
 * @version
 *   16/09/2010 Arun S - Created.
 *
 ***************************************************************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe_notification.h"
#include "specl_sfi_types.h"
#include "fbe/fbe_api_terminator_interface.h"

char * moksha_short_desc = "Test Stale Data for processor enclosure components.";
char * moksha_long_desc =
    "\n"
    "\n"
    "Moksha"
    " In Hindu Mythology, Moksha refers to liberation from the cycle of death and rebirth.\n\n"
    " This scenario tests the Stale Data for physical package processor enclosure components.\n"
    " It includes: \n"
    "    - Stale Data verification for the IO module;\n"
    "    - Stale Data verification for the BEM;\n"
    "    - Stale Data verification for the Mgmt module;\n"
    "    - Stale Data verification for the Mezzanine;\n"
    "    - Stale Data verification for the Power Supply;\n"
    "    - Stale Data verification for the Cooling (Fan);\n"
    "    - Stale Data verification for the MCU;\n"
    "    - Stale Data verification for the Suitcase;\n"
    "\n"
    "Dependencies:\n"
    "    - The SPECL should be able to work in the user space to read the status from the terminator.\n"
    "    - The terminator should be able to provide the interface function to insert/remove the components in the processor enclosure.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board with processor enclosure data\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"  
    "STEP 2: Shutdown the Terminator/Physical package.\n"
    ;

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_class_id_t           expected_class_id = FBE_CLASS_ID_BASE_BOARD;
static fbe_u64_t        expected_device_type = FBE_DEVICE_TYPE_INVALID;

static fbe_device_physical_location_t expected_phys_location = {0};

static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;

static void moksha_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);

static void moksha_test_verify_io_module_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void moksha_test_verify_bem_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void moksha_test_verify_power_supply_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void moksha_test_verify_mgmt_module_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void moksha_test_verify_mezzanine_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void moksha_test_verify_cooling_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void moksha_test_verify_suitcase_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);


void moksha(void)
{
    fbe_status_t        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     objectId;
    PSPECL_SFI_MASK_DATA    sfi_mask_data = {0};
                                                            
    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return;
    }

    fbe_semaphore_init(&sem, 0, 1);
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                  moksha_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Get handle to the board object */
    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    moksha_test_verify_io_module_stale_data(objectId, sfi_mask_data);
    moksha_test_verify_bem_stale_data(objectId, sfi_mask_data);
    moksha_test_verify_mgmt_module_stale_data(objectId, sfi_mask_data);
    moksha_test_verify_mezzanine_stale_data(objectId, sfi_mask_data);
    moksha_test_verify_power_supply_stale_data(objectId, sfi_mask_data);
    moksha_test_verify_cooling_stale_data(objectId, sfi_mask_data);
    moksha_test_verify_suitcase_stale_data(objectId, sfi_mask_data);

    fbeStatus = fbe_api_notification_interface_unregister_notification(moksha_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);
    free(sfi_mask_data);

    return;
}

static void moksha_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_bool_t           expected_notification = FALSE; 
    fbe_notification_data_changed_info_t * pDataChangeInfo = NULL;

    pDataChangeInfo = &update_object_msg->notification_info.notification_data.data_change_info;

    if((expected_object_id == update_object_msg->object_id) && 
       (expected_device_type == pDataChangeInfo->device_mask) &&
       (expected_class_id == FBE_CLASS_ID_BASE_BOARD))
    {
        switch(expected_device_type)
        {
            case FBE_DEVICE_TYPE_IOMODULE:
            case FBE_DEVICE_TYPE_BACK_END_MODULE:
            case FBE_DEVICE_TYPE_MEZZANINE:
            case FBE_DEVICE_TYPE_MGMT_MODULE: 
            case FBE_DEVICE_TYPE_SUITCASE:
            case FBE_DEVICE_TYPE_BMC:
            case FBE_DEVICE_TYPE_PS:
            case FBE_DEVICE_TYPE_FAN:
                if((expected_phys_location.sp == pDataChangeInfo->phys_location.sp) &&
                   (expected_phys_location.slot == pDataChangeInfo->phys_location.slot))
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

static void moksha_test_verify_io_module_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                           dwWaitResult;
    fbe_status_t                    fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t   ioModuleInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying IO Module Stale Data ===\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_IOMODULE;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_IO_MODULE, 9);

    /********************** DISABLE the Timestamp *************************/

    /* Disable the Mgmt module timestamp to verify the stale data. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the BEM info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified IO module Stale Data ===\n");

    /********************** ENABLE the Timestamp *************************/

    /* Enable the Mgmt module timestamp to verify the stale data. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the IO Annex info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_iom_info(objectId, &ioModuleInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ioModuleInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

    return;
}

static void moksha_test_verify_bem_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                           dwWaitResult;
    fbe_status_t                    fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       componentCount = 0;
    fbe_board_mgmt_io_comp_info_t   bemInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying BEM Stale Data ===\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    fbeStatus = fbe_api_board_get_bem_count(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount > 0)
    {
        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;

        /* Set the max_timeout value for the component */
        fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_BEM, 9);

        /********************** DISABLE the Timestamp *************************/

        /* Disable the Mgmt module timestamp to verify the stale data. */
        sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
        sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;
    
        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    
        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the BEM info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_bem_info(objectId, &bemInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(bemInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verified BEM Stale Data ===\n");

        /********************** ENABLE the Timestamp *************************/

        /* Enable the BEM timestamp. */
        sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
        sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the BEM info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_bem_info(objectId, &bemInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(bemInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO BEM present. ***\n");
    }

    return;
}

static void moksha_test_verify_mgmt_module_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                               dwWaitResult;
    fbe_status_t                        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           componentCount = 0;
    fbe_board_mgmt_mgmt_module_info_t   mgmtModInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Mgmt module Stale Data ===\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    fbeStatus = fbe_api_board_get_mgmt_module_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount > 0)
    {
        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_MGMT_MODULE;

        /* Set the max_timeout value for the component */
        fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_MGMT_MODULE, 9);

        /********************** DISABLE the Timestamp *************************/

        /* Disable the Mgmt module timestamp to verify the stale data. */
        sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
        sfi_mask_data->sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Mgmt module info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(mgmtModInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verified Mgmt module Stale Data ===\n");

        /********************** ENABLE the Timestamp *************************/

        /* Enable the Mezzanine timestamp. */
        sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
        sfi_mask_data->sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Mezzanine info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(mgmtModInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO Mgmt module present in Sentry CPU module. ***\n");
    }

    return;
}

static void moksha_test_verify_mezzanine_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                           dwWaitResult;
    fbe_status_t                    fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t   mezzInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Mezzanine Stale Data ===\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_MEZZANINE;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_MEZZANINE, 6);

    /********************** DISABLE the Timestamp *************************/
    /* Disable the Mezzanine timestamp to verify the stale data. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the Mezzanine info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mezzInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified Mezzanine Stale Data ===\n");

    /********************** ENABLE the Timestamp *************************/

    /* Enable the Mezzanine timestamp. */
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the Mezzanine info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(mezzInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

    return;
}

static void moksha_test_verify_power_supply_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                       dwWaitResult;
    fbe_status_t                fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_power_supply_info_t     psInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Power Supply Stale Data ===\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_PS;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_POWER_SUPPLY, 5);

    /********************** DISABLE the Timestamp *************************/
    /* Disable the Power Supply timestamp to verify the stale data. */
    sfi_mask_data->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the Power Supply info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified Power Supply Stale Data ===\n");

    /********************** ENABLE the Timestamp *************************/

    /* Enable the Power Supply timestamp. */
    sfi_mask_data->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the Power Supply info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

    return;
}


static void moksha_test_verify_cooling_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                       dwWaitResult;
    fbe_status_t                fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   componentCount = 0;
    fbe_cooling_info_t          coolingInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Cooling Stale Data ===\n");

    fbeStatus = fbe_api_board_get_cooling_count(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount > 0)
    {
        fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    
        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_FAN;

        /* Set the max_timeout value for the component */
        fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_COOLING_COMPONENT, 9);

        /********************** DISABLE the Timestamp *************************/

        /* Disable the Cooling timestamp to verify the stale data. */
        sfi_mask_data->structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
        sfi_mask_data->sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;
    
        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    
        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
        /* Get the Cooling info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_cooling_info(objectId, &coolingInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(coolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verified Cooling Stale Data ===\n");

        /********************** ENABLE the Timestamp *************************/

        /* Enable the Cooling timestamp. */
        sfi_mask_data->structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
        sfi_mask_data->sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Cooling info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_cooling_info(objectId, &coolingInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(coolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO Cooling Component present in Sentry CPU module. ***\n");
    }

    return;
}


static void moksha_test_verify_suitcase_stale_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;

    fbe_board_mgmt_suitcase_info_t      SuitcaseInfo = {0};

    mut_printf(MUT_LOG_TEST_STATUS, " === Verifying Suitcase Stale Data ===\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_SUITCASE;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_SUITCASE, 6);

    /********************** DISABLE the Timestamp *************************/

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_SUITCASE;

    /* Disable the Suitcase timestamp to verify the stale data. */
    sfi_mask_data->structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_DISABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the Suitcase info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_suitcase_info(objectId, &SuitcaseInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(SuitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified Suitcase Stale Data ===\n");

    /********************** ENABLE the Timestamp *************************/

    /* Enable the Suitcase timestamp. */
    sfi_mask_data->structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_ENABLE_TIMESTAMP;
    sfi_mask_data->sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;

    fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    /* Wait 60 seconds for xaction status data change notification. */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Get the Suitcase info and verify if the Xaction status change is reflected in base_board or not */
    fbeStatus = fbe_api_board_get_suitcase_info(objectId, &SuitcaseInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(SuitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);

    return;
}



