#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_port_interface.h"
#include "pp_utils.h"


fbe_status_t fbe_test_load_lapa_rios_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;

    fbe_api_terminator_device_handle_t	port_handle;
    fbe_api_terminator_device_handle_t	encl_handle;
    fbe_api_terminator_device_handle_t	drive_handle;

    fbe_u32_t                       no_of_ports = 0;
    fbe_u32_t                       no_of_encls = 0;
    fbe_u32_t                       slot = 0;

/* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (no_of_ports = 0; no_of_ports < LAPA_RIO_MAX_PORTS; no_of_ports ++)
    {
        /*insert a port*/
        sas_port.backend_number = no_of_ports;
        sas_port.io_port_number = 3 + no_of_ports;
        sas_port.portal_number = 5 + no_of_ports;
        sas_port.sas_address = 0x5000097A7800903F + no_of_ports;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        for ( no_of_encls = 0; no_of_encls < LAPA_RIO_MAX_ENCLS; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
            sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
           for (slot = 0; slot < LAPA_RIO_MAX_DRIVES; slot ++)
           {   
               if (((no_of_ports < 2) && (no_of_encls < 2) && (slot < 15))||
                   ((no_of_ports < 2) && (no_of_encls == 2) && (slot == 4))||
                   ((no_of_ports < 2) && (no_of_encls == 2) && (slot == 6))||
                   ((no_of_ports < 2) && (no_of_encls == 2) && (slot == 10)))
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
    }
    return status;
}
