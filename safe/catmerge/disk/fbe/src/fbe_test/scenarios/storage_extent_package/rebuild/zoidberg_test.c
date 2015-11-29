/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file zoidberg_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of rebuilds with SP failures.
 *
 * @version
 *  06/29/2011  - Jason White   Created
 *  03/21/2013  - Ron Proulx    Updated to use latest test infrastructure
 *                              and validate rebuild notifications.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "mut.h"                                    //  MUT unit testing infrastructure
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "sep_hook.h"                               //  Debug hook APIs
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_database_interface.h"         //  for fbe_api_database_lookup_raid_group_by_number
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_virtual_drive_interface.h"    //  automatic hot sparing
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* zoidberg_short_desc = "Rebuild error cases: SP failures";
char* zoidberg_long_desc =
    "\n"
    "\n"
    "The Zoidberg Scenario is a test of rebuild error cases due to disk failures.\n"
    "\n"
    "\n"
    "*** Zoidberg Phase 1 ********************************************************\n"
    "\n"
    "Phase 1 is a simple rebuild test for RAID-5. It tests that when a drive is\n"
    "removed and a Hot Spare swaps in for it and the hot spare is removed and reinserted,"
    " a rebuild is performed."
    "\n"
    "\n"
    "Dependencies:\n"
    "   - RAID group object can mark NR and handle RG shutdown (Dora scenario)\n"
    "   - VD is able to swap in a Hot Spare for a dead drive (Shaggy scenario)\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 4 SAS drives (PDOs)\n"
    "    [PP] 4 logical drives (LDs)\n"
    "    [SEP] 4 provision drives (PVDs)\n"
    "    [SEP] 3 virtual drives (VDs)\n"
    "    [SEP] 1 RAID object (RAID-5)\n"
    "    [SEP] 1 LUN object \n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Remove one of the drives in the RG (drive A)\n"
    "    - Remove the physical drive (PDO-A)\n"
    "    - Verify that VD-A is now in the FAIL state\n"
    "\n"
    "STEP 3: Write data to the LUN\n"
    "   - Use rdgen to write a seed pattern to the LUN\n"
    "   - Wait for writes to complete before proceeding\n"
    "\n"
    "STEP 4: Create a Hot Spare\n"
    "    - Verify that the Hot Spare begins to rebuild for the failed drive\n"
    "\n"
    "STEP 4: Remove and Re-insert the hot spare\n"
    "    - Verify that the rebuild restarts\n"
    "\n"
    "STEP 5: Verify completion of the rebuild operation\n"
    "    - Verify that the rebuild checkpoint is set to LOGICAL_EXTENT_END\n"
    "\n"
    "STEP 6: Verify that the rebuilt data is correct\n"
    "    - Use rdgen to validate that the rebuilt data is correct\n"
    "\n"
    "STEP 7: Cleanup\n"
    "    - Destroy objects\n"
    "\n"
    "\n"
        ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

/*!*******************************************************************
 * @def     ZOIDBERG_MAX_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief   Max number of drives we will test with.
 *
 *********************************************************************/
#define ZOIDBERG_MAX_RAID_GROUP_WIDTH       16

/*!*******************************************************************
 * @def     ZOIDBERG_TEST_MAX_DRIVES_TO_REMOVE
 *********************************************************************
 * @brief   Max number of drives that can be removed before the raid
 *          group goes broken.
 *
 *********************************************************************/
#define ZOIDBERG_TEST_MAX_DRIVES_TO_REMOVE  2

/*!*******************************************************************
 * @def     ZOIDBERG_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief   This test does not care about per LUN information.  Therefore
 *          only (1) LUN per raid group is required.
 *
 *********************************************************************/
#define ZOIDBERG_LUNS_PER_RAID_GROUP        1

/*!*******************************************************************
 * @def     ZOIDBERG_CHUNKS_PER_LUN
 *********************************************************************
 * @brief   Since this test validates the rebuild notifications the
 *          goal (within a 2GB memory limit).  So we use a large number
 *          of chunks per LUN.
 *
 *********************************************************************/
#define ZOIDBERG_CHUNKS_PER_LUN             20

/*!*******************************************************************
 * @def     ZOIDBERG_IO_SIZE
 *********************************************************************
 * @brief   Size (in blocks) for each rdgen I/O
 *
 *********************************************************************/
#define ZOIDBERG_IO_SIZE                    128

/*!*******************************************************************
 * @def     ZOIDBERG_FIXED_PATTERN
 *********************************************************************
 * @brief   rdgen fixed pattern to use
 *
 * @todo    Currently the only fixed pattern that rdgen supports is
 *          zeros.
 *
 *********************************************************************/
#define ZOIDBERG_FIXED_PATTERN              FBE_RDGEN_PATTERN_ZEROS

/*!*******************************************************************
 * @def     ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 3 encl + 34 pdo) 
 *
 *********************************************************************/
#define ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS 39 

/*!*******************************************************************
 * @var     zoidberg_rg_config_g
 *********************************************************************
 * @brief   This is the array of configurations we will loop over.
 *
 * @note    The capacity is ignored.  The capacity is based on chunks
 *          per LUN and number of LUNs.  The minimum width is used.
 *********************************************************************/
fbe_test_rg_configuration_t zoidberg_rg_config_g[] = 
{
    /* width, capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            5,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @def     ZOIDBERG_TEST_CONFIGS
 *********************************************************************
 * @brief   The number of raid groups under test.
 *
 *********************************************************************/
#define ZOIDBERG_TEST_CONFIGS               6

/*!*******************************************************************
 * @var zoidberg_pause_params
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t zoidberg_pause_params = { 0 };

/*!*******************************************************************
 * @var     zoidberg_b_is_AR566356_fixed
 *********************************************************************
 * @brief   Currently zoidberg does not stagger the multi-position
 *          rebuilds (by `stepping' debug hooks to guarantee which
 *          position will be rebuilt first).   
 *
 *********************************************************************/
static fbe_bool_t zoidberg_b_is_AR566356_fixed              = FBE_FALSE;    /*! @todo Add a sep util that `steps' drive insertion. */

/*!*******************************************************************
 * @struct  zoidberg_removed_drive_info_t
 *********************************************************************
 * @brief   Defines the `removed drive' information such as:
 *              o The raid group position
 *              o The associated pvd object id
 *              o etc
 * 
 *********************************************************************/
typedef struct zoidberg_removed_drive_info_s
{
    fbe_u32_t       rg_position[ZOIDBERG_TEST_MAX_DRIVES_TO_REMOVE];
    fbe_object_id_t pvd_object_id[ZOIDBERG_TEST_MAX_DRIVES_TO_REMOVE];
    fbe_u32_t       num_of_drives_removed;
    fbe_u32_t       first_index_to_rebuild;

} zoidberg_removed_drive_info_t;

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:

extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;
static void zoidberg_run_tests(fbe_test_rg_configuration_t *in_rg_config_p, void *context_p);

void zoidberg_remove_drive_and_verify(fbe_test_rg_configuration_t*        in_rg_config_p, 
                                      fbe_u32_t                           in_position, 
                                      fbe_u32_t                           in_num_objects, 
                                      fbe_api_terminator_device_handle_t* out_drive_info_p);

//  Context for rdgen.  This is an array with an entry for every LUN defined by the test.  The context is used
//  to initialize all the LUNs of the test in a single operation.
static fbe_api_rdgen_context_t fbe_zoidberg_test_context_g[ZOIDBERG_LUNS_PER_RAID_GROUP * ZOIDBERG_TEST_CONFIGS];
static zoidberg_removed_drive_info_t zoidberg_removed_drive_info[ZOIDBERG_TEST_CONFIGS];
static fbe_object_id_t               zoidberg_rg_object_ids[ZOIDBERG_TEST_CONFIGS];

/*!****************************************************************************
 *  zoidberg_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Zoidberg test.  The test does the
 *   following:
 *
 *   - Creates raid groups
 *   - For each raid group, tests rebuild of one or more drives with SP and
 *     array failures.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void zoidberg_dualsp_test(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    zoidberg_removed_drive_info_t  *removed_drive_info_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED      test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_lun_number_t                next_lun_id;        //  sets/gets number of the next lun (0, 1, 2)

    //  Initialize configuration setup parameters - start with 0
    next_lun_id = 0;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    //--------------------------------------------------------------------------
    //  Test RAID-5 - SP Dies / Array dies during rebuild
    //

    rg_config_p = &zoidberg_rg_config_g[0];
    removed_drive_info_p = &zoidberg_removed_drive_info[0];
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void *)removed_drive_info_p, zoidberg_run_tests,
                                                   ZOIDBERG_LUNS_PER_RAID_GROUP,
                                                   ZOIDBERG_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);


    //  Return
    return;

} //  End zoidberg_dualsp_test()


/*!****************************************************************************
 *  zoidberg_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Zoidberg test.  It creates the
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void zoidberg_dualsp_setup(void)
{
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_count;
    fbe_scheduler_debug_hook_t      hook;

    /* This test currently only runs in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        // Clear all previously set debug hooks
        fbe_api_scheduler_clear_all_debug_hooks(&hook);

        /* Set the proper array based on the test level.
         */
        if (test_level > 0)
        {
            /*! @note Currently the raid groups for extended testing are the same
             *        as those for qual.
             */
            rg_config_p= &zoidberg_rg_config_g[0];
        }
        else
        {
            /* Qual.
             */
            rg_config_p= &zoidberg_rg_config_g[0];
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the riad group information.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        /* Initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, raid_group_count);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, raid_group_count);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

        /* After sep is load setup notifications */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dualsp test */);

    }

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    fbe_test_sep_util_set_chunks_per_rebuild(1);

} //  End zoidberg_dualsp_setup()

/*!****************************************************************************
 *  zoidberg_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Zoidberg test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void zoidberg_dualsp_cleanup(void)
{

    /* Currently this test only runs in simulation
     */
    if (fbe_test_util_is_simulation())
    {
        /* Destroy semaphore
         */
        fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test*/);

        fbe_test_sep_util_print_dps_statistics();

        /* Always clear dualsp mode
         */
        fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;

} // End zoidberg_dualsp_cleanup()

