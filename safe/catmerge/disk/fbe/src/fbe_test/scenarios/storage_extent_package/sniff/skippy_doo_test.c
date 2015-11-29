/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file skippy_doo_test.c
 ***************************************************************************
 *
 * @brief
 *    This file contains tests of simple sniff verify operations.  
 *
 * @version
 *    12/01/2009 - Created. Vicky Guo
 *
 ***************************************************************************/

/*
 * INCLUDE FILES:
 */

#include "mut.h"                                    // define mut framework interfaces and structures
#include "fbe_test_configurations.h"                // define test configuration interfaces and structures
#include "fbe_test_package_config.h"                // define package load/unload interfaces
#include "fbe/fbe_api_discovery_interface.h"        // define discovery interfaces and structures
#include "fbe/fbe_api_provision_drive_interface.h"  // define provision drive interfaces and structures
#include "fbe/fbe_api_sim_server.h"                 // define sim server interfaces
#include "fbe/fbe_api_utils.h"                      // define fbe utility interfaces
#include "sep_utils.h"                              // define sep utility interfaces
#include "fbe_test_common_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_trace_interface.h"

/*
 * NAMED CONSTANTS:
 */

// LU configuration parameters
#define SKIPPY_DOO_LUN_CAPACITY             0x2000  // LU capacity (in blocks)
#define SKIPPY_DOO_CHUNKS_PER_LUN                4													
#define SKIPPY_DOO_LUN_NUMBER                    0  // LU number
#define SKIPPY_DOO_LUNS_PER_RAID_GROUP           1  // number of LUs in RAID group

// RAID group configuration parameters
#define SKIPPY_DOO_RAID_GROUP_CAPACITY      0xE000  // RAID group capacity (in blocks)
#define SKIPPY_DOO_RAID_GROUP_COUNT              1  // number of RAID groups
#define SKIPPY_DOO_RAID_GROUP_ID                 0  // RAID group id
#define SKIPPY_DOO_RAID_GROUP_WIDTH              2  // number of disks in RAID group

// miscellaneous constants
#define SKIPPY_DOO_POLLING_INTERVAL            1000   // polling interval (in ms)
#define SKIPPY_DOO_VERIFY_PASS_WAIT_TIME       120    // max. time (in seconds) to wait for verify pass to complete

#define SKIPPY_DOO_CHUNK_SIZE               0x800
/*
 * GLOBAL DATA:
 */


// information used to configure RAID group
fbe_test_rg_configuration_t skippy_doo_raid_group_configuration_g[] =
{
    {
        SKIPPY_DOO_RAID_GROUP_WIDTH,                    // RAID group width
        SKIPPY_DOO_RAID_GROUP_CAPACITY,                 // RAID group capacity (in blocks)
        FBE_RAID_GROUP_TYPE_RAID1,                      // RAID type
        FBE_CLASS_ID_MIRROR,                            // RAID type's class id
        FBE_BE_BYTES_PER_BLOCK,                         // block size
        SKIPPY_DOO_RAID_GROUP_ID,                       // RAID group id
        0                                               // bandwidth
    },

     /* Terminator.
     */
    { 
        FBE_U32_MAX,
        FBE_U32_MAX,
    },
    
};


/*
 * FORWARD REFERENCES:
 */

// run verify
void
skippy_doo_run_verify
(
    fbe_u32_t                                       in_verify_passes,
    fbe_object_id_t                                 in_pvd_object_id,
    fbe_provision_drive_verify_report_t*            in_verify_report_p
);

// stop verify
void
skippy_doo_stop_verify
(
    fbe_object_id_t                                 in_pvd_object_id
);

// compute total error counts for specified verify results
void
skippy_doo_sum_verify_results
(
    fbe_provision_drive_verify_error_counts_t*      in_verify_results_p,
    fbe_u32_t*                                      out_error_count_p
);

// wait for verify pass completion
void
skippy_doo_wait_for_verify_pass_completion
(
    fbe_object_id_t                                 in_pvd_object_id,
    fbe_u32_t                                       in_timeout_sec 
);

// test sniff on consumed PVD
void skippy_doo_test_consumed_pvd(fbe_test_rg_configuration_t *rg_config_p, void * context_p);

// test sniff on non-consumed PVD
void skippy_doo_test_non_consumed_pvd(void);

