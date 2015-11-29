/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file thomas_the_tank_engine_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests firmware download with error handling and recovery.
 *
 * @version
 *   03/14/2011 - Created.  chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"

#include "esp_tests.h"  
#include "pp_utils.h"  
#include "sep_utils.h"  
#include "neit_utils.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "EspMessages.h"
#include "fbe_test_common_utils.h"
#include "sir_topham_hatt_test.h"



/*************************
 *   GLOBALS
 *************************/

char * thomas_the_tank_engine_short_desc = "Drive Firmware Download";
char * thomas_the_tank_engine_long_desc ="\
Tests the online firwmare download with error handling and recovery\n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] 1 SAS PMC port\n\
        [PP] 1 SAS VIPER enclosure\n\
        [PP] 15 SAS drives/drive\n\
        [PP] 15 logical drives/drive\n\
STEP 1: Bring up the initial topology.\n\
STEP 2: Test firmware download with bad firmware.\n\
STEP 3: Test firmware download with no qualified drive.\n\
STEP 4: Test firmware download with recovered WRITE BUFFER errors.\n\
STEP 5: Test firmware download with non-recovered WRITE BUFFER errors.\n\
STEP 6: Test firmware download with wrong revision after download.\n";


/*************************
 *   GLOBAL DATA
 *************************/
static sir_topham_hatt_select_drives_t selected_drive_array[] =
{
    {
        3,
        {
            {0, 0, 5, 0}, 
            {0, 0, 6, 0}, 
            {0, 0, 7, 0}, 
        },
    },
    {
        1,
        {
            {0, 0, 8, 0}, 
        },
    },
    {
        1,
        {
            {0, 0, 9, 0}, 
        },
    },
};

/*************************
 *   FUNCTION PROTOTYPES
 *************************/



/*!**************************************************************
 * thomas_the_tank_engine_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/16/2011 - Created.  chenl6
 ****************************************************************/
void thomas_the_tank_engine_test(void)
{
    fbe_status_t status;
    fbe_drive_mgmt_fw_info_t fw_info;
    fbe_drive_mgmt_download_process_status_t dl_status;
    sir_topham_hatt_select_drives_t * dl_drives;
    fbe_object_id_t pdo_object_id;
    fbe_fdf_header_block_t  *header_p;
    fbe_fdf_serviceability_block_t *service_block;
    fbe_protocol_error_injection_error_record_t record;
    fbe_protocol_error_injection_record_handle_t record_handle1;
    fbe_drive_selected_element_t *drive;
    fbe_u32_t dl_count;
    fbe_drive_mgmt_download_drive_status_request_t  drive_request;


 /****************************************************************
 * STEP 2: download fw with bad firmware
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Thomas the Tank Engine TEST: downloading with bad firmware.\n");
    dl_drives = &selected_drive_array[0];

    /* Create fw image */
    sir_topham_hatt_create_fw(FBE_SAS_DRIVE_SIM_520, &fw_info);
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);
    header_p = fw_info.header_image_p;
    header_p->cflags &= ~FBE_FDF_CFLAGS_TRAILER_PRESENT;

    /* Send download to ESP */
    status = fbe_api_esp_drive_mgmt_download_fw(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    fbe_api_sleep(1000);
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_MGMT_FWDL_PROCESS_FAILED, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_MGMT_FWDL_PROCESS_FAIL_MISSING_TLA);

    header_p->cflags |= FBE_FDF_CFLAGS_TRAILER_PRESENT;

 /****************************************************************
 * STEP 3: download fw without qualified drive
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Thomas the Tank Engine TEST: downloading with no qualified drive.\n");
    fbe_copy_memory(header_p->vendor_id, "FUJITSU ", 8);
    /* Send download to ESP */
    status = fbe_api_esp_drive_mgmt_download_fw(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_MGMT_FWDL_PROCESS_FAILED, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_MGMT_FWDL_PROCESS_FAIL_NO_DRIVES_TO_UPGRADE);

    fbe_copy_memory(header_p->vendor_id, "SEAGATE ", 8);

 /****************************************************************
 * STEP 4: download fw with recovered WRITE BUFFER errors
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Thomas the Tank Engine TEST: Recovering from WB errors.\n");
    dl_drives = &selected_drive_array[1];
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);
    drive = &dl_drives->selected_drives[0];
    dl_count = 2;

    /* Add error record */
    fbe_test_neit_utils_init_error_injection_record(&record);
    status = fbe_api_get_physical_drive_object_id_by_location(drive->bus,
                                                              drive->enclosure,
                                                              drive->slot,
                                                              &pdo_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    record.object_id = pdo_object_id;
    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status =
        FBE_PORT_REQUEST_STATUS_SUCCESS;
    record.lba_start = 0x0;
    record.lba_end = 0xF00000;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        FBE_SCSI_WRITE_BUFFER;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = 
        0xFF;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = 
        0x01;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = 
        0x00;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = 
        0x00; 
    record.num_of_times_to_insert = dl_count - 1;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Send download */
    status = fbe_api_esp_drive_mgmt_download_fw(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_MGMT_FWDL_PROCESS_SUCCESSFUL, 80);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_MGMT_FWDL_PROCESS_NO_FAILURE);

    /* Check all drives done download */
    drive_request.bus = drive->bus; 
    drive_request.enclosure = drive->enclosure; 
    drive_request.slot = drive->slot;
