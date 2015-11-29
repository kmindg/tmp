
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_sepls_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing SEPLS cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  02/06/2012 - Created. Trupti Ghate
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

char * sepls_test_short_desc = "Test sepls cli commands";
char * sepls_test_long_desc ="\
Test Description: Test sepls command having following syntax\n\
!sepls [-help]|[-allsep]|[-lun]|[-rg]|[-vd]|[-pvd]|[-level{1|2|3|4}]\n\
  Displays SEP objects info.\n\
  By default we show the LUN, RG SEP objects, and the configured hot spares.\n\
  -allsep - This option displays all the sep objects\n\
  -lun - This option displays all the lun objects\n\
  -rg - This option displays all the RG objects\n\
  -vd - This option displays all the VD objects\n\
  -pvd - This option displays all the PVD objects\n\
  -level <1,2,3,4> - This option display objects upto the depth level specified\n\
       1 - Will display only RG objects\n\
       2 - Will display LUN and RG objects\n\
       3 - Will display LUN, RG and VD objects\n\
       4 - Will display LUN, RG, VD and PVD objects\n\n\
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
 * @def sepls_success_test_cases
 *********************************************************************
 * @brief SEPLS commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t sepls_success_test_cases[] =
{
    {
        "sepls -help",
        {"sepls [-help]|[-allsep]|[-lun]|[-rg]|[-vd]|[-pvd]|[-level{1|2|3|4}"}, 
        {"sepls: Invalid arguments specified."} 
    },
    {
        "sepls",
        {"RG"},        /* sepls output should have "RG" entry*/
        {"sepls: Invalid arguments specified."} 
    },
    {
        "sepls -lun",
        {"LUN"},      /* 'sepls -lun' output should have "LUN" entry*/
        {"sepls: Invalid arguments specified."} 
    },
    {
        "sepls -rg",
        {"RG"},      /* 'sepls -rg' output should have "RG" entry*/
        {"sepls: Invalid arguments specified."} 
    },
    {
        "sepls -vd",
        {"VD"},       /* 'sepls -vd' output should have "VD" entry*/
        {"sepls: Invalid arguments specified."} 
    },
    {
        "sepls -pvd",
        {"PVD"},      /* 'sepls -pvd' output should have "PVD" entry*/
        {"sepls: Invalid arguments specified."} 
    },
    {
        "sepls -level 1",
        {"RG"},       /* 'sepls -level 1' output should have "RG" entry*/
        {"sepls: Invalid level. Please specify the level between 1 to 4"} 
    },
    {
        "sepls -level 2",
        {"LUN"},        /* 'sepls -level 2' output should have "LUN" entry*/
        {"sepls: Invalid level. Please specify the level between 1 to 4"} 
    },
    {
        "sepls -level 3",
        {"VD"},       /* 'sepls -level 3' output should have "VD" entry*/
        {"sepls: Invalid level. Please specify the level between 1 to 4"} 
    },
    {
        "sepls -level 4",
        {"PVD"},        /* 'sepls -level 4' output should have "PVD" entry*/
        {"sepls: Invalid level. Please specify the level between 1 to 4"} 
    },
    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },
};

/*!*******************************************************************
 * @def sepls_failure_test_cases
 *********************************************************************
 * @brief sepls test cases where we expect a failure.
 *
 *********************************************************************/
static fbe_cli_cmd_t sepls_failure_test_cases[] =
{
    {
        "sepls -rcdd",
        {"sepls: Invalid arguments specified."}, 
        {"RG"} 
    },
    {
        "sepls -level 5",
        {"sepls: Invalid level. Please specify the level between 1 to 4"}, 
        {"RG"} 
    },
    /* Terminator  */
    {
        NULL, {NULL},{NULL},
    },
};

/*!*******************************************************************
 * @def FBECLI_SEPLS_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_SEPLS_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_SEPLS_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_SEPLS_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_SEPLS_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_SEPLS_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var fbecli_sepls_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t fbecli_sepls_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
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
 * end fbecli_sepls_raid_group_config_qual()
 **************************************/

/*!**************************************************************
 * fbe_test_load_sepls_config()
 ****************************************************************
 * @brief
 *  Setup for a sepls test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *  02/06/2012 - Created. Trupti Ghate
 *
 ****************************************************************/
void fbe_test_load_sepls_config(fbe_test_rg_configuration_array_t* array_p)
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
 * end fbe_test_load_sepls_config()
 ******************************************/

/*!**************************************************************
 * fbe_cli_sepls_setup()
 ****************************************************************
 * @brief
 *  Setup function for rdt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  02/06/2012 - Created. Trupti Ghate
 *
 ****************************************************************/
void fbe_cli_sepls_setup(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_sepls_raid_group_config_qual[0];
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPB,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "sepls" specific test config*/
    fbe_test_load_sepls_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_sepls_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_sepls_test()
 ****************************************************************
 * @brief
 *  Test function for rdt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  02/06/2012 - Created. Trupti Ghate
 *
 ****************************************************************/
void fbe_cli_sepls_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t case_count;

    /* These cases should not need any raid groups.
     */
    case_count = fbe_test_cli_cmd_get_count(&sepls_failure_test_cases[0]);
    fbe_cli_run_all_cli_cmds(rg_config_p, sepls_failure_test_cases, case_count);

    /* Run the error cases for all raid types supported
      */
    for (raid_type_index = 0; raid_type_index < FBECLI_SEPLS_RAID_TYPE_COUNT; raid_type_index++)
    {
        /* Qual.
         */
        rg_config_p = &fbecli_sepls_raid_group_config_qual[raid_type_index][0];
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
        elmo_create_raid_groups(rg_config_p, FBECLI_SEPLS_TEST_LUNS_PER_RAID_GROUP, FBECLI_SEPLS_TEST_CHUNKS_PER_LUN);
        /* run all cli sepls commands*/

        case_count = fbe_test_cli_cmd_get_count(&sepls_success_test_cases[0]);
        fbe_cli_run_all_cli_cmds(rg_config_p, sepls_success_test_cases, case_count);

        /* Destroy the RAID group and LU configuration. */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);
    }
    return;
}
/******************************************
 * end fbe_cli_sepls_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_sepls_teardown()
 ****************************************************************
 * @brief
 *  Teardown function for rdt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  02/06/2012 - Created. Trupti Ghate
 *
 ****************************************************************/
void fbe_cli_sepls_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_sepls_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_sepls_test.c
 *************************/