// test if the system sniff verify enable/disable works.
void skippy_doo_test_system_sniff_verify(fbe_object_id_t in_pvd_object_id);

void skippy_doo_test_sniff_verify_on_non_pvd(fbe_test_rg_configuration_t *rg_config_p );

// test if the sniff pass count persisted.
void skippy_doo_test_system_reboot(fbe_object_id_t in_pvd_object_id);
/*
 * TEST DESCRIPTION:
 */

char* skippy_doo_short_desc = "Simple Sniff Verify Operations";
char* skippy_doo_long_desc =
    "\n"
    "\n"
    "The Skippy Doo Scenario tests that sniff verify operations can start on a PVD,\n"
    "both consumed and non-consumed extents.\n"
    "\n"
    "It covers the following cases:\n"
    "    - it verifies that sniff verify operations can be started on a consumed PVD\n"
    "    - it verifies that sniff verify operations can be started on a non-consumed PVD\n"
    "\n"
    "Dependencies:\n"
    "    - The PVD must be able to start and stop a sniff verify operation\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 3 SAS drives (PDO-A to PDO-C)\n"
    "    [PP] 3 logical drives (LDO-A to LDO-C)\n"
    "    [SEP] 3 provision drives (PVD-A to PVD-C)\n"
    "    [SEP] 2 virtual drives (VD-A, VD-B)\n"
    "    [SEP] 1 RAID-1 RAID object (RG-A)\n"
    "    [SEP] 1 LU (LU-A)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package configuration\n"
    "    - Create all provision drives (PVD) with an I/O edge attached to a logical drive (LD)\n"
    "    - Create 2 virtual drives (VD-A and VD-B) and attach I/O edges to provision drives (PVD-A and PVD-B)\n"
    "    - Create a 2-drive RAID-1 RAID group object (RG-A) and attach I/O edges to virtual drives (VD-A and VD-B)\n"
    "    - Create a LU (LU-A) with an I/O edge attached to the RAID-1 RAID group object (RG-A)\n"
    "\n"
    "STEP 2: Start Sniff Verify on consumed PVD\n"
    "    - Clear verify report and check that pass/error counters are zero\n"
    "    - Set verify checkpoint to cover the entire exported capacity\n"
    "    - Set verify enable to start sniff verifies\n"
    "    - Wait for a single sniff verify pass to finish\n"
    "    - Validate that verify report reflects completed verify operation\n"
    "\n"
    "STEP 3: Stop Sniff Verify on consumed PVD\n" 
    "    - Clear verify enable to stop sniff verifies\n"
    "    - Validate that verify status indicates that sniffing is disabled\n"
    "\n"
    "STEP 4: Start Sniff Verify on non-consumed PVD\n"
    "    - Clear verify report and check that pass/error counters are zero\n"
    "    - Set verify checkpoint to cover the entire exported capacity\n"
    "    - Set verify enable to start sniff verifies\n"
    "    - Wait for a single sniff verify pass to finish\n"
    "    - Validate that verify report reflects completed verify operation\n"
    "\n"
    "STEP 5: Stop Sniff Verify on non-consumed PVD\n"
    "    - Clear verify enable to stop sniff verifies\n"
    "    - Validate that verify status indicates that sniffing is disabled\n"
    "\n";


/*!****************************************************************************
 *  skippy_doo_setup
 ******************************************************************************
 *
 * @brief
 *    This is the setup function for the Skippy Doo test. It configures all
 *    objects and attaches them into the topology as follows:
 *
 *    Create and verify the initial physical package configuration
 * 
 * @param   void
 *
 * @return  void
 *
 * @author
 *    02/12/2010 - Created. Randy Black
 *
 *****************************************************************************/

void skippy_doo_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    ///////////////////////////////////////////////////////////////////////////
    // STEP 1: Bring up the initial topology
    //
    ///////////////////////////////////////////////////////////////////////////
    if (fbe_test_util_is_simulation()){

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        elmo_physical_config();

        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        elmo_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        sep_config_load_sep_and_neit_both_sps();
        // set trace level
        elmo_set_trace_level( FBE_TRACE_LEVEL_INFO );       
    }

    /* We do not want anything to interfere with sniff rate.
     * If zeros are allowed to run, they will get priority over sniff. 
     */
    fbe_test_sep_util_disable_system_drive_zeroing();
    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}   // end skippy_doo_setup()


