/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file dora_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of marking Needs Rebuild for rebuild logging / RAID group health
 *   check.  
 *
 * @version
 *   10/13/2009 - Created. Jean Montes
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "sep_rebuild_utils.h"                      //  this test's .h file
#include "mut.h"                                    //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_database_interface.h"         //  for fbe_api_database_lookup_raid_group_by_number
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config
#include "fbe/fbe_api_scheduler_interface.h"        // for debug hook

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* dora_short_desc = "Raid group rebuild logging and shutdown test.";
char* dora_long_desc =
"\n\
The Dora test exercises raid group rebuild logging and raid group shutdown handling.\n\
Configurations tested: R5 (3 drive), R1 (2 and 3 drive), R6 (6 drive), R10 (4 drive)\n\
\n\
\n\
STEP 1: Quiesce test (R5 and R1 only)\n\
       - Start I/O.\n\
       - Quiesce SP A.\n\
       - Wait for quiesce to finish\n\
       - Unquiesce, stop I/O and check for I/O errors.\n\
STEP 2: Remove one of the drives in the RG (drive A)\n\
       - Remove the physical drive (PDO-A)\n\
       - Validate that PVD-A is now in the FAIL state\n"   
"       - Validate that VD-A is now in the FAIL state\n\
       - Validate that the RAID object is in the READY state\n\
       - Validate that the LUN object is in the READY state\n\
STEP 3: Validate that rebuild logging is processed\n\
       - Validate that rebuild logging is set set for drive A.\n\
       - Validate that the rebuild checkpoint is set to 0 for drive A.\n\
       - If we are R6 or 3 drive R1, run step 2 again for another drive.\n\
STEP 4: Remove the second drive in the RG (drive B)\n\
       - Remove the physical drive (PDO-B)\n\
       - Validate that PVD-B is now in the FAIL state\n\
       - Validate that VD-B is now in the FAIL state\n\
STEP 5: Validate the RAID group is shutdown\n\
       - Validate that the RAID object is now in the FAIL state\n\
       - Validate that the LUN object is in the FAIL state    \n\
       - Validate that the Needs Rebuild data is not set for this disk\n"
"STEP 6: Reinsert the second drive (drive B)\n\
       - Insert the physical drive (PDO-B)\n\
       - Validate that PVD-B is now in the READY state\n\
       - Validate that VD-B is now in the READY state  \n\
       - Validate that the RAID object is in the READY state\n\
       - Validate that the LUN object is in the READY state\n\
STEP 7: Reinsert the first removed drive.  \n\
       - Make sure rebuild logging is cleared.\n\
       - Wait for the rebuild to complete.\n\
       - Validate there are no needs rebuild bits set in the paged metadata..\n\
\n"
"STEP 8: Shutdown test. (R5 and R6) \n\
       - The purpose of this test is to make sure that we take I/O failures into account\n\
         prior to marking rebuild logging.  It is important to recogize through I/O failure\n\
         that a raid group is going failed prior to marking rebuild logging.\n\
         If we mark rebuild logging too soon, then it is possible for us to get data loss.\n\
       - Add a drive failure error record. For R6 add two error records.\n\
       - Start I/O and wait for the error to be hit.\n\
       - Add a drive failure error record for another position.\n\
       - Start I/O and wait for the error to be hit.\n\
       - Pull out one drive so the raid group sees the event.\n\
       - Make sure that we are not rebuild logging.\n\
       - We should make sure the raid group goes failed (this code is missing).\n\
       - Pull out another drive to make the raid group go shutdown. (R6 pulls 2 drives).\n\
       - Make sure that we are not rebuild logging.\n\
       - Restore rg and make sure we are not rebuild logging.\n\
       - Read back the data and make sure there is no error.\n\
       - (this code is missing) The test should validate that pattern for this area is the background pattern.\n\
       - (this code is missing) The test should validate that no uncorrectables or correctables were received.\n\
Final STEP : Cleanup\n\
       - Destroy objects\n\
\n\
Description last updated: 10/17/2011.\n";

//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

extern fbe_u32_t    sep_rebuild_utils_number_physical_objects_g;
extern fbe_u32_t    sep_rebuild_utils_next_hot_spare_slot_g;

//  RG configuration for the R5 
fbe_test_rg_configuration_t dora_r5_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  RG configuration for the R1
fbe_test_rg_configuration_t dora_r1_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_R1_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR, 520, 1, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  RG configuration for the triple mirror 
fbe_test_rg_configuration_t dora_triple_mirror_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR, 520, 2, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  RG configuration for the second triple mirror 
fbe_test_rg_configuration_t dora_triple_mirror_2_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR, 520, 3, 0},
	{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */}
};

//  RG configuration for the R6 
fbe_test_rg_configuration_t dora_r6_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 4, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  RG configuration for a second R6 
fbe_test_rg_configuration_t dora_r6_2_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY,
    520, 5, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  RG configuration for a dummy RG used in differential rebuild of a DB drive   
fbe_test_rg_configuration_t dora_dummy_rg_config_g = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
		SEP_REBUILD_UTILS_DUMMY_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR,
    520, 6, 0
};

//  RG configuration for the user RG on the database drives  
fbe_test_rg_configuration_t dora_db_drives_rg_config_g = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
		SEP_REBUILD_UTILS_DB_DRIVES_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_DB_DRIVES_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID5,
    FBE_CLASS_ID_PARITY, 520, 7, 0
};

//  RG configuration for the R10  
fbe_test_rg_configuration_t dora_r10_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_R10_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER, 520, 8, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  RG configuration for the second R5 
fbe_test_rg_configuration_t dora_r5_2_rg_config_g = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
		SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,
    520, 9, 0
};

//  Set of disks to be used for a dummy RG used in differential rebuild of a DB drive - disks 0-2-8 and 0-2-9
fbe_test_raid_group_disk_set_t fbe_dora_dummy_disk_set_g[SEP_REBUILD_UTILS_DUMMY_RAID_GROUP_WIDTH] =
{
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_THIRD, SEP_REBUILD_UTILS_DUMMY_SLOT_1},                // 0-2-8
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_THIRD, SEP_REBUILD_UTILS_DUMMY_SLOT_2},                // 0-2-9
}; 


//  Set of disks to be used for a user RG on the system drives - disks 0-0-0 through 0-0-2
fbe_test_raid_group_disk_set_t fbe_dora_db_drives_disk_set_g[SEP_REBUILD_UTILS_DB_DRIVES_RAID_GROUP_WIDTH] =
{
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, SEP_REBUILD_UTILS_DB_DRIVES_SLOT_0},            // 0-0-0
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, SEP_REBUILD_UTILS_DB_DRIVES_SLOT_1},            // 0-0-1
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_FIRST, SEP_REBUILD_UTILS_DB_DRIVES_SLOT_2}             // 0-0-2
};  

//  Set of disks to be used for the second R5 - disks 0-2-0 through 0-2-2
fbe_test_raid_group_disk_set_t fbe_dora_r5_2_disk_set_g[SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH] =
{
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_THIRD, SEP_REBUILD_UTILS_R5_2_SLOT_1},                 // 0-2-0
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_THIRD, SEP_REBUILD_UTILS_R5_2_SLOT_2},                 // 0-2-1
    {SEP_REBUILD_UTILS_PORT, SEP_REBUILD_UTILS_ENCLOSURE_THIRD, SEP_REBUILD_UTILS_R5_2_SLOT_3}                  // 0-2-2
};

//  Context objects used for running rdgen I/O
static fbe_api_rdgen_context_t dora_test_rdgen_context_g[SEP_REBUILD_UTILS_NUM_RAID_GROUPS][SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP];
static fbe_u32_t    dora_outstanding_async_io[SEP_REBUILD_UTILS_NUM_RAID_GROUPS][SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP] =  { 0 };

//  wait time in ms before debug hook report the error
#define DORA_WAIT_TIME 30000

#define DORA_WAIT_FOR_VERIFY 10000

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:

//  Run all tests
static void dora_run_tests(void);

//  Perform all tests for RG types R5, R3, R1
static void dora_run_raid5_raid3_raid1_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  Perform all tests for RG type triple mirror
static void dora_run_triple_mirror_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  Perform all tests for RG type R6
static void dora_run_raid6_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  Perform all tests for RG type R10
static void dora_run_raid10_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  Perform quiesce I/O test 
static void dora_quiesce_io_test(fbe_test_rg_configuration_t*  in_rg_config_p,
                                 fbe_u32_t                     iterations);

//  Perform all tests on a RAID-5, RAID-3, or RAID-1
static void dora_test_main_r5_r3_r1(
                                    fbe_test_rg_configuration_t*    in_rg_config_p);

//  Perform all tests on a triple mirror RAID-1, RAID-6
static void dora_test_main_triple_mirror_r6(
                                    fbe_test_rg_configuration_t*    in_rg_config_p);

//  Perform all tests on a RAID-10
static void dora_test_main_r10(
                               fbe_test_rg_configuration_t*    in_rg_config_p);

