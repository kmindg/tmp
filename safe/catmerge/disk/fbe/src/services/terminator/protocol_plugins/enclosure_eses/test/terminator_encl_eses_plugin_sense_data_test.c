/***************************************************************************
 *  terminator_encl_eses_plugin_sense_data_test.c
 ***************************************************************************
 *
 *  Description
 *      Test for eses encl plugin code
 *      
 *
 *  History:
 *     Oct08   Created
 *    
 ***************************************************************************/


#include "terminator_encl_eses_plugin_test.h"
#include "terminator_sas_io_api.h"


static fbe_u8_t expected_sense_info_buffer[FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE];

/* Local function definitions */
static void terminator_check_returned_sense_data(fbe_payload_ex_t *payload);

void terminator_encl_eses_plugin_sense_data_test(void)
{
    fbe_status_t status;
    fbe_payload_ex_t payload;
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u8_t *cdb = NULL;

    // This is common to all the tests
    expected_sense_info_buffer[0] = 0x70;
    expected_sense_info_buffer[7] = 10;

    memset(&payload, 0, sizeof(payload));
    payload_cdb_operation = fbe_payload_ex_allocate_cdb_operation(&payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);

    status = fbe_payload_ex_increment_cdb_operation_level(&payload);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sg_p = terminator_eses_test_alloc_memory_with_sg(1000);
    MUT_ASSERT_TRUE(sg_p != NULL);
    status = fbe_payload_ex_set_sg_list(&payload, sg_p, 1); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb); 


    // Test 1, put invalid cdb opcode
    cdb[0] = FBE_SCSI_MAINT_OUT;
    expected_sense_info_buffer[2] = 0x0;
    expected_sense_info_buffer[2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[12] = FBE_SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
    expected_sense_info_buffer[13] = FBE_SCSI_ASCQ_INVALID_COMMAND_OPERATION_CODE; 
    status = fbe_terminator_sas_virtual_phy_payload_io(&payload, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    terminator_check_returned_sense_data(&payload);


    // Test 2, Put correct cdb code but invalid other fields in cdb.
    cdb[0] = FBE_SCSI_RECEIVE_DIAGNOSTIC;
    expected_sense_info_buffer[2] = 0x0;
    expected_sense_info_buffer[2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[12] = FBE_SCSI_ASC_INVALID_FIELD_IN_CDB;
    expected_sense_info_buffer[13] = FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB; 
    status = fbe_terminator_sas_virtual_phy_payload_io(&payload, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    terminator_check_returned_sense_data(&payload);

    // Test 2, Put correct cdb  but invalid page code
    cdb[0] = FBE_SCSI_RECEIVE_DIAGNOSTIC;
    cdb[1] = 0x1;
    cdb[2] = SES_PG_CODE_EMM_SYS_LOG_PG;
    expected_sense_info_buffer[2] = 0x0;
    expected_sense_info_buffer[2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[12] = FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION;
    expected_sense_info_buffer[13] = FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION; 
    status = fbe_terminator_sas_virtual_phy_payload_io(&payload, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    terminator_check_returned_sense_data(&payload);

    //Test 3, sense buffer test for send diagnotic page
    payload_cdb_operation->cdb[0] = FBE_SCSI_SEND_DIAGNOSTIC;
    payload_cdb_operation->cdb[1] = 0x10;
    payload_cdb_operation->cdb[5] = 1;
    expected_sense_info_buffer[2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[12] = FBE_SCSI_ASC_INVALID_FIELD_IN_CDB;
    expected_sense_info_buffer[13] = FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB; 
    status = fbe_terminator_sas_virtual_phy_payload_io(&payload, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    terminator_check_returned_sense_data(&payload);

    // test 4, invalid command in sglist
    payload_cdb_operation->cdb[0] = FBE_SCSI_SEND_DIAGNOSTIC;
    payload_cdb_operation->cdb[1] = 0x10;
    payload_cdb_operation->cdb[5] = 0;
    expected_sense_info_buffer[2] = FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    expected_sense_info_buffer[12] = FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION;
    expected_sense_info_buffer[13] = FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION; 
    status = fbe_terminator_sas_virtual_phy_payload_io(&payload, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    terminator_check_returned_sense_data(&payload);
}


fbe_status_t terminator_virtual_phy_get_enclosure_type(fbe_terminator_device_ptr_t handle, fbe_sas_enclosure_type_t *encl_type)
{
    *encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;

    return(FBE_STATUS_OK);
}

static void terminator_check_returned_sense_data(fbe_payload_ex_t *payload)
{
    fbe_payload_cdb_operation_t *payload_cdb_operation;
    fbe_u8_t *sense_info_buffer = NULL;
    fbe_u8_t i = 0;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    MUT_ASSERT_TRUE(payload_cdb_operation != NULL);

    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_info_buffer);

    for(i=0; i<FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE; i++)
    {
        MUT_ASSERT_INT_EQUAL((int)(sense_info_buffer[i]), 
            expected_sense_info_buffer[i]);
    }
}

fbe_sg_element_t * terminator_eses_test_alloc_memory_with_sg( fbe_u32_t bytes )
{
    fbe_sg_element_t *sg_p;
    fbe_u8_t *sectors_p;

    /* Allocate the memory.
     */
    sectors_p = malloc(bytes);
    
    if (sectors_p == NULL)
    {
        return NULL;
    }

    /* Allocate the sg.
     */
    sg_p = malloc(sizeof(fbe_sg_element_t) * 5);

    if (sg_p == NULL)
    {
        free(sectors_p);
        return NULL;
    }

    /* Init the sgs with the memory we just allocated.
     */
    fbe_sg_element_init(&sg_p[0], bytes, sectors_p);
    fbe_sg_element_terminate(&sg_p[1]);
    
    return sg_p;
}