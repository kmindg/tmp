/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file boots_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of differenial rebuild.  
 *
 * @version
 *   12/01/2009 - Created. Jean Montes
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "sep_rebuild_utils.h"                          //  .h shared between Dora and other NR/rebuild tests
#include "mut.h"                                        //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                              //  general types
#include "sep_tests.h"                                  //  for dora_setup()
#include "fbe/fbe_api_rdgen_interface.h"                //  for fbe_api_rdgen_context_t
#include "fbe_test_configurations.h"                    //  for Diego public functions 
#include "fbe/fbe_api_database_interface.h"             //  for fbe_api_database_lookup_raid_group_by_number
#include "fbe/fbe_api_provision_drive_interface.h"      //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_physical_drive_interface.h"       //  for fbe_api_get_physical_drive_object_id_by_location
#include "fbe/fbe_api_utils.h"                          //  for fbe_api_wait_for_object_lifecycle_state
#include "neit_utils.h"                                 //  for fbe_test_neit_utils_init_error_injection_record
#include "fbe/fbe_protocol_error_injection_service.h"   //  for fbe_protocol_error_injection_error_record_t 
#include "fbe/fbe_api_protocol_error_injection_interface.h" //  for fbe_api_protocol_error_injection_*
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_discovery_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* boots_short_desc = "Differential Rebuild";
char* boots_long_desc = "\
\n\
\n\
The Boots Scenario is a test of differential rebuild.  It tests that rebuilds\n\
are performed correctly when a drive is removed and that same drive is later\n\
re-inserted, with or without write I/O performed while the drive is removed.\n"\
"\n\
-raid_test_level 0 and 1\n\
   - Raid-6 and 3 drive Raid-1 are only tested at -rtl 1.\n\
\n\
Part I - Test removing a drive and reinserting the same drive\n\
\n\
STEP 1: Bring up the initial topology\n\
\n\
STEP 2: Write an lba seeded pattern to the drives.\n\
\n\
STEP 3: Remove a drive (drive A)\n\
   - Note that for Raid-6, Raid-10 and 3 drive Raid-1 we remove 2 drives.\n\
      - This step has an issue since it is supposed to remove them together, but removes\n\
        them separately.\n\
   - R1 and R5 remove 1 drives.\n\
   - Verify that VD-A is now in the FAIL state \n"\
"\n\
STEP 4: Verify rebuild logging is started for the failed drive\n\
\n\
STEP 5: Write new data with a zeroed pattern to all of the LUNs\n\
   - Use rdgen to write a seed pattern to each LUN\n\
   - Wait for writes to complete before proceeding\n\
\n\
STEP 6: Reinsert same drive(s) (drive A)\n\
   - Verify that VD(s) are now in the READY state\n\
   - Verify that the RAID object stops rb logging\n\
\n\
STEP 7: Verify progress of the rebuild operation\n\
   - Verify that the RAID object updates the rebuild checkpoint as needed\n\
   - Verify that the RAID object updates its rebuild bitmap as each chunk\n\
     completes\n"\
"\n\
STEP 8: Verify completion of the rebuild operation\n\
   - Verify that the Needs Rebuild chunk count is 0\n\
   - Verify that the rebuild checkpoint is set to LOGICAL_EXTENT_END\n"\
"\n\
STEP 9: Verify that the rebuilt data is correct\n\
   - Use rdgen to validate that the rebuilt data is correct\n\
\n\
STEP 10: For Raid-5 only we run another test where we repeat steps 1-9 above, but skip sending I/O.\n\
   - This tests rebuild logging getting set and cleared without I/O.\n\
\n\
STEP 11: For Raid-6, Raid-10 and 3 drive Raid-1, we run 2 additional variations.\n\
   1) Remove 2 drives separately.  So we start rebuild logging on the first,\n\
      then the second drive gets inserted.\n\
   2) Remove the drives together, but insert second removed drive first.\n\
\n\
STEP 12: Cleanup\n\
   - Destroy objects\n\
\n\
Description last updated: 10/31/2011.\n";


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//  Context for rdgen.  This is an array with an entry for every LUN defined by the test.  The context is used
//  to initialize all the LUNs of the test in a single operation. 
static fbe_api_rdgen_context_t fbe_boots_test_context_g[SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP * SEP_REBUILD_UTILS_NUM_RAID_GROUPS];

extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

extern fbe_test_rg_configuration_t dora_r5_rg_config_g[];
extern fbe_test_rg_configuration_t dora_r1_rg_config_g[];
extern fbe_test_rg_configuration_t dora_r10_rg_config_g[];
extern fbe_test_rg_configuration_t dora_triple_mirror_rg_config_g[];
extern fbe_test_rg_configuration_t dora_triple_mirror_2_rg_config_g[];
extern fbe_test_rg_configuration_t dora_r6_rg_config_g[]; 
extern fbe_test_rg_configuration_t dora_db_drives_rg_config_g;
extern fbe_test_rg_configuration_t dora_dummy_rg_config_g;

extern fbe_test_raid_group_disk_set_t fbe_dora_db_drives_disk_set_g[SEP_REBUILD_UTILS_DB_DRIVES_RAID_GROUP_WIDTH];
extern fbe_test_raid_group_disk_set_t fbe_dora_dummy_disk_set_g[SEP_REBUILD_UTILS_DUMMY_RAID_GROUP_WIDTH];


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:


//  Perform all tests on a RAID-5 or RAID-3
static void boots_test_main_r5_r3(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Perform all tests on a RAID-1
static void boots_test_main_r1(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        void* context_p);

//  Utility function to perform tests via drive removal for R5/R3/R1 
static void boots_test_r5_r3_r1_via_drive_removal(
                                        fbe_test_rg_configuration_t*        in_rg_config_p, 
                                        fbe_lba_t                           in_start_lba);

//  Check the data for a RAID-1 after a rebuild 
static void boots_test_r1_check_data_after_rebuild(
                                        fbe_test_rg_configuration_t*        in_rg_config_p, 
                                        fbe_lba_t                           in_start_lba);

//  Perform test on a database drive that contains a user RG 
static void boots_test_db_drives(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        fbe_lun_number_t*                   io_next_lun_num_p);

//  Perform all tests on a RAID-6 or RAID-10
static void boots_test_main_r6_r10(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        void* context_p);

//  Test RAID-6 or RAID-10 rebuild when two drives are removed at the same time
static void boots_test_r6_r10_remove_drives_together(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Test RAID-6 or RAID-10 rebuild when two drives are removed at different times
static void boots_test_r6_r10_remove_drives_separately(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Test RAID-6 or RAID-10 rebuild when two drives are removed and reinserted at different times  
static void boots_test_r6_r10_reinsert_drives_separately(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Perform all tests on a triple mirror RAID-1
static void boots_test_main_triple_mirror(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,
                                        void* context_p);

//  Test triple mirror rebuild when two drives are removed at the same time
static void boots_test_triple_mirror_remove_drives_together(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Test triple mirror rebuild when two drives are removed at different times
static void boots_test_triple_mirror_remove_drives_separately(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Test triple mirror rebuild when two drives are removed and reinserted at different times  
static void boots_test_triple_mirror_reinsert_drives_separately(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

//  Restore a triple mirror RG after positions 0 and 1 have been removed 
static void boots_restore_triple_mirror_rg(
                                        fbe_test_rg_configuration_t*        in_rg_config_p,    
                                        fbe_api_terminator_device_handle_t* in_drive_info_1_p,
                                        fbe_api_terminator_device_handle_t* in_drive_info_2_p);

//  Test when no I/O performed while rb logging
static void boots_test_when_no_io(
                                        fbe_test_rg_configuration_t*        in_rg_config_p);

// Run raid 5 tests
static void boots_run_raid5_tests(fbe_test_rg_configuration_t* config_p, void* context_p);


//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  boots_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Boots test.  The test does the 
 *   following:
 *
 *   - Creates raid groups as needed 
 *   - For each raid group, tests differential rebuild of one or more drives
 *   - Tests the case where a differential rebuild has no data to be rebuilt
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void boots_test(void)
{
    fbe_lun_number_t        next_lun_id;        //  sets/gets number of the next lun (0, 1, 2)
    fbe_test_rg_configuration_t *rg_config_p;   // pointer to the RG test config specs

    //  Increase the hot spare timer so that hot spare does not get swap-in.  
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1800);  
    

    //  Initialize configuration setup parameters - start with 0
    next_lun_id = 0; 


    //------------------------------------------------------------------------
    //  Test RAID-5 
    //

    rg_config_p = &dora_r5_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 
                                   NULL, 
                                   boots_run_raid5_tests,
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);



    //--------------------------------------------------------------------------
    //  Test RAID-1 2-way mirror 
    //

    rg_config_p = &dora_r1_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 
                                   NULL, 
                                   boots_test_main_r1,
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);


    //--------------------------------------------------------------------------
    //  Test RAID-10
    //

    rg_config_p = &dora_r10_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 
                                   NULL, 
                                   boots_test_main_r6_r10,
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);


    //------------------------------------------------------------------------
    //  Test rebuild on the database drives with a user RG configured 
    //

/*!@ todo - Temporarily removed to avoid issues on hardware.  7/26/10 JMM. 

    //  Set up a raid group on the DB drives 
    dora_setup_raid_group(&dora_db_drives_rg_config_g, &fbe_dora_db_drives_disk_set_g[0], &next_lun_id);

    //  Perform all tests on the system drives
    boots_test_db_drives(&dora_db_drives_rg_config_g, &next_lun_id);
*/

    //--------------------------------------------------------------------------
    //  Test RAID-6 
    //

    rg_config_p = &dora_r6_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 
                                   NULL, 
                                   boots_test_main_r6_r10,
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);

    //--------------------------------------------------------------------------
    //  Test RAID-1 3-way mirror
    //

    rg_config_p = &dora_triple_mirror_rg_config_g[0];
    fbe_test_run_test_on_rg_config(rg_config_p, 
                                   NULL, 
                                   boots_test_main_triple_mirror,
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN);



    //  Return 
    return;

} //  End boots_test()


/*!****************************************************************************
 *  boots_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Boots test.  It creates the  
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void boots_setup(void)
{  
	sep_rebuild_utils_setup();
} //  End boots_setup()


/*!****************************************************************************
 *  boots_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the common teardown function for the boots test.  It destroys the  
 *   physical configuration and unloads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void boots_cleanup(void)
{  
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();   
    }

    //  Return
    return;
} //  End boots_setup()

/*!****************************************************************************
 *  boots_test_main_r5_r3
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for R5 and R3 RAID groups.  It does the 
 *   following:
 *
 *   - Removes a drive, performs some I/O, reinserts the drive, and waits for
 *     the rebuild to complete 
 *   - Verifies that the data is correct
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_main_r5_r3(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_lba_t                           start_lba;                     // lba at which to start I/Os


    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Set up the starting LBA for the I/O
    start_lba = 0x800;

    //  Remove the drive, reinsert it, and wait for the rebuild to complete  
    boots_test_r5_r3_r1_via_drive_removal(in_rg_config_p, start_lba);

    //  Verify the new data 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Return 
    return;

} //  End boots_test_main_r5_r3()


/*!****************************************************************************
 *  boots_test_main_r1
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a R1 RAID group.  It does the 
 *   following:
 *
 *   - Removes a drive, performs some I/O, reinserts the drive, and waits for
 *     the rebuild to complete 
 *   - Verifies that the data is correct 
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * 
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_main_r1(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* context_p)
{

    fbe_lba_t                           start_lba;                     // lba at which to start I/Os
    fbe_status_t                        status;

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Set up the starting LBA for the I/O
    start_lba = 0x800;

    //  Remove the drive, reinsert it, and wait for the rebuild to complete  
    boots_test_r5_r3_r1_via_drive_removal(in_rg_config_p, start_lba);

    //  Verify the new data 
    boots_test_r1_check_data_after_rebuild(in_rg_config_p, start_lba);

    //  Return 
    return;    

} //  End boots_test_main_r1()


/*!****************************************************************************
 *  boots_test_r1_check_data_after_rebuild
 ******************************************************************************
 *
 * @brief
 *   This function checks the data in a RAID-1 after a rebuild.  It does the
 *   following:
 *
 *   - Removes the drive in the primary position in the RG.  This forces it
 *     to read from the secondary drive.
 *   - Reads from the secondary drive and verifies that the data is correct 
 * 
 *   - Writes some more I/Os when the primary drive is out 
 *   - Reinserts the primary drive so it will rebuild
 *   - Reads (from the primary drive) and verifies that the data is correct 
 * 
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_start_lba          - lba at which I/O was started   
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_r1_check_data_after_rebuild(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, 
                                        fbe_lba_t                       in_start_lba)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   // drive info needed for reinsertion 
    fbe_lba_t                           start_lba_2;                    // lba at which to start second set of I/Os
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove the drive in position 0 in the RG.  This will force the read to occur from position 1 which
    //  is the drive we rebuilt.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_1);

    //  Verify the new data 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], in_start_lba);

    //  Write new data to a few different LBAs so that we can test the rebuild on position 0
    start_lba_2 = in_start_lba + 0x600; 
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Reinsert the drive in position 0 so it will rebuild
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_1);

    //  Wait until the rebuild finishes 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Verify all of the new data 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], in_start_lba);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Return 
    return;

} //  End boots_test_r1_check_data_after_rebuild()


/*!****************************************************************************
 *  boots_test_r5_r3_r1_via_drive_removal
 ******************************************************************************
 *
 * @brief
 *   This is a utility function for testing differential rebuild for R5, R3, and 
 *   R1 RAID groups by removing a drive and re-inserting it.  It does the 
 *   following:
 *
 *   - Writes a seed pattern 
 *   - Removes a drive
 *   - Writes new data to a small number of LBAs
 *   - Reinserts the drive 
 *   - Waits for the rebuild to finish
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_start_lba          - lba at which to start I/O  
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_r5_r3_r1_via_drive_removal(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        fbe_lba_t                       in_start_lba)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and rb logging starts.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Write new data to a few LBAs
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], in_start_lba);

    //  Reinsert the drive 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Wait until the rebuild finishes
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Return 
    return;

} //  End boots_test_r5_r3_r1_via_drive_removal()


/*!****************************************************************************
 *  boots_test_db_drives
 ******************************************************************************
 *
 * @brief
 *   This is the test function for differentially rebuilding a database drive.  
 *   It does the following:
 *
 *   - Writes a seed pattern to the user RG on the drives
 *   - Removes a drive
 *   - Writes new data to a small number of LBAs in the user RG
 *   - Creates a new RG so that the configuration LUN in the system RG is written to 
 *   - Reinserts the drive 
 *   - Waits for the rebuilds of the system RG and the user RG to finish
 *   - Verifies that the data is correct on the user RG
 * 
 * @param in_rg_config_p    - pointer to the RG configuration info
 * @param io_next_lun_num_p - pointer to the next LUN number to use; gets set to 
 *                            the next one available after a dummp RG is created 
 * @return  None 
 *
 *****************************************************************************/
#if 0 // unused code
static void boots_test_db_drives(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, 
                                        fbe_lun_number_t*               io_next_lun_num_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   // drive info needed for reinsertion
    fbe_lba_t                           start_lba;                      // lba at which to start I/Os
    fbe_object_id_t                     system_raid_group_id;
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;
    
    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Set up the starting LBA for the I/O
    start_lba = 0x600;

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and rb logging starts.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    // Get the id of the system raid group
    fbe_api_database_get_system_object_id(&system_raid_group_id);
    
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  For the system RG, verify that rb logging is started and that the rebuild checkpoint is 0
    sep_rebuild_utils_verify_rb_logging_by_object_id(system_raid_group_id, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    //  Write new data to a few LBAs in the user RG 
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Create a raid group on another set of disks, which will cause writes to the configuration LUN  
    sep_rebuild_utils_setup_raid_group(&dora_dummy_rg_config_g, &fbe_dora_dummy_disk_set_g[0], io_next_lun_num_p);

    //  Reinsert the drive 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Wait until the rebuild finishes for each of the RGs
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    sep_rebuild_utils_wait_for_rb_comp_by_obj_id(system_raid_group_id, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(system_raid_group_id);
    sep_rebuild_utils_check_bits(raid_group_id);

    //  Verify the data on the user RG 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Return 
    return;

} //  End boots_test_db_drives()
#endif // unused code


/*!****************************************************************************
 *  boots_test_main_r6_r10
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for RAID-6 and RAID-10 raid groups.  It 
 *   does the following:
 *
 *   - Tests differential rebuild when two drives are removed at the same time
 *     and re-inserted at the same time 
 *   - Tests differential rebuild when one drive is removed, then a second 
 *     drive is removed, and they are re-inserted at the same time 
 *   - Tests differential rebuild when two drives are removed and reinserted 
 *     at different times
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_main_r6_r10(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* context_p)
{
    fbe_u32_t                       test_level;                     //  raid/extended test level 
    fbe_status_t                    status;

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test rebuild when two drives are removed at the same time
    boots_test_r6_r10_remove_drives_together(in_rg_config_p);

    //  Get the extended test level 
    test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    //  If test level is 0, ie it is not extended, then we are done - return here 
    if (test_level == 0)
    {
        return; 
    }

    //  Test rebuild when two drives are removed but at different times
    boots_test_r6_r10_remove_drives_separately(in_rg_config_p);

    //  Test rebuild when two drives are removed and reinserted at different times
    boots_test_r6_r10_reinsert_drives_separately(in_rg_config_p);

    //  Return 
    return;

} //  End boots_test_main_r6()


/*!****************************************************************************
 *  boots_test_r6_r10_remove_drives_together
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a RAID-6 or RAID-10 RAID 
 *   group when two drives are removed at the same time and reinserted at the 
 *   same time.  It does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes two drives in the RG
 *   - Writes new data to a small number of LBAs
 *   - Reinserts both drives 
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the new data is correct 
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_r6_r10_remove_drives_together(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t  drive_info_2;                   //  drive info needed for reinsertion 
    fbe_lba_t                           start_lba;                      //  lba at which to start I/Os
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Set up the starting LBA for the I/O
    start_lba = 0x900;

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove two drives in the RG.  Check the object states change correctly and that rb logging starts. 
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, &drive_info_1);
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Write new data to a few LBAs
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Reinsert both the drives 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, &drive_info_1);
    sep_rebuild_utils_triple_mirror_r6_second_drive_restored(in_rg_config_p, &drive_info_2);

    //  Wait until the rebuilds finish 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Read the data and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Return 
    return;

} //  End boots_test_r6_r10_remove_drives_together()


/*!****************************************************************************
 *  boots_test_r6_r10_remove_drives_separately
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a RAID-6 or RAID-10 RAID 
 *   group when one drive is removed first and another is later removed (while 
 *   the first drive is still out).  It does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in position 0 in the RG
 *   - Writes new data to a small number of LBAs
 *   - Removes the drive in position 2 in the RG
 *   - Writes new data to a small number of different LBAs
 *   - Reinserts both drives 
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct 
 * 
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_r6_r10_remove_drives_separately(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t  drive_info_2;                   //  drive info needed for reinsertion 
    fbe_lba_t                           start_lba_1;                    //  lba at which to start 1st set of I/Os
    fbe_lba_t                           start_lba_2;                    //  lba at which to start 2nd set of I/Os
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and that 
    // rb logging starts.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, &drive_info_1);

    //  Write new data to a few LBAs
    start_lba_1 = 0x1500;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);

    //  Remove a second drive in the RG 
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Write new data to a few LBAs
    start_lba_2 = 0x500;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Reinsert both the drives 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, &drive_info_1);
    sep_rebuild_utils_triple_mirror_r6_second_drive_restored(in_rg_config_p, &drive_info_2);

    //  Wait until the rebuilds finish 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Read the data and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Return 
    return;

} //  End boots_test_r6_r10_remove_drives_separately()


/*!****************************************************************************
 *  boots_test_r6_r10_reinsert_drives_separately
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a RAID-6 or RAID-10 RAID 
 *   group when two drives are removed separately and reinserted separately with 
 *   overlap.  It does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in position 0 in the RG
 *   - Writes new data to a small number of LBAs
 *   - Removes the drive in position 2 in the RG
 *   - Writes new data to a small number of different LBAs
 *   - Reinserts the drive in position 2
 *   - Waits for the rebuild of the position 2 to finish
 *   - Writes new data to a small number of different LBAs
 *   - Reinserts the drive in position 0
 *   - Waits for the rebuild of the position 0 to finish
 *   - Verifies that the data is correct 
 * 
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_r6_r10_reinsert_drives_separately(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t  drive_info_2;                   //  drive info needed for reinsertion 
    fbe_lba_t                           start_lba_1;                    //  lba at which to start 1st set of I/Os
    fbe_lba_t                           start_lba_2;                    //  lba at which to start 2nd set of I/Os
    fbe_lba_t                           start_lba_3;                    //  lba at which to start 3rd set of I/Os
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and that
    //  rb logging starts
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, &drive_info_1);

    //  Write new data to a few LBAs
    start_lba_1 = 0x1000;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);

    //  Remove a second drive in the RG
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Write new data to a few LBAs
    start_lba_2 = 0;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Reinsert the second drive that we removed 
    sep_rebuild_utils_triple_mirror_r6_second_drive_restored(in_rg_config_p, &drive_info_2);

    //  Wait until the rebuild finishes 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    //  Write new data to a few LBAs
    start_lba_3 = 0x500;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_3);

    //  Reinsert the first drive that we removed 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, &drive_info_1);

    //  Wait until the rebuild finishes 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Read the data and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_3);

    //  Return 
    return;

} //  End boots_test_r6_r10_reinsert_drives_separately()


/*!****************************************************************************
 *  boots_test_main_triple_mirror
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for triple mirror raid groups.  It does the 
 *   following:
 *
 *   - Tests differential rebuild when two drives are removed at the same time
 *     and re-inserted at the same time 
 *   - Tests differential rebuild when one drive is removed, then a second 
 *     drive is removed, and they are re-inserted at the same time 
 *   - Tests differential rebuild when two drives are removed and reinserted 
 *     at different times
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_main_triple_mirror(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* context_p)
{
    fbe_u32_t                       test_level;                     //  raid/extended test level 
    fbe_status_t                    status;

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test triple mirror rebuild when two drives are removed at the same time
    boots_test_triple_mirror_remove_drives_together(in_rg_config_p);

    //  Get the extended test level 
    test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    //  If test level is 0, ie it is not extended, then we are done - return here 
    if (test_level == 0)
    {
        return; 
    }

    //  Test triple mirror rebuild when two drives are removed but at different times
    boots_test_triple_mirror_remove_drives_separately(in_rg_config_p);

    //  Test triple mirror rebuild when two drives are removed and reinserted at different times
    boots_test_triple_mirror_reinsert_drives_separately(in_rg_config_p);

    //  Return 
    return;

} //  End boots_test_main_triple_mirror()


/*!****************************************************************************
 *  boots_test_triple_mirror_remove_drives_together
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a triple mirror RAID group 
 *   when two drives are removed at the same time and reinserted at the same 
 *   time.  It does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in the secondary position in the RG
 *   - Removes the drive in the third position in the RG
 *   - Writes new data to a small number of LBAs
 *   - Reinserts both drives 
 *   - Waits for the rebuilds of both drives to finish
 *   - Removes the drive in the primary position in the RG.  This is to force it
 *     to read from the secondary drive.
 *   - Verifies that the new data is correct on the secondary drive
 *   - Removes the drive in the secondary position in the RG.  This is to force it
 *     to read from the third drive.
 *   - Verifies that the new data is correct on the third drive
 *   - Restores the RG 
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_triple_mirror_remove_drives_together(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t  drive_info_2;                   //  drive info needed for reinsertion 
    fbe_lba_t                           start_lba;                      //  lba at which to start I/Os


    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Set up the starting LBA for the I/O
    start_lba = 0x300;

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove two drives in the RG.  Check the object states change correctly and that rb logging starts. 
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Write new data to a few LBAs
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Reinsert both the drives 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);
    sep_rebuild_utils_triple_mirror_r6_second_drive_restored(in_rg_config_p, &drive_info_2);

    //  Wait until the rebuilds finish 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    //  Remove the drive in position 0 in the RG.  This will force the read to occur from position 1 which was
    //  rebuilt.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_1);

    //  Read the data from position 1 and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  Remove the drive in position 1 in the RG.  This will force the read to occur from position 2.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_2); 

    //  Read the data from position 2 and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba);

    //  At this point we have verified that 2 disks in the triple mirror were rebuilt properly.  Positions
    //  0 and 1 are removed.  Restore the RG by reinserting the two drives.
    boots_restore_triple_mirror_rg(in_rg_config_p, &drive_info_1, &drive_info_2); 

    //  Return 
    return;

} //  End boots_test_triple_mirror_remove_drives_together()


/*!****************************************************************************
 *  boots_test_triple_mirror_remove_drives_separately
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a triple mirror RAID group 
 *   when one drive is removed first and another is later removed (while the 
 *   first drive is still out).  It does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in the secondary position in the RG
 *   - Writes new data to a small number of LBAs
 *   - Removes the drive in the third position in the RG
 *   - Writes new data to a small number of different LBAs
 *   - Reinserts both drives 
 *   - Waits for the rebuilds of both drives to finish
 *   - Removes the drive in the primary position in the RG.  This is to force it
 *     to read from the secondary drive.
 *   - Verifies that the data is correct on the secondary drive
 *   - Removes the drive in the secondary position in the RG.  This is to force it
 *     to read from the third drive.
 *   - Verifies that the data is correct on the third drive
 *   - Restores the RG 
 * 
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_triple_mirror_remove_drives_separately(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t  drive_info_2;                   //  drive info needed for reinsertion 
    fbe_lba_t                           start_lba_1;                    //  lba at which to start 1st set of I/Os
    fbe_lba_t                           start_lba_2;                    //  lba at which to start 2nd set of I/Os


    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging starts
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Write new data to a few LBAs
    start_lba_1 = 0x1500;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);

    //  Remove a second drive in the RG 
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Write new data to a few LBAs
    start_lba_2 = 0x500;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Reinsert both the drives 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);
    sep_rebuild_utils_triple_mirror_r6_second_drive_restored(in_rg_config_p, &drive_info_2);

    //  Wait until the rebuilds finish 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    //  Remove the drive in position 0 in the RG.  This will force the read to occur from position 1 which was
    //  rebuilt.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_1); 
                                  
    //  Read the data from position 1 and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Remove the drive in position 1 in the RG.  This will force the read to occur from position 2.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_2); 

    //  Read the data from position 2 and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  At this point we have verified that 2 disks in the triple mirror were rebuilt properly.  Positions
    //  0 and 1 are removed.  Restore the RG by reinserting the two drives.
    boots_restore_triple_mirror_rg(in_rg_config_p, &drive_info_1, &drive_info_2); 

    //  Return 
    return;

} //  End boots_test_triple_mirror_remove_drives_separately()


/*!****************************************************************************
 *  boots_test_triple_mirror_reinsert_drives_separately
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a triple mirror RAID group 
 *   when two drives are removed separately and reinserted separately with 
 *   overlap.  It does the following:
 *
 *   - Writes a seed pattern 
 *   - Removes the drive in the secondary position in the RG
 *   - Writes new data to a small number of LBAs
 *   - Removes the drive in the third position in the RG
 *   - Writes new data to a small number of different LBAs
 *   - Reinserts the drive in the third position 
 *   - Waits for the rebuild of the third position drive to finish
 *   - Writes new data to a small number of different LBAs
 *   - Reinserts the drive in the second position 
 *   - Waits for the rebuild of the second position drive to finish
 *   - Removes the drive in the primary position in the RG.  This is to force it
 *     to read from the secondary drive.
 *   - Verifies that the data is correct on the secondary drive
 *   - Removes the drive in the secondary position in the RG.  This is to force it
 *     to read from the third drive.
 *   - Verifies that the data is correct on the third drive
 *   - Restores the RG 
 * 
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_triple_mirror_reinsert_drives_separately(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t  drive_info_2;                   //  drive info needed for reinsertion 
    fbe_lba_t                           start_lba_1;                    //  lba at which to start 1st set of I/Os
    fbe_lba_t                           start_lba_2;                    //  lba at which to start 2nd set of I/Os
    fbe_lba_t                           start_lba_3;                    //  lba at which to start 3rd set of I/Os


    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging starts
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Write new data to a few LBAs
    start_lba_1 = 0x1000;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);

    //  Remove a second drive in the RG
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Write new data to a few LBAs
    start_lba_2 = 0;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);

    //  Reinsert the second drive that we removed 
    sep_rebuild_utils_triple_mirror_r6_second_drive_restored(in_rg_config_p, &drive_info_2);

    //  Wait until the rebuild finishes 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_SECOND_REMOVED_DRIVE_POS);

    //  Write new data to a few LBAs
    start_lba_3 = 0x500;
    sep_rebuild_utils_write_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_3);

    //  Reinsert the first drive that we removed 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Wait until the rebuild finishes 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    //  Remove the drive in position 0 in the RG.  This will force the read to occur from position 1 which was
    //  rebuilt.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_1); 
                                  
    //  Read the data from position 1 and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_3);

    //  Remove the drive in position 1 in the RG.  This will force the read to occur from position 2.  
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1, sep_rebuild_utils_number_physical_objects_g,
            &drive_info_2); 

    //  Read the data from position 2 and verify it 
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_1);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_2);
    sep_rebuild_utils_verify_new_data(in_rg_config_p, &fbe_boots_test_context_g[0], start_lba_3);

    //  At this point we have verified that 2 disks in the triple mirror were rebuilt properly.  Positions
    //  0 and 1 are removed.  Restore the RG by reinserting the two drives.
    boots_restore_triple_mirror_rg(in_rg_config_p, &drive_info_1, &drive_info_2); 

    //  Return 
    return;

} //  End boots_test_triple_mirror_reinsert_drives_separately()


/*!****************************************************************************
 *  boots_restore_triple_mirror_rg
 ******************************************************************************
 *
 * @brief
 *   This function is called when a triple mirror RG has both disks 0 and 1 
 *   removed.  It will re-insert both of these drives and wait for them to 
 *   rebuild. 
 *
 * @param in_rg_config_p    - pointer to the RG configuration info
 * @param in_drive_info_1_p - pointer to structure with the "drive info" for 
 *                            position 0
 * @param in_drive_info_2_p - pointer to structure with the "drive info" for 
 *                            position 1
 *
 * @return None 
 *
 *****************************************************************************/
static void boots_restore_triple_mirror_rg(
                            fbe_test_rg_configuration_t*            in_rg_config_p,    
                            fbe_api_terminator_device_handle_t*     in_drive_info_1_p,
                            fbe_api_terminator_device_handle_t*     in_drive_info_2_p)
{
	fbe_object_id_t                     rg_object_id;
	fbe_status_t                        status;

	status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;

    //  Reinsert the drive in position 0
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0, sep_rebuild_utils_number_physical_objects_g,
            in_drive_info_1_p);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;

    //  Reinsert the drive in position 1
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1, sep_rebuild_utils_number_physical_objects_g,
            in_drive_info_2_p);

    //  Wait until the rebuilds finish of both drives 
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_0);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_POSITION_1);

    sep_rebuild_utils_check_bits(rg_object_id);

    //  Return
    return;

} //  End boots_restore_triple_mirror_rg()


/*!****************************************************************************
 *  boots_test_when_no_io
 ******************************************************************************
 *
 * @brief
 *   This function tests that differential rebuild works properly when no chunks 
 *   need to be rebuilt.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 *
 * @return  None 
 *
 *****************************************************************************/
static void boots_test_when_no_io(
                                        fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t  drive_info_1;                   //  drive info needed for reinsertion 
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);

    //  Write a seed pattern to the RG 
    sep_rebuild_utils_write_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging starts
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Reinsert the drive 
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Wait until the rebuild finishes  
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Read the data and make sure it matches the seed pattern
    sep_rebuild_utils_read_bg_pattern(&fbe_boots_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    //  Return 
    return;

} //  End boots_test_when_no_io()

/*!****************************************************************************
 *  boots_run_raid5_tests
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Boots tests that run on a Raid-5 RG
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void boots_run_raid5_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t  status;

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Perform basic tests on the RAID-5 RG 
    boots_test_main_r5_r3(config_p);

    //  Using the RAID-5 RG, test the case where there is no data to be rebuilt
    boots_test_when_no_io(config_p);

    return;
} // End  boots_run_raid5_tests()