//  Write a data pattern to all the LUNs in a RG
static void dora_test_write_background_pattern(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index);

//  Inject an error record in a RG
static void dora_test_inject_error_record(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u16_t                       in_position,
                                    fbe_lba_t                       in_lba,
                                    fbe_u16_t                       in_rg_width);

//  Start I/O to the LUNs in a RG
static void dora_test_start_io(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id);

// Stop the asynchronous I/O
static void dora_test_stop_io(     
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id);

// Cleanup the I/O context
static void dora_test_cleanup_io(     
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id);

//  Wait for error injection(s) to complete
static void dora_test_wait_for_error_injection_complete(
                                               fbe_test_rg_configuration_t*    in_rg_config_p,
                                               fbe_u16_t                       in_err_record_cnt);

//  Verify rdgen I/O results
static void dora_test_verify_io_results(
                                        fbe_u32_t   in_rdgen_context_rg_index);

//  Run rdgen I/O test on the specified SP
static void dora_test_sp_run_io_test(fbe_test_rg_configuration_t*            in_rg_config_p,
                                     fbe_api_rdgen_context_t*                in_context_p, 
                                     fbe_u32_t                               in_rdgen_context_rg_index,
                                     fbe_sim_transport_connection_target_t   in_sp,
                                     fbe_u32_t                               in_abort_options,
                                     fbe_test_sep_io_dual_sp_mode_t          in_peer_options);

//  Run quiesce/unquiesce test on the specified SP
static void dora_test_quiesce_io_run_quiesce_unquiesce_test(fbe_test_rg_configuration_t* in_rg_config_p,
                                                            fbe_u32_t in_iterations, 
                                                            fbe_sim_transport_connection_target_t in_sp);

//  Test removal of a single drive in a RAID-5, RAID-3, or RAID-1 and that rb logging is NOT set.
//  There are timing windows where on a drive failure, we do not want to mark NR right away.
void dora_test_not_rb_logging_r5_r3_r1(
                        fbe_test_rg_configuration_t*            in_rg_config_p,
                        fbe_u32_t                               in_position,
                        fbe_api_terminator_device_handle_t*     out_drive_info_p);

//  Test removal of a second drive in a RAID-5, RAID-3, or RAID-1 and that the RAID object becomes failed
void dora_test_rg_shutdown_r5_r3_r1(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_u32_t                               in_position,
                            fbe_api_terminator_device_handle_t*     out_drive_info_p);

//  Test reinsertion of a second drive in a RAID-5, RAID-3, or RAID-1 and that the RAID object becomes ready
void dora_test_rg_restore_to_degraded_r5_r3_r1(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_u32_t                               in_position,
                            fbe_api_terminator_device_handle_t*     in_drive_info_p);


static void dora_add_debug_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate);

static void dora_delete_debug_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) ;

fbe_status_t dora_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate);

static void dora_test_not_rb_logging_r5_on_io_error(
                        fbe_test_rg_configuration_t*            in_rg_config_p,
                        fbe_u32_t                               in_position_first,
                        fbe_u32_t                               in_position_second,
                        fbe_api_terminator_device_handle_t*     out_first_drive_info_p,
                        fbe_api_terminator_device_handle_t*     out_second_drive_info_p);

void dora_test_not_rb_logging_r6_on_io_error(
                        fbe_test_rg_configuration_t*                 in_rg_config_p,
                        fbe_u32_t                                           in_position_first,
                        fbe_u32_t                                           in_position_second,
                        fbe_u32_t                                           in_position_third,
                        fbe_api_terminator_device_handle_t*     out_first_drive_info_p,
                        fbe_api_terminator_device_handle_t*     out_second_drive_info_p,             
                        fbe_api_terminator_device_handle_t*     out_third_drive_info_p);


//-----------------------------------------------------------------------------
//  FUNCTIONS:

/*!****************************************************************************
 *  dora_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Dora test for single-SP testing.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_test(void)
{
    //  Ensure dualsp mode disabled
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    //  Run tests
    dora_run_tests();

    //  Return 
    return;

} //  End dora_test()

/*!****************************************************************************
 *  dora_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Dora test for dual-SP testing.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_dualsp_test(void)
{
    //  Enable dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    //  Run tests
    dora_run_tests();

    //  Always clear dualsp mode
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    //  Return 
    return;

} //  End dora_dualsp_test()

/*!****************************************************************************
 *  dora_run_tests
 ******************************************************************************
 *
 * @brief
 *   Collection of tests to test rebuild logging functionality.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void dora_run_tests(void)
{
    fbe_test_rg_configuration_t *rg_config_p;   // pointer to the RG test config specs


    // For these tests we don't want to use hot spares.
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1800);
    
    //  Run RAID5 tests
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM RAID-5 TESTS\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    rg_config_p = &dora_r5_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p,                          
                                   NULL,                                 
                                   dora_run_raid5_raid3_raid1_tests,     
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);    

    //  Run RAID-1 tests
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM RAID-1 TESTS\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    rg_config_p = &dora_r1_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p,                          
                                   NULL,                                 
                                   dora_run_raid5_raid3_raid1_tests,     
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);    

    //  Run triple-mirror tests
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM TRIPLE MIRROR TESTS\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    rg_config_p = &dora_triple_mirror_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p,                          
                                   NULL,                                 
                                   dora_run_triple_mirror_tests,         
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);    


    //  Run RAID-6 tests
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM RAID-6 TESTS\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n"); 

    rg_config_p = &dora_r6_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p,                          
                                   NULL,                                 
                                   dora_run_raid6_tests,                 
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN,FBE_FALSE);    

    //  Run RAID-10 tests
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM RAID-10 TESTS\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n"); 

    rg_config_p = &dora_r10_rg_config_g[0];
    fbe_test_run_test_on_rg_config(rg_config_p, 
                                   NULL, 
                                   dora_run_raid10_tests,
                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN);

    //  Return 
    return;

} //  End dora_run_tests()

/*!****************************************************************************
 *  dora_run_raid5_raid3_raid1_tests
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Dora tests that run on a Raid-5 RG, a Raid-3 RG, or a Raid-1 RG
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_run_raid5_raid3_raid1_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t        status;


    // Perform quiese tests; I/O is quiesed/unquiesced as part of marking needs-rebuild
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Perform quiesce test\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    dora_quiesce_io_test(config_p, 1);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    dora_test_main_r5_r3_r1(config_p);

    //  Return 
    return;

} //  End dora_run_raid5_raid3_raid1_tests()


/*!****************************************************************************
 *  dora_run_triple_mirror_tests
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Dora tests that run on a triple mirror RG
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_run_triple_mirror_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t        status;


    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Perform all tests on the triple mirror
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Perform triple mirror tests\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    dora_test_main_triple_mirror_r6(config_p);

    //  Return 
    return;

} //  End dora_run_triple_mirror_tests()


/*!****************************************************************************
 *  dora_run_raid6_tests
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Dora tests that run on a Raid-6 RG
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_run_raid6_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t        status;


    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Perform all tests on the RAID-6 RG
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Perform RAID-6 tests\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n"); 
    dora_test_main_triple_mirror_r6(config_p);

    //  Return 
    return;

} //  End dora_run_raid6_tests()


/*!****************************************************************************
 *  dora_run_raid10_tests
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Dora tests that run on a Raid-10 RG
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_run_raid10_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t        status;


    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Perform all tests on the RAID-10 RG
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Perform RAID-10 tests\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n"); 
    dora_test_main_r10(config_p);

    //  Return 
    return;

} //  End dora_run_raid10_tests()

/*!****************************************************************************
 *  dora_test_main_r5_r3_r1
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for R5, R3, and R1 RAID groups.  It does 
 *   the following:
 *
 *   - Removes a drive and verifies that the RG remains ready and rb logging starts.
 *   - Removes a second drive in the RG and verifies that the RG goes to the 
 *     FAIL state
 *   - Reinserts the second drive and verifies that the RG goes to ready
 *   - Reinserts the first drive and verifies that the RG remains ready
 *     and the RG is no longer degraded
 *   - For R5, repeats test, but this time injects errors on some disks to test
 *     that NR processing does not occur if the RG is going broken
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 *
 *****************************************************************************/
