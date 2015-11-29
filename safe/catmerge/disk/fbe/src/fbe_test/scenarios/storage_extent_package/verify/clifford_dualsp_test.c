/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file clifford_dualsp_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a user background verify test running with dual SP.
 *
 * @version
 *   1/10/2011 - Created. Zhihao Tang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe_test_configurations.h"
#include "clifford_test.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * clifford_dualsp_short_desc = "test LU verify operations with SP failure during verify";
char * clifford_dualsp_long_desc =
    "\n"
    "\n"
    "The Clifford scenario dual SP tests the verify with SP failover event.\n"
    "\n"
    "It covers the following cases:\n"
    "    - It verifies that the lun and raid objects can setup and perform a verify operation with both SPs running.\n"
    "    - It validates the accumulated errors in the LUN verify report.\n"
    "    - It validates the verify can continue on peer SP if the active side fails in the middle of a verify.\n"
    "\n"
    "Dependencies:\n"
    "    - The raid object must support verify type I/Os and any required verify meta-data.\n"
    "    - The lun object must support the user verify command set.\n"
    "    - Persistent meta-data storage.\n"
    "\n"
    "Starting Config:\n"
    "    [PP]  1 fully populated enclosures configured\n"
    "    [SEP] 1 LUN on a raid-0 object with 3 drives\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create all raid objects with I/O edges attached to all VDs.\n"
    "    - Create all lun objects with an I/O edge attached to its RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Write initial data pattern\n"
    "    - Use rdgen to write an initial data pattern to all LUN objects.\n"
    "\n"
    "STEP 3: Inject RAID stripe errors.\n"
    "    - Use rdgen to inject errors one LUN of each RAID object.\n"
    "\n"
    "STEP 4: Initiate read-only verify operation.\n"
    "    - Validate that the LU verify report properly reflects the completed\n"
    "      verify operation.\n"
    "\n"
    "STEP 5: Initiate read-only verify operation and fail the active SP.\n"
    "    - Validate that the LU verify continues on the peer SP and completes.\n"
    "    - Verify that verify report errors.\n"
    "\n"
    ;




/*!*******************************************************************
 * @def CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT   1

/*!*******************************************************************
 * @def CLIFFORD_DUALSP_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define CLIFFORD_DUALSP_TEST_LUNS_PER_RAID_GROUP    1


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static void clifford_dualsp_raid0_init_verify_test(fbe_test_rg_configuration_t * in_current_rg_config_p);
static void clifford_dualsp_raid0_verify_continue_test(fbe_test_rg_configuration_t * in_current_rg_config_p);


/*****************************************
 * GLOBAL DATA
 *****************************************/

/*!*******************************************************************
 * @var clifford_dualsp_disk_set_520
 *********************************************************************
 * @brief This is the disk set for the 520 RG.
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t clifford_dualsp_disk_set_520[] = {
    /* width = 6 */
    {0,0,4}, {0,0,5}, {0,0,6},
    /* terminator */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}
};

/*!*******************************************************************
 * @var clifford_dualsp_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t clifford_dualsp_raid_group_config[CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,         0x32000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
};

/*!*******************************************************************
 * @var fbe_clifford_dualsp_test_context_g
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t fbe_clifford_dualsp_test_context_g[CLIFFORD_DUALSP_TEST_LUNS_PER_RAID_GROUP * CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @def clifford_dualsp_debug_enable_raid_group_types
 *********************************************************************
 * @brief Global array that determines which raid group types should
 *        have debug flags enabled.
 *
 *********************************************************************/
/*! @todo Remove `FBE_RAID_GROUP_TYPE_RAID1' from the line below*/
fbe_raid_group_type_t clifford_dualsp_debug_enable_raid_group_types[CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT] = {0};



/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/


/*!**************************************************************************
 * clifford_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param void
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_dualsp_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &fbe_clifford_dualsp_test_context_g[0];


    /* First fill the raid group with a pattern.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            CLIFFORD_ELEMENT_SIZE);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "clifford background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End clifford_write_background_pattern()


/*!**************************************************************************
 * clifford_write_error_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to one VD in the RAID group.
 * 
 * @param in_object_id - ID of the VD object to write bad data on.
 * @param in_start_lba - start lba where bad data will be written.
 * @param in_size      - number of blocks to write.
 *
 * @return void
 *
 * @author
 *  10/22/2009 - Created. Moe Valois
 *
 ***************************************************************************/
