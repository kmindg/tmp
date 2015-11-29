#include "terminator_drive_sas_plugin_test.h"
#include "fbe/fbe_emcutil_shell_include.h"

int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *terminator_drive_sas_plugin_error_injection_suite;                /* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

    terminator_drive_sas_plugin_error_injection_suite = MUT_CREATE_TESTSUITE("terminator_drive_sas_plugin_error_injection_suite");   /* testsuite is created */
    MUT_ADD_TEST(terminator_drive_sas_plugin_error_injection_suite, drive_sas_plugin_inquiry_positive_test, NULL, NULL);
    MUT_ADD_TEST(terminator_drive_sas_plugin_error_injection_suite, drive_sas_plugin_error_injection_state_transition_test, NULL, NULL);
    MUT_ADD_TEST(terminator_drive_sas_plugin_error_injection_suite, drive_sas_plugin_error_injection_read_test, NULL, NULL);
    MUT_ADD_TEST(terminator_drive_sas_plugin_error_injection_suite, drive_sas_plugin_error_injection_reset_test, NULL, NULL);
    MUT_ADD_TEST(terminator_drive_sas_plugin_error_injection_suite, drive_sas_plugin_error_injection_reactivate_test, NULL, NULL);
    MUT_ADD_TEST(terminator_drive_sas_plugin_error_injection_suite, drive_sas_plugin_error_injection_reactivate_time_test, NULL, NULL);

    MUT_RUN_TESTSUITE(terminator_drive_sas_plugin_error_injection_suite);
}