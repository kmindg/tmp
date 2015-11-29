#include "edge_tap_main.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_emcutil_shell_include.h"

static void edge_tap_test_setup(void);
static void edge_tap_test_teardown(void);

int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *edge_tap_suite;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

    edge_tap_suite = MUT_CREATE_TESTSUITE("edge_tap_suite")   /* testsuite is created */
    /* add tests to suite */
    MUT_ADD_TEST(edge_tap_suite, edge_tap_set_hook_on_physical_drive_test, edge_tap_test_setup, edge_tap_test_teardown)
    MUT_ADD_TEST(edge_tap_suite, edge_tap_set_hook_on_sas_enclosure_test, edge_tap_test_setup, edge_tap_test_teardown)
    MUT_RUN_TESTSUITE(edge_tap_suite)               /* testsuite is executed */
}

static void edge_tap_test_setup(void)
{
    fbe_status_t status;
    fbe_terminator_board_info_t 	board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_terminator_api_device_handle_t            encl_id1, drive_id1;
    fbe_terminator_api_device_handle_t port_handle;
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
    sas_port.sas_address = (fbe_u64_t)0x22222222;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
	
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure*/
    sas_encl.backend_number = sas_port.backend_number;
    sas_encl.encl_number = 0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id1);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a drive to the enclosure in slot 0*/
    sas_drive.backend_number = sas_port.backend_number;
    sas_drive.encl_number = 0;
	sas_drive.slot_number = 0;
	sas_drive.capacity = 0x2000000;
	sas_drive.block_size = 520;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456780;
    memset(&sas_drive.drive_serial_number, '1' , sizeof(sas_drive.drive_serial_number));
    status = fbe_terminator_api_insert_sas_drive (encl_id1, 0, &sas_drive, &drive_id1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical pacakge===\n");

    status = physical_package_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: physical package initialized.", __FUNCTION__);

    EmcutilSleep(2000); /* Sleep 2 sec. to give a chance to come up */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting fbe_api ===\n");
    status = fbe_api_common_init();/*this would cause the map to be populated*/
    
    EmcutilSleep(2000); /* Sleep 2 sec. to give a chance to monitor */
}

static void edge_tap_test_teardown(void)
{
    fbe_status_t status;

    status = fbe_api_common_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status =  physical_package_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: physical package destroyed.", __FUNCTION__);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator destroyed.", __FUNCTION__);
}
