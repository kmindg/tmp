
/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_rdt_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing RDT cli commands
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

char * rdt_test_short_desc = "Test rdt cli commands";
char * rdt_test_long_desc ="\
Rakka\n\
        - is a fictional character in the Japanese animated series Haibane Renmei.\n\
        - Rakka's name means \"falling\" and she is in constant search for herself. \n\
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
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. SPS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Insert SFP into Port 0, Verify SPF status is inserted in ESP\n\
STEP 3: Remove SFP from Port 0, Verify SFP status is removed in ESP\n\
";

/*!*******************************************************************
 * @def FBE_CLI_CMDS_RDT
 *********************************************************************
 * @brief Number of RDT commands to test
 *
 *********************************************************************/
#if 0 /* SAFEBUG - not a good way to set array size - see change below */
#define FBE_CLI_CMDS_RDT           24
#endif

/*!*******************************************************************
 * @def cli_cmd_and_keywords_general
 *********************************************************************
 * @brief RDT commands with keywords that are general
 *
 *********************************************************************/
static fbe_cli_cmd_t cli_cmd_and_keywords_general[] =
{
    /*rdt commands - 7*/
    {
        "rdt",
        {"rdt: ERROR: Too few args."}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -h",
        {"rdt <flags> <operation> <lun decimal> <lba 0x0..capacity> [count 0x0..0x1000]"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"}, 
    },
    {
        "rdt f",
        {"rdt: ERROR: No Buffer Allocated!"},
        {"Buffer Deleted"}
    },
    {
        "rdt -object_id 1 -package_id 10",
        {"FBE-CLI ERROR!  fbe_cli_rdt_parse_cmds unknown package id found for -package_id (10) argument"}, 
        {"Buffer Deleted"} 
    },
    {
        "rdt -p",
        {"rdt: ERROR: Invalid switch."}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
};

/*!*******************************************************************
 * @def cli_cmd_and_keywords
 *********************************************************************
 * @brief RDT commands with keywords
 *
 *********************************************************************/
static fbe_cli_cmd_t cli_cmd_and_keywords[] =
{
    {
        "rdt r 0 0 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt w 0 0 2",
        {"rdt: ERROR: Data must be read into RDT buffer"},/*since fbecli comes for every commad, buffer allocated in earlier cmd is lost{"rdt: ERROR: Saved buffer is too small."}, */
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 1;rdt w 0 0 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -sp w 0 0 2",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -cp r 0 0 2",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS", "rdt: The SEP/PP reported that the RAID Check pattern Succeeded."}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 2;rdt vw 0 0 2",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"},
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 4;rdt d 0 0 4",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"},
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 4;rdt z 0 0 4",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -cz r 0 0 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS", "rdt: The SEP/PP reported that the RAID Check pattern Succeeded."}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed", "rdt: The SEP/PP reported that the RAID Check pattern Failed."} 
    },
    {
        "rdt r 0 0 1;rdt rv 0 0 800",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 1;rdt c 0 0 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 1;rdt v 0 0 800",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 1;rdt ev 0 0 800",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt r 0 0 800;rdt iv 0 0 800",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {   // corrupted LBA 0 was invalidated
        "rdt -d r 0 0 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {   // subsequent LBAs should still be valid
        "rdt -d r 0 1 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -sp wv 0 0 1",
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -object_id 0xfffffpdo -package_id physical r 0 0 1", /*read on pdo object*/
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"}, 
        {"rdt: ERROR: The SEP/PP reported that the RAID op Failed"} 
    },
    {
        "rdt -object_id 0xfffffpdo -package_id physical r 0 0 1", /*read on pdo*/
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"},
        {"rdt: Error: Negotiate info operation failed"}
    },
    {
        "rdt -object_id 0xffffffff -package_id sep r 0 0 1", /*read on raid group object id using objectid and packageid switches*/
        {"rdt: request completed", "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS"},
        {"rdt: Error: Negotiate info operation failed"}
    },
};


/*!*******************************************************************
 * @def FBECLI_RDT_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_RDT_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_RDT_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_RDT_TEST_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_RDT_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_RDT_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var fbecli_rdt_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t fbecli_rdt_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t fbecli_rdt_raid_group_config_qual[] = 
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
 * end fbecli_rdt_raid_group_config_qual()
 **************************************/


/*!**************************************************************
 * fbe_test_load_rdt_config()
 ****************************************************************
 * @brief
 *  Setup for a raid block operation test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *  11/20/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_test_load_rdt_config(fbe_test_rg_configuration_array_t* array_p)
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
 * end fbe_test_load_rdt_config()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rdt_setup()
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
void fbe_cli_rdt_setup(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_rdt_raid_group_config_qual[0];
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "rdt" specific test config*/
    fbe_test_load_rdt_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_rdt_setup()
 ******************************************/



/*!**************************************************************
 * fbe_cli_rdt_test()
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
void fbe_cli_rdt_test(void)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   raid_group_count = 0;
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* run all cli rdt commands the general rdt cli commands first, raid group/type does not matter*/
    fbe_cli_run_all_cli_cmds(&fbecli_rdt_raid_group_config_qual[0][0],
                             cli_cmd_and_keywords_general,
                             sizeof(cli_cmd_and_keywords_general) / sizeof(fbe_cli_cmd_t));

    /* Run the error cases for all raid types supported
     */
    for (raid_type_index = 0; raid_type_index < FBECLI_RDT_RAID_TYPE_COUNT; raid_type_index++)
    {
        /* Qual.
         */
        rg_config_p = &fbecli_rdt_raid_group_config_qual[raid_type_index][0];
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
        elmo_create_raid_groups(rg_config_p, FBECLI_RDT_TEST_LUNS_PER_RAID_GROUP, FBECLI_RDT_TEST_CHUNKS_PER_LUN);
        /* run all cli rdt commands*/
        fbe_cli_run_all_cli_cmds(rg_config_p,
                                 cli_cmd_and_keywords,
                                 sizeof(cli_cmd_and_keywords) / sizeof(fbe_cli_cmd_t));

        /* Destroy the RAID group and LU configuration. */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);

    }
    return;
}
/******************************************
 * end fbe_cli_rdt_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rdt_test()
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
void fbe_cli_rdt_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end fbe_cli_rdt_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_rdt_test.c
 *************************/


