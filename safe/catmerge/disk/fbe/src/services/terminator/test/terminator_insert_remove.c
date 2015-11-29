#include "terminator_test.h"

#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"
#include "terminator_enclosure.h"

/**********************************/
/*        local variables         */
/**********************************/
static fbe_u32_t fbe_cpd_shim_callback_count;
static fbe_status_t fbe_cpd_shim_callback_function_mock(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
	mut_printf(MUT_LOG_HIGH, "Callback info: Callback_type %d", callback_info->callback_type);
    switch (callback_info->callback_type)
    {
    case FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN:
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN: device_type: %d", callback_info->info.login.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN: device_id: 0x%llX", (unsigned long long)callback_info->info.login.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN: device_addr: 0x%llX", (unsigned long long)callback_info->info.login.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.login.parent_device_id);
        break;
    case FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT:
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT: device_type: %d", callback_info->info.logout.device_type);
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT: device_id: 0x%llX", (unsigned long long)callback_info->info.logout.device_id);
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT: device_addr: 0x%llX", (unsigned long long)callback_info->info.logout.device_address);
        mut_printf(MUT_LOG_HIGH, "FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT: parent_dev_id: 0x%llX", (unsigned long long)callback_info->info.logout.parent_device_id);
        break;
    default:
        break;
    }
    fbe_cpd_shim_callback_count++;
    return FBE_STATUS_OK;
}


void terminator_insert_remove(void)
{
    fbe_status_t 					status;
	fbe_terminator_board_info_t 	board_info, temp_board_info;
	fbe_terminator_sas_port_info_t	sas_port;
	fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

	fbe_terminator_api_device_handle_t	port1_handle, port2_handle;
	fbe_terminator_api_device_handle_t	encl1_0_handle, encl1_1_handle, encl1_2_handle;
    fbe_terminator_api_device_handle_t	encl1_1_handle_new, encl1_2_handle_new, encl2_0_handle_new;
    fbe_terminator_api_device_handle_t	drive_handle;
    fbe_u32_t                       port_index;


    fbe_terminator_api_device_handle_t 		device_list[MAX_DEVICES];
    terminator_cpd_port_t           cpd_port;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;

    cpd_port.state = CPD_PORT_STATE_INITIALIZED;
	cpd_port.payload_completion_function = NULL;
	cpd_port.payload_completion_context = NULL;
	cpd_port.callback_function = fbe_cpd_shim_callback_function_mock;
	cpd_port.callback_context = NULL;
    fbe_cpd_shim_callback_count = 0;

    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);

	/*initialize the terminator*/
	status = fbe_terminator_api_init();
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

	/* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
	board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
	status  = fbe_terminator_api_insert_board (&board_info);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*verify board type*/
    status = fbe_terminator_api_get_board_info(&temp_board_info);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_EQUAL(board_info.board_type, temp_board_info.board_type);

	/*insert a port*/
	sas_port.sas_address = (fbe_u64_t)0x123456789;
	sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 1;
	status  = fbe_terminator_api_insert_sas_port (&sas_port, &port1_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t) fbe_cpd_shim_callback_function_mock, NULL);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	
	/*insert a port*/
	sas_port.sas_address = (fbe_u64_t)0x22222222;
	sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 2;
    sas_port.portal_number = 3;
    sas_port.backend_number = 2;
	status  = fbe_terminator_api_insert_sas_port (&sas_port, &port2_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(2, 3, &port_index);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Register a callback to port 1, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t) fbe_cpd_shim_callback_function_mock, NULL);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* enumerate the ports */
	status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert an enclosure to port 0*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
	sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_0_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert an enclosure to port 0*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 2; // some unique ID.
	sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert an enclosure to port 0*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 2;
    sas_encl.uid[0] = 3; // some unique ID.
	sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_2_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enumerate the devices on ports */
	status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator */
   	MUT_ASSERT_INT_EQUAL(6, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);
    /* check the callback count, 2 port linkup and 3 enclosure and 3 virtual phy login */
    MUT_ASSERT_INT_EQUAL (8, fbe_cpd_shim_callback_count);

    fbe_cpd_shim_callback_count = 0;
    /* remove sas enclosure 0-1 */
    status = fbe_terminator_api_remove_device(encl1_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* enumerate the devices on ports */
	status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator, 1 board, 2 ports and 2 enclosure */
   	MUT_ASSERT_INT_EQUAL(4, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);
    /* check the callback count, 2 enclosure and 2 virtual phy logoit */
    MUT_ASSERT_INT_EQUAL (4, fbe_cpd_shim_callback_count);

    /*verify enclosure 0-1 and 0-2 are not in the device registry anymore*/
    status = fbe_terminator_api_get_enclosure_handle(1, 1, &encl1_1_handle_new);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);

    status = fbe_terminator_api_get_enclosure_handle(1, 2, &encl1_2_handle_new);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);

    /*insert an enclosure*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456781;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle_new);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_1_handle, encl1_1_handle_new);

    /*insert an enclosure*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 2;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_2_handle_new);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_NOT_EQUAL(encl1_2_handle, encl1_2_handle_new);

    /*insert an enclosure*/
    sas_encl.backend_number = 2;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 3; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456783;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.connector_id = 0;
	status  = fbe_terminator_api_insert_sas_enclosure (port2_handle, &sas_encl, &encl2_0_handle_new);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure */
   	MUT_ASSERT_INT_EQUAL(7, fbe_terminator_api_get_devices_count());

    /*insert a sas_drive to port 0 encl 2 */
    sas_drive.backend_number = 1;
    sas_drive.encl_number = 2;
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl1_2_handle_new, 3, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 1 drive*/
   	MUT_ASSERT_INT_EQUAL(8, fbe_terminator_api_get_devices_count());

    /*insert a sas_drive to the last enclosure*/
    sas_drive.backend_number = 2;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654322;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl2_0_handle_new, 0, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator, 1 board, 2 ports and 4 enclosure 2 drive*/
   	MUT_ASSERT_INT_EQUAL(9, fbe_terminator_api_get_devices_count());

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}

