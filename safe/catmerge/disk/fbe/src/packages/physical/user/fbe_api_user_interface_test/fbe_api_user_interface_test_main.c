#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_emcutil_shell_include.h"

void fbe_api_test_setup (void);
void fbe_api_test_teardown(void);

#define FBE_API_TEST_ENCL_UID_BASE          0x70

int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *fbe_api_query_command_suite;                /* pointer to testsuite structure */
    mut_testsuite_t *fbe_api_device_handle_getter_suite;                /* pointer to testsuite structure */
    mut_testsuite_t *fbe_api_control_commands_suite;
    mut_testsuite_t *fbe_api_utility_suite;
    mut_testsuite_t *fbe_api_enclosure_command_suite;
    mut_testsuite_t *fbe_api_physical_drive_command_suite;
    mut_testsuite_t *fbe_api_logical_drive_command_suite;
	mut_testsuite_t *fbe_api_object_map_test_suite;
	mut_testsuite_t *fbe_api_drive_cfg_table_test_suite;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

    fbe_api_device_handle_getter_suite = MUT_CREATE_TESTSUITE("fbe_api_device_handle_getter_suite")   /* discovery_suite is created */
    MUT_ADD_TEST(fbe_api_device_handle_getter_suite, fbe_api_get_board_object_handle_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_device_handle_getter_suite, fbe_api_get_port_object_handle_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_device_handle_getter_suite, fbe_api_object_map_interface_get_enclosure_obj_id_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_device_handle_getter_suite, fbe_api_get_physical_drive_object_handle_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_device_handle_getter_suite, fbe_api_get_logical_drive_object_handle_test, fbe_api_test_setup, fbe_api_test_teardown)

    fbe_api_query_command_suite = MUT_CREATE_TESTSUITE("fbe_api_query_command_suite")   /* testsuite is created */
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_type_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_class_id_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_port_number_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_lifecycle_state_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_topology_enumerate_objects_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_all_objects_in_state_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_physical_state_object_from_topology_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_logical_drive_state_from_topology_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_enclosure_state_from_topology_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_port_state_from_topology_test, fbe_api_test_setup, fbe_api_test_teardown)
	MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_stp_transport_edge_info_test, fbe_api_test_setup, fbe_api_test_teardown)
	MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_smp_transport_edge_info_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_discovery_edge_info_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_ssp_transport_edge_info_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_query_command_suite, fbe_api_get_object_block_edge_info_test, fbe_api_test_setup, fbe_api_test_teardown)

    fbe_api_control_commands_suite = MUT_CREATE_TESTSUITE("fbe_api_control_commands_suite")   /* testsuite is created */
    MUT_ADD_TEST(fbe_api_control_commands_suite, fbe_api_drive_reset_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_control_commands_suite, fbe_api_physical_drive_power_cycle_test, fbe_api_test_setup, fbe_api_test_teardown)

    fbe_api_utility_suite = MUT_CREATE_TESTSUITE("fbe_api_utility_suite")   /* testsuite is created */
    MUT_ADD_TEST(fbe_api_utility_suite, fbe_api_wait_for_object_lifecycle_state_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_utility_suite, fbe_api_wait_for_number_of_objects_test, fbe_api_test_setup, fbe_api_test_teardown)

    fbe_api_enclosure_command_suite = MUT_CREATE_TESTSUITE("fbe_api_enclosure_command_suite")
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_bypass_drive_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_unbypass_drive_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_firmware_download_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_firmware_download_status_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_get_info_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_get_setup_info_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_set_control_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_set_leds_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_enclosure_command_suite, fbe_api_enclosure_get_slot_to_phy_mapping_test, fbe_api_test_setup, fbe_api_test_teardown)
    
    fbe_api_physical_drive_command_suite = MUT_CREATE_TESTSUITE("fbe_api_physical_drive_command_suite")
    MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_get_error_counts_test, fbe_api_test_setup, NULL)
	/*start adding more tests over here. They would rely on the fbe_api_test_setup done before so the test would run faster*/
    MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_clear_error_counts_test, NULL, NULL)
    MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_is_wr_cache_enabled_test, NULL, NULL)
    MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_get_drive_information_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_set_get_qdepth_test, NULL, NULL)
	/*this has to be the last one, it does the destroy*/
	MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_set_get_timeout_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_physical_drive_command_suite, fbe_api_physical_drive_set_sata_write_uncorrectable_test, NULL, fbe_api_test_teardown)

    fbe_api_logical_drive_command_suite = MUT_CREATE_TESTSUITE("fbe_api_logical_drive_command_suite")
    MUT_ADD_TEST(fbe_api_logical_drive_command_suite, fbe_api_ldo_test_send_set_attributes_test, fbe_api_test_setup, fbe_api_test_teardown)
    MUT_ADD_TEST(fbe_api_logical_drive_command_suite, fbe_api_ldo_test_send_get_attributes_test, fbe_api_test_setup, fbe_api_test_teardown)

	fbe_api_object_map_test_suite = MUT_CREATE_TESTSUITE("fbe_api_object_map_test_suite")
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_init_test, fbe_api_test_setup, fbe_api_test_teardown)
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_destroy_test, fbe_api_test_setup, NULL)
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_object_exists_test, fbe_api_test_setup, NULL)
	/*start adding more tests over here. They would rely on the fbe_api_test_setup done before so the test would run faster*/
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_get_board_obj_id_test, NULL, NULL)
    MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_get_total_logical_drives_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_get_object_handle_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_get_logical_drive_object_handle_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_get_obj_info_by_handle_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_register_notification_test, NULL, NULL)
	/*this has to be the last one, it does the destroy*/
	MUT_ADD_TEST(fbe_api_object_map_test_suite, fbe_api_object_map_interface_unregister_notification_test, NULL, fbe_api_test_teardown)

	fbe_api_drive_cfg_table_test_suite = MUT_CREATE_TESTSUITE("fbe_api_drive_configuration_service_suite")
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_get_handles_test, fbe_api_test_setup, NULL)
	/*start adding more tests over here. They would rely on the fbe_api_test_setup done before so the test would run faster*/
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_get_table_entry_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_change_thresholds_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_add_port_table_entry_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_change_port_thresholds_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_get_port_handles_test, NULL, NULL)
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_get_port_table_entry_test, NULL, NULL)
    /*this has to be the last one, it does the destroy*/
	MUT_ADD_TEST(fbe_api_drive_cfg_table_test_suite, fbe_api_drive_configuration_interface_add_table_entry_test, NULL, fbe_api_test_teardown)
    

    MUT_RUN_TESTSUITE(fbe_api_device_handle_getter_suite)               /* testsuite is executed */
    MUT_RUN_TESTSUITE(fbe_api_query_command_suite)               /* testsuite is executed */
    MUT_RUN_TESTSUITE(fbe_api_control_commands_suite)               /* testsuite is executed */
    MUT_RUN_TESTSUITE(fbe_api_utility_suite)               /* testsuite is executed */
    //MUT_RUN_TESTSUITE(fbe_api_enclosure_command_suite)               /* testsuite is executed */
    MUT_RUN_TESTSUITE(fbe_api_physical_drive_command_suite)               /* testsuite is executed */
    MUT_RUN_TESTSUITE(fbe_api_logical_drive_command_suite)
	MUT_RUN_TESTSUITE(fbe_api_object_map_test_suite)
	MUT_RUN_TESTSUITE(fbe_api_drive_cfg_table_test_suite)
}