/*!****************************************************************************
 *  skippy_doo_test_consumed_pvd
 ******************************************************************************
 *
 * @brief
 *    Steps 2 & 3  - perform sniffing on a consumed PVD
 *
 * @param   
 *
 * @return  void
 *
 *****************************************************************************/
void skippy_doo_test_consumed_pvd(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t                            current_count  = 0;  // counts for current pass    
    fbe_u32_t                            previous_count = 0;  // counts for previous passes
    fbe_u32_t                            total_count    = 0;  // accumulated total counts 
    fbe_provision_drive_verify_report_t  verify_report;       // verify report
    fbe_provision_drive_verify_report_t  verify_report_persist;
    fbe_object_id_t                      skippy_doo_consumed_pvd_id;
    fbe_status_t                         status;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;

	if(! fbe_test_rg_config_is_enabled(rg_config_p)){
		return;
	}

    // get object id for 1st consumed PVD
    status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[0].bus,
                                                             rg_config_p->rg_disk_set[0].enclosure,
                                                             rg_config_p->rg_disk_set[0].slot,
                                                             &skippy_doo_consumed_pvd_id
                                                           );

    // insure that there were no errors
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    /* Validate that setting sniff on wrong objects is handled gracefully */
    skippy_doo_test_sniff_verify_on_non_pvd(rg_config_p);

    /* This test has reboot statement to test the sniff pass count persistence.
        After reboot, the active and passive states are swapped between the two SPs.
        Hence, it is required to identify which SP is active before getting sniff progress.
        NOTE: Sniff only runs on Active SP.
        Get the active SP for this object */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(skippy_doo_consumed_pvd_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        active_sp = other_sp;
    }

    /* Check if we are talking to active SP */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    ///////////////////////////////////////////////////////////////////////////
    // STEP 2: Start sniff verifies on consumed PVD
    //
    // - Clear verify report; check that pass/error counters are zero
    // - Set verify checkpoint to zero to cover the entire exported capacity
    // - Set verify enable to start sniff verifies
    // - Wait for a single sniff verify pass to finish
    // - Check verify report; pass count is 1 and all errors counters are zero
    ///////////////////////////////////////////////////////////////////////////

    // clear verify report for this provision drive
    fbe_test_sep_util_provision_drive_clear_verify_report( skippy_doo_consumed_pvd_id );

    // get verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( skippy_doo_consumed_pvd_id, &verify_report );

    // check that verify pass count is 0
    MUT_ASSERT_TRUE( verify_report.pass_count == 0 );

    // check current verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.current, &current_count );
    MUT_ASSERT_TRUE( current_count == 0 );

    // check previous verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( previous_count == 0 );

    // check total verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == 0 );

    // run a verify pass on this provision drive
    skippy_doo_run_verify( 1, skippy_doo_consumed_pvd_id, &verify_report );

    // check that verify pass count is 1
    MUT_ASSERT_TRUE( verify_report.pass_count == 1 );

    // check current verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.current, &current_count );
    MUT_ASSERT_TRUE( current_count == 0 );

    // check previous verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( previous_count == 0 );

    // check total verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == 0 );

    // check that pass count is persisted after reboot
    skippy_doo_test_system_reboot(skippy_doo_consumed_pvd_id);
    // get verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report(skippy_doo_consumed_pvd_id, &verify_report_persist);
    if(verify_report.pass_count != verify_report_persist.pass_count)
    {
		MUT_FAIL_MSG("Pass count persist failed");
    }

    ///////////////////////////////////////////////////////////////////////////
    // STEP 3: Stop sniff verifies on consumed PVD
    //
    // - Clear verify enable to stop sniff verifies
    // - Check verify status; enable flag indicates sniffing is disabled
    ///////////////////////////////////////////////////////////////////////////

    // stop verify on this provision drive
    skippy_doo_stop_verify( skippy_doo_consumed_pvd_id );
}


/*!****************************************************************************
 *  skippy_doo_test_non_consumed_pvd
 ******************************************************************************
 *
 * @brief
 *    Steps 4 & 5  - perform sniffing on a non-consumed PVD
 *
 * @param   void
 *
 * @return  void
 * 
 *****************************************************************************/
