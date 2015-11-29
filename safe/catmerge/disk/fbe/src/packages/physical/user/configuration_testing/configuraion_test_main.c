#include "configuration_test.h"
#include "fbe/fbe_emcutil_shell_include.h"

int __cdecl main (int argc , char **argv)
{
	mut_testsuite_t *config_file_test_suite;                /* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

    if (argc == 2) {
		config_test_set_file_name(argv[1]);
		/*and we have to trick mut not cry about this argument*/
		argv[1] = "";
		argc = 1;
	}

	mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

	config_file_test_suite = MUT_CREATE_TESTSUITE("config_file_test_suite")   /* testsuite is created */
    
	MUT_ADD_TEST(config_file_test_suite, config_file_test, config_file_test_init, config_file_test_destroy)
	
	MUT_RUN_TESTSUITE(config_file_test_suite)
}


