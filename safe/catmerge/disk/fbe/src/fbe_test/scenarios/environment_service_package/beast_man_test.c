/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file beast_man_test.c
 ***************************************************************************
 *
 * @brief
 *  This test verifies internal SSD status reporting.. 
 *
 * @version
 *   29-Oct-2013 bphilbin - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * beast_man_short_desc = "Verify the internal SSD status reporting..";
char * beast_man_long_desc =
    "\n"
    "\n"
    "The beast_man scenario verifies the internal SSD status reporting.\n"
    ;
static fbe_status_t beast_man_test_specl_config(void);
static fbe_status_t beast_man_test_specl_config2(void);

void beast_man_test(void)
{
    fbe_u32_t ssd_count = 0;
    fbe_esp_board_mgmt_ssd_info_t ssd_info = {0};
    fbe_esp_board_mgmt_board_info_t board_info = {0};
    fbe_status_t status;
    fbe_device_physical_location_t location = {0};
    fbe_object_id_t board_object_id;
    fbe_board_mgmt_ssd_info_t pp_ssd_info = {0};


    status = fbe_api_esp_board_mgmt_getSSDCount(&ssd_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(2, ssd_count);

    status = fbe_api_esp_board_mgmt_getBoardInfo(&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    location.sp = board_info.sp_id;
    location.slot = 0;

    status = fbe_api_esp_board_mgmt_getSSDInfo(&location, &ssd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, ssd_info.ssdSelfTestPassed);
    MUT_ASSERT_INT_EQUAL(FBE_SSD_STATE_OK, ssd_info.ssdState);


    // Check the fault register status from the physical package
    status = fbe_api_get_board_object_id(&board_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    ssd_info.slot = 0;
    status = fbe_api_board_get_ssd_info(board_object_id, &pp_ssd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, pp_ssd_info.isPeerFaultRegFail);

    // Use SFI to inject a fault in the peer's SSD
#if 0  // sfi is not working at the moment, need to debug this further
    fbe_test_esp_common_destroy_all();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    mut_printf(MUT_LOG_LOW, "*** BeastMan Test Case 2, Peer faulted SSD***\n");
    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, beast_man_test_specl_config);


    beast_man_test_specl_config();

    status = fbe_api_board_get_ssd_info(board_object_id, &pp_ssd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, pp_ssd_info.isPeerFaultRegFail);

    // Check that the peer's SSD is marked faulted
    location.sp = SP_B;
    location.slot = 0;

    status = fbe_api_esp_board_mgmt_getSSDInfo(&location, &ssd_info);

    MUT_ASSERT_INT_EQUAL(FBE_SSD_STATE_FAULTED, ssd_info.ssdState);

#endif

}



void beast_man_test_load_test_env(void)
{

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, beast_man_test_specl_config2);

    
}

fbe_status_t beast_man_test_specl_config(void)
{
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};
    fbe_status_t                    status;

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));


    sfi_mask_data->structNumber = SPECL_SFI_FLT_EXP_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    

    sfi_mask_data->structNumber = SPECL_SFI_FLT_EXP_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;
    //sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_B].faultRegisterStatus[PRIMARY_FLT_REG].transactionStatus = EMCPAL_STATUS_SUCCESS;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_B].faultRegisterStatus[PRIMARY_FLT_REG].driveFault[0] = TRUE;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_A].faultRegisterStatus[PRIMARY_FLT_REG].driveFault[0] = TRUE;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_B].faultRegisterStatus[MIRROR_FLT_REG].driveFault[0] = TRUE;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_A].faultRegisterStatus[MIRROR_FLT_REG].driveFault[0] = TRUE;
    
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sleep(6000);

    return FBE_STATUS_OK;

}

fbe_status_t beast_man_test_specl_config2(void)
{
    #if 0
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};
    fbe_status_t                    status;

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    sfi_mask_data->structNumber = SPECL_SFI_FLT_EXP_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_B].faultRegisterStatus[PRIMARY_FLT_REG].transactionStatus = EMCPAL_STATUS_SUCCESS;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.fltExpSummary[SP_B].faultRegisterStatus[PRIMARY_FLT_REG].driveFault[0] = FALSE;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sleep(6000);

    #endif
    return FBE_STATUS_OK;

}


void beast_man_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/*************************
 * end file beast_man_test.c
 *************************/