void skippy_doo_test_non_consumed_pvd(void)
{
    fbe_u32_t                            current_count  = 0;  // counts for current pass    
    fbe_u32_t                            previous_count = 0;  // counts for previous passes
    fbe_u32_t                            total_count    = 0;  // accumulated total counts 
    fbe_provision_drive_verify_report_t  verify_report;       // verify report
    fbe_provision_drive_verify_report_t  verify_report_persist;       // verify report
    fbe_test_raid_group_disk_set_t       pvd_disk_data;
    fbe_object_id_t                      skippy_doo_non_consumed_pvd_id;
    fbe_status_t                         status;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_raid_group_disk_set_t          disk_set;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;


    /* fill out  all available drive information
     */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);
    
    /* check if we have atleast one drive with 520 block size
     */
    if(fbe_test_get_520_drive_location(&drive_locations, &disk_set) != FBE_STATUS_OK)
		MUT_FAIL_MSG("Skippy doo scenario: Invalid disk configuration, No disk available with 520 block size");


    /* Get the location for the first available drive 
     */
    pvd_disk_data.bus = disk_set.bus;
    pvd_disk_data.enclosure= disk_set.enclosure;
    pvd_disk_data.slot= disk_set.slot;

    /* make sure that PVD object is in ready state */
    status = fbe_api_provision_drive_get_obj_id_by_location(pvd_disk_data.bus, 
                                                            pvd_disk_data.enclosure, 
                                                            pvd_disk_data.slot, 
                                                            &skippy_doo_non_consumed_pvd_id
                                                            );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_wait_for_object_lifecycle_state(skippy_doo_non_consumed_pvd_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* This test has reboot statement to test the sniff pass count persistence.
        After reboot, the active and passive states are swapped between the two SPs.
        Hence, it is required to identify which SP is active before getting sniff progress.
        NOTE: Sniff only runs on Active SP.
        Get the active SP for this object */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(skippy_doo_non_consumed_pvd_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        active_sp = other_sp;
    }

    /* Check if we are talking to active SP; else set target as active SP */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    ///////////////////////////////////////////////////////////////////////////
    // STEP 4: Start sniff verifies on non-consumed PVD
    //
    // - Clear verify report; check that pass/error counters are zero
    // - Set verify checkpoint to zero to cover the entire exported capacity
    // - Set verify enable to start sniff verifies
    // - Wait for a single sniff verify pass to finish
    // - Check verify report; pass count is 1 and all errors counters are zero
    ///////////////////////////////////////////////////////////////////////////

    // clear verify report for this provision drive
    fbe_test_sep_util_provision_drive_clear_verify_report( skippy_doo_non_consumed_pvd_id );

    // get verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report(skippy_doo_non_consumed_pvd_id , &verify_report );

    // check that verify pass count is 0
    MUT_ASSERT_TRUE( verify_report.pass_count == 0 );

    // check current verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.current, &current_count );
    MUT_ASSERT_TRUE( current_count == 0 );

    // check previous verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( previous_count == 0 );

    // check total verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == 0 );

    // run a verify pass on this provision drive
    skippy_doo_run_verify( 1, skippy_doo_non_consumed_pvd_id, &verify_report );

    // check that verify pass count is 1
    MUT_ASSERT_TRUE( verify_report.pass_count == 1 );

    // check current verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.current, &current_count );
    MUT_ASSERT_TRUE( current_count == 0 );

    // check previous verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( previous_count == 0 );

    // check total verify result counters; should be zero
    skippy_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == 0 );

    // check that pass count is persisted after reboot
    skippy_doo_test_system_reboot(skippy_doo_non_consumed_pvd_id);
    // get verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report(skippy_doo_non_consumed_pvd_id , &verify_report_persist);
    if(verify_report.pass_count != verify_report_persist.pass_count)
    {
		MUT_FAIL_MSG("Pass count persist failed");
    }

    ///////////////////////////////////////////////////////////////////////////
    // STEP 5: Stop sniff verifies on non-consumed PVD
    //
    // - Clear verify enable to stop sniff verifies
    // - Check verify status; enable flag indicates sniffing is disabled
    ///////////////////////////////////////////////////////////////////////////

    // stop verify on this provision drive
    skippy_doo_stop_verify( skippy_doo_non_consumed_pvd_id);
}