static void clifford_dualsp_write_error_pattern(fbe_object_id_t in_object_id, fbe_lba_t in_start_lba, fbe_u32_t in_size)
{
    fbe_api_rdgen_context_t *context_p = &fbe_clifford_dualsp_test_context_g[0];


    /* Write a clear pattern to the first VD object to generate CRC errors.
     */
    clifford_setup_fill_range_rdgen_test_context(context_p,
                                            in_object_id,
                                            FBE_CLASS_ID_INVALID,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_RDGEN_PATTERN_CLEAR,
                                            in_start_lba,
                                            in_start_lba + in_size - 1,
                                            1);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "clifford write error pattern for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End clifford_write_error_pattern()


/*!**************************************************************************
 * clifford_dualsp_raid0_init_verify_test
 ***************************************************************************
 * @brief
 *  This function initiate a a user RO type verify on a RAID-0 Raid Group.
 * 
 * @param in_current_rg_config_p    - Pointer to current RG
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_dualsp_raid0_init_verify_test(fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_status_t                                    status;
    fbe_object_id_t                                 lun_object_id;              // LUN object to test
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_object_id_t                                 first_vd_object_id;         // First VD object to test
    fbe_lba_t                                       start_lba;                  // where to write bad data
    fbe_u32_t                                       u_crc_count;                // number of uncorrectable CRC blocks
    fbe_u16_t                                       data_disks = 0;
    fbe_lba_t                                       imported_capacity = 0;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_api_raid_group_get_info_t                   raid_group_info;  //  raid group information from "get info"
    fbe_u32_t                                       timeout_ms = 30000;

    /* find number of data disks for this RG. */
    
    status = fbe_test_sep_util_get_raid_class_info(in_current_rg_config_p->raid_type,
                                                   in_current_rg_config_p->class_id,
                                                   in_current_rg_config_p->width,
                                                   in_current_rg_config_p->capacity,
                                                   &data_disks,
                                                   &imported_capacity);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 0, &first_vd_object_id);
    mut_printf(MUT_LOG_TEST_STATUS,"initiate verify on rg_id: 0x%x  rg_obj_id: 0x%x\n", 
               in_current_rg_config_p->raid_group_id, rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, first_vd_object_id);

    /* get raid group info for calculate the lun imported capacity.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Wait for the raid object to become ready
    fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                            timeout_ms, FBE_PACKAGE_ID_SEP_0);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;

    mut_printf(MUT_LOG_TEST_STATUS,"Entering RAID 0 Verify \n");

    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
    
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);

    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    mut_printf(MUT_LOG_TEST_STATUS,"initiate verify on lun number: %d lun_obj_id: 0x%x\n", 
               logical_unit_configuration_p->lun_number, lun_object_id);

    // STEP 3: Inject RAID stripe errors.
    //! @todo Inject errors at the beginning and end of the LUN extent.
    start_lba = 0x2000;
    u_crc_count = 8;

    //clifford_dualsp_write_error_pattern(first_vd_object_id, start_lba, u_crc_count);

    // Write one error on each chunk.
    clifford_dualsp_write_error_pattern(first_vd_object_id, 0x0, 1);
    clifford_dualsp_write_error_pattern(first_vd_object_id, 0x800, 1);
    clifford_dualsp_write_error_pattern(first_vd_object_id, 0x1000, 1);
    clifford_dualsp_write_error_pattern(first_vd_object_id, 0x1800, 1);
    clifford_dualsp_write_error_pattern(first_vd_object_id, 0x2000, 1);
    clifford_dualsp_write_error_pattern(first_vd_object_id, 0x2800, 1);

    // Initialize the verify report data.
    fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

    ///////////////////////////////////////////////////////////////////////////
    // STEP 4: Initiate read-only verify operation.
    // - Validate that the LU verify report properly reflects the completed
    //   verify operation.
    ///////////////////////////////////////////////////////////////////////////
    
    mut_printf(MUT_LOG_TEST_STATUS,"Initiating RO verify for lun %d\n", logical_unit_configuration_p->lun_number);
    fbe_test_sep_util_lun_initiate_verify(lun_object_id, 
                                          FBE_LUN_VERIFY_TYPE_USER_RO,
                                          FBE_TRUE, /* Verify the entire lun */   
                                          FBE_LUN_VERIFY_START_LBA_LUN_START,     
                                          FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN);      

    // Wait until the checkpoint become something valid and non-zero
    while(TRUE) 
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if(raid_group_info.ro_verify_checkpoint && (int)raid_group_info.ro_verify_checkpoint != FBE_LBA_INVALID) 
        {
            break;
        }
        EmcutilSleep(10);
    }

    mut_printf(MUT_LOG_TEST_STATUS,"RO verify checkpoint on active SP: 0x%llx\n",
	       (unsigned long long)raid_group_info.ro_verify_checkpoint);
    return;

}   // End clifford_dualsp_raid0_init_verify_test()


