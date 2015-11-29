/***************************************************************************

* Copyright (C) EMC Corporation 2013
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_mminfo_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing mminfo cli commands
 *
 * @ingroup fbe_cli_test
 *
 * @date
 *  11/04/2013 - Created. Jamin
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

#define MMINFO_USAGE_OUTPUT    "mminfo -help"
#define MMINFO_FAILED_OUTPUT    "ERROR"
#define MMINFO_ERROR_OUTPUTS     { "mminfo -help", "ERROR"}

/* Success test case for mminfo.
 * In this test case, we check version header for
 * RG, LUN and pvd. So if these structures are redefined,
 * this test case will fail and we need to modify mminfo fbecli
 */
static fbe_cli_cmd_t mminfo_success_test_cases[] =
{
    {
        "mminfo",
        {MMINFO_USAGE_OUTPUT},
        {"bc_version_header:"},
    },
    /* Test for raid groud */
    {
        /* Dump rg metadata.
         * Check rg version here.
         */
        "mminfo -object_id 0x10",
        {
            "Object: 0x10, Type: parity",
            "bc_version_header:      0x00000050",
            "blocks_rebuilt_1",
        },
        MMINFO_ERROR_OUTPUTS
    },
    {
        /* Set rg metadata */
        "mminfo -object_id 0x10 -blocks_rebuilt_1 0x55",
        {
            "blocks_rebuilt_1:       0x0000000000000055",
        },
        MMINFO_ERROR_OUTPUTS
    },

    /* Test for lun */
    {
        /* Dump LUN metadata.
         * Check LUN version here.
         */
        "mminfo -object_id 0x30",
        {
            "Object: 0x30, Type: lun",
            "bc_version_header:      0x00000030",
            "lun_flags"
        },
        MMINFO_ERROR_OUTPUTS,
    },
    {
        /* Set LUN metadata */
        "mminfo -object_id 0x30 -lun_flags 0x55",
        {
            "lun_flags:              0x0000000000000055",
        },
        MMINFO_ERROR_OUTPUTS,
    },

    /* Test for pvd */
    {
        /* Dump pvd metadata.
         * Check PVD version here.
         */
        "mminfo -object_id 0x1",
        {
            "Object: 0x1, Type: provisioned_drive",
            "bc_version_header:      0x00000040",
            "slot_number:            0x00000000",
        },
        MMINFO_ERROR_OUTPUTS
    },
    {
        /* Set pvd metadata */
        "mminfo -object_id 0x1 -slot_number 0x55",
        {
            "slot_number:            0x00000055"
        },
        MMINFO_ERROR_OUTPUTS
    },

    {
        NULL,{NULL}, {NULL},
    },
};

static fbe_cli_cmd_t mminfo_failed_test_cases[] =
{
    {
        /* Set lun flags to rg */
        "mminfo -object_id 0x10 -lun_flags 0x55",
        {
            MMINFO_FAILED_OUTPUT,
            "Invalid field",
        },
        {
            "bc_version_header:"
        },
    },
    {
        /* Set pvd flags to rg */
        "mminfo -object_id 0x10 -pvd_flags 0x55",
        {
            MMINFO_FAILED_OUTPUT,
            "Invalid field",
        },
        {
            "bc_version_header:"
        },
    },
    {
        /* Set rg flags to lun */
        "mminfo -object_id 0x30 -rg_flags 0x55",
        {
            MMINFO_FAILED_OUTPUT,
            "Invalid field",
        },
        {
            "bc_version_header:"
        },
    },
    {
        /* Set pvd flags to lun */
        "mminfo -object_id 0x30 -pvd_flags 0x55",
        {
            MMINFO_FAILED_OUTPUT,
            "Invalid field",
        },
        {
            "bc_version_header:"
        },
    },
    {
        /* Set rg flags to pvd */
        "mminfo -object_id 0x1 -rg_flags 0x55",
        {
            MMINFO_FAILED_OUTPUT,
            "Invalid field",
        },
        {
            "bc_version_header:"
        },
    },
    {
        /* Set lun flags to pvd */
        "mminfo -object_id 0x1 -lun_flags 0x55",
        {
            MMINFO_FAILED_OUTPUT,
            "Invalid field",
        },
        {
            "bc_version_header:"
        },
    },
    {
        NULL,{NULL}, {NULL},
    },
};

static fbe_test_rg_configuration_t mminfo_rg_configuration[] = {
    {
        /* width, capacity,        raid type,                        class,               block size, RAID-id,                        bandwidth.*/
        3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,        0, 0,
        /* rg_disk_set */
        {{0,0,4}, {0,0,5}, {0,0,6}}
    },
};

static fbe_cli_cmd_t *mminfo_test_cases[] = {
    &mminfo_success_test_cases[0],
    &mminfo_failed_test_cases[0],
};

/*!**************************************************************
 * fbe_cli_mminfo_setup()
 ****************************************************************
 * @brief
 *  Setup function for mminfo command execution.
 *
 * @param None.
 *
 * @return None.
 *
 * @revision
 *  11/04/2013 - Created. Jamin
 *
 ****************************************************************/
void fbe_cli_mminfo_setup(void)
{
    shrek_test_setup();

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);
    return;
}
/******************************************
 * end fbe_cli_luninfo_setup()
 ******************************************/

static void fbe_cli_mminfo_run_test_cases(fbe_cli_cmd_t *test_cases)
{
    fbe_u32_t count;

    count = fbe_test_cli_cmd_get_count(test_cases);
    fbe_cli_run_all_cli_cmds(mminfo_rg_configuration, test_cases, count);
}
/*!**************************************************************
 * fbe_cli_mminfo_test()
 ****************************************************************
 * @brief
 *  Test function for mminfo command execution.
 *
 * @param None.
 *
 * @return None.
 *
 * @revision
 *  11/04/2013 - Created. Jamin
 *
 ****************************************************************/
void fbe_cli_mminfo_test(void)
{
    fbe_u32_t i;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Start mminfo test ===\n");
    for (i = 0; i < sizeof(mminfo_test_cases) / sizeof(mminfo_test_cases[0]); i++) {
        fbe_cli_mminfo_run_test_cases(mminfo_test_cases[i]);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "=== End mminfo test ===\n");

    return;
}

/******************************************
 * end fbe_cli_mminfo_test()
 ******************************************/

/*!**************************************************************
 * fbe_cli_mminfo_test()
 ****************************************************************
 * @brief
 *  Teardown function for luninfo command execution.
 *
 * @param None.
 *
 * @return None.
 *
 * @revision
 *  11/04/2013 - Created. Jamin
 *
 ****************************************************************/
void fbe_cli_mminfo_teardown(void)
{
    shrek_test_cleanup();

    return;
}
/******************************************
 * end fbe_cli_mminfo_teardown()
 ******************************************/

/*************************
 * end file fbe_cli_mminfo_test.c
 *************************/