/*!****************************************************************************
 *  skippy_doo_test
 ******************************************************************************
 *
 * @brief
 *    This is the main entry point for Skippy Doo Scenario tests. These tests
 *    check  that  sniff verify operations can start and stop on a PVD;  both
 *    consumed and non-consumed extents.
 *
 *    Refer to the test description for more details.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    02/12/2010 - Created. Randy Black
 *
 *****************************************************************************/

void skippy_doo_test(void)
{

    fbe_test_rg_configuration_t *rg_config_p = &skippy_doo_raid_group_configuration_g[0];


    /* Now create the raid groups and run the test
     */
    fbe_test_run_test_on_rg_config(rg_config_p , NULL, skippy_doo_test_consumed_pvd,
                                   SKIPPY_DOO_LUNS_PER_RAID_GROUP,
                                   SKIPPY_DOO_CHUNKS_PER_LUN);
    skippy_doo_test_non_consumed_pvd();

    return;
}   // end skippy_doo_test()


/*!****************************************************************************
 *  skippy_doo_cleanup
 ******************************************************************************
 *
 * @brief
 *    This is the clean-up function for the Skippy Doo test.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    06/22/2010 - Created. Randy Black
 *
 *****************************************************************************/

void skippy_doo_cleanup(void)
{
    if (fbe_test_util_is_simulation()){
        // First execute teardown on SP B then on SP A
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        // First execute teardown on SP A
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();     
    }

    return;
}   // end skippy_doo_cleanup()


/*!****************************************************************************
 *  skippy_doo_run_verify
 ******************************************************************************
 *
 * @brief
 *    This function enables and waits for the selected number of verify passes
 *    to complete on the entire exported capacity of  the  specified provision
 *    drive. After all the verify passes complete, a verify report is returned
 *    to the caller.
 *
 *    The procedure to run one or more verify passes is:
 *
 *    1. Set verify checkpoint to zero to cover the entire exported capacity
 *    2. Set verify enable to start a verify pass
 *    3. For each verify pass, Wait up to 30 seconds for verify pass to finish
 *    4. Get verify report and return it to the caller
 *
 *    It is assumed that the verify report for the specified  provision  drive
 *    has already been cleared before calling this function.
 *
 * @param   in_verify_passes    -  number of verify passes to run
 * @param   in_pvd_object_id    -  provision drive object id
 * @param   in_verify_report_p  -  pointer to verify report
 *
 * @return  void
 *
 * @author
 *    02/12/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
skippy_doo_run_verify(fbe_u32_t                             in_verify_passes,
                      fbe_object_id_t                       in_pvd_object_id,
                      fbe_provision_drive_verify_report_t*  in_verify_report_p)
{
    fbe_lba_t                                checkpoint = 0;  // verify checkpoint
    fbe_u32_t                                pass_count;      // verify pass count
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status
	fbe_status_t                             status;

    /* Disable sniffing on all PVDs so that they do not interfere with the sniffing on this object. 
     * They can interfere since they may get credits while this one does not. 
     */
    fbe_test_sniff_disable_all_pvds();

    if(fbe_test_util_is_simulation())
    {
        /* Let's test that we can sniff a dozen chunks in simulation and then roll over.
         */
        //fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
        //checkpoint = verify_status.exported_capacity - (12 * SKIPPY_DOO_CHUNK_SIZE);
        fbe_test_sep_util_provision_drive_set_verify_checkpoint( in_pvd_object_id, 0);
    }
    else
    {
        // get verify status for this provision drive
        fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
        checkpoint = verify_status.exported_capacity - (1000 * SKIPPY_DOO_CHUNK_SIZE);
        fbe_test_sep_util_provision_drive_set_verify_checkpoint( in_pvd_object_id, checkpoint);
    }
    status = fbe_api_base_config_enable_background_operation(in_pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Before starting the sniff operation disable the disk zeroing.
	 */
	status = fbe_test_sep_util_provision_drive_disable_zeroing(in_pvd_object_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // enable verify on this provision drive
    fbe_test_sep_util_provision_drive_enable_verify( in_pvd_object_id );

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

    // check that verify on provision drive is enabled
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_TRUE );

    // verify started for this provision drive
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies started on object: 0x%x\n", in_pvd_object_id );

    // This function is to test if the system sniff verify enable/disable works.
    skippy_doo_test_system_sniff_verify(in_pvd_object_id);

    // now, wait for the specified number of verify passes to complete
    for ( pass_count = 0; pass_count < in_verify_passes; pass_count++ )
    {
        // wait for verifies to finish on this provision drive
        skippy_doo_wait_for_verify_pass_completion( in_pvd_object_id, SKIPPY_DOO_VERIFY_PASS_WAIT_TIME );
    }

    // get the verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, in_verify_report_p );

    fbe_test_sniff_enable_all_pvds();
    return;
}   // end skippy_doo_run_verify()