static void dora_test_main_r5_r3_r1(
                                            fbe_test_rg_configuration_t*    in_rg_config_p)
{
    fbe_api_terminator_device_handle_t      drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_2;                   //  drive info needed for reinsertion
    fbe_u32_t                               first_rdgen_context_index;      //  rdgen context for first injection
    fbe_u32_t                               second_rdgen_context_index;     //  rdgen context for second injection
    fbe_object_id_t                         lun_object_id;                  //  object ID of a LUN in the RG
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p;   //  configuration of a LUN in the RG
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test 1: Test removing a drive and confirming RG is disabled and NR set on the drive correctly
    // 
  
    //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging starts
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //
    //  Test 2: Test removing another drive and confirm that the RG is broken and NR
    //          is not marked on the second drive
    
    //  Remove a second drive in the RG.  Check the object states change correctly.
    dora_test_rg_shutdown_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_LAST_REMOVED_DRIVE_POS, &drive_info_2);

    //
    //  Test 3: Test reinserting the failed drives and confirming RG restored correctly
    // 
    
    //  Reinsert the second drive to fail.  Check the object states change correctly.
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_LAST_REMOVED_DRIVE_POS, &drive_info_2);

    //  Reinsert the first drive to fail.  Check the object states change correctly.
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    //  Wait for rebuild to finish
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_LAST_REMOVED_DRIVE_POS);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  If the RAID type is NOT RAID-5, return now.   The following test applies to RAID-5 only.
    if (FBE_RAID_GROUP_TYPE_RAID5 != in_rg_config_p->raid_type)
    {
        return;
    }

    //
    //  Test 5: Try to reproduce data-loss scenario possible if multiple drives fail in the RG and rb logging is marked 
    //          on the first drive before the second drive is reported as failed.
    // 
    //    Disk configuration for this RAID-5 RAID Group:
    //
    //                      Disk 4      Disk 5      Disk 6
    //                      ------      ------      ------
    //                      Parity      Data        Data
    //                      Parity      Data        Data
    //
    //    Data-loss example:
    //          If Disk 6 and Disk 5 are both pulled during I/O, but during NR processing, the upper layers only 
    //          know about the Disk 6 failure and mark NR on Disk 6, when both drives are brought back, a rebuild 
    //          will occur on the LUNs and the stripes may be left unknowingly inconsistent. (I/O may have continued 
    //          during the disk failures and completed on other chunks.) I/O issued after the rebuild may fail
    //          resulting in data loss.
    //  
    //          To make this timing issue less likely, when a drive fails, before marking rb logging, 
    //          NR processing should first make sure there are no other failed I/Os on other drives such 
    //          that the RG is going broken. If so, NR processing should be delayed to give time for
    //          the upper layers to recognize the additional drive failures and handle the broken RG
    //          accordingly.
    //
    //          To reproduce this timing problem in simulation, a background pattern of all zeros will be 
    //          written to all LUNs in the RAID-5 RAID group. Then an error will be injected on Disk 5, LBA 0
    //          and Disk 6, LBA 128 and a new pattern will be written to the LUNs. For the first stripe, for
    //          example, writes will complete to Disk 4 and Disk 6, but not Disk 5. Then Disk 6 will be pulled 
    //          which will mark NR on the LUNs. Then Disk 5 will be pulled so the RAID Group is broken.  When the 
    //          drives are reinserted, a rebuild will occur. If then try to read data on Disk 6, LBA 0, a media error 
    //          will occur. 
    // 
    //          With the production code enhanced to avoid this timing problem, rb logging should not be
    //          marked on Disk 6; instead, the code should recognize that there is a disk failure on
    //          disk 5 by finding the error that was injected on it and recognize that the RG should be 
    //          going broken.  NR processing will be delayed to allow for Disk 5 to be recognized as failed.
    //      
    // 
    //    NOTES:
    //          Disk        Position in RG
    //          ----        --------------
    //          4           0
    //          5           1
    //          6           2
    // 
    //          The position of the disk in the RAID Group is passed to the error injection functions.
    //
  
    // Make sure verify is not running before we begin this test
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, DORA_WAIT_FOR_VERIFY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Set the index in the rdgen context array
    first_rdgen_context_index = 0;
    second_rdgen_context_index = first_rdgen_context_index + 1;

    //  Write a test pattern to the LUNs in the RG
    dora_test_write_background_pattern(in_rg_config_p, first_rdgen_context_index);

    //  Inject an error record in the RG at disk position 1, disk LBA 0
    dora_test_inject_error_record(in_rg_config_p, 1, 0, SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH);

    //  Get the object ID of the first LUN in the RG in LBA order. Only need the first LUN in the RG for this test.
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    //  Try to write a new data pattern to the first LUN in the RG in LBA order at Host LBA 128 for 1 block.
    //  Note that the error was injected on disk position 1, disk LBA 0; this translates to Host 
    //  LBA 128.
    dora_test_start_io(in_rg_config_p, first_rdgen_context_index, 128, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Wait for error injections to complete; injected 1 error here
    dora_test_wait_for_error_injection_complete(in_rg_config_p, 1);

    //  Inject an error record in the RG at disk position 2, disk LBA 128
    dora_test_inject_error_record(in_rg_config_p, 2, 128, SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH);

    //  Try to write a new data pattern to the first LUN in the RG in LBA order at Host LBA 256 for 1 block.
    //  Note that the error was injected on disk position 2, disk LBA 128; this translates to Host 
    //  LBA 256.
    dora_test_start_io(in_rg_config_p, second_rdgen_context_index, 256, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Wait for error injections to complete; injected 1 error here
    dora_test_wait_for_error_injection_complete(in_rg_config_p, 1);

    // verify that rb logging not start after drive removal
    // It will remove the drives in specific sequence which is based on the disk position having injected error
    dora_test_not_rb_logging_r5_on_io_error(in_rg_config_p, 2, 1, &drive_info_2, &drive_info_1);

    //  Reinsert disk in position 1 and verify drive state changes 
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 1, &drive_info_1);

    //  Reinsert disk in position 2 and verify drive state changes
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, 2, &drive_info_2);

    // Stop the asynchronous I/O
    dora_test_stop_io(in_rg_config_p, first_rdgen_context_index, 256, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);
    dora_test_stop_io(in_rg_config_p, second_rdgen_context_index, 256, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Try to read data from the LUNs in the RG.
    //  If NR was marked on disk position 2 when it was removed, a rebuild would be initiated when the
    //  drive was brought back. Because of the error injection on disk position 1, when the drives 
    //  were reinserted and rebuilt, a subsequent read on disk position 2, disk LBA 0 would cause a 
    //  media error. We have data loss.
    //  Because the production code should help handle this use case, there should be no errors here.

    //  Try to read data from the first LUN in the RG in LBA order at Host LBA 0 for 1 block
    dora_test_start_io(in_rg_config_p, first_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_READ_ONLY, lun_object_id);

    //  Verify results
    dora_test_verify_io_results(first_rdgen_context_index);

    //  ! @TODO: check the data on disk by using a rdgen "read and compare" operation. Then 
    //  repeat the write operation by doing a "verify write"; this should mimic cache behavior 
    //  where cache reissues the failed write.

    // I/o is stopped (we waited for it) but we must cleanup
    dora_test_cleanup_io(in_rg_config_p, first_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_READ_ONLY, lun_object_id);

    //  Return 
    return;

} //  End dora_test_main_r5_r3_r1()

/*!****************************************************************************
 *  dora_test_main_triple_mirror_r6
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for a triple mirror RAID group and RAID-6
 *   RAID group.  It does the following:
 *
 *   - Removes a drive and verifies that the RG remains ready and rb logging started. 
 *   - Removes a second drive and verifies that the RG remains ready and rb logging started.
 *   - Removes a third drive and verifies that the RG goes to the FAIL state
 *   - Reinserts the third drive and verifies that the RG goes to ready 
 *   - Reinserts the second and first drive and verifies the RG remains ready
 * 
 *   - For RAID-6 RG, test with I/O proving that NR is not marked when the
 *     RG is going broken based on I/O, i.e. edges not reported as failed yet
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 *
 *****************************************************************************/