/*!**************************************************************************
 * clifford_dualsp_raid0_verify_continue_test
 ***************************************************************************
 * @brief
 *  This function continues a RAID-0 verify test when active SP dies. 
 * 
 * @param in_current_rg_config_p    - Pointer to current RG
 *
 * @return void
 *
 ***************************************************************************/
static void clifford_dualsp_raid0_verify_continue_test(fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_status_t                                    status;
    fbe_object_id_t                                 rg_object_id;               // RAID object to test
    fbe_api_raid_group_get_info_t                   raid_group_info;            // raid group information from "get info"
    fbe_object_id_t                                 lun_object_id;              // LUN object tested
    fbe_lun_verify_report_t                         verify_report;              // the verify report data
    fbe_u32_t                                       reported_error_count = 0;   // count of verify errors

    //  Verify configuration on SP
    mut_printf(MUT_LOG_TEST_STATUS,"Verifying SP configuration.\n");

    /* get raid group info
     */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS,"continue verify on rg_id: 0x%x  rg_obj_id: 0x%x\n", 
               in_current_rg_config_p->raid_group_id, rg_object_id);
    fbe_api_database_lookup_lun_by_number(in_current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                               &lun_object_id);
    mut_printf(MUT_LOG_TEST_STATUS,"verify lun number: %d lun_obj_id: 0x%x\n", 
               in_current_rg_config_p->logical_unit_configuration_list->lun_number, lun_object_id);

    // Make sure that the peer SP is picking up valid verify checkpoint
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(FBE_LBA_INVALID != raid_group_info.ro_verify_checkpoint);

    // Wait until the checkpoint moves
    while(TRUE) 
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS,
		   "RO verify checkpoint on current SP: 0x%llx",
		   (unsigned long long)raid_group_info.ro_verify_checkpoint);

        if((int)raid_group_info.ro_verify_checkpoint == FBE_LBA_INVALID) 
        {
            mut_printf(MUT_LOG_TEST_STATUS,"verify completed on LUN %d.\n", in_current_rg_config_p->logical_unit_configuration_list->lun_number);

            // Get the verify report for this LUN.
            do {
                EmcutilSleep(100);
                status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
            } while (status != FBE_STATUS_OK);
            
            fbe_test_sep_util_sum_verify_results_data(&verify_report.current, &reported_error_count);

            //! @todo Currently our simulation framework use separate disks for each SP, and they are not synced.
            // This caused verify to not actually taking place on the peer, even though checkpoints have been
            // visited. Following section need to be changed after the simulation framework is updated.
            mut_printf(MUT_LOG_TEST_STATUS,"verify report pass count: %d", verify_report.pass_count);
            mut_printf(MUT_LOG_TEST_STATUS,"verify report reported error count: %d", reported_error_count);
            mut_printf(MUT_LOG_TEST_STATUS,"verify report crc error count: %d", verify_report.current.uncorrectable_multi_bit_crc);

            fbe_test_sep_util_sum_verify_results_data(&verify_report.totals, &reported_error_count);
            mut_printf(MUT_LOG_TEST_STATUS,"verify report total reported error count: %d\n\n", reported_error_count);


            if(verify_report.pass_count != 1)
            {
                mut_printf(MUT_LOG_TEST_STATUS,"verify report was not updated properly.");
            }
   
            break;
        }

        EmcutilSleep(100);
    }

    return;

}   // End clifford_dualsp_raid0_verify_continue_test()