/*!****************************************************************************
 *  skippy_doo_test_system_sniff_verify()
 ******************************************************************************
 *
 * @brief
 *    This function tests if the system sniff verify enable/disable works. Since
 *    sniff verify is enable on the particular pvd, we disable the system sniff verify
 *    and wait for 6 seconds, then re-enable the sytem sniff verify, the percentage
 *    verify shouldn't progress/changed.
 *
 * @param   in_pvd_object_id  -  provision drive object id
 *
 * @return  void
 *
 * @author
 *    02/29/2012 - Created. Vera Wang
 *
 *****************************************************************************/
void skippy_doo_test_system_sniff_verify(fbe_object_id_t in_pvd_object_id)
{
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status
	fbe_status_t                             status;
    fbe_bool_t                               verify_enabled;
    fbe_u8_t                                 percentage_completed;  // sniff verify precentage_completed
    fbe_sim_transport_connection_target_t    other_sp;
    fbe_sim_transport_connection_target_t    active_sp;

    mut_printf( MUT_LOG_TEST_STATUS, "===Skippy Doo: Verify System sniff verify enable and disable===\n");

    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: sniff verify %s in ACTIVE\n", 
                (fbe_api_system_sniff_verify_is_enabled() == FBE_TRUE) ? "ENABLED" : "DISABLED");
    active_sp = fbe_api_sim_transport_get_target_server();
    /* Try to get the flag from the other SP and test if update peer succesfully. */
    other_sp = (active_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    fbe_api_sim_transport_set_target_server(other_sp);
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: sniff verify %s in PASSIVE\n", 
                (fbe_api_system_sniff_verify_is_enabled() == FBE_TRUE) ? "ENABLED" : "DISABLED");
    fbe_api_sim_transport_set_target_server(active_sp);

    // wait for a while and check verify 
    fbe_api_sleep(6000);

    // get verify status for this provision drive and make sure it's progressing
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies progress %d on object: 0x%x\n", verify_status.precentage_completed, in_pvd_object_id );

    // wait for a while and check verify 
    fbe_api_sleep(6000);

    // get verify status for this provision drive and make sure it's progressing
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies progress %d on object: 0x%x\n", verify_status.precentage_completed, in_pvd_object_id );


    // disable the system sniff verify flag which will stop the sniff verify on all pvds.
    status = fbe_api_disable_system_sniff_verify();
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: sniff verify %s in ACTIVE\n", 
                (fbe_api_system_sniff_verify_is_enabled() == FBE_TRUE) ? "ENABLED" : "DISABLED");

    /* Sniff verify is now disabled in the active server.
        Since there are reboot statements between calling this function multiple times,
        find which is the active SP after reboot. */
    active_sp = fbe_api_sim_transport_get_target_server();
    /* Try to get the flag from the other SP and test if update peer succesfully. */
    other_sp = (active_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    fbe_api_sim_transport_set_target_server(other_sp);
    verify_enabled = fbe_api_system_sniff_verify_is_enabled();
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: sniff verify %s in PASSIVE\n", 
                (verify_enabled == FBE_TRUE) ? "ENABLED" : "DISABLED");
    MUT_ASSERT_TRUE(verify_enabled == FBE_FALSE);
    fbe_api_sim_transport_set_target_server(active_sp);

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
    percentage_completed = verify_status.precentage_completed;
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies progress %d on object after disable: 0x%x\n", percentage_completed, in_pvd_object_id );

    // wait for a while and check verify doesn't progress any more since system_sniff_verify disabled.
    fbe_api_sleep(6000);

    // get verify status for this provision drive and make sure it's not progressing since global sniff verify is disabled
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies progress %d on object after disable: 0x%x\n", verify_status.precentage_completed, in_pvd_object_id );
    MUT_ASSERT_TRUE(percentage_completed == verify_status.precentage_completed);

    // enable the system sniff verify flag which will enable the sniff verify.
    status = fbe_api_enable_system_sniff_verify();
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: sniff verify %s in ACTIVE\n", 
                (fbe_api_system_sniff_verify_is_enabled() == FBE_TRUE) ? "ENABLED" : "DISABLED");

    // try to get the flag from the other SP and test if update peer succesfully.
    fbe_api_sim_transport_set_target_server(other_sp);
    verify_enabled = fbe_api_system_sniff_verify_is_enabled();
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: sniff verify %s in PASSIVE\n", 
                (verify_enabled == FBE_TRUE) ? "ENABLED" : "DISABLED");
    MUT_ASSERT_TRUE(verify_enabled == FBE_TRUE);
    fbe_api_sim_transport_set_target_server(active_sp);

    // wait for a while and check verify does progress any more since system_sniff_verify re-enabled.
    fbe_api_sleep(6000);

    // get verify status for this provision drive and make sure it continue progressing since global sniff verify is re-enabled.
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies progress %d on object after re-enable: 0x%x\n", verify_status.precentage_completed, in_pvd_object_id );

    // wait for a while and check verify does progress any more since system_sniff_verify re-enabled.
    fbe_api_sleep(6000);

    // get verify status for this provision drive and make sure it continue progressing since global sniff verify is re-enabled.
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies progress %d on object after re-enable: 0x%x\n", verify_status.precentage_completed, in_pvd_object_id );

    MUT_ASSERT_INT_NOT_EQUAL(percentage_completed, verify_status.precentage_completed);
} // end skippy_doo_test_system_sniff_verify()