static void dora_test_main_triple_mirror_r6(
                                    fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t      drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_2;                   //  drive info needed for reinsertion
    fbe_api_terminator_device_handle_t      drive_info_3;                   //  drive info needed for reinsertion
    fbe_api_terminator_device_handle_t      drive_info_4;                   //  drive info needed for reinsertion
    fbe_api_terminator_device_handle_t      drive_info_5;                   //  drive info needed for reinsertion
    fbe_u32_t                               first_rdgen_context_index;      //  rdgen context for first injection
    fbe_u32_t                               second_rdgen_context_index;     //  rdgen context for second injection
    fbe_u32_t                               third_rdgen_context_index;      //  rdgen context for third injection
    fbe_object_id_t                         rg_object_id;                   //  object ID of RG
    fbe_object_id_t                         lun_object_id;                  //  object ID of a LUN in the RG
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p;   //  configuration of a LUN in the RG
    fbe_status_t                            status;                         //  return status
    fbe_api_logical_error_injection_record_t    error_record = {0};         //  error injection record
    fbe_api_logical_error_injection_get_stats_t stats;                      //  error injection stats


    //  Remove a single drive in the RG (position 1).  Check the object states change correctly and that 
    //  rb logging started.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, 1, &drive_info_1);

    //  Remove a second drive in the RG.  Check that the RG stays ready and the drive is marked rb logging. 
    //  Note that this function removes position 2 in the RG
    //  ! @TODO: change this API so more generic, i.e. pass along the position number
    //  This API is called across multiple tests
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Remove a third drive in the RG (position 0).  Check that the RG goes broken.
    dora_test_rg_shutdown_r5_r3_r1(in_rg_config_p, 0, &drive_info_3);

    //  Reinsert the third drive to fail.  Check that the RG becomes ready.
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 0, &drive_info_3);

    //  Reinsert the second drive that failed and verify drive state changes
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, 2, &drive_info_2);

    //  Reinsert the first drive that failed and verify drive state changes
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, 1, &drive_info_1);

    //  Wait for rebuild to finish
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, 1);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, 2);
    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, 0);

    //  Get the RG object ID
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    sep_rebuild_utils_check_bits(rg_object_id);

    //
    //  Test 2: Try to reproduce data-loss scenario possible if multiple drives fail in the RG and rb logging is marked 
    //          on the first drive before the other drives are reported as failed.
    // 
    //          This applies to parity RAID groups only. This function tests R1 and R6, so this test applies here to R6.
    // 
    //          Disk configuration for this RAID-6 RAID Group:
    //
    //
    //                      Disk 4      Disk 5      Disk 6      Disk 7      Disk 8   Disk 9    
    //                      ------      ------      ------      ------      ------   ------
    //                      Parity      Parity      Data        Data        Data     Data
    //                      Parity      Parity      Data        Data        Data     Data
    //
    //          Data-loss example:
    // 
    //          If Disk 9, Disk 8, Disk 7 are pulled during I/O, but during NR processing, the upper layers only 
    //          know about the Disk 9 failure and mark NR on Disk 9, when all drives are brought back, a rebuild 
    //          will occur on the LUNs and the stripes may be left unknowingly inconsistent. (I/O may have continued 
    //          during the disk failures and completed on other chunks.) I/O issued after the rebuild may fail
    //          resulting in data loss.
    //  
    //          To make this timing issue less likely, when a drive fails, before marking rb logging, 
    //          NR processing should first make sure there are no other failed I/Os on other drives and/or other failed 
    //          edges such that the RG is going broken. If so, NR processing should be delayed to give time for
    //          the upper layers to recognize the additional drive failures and handle the broken RG
    //          accordingly.
    //
    //          To reproduce this timing problem in simulation, a background pattern of all zeros will be 
    //          written to all LUNs in the RAID-6 RAID group. Then an error will be injected on Disk 7, LBA 0, 
    //          Disk 8, LBA 0 and Disk 9, LBA 128 and a new pattern will be written to the LUNs. For the first stripe, for
    //          example, writes will complete to all drives, but Disk 7 and Disk 8. Then Disk 9 will be pulled 
    //          which will mark NR on the LUNs. Then Disk 7 and Disk 8 will be pulled so the RAID Group is broken. 
    //          When the drives are reinserted, a rebuild will occur. If then try to read data on Disk 9, LBA 0, a media 
    //          error will occur. 
    // 
    //          With the production code enhanced to avoid this timing problem, rb logging should not be
    //          marked on Disk 9; instead, the code should recognize that there is a disk failure on
    //          disk 7 and disk 8 by finding the error that was injected and otherwise recognize that the RG should be 
    //          going broken.  NR processing will be delayed to allow for Disk 8 and Disk 7 to be recognized as failed.
    //      
    // 
    //    NOTES:
    //          Disk        Position in RG
    //          ----        --------------
    //          4           0
    //          5           1
    //          6           2
    //          7           3
    //          8           4
    //          9           5
    // 
    //          The position of the disk in the RAID Group is passed to the error injection functions.
    //

    //  Only do this subtest for RAID-6 RAID groups
    if (FBE_RAID_GROUP_TYPE_RAID6 != in_rg_config_p->raid_type)
    {
        return;
    }

    //  Set the index in the rdgen context array
    first_rdgen_context_index = 0;
    second_rdgen_context_index = first_rdgen_context_index + 1;
    third_rdgen_context_index = second_rdgen_context_index + 1;

    //  Write a test pattern to the LUNs in the RG
    dora_test_write_background_pattern(in_rg_config_p, first_rdgen_context_index);    

    //  Disable error injection records in the error injection table (initializes records)
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable the error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Setup error record for error injection
    //  Injecting error on position 3, LBA 0
    //  ! @TODO: consider refactoring this code a bit; for this test, injecting errors on two positions
    //  for which I/O is then issued across those positions, so not issuing I/O for one error injection
    //  at a time which is what the utility functions expect; can update Dora to use tables, for example
    error_record.pos_bitmap = 1 << 3;                           //  Injecting error on given disk position
    error_record.width      = SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH;    //  RG width
    error_record.lba        = 0;                                //  Physical address to begin errors
    error_record.blocks     = in_rg_config_p->capacity;                                //  Blocks to extend errors
    error_record.err_type   = FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR;   // Error type
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;          // Error mode
    error_record.err_count  = 0;                                //  Errors injected so far
    error_record.err_limit  = SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP;    //  Limit of errors to inject
    error_record.skip_count = 0;                                //  Limit of errors to skip
    error_record.skip_limit = 0;                                //  Limit of errors to inject
    error_record.err_adj    = 0;                                //  Error adjacency bitmask
    error_record.start_bit  = 0;                                //  Starting bit of an error
    error_record.num_bits   = 0;                                //  Number of bits of an error
    error_record.bit_adj    = 0;                                //  Indicates if erroneous bits adjacent
    error_record.crc_det    = 0;                                //  Indicates if CRC detectable

    //  Create error injection record
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable error injection on the RG
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection: pos %d, lba %d ==", __FUNCTION__, 3, 0);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Check error injection stats
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    //  Setup error record for error injection
    //  Injecting error on position 4, LBA 0
    error_record.pos_bitmap = 1 << 4;                           //  Injecting error on given disk position
    error_record.width      = SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH;    //  RG width
    error_record.lba        = 0;                                //  Physical address to begin errors
    error_record.blocks     = in_rg_config_p->capacity;                                //  Blocks to extend errors
    error_record.err_type   = FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR;   // Error type
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;          // Error mode
    error_record.err_count  = 0;                                //  Errors injected so far
    error_record.err_limit  = SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP;    //  Limit of errors to inject
    error_record.skip_count = 0;                                //  Limit of errors to skip
    error_record.skip_limit = 0;                                //  Limit of errors to inject
    error_record.err_adj    = 0;                                //  Error adjacency bitmask
    error_record.start_bit  = 0;                                //  Starting bit of an error
    error_record.num_bits   = 0;                                //  Number of bits of an error
    error_record.bit_adj    = 0;                                //  Indicates if erroneous bits adjacent
    error_record.crc_det    = 0;                                //  Indicates if CRC detectable

    //  Create error injection record
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable error injection on the RG
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection: pos %d, lba %d ==", __FUNCTION__, 4, 0);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Check error injection stats
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 2);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    //  Get the object ID of the first LUN in the RG in LBA order. Only need the first LUN in the RG for this test.
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    //  Try to write a new data pattern to the first LUN in the RG in LBA order that covers Host LBA 128 and 256.
    //  Note that an error was injected on disk position 3, disk LBA 0; this translates to Host LBA 256.
    //  Another error was injected on disk position 4, disk LBA 0; this translates to Host LBA 128.
    dora_test_start_io(in_rg_config_p, first_rdgen_context_index, 128, 384, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Wait for error injections to complete; injected 2 errors here
    dora_test_wait_for_error_injection_complete(in_rg_config_p, 2);

    //  Inject an error record in the RG at disk position 5, disk LBA 128
    dora_test_inject_error_record(in_rg_config_p, 5, 128, SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH);

    //  Try to write a new data pattern to the first LUN in the RG in LBA order at Host LBA 512 for 1 block.
    //  Note that the error was injected on disk position 5, disk LBA 128; this translates to Host 
    //  LBA 512.
    dora_test_start_io(in_rg_config_p, second_rdgen_context_index, 512, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Wait for error injections to complete; injected 1 error here
    dora_test_wait_for_error_injection_complete(in_rg_config_p, 1);

    // verify that rb logging not start after drive removal
    // It will remove the drives in specific sequence which is based on the disk position having injected error
    dora_test_not_rb_logging_r6_on_io_error(in_rg_config_p, 5, 4, 3, &drive_info_5, &drive_info_4, &drive_info_3);

    //  Reinsert disk in position 3 and verify drive state changes 
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 3, &drive_info_3);

    //  Reinsert disk in position 4 and verify drive state changes
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, 4, &drive_info_4);

    //  Reinsert disk in position 5 and verify drive state changes
    sep_rebuild_utils_rg_ready_after_drive_restored_r5_r3_r1(in_rg_config_p, 5, &drive_info_5);

    // Stop the asynchronous I/O
    dora_test_stop_io(in_rg_config_p, first_rdgen_context_index, 128, 384, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);
    dora_test_stop_io(in_rg_config_p, second_rdgen_context_index, 512, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Try to read data from the LUNs in the RG.
    //  If NR was marked on disk position 5 when it was removed, a rebuild would be initiated when the
    //  drive was brought back. Because of the error injection on disk position 3 and 4, when the drives 
    //  were reinserted and rebuilt, a subsequent read on disk position 5, disk LBA 0 would cause a 
    //  media error. We have data loss.
    //  Because the production code should help handle this use case, there should be no errors here.

    //  Try to read data from the first LUN in the RG in LBA order at Host LBA 0 for 1 block
    dora_test_start_io(in_rg_config_p, third_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_READ_ONLY, lun_object_id);

    //  Verify results
    dora_test_verify_io_results(third_rdgen_context_index);

    //  ! @TODO: check the data on disk by using a rdgen "read and compare" operation. Then 
    //  repeat the write operation by doing a "verify write"; this should mimic cache behavior 
    //  where cache reissues the failed write.
    
    // I/o is stopped (we waited for it) but we must cleanup
    dora_test_cleanup_io(in_rg_config_p, third_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_READ_ONLY, lun_object_id);

    //  Return 
    return;

} //  End dora_test_main_triple_mirror_r6()


