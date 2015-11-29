#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

#define MAUI_DRIVE_MAX 12

fbe_status_t fbe_test_load_maui_config(SPID_HW_TYPE platform_type, fbe_test_params_t *test)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;

    fbe_api_terminator_device_handle_t	port0_handle;
    fbe_api_terminator_device_handle_t	encl0_0_handle;
    fbe_api_terminator_device_handle_t	drive_handle;
    fbe_u32_t                           num_handles;
    fbe_u32_t                       slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
    board_info.board_type = test->board_type;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = test->backend_number;
    sas_port.io_port_number = test->io_port_number;
    sas_port.portal_number = test->portal_number;
    sas_port.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(test->backend_number);
    sas_port.port_type =  test->port_type;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = test->backend_number;
    sas_encl.encl_number = test->encl_number;
    sas_encl.uid[0] = 1; ((test->backend_number * 8) + test->encl_number); // some unique ID.
    sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(test->backend_number, test->encl_number);
    sas_encl.encl_type = test->encl_type;
    status  = fbe_test_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle, &num_handles);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < MAUI_DRIVE_MAX; slot ++) {
	/*insert a sas_drive to port 0 encl 0 slot */
    status  = fbe_test_pp_util_insert_sas_drive(test->backend_number,
                                                test->encl_number,
                                                slot,
                                                520,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}