/*!****************************************************************************
 *  skippy_doo_stop_verify
 ******************************************************************************
 *
 * @brief
 *    This function stops verifies on the specified provision drive.
 *
 * @param   in_pvd_object_id  -  provision drive object id
 *
 * @return  void
 *
 * @author
 *    02/12/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
skippy_doo_stop_verify(fbe_object_id_t in_pvd_object_id)
{
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status

    // disable verify on this provision drive
    fbe_test_sep_util_provision_drive_disable_verify( in_pvd_object_id );

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

    // check that verify on provision drive is disabled
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_FALSE );
    
    // verify stopped for this provision drive
    mut_printf( MUT_LOG_TEST_STATUS, "Skippy Doo: verifies stopped on object: 0x%x\n", in_pvd_object_id );

    return;
}   // end skippy_doo_stop_verify()


/*!****************************************************************************
 *  skippy_doo_sum_verify_results
 ******************************************************************************
 *
 * @brief
 *    This function computes total error counts for specified verify results.
 *
 * @param   in_verify_results_p  -  pointer to verify results
 * @param   out_error_count_p    -  pointer to count of reported errors
 *
 * @return  void
 *
 * @author
 *    02/12/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
skippy_doo_sum_verify_results(fbe_provision_drive_verify_error_counts_t* in_verify_results_p,
                              fbe_u32_t*                                 out_error_count_p)
{
    fbe_u32_t                       count;              // error count

    // sum verify error counters
    count  = in_verify_results_p->recov_read_count;
    count += in_verify_results_p->unrecov_read_count;

    // output count of reported verify errors
    *out_error_count_p = count;

    return;
}   // end skippy_doo_sum_verify_results()


/*!****************************************************************************
 *  skippy_doo_wait_for_verify_pass_completion
 ******************************************************************************
 *
 * @brief
 *    This function waits the specified timeout interval for a verify pass to
 *    complete on a provision drive.  Note that the provision drive is polled
 *    every 100 ms during the timeout interval for a completed verify pass.
 *
 * @param   in_pvd_object_id  -  provision drive to check for a verify pass
 * @param   in_timeout_sec    -  verify pass timeout interval (in seconds)
 *
 * @return  void
 *
 * @author
 *    02/12/2010 - Created. Randy Black
 *
 *****************************************************************************/

