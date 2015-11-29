
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_pdo_test.c
***************************************************************************
*
* DESCRIPTION
* This is the test of fbe cli "pdo".
* 
* HISTORY
*   09/28/2010:  Created. hud1
*
***************************************************************************/

#include "fbe_cli_tests.h"
#include "fbe/fbe_api_sim_transport.h"


// fbe cli command and keyword array. 
static fbe_cli_cmd_t test_pdo_default =
{
	"pdo",
	{"Discovered 0015 objects in total.", "Discovery| 0x1 ENABLED ", "0x2 DISCOVERY"}, //expected keywords for "pdo"
	{"list/l             -list stuff", "enclbuf/eb         -Enclosure [ESES]Buffer commands", "rdgen/rd           -generate I/O", "createrg/crg       -Create new RG/ modify existing RG"} //unexpected keywords for "pdo"
};

static fbe_cli_cmd_t test_pdo_invalid_arg =
{
    "pdo -asdf",          /*cmd*/
    {"invalid"},          /*expected keywords*/
    {"success"}           /*unexpected*/   
};

static fbe_cli_cmd_t test_pdo_get_mode_pages =
{ 
    "pdo -p 0 -e 0 -s 0 -get_mode_pages",                   /*cmd*/
    {""},                                                   /*expected keywords*/
    {"invalid", "no valid command", "not support"} /*unexpected*/
};

static fbe_cli_cmd_t test_pdo_get_mode_pages_raw =
{ 
    "pdo -p 0 -e 0 -s 0 -get_mode_pages -raw",                  /*cmd*/
    {""},                                                       /*expected keywords*/ 
    {"invalid", "no valid command", "not support"}     /*unexpected*/
};

// "pdo" test setup function
void fbe_cli_pdo_setup(void)
{
	// Config test to be done under SPB
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

	// Set up fbecli to run in interactive mode with option "s a"
	init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPB,CLI_MODE_TYPE_INTERACTIVE);

	// Load "pdo" test config
	fbe_cli_common_setup(fbe_test_load_pdo_config);

}
// "pdo" test

void fbe_cli_pdo_test(void)
{
    fbe_bool_t result;
	// As an example, all expected keywords are found and unexpected keywords are not found, the result is TRUE, so this test is passed

    /* Test invalid arg. */
    result = fbe_cli_base_test(test_pdo_invalid_arg);
	MUT_ASSERT_TRUE(result);

    /* Test default pdo cmd */
    result = fbe_cli_base_test(test_pdo_default);
	MUT_ASSERT_TRUE(result);    

    /* Test -get_mode_pages.  Just a sanity test that it returns successfully. */
    result = fbe_cli_base_test(test_pdo_get_mode_pages);
    MUT_ASSERT_TRUE(result);

    result = fbe_cli_base_test(test_pdo_get_mode_pages_raw);
    MUT_ASSERT_TRUE(result);


}

// "pdo" test teardown function
void fbe_cli_pdo_teardown(void)
{
	// Reset test control by default values
	reset_test_control();

	// Unload used packages
	fbe_cli_common_teardown();

	// Reset test to be done under SPA
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
}