/*!****************************************************************************
 *  dora_test_main_r10
 ******************************************************************************
 *
 * @brief
 *   This is the main test function for a RAID-10 RAID group.  It does the following:
 *
 *   - Removes a drive and verifies that the RG remains ready and rb logging is started.
 *   - Removes a second drive in another mirrored pair and verifies that the RG remains ready and rb logging is marked
 *     for it
 *   - Removes a third drive and verifies that the RG goes to the FAIL state
 *   - Reinserts the third drive and verifies that the RG goes to ready
 *   - Reinserts the second drive and verifies the RG still ready
 *   - Removes a second drive in the same mirrored pair as the first drive and
 *     verifies that the RG goes to the FAIL state
 *   - Reinserts the second drive and verifies the RG goes to ready
 *
 * @param in_rg_config_p     - pointer to the RG configuration info 
 *
 * @return  None 
 *
 *****************************************************************************/
static void dora_test_main_r10(
                                    fbe_test_rg_configuration_t*    in_rg_config_p)
{

    fbe_api_terminator_device_handle_t      drive_info_1;       //  drive info of 1st drive removed 
    fbe_api_terminator_device_handle_t      drive_info_2;       //  drive info of 2nd drive removed
    fbe_api_terminator_device_handle_t      drive_info_3;       //  drive info of 3rd drive removed 
    fbe_object_id_t                     raid_group_id;
    fbe_status_t                        status;
    fbe_api_rg_get_status_t     rebuild_status;

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Remove a single drive in the RG.  Check the object states change correctly and that
    //  rb logging is started. This removes position 1 in the RG which is in the first mirrored pair.
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(in_rg_config_p, 1, &drive_info_1);

    //  Remove a second drive in the RG.  Check that the RG stays ready and the drive is marked rb logging. 
    //  This removes position 2 in the RG which is in the second mirrored pair.
    //  Because we lost drives in different mirrored pairs, the RG remains degraded.
    sep_rebuild_utils_triple_mirror_r6_second_drive_removed(in_rg_config_p, &drive_info_2);

    //  Remove a third drive in the RG.  Check that the RG goes broken.
    //  This removes position 0 in the RG which is in the first mirrored pair.
    dora_test_rg_shutdown_r5_r3_r1(in_rg_config_p, 0, &drive_info_3);

    //  Reinsert the third drive to fail.  Check the RG becomes ready.
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 0, &drive_info_3);

    //  Reinsert the second drive to fail.  Check the RG still ready.
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 2, &drive_info_2);

    //  Now one drive remains removed, position 1 in the RG which is in the first mirrored pair.
    // 
    //  Remove the other drive in the first mirrored pair. Check that the RG goes broken.
    //  This removes position 0 in the RG in the first mirrored pair.
    //  Because we lost both drives in a mirrored pair, the RG goes broken.
    dora_test_rg_shutdown_r5_r3_r1(in_rg_config_p, 0, &drive_info_3);

    //  Reinsert the last drive to fail.  Check the RG becomes ready.
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 0, &drive_info_3);

    status = fbe_api_raid_group_get_rebuild_status(raid_group_id, &rebuild_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(rebuild_status.percentage_completed != 100);

    // reinsert the first drive , for cleanup when running on hardware
    dora_test_rg_restore_to_degraded_r5_r3_r1(in_rg_config_p, 1, &drive_info_1);

    sep_rebuild_utils_wait_for_rb_comp(in_rg_config_p, 1);

    status = fbe_api_raid_group_get_rebuild_status(raid_group_id, &rebuild_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(rebuild_status.percentage_completed == 100);

    sep_rebuild_utils_check_bits(raid_group_id);

    //  Return 
    return;

} //  End dora_test_main_r10()


/*!****************************************************************************
 *  dora_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Dora test for single-SP testing.
 *   It creates the physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_setup(void)
{  
	sep_rebuild_utils_setup();

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

} //  End dora_setup()


/*!****************************************************************************
 *  dora_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the common cleanup function for the Dora test for single-SP testing.
 *   It destroys the physical configuration and unloads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_cleanup(void)
{  

    fbe_test_sep_util_restore_sector_trace_level();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();   
    }

    return;

} //  End dora_cleanup()


/*!****************************************************************************
 *  dora_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Dora test for dual-SP testing.
 *   It creates the physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_dualsp_setup(void)
{  
	sep_rebuild_utils_dualsp_setup();

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

} //  End dora_dualsp_setup()


/*!****************************************************************************
 *  dora_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the common cleanup function for the Dora test for dual-SP testing.
 *   It destroys the physical configuration and unloads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_dualsp_cleanup(void)
{  

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

} //  End dora_dualsp_cleanup()

/*!****************************************************************************
 *  dora_test_not_rb_logging_r5_r3_r1
 ******************************************************************************
 *
 * @brief
 *   Test that rb logging is NOT set for a drive when the drive is removed
 *   from a RAID-5, RAID-3, or RAID-1 RG (when the RG is enabled). There are
 *   timing windows where we do not want to mark NR on a drive failure.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - position of disk to remove
 * @param out_drive_info_p      - pointer to structure that gets filled in with the 
 *                                "drive info" for the drive
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_test_not_rb_logging_r5_r3_r1(
                        fbe_test_rg_configuration_t*            in_rg_config_p,
                        fbe_u32_t                               in_position,
                        fbe_api_terminator_device_handle_t*     out_drive_info_p)
{

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing position %d in the RG\n", __FUNCTION__, in_position);
        
    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove a drive in the RG and verify the drive state changes
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position, 
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_drive_info_p);  

    // Note that we cannot validate the status since depending on which drive pull we are on
    // the RG might be in either Activate or FAIL.

    //  Verify that rb logging is NOT set for the drive
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that rb logging is NOT set for the drive
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position);

        //  Set the target server back to the local SP
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End dora_test_not_rb_logging_r5_r3_r1()

/*!****************************************************************************
 *  dora_test_not_rb_logging_r5_on_io_error
 ******************************************************************************
 *
 * @brief
 *   Test that rb logging is NOT set for a drive when one drive has reported an error and we remove 
 *   the another drive from a RAID-5 RG (when the RG is enabled). There are timing windows where
 *   we do not want to mark NR on a drive failure.
 *
 * @param in_rg_config_p                 - pointer to the RG configuration info
 * @param in_position_first               - first position of disk to remove
 * @param in_position_second           - second position of disk to remove
 * @param out_first_drive_info_p       - pointer to structure that gets filled in with the 
 *                                                     "drive info" for the first drive
 * @param out_second_drive_info_p   - pointer to structure that gets filled in with the 
 *                                                     "drive info" for the second drive * 
 * @return  None 
 *
 * @Note    This test expect that error was inserted on drive 1st and 2nd position before calling it.
 *
 *****************************************************************************/
void dora_test_not_rb_logging_r5_on_io_error(
                        fbe_test_rg_configuration_t*                 in_rg_config_p,
                        fbe_u32_t                                           in_position_second,
                        fbe_u32_t                                           in_position_first,
                        fbe_api_terminator_device_handle_t*     out_second_drive_info_p,
                        fbe_api_terminator_device_handle_t*     out_first_drive_info_p)
{

    fbe_object_id_t             raid_group_id;
    fbe_status_t                 status;


    // The test verification sequence is
    // 1. Add a debug hook which prevent raid group to go into broken state during eval RL.
    // 2. Pull the disk from second position.
    // 3. Wait for the debug hook to hit. Once debug hook hit, we are sure that raid group is just about 
    //     to go in fail state based on one position has reported an error and another position is failed in RG.
    // 4. Pull disk from first position. 
    // 5. Delete the debug hook.
    // 6. Verify that lun and rg are in FAIL state.
    // 7. Verify that both the positions are not marked for RL.
    // 8. If it is dual sp test, it verify the step 6 & 7 on passive SP.

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "dora test:  R5 Not rb logging on error test start");

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // set the debug hook which prevent to go rg in fail state during eval RL 
    dora_add_debug_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,  
                                FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN);

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing disk at position %d in the RG(object id 0x%x)", 
            __FUNCTION__, in_position_second,raid_group_id);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;


    //  Remove 1st drive in the RG and verify the drive state changes
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position_second, 
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_second_drive_info_p); 

    // wait for the debug hook to hit 
    status = dora_wait_for_target_SP_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,  
                                FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    // Note that we cannot validate the status since depending on which drive pull we are on
    // the RG might be in either Activate or FAIL.

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing disk at position %d in the RG(object id 0x%x)", 
            __FUNCTION__, in_position_first, raid_group_id);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove 2nd in the RG and verify the drive state changes 
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position_first,
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_first_drive_info_p);  


    // delete the debug hook 
    dora_delete_debug_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,  
                                FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN);

    //  Verify that the RAID and LUN objects are in the FAILED state 
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_FAIL);

    //  Verify that rb logging is NOT set for the drive removed from second position
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_second);


    //  Verify that rb logging is not set for the drive removed from first position
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_first);


    // verification on passive sp
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the FAILED state 
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_FAIL);

        //  Verify that rb logging is not set for the drive at second position
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_second);

        //  Verify that rb logging is NOT set for the drive at first position
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_first);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "dora test:  R5 Not rb logging on error test complete\n");

    //  Return
    return;

} //  End dora_test_not_rb_logging_r5_on_io_error()


