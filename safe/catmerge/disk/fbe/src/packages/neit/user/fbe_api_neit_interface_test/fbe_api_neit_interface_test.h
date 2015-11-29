#include "mut.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_neit_package.h"
#include "fbe_trace.h"

/*fbe_api_device_handle_getter_suite*/

void fbe_api_neit_test_setup (void);
void fbe_api_neit_test_teardown(void);
void fbe_api_protocol_error_injection_add_remove_record_test(void);

void fbe_api_logical_error_injection_add_tests(mut_testsuite_t *suite_p);

