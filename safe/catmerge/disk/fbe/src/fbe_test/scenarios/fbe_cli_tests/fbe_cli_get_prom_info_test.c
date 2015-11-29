/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_get_prom_info_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing getprominfo cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  06-Sep-2011 - Created  Shubhada Savdekar
 *
***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_tests.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "fbe_test_esp_utils.h"

//#include "fbe/fbe_api_sim_transport.h"
//#include "sep_utils.h"


//#include "fbe/fbe_api_discovery_interface.h"
//#include "fbe/fbe_api_common_transport.h"
//#include "fbe/fbe_api_job_service_interface.h"

char * get_prom_info_test_short_desc = "Test getprominfo cli commands";
char * get_prom_info_test_long_desc ="\
Test Description: Test getprominfo command having following syntax\n\
getprominfo <operations> <switches>\n\
getprominfo -h | -help\n\
getprominfo -r -b <bus> -e <enclosure> -s <slot> -c <component id> -sp <0|1> \n\
getprominfo -w -b<bus> -e <enclosure> -s<slot> -c<component id> -f <field> -o <offset>  -bs <buffer size> <data>\n\
getprominfo -l \n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 2: Start fbecli application for each command\n\
STEP 3: Check expected output and unexpected output\n\
";

/*!*******************************************************************
 * @def FBE_CLI_CMDS_GETPROMINFO
 *********************************************************************
 * @brief Number of getprominfo commands to test
 *
 *********************************************************************/
#define FBE_CLI_CMDS_GET_PROM_INFO          4 

/*!*******************************************************************
 * @def cli_cmd_and_keywords
 *********************************************************************
 * @brief getprominfo commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t cli_cmd_and_keywords[FBE_CLI_CMDS_GET_PROM_INFO] =
{
    /*resumeprominfo commands */
    {
        "rp -h",
        {"resumeprom <operation> <switches>"},
        {"resumeprom: ERROR: Invalid Input"}
    },
    {
        "rp -r -b 0 -e 0 -s 1 -d midplane",
        {"The resume prom read successfully"},
        {"resumeprom: ERROR: Error in reading the prom info", "resumeprom: ERROR:"}
    },
    {
        "rp -w -b 0 -e 0 -s 1 -d midplane -f 4 -bs 45",
        {"The resume prom written successfully"},
        {"resumeprom: ERROR: ERROR in writing to the resume prom","resumeprom: ERROR:"}
    },
    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },
};

/*!**************************************************************
 * fbe_cli_get_prom_info_setup()
 ****************************************************************
 * @brief
 *  Setup function for getprominfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  06-Sep-2011 - Created.  Shubhada Savdekar
 *
 ****************************************************************/
void fbe_cli_get_prom_info_setup(void)
{
    /* Config test to be done under SPA*/
    fbe_status_t status;
        
    // Set up fbecli to run in script mode with option "s a"
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

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
    /* Load the logical packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_test_common_util_test_setup_init();

    return;
}
/******************************************
 * end fbe_cli_get_prom_info_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_get_prom_info_test()
 ****************************************************************
 * @brief
 *  Test function for getprominfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  06-Sep-2011 - Created. Shubhada Savdekar
 *
 ****************************************************************/
void fbe_cli_get_prom_info_test(void)
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
/******************************************
 * end fbe_cli_get_prom_info_test()
 ******************************************/
/*!**************************************************************
 * void fbe_cli_get_prom_info_teardown()
 ****************************************************************
 * @brief
 *  Teardown function for getprominfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  06-Sep-2011 - Created. Shubhada Savdekar
 *
 ****************************************************************/
void fbe_cli_get_prom_info_teardown(void)
{
    // Reset test control by default values
    reset_test_control();

    // Unload used packages
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end fbe_cli_get_prom_info_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_get_prom_info_test.c
 *************************/