/*!***************************************************************************
 *          zoidberg_write_background_pattern()
 *****************************************************************************
 *
 * @brief   Write a pattern to all LUNs 
 *
 * @param   rg_config_p - Pointer to raid groups under test  
 * @param   raid_group_count - Number of raid groups to test  
 *
 * @return  None
 *
 * @author
 *  03/15/2013 Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_write_background_pattern(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_u32_t raid_group_count)              
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *rdgen_context_p = &fbe_zoidberg_test_context_g[0];
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing background pattern ==", 
               __FUNCTION__);

    /* Issue a fixed pattern to a distributed area.
     */
    status = fbe_test_sep_rebuild_generate_distributed_io(rg_config_p, raid_group_count, rdgen_context_p,
                                                          FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                          FBE_RDGEN_PATTERN_LBA_PASS,
                                                          ZOIDBERG_IO_SIZE);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_context_p->start_io.statistics.error_count, 0);

    return;
}
/****************************************** 
 * end zoidberg_write_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          zoidberg_verify_background_pattern()
 *****************************************************************************
 *
 * @brief   Verify background pattern
 *
 * @param   rg_config_p - Pointer to raid groups under test  
 * @param   raid_group_count - Number of raid groups to test  
 *
 * @return  None
 *
 * @author
 *  03/15/2013 Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_verify_background_pattern(fbe_test_rg_configuration_t *rg_config_p, 
                                               fbe_u32_t raid_group_count)               
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *rdgen_context_p = &fbe_zoidberg_test_context_g[0];
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Verify background pattern ==", 
               __FUNCTION__);
    
    /* Issue a fixed pattern to a distributed area.
     */
    status = fbe_test_sep_rebuild_generate_distributed_io(rg_config_p, raid_group_count, rdgen_context_p,
                                                          FBE_RDGEN_OPERATION_READ_CHECK,
                                                          FBE_RDGEN_PATTERN_LBA_PASS,
                                                          ZOIDBERG_IO_SIZE);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_context_p->start_io.statistics.error_count, 0);

    return;
}
/****************************************** 
 * end zoidberg_verify_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          zoidberg_write_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Seed all the LUNs with a fixed (i.e. doesn't vary) pattern.
 *
 * @param   rg_config_p - Pointer to raid groups under test  
 * @param   raid_group_count - Number of raid groups to test   
 *
 * @return  None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void zoidberg_write_fixed_pattern(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *rdgen_context_p = &fbe_zoidberg_test_context_g[0];
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing fixed pattern ==", 
               __FUNCTION__);

    /* Issue a fixed pattern to a distributed area.
     */
    status = fbe_test_sep_rebuild_generate_distributed_io(rg_config_p, raid_group_count, rdgen_context_p,
                                                          FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                          ZOIDBERG_FIXED_PATTERN,
                                                          ZOIDBERG_IO_SIZE);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_context_p->start_io.statistics.error_count, 0);

    return;
}
/******************************************
 * end zoidberg_write_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          zoidberg_read_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Read all lUNs and validate fixed pattern
 *
 * @param   rg_config_p - Pointer to raid groups under test  
 * @param   raid_group_count - Number of raid groups to test   
 *
 * @return  None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void zoidberg_read_fixed_pattern(fbe_test_rg_configuration_t *rg_config_p,  
                                        fbe_u32_t raid_group_count)                
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *rdgen_context_p = &fbe_zoidberg_test_context_g[0];
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating fixed pattern ==", 
               __FUNCTION__);
    
    /* Issue a fixed pattern to a distributed area.
     */
    status = fbe_test_sep_rebuild_generate_distributed_io(rg_config_p, raid_group_count, rdgen_context_p,
                                                          FBE_RDGEN_OPERATION_READ_CHECK,
                                                          ZOIDBERG_FIXED_PATTERN,
                                                          ZOIDBERG_IO_SIZE);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_context_p->start_io.statistics.error_count, 0);

    return;
}
/******************************************
 * end zoidberg_read_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          zoidberg_remove_drives()
 *****************************************************************************
 *
 * @brief   This method removes the drives in a fashion (sequential, random, etc)
 *          specified and populate the `removed drive information'
 *
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   removed_drive_info_p - Pointer to array (per raid group) removed
 *              drive information.
 * @param   b_is_differential_rebuild - FBE_TRUE - Differential rebuild test
 *              (i.e. the same drives will be re-inserted) 
 *
 * @return  None
 *
 * @author
 *  03/15/2013 Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_remove_drives(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   zoidberg_removed_drive_info_t *removed_drive_info_p,
                                   fbe_bool_t b_is_differential_rebuild)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    zoidberg_removed_drive_info_t  *current_removed_info_p = removed_drive_info_p;
    fbe_u32_t                       removed_drive_index;
    fbe_u32_t                       rg_index = 0;
    fbe_u32_t                       position;
    fbe_object_id_t                 pvd_object_ids[ZOIDBERG_MAX_RAID_GROUP_WIDTH];
    fbe_u32_t                       num_drives_to_remove;
    fbe_test_drive_removal_mode_t   removal_mode = FBE_DRIVE_REMOVAL_MODE_RANDOM;   /* By default use random mode */
    fbe_test_drive_removal_mode_t   current_removal_mode;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    /* If the `-usd_default_drives' option is set then use sequential mode
     */
    if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Using sequential remove mode");
        removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;
    }
    
    /* Walk thru all the raid group test configurations and:
     *  o If differential rebuild remember the original drive information
     *  
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)           || 
            (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)    )
        {
            current_rg_config_p++;
            current_removed_info_p++;
            continue;
        }

        /* Make sure our objects are in the ready state.
         */
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        /* Get the pvd object ids for all positions
         */
        for (position = 0; position < current_rg_config_p->width; position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position].bus,
                                                                current_rg_config_p->rg_disk_set[position].enclosure,
                                                                current_rg_config_p->rg_disk_set[position].slot,
                                                                &pvd_object_ids[position]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Remove the drives.
         */
        current_removal_mode = removal_mode;
        mut_printf(MUT_LOG_TEST_STATUS, "== remove drives for rg: 0x%x ==", zoidberg_rg_object_ids[rg_index]);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
        {
            num_drives_to_remove = 2;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            num_drives_to_remove = 0;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Raid 10 goes sequential.
             */
            current_removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;
            num_drives_to_remove = 1;
        }
        else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
        {
            num_drives_to_remove = current_rg_config_p->width - 1;
        }
        else
        {
            num_drives_to_remove = 1;
        }
        big_bird_remove_all_drives(current_rg_config_p, 1, num_drives_to_remove,
                                   b_is_differential_rebuild,   /* Caller determines if drive is re-inserted*/
                                   0,                           /* msec wait between removals */
                                   current_removal_mode         /* Removal mode determined above */);

        /* Populate the drive removal infomation
         */
        current_removed_info_p->num_of_drives_removed = num_drives_to_remove;
        current_removed_info_p->first_index_to_rebuild = num_drives_to_remove - 1;
        for (removed_drive_index = 0; removed_drive_index < num_drives_to_remove; removed_drive_index++)
        {
            fbe_u32_t   removed_position;

            removed_position = current_rg_config_p->drives_removed_array[removed_drive_index];
            current_removed_info_p->rg_position[removed_drive_index] = removed_position;
            current_removed_info_p->pvd_object_id[removed_drive_index] = pvd_object_ids[removed_position];
        }

        /* Goto next raid group
         */
        current_rg_config_p++;
        current_removed_info_p++;
    }

    return;
}
/******************************** 
 * end zoidberg_remove_drives()
 *******************************/

/*!***************************************************************************
 *          zoidberg_insert_drives()
 *****************************************************************************
 *
 * @brief   This method inserts the drives in a fashion (sequential, random, etc)
 *          specified and populate the `removed drive information'
 *
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   removed_drive_info_p - Pointer to array (per raid group) removed
 *              drive information.
 * @param   b_is_differential_rebuild - FBE_TRUE - Differential rebuild test
 *              (i.e. the same drives will be re-inserted) 
 * @param   num_of_drives_to_insert - Number of drives to insert
 *
 * @return  None
 *
 * @author
 *  03/15//2013 Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   zoidberg_removed_drive_info_t *removed_drive_info_p,
                                   fbe_bool_t b_is_differential_rebuild,
                                   fbe_u32_t num_of_drives_to_insert)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       removed_drive_index;
    fbe_object_id_t pvd_object_ids[ZOIDBERG_MAX_RAID_GROUP_WIDTH];
    fbe_u32_t       position;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    /*! @note Walk thru all the raid group test configurations and:
     *          o If differential rebuild remember the original drive information
     *  
     *        Currently there is a bug in big_bird_insert_all_drives that
     *        ignores the `num_of_drives_to_insert' parameter.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, 
                               num_of_drives_to_insert,     /* Only insert this many drives */
                               b_is_differential_rebuild    /* Insert the same (differential rebuild) or different (full rebuild) drive*/);


    /* If this is a full rebuild we need to refresh the disk and extra disk info.
     */
    if (b_is_differential_rebuild == FBE_FALSE)
    {
        /* Get `this' and other SP.
         */
        this_sp = fbe_api_sim_transport_get_target_server();
        other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
        MUT_ASSERT_TRUE(raid_group_count == 1);

        /* Refresh the raid group disks 
         */
        fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

        fbe_test_sep_util_wait_for_all_objects_ready(rg_config_p);
        fbe_api_sim_transport_set_target_server(other_sp);
        fbe_test_sep_util_wait_for_all_objects_ready(rg_config_p);
        fbe_api_sim_transport_set_target_server(this_sp);


        /* Get the pvd object ids for all positions
         */
        for (position = 0; position < rg_config_p->width; position++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                                    rg_config_p->rg_disk_set[position].enclosure,
                                                                    rg_config_p->rg_disk_set[position].slot,
                                                                    &pvd_object_ids[position]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Populate the drive removal infomation
         */
        for (removed_drive_index = 0; removed_drive_index < removed_drive_info_p->num_of_drives_removed; removed_drive_index++)
        {
            fbe_u32_t   removed_position;

            removed_position = removed_drive_info_p->rg_position[removed_drive_index];
            removed_drive_info_p->pvd_object_id[removed_drive_index] = pvd_object_ids[removed_position];
        }
    }

    return;
}
/******************************** 
 * end zoidberg_insert_drives()
 *******************************/

/*!***************************************************************************
 *          zoidberg_reboot()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/25/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_reboot(fbe_sep_package_load_params_t *sep_params_p)
{   
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_preserve_data = FBE_TRUE;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are in dualsp mode (the normal mode on the array), shutdown
     * the current SP to transfer control to the other SP.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        /* Mark `peer dead' so that we don't wait for notifications from the 
         * peer.
         */
        status = fbe_test_common_notify_mark_peer_dead(b_is_dualsp_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If both SPs are enabled shutdown this SP and validate that the other 
         * SP takes over.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Dualsp: %d shutdown SP: %d make current SP: %d ==", 
                   __FUNCTION__, b_is_dualsp_mode, this_sp, other_sp);

        /* Only use the `quick' destroy if `preserve data' is not set.
         */
        if (b_preserve_data == FBE_TRUE)
        {
            fbe_test_sep_util_destroy_neit_sep();
        }
        else
        {
            fbe_api_sim_transport_destroy_client(this_sp);
            fbe_test_sp_sim_stop_single_sp(this_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);
        }

        /* Set the transport server to the correct SP.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(this_sp, other_sp);
    }
    else
    {
        /* Else simply reboot this SP. (currently no specific reboot params for NEIT).
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot current SP: %d ==", 
                   __FUNCTION__, this_sp);
        status = fbe_test_common_reboot_this_sp(sep_params_p, NULL /* No params for NEIT*/); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }  

    return status;
}
/******************************************
 * end zoidberg_reboot()
 ******************************************/