#if 0
    status = fbe_api_esp_drive_mgmt_get_download_drive_status(&drive_request);
#endif
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(drive_request.status.state == FBE_DRIVE_MGMT_FWDL_COMPLETE);
    MUT_ASSERT_TRUE(drive_request.status.fail_code == FBE_DRIVE_MGMT_FWDL_FAIL_NO_ERROR);
    MUT_ASSERT_TRUE(drive_request.status.dl_count == dl_count);

    /* Stop the error injection */
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the error injection records */
    status = fbe_api_protocol_error_injection_remove_record(record_handle1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

 /****************************************************************
 * STEP 5: download fw with non-recovered WRITE BUFFER errors
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Thomas the Tank Engine TEST: Failure with non-recovered WB errors.\n");
    dl_drives = &selected_drive_array[2];
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);
    drive = &dl_drives->selected_drives[0];
    dl_count = 2;

    /* Add error record */
    status = fbe_api_get_physical_drive_object_id_by_location(drive->bus,
                                                              drive->enclosure,
                                                              drive->slot,
                                                              &pdo_object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    record.object_id = pdo_object_id;
    record.num_of_times_to_insert = dl_count;
    
    status = fbe_api_protocol_error_injection_add_record(&record, &record_handle1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Send download */
    status = fbe_api_esp_drive_mgmt_download_fw(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /* Wait download process till done */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_MGMT_FWDL_PROCESS_FAILED, 100);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_MGMT_FWDL_PROCESS_FAIL_WRITE_BUFFER);

    /* Check all drives done download */
    drive_request.bus = drive->bus; 
    drive_request.enclosure = drive->enclosure; 
    drive_request.slot = drive->slot;
#if 0
    status = fbe_api_esp_drive_mgmt_get_download_drive_status(&drive_request);
#endif
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(drive_request.status.state == FBE_DRIVE_MGMT_FWDL_FAIL);
    MUT_ASSERT_TRUE(drive_request.status.fail_code == FBE_DRIVE_MGMT_FWDL_FAIL_WRITE_BUFFER);
    MUT_ASSERT_TRUE(drive_request.status.dl_count == dl_count);

    /* Stop the error injection */
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Remove the error injection records */
    status = fbe_api_protocol_error_injection_remove_record(record_handle1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

 /****************************************************************
 * STEP 6: download fw with wrong revision after download
 ****************************************************************/
    mut_printf(MUT_LOG_TEST_STATUS,"Thomas the Tank Engine TEST: downloading with wrong revision.\n");
    dl_drives = &selected_drive_array[1];
    sir_topham_hatt_select_drives(fw_info.header_image_p, dl_drives);
    service_block = (fbe_fdf_serviceability_block_t *)((fbe_u8_t *)header_p + header_p->trailer_offset);
    fbe_copy_memory(service_block->fw_revision, "ES02", 4);

    /* Send download to ESP */
    status = fbe_api_esp_drive_mgmt_download_fw(&fw_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check download process started */
    status = sir_topham_hatt_wait_download_process_state(&dl_status, FBE_DRIVE_MGMT_FWDL_PROCESS_FAILED, 100);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(dl_status.fail_code == FBE_DRIVE_MGMT_FWDL_PROCESS_FAIL_FW_REV_MISMATCH);

    fbe_copy_memory(header_p->vendor_id, "SEAGATE ", 8);

    /* Clean up */
    sir_topham_hatt_destroy_fw(&fw_info);

    return;
}
/******************************************
 * end thomas_the_tank_engine_test()
 ******************************************/

/****************************************
 * end file thomas_the_tank_engine_test.c
 ****************************************/

