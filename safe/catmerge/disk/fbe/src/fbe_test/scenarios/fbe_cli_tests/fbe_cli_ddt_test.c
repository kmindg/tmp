/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_ddt_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing DDT cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  06/01/2011 - Created  Shubhada Savdekar
 *  06/15/2012 - Adapted for ddt. 
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
#include "fbe/fbe_api_job_service_interface.h"

char * ddt_test_short_desc = "Test ddt cli commands";
char * ddt_test_long_desc ="\
Test Description: Test ddt command having following syntax\n\
ddt [switches] <operation> <frunum> <lba (64bit)> [count]\n\
ddt [switches] r <b_e_s>  <lba (64bit)> [count]\n\
ddt x \n\
ddt e offset byte <byte...> \n\
ddt i [pattern] \n\
ddt [switches] k <b_e_s> DEAD\n\
ddt [switches] u <b_e_s> \n\
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
 * @def FBE_CLI_CMDS_DDT
 *********************************************************************
 * @brief Number of DDT commands to test
 *
 *********************************************************************/
#define FBE_CLI_CMDS_DDT          5//15

/*!*******************************************************************
 * @def cli_cmd_and_keywords
 *********************************************************************
 * @brief DDT commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t cli_cmd_and_keywords[FBE_CLI_CMDS_DDT] =
{
    {
	    ACCESS_ENG_MODE_STR, /* Enable ddt command access. */
	    {""}, //expected keywords for "access"
	    {"Error","failed","ERROR"} //unexpected keywords for "access"
    },
    {
	    "ddt -pdo k 0_1_0 ", /* Kill drive - without DEAD option*/
	    {"ERROR!","Missing required"}, //expected keywords for "ddt"
	    {"object state to failed"}, //expected keywords for "ddt"
    },
    {
	    "ddt -pdo k 0_1_0 DEAD", /* Invalidate Checksum */
	    {"object state to failed"}, //expected keywords for "ddt"
	    {"Error","ERROR!","Missing required"} //unexpected keywords for "ddt"
    },
    {
	    "ddt -pdo u 0_1_1 1", /* Set standby on*/
	    {"Please use green or gr command"}, //expected keywords for "ddt"
	    {"Error"} //unexpected keywords for "ddt"
    },
    {
	    "ddt -pdo u 0_1_1 0", /* Set standby off*/
	    {"Please use green or gr command"}, //expected keywords for "ddt"
	    {"Error"} //unexpected keywords for "ddt"
    },

#if 0 /* options x, i depend on the execution of a previous r
       * Execution of one entry after the other with previous state 
       * retained is not supported by the test frame work at this time.
       * Commenting out for now to identify a work around for executing 
       * atleast 2 subsequent ddt commands with the read buffer preserved across 
       * multiple requests.
       */
    {
	    "ddt -pdo r 0_0_0 1000 1",
	    {"your data","Data read into buffer"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt x 1", /* Checksum */
	    {"Buffer checksum: old","Checksum calculation completed"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt e 7 FF F5", /* Edit buffer */
	    {"Editing done sucessfully"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt x 1", /* Checksum */
	    {"Buffer checksum: old","Checksum calculation completed"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },

    {
	    "ddt i dh 1", /* Invalidate Checksum */
	    {"Checksum invalidation completed sucessfully","dh"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt i raid 1", /* Invalidate Checksum */
	    {"Checksum invalidation completed sucessfully","raid"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },

    {
	    "ddt -pdo r 0_0_0 1100 4",
	    {"your data","Data read into buffer"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt i raid 4", /* Invalidate Checksum */
	    {"Checksum invalidation completed sucessfully","raid"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt -pdo r 0_0_0 1100 4",
	    {"your data","Data read into buffer"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
    {
	    "ddt i corrupt 4", /* Invalidate Checksum */
	    {"Checksum invalidation completed sucessfully","corrupt"}, //expected keywords for "ddt"
	    {"Error","failed","ERROR"} //unexpected keywords for "ddt"
    },
#endif
};


/*!*******************************************************************
 * @def FBECLI_DDT_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_DDT_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_DDT_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_DDT_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_DDT_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_DDT_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var fbecli_ddt_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t fbecli_ddt_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] =
#else
fbe_test_rg_configuration_array_t fbecli_ddt_raid_group_config_qual[] =
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* Raid 1 configurations.
     */    /* Raid 1 configurations.
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
 * end fbecli_ddt_raid_group_config_qual()
 **************************************/
/*!**************************************************************
 * fbe_test_load_ddt_config()
 ****************************************************************
 * @brief
 *  Setup for a raid block operation test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *  06/15/2012 - Updated. 
 *
 ****************************************************************/
void fbe_test_load_ddt_config(fbe_test_rg_configuration_array_t* array_p)
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
 * end fbe_test_load_ddt_config()
 ******************************************/

/*!**************************************************************
 * fbe_cli_ddt_setup()
 ****************************************************************
 * @brief
 *  Setup function for ddt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  06/15/2012 - Updated. 
 *
 ****************************************************************/
void fbe_cli_ddt_setup(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_ddt_raid_group_config_qual[0];

    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "ddt" specific test config*/
    fbe_test_load_ddt_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_ddt_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_ddt_test()
 ****************************************************************
 * @brief
 *  Test function for ddt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  06/15/2012 - Updated. 
 *
 ****************************************************************/
void fbe_cli_ddt_test(void)
{
    fbe_u32_t                   raid_type_index = 1;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Qual.
     */
    rg_config_p = &fbecli_ddt_raid_group_config_qual[raid_type_index][0];
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                   raid_type_string_p, rg_config_p->raid_type);
        return;
    }
    raid_group_count = fbe_test_get_rg_count(rg_config_p);

    if (raid_group_count == 0)
    {
        return;
    }
    elmo_create_raid_groups(rg_config_p, FBECLI_DDT_TEST_LUNS_PER_RAID_GROUP, FBECLI_DDT_TEST_CHUNKS_PER_LUN);
    /* run all cli ddt commands*/
    fbe_cli_run_all_cli_cmds(rg_config_p,
                             cli_cmd_and_keywords,
                             FBE_CLI_CMDS_DDT);

    /* Destroy the RAID group and LU configuration. */
    fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);

    return;
}
/******************************************
 * end fbe_cli_ddt_test()
 ******************************************/
/*!**************************************************************
 * void fbe_cli_ddt_teardown()
 ****************************************************************
 * @brief
 *  Teardown function for ddt command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *  06/15/2012 - Updated. 
 *
 ****************************************************************/
void fbe_cli_ddt_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_ddt_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_ddt_test.c
 *************************/

