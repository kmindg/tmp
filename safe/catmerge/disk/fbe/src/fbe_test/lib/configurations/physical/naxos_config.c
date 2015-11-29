#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "physical_package_tests.h"
#include "pp_utils.h"

fbe_status_t fbe_test_load_naxos_config(fbe_test_params_t *test)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;

    fbe_api_terminator_device_handle_t	port0_handle;
    fbe_api_terminator_device_handle_t	encl_handle;
    fbe_api_terminator_device_handle_t	drive_handle;

    fbe_u32_t                       no_of_encls = 0;
    fbe_u32_t                       slot = 0;
    fbe_u32_t                           num_handles;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = test->backend_number;
    sas_port.io_port_number = test->io_port_number;
    sas_port.portal_number = test->portal_number;
    sas_port.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(test->backend_number);
    sas_port.port_type = test->port_type;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for ( no_of_encls = 0; no_of_encls < test->max_enclosures; no_of_encls++ )
    {
        /*insert an enclosure to port 0*/
        sas_encl.backend_number = test->backend_number;
        sas_encl.encl_number = no_of_encls;
        sas_encl.uid[0] = no_of_encls; // some unique ID.
        sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(test->backend_number, no_of_encls);
        sas_encl.encl_type = test->encl_type;
        status  = fbe_test_insert_sas_enclosure (port0_handle, &sas_encl, &encl_handle, &num_handles);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ( no_of_encls != 2 )
        {
           for (slot = 0; slot < test->max_drives; slot ++) 
           {    /*insert a sas_drive to port 0 encl 0 slot */
               status  = fbe_test_pp_util_insert_sas_drive(test->backend_number,
                                                           no_of_encls,
                                                           slot,
                                                           520,
                                                           TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                           &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
           }
        }
        else
        {
            for (slot = 2; slot < (test->max_drives-1); slot ++) 
            {
                 status  = fbe_test_pp_util_insert_sas_drive(test->backend_number,
                                                             no_of_encls,
                                                             slot,
                                                             520,
                                                             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                             &drive_handle);
                 MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
    }
    return status;
}

/*!***************************************************************
 * fbe_test_load_naxos_config_table_driven()
 ****************************************************************
 * @brief
 * This function loads the actual configuration requested by
 * the test.
 *
 * @param test - parameters for configuration on which test is to
 *               executed.
 * @return FBE_STATUS_OK if no errors.
 * 
 * @author
 *  03/03/2010 - Created. Sujay Ranjan
 ****************************************************************/
fbe_status_t fbe_test_load_naxos_config_table_driven(fbe_test_params_t *test)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    fbe_u32_t                       encl_idx = 0;
    fbe_u32_t                       slot = 0;
    fbe_u32_t                       num_handles = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = test->backend_number;
    sas_port.io_port_number = test->io_port_number;
    sas_port.portal_number = test->portal_number;
    sas_port.sas_address = FBE_TEST_BUILD_PORT_SAS_ADDRESS(test->backend_number);
    sas_port.port_type = test->port_type;
    status = fbe_api_terminator_insert_sas_port(&sas_port,&port0_handle);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for ( encl_idx = 0; encl_idx < test->max_enclosures; encl_idx++ )
    {
        sas_encl.backend_number = test->backend_number;
        sas_encl.encl_number = encl_idx;
        sas_encl.uid[0] = encl_idx; // some unique ID.
        sas_encl.sas_address = FBE_TEST_BUILD_ENCL_SAS_ADDRESS(test->backend_number, encl_idx);
        sas_encl.encl_type = test->encl_type;
        sas_encl.connector_id = 0;
        status = fbe_test_insert_sas_enclosure(port0_handle, &sas_encl, &encl_handle, &num_handles);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (test->max_drives > 0)
        {
            for (slot = 0; slot < test->max_drives; slot++) 
            {
                if (test->drive_mask & BIT(slot))
                {
                    /*insert a sas_drive to port 0 encl 0 slot */
                    sas_drive.backend_number = test->backend_number;
                    sas_drive.encl_number = encl_idx;
                    sas_drive.sas_address = FBE_TEST_BUILD_DISK_SAS_ADDRESS(test->backend_number, encl_idx, slot);
                    sas_drive.drive_type = test->drive_type;
                    sas_drive.capacity = 0x10B5C730;
                    sas_drive.block_size = 520;
                    status  = fbe_test_insert_sas_drive(encl_handle, slot, &sas_drive, &drive_handle);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
    }
    return status;
}

