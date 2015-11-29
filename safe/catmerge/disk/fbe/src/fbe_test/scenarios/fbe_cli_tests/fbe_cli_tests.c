#include "fbe_cli_tests.h"
#include "mut.h"
#include "fbe_test_common_utils.h"

mut_testsuite_t *fbe_test_create_fbe_cli_test_suite(mut_function_t startup, mut_function_t teardown) {

    mut_testsuite_t * fbe_cli_test_suite = NULL; 
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        fbe_cli_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("fbe_cli_test_suite",startup, teardown);   /* fbe cli testsuite is created */
    }
    else
    {
        fbe_cli_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("extended_fbe_cli_test_suite",startup, teardown);   /* fbe cli testsuite is created */
    }
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_pdo_test, fbe_cli_pdo_setup, fbe_cli_pdo_teardown); /* add test of "pdo" command */
    /* MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_ldo_test, fbe_cli_ldo_setup, fbe_cli_ldo_teardown);  ldo command is not supported.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_rdt_test, fbe_cli_rdt_setup, fbe_cli_rdt_teardown,
                                  rdt_test_short_desc, rdt_test_long_desc); /* add test of "rdt" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_rderr_test, fbe_cli_rderr_setup, fbe_cli_rderr_teardown,
                                  rderr_test_short_desc, rderr_test_long_desc); /* add test of "rderr" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_rginfo_test, fbe_cli_rginfo_setup, fbe_cli_rginfo_teardown,
                                  rginfo_test_short_desc, rginfo_test_long_desc); /* add test of "rginfo" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_stinfo_test, fbe_cli_stinfo_setup, fbe_cli_stinfo_teardown,
                                  stinfo_test_short_desc, stinfo_test_long_desc); /* add test of "stinfo" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_luninfo_test, fbe_cli_luninfo_setup, fbe_cli_luninfo_teardown,
                                  luninfo_test_short_desc, luninfo_test_long_desc); /* add test of "luninfo" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_odt_test, fbe_cli_odt_setup, fbe_cli_odt_teardown,
                                  odt_test_short_desc, odt_test_long_desc); /* add test of "odt" command.*/
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_sfpinfo_test, fbe_cli_sfpinfo_setup, 
                                  fbe_cli_sfpinfo_teardown);/* add test of "sfpinfo" command.*/
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_getwwn_test, fbe_cli_getwwn_setup, 
                                  fbe_cli_getwwn_teardown);/* add test of "getwwn" command.*/
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_db_transaction_test, fbe_cli_db_transaction_setup,
                                    fbe_cli_db_transaction_teardown); /* add test of "database -getTransaction" command */
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_forcerpread_test, fbe_cli_forcerpread_setup, 
                                  fbe_cli_forcerpread_teardown);/* add test of "forcerpread" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_get_prom_info_test, fbe_cli_get_prom_info_setup, fbe_cli_get_prom_info_teardown,
                                  get_prom_info_test_short_desc, get_prom_info_test_long_desc); /* add test of "getprominfo" command.*/
    MUT_ADD_TEST_WITH_DESCRIPTION(fbe_cli_test_suite, fbe_cli_bgo_enable_disable_test, fbe_cli_bgo_enable_disable_setup, fbe_cli_bgo_enable_disable_teardown,
                                  bgo_enable_disable_test_short_desc, bgo_enable_disable_test_long_desc); /* add test of "bgo_enable" command.*/

    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_sepls_test, fbe_cli_sepls_setup, 
                                  fbe_cli_sepls_teardown);/* add test of "sepls" command.*/
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_ddt_test, fbe_cli_ddt_setup, fbe_cli_ddt_teardown); /* add test of "ddt" command */
    MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_mminfo_test, fbe_cli_mminfo_setup, fbe_cli_mminfo_teardown); /* add test of "mminfo" command */
    /*enable this test when system config backup &restore tool completed*/
   // MUT_ADD_TEST(fbe_cli_test_suite, fbe_cli_systemdump_test, fbe_cli_systemdump_setup, fbe_cli_systemdump_teardown); /* add test of "systemdump" command */
	/* End of fbe_cli tests*/

    return fbe_cli_test_suite;
}