/*!**************************************************************
 * clifford_dualsp_test()
 ****************************************************************
 * @brief
 *  This is the entry point into the Clifford test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_dualsp_test(void)
{
   fbe_test_rg_configuration_t              *rg_config_p = clifford_dualsp_raid_group_config;
   fbe_sim_transport_connection_target_t    active_connection_target    = FBE_SIM_SP_A;
   fbe_sim_transport_connection_target_t    passive_connection_target   = FBE_SIM_SP_B;

   mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Clifford Test ===\n");


   // Set the active SP
   fbe_api_sim_transport_set_target_server(active_connection_target);

   // Set the trace level
   fbe_test_sep_util_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_HIGH);
   
   // Set the debug flags as required
   clifford_set_debug_flags(rg_config_p, FBE_TRUE);

   // Write initial data pattern to all configured LUNs.
   // This is necessary because bind is not working yet.
   clifford_dualsp_write_background_pattern();

   // Initiate a verify test and let it finish on the original side (SPA)
   clifford_dualsp_raid0_init_verify_test(rg_config_p);
   clifford_dualsp_raid0_verify_continue_test(rg_config_p);

   // Initiate a verify test again
   clifford_dualsp_raid0_init_verify_test(rg_config_p);

   // Destroy the configuration on the active side; simulating SP failure
   fbe_api_sim_transport_set_target_server(active_connection_target);
   fbe_test_sep_util_destroy_sep_physical();
   fbe_api_sim_transport_set_target_server(passive_connection_target);
   mut_printf(MUT_LOG_TEST_STATUS,"Destroyed SPA and continue verify on SPB...\n");

   // Continue the verify test on peer SP
   clifford_dualsp_raid0_verify_continue_test(rg_config_p);

   // Repeat the test on the SP again.
   // Initiate a verify test and let it finish on the original side (SPB)
   clifford_dualsp_raid0_init_verify_test(rg_config_p);
   clifford_dualsp_raid0_verify_continue_test(rg_config_p);

   return;

}  // End clifford_dualsp_test()



/*!**************************************************************
 * clifford_dualsp_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the Clifford test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void clifford_dualsp_setup(void)
{
    fbe_lun_number_t            lun_number = 0;
	fbe_cmi_service_get_info_t  spa_cmi_info;
    fbe_object_id_t             rg_object_id;

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    // Load SEP on SPA first
	mut_printf(MUT_LOG_LOW, "=== clifford_dualsp_test Loading Physical Config on SPA ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    // Set SPA to use simulated drive service
    fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY);

    // Load the physical package and create the physical configuration. 
    elmo_physical_config();

    // Load the logical packages. 
    sep_config_load_sep_and_neit();

    // Wait until SPA is ready
	do {
		fbe_api_sleep(100);
		fbe_api_cmi_service_get_info(&spa_cmi_info);

		if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			break;
		}
	} while(TRUE);

    // Load configuration on SPA
	mut_printf(MUT_LOG_LOW, "=== clifford_dualsp_test Loading creating RG & LUN on SPA ===");

    fbe_test_sep_util_fill_raid_group_disk_set(&clifford_dualsp_raid_group_config[0], CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT, clifford_dualsp_disk_set_520, NULL);
    fbe_test_sep_util_fill_raid_group_lun_configuration(&clifford_dualsp_raid_group_config[0],
                                                        CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT,   
                                                        lun_number,         /* first lun number for this RAID group. */
                                                        CLIFFORD_DUALSP_TEST_LUNS_PER_RAID_GROUP,
                                                        0x9000);

    /* Kick off the creation of all the raid group with Logical unit configuration. */
    fbe_test_sep_util_create_raid_group_configuration(&clifford_dualsp_raid_group_config[0], CLIFFORD_DUALSP_TEST_RAID_GROUP_COUNT);

    // Wait RAID object to be ready on SPA
    fbe_api_database_lookup_raid_group_by_number(clifford_dualsp_raid_group_config[0].raid_group_id, &rg_object_id);
    fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);

    EmcutilSleep(10000);

    // Load SEP on SPB
	mut_printf(MUT_LOG_LOW, "=== clifford_dualsp_test Loading Physical Config on SPB ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    // Set SPB to use simulated drive service
    fbe_api_terminator_set_simulated_drive_type(TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY);

    elmo_physical_config();
    sep_config_load_sep_and_neit();

    // Wait RAID object to be ready on SPB
    fbe_api_database_lookup_raid_group_by_number(clifford_dualsp_raid_group_config[0].raid_group_id, &rg_object_id);
    fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);

    // Set connection back to active side
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;

}   // End clifford_dualsp_setup()



/*!****************************************************************************
 * clifford_dualsp_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void clifford_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);


    /* First execute teardown on SP B then on SP A
    */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();

    /* First execute teardown on SP A
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
}// End clifford_dualsp_cleanup()


/*************************
 * end file clifford_dualsp_test.c
 *************************/