/* Hardcoded configuration */
void fbe_api_test_setup (void)
{
    fbe_status_t status;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t port_handle;
    fbe_terminator_api_device_handle_t        encl_id[8];
    fbe_terminator_api_device_handle_t        drive_id[120];
    fbe_u32_t                       i;
	fbe_terminator_sata_drive_info_t 	sata_drive;

    /*before loading the physical package we initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);
    
	/* Enable the IO mode.  This will cause the terminator to create
     * disk files as drive objects are created.
     */ 
    status = fbe_terminator_api_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DREADNOUGHT_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;

    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert three enclosures */
    for (i = 0; i < 3; i++) {
        sas_encl.backend_number = sas_port.backend_number;
        sas_encl.encl_number = i;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
        sas_encl.sas_address = (fbe_u64_t)0x50000970000000FF + 0x0000000100000000 * i;
        sas_encl.connector_id = 0;
        sprintf(sas_encl.uid,"%02X%02X", FBE_API_TEST_ENCL_UID_BASE, i);
        status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id[i]);/*add encl on port 0*/
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /* insert two drives into first enclosure */
    for(i = 0; i < 2; i++){
        /*insert a drive to the enclosure in slot 0*/
        sas_drive.backend_number = sas_port.backend_number;
        sas_drive.encl_number = 0;
		sas_drive.slot_number = i;
		sas_drive.capacity = 0x2000000;
		sas_drive.block_size = 520;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
        memset(&sas_drive.drive_serial_number, i , sizeof(sas_drive.drive_serial_number));
        status = fbe_terminator_api_insert_sas_drive (encl_id[0], i, &sas_drive, &drive_id[i]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /* insert two drives into second enclosure */
    for(i = 1 * 15; i < 1 * 15 + 2; i++){
        /*insert a drive to the enclosure in slot 0*/
        sas_drive.backend_number = sas_port.backend_number;
        sas_drive.encl_number = 1;
		sas_drive.slot_number = i - 1 * 15;
		sas_drive.capacity = 0x2000000;
		sas_drive.block_size = 520;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
        memset(&sas_drive.drive_serial_number, i , sizeof(sas_drive.drive_serial_number));
        status = fbe_terminator_api_insert_sas_drive (encl_id[1], i - 1 * 15, &sas_drive, &drive_id[i]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /*insert a SATA drive to the third enclosure in slot 0*/
    sata_drive.backend_number = sas_port.backend_number;
    sata_drive.encl_number = 2;
    sata_drive.drive_type = FBE_SATA_DRIVE_SIM_512;
    sata_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
	sata_drive.capacity = 0x2000000;
	sata_drive.block_size = 512;
    memset(&sata_drive.drive_serial_number, '1' , sizeof(sas_drive.drive_serial_number));
    status = fbe_terminator_api_insert_sata_drive ( encl_id[2], 0, &sata_drive, &drive_id[30]);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Insert SATA drive to port 0, enclosure 2, slot 0 ===\n");

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical pacakge ===\n");
    status = physical_package_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    EmcutilSleep(5000); /* 5 sec should be more than enough */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting fbe_api ===\n");
    status = fbe_api_common_init_sim();/*this would cause the map to be populated*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}

void fbe_api_test_teardown(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== destroying all configurations ===\n");

    status = fbe_api_common_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status =  physical_package_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    //MUT_ASSERT_INT_EQUAL_MSG(0, fbe_trace_get_critical_error_counter(), "=== Physical Package had critical error logged!!! ===");
    //MUT_ASSERT_INT_EQUAL_MSG(0, fbe_trace_get_error_counter(), "=== Physical Package had error logged!!! ===");

    return;
}