/*!****************************************************************************
 *  dora_test_not_rb_logging_r5_on_io_error
 ******************************************************************************
 *
 * @brief
 *   Test that rb logging is NOT set for a drive when one drive has reported an error and we remove 
 *   the another drive from a RAID-5 RG (when the RG is enabled). There are timing windows where
 *   we do not want to mark NR on a drive failure.
 *
 * @param in_rg_config_p                 - pointer to the RG configuration info
 * @param in_position_first               - first position of disk to remove
 * @param in_position_second           - second position of disk to remove
 * @param out_first_drive_info_p       - pointer to structure that gets filled in with the 
 *                                                     "drive info" for the first drive
 * @param out_second_drive_info_p   - pointer to structure that gets filled in with the 
 *                                                     "drive info" for the second drive * 
 * @return  None 
 *
 * @Note    This test expect that error was inserted on drive 3rd and 4th position before calling it.
 *
 *****************************************************************************/
void dora_test_not_rb_logging_r6_on_io_error(
                        fbe_test_rg_configuration_t*                 in_rg_config_p,
                        fbe_u32_t                                           in_position_fifth,
                        fbe_u32_t                                           in_position_fourth,
                        fbe_u32_t                                           in_position_third,
                        fbe_api_terminator_device_handle_t*     out_fifth_drive_info_p,
                        fbe_api_terminator_device_handle_t*     out_fourth_drive_info_p,             
                        fbe_api_terminator_device_handle_t*     out_third_drive_info_p)
{

    fbe_object_id_t             raid_group_id;
    fbe_status_t                 status;


    // The test verification sequence is
    // 1. Add a debug hook which prevent raid group to go into broken state during eval RL.
    // 2. Pull the disk from fifth position.
    // 3. Wait for the debug hook to hit. Once debug hook hit, we are sure that raid group is just about 
    //     to go in fail state based on two position has reported an error and another position is failed in RG.
    // 4. Pull disk from fourth position. 
    // 5. Pull disk from third position. 
    // 6. Delete the debug hook.
    // 7. Verify that lun and rg are in FAIL state.
    // 8. Verify that both all removed drive positions are not marked for RL.
    // 9. If it is dual sp test, it verify the step 7 & 8 on passive SP.

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "dora test: R6  Not rb logging on error test start");

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // set the debug hook which prevent to go rg in fail state during eval RL 
    dora_add_debug_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,  
                                FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN);


    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing disk at position %d in the RG(object id 0x%x)",
                                            __FUNCTION__, in_position_fifth,raid_group_id);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove 1st drive in the RG and verify the drive state changes 
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position_fifth, 
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_fifth_drive_info_p); 

    // wait for the debug hook to hit 
    status = dora_wait_for_target_SP_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,  
                                FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    // Note that we cannot validate the status since depending on which drive pull we are on
    // the RG might be in either Activate or FAIL.

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing disk at position %d in the RG(object id 0x%x)", 
            __FUNCTION__, in_position_fourth, raid_group_id);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove 2nd drive in the RG and verify the drive state changes 
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position_fourth,
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_fourth_drive_info_p);  


    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing disk at position %d in the RG(object id 0x%x)", 
            __FUNCTION__, in_position_third, raid_group_id);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove 3rd drive in the RG and verify the drive state changes 
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position_third,
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_third_drive_info_p);  

    // delete the debug hook 
    dora_delete_debug_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,  
                                FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN);

    //  Verify that the RAID and LUN objects are in the FAILED state 
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_FAIL);

    //  Verify that rb logging is not set for the drive removed from fifth position
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_fifth);


    //  Verify that rb logging is not set for the drive removed from fourth position
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_fourth);


    //  Verify that rb logging is not set for the drive removed from third position
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_third);


    // verification on passive sp
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the FAILED state 
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_FAIL);

        //  Verify that rb logging is not set for the drive at fifth position
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_fifth);

        //  Verify that rb logging is NOT set for the drive at fourth position
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_fourth);

        //  Verify that rb logging is not set for the drive removed from third position
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position_third);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "dora test: R6  Not rb logging on error test complete\n");

    //  Return
    return;

} //  End dora_test_not_rb_logging_r5_on_io_error()




/*!****************************************************************************
 *  dora_test_rg_shutdown_r5_r3_r1
 ******************************************************************************
 *
 * @brief
 *   Test that the RAID group is shutdown (the object is in the failed state)
 *   when a second drive is removed in a RAID-5, RAID_3 or RAID_1; or a third 
 *   drive is removed in a triple mirror RAID-1. 
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - position of disk to remove
 * @param out_drive_info_p      - pointer to structure that gets filled in with the 
 *                                "drive info" for the drive
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_test_rg_shutdown_r5_r3_r1(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_u32_t                               in_position,
                            fbe_api_terminator_device_handle_t*     out_drive_info_p)
{

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Removing second drive (third for a triple mirror) in the RG\n", 
            __FUNCTION__);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;

    //  Remove another drive in the RG and verify the drive state changes 
    sep_rebuild_utils_remove_drive_and_verify(in_rg_config_p,
                                              in_position,
                                              sep_rebuild_utils_number_physical_objects_g,
                                              out_drive_info_p);  

    //  Verify that the RAID and LUN objects are in the FAILED state 
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_FAIL);

    //  Verify that rb logging is not set for the drive 
    sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the FAILED state 
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_FAIL);

        //  Verify that rb logging is not set for the drive 
        sep_rebuild_utils_verify_not_rb_logging(in_rg_config_p, in_position);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End dora_test_rg_shutdown_r5_r3_r1()

/*!****************************************************************************
 *  dora_test_rg_restore_to_degraded_r5_r3_r1
 ******************************************************************************
 *
 * @brief
 *   Test that the RG becomes ready when the second drive to fail is reinserted
 *   in a RAID-5, RAID-3 or RAID-1 or when the third drive to fail is reinserted 
 *   in a triple mirror RAID-1.
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - position of disk to remove
 * @param in_drive_info_p       - pointer to structure with the "drive info" for the
 *                                drive
 *
 * @return  None 
 *
 *****************************************************************************/
void dora_test_rg_restore_to_degraded_r5_r3_r1(
                            fbe_test_rg_configuration_t*            in_rg_config_p,
                            fbe_u32_t                               in_position,    
                            fbe_api_terminator_device_handle_t*     in_drive_info_p)
{

    //  Print message as to where the test is at 
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinserting second drive (third for a triple mirror) in the RG\n", 
                __FUNCTION__);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;

    //  Reinsert the last drive that was removed and verify drive state changes 
    sep_rebuild_utils_reinsert_drive_and_verify(in_rg_config_p,
                                                in_position, 
                                                sep_rebuild_utils_number_physical_objects_g,
                                                in_drive_info_p);   
               
    //  Verify that the RAID and LUN objects are in the READY state 
    sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_sim_transport_connection_target_t   local_sp;               // local SP id
        fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

        //  Get the ID of the local SP. 
        local_sp = fbe_api_sim_transport_get_target_server();

        //  Get the ID of the peer SP. 
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        //  Set the target server to the peer and do verification there 
        fbe_api_sim_transport_set_target_server(peer_sp);

        //  Verify that the RAID and LUN objects are in the READY state 
        sep_rebuild_utils_check_raid_and_lun_states(in_rg_config_p, FBE_LIFECYCLE_STATE_READY);

        //  Set the target server back to the local SP 
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    //  Return
    return;

} //  End dora_test_rg_restore_to_degraded_r5_r3_r1()

/*!**************************************************************************
 * dora_test_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to write background pattern
 *
 * @return void
 *
 ***************************************************************************/