//void terminator_insert_remove_sas_port(void)
//{
//    fbe_status_t                    status;
//    fbe_terminator_board_info_t     board_info;
//    fbe_terminator_sas_port_info_t  sas_port;
//
//    fbe_terminator_api_device_handle_t      device_list[MAX_DEVICES];
//    fbe_u32_t                       total_devices = 0;
//    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
//    fbe_u32_t                       i = 0;
//    fbe_terminator_device_ptr_t handle;
//    fbe_u32_t       port_index;
//
//    mut_printf(MUT_LOG_HIGH, "%s %s ",__FILE__, __FUNCTION__);
//
//    /*initialize the terminator*/
//    status = fbe_terminator_api_init();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);
//
//    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
//    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
//    status  = fbe_terminator_api_insert_board (&board_info);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /*insert a port*/
//    sas_port.sas_address = (fbe_u64_t)0x111111111;
//    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
//    sas_port.io_port_number = 0;
//    sas_port.portal_number = 1;
//    sas_port.backend_number = 0;
//
//    status  = fbe_terminator_api_insert_sas_port (0, &sas_port);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    status = fbe_terminator_miniport_api_port_init(0, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /* Register a callback to port 0, so we can destroy the object properly */
//    status = fbe_terminator_miniport_api_register_callback(port_index, fbe_cpd_shim_callback_function_mock, NULL);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /*insert a port*/
//    sas_port.sas_address = (fbe_u64_t)0x22222222;
//    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
//    sas_port.io_port_number = 1;
//    sas_port.portal_number = 1;
//    sas_port.backend_number = 1;
//
//    status  = fbe_terminator_api_insert_sas_port (1, &sas_port);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /* Register a callback to port 1, so we can destroy the object properly */
//    status = fbe_terminator_miniport_api_port_init(1, 1, &port_index);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /* Register a callback to port 0, so we can destroy the object properly */
//    status = fbe_terminator_miniport_api_register_callback(port_index, fbe_cpd_shim_callback_function_mock, NULL);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /* enumerate the ports */
//    status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//
//    /* Peter Puhov: I think it is wrong test
//    for (i=0; i<cpd_shim_enumerate_backend_ports.number_of_io_ports; i++)
//    {
//        MUT_ASSERT_TRUE(i==cpd_shim_enumerate_backend_ports.io_port_array[i])
//    }
//    */
//
//    /* enumerate the devices on ports */
//    terminator_get_port_ptr_by_port_portal(0, 1, &handle);
//    status = terminator_enumerate_devices (handle, device_list, &total_devices);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    MUT_ASSERT_INTEGER_EQUAL(0, total_devices);
//
//    status = fbe_terminator_api_destroy();
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
//}
//
