#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "pp_utils.h"

#define PDO_DRIVE_MAX 12

fbe_status_t fbe_test_load_pdo_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

    fbe_api_terminator_device_handle_t	port0_handle;
    fbe_api_terminator_device_handle_t	encl0_0_handle;
    fbe_api_terminator_device_handle_t	drive_handle;

    fbe_u32_t                       slot = 0;

    /*insert a board*/
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = 0x5000097A78009071;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < PDO_DRIVE_MAX; slot ++) {
	/*insert a sas_drive to port 0 encl 0 slot */
	sas_drive.backend_number = 0;
	sas_drive.encl_number = 0;
	sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
	sas_drive.block_size = 520;
	sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number) << 16) + slot;
	sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
	status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, slot, &sas_drive, &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}
