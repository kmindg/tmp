/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file aurora_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a LU ownership test for LU objects.
 *
 * @version
 *   9/14/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"

#include "pp_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_tests.h"



/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   TEST DESCRIPTION
 *************************/
char * aurora_short_desc = "Lun power saving logic verification";
char * aurora_long_desc =
"The LUN Power Saving Scenario (Aurora) tests the ability of the LUN to act according to user defined power saving policy. \n"
"The test also verifies the LUN’s ability to understand that the downstream storage extent is also in power saving mode. \n"
"It covers the following cases: \n"
"- A LUN is instructed to enter into power saving mode after 10 seconds without I/O. The scenario verifies that LUN does not enter  \n"
"        into power saving mode if I/O is sent every 5 seconds.  \n"
"        The scenario is also verifies that LUN does enter into power saving mode after 10 seconds without I/O. \n"
"        - Two LUN’s attached to the RAID instructed to enter into power saving mode after 10 and 20 seconds.  \n"
"        The scenario verifies that after 10 seconds first LUN reports that he is in power saving state,  \n"
"        but downstream storage extent (RAID) is not.  \n"
"        The scenario also verifies that after 20 seconds first LUN that he is in power saving  \n"
"        state and his downstream storage extent is in power saving state. \n"
"Starting Config: \n"
"        [PP] armada board \n"
"        [PP] SAS PMC port \n"
"        [PP] viper enclosure \n"
"        [PP] two SAS drives (PDO-0 and PDO-1)) \n"
"        [PP] two logical drives (LD-0 and LD-1) \n"
"        [SEP] two provision drives (PVD-0 and PVD-1) \n"
"        [SEP] two virtual drives (VD-0 and VD-1) \n"
"        [SEP] raid object (mirror) \n"
"        [SEP] two LUN objects (LUN-0 and LUN-1) \n"
"STEP 1: Bring up the initial topology. \n"
"        - Create and verify the initial physical package config. \n"
"        - Create and verify the initial SEP config. \n"
"STEP 2: Single LUN test. \n"
"        - Set power saving parameters for LUN-0 to start power saving after 10 seconds without I/O. \n"
"        - Send I/O to LUN-0 every 5 seconds. \n"
"        - Verify that LUN-0 does not enter to power saving mode after 15 seconds. \n"
"        - Stop I/O. \n"
"        - Verify that LUN-0 does enter into power saving mode after 10 seconds. \n"
"STEP 3: Actual power saving report test \n"
"        - Set power saving parameters for LUN-0 to start power saving after 10 seconds without I/O. \n"
"        - Set power saving parameters for LUN-1 to start power saving after 20 seconds without I/O \n"
"        - Verify that LUN-0 does enter into power saving mode after 10 seconds. \n"
"        - Verify that LUN-0 does not report actual power saving. \n"
"        - Verify that LUN-1 does enter into power saving mode after 20 seconds. \n"
"        - Verify that both LUN-0 and LUN-1 reports actual power saving.\n";

void aurora_test_init(void)
{
    /* insert a board and some drives */
    elmo_physical_config();

    /* Load Storage Extent Package */
	sep_config_load_sep();

    return;
}

void aurora_test_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Aurora destroy configuration ===\n");
    fbe_test_sep_util_destroy_sep_physical();
    return;
}

void aurora_test(void)
{
    fbe_status_t status;
    fbe_api_provision_drive_calculate_capacity_info_t provision_drive_capacity_info;
    fbe_api_virtual_drive_calculate_capacity_info_t virtual_drive_capacity_info;

	 mut_printf(MUT_LOG_LOW, "=== Aurora test ===\n");

    provision_drive_capacity_info.imported_capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    provision_drive_capacity_info.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    status = fbe_api_provision_drive_calculate_capacity(&provision_drive_capacity_info);

    virtual_drive_capacity_info.imported_capacity = provision_drive_capacity_info.exported_capacity;
    virtual_drive_capacity_info.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    status = fbe_api_virtual_drive_calculate_capacity(&virtual_drive_capacity_info);

	mut_printf(MUT_LOG_LOW, "=== Aurora test Done ===\n");

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*************************
 * end file aurora_test.c
 *************************/


