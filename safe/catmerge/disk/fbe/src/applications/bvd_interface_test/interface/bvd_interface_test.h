#ifndef BVD_INTERFACE_TEST_H
#define BVD_INTERFACE_TEST_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"

#define CMD_MAX_ARGC 10 /* Max number of command line arguments. */
#define CMD_MAX_ARG_SIZE 20 /* Max number of chars per argument. */
#define CMD_MAX_SIZE CMD_MAX_ARGC*CMD_MAX_ARG_SIZE
#define MAX_BUFFER_SIZEOF 2048


/* Common utilities */
void bvd_interface_test_printf(const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void bvd_interface_test_prompt_mode();

#endif /* BVD_INTERFACE_TEST_H */
