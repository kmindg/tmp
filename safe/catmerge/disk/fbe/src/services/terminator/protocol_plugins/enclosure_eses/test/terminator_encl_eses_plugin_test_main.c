/***************************************************************************
 *  terminator_encl_eses_plugin_test_main.c
 ***************************************************************************
 *
 *  Description
 *      Eses encl plugin test suite
 *      
 *
 *  History:
 *     Oct08   Created
 *    
 ***************************************************************************/

#include "terminator_encl_eses_plugin_test.h"
#include "fbe/fbe_emcutil_shell_include.h"

int __cdecl main (int argc , char **argv)
{
	mut_testsuite_t *terminator_encl_eses_plugin_test_suite;                /* pointer to testsuite structure */

#include "fbe/fbe_emcutil_shell_maincode.h"

	mut_init(argc, argv);                               /* before proceeding we need to initialize MUT infrastructure */

	terminator_encl_eses_plugin_test_suite = MUT_CREATE_TESTSUITE("terminator_encl_eses_plugin_test_suite");   /* testsuite is created */
	MUT_ADD_TEST(terminator_encl_eses_plugin_test_suite, terminator_encl_eses_plugin_sense_data_test, NULL, NULL);

	MUT_RUN_TESTSUITE(terminator_encl_eses_plugin_test_suite);
}
