/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file chulbuli.c
 ***************************************************************************
 *
 * @brief
 *  This file contains handling of SMB errors for the PE components *
 * @version
 *   21/06/2010 Arun S - Created.
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

char * chulbuli_short_desc = "Test SMB errors for processor enclosure components.";
char * chulbuli_long_desc =
    "\n"
    "\n"
    "The Chulbuli scenario tests the SMB errors for physical package processor enclosure components.\n"
    "It includes: \n"
    "    - The SMB error verification for the IO module;\n"
    "    - The SMB error verification for the BEM;\n"
    "    - The SMB error verification for the Mgmt module;\n"
    "    - The SMB error verification for the Mezzanine;\n"
    "    - The SMB error verification for the Power Supply;\n"
    "    - The SMB error verification for the Cooling (Fan);\n"
    "    - The SMB error verification for the MCU;\n"
    "    - The SMB error verification for the Suitcase;\n"
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
static fbe_device_data_type_t   expected_data_type = FBE_DEVICE_DATA_TYPE_INVALID;
static fbe_device_physical_location_t expected_phys_location = {0};

static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;

static void chulbuli_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);

static void chulbuli_test_verify_io_module_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void chulbuli_test_verify_bem_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void chulbuli_test_verify_power_supply_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void chulbuli_test_verify_mgmt_module_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void chulbuli_test_verify_mezzanine_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void chulbuli_test_verify_cooling_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);
static void chulbuli_test_verify_suitcase_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data);

#define MAX_SMB_ERRORS  2

/* In case you happen to enable/add more SMB Error to the list,
 * please update the value of MAX_SMB_ERRORS as well without fail.
 */
fbe_u32_t smb_errors[MAX_SMB_ERRORS] =
{
    //STATUS_SMB_DEV_ERR,                          // Device error
    //STATUS_SMB_BUS_ERR,                          // Bus collision / busy
    STATUS_SMB_ARB_ERR,                          // Bus arbitration error
    //STATUS_SMB_FAILED_ERR,                       // Failed error (occurs when a 'kill' cmd is sent to terminate the transaction 
    //STATUS_SMB_TRANSACTION_IN_PROGRESS,          // Host Controller is busy
    //STATUS_SMB_BYTE_DONE_COMPLETE,               // A byte of data was transfered successfully
    //STATUS_SMB_TRANSACTION_COMPLETE,             // The transaction has terminated by signalling the INTR bit. 
    //STATUS_SMB_DEVICE_NOT_PRESENT,               // Device not physically inserted
    //STATUS_SMB_INVALID_BYTE_COUNT,               // Invalid Byte Count specified
    //STATUS_SMB_DEVICE_NOT_VALID_FOR_PLATFORM,    // Invalid Device for platform
    STATUS_SMB_SELECTBITS_FAILED,                // Failed to set the SelectBits on the SuperIO Mux.
    //STATUS_SMB_IO_TIMEOUT,                       // IO Timeout error on bus.
    //STATUS_SMB_INSUFFICIENT_RESOURCES,           // An errorTs could not be allocated 
    //STATUS_SMB_DEVICE_POWERED_OFF                // The device is powered OFF
};



void chulbuli(void)
{
    fbe_status_t        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     objectId;
    PSPECL_SFI_MASK_DATA    sfi_mask_data = {0};
                                                            
    fbe_semaphore_init(&sem, 0, 1);
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                  chulbuli_commmand_response_callback_function,
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

    chulbuli_test_verify_io_module_data(objectId, sfi_mask_data);
    chulbuli_test_verify_bem_data(objectId, sfi_mask_data);
    chulbuli_test_verify_mgmt_module_data(objectId, sfi_mask_data);
    chulbuli_test_verify_mezzanine_data(objectId, sfi_mask_data);
    chulbuli_test_verify_power_supply_data(objectId, sfi_mask_data);
    chulbuli_test_verify_cooling_data(objectId, sfi_mask_data);
    chulbuli_test_verify_suitcase_data(objectId, sfi_mask_data);

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified the SMB Error for all PE components ===\n");

    fbeStatus = fbe_api_notification_interface_unregister_notification(chulbuli_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);
    free(sfi_mask_data);

    return;
}

static void chulbuli_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
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

static void chulbuli_test_verify_io_module_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                   dwWaitResult;
    fbe_u32_t               index = 0;
    fbe_status_t            fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t ioModuleInfo = {0};
    fbe_board_mgmt_misc_info_t      miscInfo = {0};

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> IO MODULE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_IOMODULE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_IO_MODULE, 9);

    for(index = 0; index < MAX_SMB_ERRORS; index++)
    {
        /********************** SET the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

        /* Get the IO Module info. */
        sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the IO Module info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[ioModuleInfo.associatedSp].ioStatus[ioModuleInfo.slotNumOnBlade].transactionStatus = smb_errors[index];

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

        if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
        {
            mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

            fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

            mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
        }

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
    }

    return;
}

