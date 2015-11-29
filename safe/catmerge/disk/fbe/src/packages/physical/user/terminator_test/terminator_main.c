#include "terminator.h"
#include "fbe_trace.h"
#include "fbe/fbe_emcutil_shell_include.h"

int __cdecl main (int argc , char **argv)
{
    mut_testsuite_t *terminator_suite;                /* pointer to testsuite structure */
    mut_testsuite_t *miniport_api_TestSuite;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */
    fbe_trace_set_default_trace_level (FBE_TRACE_LEVEL_DEBUG_HIGH);

    terminator_suite = MUT_CREATE_TESTSUITE("terminator_suite")   /* testsuite is created */
    MUT_ADD_TEST(terminator_suite, terminator_insert_remove, NULL, NULL)
    //MUT_ADD_TEST(terminator_suite, terminator_load_config_file_test, NULL, NULL)
    /*test the miniport apis*/
    miniport_api_TestSuite = MUT_CREATE_TESTSUITE("Terminator_miniport_api_suite")
//  MUT_ADD_TEST (miniport_api_TestSuite,
//                terminator_miniport_api_test_enumerate_ports,
//                terminator_miniport_api_test_init,
//                terminator_miniport_api_test_destroy)
//  MUT_ADD_TEST (miniport_api_TestSuite,
//                terminator_miniport_api_test_register_async,
//                terminator_miniport_api_test_init,
//                terminator_miniport_api_test_destroy)
//  MUT_ADD_TEST (miniport_api_TestSuite,
//                terminator_miniport_api_test_register_completion,
//                terminator_miniport_api_test_init,
//                terminator_miniport_api_test_destroy)
//  MUT_ADD_TEST (miniport_api_TestSuite,
//                terminator_miniport_api_test_get_port_type,
//                terminator_miniport_api_test_init,
//                terminator_miniport_api_test_destroy)
//  MUT_ADD_TEST (miniport_api_TestSuite,
//                terminator_miniport_api_test_get_port_info,
//                terminator_miniport_api_test_init,
//                terminator_miniport_api_test_destroy)
//  MUT_ADD_TEST(miniport_api_TestSuite,
//               terminator_miniport_api_test_generate_drive_io,
//                terminator_miniport_api_test_init,
//                terminator_miniport_api_test_destroy)
//  MUT_ADD_TEST(miniport_api_TestSuite,
//      terminator_miniport_api_test_port_reset,
//      NULL,
//      NULL)

    //comment this for now as we already have a seperate ess_test_suite.
    /*
    MUT_ADD_TEST(miniport_api_TestSuite,
                 terminator_miniport_api_test_generate_phy_io,
                  terminator_miniport_api_test_init,
                  terminator_miniport_api_test_destroy)
    */
    MUT_RUN_TESTSUITE(terminator_suite)               /* testsuite is executed */
    //MUT_RUN_TESTSUITE(miniport_api_TestSuite)
}
