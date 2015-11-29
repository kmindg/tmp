#ifndef FBE_TEST_SELF_TESTS_H
#define FBE_TEST_SELF_TESTS_H

#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"

#define FBE_TEST_SELF_TEST_LOG_DIR_NAME "self_test_log_dir"
#define FBE_TEST_SELF_TEST_LOG_DIR "./"FBE_TEST_SELF_TEST_LOG_DIR_NAME"/"

// all self tests should be done in this function
mut_testsuite_t *fbe_test_create_self_test_suite(mut_function_t startup, mut_function_t teardown);

extern char * fbe_test_self_test_log_test_short_desc;
void fbe_test_self_test_log_test(void);

#endif /* FBE_TEST_SELF_TESTS_H */