static void chulbuli_test_verify_bem_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                           dwWaitResult;
    fbe_u32_t                       index = 0;
    fbe_status_t                    fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       bemIndex = 0;
    fbe_u32_t                       componentCount = 0;
    fbe_board_mgmt_io_comp_info_t   bemInfo = {0};
    fbe_board_mgmt_misc_info_t      miscInfo = {0};

    fbeStatus = fbe_api_board_get_bem_count(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount > 0)
    {
        fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

        mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> BEM ===\n");

        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

        /* Set the max_timeout value for the component */
        fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_BEM, 9);

        for(index = 0; index < MAX_SMB_ERRORS; index++)
        {
            /********************** SET the SMB Errors *************************/
            mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

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
            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[bemInfo.associatedSp].ioStatus[bemIndex].transactionStatus = smb_errors[index];

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

            if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
            {
                mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

                fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
                MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

                mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
            }

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
        }
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO BEM present. ***\n");
    }

    return;
}

static void chulbuli_test_verify_mgmt_module_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                               dwWaitResult;
    fbe_u32_t                           index = 0;
    fbe_status_t                        fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           componentCount = 0;
    fbe_board_mgmt_mgmt_module_info_t   mgmtModInfo = {0};
    fbe_board_mgmt_misc_info_t          miscInfo = {0};

    fbeStatus = fbe_api_board_get_mgmt_module_count_per_blade(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount > 0)
    {
        fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

        mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> MGMT MODULE ===\n");

        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_MGMT_MODULE;
        expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

        /* Set the max_timeout value for the component */
        fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_MGMT_MODULE, 9);

        for(index = 0; index < MAX_SMB_ERRORS; index++)
        {
            /********************** SET the SMB Errors *************************/
            mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

            /* Get the Mgmt module info. */
            sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
            sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Set the Mgmt module info. */
            sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary->mgmtStatus->transactionStatus = smb_errors[index];

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Wait 60 seconds for xaction status data change notification. */
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
            MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

            mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

            /* Get the Mgmt module info and verify if the Xaction status change is reflected in base_board or not */
            fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(mgmtModInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

            if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
            {
                mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

                fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
                MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

                mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
            }

            /********************** CLEAR the SMB Errors *************************/
            mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

            /* Get the Mgmt module info. */
            sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
            sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.mgmtStatus.sfiMaskStatus = TRUE;

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Set the Mgmt module info. */
            sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary->mgmtStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Wait 60 seconds for xaction status data change notification. */
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
            MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

            /* Get the Mgmt module info and verify if the Xaction status change is reflected in base_board or not */
            fbeStatus = fbe_api_board_get_mgmt_module_info(objectId, &mgmtModInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(mgmtModInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
        }
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO Mgmt module present in Sentry CPU module. ***\n");
    }

    return;
}

static void chulbuli_test_verify_mezzanine_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                           dwWaitResult;
    fbe_u32_t                       index = 0;
    fbe_u32_t                       mezzIndex = 0;
    fbe_status_t                    fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_io_comp_info_t   mezzInfo = {0};
    fbe_board_mgmt_misc_info_t      miscInfo = {0};

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> MEZZANINE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_MEZZANINE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_MEZZANINE, 6);

    for(index = 0; index < MAX_SMB_ERRORS; index++)
    {
        /********************** SET the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

        /* Get the Mezzanine info. */
        sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Mezzanine info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
            while (mezzIndex < TOTAL_IO_MOD_PER_BLADE - 1)
            {
                if ((sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzInfo.associatedSp].ioStatus[mezzIndex].moduleType == IO_MODULE_TYPE_MEZZANINE) ||
                    (sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzInfo.associatedSp].ioStatus[mezzIndex].moduleType == IO_MODULE_TYPE_ONBOARD))
                {
                    break;
                }
                mezzIndex++;
            }
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzInfo.associatedSp].ioStatus[mezzIndex].transactionStatus = smb_errors[index];

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

        /* Get the Mezzanine info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(mezzInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

        if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
        {
            mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

            fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

            mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
        }

        /********************** CLEAR the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

        /* Get the Mezzanine info. */
        sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.ioModStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Mezzanine info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[mezzInfo.associatedSp].ioStatus[mezzIndex].transactionStatus = EMCPAL_STATUS_SUCCESS;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Mezzanine info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_mezzanine_info(objectId, &mezzInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(mezzInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    }

    return;
}

