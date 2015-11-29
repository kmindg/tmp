/***************************************************************************
 *  terminator_eses_tests_payload.c
 ***************************************************************************
 *
 *  Description
 *      ESES enclosure sense data tests
 *      
 *
 *  History:
 *     Oct08   Created
 *    
 ***************************************************************************/

#include "terminator_test.h"

#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"

#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "terminator_eses_test.h"

/**********************************/
/*        local variables         */
/**********************************/
static fbe_u32_t fbe_cpd_shim_callback_count;
static fbe_semaphore_t      update_semaphore;
// One sense data buffer array for each enclosure ( 10 encls supported)
static fbe_u8_t expected_sense_info_buffer[10][FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE];
// Whenever the test issues the ESES command to an enclosure, this will reflect that
// particular enclosures ID to which the ESES command was issued.
static current_encl_id = 0;

/**********************************/
/*        local functions         */
/**********************************/
void init_sense_data();
static fbe_status_t fbe_cpd_shim_callback_function_mock(
    fbe_cpd_shim_callback_info_t * callback_info, 
    fbe_cpd_shim_callback_context_t context);
fbe_status_t terminator_miniport_api_test_sense_data_completion(
    fbe_payload_ex_t * payload,
    fbe_payload_ex_completion_context_t context);


void terminator_eses_sense_data_test(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t   encl_id;
    fbe_terminator_api_device_handle_t   drive_id;
    fbe_u32_t                       i = 0;
    fbe_u32_t                       context = 0;
    fbe_u32_t       port_index;
    fbe_sg_element_t *sg_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
    fbe_semaphore_init(&update_semaphore, 0, FBE_SEMAPHORE_MAX);
    init_sense_data();

    /*initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x111111111;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_terminator_api_insert_sas_port (0, &sas_port);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure*/
    // id 2
    sas_encl.backend_number = sas_port.backend_number;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure*/
    // id 4
    sas_encl.backend_number = sas_port.backend_number;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 4; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure*/
    // id 6
    sas_encl.uid[0] = 6; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (0, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to the second last enclosure*/
    // inserted in encl id 4
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status = fbe_terminator_api_insert_sas_drive (0, encl_id-2, 0, &sas_drive, &drive_id);/*add drive to port 0 encl 3*/
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	status =  fbe_terminator_miniport_api_register_payload_completion(0, 
        terminator_miniport_api_test_sense_data_completion, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // alloocate memory for the sglist
    sg_p = fbe_ldo_test_alloc_memory_with_sg(1000);

    // test 1, issue eses io to the virtual phy. There are no errors
    current_encl_id = 4;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);;
	fbe_semaphore_wait(&update_semaphore, NULL);

    // test 2, issue eses io to the virtual phy. There are no errors  
    current_encl_id = 2;
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);;
	fbe_semaphore_wait(&update_semaphore, NULL);

    // Test 3. Ask terminator to unsupport the page and issue the scsi command.
    status = fbe_terminator_api_mark_eses_page_unsupported(FBE_SCSI_RECEIVE_DIAGNOSTIC, SES_PG_CODE_ENCL_STAT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    current_encl_id = 2;
    expected_sense_info_buffer[current_encl_id][2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[current_encl_id][12] = FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION;
    expected_sense_info_buffer[current_encl_id][13] = FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION;

    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_semaphore_wait(&update_semaphore, NULL);

    //test 4. Ask terminator to unsupport the page again and issue the scsi command.
    status = fbe_terminator_api_mark_eses_page_unsupported(FBE_SCSI_RECEIVE_DIAGNOSTIC, SES_PG_CODE_ENCL_STAT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);;
    
    current_encl_id = 4;
    expected_sense_info_buffer[current_encl_id][2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[current_encl_id][12] = FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION;
    expected_sense_info_buffer[current_encl_id][13] = FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION;

    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_semaphore_wait(&update_semaphore, NULL);

    //test 5. issue the scsi command. The page is still unsupported
    status = fbe_terminator_api_mark_eses_page_unsupported(FBE_SCSI_RECEIVE_DIAGNOSTIC, SES_PG_CODE_ENCL_STAT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);;
    current_encl_id = 4;
    expected_sense_info_buffer[current_encl_id][2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[current_encl_id][12] = FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION;
    expected_sense_info_buffer[current_encl_id][13] = FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION;
    status = terminator_miniport_api_issue_eses_cmd(0, 5, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_semaphore_wait(&update_semaphore, NULL);

    // Test 6. Ask terminator to unsupport the page and issue the scsi command.
    status = fbe_terminator_api_mark_eses_page_unsupported(FBE_SCSI_RECEIVE_DIAGNOSTIC, SES_PG_CODE_EMC_ENCL_STAT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);;
    current_encl_id = 2;
    expected_sense_info_buffer[current_encl_id][2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[current_encl_id][12] = FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION;
    expected_sense_info_buffer[current_encl_id][13] = FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION;
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_RECEIVE_DIAGNOSTIC, &ses_pg_code_emc_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_semaphore_wait(&update_semaphore, NULL);

    // Issue invalid cdb command.
    current_encl_id = 2;
    expected_sense_info_buffer[current_encl_id][2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[current_encl_id][12] = FBE_SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
    expected_sense_info_buffer[current_encl_id][13] = FBE_SCSI_ASCQ_INVALID_COMMAND_OPERATION_CODE;
    status = terminator_miniport_api_issue_eses_cmd(0, 3, FBE_SCSI_REZERO, &ses_pg_code_encl_stat, sg_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	fbe_semaphore_wait(&update_semaphore, NULL);

    /* Alex A.: it is now important that we destroy Terminator each time we initialize it. */
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}

// The completion function that will be called by terminator
fbe_status_t terminator_miniport_api_test_sense_data_completion(
    fbe_payload_ex_t * payload,
    fbe_payload_ex_completion_context_t context)
{
    fbe_status_t status;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *sense_info_buffer = NULL;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u8_t i;
   
    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);

    status = fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_info_buffer);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for(i=0; i<FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE; i++)
    {
        MUT_ASSERT_INT_EQUAL((int)(sense_info_buffer[i]), 
            expected_sense_info_buffer[current_encl_id][i]);
    }
    fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

void init_sense_data()
{
    fbe_u8_t i;

    for(i=0; i<10; i++)
    {
        expected_sense_info_buffer[i][0] = 0x70;
        expected_sense_info_buffer[i][7] = 10;
    }
}

/**************************************************************
 * DUMMY FUNCTION
 **************************************************************/
static fbe_status_t fbe_cpd_shim_callback_function_mock(
    fbe_cpd_shim_callback_info_t * callback_info, 
    fbe_cpd_shim_callback_context_t context)
{
    return(FBE_STATUS_OK);
}
