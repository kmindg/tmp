/**************************************************************************
 * Copyright (C) EMC Corporation 2008 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
***************************************************************************/

#include "terminator_test.h"
#include "terminator_enclosure.h"
#include "terminator_drive.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"
#include "fbe_terminator_file_api.h"
#include "fbe/fbe_winddk.h"
#include "terminator_virtual_phy.h"

fbe_u32_t fbe_cpd_shim_callback_count;
static fbe_status_t fbe_cpd_shim_callback_function_mock(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    switch (callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN: device_type: %d", callback_info->info.login.device_type);
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT:
        mut_printf(MUT_LOG_HIGH, "FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT: device_type: %d", callback_info->info.logout.device_type);
        break;
    default:
        break;
    }
    fbe_cpd_shim_callback_count++;
    return FBE_STATUS_OK;
}

void terminator_enclosure_firmware_download_activate_test(void)
{
    fbe_status_t 			            status;
    fbe_terminator_board_info_t 	    board_info, temp_board_info;
    fbe_terminator_sas_port_info_t	    sas_port;
    fbe_terminator_sas_encl_info_t	    sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    
    fbe_terminator_api_device_handle_t	port1_handle;
    fbe_terminator_api_device_handle_t	encl1_0_handle, encl1_1_handle;
    fbe_terminator_api_device_handle_t	drive_handle;
    fbe_u32_t                           port_index;

    fbe_terminator_device_ptr_t enclosure_ptr;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    tdr_status_t tdr_status;

    terminator_sas_virtual_phy_info_t *virtual_phy_info = NULL;
    terminator_enclosure_firmware_new_rev_record_t *new_rev_record = NULL;
    CHAR *lcc_old_revision_number_a = "54321", *lcc_new_revision_number_a = "12345";
    CHAR *lcc_old_revision_number_b = "22222", *lcc_new_revision_number_b = "45685";
    CHAR ver_num[5];
    fbe_u32_t current_count;
    fbe_u8_t download_status;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

    /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "***** %s: terminator initialized. *****", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*verify platform type*/
    status = fbe_terminator_api_get_board_info(&temp_board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(board_info.platform_type, temp_board_info.platform_type);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 1;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 1*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 1*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 1 encl 1 */
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 1;
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    status  = fbe_terminator_api_insert_sas_drive (encl1_1_handle, 3, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 1 drive*/
    MUT_ASSERT_INT_EQUAL(5, fbe_terminator_api_get_devices_count());

    // give some time for the logins to complete
    EmcutilSleep(2000);

    // get virtual phy pointer
    tdr_status = fbe_terminator_device_registry_get_device_ptr(encl1_0_handle, &enclosure_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = terminator_get_enclosure_virtual_phy_ptr(enclosure_ptr, &virtual_phy_ptr);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // set to update enclosure firmware revision number
    status = terminator_set_need_update_enclosure_firmware_rev(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // set two time intervals to a short period for testing
    status = terminator_set_enclosure_firmware_activate_time_interval(virtual_phy_ptr, 2000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = terminator_set_enclosure_firmware_reset_time_interval(virtual_phy_ptr, 2000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // first set old firmware revision number to LCC at both sides
    status = terminator_eses_set_ver_num(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, SIDE_A, SES_COMP_TYPE_LCC_MAIN, lcc_old_revision_number_a);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = terminator_eses_set_ver_num(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, SIDE_B, SES_COMP_TYPE_LCC_MAIN, lcc_old_revision_number_b);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // call firmware download for LCC at both sides
    status = terminator_enclosure_firmware_download(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, TERMINATOR_SP_A,
                                                                SES_COMP_TYPE_LCC_MAIN, 0, lcc_new_revision_number_a);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = terminator_enclosure_firmware_download(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, TERMINATOR_SP_B,
                                                                SES_COMP_TYPE_LCC_MAIN, 1, lcc_new_revision_number_b);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // verify new revision number is kept in queue
    virtual_phy_info = (terminator_sas_virtual_phy_info_t *)base_component_get_attributes((base_component_t *)virtual_phy_ptr);
    MUT_ASSERT_NOT_NULL(virtual_phy_info);
    new_rev_record = (terminator_enclosure_firmware_new_rev_record_t *)fbe_queue_front(&virtual_phy_info->new_firmware_rev_queue_head);
    MUT_ASSERT_NOT_NULL(new_rev_record);
    MUT_ASSERT_BUFFER_EQUAL(lcc_new_revision_number_a, new_rev_record->new_rev_number, sizeof(new_rev_record->new_rev_number));
    MUT_ASSERT_NOT_NULL(new_rev_record->queue_element.next);
    new_rev_record = (terminator_enclosure_firmware_new_rev_record_t *)(new_rev_record->queue_element.next);
    MUT_ASSERT_BUFFER_EQUAL(lcc_new_revision_number_b, new_rev_record->new_rev_number, sizeof(new_rev_record->new_rev_number));

    // call firmware activate for SPA side LCC, and require to update the revision ID
    status = terminator_enclosure_firmware_activate(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, TERMINATOR_SP_A, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // wait for a while, test activating is not finished
    mut_printf(MUT_LOG_LOW, "wait for activating SPA side LCC...");
    EmcutilSleep(virtual_phy_info->reset_time_intervel);
    status = terminator_eses_get_download_status(virtual_phy_ptr, &download_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(ESES_DOWNLOAD_STATUS_NONE, download_status);

    // wait for activating finishing
    EmcutilSleep(virtual_phy_info->activate_time_intervel + 1000);

    // verify that activating successfully and download status be ESES_DOWNLOAD_STATUS_NONE
    status = terminator_eses_get_download_status(virtual_phy_ptr, &download_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(ESES_DOWNLOAD_STATUS_NONE, download_status);

    // verify new revision number is updated
    status = terminator_eses_get_ver_num(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, SIDE_A, SES_COMP_TYPE_LCC_MAIN, ver_num);
    MUT_ASSERT_BUFFER_EQUAL(lcc_new_revision_number_a, ver_num, sizeof(ver_num));

    // call firmware activate for SPB side LCC
    status = terminator_enclosure_firmware_activate(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, TERMINATOR_SP_B, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // before the activating thread starting actual work, set the download status to bad one,
    // this should fail the activating
    status = terminator_eses_set_download_status(virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // save current fbe_cpd_shim_callback_count
    current_count = fbe_cpd_shim_callback_count;

    // wait for activating
    mut_printf(MUT_LOG_LOW, "wait for activating SPB side LCC...");
    EmcutilSleep(virtual_phy_info->reset_time_intervel + virtual_phy_info->activate_time_intervel + 1000);

    // fbe_cpd_shim_callback_count should keep unchanged becasue activating fails
    MUT_ASSERT_INT_EQUAL(current_count, fbe_cpd_shim_callback_count);

    // verify new revision number is not updated
    status = terminator_eses_get_ver_num(virtual_phy_ptr, SES_SUBENCL_TYPE_LCC, SIDE_B, SES_COMP_TYPE_LCC_MAIN, ver_num);
    MUT_ASSERT_BUFFER_NOT_EQUAL(lcc_new_revision_number_b, ver_num, sizeof(ver_num));

    // verify that activating failed and download status be ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM
    status = terminator_eses_get_download_status(virtual_phy_ptr, &download_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM, download_status);

    // verify new revision queue is empty
    MUT_ASSERT_TRUE(fbe_queue_is_empty(&virtual_phy_info->new_firmware_rev_queue_head));

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}