static void dora_test_write_background_pattern( 
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index)
{
    fbe_api_rdgen_context_t*                    context_p = &dora_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                                   lun_index;
    fbe_object_id_t                             lun_object_id;
    fbe_test_logical_unit_configuration_t*      logical_unit_configuration_p = NULL;

    /* Send I/O to each LUN in this raid group
     */
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        /* Fill each LUN in the raid group will a fixed pattern
         */
        fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_ZERO_ONLY,  /* write zeroes */
                                                FBE_LBA_INVALID, /* use max capacity */
                                                SEP_REBUILD_UTILS_ELEMENT_SIZE);

        /* Goto next LUN.
         */
        context_p++;
        logical_unit_configuration_p++;
    }

    /* Run our I/O test.
     */
    context_p = &dora_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "dora background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End dora_test_write_background_pattern()


/*!**************************************************************
 * dora_test_inject_error_record()
 ****************************************************************
 * @brief
 *  Injects an error record
 * 
 * @param in_rg_config_p    - raid group config information
 * @param in_position       - disk position in RG to inject error
 * @param in_lba            - LBA on disk to inject error
 * @param in_rg_width       - width of RG
 *
 * @return None.
 *
 ****************************************************************/
static void dora_test_inject_error_record(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u16_t                       in_position,
                                    fbe_lba_t                       in_lba,
                                    fbe_u16_t                       in_rg_width)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_logical_error_injection_record_t    error_record = {0};
    

    //  Disable error injection records in the error injection table (initializes records)
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable the error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    //  Setup error record for error injection
    //  Inject error on given disk position and LBA
    error_record.pos_bitmap = 1 << in_position;                 //  Injecting error on given disk position
    error_record.width      = in_rg_width;                      //  RG width
    error_record.lba        = in_lba;                           //  Physical address to begin errors
    error_record.blocks     = in_rg_config_p->capacity;                                //  Blocks to extend errors
    error_record.err_type   = FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR;   // Error type
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;          // Error mode
    error_record.err_count  = 0;                                //  Errors injected so far
    error_record.err_limit  = SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP;    //  Limit of errors to inject
    error_record.skip_count = 0;                                //  Limit of errors to skip
    error_record.skip_limit = 0;                                //  Limit of errors to inject
    error_record.err_adj    = 0;                                //  Error adjacency bitmask
    error_record.start_bit  = 0;                                //  Starting bit of an error
    error_record.num_bits   = 0;                                //  Number of bits of an error
    error_record.bit_adj    = 0;                                //  Indicates if erroneous bits adjacent
    error_record.crc_det    = 0;                                //  Indicates if CRC detectable

    //  Create error injection record
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable error injection on the RG
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection: pos %d, lba %llu ==", __FUNCTION__, in_position, (unsigned long long)in_lba);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Check error injection stats
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    return;

}   // End dora_test_inject_error_record()


/*!**************************************************************************
 * dora_test_start_io
 ***************************************************************************
 * @brief
 *  This function issues I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 * @param in_lun_object_id          - LUN object ID; FBE_OBJECT_ID_INVALID if want to issue I/O to all LUNs
 *
 * @return void
 *
 ***************************************************************************/
static void dora_test_start_io(     
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &dora_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                              *outstanding_io_p = &dora_outstanding_async_io[in_rdgen_context_rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;



    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {
        // Validate that asyncrounous I/O has been stopped
        MUT_ASSERT_INT_EQUAL(0, *outstanding_io_p);

        //  Send a single I/O to the LU object provided as input
        status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                                  in_lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  in_rdgen_op,
                                                  FBE_RDGEN_LBA_SPEC_FIXED,
                                                  in_lba,
                                                  in_num_blocks,
                                                  FBE_RDGEN_OPTIONS_INVALID);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        // Increment the outstanding I/O count.
        *outstanding_io_p = *outstanding_io_p + 1;
        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
   
        // Validate that asyncrounous I/O has been stopped
        MUT_ASSERT_INT_EQUAL(0, *outstanding_io_p);
         
        //  Send a single I/O to the LU object
        status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  in_rdgen_op,
                                                  FBE_RDGEN_LBA_SPEC_FIXED,
                                                  in_lba,
                                                  in_num_blocks,
                                                  FBE_RDGEN_OPTIONS_INVALID);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Increment the outstanding I/O count.
        *outstanding_io_p = *outstanding_io_p + 1;

        // Goto the next LUN
        context_p++;
        outstanding_io_p++;
        logical_unit_configuration_p++;
    }

    return;

}   // End dora_test_start_io()

/*!**************************************************************************
 * dora_test_stop_io
 ***************************************************************************
 * @brief
 *  This function stops I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 * @param in_lun_object_id          - LUN object ID; FBE_OBJECT_ID_INVALID if want to issue I/O to all LUNs
 *
 * @return void
 *
 ***************************************************************************/
static void dora_test_stop_io(     
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &dora_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                              *outstanding_io_p = &dora_outstanding_async_io[in_rdgen_context_rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;


    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {

        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);

        //  Stop a single I/O to the LU object provided as input
        status = fbe_api_rdgen_stop_tests(context_p, 1);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
 
        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);
           
        //  Stop a single I/O to the LU object
        status = fbe_api_rdgen_stop_tests(context_p, 1);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        // Goto the next LUN
        context_p++;
        outstanding_io_p++;
        logical_unit_configuration_p++;
    }

    return;

}   // End dora_test_stop_io()

/*!**************************************************************************
 * dora_test_cleanup_io
 ***************************************************************************
 * @brief
 *  This function cleansup I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 * @param in_lun_object_id          - LUN object ID; FBE_OBJECT_ID_INVALID if want to issue I/O to all LUNs
 *
 * @return void
 *
 ***************************************************************************/
static void dora_test_cleanup_io(     
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &dora_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                              *outstanding_io_p = &dora_outstanding_async_io[in_rdgen_context_rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;


    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {

        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);

        // Destroy all the contexts (even if we got an error).
        status = fbe_api_rdgen_test_context_destroy(context_p);
    
        // Make sure the I/O was cleaned up successfully.
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
 
        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);
     
        // Destroy all the contexts (even if we got an error).
        status = fbe_api_rdgen_test_context_destroy(context_p);
    
        // Make sure the I/O was cleaned up successfully.
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);              

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        // Goto the next LUN
        context_p++;
        outstanding_io_p++;
        logical_unit_configuration_p++;
    }

    return;

}   // End dora_test_cleanup_io()

/*!**************************************************************
 * dora_test_wait_for_error_injection_complete()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete
 *
 * @param   in_rg_config_p      - raid group config information
 * @param   in_err_record_cnt   - number of errors injected
 *
 * @return  None
 *
 ****************************************************************/
static void dora_test_wait_for_error_injection_complete(
                                               fbe_test_rg_configuration_t*    in_rg_config_p,
                                               fbe_u16_t                       in_err_record_cnt)
{
    fbe_status_t                                        status;
    fbe_u32_t                                           sleep_count;
    fbe_u32_t                                           sleep_period_ms;
    fbe_u32_t                                           max_sleep_count;
    fbe_object_id_t                                     rg_object_id;
    fbe_api_logical_error_injection_get_stats_t         stats; 


    //  Intialize local variables
    sleep_period_ms = 100;
    max_sleep_count = (SEP_REBUILD_UTILS_MAX_ERROR_INJECT_WAIT_TIME_SECS * 1000) / sleep_period_ms;

    //  Get RG object ID
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    //  Wait for errors to be injected
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);
    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(stats.num_errors_injected >= in_err_record_cnt)
        {
            // Break out of the loop
            mut_printf(MUT_LOG_TEST_STATUS, "== %s found %llu errors injected ==\n", __FUNCTION__, (unsigned long long)stats.num_errors_injected);
            break;
        }
                        
        // Sleep 
        fbe_api_sleep(sleep_period_ms);
    }

    //  The error injection took too long and timed out
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    //  Disable the error injection service
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(in_rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} // End dora_test_wait_for_error_injection_complete()


/*!**************************************************************************
 * dora_test_verify_io_results
 ***************************************************************************
 * @brief
 *  This function verifies the results of issuing I/O to the LUNs in a RG
 *  using rdgen.
 * 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 *
 * @return void
 *
 ***************************************************************************/
static void dora_test_verify_io_results(
                                        fbe_u32_t   in_rdgen_context_rg_index)
{
    fbe_api_rdgen_context_t*                context_p = &dora_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_payload_block_operation_status_t    block_op_status;
    fbe_payload_block_operation_qualifier_t block_op_qualifier;
    fbe_status_t                            status;


    //  Wait for the IO's to complete before checking the status
    status = fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Make sure the rdgen status is success
    MUT_ASSERT_INT_EQUAL(context_p->status, FBE_STATUS_OK);

    //  Make sure the I/O status is success
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK); 
    
    //  Make sure the block status is sucess
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate rdgen I/O ==\n", __FUNCTION__);
    block_op_status = context_p->start_io.status.block_status;

    if (block_op_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                       __FUNCTION__, context_p->start_io.status.block_status);
        MUT_FAIL_MSG("block status from rdgen was unexpected");
    }

    //  Make sure the block qualifier is success
    block_op_qualifier = context_p->start_io.status.block_qualifier;
    if (block_op_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                       __FUNCTION__, context_p->start_io.status.block_status);
        MUT_FAIL_MSG("block qualifier status from rdgen was unexpected");
    }

    return;

}   // End dora_test_verify_io_results()

