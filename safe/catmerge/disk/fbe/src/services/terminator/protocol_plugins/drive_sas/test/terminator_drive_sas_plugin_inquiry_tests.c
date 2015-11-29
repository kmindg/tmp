#include "terminator_drive_sas_plugin_test.h"
#include "terminator_sas_io_api.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_payload_ex.h"

fbe_u8_t expected_serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];

void drive_sas_plugin_inquiry_positive_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t        cmd_payload;
    fbe_payload_cdb_operation_t* op_exe_cdb = NULL;    
    fbe_u8_t        *b_ptr = NULL; 
    terminator_sas_drive_inq_data_t	*inq_data = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_u8_t        drive_handle[1]; /* get an arbitrary pointer for the drive handle.  Mock does not use the handle!!! */

    /* Setup the  payload */
    status = fbe_payload_ex_init(&cmd_payload);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    op_exe_cdb = fbe_payload_ex_allocate_cdb_operation(&cmd_payload);
    MUT_ASSERT_TRUE(op_exe_cdb != NULL);
    
    status = fbe_payload_ex_increment_cdb_operation_level(&cmd_payload);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
    /* Setup the payload with the data we expect to receive/test */
    sg_list = (fbe_sg_element_t  *)malloc(520+(sizeof(fbe_sg_element_t) * 2));
    sg_list[0].count = FBE_SCSI_INQUIRY_DATA_SIZE;
    sg_list[0].address = (fbe_u8_t *)(sg_list + 2);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    status = fbe_payload_ex_set_sg_list(&cmd_payload, sg_list, 1);
    
    /* just give a fake device id and timeout value */
    
    status = fbe_payload_cdb_build_inquiry(op_exe_cdb, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Give it an arbitrary port number */

    memset(expected_serial_number, 'A', FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    /* Setup the sas_drive_info with the data we expect to receive/test */

    /* Call the function under test */
    status = fbe_terminator_sas_drive_payload(&cmd_payload, drive_handle);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Verify the data returned are as expected */
    MUT_ASSERT_INT_NOT_EQUAL(0, cmd_payload.sg_list->count);
    b_ptr = (fbe_u8_t *)cmd_payload.sg_list->address;
    MUT_ASSERT_BUFFER_EQUAL((char*)&expected_serial_number, 
                            (char*)(b_ptr+FBE_SCSI_INQUIRY_SERIAL_NUMBER_OFFSET), 
                            FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    free(sg_list);
    
    status = fbe_payload_ex_release_cdb_operation(&cmd_payload, op_exe_cdb);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    
    
    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}


/* These are the mock functions that simulates Terminator returns */
fbe_status_t terminator_drive_get_type(fbe_terminator_device_ptr_t handle, fbe_sas_drive_type_t *drive_type)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        return FBE_STATUS_OK;
    }
}
fbe_status_t terminator_drive_get_serial_number(fbe_terminator_device_ptr_t handle, fbe_u8_t **serial_number)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        memset(expected_serial_number, 'A', FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
        *serial_number = expected_serial_number;
        return FBE_STATUS_OK;
    }
}

fbe_status_t terminator_drive_get_state(fbe_terminator_device_ptr_t handle, terminator_sas_drive_state_t * drive_state)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {        
        *drive_state = TERMINATOR_SAS_DRIVE_STATE_OK;
        return FBE_STATUS_OK;
    }
}

fbe_status_t terminator_get_port_index(fbe_terminator_device_ptr_t handle, fbe_u32_t * port_number)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *port_number = 0;
        return FBE_STATUS_OK;
    }
}

fbe_status_t terminator_get_drive_enclosure_number(fbe_terminator_device_ptr_t handle, fbe_u32_t * enclosure_number)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *enclosure_number = 0;
        return FBE_STATUS_OK;
    }
}
fbe_status_t terminator_get_drive_slot_number(fbe_terminator_device_ptr_t handle, fbe_u32_t * slot_number)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *slot_number = 0;
        return FBE_STATUS_OK;
    }
}

fbe_status_t fbe_terminator_get_io_mode(fbe_terminator_io_mode_t *io_mode)
{
    *io_mode = FBE_TERMINATOR_IO_MODE_DISABLED;
    return FBE_STATUS_OK;
}
