#include "mut.h"
#include "fbe/fbe_types.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "physical_package_tests.h"

#define YANCEY_DRIVE_MAX 12

fbe_status_t fbe_test_load_yancey_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t 	board_info;

    fbe_terminator_sas_port_info_t	sas_port;
    fbe_api_terminator_device_handle_t	port0_handle;    

//    fbe_terminator_sas_port_info_t	sas_port_unc;
//    fbe_api_terminator_device_handle_t	port_unc_handle;

    fbe_terminator_iscsi_port_info_t   iscsi_port;
    fbe_api_terminator_device_handle_t	port_iscsi_fe_handle = 0xFFFF;

    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    
    fbe_api_terminator_device_handle_t	encl0_0_handle;
    fbe_api_terminator_device_handle_t	drive_handle;
    fbe_u32_t                       slot = 0;

    fbe_terminator_fc_port_info_t   fc_port_fe;
    fbe_api_terminator_device_handle_t	port_fc_fe_handle = 0xFFFF;

    fbe_cpd_shim_hardware_info_t                hardware_info;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.port_role = FBE_CPD_SHIM_PORT_ROLE_BE;
    sas_port.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 0x34;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert a FC FE port*/
    fc_port_fe.backend_number = 1;
    fc_port_fe.io_port_number = 5;
    fc_port_fe.portal_number = 0;
    fc_port_fe.diplex_address = 0x5;
    fc_port_fe.port_type = FBE_PORT_TYPE_FC_PMC;
    fc_port_fe.port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
    fc_port_fe.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
    status  = fbe_api_terminator_insert_fc_port(&fc_port_fe, &port_fc_fe_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a iScsi FE port*/
    iscsi_port.backend_number = 2;
    iscsi_port.io_port_number = 6;
    iscsi_port.portal_number = 0;    
    iscsi_port.port_type = FBE_PORT_TYPE_ISCSI;
    iscsi_port.port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
    iscsi_port.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;
    status  = fbe_api_terminator_insert_iscsi_port(&iscsi_port, &port_iscsi_fe_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

#if 0
    /*insert a uncommitted SAS port*/
    sas_port_unc.backend_number = 0xFFFF;
    sas_port_unc.io_port_number = 4;
    sas_port_unc.portal_number = 0;
    sas_port_unc.sas_address = 0x5000097A7811903F;
    sas_port_unc.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port_unc.port_role = FBE_CPD_SHIM_PORT_ROLE_UNC;
    sas_port_unc.port_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;

    status  = fbe_api_terminator_insert_sas_port (&sas_port_unc, &port_unc_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a FC BE port*/
    fc_port_be.backend_number = 2;
    fc_port_be.io_port_number = 6;
    fc_port_be.portal_number = 0;
    fc_port_be.diplex_address = 0x6;
    fc_port_be.port_type = FBE_PORT_TYPE_FC_PMC;
    status  = fbe_terminator_api_insert_fc_port(&fc_port_be, &port_fc_be_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif
    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = 0x5000097A78009071;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (slot = 0; slot < YANCEY_DRIVE_MAX; slot ++) {
	    /*insert a sas_drive to port 0 encl 0 slot */
	    sas_drive.backend_number = 0;
	    sas_drive.encl_number = 0;
	    sas_drive.capacity = 0x10B5C730;
	    sas_drive.block_size = 520;
	    sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number) << 16) + slot;
	    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
	    status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, slot, &sas_drive, &drive_handle);
	    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
