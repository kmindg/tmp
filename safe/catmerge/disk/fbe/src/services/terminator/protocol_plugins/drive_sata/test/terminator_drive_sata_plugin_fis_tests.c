#include "terminator_drive_sata_plugin_test.h"
#include "terminator_sas_io_api.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_payload_fis_operation.h"

fbe_u8_t expected_serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];

void drive_sata_plugin_fis_command1_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_io_t terminator_io;
    fbe_payload_ex_t   payload;
    fbe_u8_t        drive_handle[1]; /* get an arbitrary pointer for the drive handle.  Mock does not use the handle!!! */

    /* TODO: Setup the payload */
    fbe_payload_ex_init(&payload);
    fbe_payload_ex_allocate_fis_operation (&payload);
    fbe_payload_ex_increment_fis_operation_level(&payload);
    /* Call the function under test */
    fbe_zero_memory((void *)&terminator_io, sizeof(fbe_terminator_io_t));
    terminator_io.payload = &payload;
    status = fbe_terminator_sata_drive_payload(terminator_io, drive_handle);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* TODO: Use MUT_ASSERT_XXX to Verify the data returned are as expected */
    
    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}


/* USE MOCK function like these 
These are the mock functions that simulates Terminator returns */
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
