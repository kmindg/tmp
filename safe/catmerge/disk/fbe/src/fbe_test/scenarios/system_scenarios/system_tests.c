#include "system_tests.h"
#include "mut.h"
#include "fbe_test_common_utils.h"

mut_testsuite_t *fbe_test_create_system_test_suite(mut_function_t startup, mut_function_t teardown) {

    mut_testsuite_t *system_test_suite = NULL;
    if (fbe_test_get_default_extended_test_level() == 0)
    {
        system_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("system_test_suite",startup, teardown);
    }
    else
    {
        system_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("extended_system_test_suite",startup, teardown);
    }
    MUT_ADD_TEST_WITH_DESCRIPTION(system_test_suite, fbe_sim_load_test, NULL, NULL, fbe_sim_test_short_desc, NULL);

    return system_test_suite;
}
