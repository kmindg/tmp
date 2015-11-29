/***************************************************************************
* Copyright (C) EMC Corporation 2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_bgo_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing background operations cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  9/9/2011 - Created - Vishnu Sharma.
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

char * bgo_enable_disable_test_short_desc = "Test bgo_enable,bgo_disable,bgo_status cli commands";
char * bgo_enable_disable_test_long_desc ="\
Test Description: Test bgo command having following syntax\n\
bgo_enable   -disk 0_0_6 -operation  PVD_DZ \n\
bgo_disable -disk 0_0_6 -operation  PVD_DZ\n\
bgo_status -disk 0_0_6 -operation  PVD_DZ\n\
bgo_enable   -rg  1 -operation  RG_REB \n\
bgo_disable -rg  1 -operation  RG_REB\n\
bgo_status -rg  1 -operation  RG_REB\n\
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
 * @def FBE_CLI_CMDS_LUNINFO
 *********************************************************************
 * @brief Number of bgo halt and unhalt to test
 *
 *********************************************************************/
#define FBE_CLI_CMDS_BGO           7

/* fbe cli luninfo commands and keyword array. */
/*!*******************************************************************
 * @def cli_cmd_and_keywords
 *********************************************************************
 * @brief BGO commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t cli_cmd_and_keywords[FBE_CLI_CMDS_BGO] =
{
    {
        "bgo_enable",
        {"ERROR: Too few args."}, 
        {"bgo_enable: Success."} 
    },
    {
        "bgo_enable -h",
        {"Get information about bgo."}, 
        {"bgo_enable: ERROR: Too few args."} 
    },
    {
        "bgo_enable   -disk  -operation  PVD_DZ ",
        {"ERROR: Too few args."}, 
        {"bgo_enable: Success."} 
    },
    {
        "bgo_enable   -rg  -operation  RG_REB",
        {"ERROR: Too few args."}, 
        {"bgo_enable: Success."} 
    },
    {
        "bgo_enable   -rg  1 -operation  ",
        {"ERROR: Too few args."}, 
        {"bgo_enable: Success."} 
    },
    {
        "bgo_disable -rg   -operation  RG_REB",
        {"ERROR: Too few args."}, 
        {"bgo_disable: Success."} 
    },
    {
        "bgo_status -rg   -operation  RG_REB",
        {"ERROR: Too few args."}, 
        {"bgo_status: Success."} 
    },
};

/*!*******************************************************************
 * @def FBECLI_BGO_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_BGO_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_BGO_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_BGO_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_BGO_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_BGO_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var fbecli_bgo_halt_unhalt_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t fbecli_bgo_halt_unhalt_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t fbecli_bgo_halt_unhalt_raid_group_config_qual[] = 
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
 * end fbecli_bgo_halt_unhalt_raid_group_config_qual()
 **************************************/

/*!**************************************************************
 * fbe_test_load_bgo_enable_disable_config()
 ****************************************************************
 * @brief
 *  Setup for a bgo_halt ,bgo_unhalt,bgo_status test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *  9/9/2011 - Created - Vishnu Sharma.
 *
 ****************************************************************/
void fbe_test_load_bgo_enable_disable_config(fbe_test_rg_configuration_array_t* array_p)
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
 * end fbe_test_load_bgo_enable_disable_config()
 ******************************************/


/*!**************************************************************
 * fbe_cli_bgo_enable_disable_setup()
 ****************************************************************
 * @brief
 *  Setup function for luninfo command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  09/09/2011 - Created - Vishnu Sharma.
 *
 ****************************************************************/
void fbe_cli_bgo_enable_disable_setup(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_bgo_halt_unhalt_raid_group_config_qual[0];
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "rginfo" specific test config*/
    fbe_test_load_bgo_enable_disable_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_bgo_enable_disable_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_bgo_enable_disable_test()
 ****************************************************************
 * @brief
 *  Test function for bgo enable and bgo disable command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  09/09/2011 - Created - Vishnu Sharma.
 *
 ****************************************************************/
void fbe_cli_bgo_enable_disable_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Run the error cases for all raid types supported
     */
    for (raid_type_index = 0; raid_type_index < FBECLI_BGO_RAID_TYPE_COUNT; raid_type_index++)
    {
        /* Qual.
         */
        rg_config_p = &fbecli_bgo_halt_unhalt_raid_group_config_qual[raid_type_index][0];
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
        elmo_create_raid_groups(rg_config_p, FBECLI_BGO_TEST_LUNS_PER_RAID_GROUP, FBECLI_BGO_TEST_CHUNKS_PER_LUN);
        /* run all cli rginfo and stinfo commands*/
        fbe_cli_run_all_cli_cmds(rg_config_p,
                                 cli_cmd_and_keywords,
                                 FBE_CLI_CMDS_BGO);
        /* Destroy the RAID group and LU configuration. */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);
    }
    return;
}
/******************************************
 * end fbe_cli_bgo_enable_disable_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_bgo_enable_disable_teardown()
 ****************************************************************
 * @brief
 *  Teardown function for bgo halt and bgo unhalt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  09/09/2011 - Created - Vishnu Sharma.
 *
 ****************************************************************/
void fbe_cli_bgo_enable_disable_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_bgo_enable_disable_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_bgo_test.c
 *************************/


