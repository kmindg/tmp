
/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*******************************************************************
 * @def FBE_CLI_CMDS_FORCERPREAD
 *********************************************************************
 * @brief Number of forcerpread commands to test
 *
 *********************************************************************/


/***************************************************************************
* fbe_cli_forcerpread_test.c
***************************************************************************
*
* DESCRIPTION
* This is the test of fbe cli "forcerpread".
* 
* HISTORY
*   09/28/2010:  Created. Harshal Wanjari
*
***************************************************************************/
#include "fbe_cli_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_environment_limit_interface.h"
#include "esp_tests.h"
#include "fbe_test_common_utils.h"


// fbe cli command and keyword array.
static fbe_cli_cmd_t cli_cmd_and_keywords[] =
{
    {
        "forcerpread", //fbe cli command
        {"forcerpread: Too few arguments."}, //expected keywords for "forcerpread"
        {"Resume PROM read initated successfully. "} //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -c 9 -sp 0", //fbe cli command
        {"FBE-CLI ERROR!  forcerpread : Too few arguments."}, //expected keywords for "forcerpread"
        {"Resume PROM read initated successfully. "} ,//unexpected keywords for "forcerpread"
     },
    {
        "forcerpread -h", //fbe cli command
        {" forcerpread/frr  - Forces resume prom reading. Usage:"}, //expected keywords for "forcerpread"
        {"Resume PROM read initated successfully. "}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d midplane -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d lcc -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d fan -sp 0", //fbe cli command
        {"resumeprom: ERROR: Invalid device type for the location provided."}, //expected keywords for "forcerpread"
        {"Resume PROM read initated successfully."}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d iom -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "} ,//unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d ps -sp 0",  //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d mezz -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d mgmt -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d sp -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },    
    {
        "forcerpread -b 0 -e 0 -s 0 -d drivemidplane -sp 0", //fbe cli command
        {"resumeprom: ERROR: Invalid device type for the location provided."}, //expected keywords for "forcerpread"
        {"Resume PROM read initated successfully."}, //unexpected keywords for "forcerpread"
    },
    {
        "forcerpread -b 0 -e 0 -s 0 -d bbu -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },    
    {
        "forcerpread -b 0 -e 0 -s 0 -d bm -sp 0", //fbe cli command
        {"Resume PROM read initated successfully."}, //expected keywords for "forcerpread"
        {"FBE-CLI ERROR!  "}, //unexpected keywords for "forcerpread"
    },    
    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },
};

//static int FBE_CLI_CMDS_FORCERPREAD = sizeof(cli_cmd_and_keywords)/(sizeof(cli_cmd_and_keywords[0]));


// "forcerpread" test setup function
void fbe_cli_forcerpread_setup(void)
{
        
    // Set up fbecli to run in script mode with option "s a"
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
   
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        //fbe_test_sep_util_init_rg_configuration_array(&clifford_raid_group_config[0]);
    
        // Load the physical package and create the physical configuration. 
        elmo_physical_config();
    
        /* Load the logical packages (ESP and SEP). 
         */
        elmo_load_sep_and_neit();
    
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }
}

// "forcerpread" test
void fbe_cli_forcerpread_test(void)
{
    fbe_u32_t count = fbe_test_cli_cmd_get_count(cli_cmd_and_keywords);
    fbe_u32_t index;
    fbe_bool_t result;

    for ( index = 0; index < (count-1); index++)
    {
        //As an example, one expected keyword is NOT found, the result is FALSE.
        result = fbe_cli_base_test(cli_cmd_and_keywords[index]);
        
        //As an example, check whether result is an expected false
        MUT_ASSERT_TRUE(result);
    }
    return ;
}

// "forcerpread" test teardown function
void fbe_cli_forcerpread_teardown(void)
{
    // Reset test control by default values
    reset_test_control();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
