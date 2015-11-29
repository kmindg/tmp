
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_rginfo_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing RGINFO cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  11/20/2010 - Created. Swati Fursule
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

char * rginfo_test_short_desc = "Test rginfo cli commands";
char * rginfo_test_long_desc ="\
Test Description: Test rginfo command having following syntax\n\
rginfo –h |-help\n\
rginfo -d -rg <raidgroup number|all> |-object_id <object id | all> \n\
-debug_flags <value> -library_flags <value> -trace_level <value> \n\
Note that Raid group number, debug flags, trace level values are in Hexadecimal\n\
Switches:\n\
    -d    - Display mode (else set mode)\n\
    -rg   - for Raid group [RG number | all]\n\
    -object_id  - for Object Id [Object Id | all ]\n\
    -debug_flags  - [DEBUG_flag value ]\n\
    -library_flags  - [library_flag value ]\n\
    -trace_level  - [trace_level value ]\n\
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
 * @def FBE_CLI_CMDS_RGINFO
 *********************************************************************
 * @brief Number of RDT commands to test
 *
 *********************************************************************/
#define FBE_CLI_CMDS_RGINFO           38

/*!*******************************************************************
 * @def rginfo_success_test_cases
 *********************************************************************
 * @brief RGINFO commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t rginfo_success_test_cases[] =
{
    {
        "rginfo -h",
        {"rginfo -d -rg <raidgroup number|all|user> |-object_id <object id | all>"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -get_rg_debug_flags -rg 0",
        {"Raid Group Debug Flag:"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -get_lib_debug_flags -rg 0",
        {"Library Debug Flag:"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -gtl -rg 0",
        {"Trace level Information :"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -d -rg 0",
        {"Raid group Information :"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -rg 0 -debug_flags 1",
        {"Set RAID GROUP DEBUG FLAG to"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -rg 0 -library_flags 1",
        {"Set RAID LIBRARY FLAG to "}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -rg 0 -trace_level 1",
        {"Set trace levels to "}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -grdf -object_id 0xffffffff",
        {"Raid Group Debug Flag:"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -gldf -object_id 0xffffffff",
        {"Library Debug Flag:"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -gtl -object_id 0xffffffff",
        {"Trace level Information :"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -d  -object_id 0xffffffff",
        {"Raid group Information :"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo  -object_id 0xffffffff -debug_flags 1",
        {"Set RAID GROUP DEBUG FLAG to"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo  -object_id 0xffffffff -library_flags 1",
        {"Set RAID LIBRARY FLAG to "}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo  -object_id 0xffffffff -trace_level 1",
        {"Set trace levels to "}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -d -rg all",
        {"Raid group Information :"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -d -object_id all",
        {"Raid group Information :"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -default_debug_flags 0x2",
        {"Set default RAID GROUP DEBUG FLAG to"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -default_library_flags 0x4",
        {"Set default RAID LIBRARY DEBUG FLAG to"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -grdf",
        {"Raid Group Debug Flag:"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -gldf",
        {"Library Debug Flag:"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    {
        "rginfo -display_raid_memory_stats",
        {"RAID MEMORY STATISTICS :"}, 
        {"rginfo: ERROR: Not able to get raid memory statistics"} 
    },
    {
        "rginfo -default_library_flags 0x4",
        {"Set default RAID LIBRARY DEBUG FLAG to"}, 
        {"rginfo: ERROR: Too few args."} 
    },
    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },
};

/*!*******************************************************************
 * @def rginfo_failure_test_cases
 *********************************************************************
 * @brief rginfo test cases where we expect a failure.
 *
 *********************************************************************/
static fbe_cli_cmd_t rginfo_failure_test_cases[] =
{
    {
        "rginfo",
        {"rginfo: ERROR: Too few args."}, 
        {"rginfo: Success."} 
    },
    {
        "rginfo -rg 0 -debug_flags 0x800000",
        {"rginfo: ERROR: Invalid argument (raid group debug flags"}, 
        {"Set raid group debug flags"} 
    },
    {
        "rginfo -rg 0 -library_flags 0x800000",
        {"rginfo: ERROR: Invalid argument (library debug flags"}, 
        {"Set raid library debug flags to"} 
    },
    {
        "rginfo -d -rg 0x5",
        {"rginfo: ERROR: Invalid argument (Raid group number should be in decimal)"}, 
        {"Raid group Information :"} 
    },

    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },
};

/*!*******************************************************************
 * @def FBECLI_RGINFO_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_RGINFO_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_RGINFO_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_RGINFO_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_RGINFO_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_RGINFO_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var fbecli_rginfo_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t fbecli_rginfo_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t fbecli_rginfo_raid_group_config_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 3 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 6 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 0 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 10 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end fbecli_rginfo_raid_group_config_qual()
 **************************************/

/*!**************************************************************
 * fbe_test_load_rginfo_config()
 ****************************************************************
 * @brief
 *  Setup for a RGINFO test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *  11/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_test_load_rginfo_config(fbe_test_rg_configuration_array_t* array_p)
{
    fbe_u32_t raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    fbe_u32_t test_index;

    for (test_index = 0; test_index < raid_type_count; test_index++)
    {
        fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
    }
    fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
    elmo_load_sep_and_neit();
    return;
}

/******************************************
 * end fbe_test_load_rginfo_config()
 ******************************************/


/*!**************************************************************
 * fbe_cli_rginfo_setup()
 ****************************************************************
 * @brief
 *  Setup function for rdt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_setup(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_rginfo_raid_group_config_qual[0];
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPB,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "rginfo" specific test config*/
    fbe_test_load_rginfo_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_rginfo_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rginfo_test()
 ****************************************************************
 * @brief
 *  Test function for rdt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t case_count;

    /* These cases should not need any raid groups.
     */
    case_count = fbe_test_cli_cmd_get_count(&rginfo_failure_test_cases[0]);
    fbe_cli_run_all_cli_cmds(rg_config_p, rginfo_failure_test_cases, case_count);

    /* Run the error cases for all raid types supported
     */
    for (raid_type_index = 0; raid_type_index < FBECLI_RGINFO_RAID_TYPE_COUNT; raid_type_index++)
    {
        /* Qual.
         */
        rg_config_p = &fbecli_rginfo_raid_group_config_qual[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_count(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        elmo_create_raid_groups(rg_config_p, FBECLI_RGINFO_TEST_LUNS_PER_RAID_GROUP, FBECLI_RGINFO_TEST_CHUNKS_PER_LUN);
        /* run all cli rginfo commands*/

        case_count = fbe_test_cli_cmd_get_count(&rginfo_success_test_cases[0]);
        fbe_cli_run_all_cli_cmds(rg_config_p, rginfo_success_test_cases, case_count);

        /* Destroy the RAID group and LU configuration. */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);
    }
    return;
}
/******************************************
 * end fbe_cli_rginfo_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rginfo_test()
 ****************************************************************
 * @brief
 *  Teardown function for rdt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_rginfo_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_rginfo_test.c
 *************************/


