#ifndef TERMINATOR_ENCL_ESES_PLUGIN_TEST_H
#define TERMINATOR_ENCL_ESES_PLUGIN_TEST_H

#include "mut.h"
#include "terminator_sas_io_api.h"

/* terminator_encl_eses_plugin_test_suite */
void terminator_encl_eses_plugin_sense_data_test(void);
fbe_sg_element_t * terminator_eses_test_alloc_memory_with_sg( fbe_u32_t bytes );

#endif /* TERMINATOR_ENCL_ESES_PLUGIN_TEST_H */
