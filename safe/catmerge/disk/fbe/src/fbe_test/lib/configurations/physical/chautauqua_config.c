 /***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file chautauqua_physical_config.c
 ***************************************************************************
 *
 * @brief
 *  This file contains setup for the chautauqua tests.
 *
 * @version
 *   11/19/2009 - Created. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_port_interface.h"
#include "pp_utils.h"

fbe_status_t fbe_test_load_chautauqua_config(fbe_test_params_t *test, SPID_HW_TYPE platform_type)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t sas_port;
    fbe_terminator_sas_encl_info_t sas_encl;
    fbe_cpd_shim_hardware_info_t                hardware_info;

    fbe_api_terminator_device_handle_t port_handle;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_api_terminator_device_handle_t drive_handle;

    fbe_u32_t no_of_ports = 0;
    fbe_u32_t no_of_encls = 0;
    fbe_u32_t slot = 0;
    fbe_u32_t num_handles = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (no_of_ports = 0; no_of_ports < CHAUTAUQUA_MAX_PORTS; no_of_ports ++)
    {
        /*insert a port*/
        sas_port.backend_number = no_of_ports;
        sas_port.io_port_number =  test->io_port_number + no_of_ports;
        sas_port.portal_number = test->portal_number + no_of_ports;
        sas_port.sas_address = FBE_BUILD_PORT_SAS_ADDRESS(no_of_ports);
        sas_port.port_type =  test->port_type;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* configure port hardware info */
        fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
        hardware_info.vendor = 0x11F8;
        hardware_info.device = 0x8001;
        hardware_info.bus = 0x34;
        hardware_info.slot = 0;
        hardware_info.function = no_of_ports;

        status = fbe_api_terminator_set_hardware_info(port_handle,&hardware_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        for ( no_of_encls = 0; no_of_encls < test->max_enclosures; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(no_of_ports, no_of_encls);
            sas_encl.encl_type =  test->encl_type;
            sas_encl.connector_id = 0;
            status  = fbe_test_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle, &num_handles);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            for (slot = 0; slot < test->max_drives; slot ++)
            {
                    status  = fbe_test_pp_util_insert_sas_drive(no_of_ports,
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
