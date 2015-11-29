#include "terminator_test.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_emcutil_shell_include.h"

int __cdecl main (int argc , char *argv[])
{
    mut_testsuite_t *apiTestSuite;
    mut_testsuite_t *terminator_suite;
    mut_testsuite_t *baseComponentTestSuite;
    mut_testsuite_t *boardTestSuite;
    mut_testsuite_t *portTestSuite;
    //mut_testsuite_t *esesTestSuite;
    mut_testsuite_t *terminator_device_registry_test_suite;
    //mut_testsuite_t *terminator_creation_api_test_suite;
    mut_testsuite_t *terminator_class_management_test_suite;

    #include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

    /* API test suite */
    apiTestSuite = MUT_CREATE_TESTSUITE("terminator_api_suite")

    MUT_ADD_TEST(apiTestSuite, terminator_api_init_destroy_test, NULL, NULL)
    /* terminator_api_test_teardown should be called for subsequent non-insert/remove tests */
    MUT_ADD_TEST(apiTestSuite, terminator_api_d60_test,          NULL, terminator_api_test_teardown)

    MUT_ADD_TEST(apiTestSuite, terminator_api_d120_test,          NULL, terminator_api_test_teardown)


    /*
    MUT_ADD_TEST(apiTestSuite, terminator_api_get_device_id_test,          NULL, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_set_get_drive_info_test,     NULL, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_force_logout_login_test,     NULL, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_set_get_enclosure_info_test, NULL, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_set_get_io_mode_test,        NULL, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_device_login_state_test,     NULL, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_reset_device_test,              terminator_api_test_setup, terminator_api_test_teardown)
    MUT_ADD_TEST(apiTestSuite, terminator_api_miniport_sas_device_table_test, terminator_api_test_setup, terminator_api_test_teardown)
    //*/

    /* Terminator test suite */
    terminator_suite = MUT_CREATE_TESTSUITE("terminator_suite")

    MUT_ADD_TEST(terminator_suite, terminator_insert_remove,                             NULL, NULL)
    MUT_ADD_TEST(terminator_suite, terminator_pull_reinsert_drive,                       NULL, NULL)
    MUT_ADD_TEST(terminator_suite, terminator_pull_reinsert_enclosure,                   NULL, NULL)
    MUT_ADD_TEST(terminator_suite, terminator_enclosure_firmware_download_activate_test, NULL, NULL)

    /* base component test suite */
    baseComponentTestSuite = MUT_CREATE_TESTSUITE("baseComponentTestSuite")

    MUT_ADD_TEST (baseComponentTestSuite, baseComponentTest_init,                           NULL, NULL)
    MUT_ADD_TEST (baseComponentTestSuite, baseComponentTest_add_child,                      NULL, NULL)
    MUT_ADD_TEST (baseComponentTestSuite, baseComponentTest_add_remove_child,               NULL, NULL)
    MUT_ADD_TEST (baseComponentTestSuite, baseComponentTest_get_child_count,                NULL, NULL)
    MUT_ADD_TEST (baseComponentTestSuite, baseComponentTest_chain_component,                NULL, NULL)
    MUT_ADD_TEST (baseComponentTestSuite, baseComponentTest_base_component_get_first_child, NULL, NULL)

    /* board test suite */
    boardTestSuite = MUT_CREATE_TESTSUITE("boardTestSuite")

    MUT_ADD_TEST (boardTestSuite, board_new_test, NULL, NULL)

    /* port test suite */
    portTestSuite = MUT_CREATE_TESTSUITE("portTestSuite")

    MUT_ADD_TEST (portTestSuite, port_new_test,             NULL, NULL)
    MUT_ADD_TEST (portTestSuite, port_set_port_number_test, NULL, NULL)
    MUT_ADD_TEST (portTestSuite, get_sas_port_address_test, NULL, NULL)
    MUT_ADD_TEST (portTestSuite, port_destroy_test,         NULL, NULL)

    /* eses test suite */
    /*
    esesTestSuite = MUT_CREATE_TESTSUITE("terminator_eses_suite")

    MUT_ADD_TEST(esesTestSuite, terminator_set_enclosure_drive_phy_status_test,             NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_addl_status_page_test,                           NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_set_enclosure_ps_cooling_tempSensor_status_test, NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_eses_sense_data_test,                            NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_config_page_gen_code_test,                       NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_download_mcode_page_test,                        NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_config_page_version_descriptors_test,            NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_control_page_drive_bypass_test,                  NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_led_patterns_test,                               NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_drive_poweroff_test,                             NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_read_buf_test,                                   NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_write_buf_test,                                  NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_mode_cmds_test,                                  NULL, NULL)
    MUT_ADD_TEST(esesTestSuite, terminator_magnum_enclosure_test,                           NULL, NULL)
    //*/

    /* device registry test suite */
    terminator_device_registry_test_suite = MUT_CREATE_TESTSUITE("terminator_device_registry_test_suite")

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_should_not_do_it_before_init,
                 NULL,
                 NULL)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_init_test,
                 terminator_device_registry_test_setup,
                 terminator_device_registry_test_teardown)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_add_test,
                 terminator_device_registry_test_setup,
                 terminator_device_registry_test_teardown)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_remove_test,
                 terminator_device_registry_test_setup,
                 terminator_device_registry_test_teardown)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_enumerate_all_test,
                 terminator_device_registry_test_setup, 
                 terminator_device_registry_test_teardown)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_enumerate_by_type_test,
                 terminator_device_registry_test_setup,
                 terminator_device_registry_test_teardown)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_counters_test,
                 terminator_device_registry_test_setup,
                 terminator_device_registry_test_teardown)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_get_type_test,
                 terminator_device_registry_test_setup,
                 terminator_device_registry_test_teardown)

    /* this test destroy the registry, does not need the common teardown */
    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_destroy_test,
                 terminator_device_registry_test_setup,
                 NULL)

    MUT_ADD_TEST(terminator_device_registry_test_suite,
                 terminator_device_registry_should_not_do_it_after_destroy,
                 NULL,
                 NULL)

    /* creation api test suite */
    /*
    terminator_creation_api_test_suite = MUT_CREATE_TESTSUITE("terminator_creation_api_test_suite")

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_create_device_test,
                 NULL,
                 terminator_api_test_teardown)

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_enumerate_devices_test,
                 NULL,
                 terminator_api_test_teardown)

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_get_device_type_test,
                 NULL,
                 terminator_api_test_teardown)

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_set_device_attr_test,
                 NULL,
                 terminator_api_test_teardown)

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_get_device_attr_test,
                 NULL,
                 terminator_api_test_teardown)

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_destroy_device_test,
                 NULL,
                 terminator_api_test_teardown)

    MUT_ADD_TEST(terminator_creation_api_test_suite,
                 terminator_creation_api_insert_and_activate_device_test,
                 NULL,
                 terminator_api_test_teardown)
    //*/

    /* TCM test suite */
    terminator_class_management_test_suite = MUT_CREATE_TESTSUITE("terminator_class_management_test_suite")

    MUT_ADD_TEST(terminator_class_management_test_suite,
                 terminator_class_management_test,
                 NULL,
                 NULL)

    /* run the test suites */
    MUT_RUN_TESTSUITE(apiTestSuite)
    MUT_RUN_TESTSUITE(terminator_suite)
    MUT_RUN_TESTSUITE(baseComponentTestSuite)
    MUT_RUN_TESTSUITE(boardTestSuite)
    MUT_RUN_TESTSUITE(portTestSuite)

    //MUT_RUN_TESTSUITE(esesTestSuite)
    MUT_RUN_TESTSUITE(terminator_device_registry_test_suite)
    //MUT_RUN_TESTSUITE(terminator_creation_api_test_suite)
    MUT_RUN_TESTSUITE(terminator_class_management_test_suite)
}

/* Temporary hack - should be removed */
fbe_status_t fbe_get_package_id(fbe_package_id_t *package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}