/*!****************************************************************************
 *  dora_quiesce_io_test 
 ******************************************************************************
 *
 * @brief
 *  This function runs a quiesce I/O test on the given RG for the specified number of iterations.
 * 
 * @param in_rg_config_p    - incoming RG               
 * @param iterations        - max number of quiesce/unquiesce iterations.               
 *
 * @return None
 * 
 *****************************************************************************/
static void dora_quiesce_io_test(fbe_test_rg_configuration_t*  in_rg_config_p,
                                 fbe_u32_t                     iterations)
{
    fbe_status_t                            status;
    fbe_sim_transport_connection_target_t   local_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_u32_t                               local_rdgen_context_rg_index;
    fbe_u32_t                               peer_rdgen_context_rg_index;
    fbe_api_rdgen_context_t*                local_context_p;
    fbe_api_rdgen_context_t*                peer_context_p;
    fbe_u32_t                               rdgen_abort_options = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_test_sep_io_dual_sp_mode_t          rdgen_peer_options = FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID;

    //  Get the ID of the local SP
    local_sp = fbe_api_sim_transport_get_target_server();

    //  Get the ID of the peer SP
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    //  Set the index in the rdgen context array for local SP
    local_rdgen_context_rg_index = 0;

    //  Set the context pointer for local SP for rdgen tests
    local_context_p = &dora_test_rdgen_context_g[local_rdgen_context_rg_index][0];

    //  Set the index in the rdgen context array for peer SP
    peer_rdgen_context_rg_index = local_rdgen_context_rg_index + 1;

    //  Set the context pointer for SP for rdgen tests on peer
    peer_context_p = &dora_test_rdgen_context_g[peer_rdgen_context_rg_index][0];

    //  Write a test pattern to the LUNs in the RG.
    //  Do this on one SP only.
    dora_test_write_background_pattern(in_rg_config_p, peer_rdgen_context_rg_index);

    //  If dual-SP, set the rdgen peer options accordingly
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        rdgen_peer_options = FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE;
    }

    //  Run I/O test on the local SP; it will load-balance I/O to the peer
    dora_test_sp_run_io_test(in_rg_config_p, 
                             local_context_p, 
                             local_rdgen_context_rg_index, 
                             local_sp, 
                             rdgen_abort_options,
                             rdgen_peer_options);

    //  Run quiesce test on the local SP on the specified RG for the specified number of iterations
    dora_test_quiesce_io_run_quiesce_unquiesce_test(in_rg_config_p, iterations, local_sp);

    // Stop I/O on local SP and make sure no errors
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop rdgen tests on SP %d",  __FUNCTION__, local_sp);
    status = fbe_api_rdgen_stop_tests(local_context_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(local_context_p->start_io.statistics.error_count, 0);

    //  Set target server back to first SP and let test finish normally
    fbe_api_sim_transport_set_target_server(local_sp);

    return;

} //  End dora_quiesce_io_test()


/*!****************************************************************************
 *  dora_test_sp_run_io_test 
 ******************************************************************************
 *
 * @brief
 *  This function runs rdgen tests on the given RG on the specified SP.
 * 
 * @param in_rg_config_p            - incoming RG
 * @param in_context_p              - pointer to context for rdgen test
 * @param in_rdgen_context_rg_index - index into rdgen context array
 * @param in_sp                     - SP               
 *
 * @return None
 * 
 *****************************************************************************/
static void dora_test_sp_run_io_test(fbe_test_rg_configuration_t*            in_rg_config_p,
                                     fbe_api_rdgen_context_t*                in_context_p, 
                                     fbe_u32_t                               in_rdgen_context_rg_index,
                                     fbe_sim_transport_connection_target_t   in_sp,
                                     fbe_u32_t                               in_abort_options,
                                     fbe_test_sep_io_dual_sp_mode_t          in_peer_options)
{
    fbe_status_t                status;
    fbe_bool_t                  abort_b = FBE_FALSE;
    fbe_bool_t                  peer_b  = FBE_FALSE;


    //  Set the target server to the incoming SP
    fbe_api_sim_transport_set_target_server(in_sp);

    //  Determine if aborts allowed for this test
    if (in_abort_options != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        abort_b = FBE_TRUE;
    }

    //  Configure the rdgen abort options
    status =  fbe_test_sep_io_configure_abort_mode(abort_b, in_abort_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Determine if peer I/O allowed for this test
    if (in_peer_options != FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID)
    {
        peer_b = FBE_TRUE;
    }

    //  Configure the rdgen peer options
    status = fbe_test_sep_io_configure_dualsp_io_mode(peer_b, in_peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Set context for tests
    fbe_test_sep_io_setup_standard_rdgen_test_context(in_context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       SEP_REBUILD_UTILS_THREADS_PER_LUN, /* threads per lun */
                                       SEP_REBUILD_UTILS_ELEMENT_SIZE,
                                       abort_b,
                                       peer_b);

    //  Run I/O test on the specified SP
    mut_printf(MUT_LOG_TEST_STATUS, "%s: run rdgen tests on SP %d",  __FUNCTION__, in_sp);
    status = fbe_api_rdgen_start_tests(in_context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

} //  End dora_test_sp_run_io_test()


/*!****************************************************************************
 *  dora_test_quiesce_io_run_quiesce_unquiesce_test 
 ******************************************************************************
 *
 * @brief
 *  This function runs a quiesce I/O test on the given RG on the specified SP.
 * 
 * @param in_rg_config_p            - incoming RG
 * @param in_iterations             - number of iterations of test
 * @param in_sp                     - SP               
 *
 * @return None
 * 
 *****************************************************************************/

static void dora_test_quiesce_io_run_quiesce_unquiesce_test(fbe_test_rg_configuration_t* in_rg_config_p,
                                                            fbe_u32_t in_iterations, 
                                                            fbe_sim_transport_connection_target_t in_sp)
{
    fbe_object_id_t             rg_object_id;
    fbe_u32_t                   test_count;
    fbe_status_t                status;


    //  Set the target server to the incoming SP
    fbe_api_sim_transport_set_target_server(in_sp);

    //  Get the object ID of the incoming RG on specified SP
    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    //  For each iteration of our test, quiesce and unquiesce I/O on the incoming RG
    for (test_count = 0; test_count < in_iterations; test_count++)
    {
        fbe_u32_t sleep_time = 1 + (fbe_random() % SEP_REBUILD_UTILS_QUIESCE_TEST_MAX_SLEEP_TIME);

        mut_printf(MUT_LOG_TEST_STATUS, "=== starting %s SP %d iteration %d sleep_time: %d msec===", 
                   __FUNCTION__, in_sp, test_count, sleep_time);
        EmcutilSleep(sleep_time);

        //  Quiesce I/O on the given RG
        status = fbe_api_raid_group_quiesce(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  If heavy load on system, can take a while for monitor to run to process the quiesce condition.
        //  Therefore, allow 60 seconds here for getting all quiesce flags set.
        status = fbe_test_sep_utils_wait_for_base_config_clustered_flags(rg_object_id, 
                                                                         SEP_REBUILD_UTILS_WAIT_SEC,
                                                                         FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                                         FBE_FALSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  Unquiesce I/O on the given RG
        status = fbe_api_raid_group_unquiesce(rg_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //  If heavy load on system, can take a while for monitor to run to process the unquiesce condition.
        //  Therefore, allow 60 seconds here for getting all quiesce flags unset.
        status = fbe_test_sep_utils_wait_for_base_config_clustered_flags(rg_object_id, 
                                                                         SEP_REBUILD_UTILS_WAIT_SEC,
                                                                         FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                                         FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

} //  End dora_test_quiesce_io_run_quiesce_unquiesce_test()


/*!**************************************************************
 * dora_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t dora_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = DORA_WAIT_TIME;
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS,
		       "We have hit the debug hook %llu times",
		       (unsigned long long)hook.counter);
            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state %d substate %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end dora_wait_for_target_SP_hook()
 ******************************************/
 
/*!**************************************************************
 * dora_delete_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return None 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void dora_delete_debug_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;

    hook.object_id = rg_object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    status = fbe_api_scheduler_get_debug_hook(&hook);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
	       (unsigned long long)hook.counter);

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook: rg obj id 0x%X state %d, substate %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                            state,
                            substate,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;
}
/******************************************
 * end dora_delete_debug_hook()
 ******************************************/

/*!**************************************************************
 * dora_add_debug_hook()
 ****************************************************************
 * @brief
 *  Add the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void dora_add_debug_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_status_t    status;

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook: rg obj id 0x%X state %d substate %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                            state,
                            substate,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return ;
}
/******************************************
 * end dora_add_debug_hook()
 ******************************************/


/*************************
 * end file dora_test.c
 *************************/
