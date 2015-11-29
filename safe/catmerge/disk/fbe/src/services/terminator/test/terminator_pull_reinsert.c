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


void terminator_pull_reinsert_drive(void)
{
    fbe_status_t 					status;
	fbe_terminator_board_info_t 	board_info, temp_board_info;
	fbe_terminator_sas_port_info_t	sas_port;
	fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

	fbe_terminator_api_device_handle_t	port1_handle;
	fbe_terminator_api_device_handle_t	encl1_0_handle, encl1_1_handle;
//    fbe_terminator_api_device_handle_t	encl1_1_handle_new, encl1_2_handle_new, encl2_0_handle_new;
    fbe_terminator_api_device_handle_t	drive_handle, pulled_drive_handle;
    fbe_u32_t                       port_index;


    fbe_terminator_api_device_handle_t 		device_list[MAX_DEVICES];
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;

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

	/* enumerate the ports */
	status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert an enclosure to port 0*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
	sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_0_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert an enclosure to port 0*/
    sas_encl.backend_number = 1;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 2; // some unique ID.
	sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
	status  = fbe_terminator_api_insert_sas_enclosure (port1_handle, &sas_encl, &encl1_1_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert a sas_drive to port 0 encl 0 */
	sas_drive.backend_number = 1;
	sas_drive.encl_number = 0;
	sas_drive.sas_address = (fbe_u64_t)0x987654321;
	sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x10BD0;
        sas_drive.block_size = 520;
	status  = fbe_terminator_api_insert_sas_drive (encl1_0_handle, 0, &sas_drive, &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* keep the drive to be pulled in next section */
	pulled_drive_handle = drive_handle;

	/*insert a sas_drive to port 0 encl 0 */
	sas_drive.backend_number = 1;
	sas_drive.encl_number = 0;
	sas_drive.sas_address = (fbe_u64_t)0x888888888;
	sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x10BD0;
        sas_drive.block_size = 520;
	status  = fbe_terminator_api_insert_sas_drive (encl1_0_handle, 1, &sas_drive, &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert a sas_drive to port 0 encl 1 */
	sas_drive.backend_number = 1;
	sas_drive.encl_number = 1;
	sas_drive.sas_address = (fbe_u64_t)0x777777777;
	sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x10BD0;
        sas_drive.block_size = 520;
	status  = fbe_terminator_api_insert_sas_drive (encl1_1_handle, 0, &sas_drive, &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*insert a sas_drive to port 0 encl 1 */
	sas_drive.backend_number = 1;
	sas_drive.encl_number = 1;
	sas_drive.sas_address = (fbe_u64_t)0x666666666;
	sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.capacity = 0x10BD0;
        sas_drive.block_size = 520;
	status  = fbe_terminator_api_insert_sas_drive (encl1_1_handle, 1, &sas_drive, &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enumerate the devices on ports */
	status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator */
	/* 1 borad + 1 port + 2 enclosures + 4 drives */
   	MUT_ASSERT_INT_EQUAL(8, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);
    /* check the callback count, 1 port linkup and 2 enclosures 2 virtual phy and 4 drives login */
    MUT_ASSERT_INT_EQUAL (9, fbe_cpd_shim_callback_count);

    /* Verify that drive in port 0 enclosure 0 slot 0 is available */

    status = fbe_terminator_api_get_drive_handle(1, 0, 0, &drive_handle);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* pull a drive from port 0 enclosure 0 slot 0 */
	fbe_cpd_shim_callback_count = 0;
	status = fbe_terminator_api_pull_drive(pulled_drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    EmcutilSleep(1000);
    /* check the callback count, 1 drive logout */
    MUT_ASSERT_INT_EQUAL (1, fbe_cpd_shim_callback_count);
    /* Verify that drive in port 0 enclosure 0 slot 0 is unavailable */
    status = fbe_terminator_api_get_drive_handle(1, 0, 0, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);

    /* Verify that drive in port 0 enclosure 1 slot 2 is unavailable */
    status = fbe_terminator_api_get_drive_handle(1, 1, 2, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    /* reinsert a drive to port 0 enclosure 1 slot 2 */
    fbe_cpd_shim_callback_count = 0;
    status = fbe_terminator_api_reinsert_drive(encl1_1_handle, 2, pulled_drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    EmcutilSleep(1000);
    /* check the callback count, 1 drive login */
    MUT_ASSERT_INT_EQUAL (1, fbe_cpd_shim_callback_count);
    /* Verify that drive in port 0 enclosure 1 slot 2 is available */
    status = fbe_terminator_api_get_drive_handle(1, 1, 2, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}

void terminator_pull_reinsert_enclosure(void)
{
    fbe_status_t                    status;
    fbe_terminator_board_info_t     board_info, temp_board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle, encl0_1_handle, encl0_2_handle, new_encl_handle, rtn_encl_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    fbe_u32_t                       port_index;


    fbe_terminator_api_device_handle_t      device_list[MAX_DEVICES];
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;

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

    /*verify platform type*/
    status = fbe_terminator_api_get_board_info(&temp_board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(board_info.platform_type, temp_board_info.platform_type);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_miniport_api_port_init(1, 2, &port_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Register a callback to port 0, so we can destroy the object properly */
    status = fbe_terminator_miniport_api_register_callback(port_index, (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_callback_function_mock, NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enumerate the ports */
    status  = fbe_terminator_miniport_api_enumerate_cpd_ports (&cpd_shim_enumerate_backend_ports);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 1;
    sas_encl.uid[0] = 2; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 2;
    sas_encl.uid[0] = 3; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x123456784;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_2_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl0_0_handle, 0, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x888888888;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl0_0_handle, 1, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 1 */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 1;
    sas_drive.sas_address = (fbe_u64_t)0x777777777;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl0_1_handle, 0, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 1 */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 1;
    sas_drive.sas_address = (fbe_u64_t)0x666666666;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl0_1_handle, 1, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 2 */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 2;
    sas_drive.sas_address = (fbe_u64_t)0x77345237;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl0_2_handle, 0, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 2 */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 2;
    sas_drive.sas_address = (fbe_u64_t)0x657673456;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10BD0;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive (encl0_2_handle, 1, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enumerate the devices on ports */
    status = fbe_terminator_api_enumerate_devices(device_list, MAX_DEVICES);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* check the number of devices in terminator */
    /* 1 borad + 1 port + 3 enclosures + 6 drives */
    MUT_ASSERT_INT_EQUAL(11, fbe_terminator_api_get_devices_count());
    EmcutilSleep(1000);
    /* check the callback count, 1 port linkup and 3 enclosures 3 virtual phy and 6 drives login */
    MUT_ASSERT_INT_EQUAL (13, fbe_cpd_shim_callback_count);

    /* verify the chain depth of enclosure 0_1 and 0_2*/
    MUT_ASSERT_INT_EQUAL(1, sas_enclosure_handle_get_enclosure_chain_depth(encl0_1_handle));
    MUT_ASSERT_INT_EQUAL(2, sas_enclosure_handle_get_enclosure_chain_depth(encl0_2_handle));

    /* pull enclosure 0_1 from port 0 */
    fbe_cpd_shim_callback_count = 0;
    status = fbe_terminator_api_pull_sas_enclosure(encl0_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    EmcutilSleep(1000);
    /* check the callback count, 2 enclosures, 2 virtual phy and 4 drives logout */
    MUT_ASSERT_INT_EQUAL (8, fbe_cpd_shim_callback_count);
    /* Verify that enclosure 0-1 and 0-2 in port 0 is unavailable */
    status = fbe_terminator_api_get_enclosure_handle(0, 1, &rtn_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);
    status = fbe_terminator_api_get_enclosure_handle(0, 2, &rtn_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);

    /*insert a new enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 4;
    sas_encl.uid[0] = 3; // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x120006782;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BULLET;
    status  = fbe_terminator_api_insert_sas_enclosure (port0_handle, &sas_encl, &new_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    EmcutilSleep(500);

    /* reinsert enclosure 0_1 to port 0 */
    fbe_cpd_shim_callback_count = 0;
    status = fbe_terminator_api_reinsert_sas_enclosure(port0_handle, encl0_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    EmcutilSleep(1000);
    /* check the callback count, 2 enclosures, 2 virtual phy and 4 drives login */
    MUT_ASSERT_INT_EQUAL (8, fbe_cpd_shim_callback_count);
    /* Verify that enclosure 0-1 and 0-2 in port 0 is unavailable */
    status = fbe_terminator_api_get_enclosure_handle(0, 1, &rtn_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_terminator_api_get_enclosure_handle(0, 2, &rtn_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Verify that drive in port 0 enclosure 1 slot 1 is available */
    status = fbe_terminator_api_get_drive_handle(0, 1, 1, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Verify that drive in port 0 enclosure 2 slot 0 is available */
    status = fbe_terminator_api_get_drive_handle(0, 2, 0, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify the chain depth of enclosure 0_1 and 0_2, they should increase by 1 because a new enclosure has been inserted*/
    MUT_ASSERT_INT_EQUAL(2, sas_enclosure_handle_get_enclosure_chain_depth(encl0_1_handle));
    MUT_ASSERT_INT_EQUAL(3, sas_enclosure_handle_get_enclosure_chain_depth(encl0_2_handle));

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}
