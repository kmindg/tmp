#include "terminator_miniport_sas_plugin_test.h"
#include "fbe/fbe_types.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_emcutil_shell_include.h"

int __cdecl main (int argc , char **argv)
{
	mut_testsuite_t *terminator_miniport_sas_plugin_devic_table_suite;		/* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

	mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

	terminator_miniport_sas_plugin_devic_table_suite = MUT_CREATE_TESTSUITE("terminator_miniport_sas_plugin_devic_table_suite");   /* testsuite is created */
	MUT_ADD_TEST(terminator_miniport_sas_plugin_devic_table_suite, 
				 miniport_sas_plugin_device_table_double_init_test, 
				 miniport_sas_plugin_device_table_init_test, 
				 miniport_sas_plugin_device_table_destroy_test);

	MUT_ADD_TEST(terminator_miniport_sas_plugin_devic_table_suite,
				 miniport_sas_plugin_device_table_add_record_test, 
				 miniport_sas_plugin_device_table_init_test, 
				 miniport_sas_plugin_device_table_destroy_test);

	MUT_ADD_TEST(terminator_miniport_sas_plugin_devic_table_suite,
				 miniport_sas_plugin_device_table_add_remove_record_test, 
				 miniport_sas_plugin_device_table_init_test, 
				 miniport_sas_plugin_device_table_destroy_test);

	MUT_ADD_TEST(terminator_miniport_sas_plugin_devic_table_suite,
				 miniport_sas_plugin_device_table_get_device_id_test, 
				 miniport_sas_plugin_device_table_init_test, 
				 miniport_sas_plugin_device_table_destroy_test);

	MUT_ADD_TEST(terminator_miniport_sas_plugin_devic_table_suite,
				 miniport_sas_plugin_device_table_reserve_test, 
				 miniport_sas_plugin_device_table_init_test, 
				 miniport_sas_plugin_device_table_destroy_test);

	MUT_RUN_TESTSUITE(terminator_miniport_sas_plugin_devic_table_suite);
}