/*!***************************************************************************
 *          zoidberg_reboot_cleanup()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/25/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_reboot_cleanup(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t                               test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t            *rg_config_p = NULL;
    fbe_bool_t                              b_preserve_data = FBE_TRUE;
    fbe_bool_t                              test_hs;

    /* Get the array of array test configs.
     */
    if (test_level > 0)
    {
        /* Extended.
         */
        rg_config_p = &zoidberg_rg_config_g[0];
    }
    else
    {
        /* Qual.
         */
        rg_config_p = &zoidberg_rg_config_g[0];
    }

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are single SP mode reboot this SP.
     */
    if (b_is_dualsp_mode == FBE_FALSE)
    {
        /* If we are in single SP mode there currently is no cleanup neccessary.
         */
    }
    else
    {
        /* Mark the peer `alive' so that we wait for notifications on both SPs.
         */
        status = fbe_test_common_notify_mark_peer_alive(b_is_dualsp_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Else we are in dualsp mode. Boot the `other' SP and use/set the same
         * debug hooks that we have for this sp.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot other SP: %d ==", 
                   __FUNCTION__, other_sp);

        /* Only use the `quick' create if `preserve data' is not set.
         */
        if (b_preserve_data == FBE_TRUE)
        {
            status = fbe_test_common_boot_sp(other_sp, 
                                             sep_params_p,
                                             NULL   /* No neit params*/);
        }
        else
        {
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_test_boot_sp(rg_config_p, other_sp);
        }

        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_TRUE(sep_rebuild_utils_number_physical_objects_g == ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
        status = fbe_test_sep_util_wait_for_database_service(50000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
        status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Change all unused `spares' to `unconsumed'.
         */
        status = fbe_test_sep_util_unconfigure_all_spares_in_the_system();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_provision_drive_get_config_hs_flag(&test_hs);
        MUT_ASSERT_TRUE(test_hs);
    }  

    return status;
}
/******************************************
 * end zoidberg_reboot_cleanup()
 ******************************************/

/*!****************************************************************************
 *  zoidberg_dual_sp_crash
 ******************************************************************************
 *
 * @brief
 *   Shutdown sep and neit on both sps
 *
 * @param   sep_params_p - pointer to the parameters (debug hooks) to start the sps with
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   b_is_differential_rebuild - FBE_TRUE - This is a differential rebuild
 *              so the removed drives will be re-inserted.
 *
 * @return  None
 *
 *****************************************************************************/
static void zoidberg_dual_sp_crash(fbe_sep_package_load_params_t *sep_params_p,
                                   fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   zoidberg_removed_drive_info_t *removed_drive_info_p,
                                   fbe_bool_t b_is_differential_rebuild) {
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_status_t                            status;
    fbe_bool_t                              test_hs;

    /* Destroy semaphore
     */
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test*/);

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Dualsp crash==", __FUNCTION__);

    /* shutdown both sps 
     */
    fbe_api_sim_transport_set_target_server(this_sp);
    fbe_test_sep_util_destroy_neit_sep();
    fbe_api_sim_transport_set_target_server(other_sp);
    fbe_test_sep_util_destroy_neit_sep();

    /* boot up both sps 
     */
    status = fbe_test_common_boot_sp(this_sp, 
                                     sep_params_p,
                                     NULL   /* No neit params*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_common_boot_sp(other_sp, 
                                     sep_params_p,
                                     NULL   /* No neit params*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_wait_for_database_service(50000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_set_chunks_per_rebuild(1); 
    fbe_api_sleep(20000);

    /* After sep is load setup notifications */
    fbe_test_common_setup_notifications(FBE_TRUE /* This is a dualsp test */);

    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Change all unused `spares' to `unconsumed'.
     */
    status = fbe_test_sep_util_unconfigure_all_spares_in_the_system();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_provision_drive_get_config_hs_flag(&test_hs);
    MUT_ASSERT_TRUE(test_hs);

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/*******************************
 * end zoidberg_dual_sp_crash()
 *******************************/

/*!****************************************************************************
 *  zoidberg_remove_drive_and_verify
 ******************************************************************************
 *
 * @brief
 *    This function removes a drive.  It waits for the logical and physical
 *    drive objects to be destroyed.  
 *
 * @param in_rg_config_p        - pointer to the RG configuration info
 * @param in_position           - disk position in the RG
 * @param in_num_objects        - the number of physical objects that should exist
 *                                after the disk is removed
 * @param out_drive_info_p      - pointer to structure that gets filled in with the
 *                                "drive info" for the drive
 * @return None
 *
 *****************************************************************************/
void zoidberg_remove_drive_and_verify(fbe_test_rg_configuration_t*        in_rg_config_p, 
                                      fbe_u32_t                           in_position, 
                                      fbe_u32_t                           in_num_objects, 
                                      fbe_api_terminator_device_handle_t* out_drive_info_p)
{
    fbe_status_t                status;                             // fbe status
    fbe_sim_transport_connection_target_t   local_sp;               // local SP id
    fbe_sim_transport_connection_target_t   peer_sp;                // peer SP id

    /*  Get the ID of the local SP. 
     */
    local_sp = fbe_api_sim_transport_get_target_server();

    /*  Get the ID of the peer SP. 
     */
    if (FBE_SIM_SP_A == local_sp)
    {
        peer_sp = FBE_SIM_SP_B;
    }
    else
    {
        peer_sp = FBE_SIM_SP_A;
    }

    /*  Remove the drive
     */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(in_rg_config_p->rg_disk_set[in_position].bus,
                                             in_rg_config_p->rg_disk_set[in_position].enclosure,
                                             in_rg_config_p->rg_disk_set[in_position].slot,
                                             out_drive_info_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*  Set the target server to the peer and remove the drive there.  */
            fbe_api_sim_transport_set_target_server(peer_sp);

            status = fbe_test_pp_util_pull_drive(in_rg_config_p->rg_disk_set[in_position].bus,
                                                 in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                 in_rg_config_p->rg_disk_set[in_position].slot,
                                                 out_drive_info_p);
        }
        
        /*  Verify the PDO and LDO are destroyed by waiting for the number of physical objects to be equal to the
         *  value that is passed in. Note that drives are removed differently on hardware; this check applies
         *  to simulation only.
         */
        status = fbe_api_wait_for_number_of_objects(in_num_objects, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);       
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
                                                in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                in_rg_config_p->rg_disk_set[in_position].slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*  Set the target server to the peer and remove the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            status = fbe_test_pp_util_pull_drive_hw(in_rg_config_p->rg_disk_set[in_position].bus,
                                                    in_rg_config_p->rg_disk_set[in_position].enclosure,
                                                    in_rg_config_p->rg_disk_set[in_position].slot);
        }        
    }

    /*  Print message as to where the test is at */
    mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", 
               in_rg_config_p->rg_disk_set[in_position].bus,
               in_rg_config_p->rg_disk_set[in_position].enclosure,
               in_rg_config_p->rg_disk_set[in_position].slot);

    /*  Set the target server back to the local SP. */
    fbe_api_sim_transport_set_target_server(local_sp);

    return;

} 
/***********************************************
 * end zoidberg_remove_drive_and_verify()
 ***********************************************/

/*!****************************************************************************
 *          zoidberg_init_reboot_hooks()
 ******************************************************************************
 *
 * @brief   Zero all the reboot hooks (done at the start of each test)
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *  06/11/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_init_reboot_hooks(void)
{
    fbe_u32_t hook_index = 0;

    /* Zero the reboot parameters
     */
    fbe_zero_memory(&zoidberg_pause_params, sizeof(fbe_sep_package_load_params_t));

    /*! @note Currently the only load parameter that is preserved are the 
     *        debug hooks.
     */
    for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++)
    {
        /* Just mark the hook invalid
         */
        zoidberg_pause_params.scheduler_hooks[hook_index].object_id = FBE_OBJECT_ID_INVALID;
    }

    return;
}
/***********************************************
 * end zoidberg_init_reboot_hooks()
 ***********************************************/

/*!****************************************************************************
 *          zoidberg_add_reboot_hook()
 ******************************************************************************
 *
 * @brief   Save the rebuild hook with checkpoint_to_pause_at in the sep_params
 *
 * @param   rg_object_id  - raid group object
 * @param   monitor_state - Monitor state to pause at during boot
 * @param   monitor_substate - Monitor sub-state to pause at during boot
 * @param   checkpoint_to_pause_at - hook checkpoint
 * @param   val2 - Currently not used
 * @param   check_type - Check type to pause at during boot
 * @param   action - Action to pause at during boot
 *
 * @return  status - Hook was set
 *
 * @author
 *  05/22/2013  Deanna Heng - Created
 *
 *****************************************************************************/
