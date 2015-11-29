#include "terminator_drive_sata_plugin_test.h"
#include "fbe/fbe_types.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "terminator_sas_io_neit.h"
#include "fbe_terminator.h"
#include "fbe/fbe_emcutil_shell_include.h"


int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *terminator_drive_sata_plugin_fis_suite;        /* pointer to testsuite structure */
    // TODO: re-enable this when we have a error injection test suite
    // mut_testsuite_t *terminator_drive_sata_plugin_error_injection_suite;     /* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

    terminator_drive_sata_plugin_fis_suite = MUT_CREATE_TESTSUITE("terminator_drive_sata_plugin_fis_suite");   /* testsuite is created */
    MUT_ADD_TEST(terminator_drive_sata_plugin_fis_suite, drive_sata_plugin_fis_command1_test, NULL, NULL);

    // TODO: re-enable this when we have a error injection test suite
//  terminator_drive_sata_plugin_error_injection_suite = MUT_CREATE_TESTSUITE("terminator_drive_sata_plugin_error_injection_suite");   /* testsuite is created */
//  MUT_ADD_TEST(terminator_drive_sata_plugin_error_injection_suite, drive_sata_plugin_error_injection1_test, NULL, NULL);


    MUT_RUN_TESTSUITE(terminator_drive_sata_plugin_fis_suite);
    // TODO: re-enable this when we have a error injection test suite
    //MUT_RUN_TESTSUITE(terminator_drive_sata_plugin_error_injection_suite);
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

fbe_status_t terminator_get_sata_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_terminator_sata_drive_info_t *drive_info)
{
    if (device_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        drive_info->backend_number = 0;
        drive_info->encl_number = 0;
        drive_info->slot_number = 0;
        drive_info->sas_address = 0x1234;
        drive_info->drive_type = FBE_SATA_DRIVE_SIM_512;
        fbe_copy_memory(&drive_info->drive_serial_number, "12345678901234567890", FBE_SATA_SERIAL_NUMBER_SIZE); 
        return FBE_STATUS_OK;
    }
}
