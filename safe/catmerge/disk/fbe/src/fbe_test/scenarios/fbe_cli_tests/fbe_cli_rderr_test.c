
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_rderr_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing RDERR cli commands
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

char * rderr_test_short_desc = "Test rderr cli commands";
char * rderr_test_long_desc ="\
Test Description: Test rginfo command having following syntax\n\
rderr [switches] <recnum | all>\n\
Switches:\n\
     -help |h  - Displays help\n\
     -display |d - display records\n\
     -info |i - display information on available tables\n\
     -table |t - switch to new default table [0..max_tables]\n\
     -modify |m - [start end] modify records\n\
     -create|c - create records\n\
     -remove|r - [start end] remove records\n\
     -mode    [CNT | ALW | RND | SKP] [start end]\n\
              change mode for range of records or all records\n\
     -errtype [HARD | CRC | TS | WS | BTS | BWS] [start end]\n\
              change type for range of records or all records\n\
     -gap [blocks] [start] [end]\n\
              Change the gap in between records for the given\n\
              range of records.  This keeps the LBA at the start\n\
              of the range the same, but following LBAS will be\n\
              offset by the given block count.\n\
     -pos [bitmask] [start] [end]\n\
              Change the position bitmask for the given\n\
              range of records.\n\
     -enable|ena  - enable RDERR\n\
     -disable| dis- disable RDERR\n\
     -enable_for_rg | ena_rg - [Raid Group Number] enable RDERR for specific Raid group.\n\
     -disable_for_rg | dis_rg - [Raid Group Number]disable RDERR for specific Raid group.\n\
     -enable_for_object | ena_obj - [Object Id, Package Id] enable RDERR for specific object id in given package id.\n\
     -disable_for_object |dis_obj - [Object Id, Package Id] disable RDERR for specific object id in given package id\n\
     -enable_for_class |ena_cla - [Object Id, Package Id] enable RDERR for specific class id in given package id.\n\
     -disable_for_class |dis_cla - [Object Id, Package Id] disable RDERR for specific class id in given package id\n\
     -stats - Display statistics of logical error injection service\n\
     -stats_for_rg- - [Raid Group Number] Display statistics for raid group.\n\
     -stats_for_object -[Object Id] Display statistics for Object.\n\
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

#define FBE_CLI_CMDS_RDERR           19

/* fbe cli RDT commands and keyword array. */
static fbe_cli_cmd_t cli_cmd_and_keywords[FBE_CLI_CMDS_RDERR] =
{
    {
        "rderr",
        {"rderr Status               DISABLED"}, 
        {"RDERR status ENABLED"} 
    },

    {
        "rderr -ena",
        {"RDERR status ENABLED"}, 
        {"RDERR status DISABLED"} 
    },
    {
        "rderr -dis",
        {"RDERR status DISABLED"}, 
        {"RDERR status ENABLED"} 
    },
    {
        "rderr -enable",
        {"RDERR status ENABLED"}, 
        {"RDERR status DISABLED"} 
    },
    {
        "rderr -disable",
        {"RDERR status DISABLED"}, 
        {"RDERR status ENABLED"} 
    },
    {
        "rderr -enable_for_rg 0",
        {"RDERR status ENABLED for Object id "}, 
        {"rderr: ERROR: Not able to enable error injection"} 
    },
    {
        "rderr -disable_for_rg 0",
        {"RDERR status DISABLED for Object id "}, 
        {"rderr: ERROR: Not able to enable error injection"} /*--check---*/
    },
    {
        "rderr -i",
        {"Table "}, 
        {"RDERR status ENABLED"} 
    },
    {
        "rderr -t 2",
        {"is loaded for LEI service"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -t 2;rderr -d",
        {"Error records information displayed successfully"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -stats",
        {"Statistics information on the LEI service"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -stats_for_rg 0",
        {"Object Statistics information on the LEI service"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -stats_for_object 0xffffffff",
        {"Object Statistics information on the LEI service"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -enable_for_object 0xffffffff 4",
        {"RDERR status ENABLED for Object id "}, 
        {"rderr: ERROR: Not able to enable error injection"} 
    },
    {
        "rderr -disable_for_object 0xffffffff 4",
        {"RDERR status DISABLED for Object id "},
        {"rderr: ERROR: Not able to disable error injection for Object id"}, 
    },
    {
        "rderr -t 2;rderr -mode TNR 0 1",
        {"is loaded for LEI service", "Modifications Completed"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -t 2;rderr -errtype TS 126 127",
        {"is loaded for LEI service", "Modifications Completed"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -t 2;rderr -pos 0x4 12 15",
        {"is loaded for LEI service", "Modifications Completed"}, 
        {"rderr: ERROR: Invalid Argument"} 
    },
    {
        "rderr -t 2;rderr -gap 4 12 15",
        {"is loaded for LEI service", "Modifications Completed"}, 
        {"rderr: ERROR: Invalid Argument"} 
    }
};


/*!*******************************************************************
 * @def FBECLI_RDERR_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_RDERR_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_RDERR_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_RDERR_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_RDERR_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_RDERR_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var fbecli_rderr_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t fbecli_rderr_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t fbecli_rderr_raid_group_config_qual[] = 
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
 * end fbecli_rderr_raid_group_config_qual()
 **************************************/

/*!**************************************************************
 * fbe_test_load_rderr_config()
 ****************************************************************
 * @brief
 *  Setup for a RDERR test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *  11/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_test_load_rderr_config(fbe_test_rg_configuration_array_t* array_p)
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
 * end fbe_test_load_rderr_config()
 ******************************************/


/*!**************************************************************
 * fbe_cli_rderr_setup()
 ****************************************************************
 * @brief
 *  Setup function for RDERR command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_setup(void)
{
    fbe_u32_t CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_rderr_raid_group_config_qual[0];
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPB,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "rginfo" specific test config*/
    fbe_test_load_rginfo_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_rderr_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rderr_test()
 ****************************************************************
 * @brief
 *  Test function for RDERR command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  10/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rderr_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Run the error cases for all raid types supported
     */
    for (raid_type_index = 0; raid_type_index < FBECLI_RDERR_RAID_TYPE_COUNT; raid_type_index++)
    {
        /* Qual.
         */
        rg_config_p = &fbecli_rderr_raid_group_config_qual[raid_type_index][0];
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
        elmo_create_raid_groups(rg_config_p, FBECLI_RDERR_TEST_LUNS_PER_RAID_GROUP, FBECLI_RDERR_TEST_CHUNKS_PER_LUN);
        /* run all cli rderr commands*/
        fbe_cli_run_all_cli_cmds(rg_config_p,
                                 cli_cmd_and_keywords,
                                 FBE_CLI_CMDS_RDERR);

        /* Destroy the RAID group and LU configuration. */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);
    }
    return;
}
/******************************************
 * end fbe_cli_rderr_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rderr_teardown()
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
void fbe_cli_rderr_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_rderr_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_rderr_test.c
 *************************/