static void chulbuli_test_verify_power_supply_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                       dwWaitResult;
    fbe_u32_t                   index = 0;
    fbe_status_t                fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_power_supply_info_t     psInfo = {0};
    fbe_board_mgmt_misc_info_t  miscInfo = {0};

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> POWER SUPPLY ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_PS;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_POWER_SUPPLY, 5);

    for(index = 0; index < MAX_SMB_ERRORS; index++)
    {
        /********************** SET the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

        /* Get the Power Supply info. */
        sfi_mask_data->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Power Supply info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.psStatus.psSummary->psStatus->transactionStatus = smb_errors[index];

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

        /* Get the Power Supply info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

        if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
        {
            mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

            fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

            mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
        }

        /********************** CLEAR the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

        /* Get the Power Supply info. */
        sfi_mask_data->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Power Supply info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.psStatus.psSummary->psStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Power Supply info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_power_supply_info(objectId, &psInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(psInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    }

    return;
}


static void chulbuli_test_verify_cooling_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                       dwWaitResult;
    fbe_u32_t                   index = 0;
    fbe_status_t                fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   componentCount = 0;
    fbe_cooling_info_t          coolingInfo = {0};
    fbe_board_mgmt_misc_info_t  miscInfo = {0};

    fbeStatus = fbe_api_board_get_cooling_count(objectId, &componentCount);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    if(componentCount > 0)
    {
        fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

        mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> COOLING ===\n");

        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_FAN;
        expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

        /* Set the max_timeout value for the component */
        fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_COOLING_COMPONENT, 9);

        for(index = 0; index < MAX_SMB_ERRORS; index++)
        {
            /********************** SET the SMB Errors *************************/
            mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

            /* Get the Cooling info. */
            sfi_mask_data->structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
            sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Set the Cooling info. */
            sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.fanStatus.fanSummary[miscInfo.spid].fanPackStatus[coolingInfo.slotNumOnEncl].transactionStatus = smb_errors[index];

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Wait 60 seconds for xaction status data change notification. */
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
            MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

            mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

            /* Get the Cooling info and verify if the Xaction status change is reflected in base_board or not */
            fbeStatus = fbe_api_board_get_cooling_info(objectId, &coolingInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(coolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

            if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
            {
                mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

                fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
                MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

                mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
            }

            /********************** CLEAR the SMB Errors *************************/
            mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

            /* Get the Cooling info. */
            sfi_mask_data->structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
            sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Set the Cooling info. */
            sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
            sfi_mask_data->sfiSummaryUnions.fanStatus.fanSummary[miscInfo.spid].fanPackStatus[coolingInfo.slotNumOnEncl].transactionStatus = EMCPAL_STATUS_SUCCESS;

            fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

            /* Wait 60 seconds for xaction status data change notification. */
            dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
            MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

            /* Get the COOLING info and verify if the Xaction status change is reflected in base_board or not */
            fbeStatus = fbe_api_board_get_cooling_info(objectId, &coolingInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(coolingInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
        }
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "*** NO Cooling Component present in Sentry CPU module. ***\n");
    }

    return;
}


static void chulbuli_test_verify_suitcase_data(fbe_object_id_t objectId, PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    DWORD                           dwWaitResult;
    fbe_u32_t                       index = 0;
    fbe_status_t                    fbeStatus = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_misc_info_t      miscInfo = {0};
    fbe_board_mgmt_suitcase_info_t  SuitcaseInfo = {0};

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    mut_printf(MUT_LOG_TEST_STATUS, " === PE Component -> SUITCASE ===\n");

    /* We want to get notification from this object*/
    expected_object_id = objectId;
    expected_device_type = FBE_DEVICE_TYPE_SUITCASE;
    expected_data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    /* Set the max_timeout value for the component */
    fbe_api_board_set_comp_max_timeout(objectId, FBE_ENCL_SUITCASE, 6);

    for(index = 0; index < MAX_SMB_ERRORS; index++)
    {
        /********************** SET the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Injecting the SMB Error: 0x%x ===", smb_errors[index]);

        /* We want to get notification from this object*/
        expected_object_id = objectId;
        expected_device_type = FBE_DEVICE_TYPE_SUITCASE;

        /* Get the Suitcase info. */
        sfi_mask_data->structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Suitcase info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.suitcaseStatus.suitcaseSummary->suitcaseStatus->transactionStatus = smb_errors[index];

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        mut_printf(MUT_LOG_TEST_STATUS, " === Verifying the SMB Error ===");

        /* Get the Suitcase info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_suitcase_info(objectId, &SuitcaseInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(SuitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL);

        if (smb_errors[index] == STATUS_SMB_SELECTBITS_FAILED)
        {
            mut_printf(MUT_LOG_TEST_STATUS, " Verifying the SMB Select Bit");

            fbeStatus = fbe_api_board_get_misc_info(objectId, &miscInfo);
            MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(miscInfo.smbSelectBitsFailed == TRUE);

            mut_printf(MUT_LOG_TEST_STATUS, " Verified the SMB Select Bit");
        }

        /********************** CLEAR the SMB Errors *************************/
        mut_printf(MUT_LOG_TEST_STATUS, " === Clearing the SMB Error ===\n");

        /* Get the Suitcase info. */
        sfi_mask_data->structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
        sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Set the Suitcase info. */
        sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data->sfiSummaryUnions.suitcaseStatus.suitcaseSummary->suitcaseStatus->transactionStatus = EMCPAL_STATUS_SUCCESS;

        fbeStatus = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

        /* Wait 60 seconds for xaction status data change notification. */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

        /* Get the Suitcase info and verify if the Xaction status change is reflected in base_board or not */
        fbeStatus = fbe_api_board_get_suitcase_info(objectId, &SuitcaseInfo);
        MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(SuitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    }

    return;
}

