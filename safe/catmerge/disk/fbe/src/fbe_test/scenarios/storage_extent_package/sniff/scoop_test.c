/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file scoop_test.c
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
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"

#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   TEST DESCRIPTION
 *************************/
char * scoop_short_desc = "Sniff induced remap logic verification - not implemented";
char * scoop_long_desc =
"The Sniff induced remap Scenario (Scoop) tests the ability of the RAID to react to certain type of data events. \n"
"When the unrecoverable media error occurred during Sniff operation of the Provision Drive object it sends data event with problematic LBA. \n"
"The bad LBA is translated by block transport server as event “travels” along the edges. \n"
"When RAID receives the data event it should set Need Rebuild flag for appropriate area. \n"
"The test verifies that this flag is set. The RAID monitor should perform the rebuild of this area and clear the Need Rebuild flag. \n"
"The test verifies that rebuild happened and that the Need Rebuild flag is cleared. \n"
"It covers the following cases: \n"
"	- Provision drive object had a media error during sniff operation. \n"
"The bad block needs to be remapped.  \n"
"It is done by sending data event to the edge which will result in setting NR flag at the RAID level.  \n"
"RAID monitor should check NR flags and rebuild appropriate area.  \n"
"After read part of the rebuild process RAID will send WRITE VERIFY command to the area which covers the bad block.  \n"
"PDO will remap only one block at a time if WRITE VERIFY was not successful,  \n"
"so it is possible that raid will send WRITE VERIFY several times for the same area.  \n"
"Upon successful WRITE VERIFY completion RAID should clear NR flag. The scenario verifies that all remap steps a taken. \n"
"Starting Config: \n"
"        [PP] armada board \n"
"        [PP] SAS PMC port \n"
"        [PP] viper enclosure \n"
"        [PP] 3 SAS drives (PDO-[0..2]) \n"
"        [PP] 3 logical drives (LD-[0..2]) \n"
"        [SEP] 3 provision drives (PVD-[0..2]) \n"
"        [SEP] 3 virtual drives (VD-[0..2]) \n"
"        [SEP] raid object (RAID5 2+1) \n"
"        [SEP] two LUN objects (LUN-0 and LUN-1) \n"
"STEP 1: Bring up the initial topology. \n"
"        - Create and verify the initial physical package config. \n"
"        - Create and verify the initial SEP config. \n"
"STEP 2: Single block failure. \n"
"        - Setup NEIT to fail read from PDO-0 LBA-0 with media error. \n"
"        - Send usurper command to PVD to start sniff manually. \n"
"        - Verify that Metadata service received request to setup NR flag. \n"
"        - Verify that PDO-0 received WRITE VERIFY command. \n"
"        - Verify that Metadata service received request to clear NR flag. \n"
"STEP 3: Multiple block failure \n"
"        - Setup NEIT to fail read from PDO-0 LBA-0 and LBA-1 with media error. \n"
"        - Send usurper command to PVD to start sniff manually. \n"
"        - Verify that Metadata service received request to setup NR flag. \n"
"        - Verify that PDO-0 received WRITE VERIFY command. \n"
"        - Fail this command with LBA-1 as a bad block  \n"
"        - Verify that PDO-0 received WRITE VERIFY command. \n"
"        - Verify that Metadata service received request to clear NR flag.\n"
"----------------COMMENTS-----------------\n"
"1. This test currently is not implemented.\n"
"2. Insert media errors on a random lba rather than just lba 0\n"
"3. Add a test case to insert random multiple media errors across multiple drives\n"
"4. Use various raid types to test sniffer\n"
"5. Add a test case for sniffer detecting errors on a hot spare\n"
"6. Add a test case to rebuild another drive during the sniff operation\n"
"Description last updated: 9/16/2011.\n";

static fbe_object_id_t scoop_test_pvd_object_id = FBE_OBJECT_ID_INVALID;

void scoop_test(void)
{

    return;
}

void scoop_test_init(void)
{
	fbe_status_t status;
    fbe_terminator_api_device_handle_t  port_handle;
    fbe_terminator_api_device_handle_t  encl_handle;
    fbe_terminator_api_device_handle_t  drive_handle;
    
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* insert a board */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port_handle);

	fbe_test_pp_util_insert_viper_enclosure(0, 0, port_handle, &encl_handle);
	fbe_test_pp_util_insert_sas_drive(0, 0, 0, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
	fbe_test_pp_util_insert_sas_drive(0, 0, 1, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY+1, &drive_handle);
	fbe_test_pp_util_insert_sas_drive(0, 0, 2, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY+2, &drive_handle);
	fbe_test_pp_util_insert_sas_drive(0, 0, 3, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY+3, &drive_handle);

    /* Init fbe api on server */
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	status = fbe_api_wait_for_number_of_objects(7, 10000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    /* Load Storage Extent Package */
	sep_config_load_sep();

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 3, &scoop_test_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_wait_for_object_lifecycle_state(scoop_test_pvd_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

void scoop_test_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== scoop destroy starts ===\n");
    fbe_test_sep_util_destroy_sep_physical();
    mut_printf(MUT_LOG_LOW, "=== scoop destroy completes ===\n");

    return;
}

/*************************
 * end file scoop_test.c
 *************************/


