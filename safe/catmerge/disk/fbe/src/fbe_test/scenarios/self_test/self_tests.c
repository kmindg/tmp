#include "self_tests.h"
#include "mut.h"

mut_testsuite_t *fbe_test_create_self_test_suite(mut_function_t startup, mut_function_t teardown) {

    mut_testsuite_t *self_test_suite;
    self_test_suite = MUT_CREATE_TESTSUITE_ADVANCED("self_test_suite",startup, teardown);
    MUT_ADD_TEST_WITH_DESCRIPTION(self_test_suite, fbe_test_self_test_log_test, NULL, NULL, fbe_test_self_test_log_test_short_desc, NULL);

    return self_test_suite;
}
