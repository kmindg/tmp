
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_ldo_test.c
***************************************************************************
*
* DESCRIPTION
* This is the test of fbe cli "ldo".
* 
* HISTORY
*   09/28/2010:  Created. hud1
*
***************************************************************************/

#include "fbe_cli_tests.h"


// fbe cli command and keyword array.
static fbe_cli_cmd_t cli_cmd_and_keywords =
{
	"ldo", //fbe cli command
	{"0x0005 | 0_0_0  | 0x10b5c731 | 0520      | 0x02      READY"}, //expected keywords for "ldo"
	{"list/l             -list stuff", "enclbuf/eb         -Enclosure [ESES]Buffer commands", "rdgen/rd           -generate I/O", "createrg/crg       -Create new RG/ modify existing RG"} //unexpected keywords for "ldo"
	
};

// "ldo" test setup function
void fbe_cli_ldo_setup(void)
{
	// Set up fbecli to run in script mode with option "s a"
	init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_SCRIPT);

	// Load "ldo" test config
	fbe_cli_common_setup(fbe_test_load_ldo_config);
	
}
// "ldo" test
void fbe_cli_ldo_test(void)
{
	//As an example, one expected keyword is NOT found, the result is FALSE.
	BOOL result = fbe_cli_base_test(cli_cmd_and_keywords);

	//As an example, check whether result is an expected false
	MUT_ASSERT_TRUE(result);
}

// "ldo" test teardown function
void fbe_cli_ldo_teardown(void)
{
	// Reset test control by default values
	reset_test_control();

	// Unload used packages
	fbe_cli_common_teardown();
}


