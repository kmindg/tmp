
/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_sfpinfo_test.c
***************************************************************************
*
* DESCRIPTION
* This is the test of fbe cli "sfpinfo".
* 
* HISTORY
*   09/28/2010:  Created. Harshal Wanjari
*
***************************************************************************/

#include "fbe_cli_tests.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_esp_utils.h"


// fbe cli command and keyword array.
static fbe_cli_cmd_t cli_cmd_and_keywords =
{
    "sfpinfo", //fbe cli command
    {"Information on SFP for SPA"}, //expected keywords for "sfpinfo"
    {"FBE-CLI ERROR!  "} //unexpected keywords for "sfpinfo"
};

// "sfpinfo" test setup function
void fbe_cli_sfpinfo_setup(void)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    // Set up fbecli to run in script mode with option "s a"
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_SCRIPT);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_esp_load_simple_config(SPID_DEFIANT_M1_HW_TYPE, FBE_SAS_ENCLOSURE_TYPE_VIPER);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_startPhyPkg(TRUE, SIMPLE_CONFIG_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Load the logical packages */
    sep_config_load_sep_and_neit();

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_test_common_util_test_setup_init();

}
// "sfpinfo" test
void fbe_cli_sfpinfo_test(void)
{
    //As an example, one expected keyword is NOT found, the result is FALSE.
    BOOL result = fbe_cli_base_test(cli_cmd_and_keywords);

    //As an example, check whether result is an expected true
    MUT_ASSERT_TRUE(result);
}

// "sfpinfo" test teardown function
void fbe_cli_sfpinfo_teardown(void)
{
    // Reset test control by default values
    reset_test_control();

     // Unload used packages
    fbe_test_sep_util_destroy_neit_sep_physical();
}