static fbe_status_t zoidberg_add_reboot_hook(fbe_object_id_t rg_object_id,
                                             fbe_u32_t monitor_state,
                                             fbe_u32_t monitor_substate,
                                             fbe_lba_t checkpoint_to_pause_at, 
                                             fbe_u64_t val2,
                                             fbe_u32_t check_type,
                                             fbe_u32_t action)
{
    fbe_status_t    status = FBE_STATUS_INSUFFICIENT_RESOURCES; /* By default there was no hook slot available. */
    fbe_u32_t       hook_index = 0;

    /*! @note Currently the only load parameter that is preserved are the 
     *        debug hooks.
     */
    for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++)
    {
        /* Find a `free' slot
         */
        if ((zoidberg_pause_params.scheduler_hooks[hook_index].object_id == 0)                     ||
            (zoidberg_pause_params.scheduler_hooks[hook_index].object_id == FBE_OBJECT_ID_INVALID)    )
        {
            /* Free slot found.  Populate the hook and return success.
             */
            zoidberg_pause_params.scheduler_hooks[hook_index].object_id = rg_object_id;
            zoidberg_pause_params.scheduler_hooks[hook_index].monitor_state = monitor_state;
            zoidberg_pause_params.scheduler_hooks[hook_index].monitor_substate = monitor_substate;
            zoidberg_pause_params.scheduler_hooks[hook_index].val1 = checkpoint_to_pause_at;
            zoidberg_pause_params.scheduler_hooks[hook_index].val2 = val2;
            zoidberg_pause_params.scheduler_hooks[hook_index].check_type = check_type;
            zoidberg_pause_params.scheduler_hooks[hook_index].action = action;

            /* Return success.
             */
            return FBE_STATUS_OK;
        }
    }

    /* Report no slot found.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x monitor: %d substate: %d no slot (out of %d) !!",
               __FUNCTION__, rg_object_id, monitor_state, monitor_substate, MAX_SCHEDULER_DEBUG_HOOKS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/***********************************************
 * end zoidberg_add_reboot_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_set_rebuild_start_hook()
 *****************************************************************************
 *
 * @brief   Set a debug hook when the rebuild of the user area starts.
 *
 * @param   rg_object_id - object id of vd associated with hook
 * @param   b_set_on_active - If FBE_TRUE set the hook on the object' Active SP
 * @param   b_set_on_passive - If FBE_TRUE set the hook on the object' Passive SP
 * @param   b_set_on_reboot - If FBE_TRUE save this hook in the reboot parameters
 *
 * @return  None
 *
 * @author
 *  03/21/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_set_rebuild_start_hook(fbe_object_id_t rg_object_id,
                                            fbe_bool_t b_set_on_active,
                                            fbe_bool_t b_set_on_passive,
                                            fbe_bool_t b_set_on_reboot)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If requested, set the hook on the object's Active SP
     */
    if (b_set_on_active)
    {
        status = fbe_test_add_debug_hook_active(rg_object_id, 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                0,
                                                0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* If requested, set the hook on the object's Passive SP
     */
    if (b_set_on_passive)
    {
        status = fbe_test_add_debug_hook_passive(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                 0,
                                                 0,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* If requested add the debug hook to the boot parameters
     */
    if (b_set_on_reboot)
    {
        status = zoidberg_add_reboot_hook(rg_object_id,                                
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,  
                                          FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                          0,   
                                          0,                                        
                                          SCHEDULER_CHECK_STATES,
                                          SCHEDULER_DEBUG_ACTION_PAUSE);               
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x set on active: %d passive: %d reboot: %d ",
               __FUNCTION__, rg_object_id, b_set_on_active, b_set_on_passive, b_set_on_reboot);

    return;
}
/***********************************************
 * end zoidberg_set_rebuild_start_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_wait_for_rebuild_start_hook()
 *****************************************************************************
 *
 * @brief   Wait a debug hook when the rebuild of the user area starts.
 *
 * @param   rg_object_id - object id of vd associated with hook
 * @param   b_wait_on_active - If FBE_TRUE set the hook on the object' Active SP
 * @param   b_wait_on_passive - If FBE_TRUE set the hook on the object' Passive SP
 *
 * @return  None
 *
 * @author
 *  06/12/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_wait_for_rebuild_start_hook(fbe_object_id_t rg_object_id,
                                                 fbe_bool_t b_wait_on_active,
                                                 fbe_bool_t b_wait_on_passive)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If requested, wait for the hook on the object's Active SP
     */
    if (b_wait_on_active)
    {
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                     FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0,
                                                     0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* If requested, wait for the hook on the object's Passive SP
     */
    if (b_wait_on_passive)
    {
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                     FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0,
                                                     0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x wait on active: %d passive: %d ",
               __FUNCTION__, rg_object_id, b_wait_on_active, b_wait_on_passive);

    return;
}
/***********************************************
 * end zoidberg_wait_for_rebuild_start_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_clear_rebuild_start_hook()
 *****************************************************************************
 *
 * @brief   Clear a debug hook when the rebuild of the user area starts.
 *
 * @param   rg_object_id - object id of vd associated with hook
 * @param   b_set_on_active - If FBE_TRUE set the hook on the object' Active SP
 * @param   b_set_on_passive - If FBE_TRUE set the hook on the object' Passive SP
 *
 * @return  None
 *
 * @author
 *  03/21/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_clear_rebuild_start_hook(fbe_object_id_t rg_object_id,
                                              fbe_bool_t b_clear_on_active,
                                              fbe_bool_t b_clear_on_passive)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If requested, clear the hook on the object's Active SP
     */
    if (b_clear_on_active)
    {
        status = fbe_test_del_debug_hook_active(rg_object_id, 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                0,
                                                0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* If requested, clear the hook on the object's Passive SP
     */
    if (b_clear_on_passive)
    {
        status = fbe_test_del_debug_hook_passive(rg_object_id, 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                0,
                                                0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x clear on active: %d passive: %d",
               __FUNCTION__, rg_object_id, b_clear_on_active, b_clear_on_passive);

    return;
}
/***********************************************
 * end zoidberg_clear_rebuild_start_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_set_percent_rebuilt_hook()
 *****************************************************************************
 * @brief
 *  This function determines the lba for the rebuild hook based
 *  on the user space lba passed.
 *
 * @param   rg_object_id - object id of vd associated with hook
 * @param   percent_rebuilt_before_pause - Percentage rebuilt before we pause
 * @param   b_set_on_active - If FBE_TRUE set the hook on the object' Active SP
 * @param   b_set_on_passive - If FBE_TRUE set the hook on the object' Passive SP
 * @param   b_set_on_reboot - If FBE_TRUE save this hook in the reboot parameters
 *
 * @return  lba - lba of hook
 *
 * @author
 *  03/21/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_lba_t zoidberg_set_percent_rebuilt_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t percent_rebuilt_before_pause,
                                                   fbe_bool_t b_set_on_active,
                                                   fbe_bool_t b_set_on_passive,
                                                   fbe_bool_t b_set_on_reboot)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lba_t       checkpoint_to_pause_at = FBE_LBA_INVALID;

    /* If the requested checkpoint is 0 or larger than 100 it is not valid.
     */
    if (((fbe_s32_t)percent_rebuilt_before_pause <= 0)   ||
        (percent_rebuilt_before_pause            >  100)    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: rg obj: 0x%x pause percent: %d not valid (must range from 1 to 100)",
                   __FUNCTION__, rg_object_id, percent_rebuilt_before_pause);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return 0;
    }

    /* Use common API to determine the `pause at' lba.
     */
    status = fbe_test_sep_rebuild_get_raid_group_checkpoint_to_pause_at(rg_object_id,
                                                                        percent_rebuilt_before_pause,
                                                                        &checkpoint_to_pause_at);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* If requested set on the active
     */
    if (b_set_on_active)
    {
        status = fbe_test_add_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                checkpoint_to_pause_at,
                                                0,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If requested set on the passive
     */
    if (b_set_on_passive)
    {
        status = fbe_test_add_debug_hook_passive(rg_object_id,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                 checkpoint_to_pause_at,
                                                 0,
                                                 SCHEDULER_CHECK_VALS_GT,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If requested add the debug hook to the boot parameters
     */
    if (b_set_on_reboot)
    {
        status = zoidberg_add_reboot_hook(rg_object_id,                                 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,   
                                          FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,        
                                          checkpoint_to_pause_at,                       
                                          0,                                            
                                          SCHEDULER_CHECK_VALS_GT,                      
                                          SCHEDULER_DEBUG_ACTION_PAUSE);                
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x pause percent: %d lba: 0x%llx active: %d passive: %d reboot: %d",
               __FUNCTION__, rg_object_id, percent_rebuilt_before_pause, 
               (unsigned long long)checkpoint_to_pause_at, 
               b_set_on_active, b_set_on_passive, b_set_on_reboot);

    /* Return the checkpoint to stop at.
     */
    return checkpoint_to_pause_at;
}
/***********************************************
 * end zoidberg_set_percent_rebuilt_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_wait_for_percent_rebuilt_hook()
 *****************************************************************************
 *
 * @brief   Wait a debug hook when the rebuild of the user area starts.
 *
 * @param   rg_object_id - object id of vd associated with hook
 * @param   checkpoint_to_pause_at - Checkpoint before we pause
 * @param   b_wait_on_active - If FBE_TRUE set the hook on the object' Active SP
 * @param   b_wait_on_passive - If FBE_TRUE set the hook on the object' Passive SP
 *
 * @return  None
 *
 * @author
 *  06/12/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_wait_for_percent_rebuilt_hook(fbe_object_id_t rg_object_id,
                                                   fbe_lba_t checkpoint_to_pause_at,
                                                   fbe_bool_t b_wait_on_active,
                                                   fbe_bool_t b_wait_on_passive)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If requested, wait for the hook on the object's Active SP
     */
    if (b_wait_on_active)
    {
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                     FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                     SCHEDULER_CHECK_VALS_GT,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     checkpoint_to_pause_at,
                                                     0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* If requested, wait for the hook on the object's Passive SP
     */
    if (b_wait_on_passive)
    {
        status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                                     FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                     SCHEDULER_CHECK_VALS_GT,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     checkpoint_to_pause_at,
                                                     0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     
    }

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x wait on active: %d passive: %d ",
               __FUNCTION__, rg_object_id, b_wait_on_active, b_wait_on_passive);

    return;
}
/***********************************************
 * end zoidberg_wait_for_percent_rebuilt_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_clear_percent_rebuilt_hook()
 *****************************************************************************
 * @brief
 *  This function determines the lba for the rebuild hook based
 *  on the user space lba passed.
 *
 * @param   rg_object_id - object id of vd associated with hook
 * @param   checkpoint_to_pause_at - Checkpoint set in the set debug hook
 * @param   b_clear_on_active - If FBE_TRUE clear the hook on the object' Active SP
 * @param   b_clear_on_passive - If FBE_TRUE clear the hook on the object' Passive SP
 *
 * @return  lba - lba of hook
 *
 * @author
 *  03/21/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_clear_percent_rebuilt_hook(fbe_object_id_t rg_object_id, 
                                                fbe_lba_t checkpoint_to_pause_at,
                                                fbe_bool_t b_clear_on_active,
                                                fbe_bool_t b_clear_on_passive)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: rg obj: 0x%x checkpoint: 0x%llx active: %d passive: %d",
               __FUNCTION__, rg_object_id, 
               (unsigned long long)checkpoint_to_pause_at,
               b_clear_on_active, b_clear_on_passive);

    /* If requested clear on the active
     */
    if (b_clear_on_active)
    {
        status = fbe_test_del_debug_hook_active(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                checkpoint_to_pause_at,
                                                0,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If requested clear on the passive
     */
    if (b_clear_on_passive)
    {
        status = fbe_test_del_debug_hook_passive(rg_object_id,
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                checkpoint_to_pause_at,
                                                0,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
/***********************************************
 * end zoidberg_clear_percent_rebuilt_hook()
 ***********************************************/

/*!***************************************************************************
 *          zoidberg_validate_rebuild_start()
 *****************************************************************************
 *
 * @brief   Hook as been set to stop rebuild process (on Active SP).  Either
 *          re-insert removed drive or replace failed drive and then validate
 *          that the reconstruct START notification is generated.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   b_is_differential_rebuild - FBE_TRUE - This is a differential rebuild
 * @param   caller - Function that called this method
 *
 * @return  status
 *
 * @author
 *  06/12/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_validate_rebuild_start(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t raid_group_count,
                                                    zoidberg_removed_drive_info_t *removed_drive_info_p,
                                                    fbe_bool_t b_is_differential_rebuild,
                                                    const char *caller)
{
    fbe_status_t                    status;
    fbe_test_rg_configuration_t    *current_rg_config_p;
    zoidberg_removed_drive_info_t  *current_removed_info_p;
    fbe_u32_t                       rebuild_index_to_wait_for;
    fbe_u32_t                       rg_index;
    fbe_notification_info_t         notification_info;
    fbe_object_id_t                 spare_object_id = FBE_OBJECT_ID_INVALID;

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s %s drives and validate reconstruct START notification ==",
               caller, (b_is_differential_rebuild) ? "Re-insert" : "Replace");

    /* Set notification to wait for (START) and then either re-insert or 
     * replace the drives.
     */
    current_rg_config_p = rg_config_p;              
    current_removed_info_p = removed_drive_info_p;  
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Update the objects adn determine the position.
         */
        sep_rebuild_utils_number_physical_objects_g += 1;
        rebuild_index_to_wait_for = current_removed_info_p->first_index_to_rebuild;

        /* If this is a full rebuild get the spare object id.
         */
        if (b_is_differential_rebuild == FBE_FALSE)
        {
            /* Update the pvd object id to wait for with the new one
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->extra_disk_set[rebuild_index_to_wait_for].bus,
                                                                    current_rg_config_p->extra_disk_set[rebuild_index_to_wait_for].enclosure,
                                                                    current_rg_config_p->extra_disk_set[rebuild_index_to_wait_for].slot,
                                                                    &spare_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, spare_object_id);

            /* Update the pvd to wait for.
             */
            current_removed_info_p->pvd_object_id[rebuild_index_to_wait_for] = spare_object_id;
        }

        /* Currently the `wait for notification' code can only wait for (1)
         * notification at a time.  Always wait for the last rebuild index.
         */
        status = fbe_test_common_set_notification_to_wait_for(current_removed_info_p->pvd_object_id[rebuild_index_to_wait_for],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Re-insert the removed drives and wait for the rebuilt to start
         */
        zoidberg_insert_drives(current_rg_config_p, 1, current_removed_info_p,
                               b_is_differential_rebuild,   /* This determines if differential or full rebuild */
                               current_removed_info_p->num_of_drives_removed /* Only insert (1) position at a time */);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Validate the reconstruct START notification.  Since the reconstruct
         * method runs before the rebuild and the passive will not run the rebuild
         * the state can either be START or progress.
         */
        MUT_ASSERT_INT_EQUAL(0, notification_info.notification_data.data_reconstruction.percent_complete);
        MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_START)       ||
                        (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_IN_PROGRESS)    );

        /* Goto next raid group and remove info
         */
        current_rg_config_p++;
        current_removed_info_p++;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/********************************************** 
 * end zoidberg_validate_rebuild_start()
 **********************************************/

/*!***************************************************************************
 *          zoidberg_determine_rebuild_position_to_wait_for()
 *****************************************************************************
 *
 * @brief   We have just shutdown the (CMI) Active SP.  Some of the raid group
 *          will transfer the rebuild process to the remaining SP.  We need to
 *          determine which position we need to wait for.  We simply let the
 *          rebuild start and then find the highest rebuild checkpoint.
 *
 * @param   rg_object_id - The raid group object id
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   target_checkpoint - The target checkpoint to wait for
 * @param   removed_index_p - Pointer to removed index to wait for.
 *
 * @return  status
 *
 * @author
 *  03/24/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_determine_rebuild_position_to_wait_for(fbe_object_id_t rg_object_id,
                                                                    zoidberg_removed_drive_info_t *removed_drive_info_p,
                                                                    fbe_lba_t target_checkpoint)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_lba_t                       highest_rebuild_checkpoint = 0;
    fbe_u32_t                       highest_rebuilt_position = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t                       position;
    fbe_u32_t                       removed_drive_index;

    /* Simply remove the start hook and wait for target checkpoint.
     */
    zoidberg_clear_rebuild_start_hook(rg_object_id, 
                                      FBE_TRUE, /* Clear hook on object's Active SP */
                                      FBE_FALSE /* The other SP is shutdown, can't clear the hook */);

    /* Wait for the first rebuild progress hook
     */
    zoidberg_wait_for_percent_rebuilt_hook(rg_object_id,
                                           target_checkpoint,
                                           FBE_TRUE,  /* Wait on the active SP */
                                           FBE_FALSE  /* Do not wait on the passive */);

    /* Now get the raid group info and detemine the highest rebuilt position.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now find the highest rebuild position.
     */
    for (position = 0; position < rg_info.width; position++) {
        if ((rg_info.rebuild_checkpoint[position] != FBE_LBA_INVALID)            &&
            (rg_info.rebuild_checkpoint[position] >= highest_rebuild_checkpoint)    ) {
            /*! @note We must handle multiple positions with the same checkpoint.
             *        Simply trace a message that a `parallel' rebuild is in progress
             *        and select the last position.
             */
            if ((highest_rebuilt_position != FBE_TEST_SEP_UTIL_INVALID_POSITION)     &&
                (highest_rebuild_checkpoint == rg_info.rebuild_checkpoint[position])    ) {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "== %s rg obj: 0x%x chkpt: 0x%llx parallel rebuild pos: %d pos: %d ==",
                           __FUNCTION__, rg_object_id, highest_rebuild_checkpoint,
                           highest_rebuilt_position, position);
            }
            highest_rebuild_checkpoint = rg_info.rebuild_checkpoint[position];
            highest_rebuilt_position = position;
        }
    }
    MUT_ASSERT_TRUE((highest_rebuild_checkpoint != 0)          &&
                    (highest_rebuilt_position < rg_info.width)    );

    /* Now find the removed drive index we should use.
     */
    MUT_ASSERT_TRUE(removed_drive_info_p->num_of_drives_removed > 0);
    for (removed_drive_index = 0; removed_drive_index < removed_drive_info_p->num_of_drives_removed; removed_drive_index++) {
        if (removed_drive_info_p->rg_position[removed_drive_index] == highest_rebuilt_position) {
            break;
        }
    }
    MUT_ASSERT_TRUE(removed_drive_index < removed_drive_info_p->num_of_drives_removed);

    /* Print a message
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s rg obj: 0x%x removed: %d index: %d pvd obj: 0x%x ==",
               __FUNCTION__, rg_object_id, removed_drive_info_p->num_of_drives_removed,
               removed_drive_index, removed_drive_info_p->pvd_object_id[removed_drive_index]);

    /* Update the index ot wait for.
     */
    removed_drive_info_p->first_index_to_rebuild;

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*************************************************************
 * end zoidberg_determine_rebuild_position_to_wait_for()
 *************************************************************/

/*!***************************************************************************
 *          zoidberg_validate_rebuild_continues_after_sp_shutdown()
 *****************************************************************************
 *
 * @brief   We have just shutdown the (CMI) Active SP.  Some of the raid group
 *          will transfer the rebuild process to the remaining SP.  Validate
 *          that we do not see a second START reconstruct notification and that
 *          we see the proper reconstruct progress notification.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   start_checkpoint_p - pointer to the checkpoints for the rebuild hooks
 * @param   target_checkpoint_p - pointer to the checkpoints for the rebuild hooks
 * @param   caller - Function that called this method
 *
 * @return  status
 *
 * @author
 *  05/22/2013  Deanna Heng - Updated.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_validate_rebuild_continues_after_sp_shutdown(fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t raid_group_count,
                                                                          zoidberg_removed_drive_info_t *removed_drive_info_p,
                                                                          fbe_lba_t *start_checkpoint_p,
                                                                          fbe_lba_t *target_checkpoint_p,
                                                                          const char *caller)
{
    fbe_status_t                    status;
    zoidberg_removed_drive_info_t  *current_removed_info_p;
    fbe_u32_t                       rg_index;
    fbe_notification_info_t         notification_info;
    fbe_bool_t                      b_is_dualsp_test_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validate rebuild continues after SP shutdown ==",
               caller);

    /* Set the notification to wait for then remove the rebuild start hook and
     * validate that the proper reconstruct progress notification is generated.
     * Since there is only (1) SP remaining, set the test mode to single SP.
     */
    FBE_UNREFERENCED_PARAMETER(rg_config_p);
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    current_removed_info_p = removed_drive_info_p;  
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Invoke method that determines which position is rebuilding.
         */
        status = zoidberg_determine_rebuild_position_to_wait_for(zoidberg_rg_object_ids[rg_index],
                                                                 current_removed_info_p,
                                                                 *start_checkpoint_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the notification to wait for.
         */
        status = fbe_test_common_set_notification_to_wait_for(current_removed_info_p->pvd_object_id[current_removed_info_p->first_index_to_rebuild],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now clear the rebuild start hook.
         */
        zoidberg_clear_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                            *start_checkpoint_p,
                                            FBE_TRUE,   /* Clear the rebuild percent hook on the Active */
                                            FBE_FALSE   /* Don't clear on passive (there is no passive) */);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       120000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct progress notification.
         */
        MUT_ASSERT_TRUE(notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_IN_PROGRESS);
        MUT_ASSERT_INT_NOT_EQUAL(0, notification_info.notification_data.data_reconstruction.percent_complete);

        /* Wait for the first rebuild progress hook
         */
        zoidberg_wait_for_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index],
                                               *target_checkpoint_p,
                                               FBE_TRUE,  /* Wait on the active SP */
                                               FBE_FALSE  /* Do not wait on the passive */);

        /* Set a debug hook when the rebuild starts on the Active so that
         * the debug hooks are consistent on both SPs when the other SP is
         * booted.
         */
        zoidberg_set_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                        FBE_FALSE,  /* Don't set the hook on the Passive SP */
                                        FBE_TRUE    /* Stop the booting SP at the rebuild start */);

        /* Now clear the rebuild hook for the checkpoint requested.
         */
        zoidberg_clear_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                            *target_checkpoint_p,
                                            FBE_TRUE,   /* Clear the rebuild percent hook on the Active */
                                            FBE_FALSE   /* Don't clear on passive (there is no passive) */);

        /* Goto next
         */
        current_removed_info_p++;
        *start_checkpoint_p++;
        target_checkpoint_p++;
    }

    /* Now set the target test  mode back to dualsp
     */
    fbe_test_sep_util_set_dualsp_test_mode(b_is_dualsp_test_mode);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*************************************************************
 * end zoidberg_validate_rebuild_continues_after_sp_shutdown()
 *************************************************************/

/*!***************************************************************************
 *          zoidberg_validate_rebuild_continues_after_sp_boot()
 *****************************************************************************
 *
 * @brief   All raid group are active on the remaining SP.  Boot the SP that
 *          was previously shutdown and validate that (some) of the raid groups
 *          that become active on the booted SP continue the rebuild (i.e. do
 *          not restart).
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   percent_rebuilt_before_boot - This was the percent rebuilt before the
 *              shutdown SP is booted.  We expect that the percent should be at
 *              least this amount.
 * @param   target_checkpoint_p - pointer to the checkpoints for the rebuild hooks
 * @param   caller - Function that called this method
 *
 * @return  status
 *
 * @author
 *  06/12/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_validate_rebuild_continues_after_sp_boot(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count,
                                                                      zoidberg_removed_drive_info_t *removed_drive_info_p,
                                                                      fbe_u32_t percent_rebuilt_before_boot,
                                                                      fbe_lba_t *target_checkpoint_p,
                                                                      const char *caller)
{
    fbe_status_t                    status;
    zoidberg_removed_drive_info_t  *current_removed_info_p;
    fbe_u32_t                       rebuild_index_to_wait_for;
    fbe_u32_t                       rg_index;
    fbe_notification_info_t         notification_info;

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validate rebuild continues from: %d percent after SP boot ==",
               caller, percent_rebuilt_before_boot);

    /* We have just booted the SP so wait for the raid groups to be Ready on
     * both SPs.  We have a set a debug hook to stop the rebuild process when
     * it starts.
     */
    FBE_UNREFERENCED_PARAMETER(rg_config_p);
    current_removed_info_p = removed_drive_info_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* We have just booted the other SP.  Wait for this raid group to be
         * Ready on BOTH SPs.
        */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              zoidberg_rg_object_ids[rg_index], 
                                                                              FBE_LIFECYCLE_STATE_READY, 
                                                                              60000, 
                                                                              FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for rebuild process to start on the Active.
         */
        zoidberg_wait_for_rebuild_start_hook(zoidberg_rg_object_ids[rg_index],
                                             FBE_TRUE,  /* Wait on the active SP */
                                             FBE_FALSE  /* Do not wait on the passive */);

        /* Currently the `wait for notification' code can only wait for (1)
         * notification.  We have determined the current rebuild index.
         */
        rebuild_index_to_wait_for = current_removed_info_p->first_index_to_rebuild;

        status = fbe_test_common_set_notification_to_wait_for(current_removed_info_p->pvd_object_id[rebuild_index_to_wait_for],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Clear the rebuild start hook (since it was set for the boot SP we must
         * clear it on the active and passive).
         */
        zoidberg_clear_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                          FBE_TRUE, /* Clear hook on object's Active SP */
                                          FBE_TRUE  /* Clear hook on the passive SP also */);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct progress notification.
         */
        MUT_ASSERT_INT_EQUAL(DATA_RECONSTRUCTION_IN_PROGRESS, notification_info.notification_data.data_reconstruction.state);
        MUT_ASSERT_TRUE(notification_info.notification_data.data_reconstruction.percent_complete >= percent_rebuilt_before_boot);

        /* Goto the next
         */
        current_removed_info_p++;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*********************************************************
 * end zoidberg_validate_rebuild_continues_after_sp_boot()
 *********************************************************/

/*!***************************************************************************
 *          zoidberg_validate_rebuild_completes_after_sp_boot()
 *****************************************************************************
 *
 * @brief   We have booted the SP that was previously shutdown and we have
 *          validated that the rebuild continued from where it left off.  Now
 *          validate that the reconstruct END notification is generated on
 *          both SPs.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   percent_rebuilt_after_boot - This was the percent rebuilt before the
 *              shutdown SP is booted.  We expect that the percent should be at
 *              least this amount.
 * @param   target_checkpoint_p - pointer to the checkpoints for the rebuild hooks
 * @param   caller - Function that called this method
 *
 * @return  status
 *
 * @author
 *  06/12/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t zoidberg_validate_rebuild_completes_after_sp_boot(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count,
                                                                      zoidberg_removed_drive_info_t *removed_drive_info_p,
                                                                      fbe_u32_t percent_rebuilt_after_boot,
                                                                      fbe_lba_t *target_checkpoint_p,
                                                                      const char *caller)
{
    fbe_status_t                    status;
    fbe_test_rg_configuration_t    *current_rg_config_p;
    zoidberg_removed_drive_info_t  *current_removed_info_p;
    fbe_u32_t                       rebuild_index_to_wait_for;
    fbe_u32_t                       rg_index;
    fbe_notification_info_t         notification_info;

    /* Trace some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validate rebuild completes after SP boot ==",
               caller);

    /* We have validated that the rebuild continues after the previously
     * shutdown SP is booted.  Now validate progress or reconstruct END
     * notification is properly generated.
     */
    current_rg_config_p = rg_config_p;
    current_removed_info_p = removed_drive_info_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Wait for rebuild hook to be hit
         */
        zoidberg_wait_for_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index],
                                               *target_checkpoint_p,
                                               FBE_TRUE,  /* Wait on the active SP */
                                               FBE_FALSE  /* Do not wait on the passive */);

        /* Currently the `wait for notification' code can only wait for (1)
         * notification at a time.  We have already determined the first index
         * to rebuild.
         */
        rebuild_index_to_wait_for = current_removed_info_p->first_index_to_rebuild;
        status = fbe_test_common_set_notification_to_wait_for(current_removed_info_p->pvd_object_id[rebuild_index_to_wait_for],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Clear the rebuild percent hook 
         */
        zoidberg_clear_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                            *target_checkpoint_p,
                                            FBE_TRUE,   /* Clear the rebuild percent hook on the Active */
                                            FBE_TRUE    /* Hook was set on passive also */);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct progress or end notification.
         */
        if (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_IN_PROGRESS)
        {
            MUT_ASSERT_TRUE(notification_info.notification_data.data_reconstruction.percent_complete >= percent_rebuilt_after_boot);
        }
        else if (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_END)
        {
            /*! @note If the rebuild was not complete prior to the reboot the 
             *        percent should be 100.  If it was already completed it will be 0.
             */
            MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.percent_complete == 100) ||
                            (notification_info.notification_data.data_reconstruction.percent_complete == 0)      );
        }
        else
        {
            /* Fail the test
             */
            MUT_ASSERT_INT_EQUAL(DATA_RECONSTRUCTION_IN_PROGRESS, notification_info.notification_data.data_reconstruction.state);
        }

        /* Wait until the rebuilds finish
         */
        sep_rebuild_utils_wait_for_rb_comp(current_rg_config_p, current_removed_info_p->rg_position[rebuild_index_to_wait_for]);
        sep_rebuild_utils_check_bits(zoidberg_rg_object_ids[rg_index]);

        /* Goto next
         */
        current_rg_config_p++;
        current_removed_info_p++;
        target_checkpoint_p++;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*********************************************************
 * end zoidberg_validate_rebuild_completes_after_sp_boot()
 *********************************************************/

