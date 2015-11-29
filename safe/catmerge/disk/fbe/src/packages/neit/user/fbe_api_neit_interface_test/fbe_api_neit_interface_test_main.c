#include "fbe_api_neit_interface_test.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_emcutil_shell_include.h"

#define FBE_API_TEST_ENCL_UID_BASE          0x70

int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *fbe_api_protocol_error_injection_suite;                /* pointer to testsuite structure */
    mut_testsuite_t *fbe_api_logical_error_injection_suite;                /* pointer to testsuite structure */
#include "fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

    fbe_api_protocol_error_injection_suite = MUT_CREATE_TESTSUITE("fbe_api_protocol_error_injection_suite")   /* discovery_suite is created */
    MUT_ADD_TEST(fbe_api_protocol_error_injection_suite, fbe_api_protocol_error_injection_add_remove_record_test, fbe_api_neit_test_setup, fbe_api_neit_test_teardown)

    fbe_api_logical_error_injection_suite = MUT_CREATE_TESTSUITE("fbe_api_logical_error_injection_suite")
    fbe_api_logical_error_injection_add_tests(fbe_api_logical_error_injection_suite);
  
    MUT_RUN_TESTSUITE(fbe_api_protocol_error_injection_suite)               /* testsuite is executed */
    MUT_RUN_TESTSUITE(fbe_api_logical_error_injection_suite)               /* testsuite is executed */
}

/* Hardcoded configuration */
void fbe_api_neit_test_setup (void)
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

    /*before loading the physical package we initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: terminator initialized.", __FUNCTION__);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type =  SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_terminator_api_insert_board(&board_info);
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
        sprintf(sas_encl.uid,"%02X%02X", FBE_API_TEST_ENCL_UID_BASE, i);
        status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id[i]);/*add encl on port 0*/
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /* insert two drives into first enclosure */
    for(i = 0; i < 2; i++){
        /*insert a drive to the enclosure in slot 0*/
        sas_drive.backend_number = sas_port.backend_number;
        sas_drive.encl_number = 0;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
        sas_drive.block_size = 520;
        sas_drive.capacity = FBE_U32_MAX;
        memset(&sas_drive.drive_serial_number, i , sizeof(sas_drive.drive_serial_number));
        status = fbe_terminator_api_insert_sas_drive (encl_id[0], i, &sas_drive, &drive_id[i]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /* insert two drives into second enclosure */
    for(i = 1 * 15; i < 1 * 15 + 2; i++){
        /*insert a drive to the enclosure in slot 0*/
        sas_drive.backend_number = sas_port.backend_number;
        sas_drive.encl_number = 1;
        sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
        sas_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
        sas_drive.block_size = 520;
        sas_drive.capacity = FBE_U32_MAX;
        memset(&sas_drive.drive_serial_number, i , sizeof(sas_drive.drive_serial_number));
        status = fbe_terminator_api_insert_sas_drive (encl_id[1], i - 1 * 15, &sas_drive, &drive_id[i]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

//  /*insert a SATA drive to the third enclosure in slot 0*/
//  sata_drive.backend_number = sas_port.backend_number;
//  sata_drive.encl_number = 2;
//  sata_drive.drive_type = FBE_SATA_DRIVE_SIM_512;
//  sata_drive.sas_address = (fbe_u64_t)0x500FBE0000000000 + 0x0000000100000000 * i;
//  memset(&sata_drive.drive_serial_number, '1' , sizeof(sas_drive.drive_serial_number));
//  status = fbe_terminator_api_insert_sata_drive ( encl_id[2], 0, &sata_drive, &drive_id[30]);
//  MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//  mut_printf(MUT_LOG_TEST_STATUS, "=== Insert SATA drive to port 0, enclosure 2, slot 0 ===\n");

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package ===\n");
    fbe_test_load_physical_package();
    

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting neit package ===\n");
    fbe_test_load_neit_package(NULL);
   

    EmcutilSleep(5000); /* 5 sec should be more than enough */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting fbe_api ===\n");

    status = fbe_api_common_init_sim();/*this would cause the map to be populated*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, physical_package_io_entry, physical_package_control_entry);
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_NEIT, NULL, neit_package_control_entry);
    return;
}

void fbe_api_neit_test_teardown(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "=== destroying all configurations ===\n");

    fbe_test_unload_neit_package();
    fbe_test_unload_physical_package();
    
    status = fbe_api_common_destroy_sim();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Destroy the terminator.*/
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

/* Temporary hack - should be removed */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}

