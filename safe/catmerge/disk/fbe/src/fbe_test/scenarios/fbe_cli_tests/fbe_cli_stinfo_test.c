/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_stinfo_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing stin cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  6/1/2011 - Created from the rginfo cli test. Rob Foley
 *
***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_cli_tests.h"
#include "fbe/fbe_api_sim_transport.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"

char * stinfo_test_short_desc = "Test stinfo cli commands";
char * stinfo_test_long_desc ="\
Test Description: Test stinfo commands with both valid and invalid arguments.";

/* fbe cli RDT commands and keyword array. */
/*!*******************************************************************
 * @def stinfo_test_cases
 *********************************************************************
 * @brief stinfo commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t stinfo_test_cases[] =
{
    /* Sector tracing commands*/
    {
        "stinfo",
        {"Get sector trace information succeeded"}, 
        {"Get sector trace information failed"} 
    },
    {
        "stinfo -h",
        {"sector_trace_info | stinfo -v | -verbose"}, 
        {"Get sector trace information failed"} 
    },
    {
        "stinfo -v",
        {"Get sector trace information succeeded"}, 
        {"Get sector trace information failed"} 
    },
    {
        "stinfo -stl",
        {"stinfo SUCCESS: Restore trace level succeeded"}, 
        {"Set / Restore trace level failed"} 
    },
    {
        "stinfo -stl 2",
        {"stinfo SUCCESS: Set trace level succeeded"}, 
        {"Set / Restore trace level failed"} 
    },
    {
        "stinfo -stm",
        {"stinfo SUCCESS: Restore trace flags/mask succeeded"}, 
        {"Set / Restore  trace flags/mask failed"} 
    },
    {
        "stinfo -stm 2",
        {"stinfo SUCCESS: Set trace flags/mask succeeded"}, 
        {"Set / Restore  trace flags/mask failed"} 
    },
    {
        "stinfo -reset_counters",
        {"stinfo SUCCESS: Reset counters succeeded"}, 
        {"stinfo ERROR: Reset counters failed"} 
    },
    {
        "stinfo -display_counters",
        {"stinfo SUCCESS: Display counters succeeded"}, 
        {"stinfo ERROR: Display counters failed"} 
    },
    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },

};

/*!**************************************************************
 * fbe_cli_stinfo_setup()
 ****************************************************************
 * @brief
 *  Setup function for stinfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  6/1/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_stinfo_setup(void)
{
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPB,CLI_MODE_TYPE_INTERACTIVE);

    fbe_cli_common_setup(fbe_test_load_pdo_config);
    elmo_load_sep_and_neit();
    return;
}
/******************************************
 * end fbe_cli_rginfo_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_stinfo_test()
 ****************************************************************
 * @brief
 *  Test function for stinfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  6/1/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_stinfo_test(void)
{
    fbe_test_cli_run_cmds(stinfo_test_cases);
    return;
}
/******************************************
 * end fbe_cli_stinfo_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_stinfo_test()
 ****************************************************************
 * @brief
 *  Teardown function for stinfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  6/1/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_stinfo_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_stinfo_teardown()
 ******************************************/
/*************************
 * end file fbe_cli_stinfo_test.c
 *************************/