/*!****************************************************************************
 *  zoidberg_wait_for_rebuilds_to_complete
 ******************************************************************************
 *
 * @brief
 *   Check that the proper notfications are being sent for the rebuilding pvd
 *   and verify that rebuild completes on the raid groups
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 * @param   target_checkpoint_p - pointer to the checkpoints for the rebuild hooks
 * @param   is_dual_sp_failure - whether the test had failed both sps or not
 *
 * @return  None
 *
 * @author
 *  05/22/2013  Deanna Heng - Created.
 *
 *****************************************************************************/
static void zoidberg_wait_for_rebuilds_to_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t raid_group_count,
                                                   zoidberg_removed_drive_info_t *removed_drive_info_p,
                                                   fbe_lba_t *target_checkpoint_p,
                                                   fbe_bool_t is_dual_sp_failure)
{
    fbe_status_t                            status;
    fbe_u32_t                               rebuild_index_to_wait_for;
    fbe_u32_t                               rg_index;
    fbe_notification_info_t                 notification_info;


    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Verify the SEP objects is in expected lifecycle State  */
        status = fbe_api_wait_for_object_lifecycle_state(zoidberg_rg_object_ids[rg_index],
                                                         FBE_LIFECYCLE_STATE_READY,
                                                         60000,
                                                         FBE_PACKAGE_ID_SEP_0);

        /* Currently the `wait for notification' code can only wait for (1)
         * notification at a time.  We have alredy determined the first index
         * to rebuild.
         */
        rebuild_index_to_wait_for = removed_drive_info_p->first_index_to_rebuild;

        status = fbe_test_common_set_notification_to_wait_for(removed_drive_info_p->pvd_object_id[rebuild_index_to_wait_for],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Clear the rebuild percent hook 
         */
        zoidberg_clear_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                            *target_checkpoint_p,
                                            FBE_TRUE,   /* Clear the rebuild percent hook on the Active */
                                            is_dual_sp_failure   /* Clear the hook on the passive sp if dual sp failure */);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct progress or end notification.
         */
        if (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_IN_PROGRESS)
        {
            MUT_ASSERT_INT_NOT_EQUAL(0, notification_info.notification_data.data_reconstruction.percent_complete);
        }
        else if (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_END)
        {
            /*! @note If the rebuild was not complete prior to the reboot the 
             *        percent should be 100.  If it was already completed it will be 0.
             */
            MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.percent_complete == 100) ||
                            (notification_info.notification_data.data_reconstruction.percent_complete == 0)      );
        }
        else
        {
            /* Fail the test
             */
            MUT_ASSERT_INT_EQUAL(DATA_RECONSTRUCTION_IN_PROGRESS, notification_info.notification_data.data_reconstruction.state);
        }

        /* Wait until the rebuilds finish
         */
        sep_rebuild_utils_wait_for_rb_comp(rg_config_p, removed_drive_info_p->rg_position[rebuild_index_to_wait_for]);
        sep_rebuild_utils_check_bits(zoidberg_rg_object_ids[rg_index]);

        rg_config_p++;
        removed_drive_info_p++;
        target_checkpoint_p++;
    }
}
/********************************************** 
 * end zoidberg_wait_for_rebuilds_to_complete()
 **********************************************/
