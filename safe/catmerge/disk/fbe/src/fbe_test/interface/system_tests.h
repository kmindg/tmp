#ifndef FBE_TEST_SYSTEM_TESTS_H
#define FBE_TEST_SYSTEM_TESTS_H

#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"

// all additions of tests to the suite should be done in this function
mut_testsuite_t *fbe_test_create_system_test_suite(mut_function_t startup, mut_function_t teardown);

extern char * fbe_sim_test_short_desc;
void fbe_sim_load_test(void);
void fbe_sim_load_test_set_port_base(fbe_u16_t port_base);

#endif /* FBE_TEST_SYSTEM_TESTS_H */