void skippy_doo_wait_for_verify_pass_completion(fbe_object_id_t in_pvd_object_id,
                                                fbe_u32_t       in_timeout_sec)
{
    fbe_u32_t                               poll_time;            // poll time
    fbe_u32_t                               verify_pass;          // verify pass to wait for
    fbe_provision_drive_verify_report_t     verify_report = {0};  // verify report

    // get verify report for specified provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

    // determine verify pass to wait for
    verify_pass = verify_report.pass_count + 1;

    // wait specified timeout interval for a verify pass to complete
    for ( poll_time = 0; poll_time < (in_timeout_sec * 10); poll_time++ )
    {
        // get verify report for specified provision drive
        fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

        // finished when next verify pass completes
        if ( verify_report.pass_count == verify_pass )
        {
            // verify pass completed for provision drive
            mut_printf( MUT_LOG_TEST_STATUS,
                        "Skippy Doo: verify pass %d completed for object: 0x%x\n",
                        verify_report.pass_count, in_pvd_object_id
                      );

            return;
        }

        // wait for next poll
        fbe_api_sleep( SKIPPY_DOO_POLLING_INTERVAL );
    }

    // verify pass failed to complete for provision drive
    mut_printf( MUT_LOG_TEST_STATUS,
                "Skippy Doo: verify pass failed to complete in %d seconds for object: 0x%x\n", 
                in_timeout_sec, in_pvd_object_id
              );

    MUT_ASSERT_TRUE( FBE_FALSE );

    return;
}   //end skippy_doo_wait_for_verify_pass_completion()

/*!****************************************************************************
 * skippy_doo_test_sniff_verify_on_non_pvd
 ******************************************************************************
 *
 * @brief
 *    This function tries to enables sniff verifies on non PVD and makes sure
 * job service can handle the failure and roll back errors by issuing another
 * verify right behind it
 *
 * @param   in_provision_drive_object_id  -  provision drive object id
 *
 * @return  fbe_status_t                  -  status of this operation
 *
 * @version
 *    09/21/2012 - Created. Ashok Tamilarasan
 *    
 ******************************************************************************/

void skippy_doo_test_sniff_verify_on_non_pvd(fbe_test_rg_configuration_t *rg_config_p )
{
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_object_id_t                                 lun_object_id_1;
    fbe_object_id_t                                 skippy_doo_consumed_pvd_id;
    fbe_job_service_error_type_t                    js_error_type;

    logical_unit_configuration_p = &(rg_config_p[0].logical_unit_configuration_list[0]);
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id_1);

    fbe_api_job_service_set_pvd_sniff_verify(lun_object_id_1, &job_status, &js_error_type, FBE_TRUE);

    MUT_ASSERT_TRUE_MSG((js_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR), "Configuring PVD to enable sniff verify should not succeed");

    /* we know the previous operation is going to cause errors and so clear it */
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);

     // get object id for 1st consumed PVD
    status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[0].bus,
                                                             rg_config_p->rg_disk_set[0].enclosure,
                                                             rg_config_p->rg_disk_set[0].slot,
                                                             &skippy_doo_consumed_pvd_id
                                                           );

    /* if the previous failure was cleaned up correctly, this should succeed */
    fbe_test_sep_util_provision_drive_enable_verify(skippy_doo_consumed_pvd_id);

    /* Put it back to the original disabled state */
    fbe_test_sep_util_provision_drive_disable_verify(skippy_doo_consumed_pvd_id);

}

/*!****************************************************************************
 * skippy_doo_test_system_reboot
 ******************************************************************************
 *
 * @brief
 *    This function tries to reboot sep and neit package
 *
 * @param   in_pvd_object_id  -  the object id of the pvd we are rebooting for
 *
 * @return	none
 *    
 ******************************************************************************/

void skippy_doo_test_system_reboot(fbe_object_id_t in_pvd_object_id)
{
    fbe_status_t status;
	
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*Verify the PVD object gets to ready state in reasonable time */
	status = fbe_api_wait_for_object_lifecycle_state(in_pvd_object_id, 
													FBE_LIFECYCLE_STATE_READY, 30000,
													FBE_PACKAGE_ID_SEP_0);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end skippy_doo_test_system_reboot()
 ******************************************/