/*!****************************************************************************
 *  zoidberg_test_diff_rebuild_sp_failure
 ******************************************************************************
 *
 * @brief
 *   This tests that a rebuild will continue on the other SP after the local SP
 *   is killed.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 *
 * @return  None
 *
 *****************************************************************************/
static void zoidberg_test_diff_rebuild_sp_failure(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_u32_t raid_group_count,
                                                  zoidberg_removed_drive_info_t *removed_drive_info_p)
{
    fbe_status_t                            status;
    fbe_sim_transport_connection_target_t   orig_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t                               rg_index;
    fbe_lba_t                               rebuild_start_checkpoint_after_sp_shutdown[ZOIDBERG_TEST_CONFIGS];
    fbe_lba_t                               rebuild_checkpoint_after_sp_shutdown[ZOIDBERG_TEST_CONFIGS];
    fbe_lba_t                               rebuild_checkpoint_after_sp_boot[ZOIDBERG_TEST_CONFIGS];
    fbe_u32_t                               start_percent_rebuilt_after_sp_shutdown =  5;  /* Pause (prior to SP shutdown) after  5% rebuilt. */
    fbe_u32_t                               percent_rebuilt_after_sp_shutdown       = 10;  /* Pause (prior to SP shutdown) after 10% rebuilt. */
    fbe_u32_t                               percent_rebuilt_after_sp_boot           = 20;  /* Pause (after SP is booted ) after  20% rebuilt. */

    /* Print message as to where the test is at and zero the reboot hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);
    zoidberg_init_reboot_hooks();

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    orig_sp = this_sp;
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, this_sp);
    mut_printf(MUT_LOG_LOW, "=== ACTIVE(local) side (%s) ===", this_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP 
     */
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /*  Get the number of physical objects in existence at this point in time.  This number is
     *  used when checking if a drive has been removed or has been reinserted.
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sep_rebuild_utils_number_physical_objects_g == ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS);

    /* go through all the RG for which user has provided configuration data. */
    zoidberg_write_background_pattern(rg_config_p, raid_group_count);

    /* Remove the drive and populate the removed information.
     */
    zoidberg_remove_drives(rg_config_p, raid_group_count, removed_drive_info_p,
                           FBE_TRUE /* This is a differential rebuild test*/);

    /* Validate the background pattern */
    zoidberg_verify_background_pattern(rg_config_p, raid_group_count);

    /* Write new data*/
    zoidberg_write_fixed_pattern(rg_config_p, raid_group_count);

    /* Add a debug hook when the rebuild starts (only on the Active)
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Set the `rebuild start' hook for the object's Active SP
         */
        zoidberg_set_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                        FBE_TRUE,   /* Set hook on passive so that when it becomes active it stops */
                                        FBE_FALSE   /* We will set the reboot rebuild hook later */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is shutdown.
         */
        rebuild_start_checkpoint_after_sp_shutdown[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                  start_percent_rebuilt_after_sp_shutdown,
                                                                                  FBE_TRUE, /* Set hook on object's Active SP */
                                                                                  FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                  FBE_FALSE /* Do not set this hook when the SP is booted */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is shutdown.
         */
        rebuild_checkpoint_after_sp_shutdown[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                  percent_rebuilt_after_sp_shutdown,
                                                                                  FBE_TRUE, /* Set hook on object's Active SP */
                                                                                  FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                  FBE_FALSE /* Do not set this hook when the SP is booted */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is booted.
         */
        rebuild_checkpoint_after_sp_boot[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                    percent_rebuilt_after_sp_boot,
                                                                                    FBE_TRUE, /* Set hook on object's Active SP */
                                                                                    FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                    FBE_TRUE  /* Set this hook when the SP is booted */);
    }

    /* Invoke the method to re-insert the drives and validate that the 
     * reconstruct START notification is properly generated.
     */
    status = zoidberg_validate_rebuild_start(rg_config_p, raid_group_count, removed_drive_info_p,
                                             FBE_TRUE,  /* This is a differential rebuild test */
                                             __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Shutdown the (CMI) Active SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP ==", __FUNCTION__);
    status = zoidberg_reboot(&zoidberg_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(other_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP - complete ==", __FUNCTION__);

    /* Validate that the rebuild continues (i.e. does not restart) after (some)
     * of the raid groups move the rebuild process to remaining SP.
     */
    status = zoidberg_validate_rebuild_continues_after_sp_shutdown(rg_config_p, raid_group_count, removed_drive_info_p,
                                                                   &rebuild_start_checkpoint_after_sp_shutdown[0], 
                                                                   &rebuild_checkpoint_after_sp_shutdown[0],
                                                                   __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Boot the orignal SP with the previously configured debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot this SP ==", __FUNCTION__);
    status = zoidberg_reboot_cleanup(&zoidberg_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot this SP - complete ==", __FUNCTION__);

    /* Now validate that the rebuild continued from where it left off.
     */
    status = zoidberg_validate_rebuild_continues_after_sp_boot(rg_config_p, raid_group_count, removed_drive_info_p,
                                                               percent_rebuilt_after_sp_shutdown,
                                                               &rebuild_checkpoint_after_sp_boot[0],
                                                               __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now validate that the reconstruct END notification is properly generated.
     */
    status = zoidberg_validate_rebuild_completes_after_sp_boot(rg_config_p, raid_group_count, removed_drive_info_p,
                                                               percent_rebuilt_after_sp_boot,
                                                               &rebuild_checkpoint_after_sp_boot[0],
                                                               __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear all the debug hooks.
     */
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Read the fixed pattern.
     */
    zoidberg_read_fixed_pattern(rg_config_p, raid_group_count);

    sep_rebuild_utils_validate_event_log_errors();

    return;
} 
/********************************************** 
 * end zoidberg_test_diff_rebuild_sp_failure()
 **********************************************/
/*!****************************************************************************
 *  zoidberg_test_diff_rebuild_array_failure
 ******************************************************************************
 *
 * @brief
 *   This tests that a rebuild will continue after the array comes back after
 *   being killed.
 * 
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 *
 * @return  None
 *
 * @author
 *  05/22/2013  Deanna Heng - Updated.
 *
 *****************************************************************************/
static void zoidberg_test_diff_rebuild_array_failure(fbe_test_rg_configuration_t *rg_config_p,            
                                                     fbe_u32_t raid_group_count,                          
                                                     zoidberg_removed_drive_info_t *removed_drive_info_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    zoidberg_removed_drive_info_t  *current_removed_info_p = removed_drive_info_p;
    fbe_u32_t                       rg_index;
    fbe_u32_t                       rebuild_index_to_wait_for;
    fbe_lba_t                       target_checkpoint[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                       percent_rebuilt_before_pause = 10;   /* Pause after 10% is rebuilt */
    fbe_notification_info_t         notification_info;
    fbe_bool_t                      is_dual_sp_failure = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);
    zoidberg_init_reboot_hooks();

    /*  Get the number of physical objects in existence at this point in time.  This number is
     *  used when checking if a drive has been removed or has been reinserted.
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sep_rebuild_utils_number_physical_objects_g == ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS);

    /* go through all the RG for which user has provided configuration data. */
    zoidberg_write_background_pattern(current_rg_config_p, raid_group_count);

    /* Remove the drive and populate the removed information.
     */
    zoidberg_remove_drives(rg_config_p, raid_group_count, removed_drive_info_p,
                           FBE_TRUE /* This is a differential rebuild test*/);

    /* Validate the background pattern */
    zoidberg_verify_background_pattern(current_rg_config_p, raid_group_count);

    /* Write new data*/
    zoidberg_write_fixed_pattern(current_rg_config_p, raid_group_count);

    /* Add a debug hook when the rebuild starts (only on the Active)
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Set the `rebuild start' hook for the object's Active SP
         */
        zoidberg_set_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                        FBE_FALSE,  /* Don't set the hook on the Passive SP */
                                        FBE_FALSE   /* Don't save the rebuild start hook */);

        /* Determine and set the target checkpoint for this raid group.
         */
        target_checkpoint[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                        percent_rebuilt_before_pause,
                                                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                                                        FBE_TRUE,   /* Don't set the hook on the Passive SP */
                                                                        FBE_TRUE    /* Save the rebuild hook during shutdown */);
    }

     mut_printf(MUT_LOG_TEST_STATUS, "Reinserting drives...");

    current_rg_config_p = rg_config_p;              
    current_removed_info_p = removed_drive_info_p;  
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        sep_rebuild_utils_number_physical_objects_g += 1;

        /* Currently the `wait for notification' code can only wait for (1)
         * notification at a time.  We have already determined the first index
         * to rebuild.
         */
        rebuild_index_to_wait_for = current_removed_info_p->first_index_to_rebuild;
        status = fbe_test_common_set_notification_to_wait_for(current_removed_info_p->pvd_object_id[rebuild_index_to_wait_for],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Re-insert the removed drives and waitr for the rebuilt to start
         */
        zoidberg_insert_drives(current_rg_config_p, 1, current_removed_info_p,
                               FBE_TRUE,    /* This is a differential rebuild */
                               current_removed_info_p->num_of_drives_removed            /* Only insert (1) position at a time */);


        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        /* Validate the reconstruct start notification.
         */
        MUT_ASSERT_INT_EQUAL(0, notification_info.notification_data.data_reconstruction.percent_complete);
        MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_START)       ||
                        (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_IN_PROGRESS)    );

        /* Clear the rebuild start hook and wait for the rebuilt to start
         */
        zoidberg_clear_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                          FBE_TRUE,   /* Clear hook on object's Active SP */
                                          FBE_FALSE   /* Don't set the hook on the Passive SP */);


        /* Wait for the rebuild to start (but don't wait for the rebuild hook)
         */
        fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(zoidberg_rg_object_ids[rg_index]);

        current_rg_config_p++;
        current_removed_info_p++;
    }

    /* Crash both SPs */
    current_rg_config_p = rg_config_p;
    current_removed_info_p = removed_drive_info_p;
    zoidberg_dual_sp_crash(&zoidberg_pause_params, current_rg_config_p, raid_group_count, current_removed_info_p, 
                           FBE_TRUE    /* This is a differential rebuild test*/);

    /* Validate that the rebuild continues on the new SP.
     */
    current_rg_config_p = rg_config_p;              
    current_removed_info_p = removed_drive_info_p;
    zoidberg_wait_for_rebuilds_to_complete(rg_config_p, raid_group_count, removed_drive_info_p, 
                                           &target_checkpoint[0], is_dual_sp_failure);

    /* Clear all the debug hooks.
     */
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Read the fixed pattern.
     */
    current_rg_config_p = rg_config_p;
    zoidberg_read_fixed_pattern(current_rg_config_p, raid_group_count);

    sep_rebuild_utils_validate_event_log_errors();

    return;

} 
/************************************************ 
 * end zoidberg_test_diff_rebuild_array_failure()
 ************************************************/

/*!****************************************************************************
 *          zoidberg_test_full_rebuild_sp_failure()
 ******************************************************************************
 *
 * @brief   This test starts a full rebuild on SPx then kills SPx then validates
 *          that rebuild completes on SPy.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 *
 * @return  None
 *
 * @author
 *  03/18/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_test_full_rebuild_sp_failure(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_u32_t raid_group_count,
                                                  zoidberg_removed_drive_info_t *removed_drive_info_p)
{
    fbe_status_t                            status;
    fbe_sim_transport_connection_target_t   orig_sp;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_u32_t                               rg_index;
    fbe_lba_t                               rebuild_start_checkpoint_after_sp_shutdown[ZOIDBERG_TEST_CONFIGS];
    fbe_lba_t                               rebuild_checkpoint_after_sp_shutdown[ZOIDBERG_TEST_CONFIGS];
    fbe_lba_t                               rebuild_start_checkpoint_after_sp_boot[ZOIDBERG_TEST_CONFIGS];
    fbe_lba_t                               rebuild_checkpoint_after_sp_boot[ZOIDBERG_TEST_CONFIGS];
    fbe_u32_t                               start_percent_rebuilt_after_sp_shutdown =  5;  /* Pause (prior to SP shutdown) after  5% rebuilt. */
    fbe_u32_t                               percent_rebuilt_after_sp_shutdown       = 10;  /* Pause (prior to SP shutdown) after 10% rebuilt. */
    fbe_u32_t                               start_percent_rebuilt_after_sp_boot     = 15;  /* Pause (after SP is booted ) after  15% rebuilt. */
    fbe_u32_t                               percent_rebuilt_after_sp_boot           = 20;  /* Pause (after SP is booted ) after  20% rebuilt. */

    /* Print message as to where the test is at and zero the reboot hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);
    zoidberg_init_reboot_hooks();

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    orig_sp = this_sp;
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, this_sp);
    mut_printf(MUT_LOG_LOW, "=== ACTIVE(local) side (%s) ===", this_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP 
     */
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /*  Get the number of physical objects in existence at this point in time.  This number is
     *  used when checking if a drive has been removed or has been reinserted.
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sep_rebuild_utils_number_physical_objects_g == ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS);

    /* Set `trigger spare timer' back to small value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5); /* 5 second hotspare timeout */

    /* go through all the RG for which user has provided configuration data. */
    zoidberg_write_background_pattern(rg_config_p, raid_group_count);

    /* Remove the drive and populate the removed information.
     */
    zoidberg_remove_drives(rg_config_p, raid_group_count, removed_drive_info_p,
                           FBE_FALSE /* This is a full rebuild test*/);

    /* Validate the background pattern */
    zoidberg_verify_background_pattern(rg_config_p, raid_group_count);

    /* Write new data*/
    zoidberg_write_fixed_pattern(rg_config_p, raid_group_count);

    /* Add a debug hook when the rebuild starts (only on the Active)
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Set the `rebuild start' hook for the object's Active SP
         */
        zoidberg_set_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                        FBE_TRUE,   /* Set hook on passive so that when it becomes active it stops */
                                        FBE_FALSE   /* We will set the reboot rebuild hook later */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is shutdown.
         */
        rebuild_start_checkpoint_after_sp_shutdown[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                  start_percent_rebuilt_after_sp_shutdown,
                                                                                  FBE_TRUE, /* Set hook on object's Active SP */
                                                                                  FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                  FBE_FALSE /* Do not set this hook when the SP is booted */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is shutdown.
         */
        rebuild_checkpoint_after_sp_shutdown[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                  percent_rebuilt_after_sp_shutdown,
                                                                                  FBE_TRUE, /* Set hook on object's Active SP */
                                                                                  FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                  FBE_FALSE /* Do not set this hook when the SP is booted */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is booted.
         */
        rebuild_start_checkpoint_after_sp_boot[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                    start_percent_rebuilt_after_sp_boot,
                                                                                    FBE_TRUE, /* Set hook on object's Active SP */
                                                                                    FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                    FBE_TRUE  /* Set this hook when the SP is booted */);

        /* Set a rebuild hook so that all raid groups will stop at
         * when the current (CMI) Active SP is booted.
         */
        rebuild_checkpoint_after_sp_boot[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                                    percent_rebuilt_after_sp_boot,
                                                                                    FBE_TRUE, /* Set hook on object's Active SP */
                                                                                    FBE_TRUE, /* Also set the hook on the Passive SP */
                                                                                    FBE_TRUE  /* Set this hook when the SP is booted */);
    }

    /* Invoke the method to re-insert the drives and validate that the 
     * reconstruct START notification is properly generated.
     */
    status = zoidberg_validate_rebuild_start(rg_config_p, raid_group_count, removed_drive_info_p,
                                             FBE_FALSE, /* This is a full rebuild test */
                                             __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Shutdown the (CMI) Active SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP ==", __FUNCTION__);
    status = zoidberg_reboot(&zoidberg_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(other_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown this SP - complete ==", __FUNCTION__);

    /* Validate that the rebuild continues (i.e. does not restart) after (some)
     * of the raid groups move the rebuild process to remaining SP.
     */
    status = zoidberg_validate_rebuild_continues_after_sp_shutdown(rg_config_p, raid_group_count, removed_drive_info_p,
                                                                   &rebuild_start_checkpoint_after_sp_shutdown[0], 
                                                                   &rebuild_checkpoint_after_sp_shutdown[0],
                                                                   __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Boot the orignal SP with the previously configured debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot this SP ==", __FUNCTION__);
    status = zoidberg_reboot_cleanup(&zoidberg_pause_params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot this SP - complete ==", __FUNCTION__);

    /* Now validate that the rebuild continued from where it left off.
     */
    status = zoidberg_validate_rebuild_continues_after_sp_boot(rg_config_p, raid_group_count, removed_drive_info_p,
                                                               percent_rebuilt_after_sp_shutdown,
                                                               &rebuild_checkpoint_after_sp_boot[0],
                                                               __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now validate that the reconstruct END notification is properly generated.
     */
    status = zoidberg_validate_rebuild_completes_after_sp_boot(rg_config_p, raid_group_count, removed_drive_info_p,
                                                               percent_rebuilt_after_sp_boot,
                                                               &rebuild_checkpoint_after_sp_boot[0],
                                                               __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear all the debug hooks.
     */
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Read the fixed pattern.
     */
    zoidberg_read_fixed_pattern(rg_config_p, raid_group_count);

    sep_rebuild_utils_validate_event_log_errors();

    /* Set `trigger spare timer' back to normal value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    return;

} 
/********************************************** 
 * end zoidberg_test_full_rebuild_sp_failure()
 **********************************************/

/*!****************************************************************************
 *          zoidberg_test_full_rebuild_array_failure()
 ******************************************************************************
 *
 * @brief   This test starts a full rebuild on SPx then kills SPx then validates
 *          that rebuild completes on SPy.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 *
 * @return  None
 *
 * @author
 *  03/18/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
/*!****************************************************************************
 *          zoidberg_test_full_rebuild_array_failure()
 ******************************************************************************
 *
 * @brief   This test starts a full rebuild on SPx then kills SPx then validates
 *          that rebuild completes on SPy.
 *
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 * @param   removed_drive_info_p - Pointer to array for removed drive information
 *
 * @return  None
 *
 * @author
 *  03/18/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void zoidberg_test_full_rebuild_array_failure(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t raid_group_count,
                                                     zoidberg_removed_drive_info_t *removed_drive_info_p)
{
    fbe_status_t                            status;
    fbe_sim_transport_connection_target_t   local_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_u32_t                               rebuild_index_to_wait_for;
    fbe_u32_t                               rg_index;
    fbe_lba_t                               target_checkpoint[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               percent_rebuilt_before_pause = 50;   /* Pause after 50% has been rebuilt*/
    fbe_notification_info_t                 notification_info;
    fbe_object_id_t                         spare_object_id = FBE_OBJECT_ID_INVALID;
    //fbe_sep_package_load_params_t           pause_params = { 0 };
    fbe_bool_t                              is_dual_sp_failure = FBE_TRUE;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_TEST_STATUS, "%s\n", __FUNCTION__);
    zoidberg_init_reboot_hooks();

    /* Get the local SP ID */
    local_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, local_sp);
    mut_printf(MUT_LOG_LOW, "=== ACTIVE(local) side (%s) ===", local_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP */
    peer_sp = (local_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    fbe_api_sim_transport_set_target_server(local_sp);

    /*  Get the number of physical objects in existence at this point in time.  This number is
     *  used when checking if a drive has been removed or has been reinserted.
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sep_rebuild_utils_number_physical_objects_g == ZOIDBERG_TEST_NUMBER_OF_PHYSICAL_OBJECTS);

    /* Set `trigger spare timer' to small value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5); /* 5 second hotspare timeout */

    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* go through all the RG for which user has provided configuration data. */
        zoidberg_write_background_pattern(&rg_config_p[rg_index], 1);
    
        /* Remove the drive and populate the removed information.
         */
        zoidberg_remove_drives(&rg_config_p[rg_index], 1, &removed_drive_info_p[rg_index],
                               FBE_FALSE /* This is a full rebuild test*/);


        fbe_test_sep_util_get_virtual_drive_object_id_by_position(zoidberg_rg_object_ids[rg_index], 
                                                                  removed_drive_info_p[rg_index].rg_position[0],
                                                                  &vd_object_id);

        /* set the unused drive as spare flag of virtual drive object. 
         * this should already be done, but just in case
         */
        fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id,FBE_FALSE);

        /* Validate the background pattern */
        zoidberg_verify_background_pattern(&rg_config_p[rg_index], 1);
    
        /* Write new data*/
        zoidberg_write_fixed_pattern(&rg_config_p[rg_index], 1);

        /* Set the `rebuild start' hook for the object's Active SP
         */
        zoidberg_set_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                        FBE_TRUE,   /* Don't set the hook on the Passive SP */
                                        FBE_FALSE   /* Don't save the rebuild start hook */);

        /* Determine and set the target checkpoint for this raid group.
         */
        target_checkpoint[rg_index] = zoidberg_set_percent_rebuilt_hook(zoidberg_rg_object_ids[rg_index], 
                                                                        percent_rebuilt_before_pause,
                                                                        FBE_TRUE,   /* Set hook on object's Active SP */
                                                                        FBE_TRUE,   /* Set the hook on the Passive SP */
                                                                        FBE_TRUE    /* Save the rebuild hook during shutdown */);
        mut_printf(MUT_LOG_TEST_STATUS, "Replacing drives...");

        /* Refresh extra disk infomation for this raid group
         */
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(&rg_config_p[rg_index], 1);

        /* We have already determined the first index to rebuild.
         */
        rebuild_index_to_wait_for = removed_drive_info_p[rg_index].first_index_to_rebuild;

        /* Replace the removed drives
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p[rg_index].extra_disk_set[rebuild_index_to_wait_for].bus,
                                                                rg_config_p[rg_index].extra_disk_set[rebuild_index_to_wait_for].enclosure,
                                                                rg_config_p[rg_index].extra_disk_set[rebuild_index_to_wait_for].slot,
                                                                &spare_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Setup notification to wait for.
         */
        status = fbe_test_common_set_notification_to_wait_for(spare_object_id,
                                                              FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Replace all of the removed drives
         */
        zoidberg_insert_drives(&rg_config_p[rg_index], 1, &removed_drive_info_p[rg_index],
                               FBE_FALSE,   /* This is a full rebuild */
                               removed_drive_info_p[rg_index].num_of_drives_removed /* Insert all removed drives */);

        /* Clear the rebuild start hook and wait for the rebuilt to start
         */
        zoidberg_clear_rebuild_start_hook(zoidberg_rg_object_ids[rg_index], 
                                          FBE_TRUE,   /* Clear hook on object's Active SP */
                                          FBE_TRUE   /* Don't set the hook on the Passive SP */);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct start notification.
         */
        MUT_ASSERT_INT_EQUAL(0, notification_info.notification_data.data_reconstruction.percent_complete);
        MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_START)       ||
                        (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_IN_PROGRESS));
        /* Wait for the rebuild to start (but don't wait for the rebuild hook)
         */
        fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(zoidberg_rg_object_ids[rg_index]);
    }

    /* Reboot the active SP and preserve the data.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown both SPs ==", __FUNCTION__);
    zoidberg_dual_sp_crash(&zoidberg_pause_params,
                           rg_config_p, raid_group_count, removed_drive_info_p, 
                           FBE_FALSE    /* This is a full rebuild test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(peer_sp);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Shutdown both SPs - complete ==", __FUNCTION__);

    zoidberg_wait_for_rebuilds_to_complete(rg_config_p, raid_group_count, removed_drive_info_p, 
                                           &target_checkpoint[0], is_dual_sp_failure); 

    /* Clear all the debug hooks.
     */
    status = fbe_test_clear_all_debug_hooks_both_sps();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Read the fixed pattern.
     */
    zoidberg_read_fixed_pattern(rg_config_p, raid_group_count);

    sep_rebuild_utils_validate_event_log_errors();

    /* Set `trigger spare timer' back to normal value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(300); /* 300 second hotspare timeout */

    return;

} 
/********************************************** 
 * end zoidberg_test_full_rebuild_array_failure()
 **********************************************/

/*!***************************************************************************
 *  zoidberg_run_tests
 *****************************************************************************
 *
 * @brief
 *   This function runs the Zoidberg test.
 *
 * @param   rg_config_p - Pointer to the config
 *          context_p      - Pointer to remove drive information
 *
 * @return  None
 *
 *****************************************************************************/
static void zoidberg_run_tests(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    zoidberg_removed_drive_info_t  *removed_drive_info_p = (zoidberg_removed_drive_info_t *)context_p;
    fbe_u32_t                       testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Get the array of raid group object ids.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_test_get_rg_ds_object_id(current_rg_config_p, 
                                     &zoidberg_rg_object_ids[rg_index], 
                                     0 /* Choose first mirror in RAID-10*/);

        current_rg_config_p++;
    }

    /* Lower the peer update checkpoint interval from default.
     */
    fbe_test_sep_util_set_update_peer_checkpoint_interval(1);

    /* Test 1:  SP fails during differential rebuild
     */
    zoidberg_test_diff_rebuild_sp_failure(rg_config_p, raid_group_count, removed_drive_info_p);

    /* Test 2: Both SPs fail during diffential rebuild.
     */
    zoidberg_test_diff_rebuild_array_failure(rg_config_p, raid_group_count, removed_drive_info_p);

    /* If extended testing is enabled execute the full array shutdown tests.
     */
    if (testing_level > 0) 
    {

        /*! @todo Until we fix multi-drive rebuilds to guarantee the order 
         *       (i.e. which drive will rebuild first) disable the following
         *       tests.
         */
        if (zoidberg_b_is_AR566356_fixed == FBE_TRUE)
        {
        /* Test 3: During a full rebuild the active SP goes away.
         */
        zoidberg_test_full_rebuild_sp_failure(rg_config_p, raid_group_count, removed_drive_info_p);

        } /* end if zoidberg_b_is_AR566356_fixed */

        /* Test 4: Both SPs fail during a full rebuild.
         */
        zoidberg_test_full_rebuild_array_failure(rg_config_p, raid_group_count, removed_drive_info_p);
    }


    /* Set the peer update checkpoint interval back to the default
     */
    fbe_test_sep_util_set_update_peer_checkpoint_interval(FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_SECS);
}



