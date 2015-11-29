#ifndef HW_TESTS_H
#define HW_TESTS_H

#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"

// add any new test cases to the following function
mut_testsuite_t * fbe_test_create_real_hardware_test_suite(mut_function_t startup, mut_function_t teardown);

extern char * static_config_hw_short_desc;
extern char * static_config_hw_long_desc;
void static_config_hw_test(void);

extern char * larry_t_lobster_hw_short_desc;
extern char * larry_t_lobster_hw_long_desc;
void larry_t_lobster_hw_test(void);
void larry_t_lobster_hw_test_setup(void);
void larry_t_lobster_hw_test_cleanup(void);

#endif /* HW_TESTS_H */

