/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file bender_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of rebuilds with disk failures.
 *
 * @version
 *   06/29/2011 - Created. Jason White
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "mut.h"                                    //  MUT unit testing infrastructure
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_hook.h"                               //  wait for debug hook
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_virtual_drive_interface.h"    //  for fbe_api_control_automatic_hot_spare
#include "fbe/fbe_api_database_interface.h"
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config
#include "fbe/fbe_api_scheduler_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* bender_test_short_desc = "Rebuild error cases: HS dies, original drive dies";
char* bender_test_long_desc =
"\n"
"\n"
"The Bender Scenario is a test of rebuild error cases due to disk failures.\n"
"\n"
"\n"
"*** Bender Phase 1 ********************************************************\n"
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
"arguments:   -use_default_drives - Uses the set default drives for the test,"
"                                   without this option random drives are used.\n"
"\n"
;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

#define BENDER_TEST_TIMEOUT 60000
#define BENDER_TEST_CONFIGS 5
#define BENDER_CHUNKS_PER_LUN 4
/*!*******************************************************************
 * @def BENDER_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define BENDER_WAIT_MSEC BENDER_TEST_TIMEOUT

/*!*******************************************************************
 * @def BENDER_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BENDER_SMALL_IO_SIZE_MAX_BLOCKS 64

/*!*******************************************************************
 * @def BENDER_TEST_MAX_NUM_DATA_DISKS
 *********************************************************************
 * @brief Max number of data disks
 *
 *********************************************************************/
#define BENDER_TEST_MAX_NUM_DATA_DISKS  2

/*!*******************************************************************
 * @def BENDER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs per raid group
 *
 *********************************************************************/
#define BENDER_LUNS_PER_RAID_GROUP  2

/*!*******************************************************************
 * @def BENDER_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BENDER_LARGE_IO_SIZE_MAX_BLOCKS 512

//  Context for rdgen.  This is an array with an entry for every LUN defined by the test.  The context is used
//  to initialize all the LUNs of the test in a single operation.
static fbe_api_rdgen_context_t fbe_bender_test_context_g[BENDER_LUNS_PER_RAID_GROUP * BENDER_TEST_CONFIGS];


static fbe_u32_t rg_chunk_stripe_blocks[BENDER_TEST_CONFIGS]; 

//  Slot of the next Hot Spare that is available for use.  One enclosure is allocated for Hot Spares and any
//  test can configure a HS in the next availabe slot.
extern fbe_u32_t sep_rebuild_utils_next_hot_spare_slot_g;

//  Number of physical objects in the test configuration.  This is needed for determining if drive has been
//  fully removed (objects destroyed) or fully inserted (objects created).
extern fbe_u32_t sep_rebuild_utils_number_physical_objects_g;

//  RG configurations
fbe_test_rg_configuration_t bender_rg_config_g[] =
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    // All mirror raid groups have (1) data position
    // All other types must have (2) data positions
    {3, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520, 0, 0},
    {2, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR,  520, 1, 0},
    {3, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR,  520, 2, 0},
    {4, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,520, 3, 0},
    {4, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY,  520, 4, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
static void bender_test_rebuild_hs_drive_failure(fbe_test_rg_configuration_t *in_rg_config_p,
                                                 void                        *context_p, 
                                                 fbe_u32_t                   *drives_to_remove);

static void bender_test_diff_rebuild_drive_failure(fbe_test_rg_configuration_t *in_rg_config_p, 
                                                   void                        *context_p,
                                                   fbe_u32_t                   *drives_to_remove);

static void bender_test_rebuild_after_rg_shutdown(fbe_test_rg_configuration_t *in_rg_config_p, 
                                                  void                        *context_p,
                                                  fbe_u32_t                   *drives_to_remove);

static void bender_test_remove_reinsert_drive_validate_spare_delay(fbe_test_rg_configuration_t *rg_config_p,
                                                                   void *context_p,
                                                                   fbe_u32_t *drives_to_remove);

static void bender_test_validate_lun_rebuild_notifications(fbe_test_rg_configuration_t *rg_config_p,
                                                           void                        *context_p,
                                                           fbe_u32_t                   *drives_to_remove);

static void bender_test_glitch_drive_during_io(fbe_test_rg_configuration_t *rg_config_p,
                                               void                        *context_p,
                                               fbe_u32_t                   *drives_to_remove);

static void bender_test_run_tests(fbe_test_rg_configuration_t *in_rg_config_p, void *context_p);


/*!****************************************************************************
 *  bender_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Bender test.  The test does the
 *   following:
 *
 *   - Creates raid groups and Hot Spares as needed
 *   - For each raid group, tests rebuild of one or more drives
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void bender_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_lun_number_t        next_lun_id;        //  sets/gets number of the next lun (0, 1, 2)

    //  Initialize configuration setup parameters - start with 0
    next_lun_id = 0;

    //--------------------------------------------------------------------------
    //  Test RAID-1 2-way mirror - Differential Rebuild drive failure
    //

    rg_config_p = &bender_rg_config_g[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    fbe_test_common_util_test_setup_init();

    //  Set up and perform tests on RAID-5 raid group
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, bender_test_run_tests,
                                   BENDER_LUNS_PER_RAID_GROUP,
                                   BENDER_CHUNKS_PER_LUN);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    //  Return
    return;

} //  End bender_dualsp_test()


/*!****************************************************************************
 *  bender_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Bender test.  It creates the
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void bender_dualsp_setup(void)
{
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_count;
    fbe_scheduler_debug_hook_t      hook;

    /* This test currently only runs in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        /* Set the proper array based on the test level.
         */
        if (test_level > 0)
        {
            /*! @note Currently the raid groups for extended testing are the same
             *        as those for qual.
             */
            rg_config_p= &bender_rg_config_g[0];
        }
        else
        {
            /* Qual.
             */
            rg_config_p= &bender_rg_config_g[0];
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

        /* Clear all previously set debug hooks
         */
        fbe_api_scheduler_clear_all_debug_hooks(&hook);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, raid_group_count);

        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* Clear all previously set debug hooks
         */
        fbe_api_scheduler_clear_all_debug_hooks(&hook);

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

        //  Initialize the next hot spare slot to 0
        sep_rebuild_utils_next_hot_spare_slot_g = 0;
    }

    return;

} //  End bender_dualsp_setup()

/*!****************************************************************************
 *  bender_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Bender test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void bender_dualsp_cleanup(void)
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
}

static void bender_test_run_tests(fbe_test_rg_configuration_t *in_rg_config_p,
                                  void *context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t                       max_drives_to_remove[BENDER_TEST_CONFIGS];
    fbe_test_rg_configuration_t    *rg_config_p;
    fbe_u32_t                       i;
    fbe_api_raid_group_get_info_t   rg_info;
    fbe_object_id_t                 rg_object_id;
    fbe_u32_t                       glitch_drive_iterations = 1;
#if 0
    fbe_u32_t                       validate_spare_delay_iterations = 1;
#endif

    rg_config_p = in_rg_config_p;
    for (i=0; i<BENDER_TEST_CONFIGS; i++)
    {
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6 ||
            (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 &&
             rg_config_p->width == SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH))
        {
            max_drives_to_remove[i] = 2;
        }
        else
        {
            max_drives_to_remove[i] = 1;
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        rg_chunk_stripe_blocks[i] = rg_info.num_data_disk * FBE_RAID_DEFAULT_CHUNK_SIZE * fbe_test_sep_util_get_chunks_per_rebuild();
        rg_config_p++;
    }

    /* Test 1: 
     */
    /*! Test 1: `Glitch' one of the drives and validate I/O status.
     *
     *  @note Since it takes multiple iterations to detect the failure, if the
     *        test level is greater than 1 run (10) iterations of glitch drive.
     */
    if (test_level > 1)
    {
        glitch_drive_iterations = 10;
    }
    for (i = 0; i < glitch_drive_iterations; i++)
    {
        bender_test_glitch_drive_during_io(in_rg_config_p, context_p, &max_drives_to_remove[0]);
    }

#if 0
    /*! Test 2: Validates that turning off automatic hot-sparing waits
     *
     *  @note Since it takes multiple iterations to detect the failure, if the
     *        test level is greater than 1 run (20) iterations of validate 
     *        spare delay.
     */
    if (test_level > 1)
    {
        validate_spare_delay_iterations = 20;
    }
    for (i = 0; i < validate_spare_delay_iterations; i++)
    {
        bender_test_remove_reinsert_drive_validate_spare_delay(in_rg_config_p, context_p, &max_drives_to_remove[0]);
    }

    /* Test 3: Remove sufficient drives to break raid group.  Re-insert drives
     */
    bender_test_rebuild_after_rg_shutdown(in_rg_config_p, context_p, &max_drives_to_remove[0]);

    /* Test 4: Remove and re-insert drives (differential rebuild)
     */
    bender_test_diff_rebuild_drive_failure(in_rg_config_p, context_p, &max_drives_to_remove[0]);

    /* Test 5: Validates proper lun rebuild notifications are generated for
     *         a differential rebuild.
     */
    bender_test_validate_lun_rebuild_notifications(in_rg_config_p, context_p, &max_drives_to_remove[0]);

    /* Skip this for now, as the test has to be modified to accomodate R6 and R1 (3 Mirror) 
     * which deals with 2 drive removal.
     */
    //bender_test_rebuild_hs_drive_failure(in_rg_config_p, context_p, &max_drives_to_remove[0]);
#endif
}

/*!****************************************************************************
*  bender_test_rebuild_hs_drive_failure
******************************************************************************
*
* @brief
*   This function tests killing the rebuilding drive (HS) during basic rebuilds for a RAID-6 or RAID-10 RAID group.  It
*   does the following:
*
*   - Writes a seed pattern
*   - Removes the drive in position 0 in the RG
*   - Removes the drive in position 2 in the RG
*   - Configures two Hot Spares, which will swap in for each of the drives
*   - Removes both of the HS drives while they are rebuilding
*   - Reinserts both HS drives
*   - Waits for the rebuilds of both drives to finish
*   - Verifies that the data is correct
*
*   At the end of this test, the RG is enabled and 2 positions have been
*   swapped to permanant spares.
*
* @param in_rg_config_p     - pointer to the RG configuration info
* @param context_p          - pointer to the context
* @param drives_to_remove   - array of the number of drives to remove
*
* @return  None
*
*****************************************************************************/
static void bender_test_rebuild_hs_drive_failure(fbe_test_rg_configuration_t *in_rg_config_p,
                                                 void                        *context_p,
                                                 fbe_u32_t                   *drives_to_remove)
{
    fbe_api_terminator_device_handle_t  drive_info[BENDER_TEST_CONFIGS][2];                   //  drive info needed for reinsertion
    fbe_status_t                        status;
    fbe_test_raid_group_disk_set_t      hs_config[BENDER_TEST_CONFIGS][2];
    fbe_u32_t                           hs_count = 0;
    fbe_u32_t                           i;
    fbe_u32_t                           rg_index;
    fbe_u32_t                           drive_slots_to_be_removed[BENDER_TEST_CONFIGS][2];
    fbe_object_id_t                     raid_group_object_id[BENDER_TEST_CONFIGS];
    fbe_lba_t                           target_checkpoint[BENDER_TEST_CONFIGS];
    fbe_test_rg_configuration_t*        rg_config_p;
    fbe_u32_t*                          drives_to_remove_p;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if (fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
        // Turn off automatic sparing.
        fbe_api_control_automatic_hot_spare(FBE_FALSE);

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            fbe_object_id_t mirror_obj_id = FBE_OBJECT_ID_INVALID;
            fbe_u32_t adjusted_position = 0;

            /* RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
             * object. Otherwise use the object id already found and the input position.
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                                      &mirror_obj_id, &adjusted_position);
                raid_group_object_id[rg_index] = mirror_obj_id;
            }
            else
            {
                /* Get the RG's object id. */
                fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);
            }

            rg_config_p++;
        }

        //  Get the number of physical objects in existence at this point in time.  This number is
        //  used when checking if a drive has been removed or has been reinserted.
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* go through all the RG for which user has provided configuration data. */
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            //  Write a seed pattern to the RG
            sep_rebuild_utils_write_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
                drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
                drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
            }
            else
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
                sep_rebuild_utils_get_random_drives(rg_config_p, *drives_to_remove_p, drive_slots_to_be_removed[rg_index]);
            }
            rg_config_p++;
            drives_to_remove_p++;
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            target_checkpoint[rg_index] = 0x800 * fbe_test_sep_util_get_chunks_per_rebuild();

            mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...obj: 0x%x, chkpt: 0x%llx", 
                       raid_group_object_id[rg_index], target_checkpoint[rg_index]);

            status = fbe_test_add_debug_hook_active(raid_group_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      target_checkpoint[rg_index],
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_PAUSE);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            /* We don't want to wait for the rebuild to start on the second drive. Rebuild will not start 
             * on the second drive because due to the hook set on the first drive, the rebuild never completes
             * and it will not allow the rebuild to start on the second drive.. For now, we have to skip it.. 
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6 ||
                (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 &&
                 rg_config_p->width == SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH))
            {
                rg_config_p++;
                drives_to_remove_p++;
            }
            else
            {
                for (i=0;i<*drives_to_remove_p;i++)
                {
                    /* go through all the RG for which user has provided configuration data. */
                    sep_rebuild_utils_number_physical_objects_g -= 1;
    
                    //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
                    //  is marked.
                    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], 
                                                              sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
                }
    
                mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
                sep_rebuild_utils_write_new_data(rg_config_p, &fbe_bender_test_context_g[0], 0x200);
    
                for (i=0;i<*drives_to_remove_p;i++)
                {
                    status = sep_rebuild_utils_find_free_drive(&hs_config[rg_index][i], rg_config_p->unused_disk_set, hs_count);
    
                    if (status != FBE_STATUS_OK)
                    {
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Failed to find a free drive to configure as HS . ****");
                        return;
                    }
                    sep_rebuild_utils_setup_hot_spare(hs_config[rg_index][i].bus, hs_config[rg_index][i].enclosure, hs_config[rg_index][i].slot);
                    hs_count++;
    
                    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
                }
                rg_config_p++;
                drives_to_remove_p++;
            }
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            /* We don't want to wait for the rebuild to start on the second drive. Rebuild will not start 
             * on the second drive because due to the hook set on the first drive, the rebuild never completes
             * and it will not allow the rebuild to start on the second drive.. For now, we have to skip it.. 
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6 ||
                (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 &&
                 rg_config_p->width == SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH))
            {
                rg_config_p++;
                drives_to_remove_p++;
            }
            else
            {
                for (i=0;i<*drives_to_remove_p;i++)
                {
                    sep_rebuild_utils_number_physical_objects_g -= 1;
                    sep_rebuild_utils_remove_hs_drive(hs_config[rg_index][i].bus, hs_config[rg_index][i].enclosure, hs_config[rg_index][i].slot,
                                                      raid_group_object_id[rg_index],
                                                      drive_slots_to_be_removed[rg_index][i],
                                                      sep_rebuild_utils_number_physical_objects_g,
                                                      &drive_info[rg_index][i]);

                    sep_rebuild_utils_verify_rb_logging(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
                    sep_rebuild_utils_check_for_reb_restart(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
                }
                rg_config_p++;
                drives_to_remove_p++;
            }
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            /* We don't want to wait for the rebuild to start on the second drive. Rebuild will not start 
             * on the second drive because due to the hook set on the first drive, the rebuild never completes
             * and it will not allow the rebuild to start on the second drive.. For now, we have to skip it.. 
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6 ||
                (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 &&
                 rg_config_p->width == SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH))
            {
                rg_config_p++;
                drives_to_remove_p++;
            }
            else
            {
                for (i=0;i<*drives_to_remove_p;i++)
                {
                    sep_rebuild_utils_number_physical_objects_g += 1;
                    sep_rebuild_utils_reinsert_removed_drive_no_check(hs_config[rg_index][i].bus, hs_config[rg_index][i].enclosure, 
                                                                      hs_config[rg_index][i].slot, &drive_info[rg_index][i]);
                }
                for (i=0;i<*drives_to_remove_p;i++)
                {
                    status = fbe_test_sep_util_wait_for_pvd_state(hs_config[rg_index][i].bus, hs_config[rg_index][i].enclosure, 
                                                                  hs_config[rg_index][i].slot, FBE_LIFECYCLE_STATE_READY, BENDER_TEST_TIMEOUT);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
                rg_config_p++;
                drives_to_remove_p++;
            }
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");
            status = fbe_test_wait_for_debug_hook_active(raid_group_object_id[rg_index], 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                SCHEDULER_CHECK_VALS_LT, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                target_checkpoint[rg_index], 
                                                NULL);

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...obj: 0x%x, chkpt: 0x%llx", 
                       raid_group_object_id[rg_index], target_checkpoint[rg_index]);
            status = fbe_test_del_debug_hook_active(raid_group_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      target_checkpoint[rg_index],
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_PAUSE);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            for (i=0;i<*drives_to_remove_p;i++)
            {
                /* Wait until the rebuilds finish */
                sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
            }

            sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

            /* Verify the SEP objects is in expected lifecycle State  */
            status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
                                                             FBE_LIFECYCLE_STATE_READY,
                                                             BENDER_WAIT_MSEC,
                                                             FBE_PACKAGE_ID_SEP_0);

            /* Wait for all lun objects of this raid group to be ready */
            status = fbe_test_sep_util_wait_for_all_lun_objects_ready(rg_config_p,
                                                                      fbe_api_transport_get_target_server());
            if (status != FBE_STATUS_OK)
            {
                /* Failure, break out and return the failure. */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                           rg_config_p->raid_group_id);
                break;
            }

            /* Read the data and make sure it matches the new seed pattern */
            sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_bender_test_context_g[0], 0x200);
            rg_config_p++;
            drives_to_remove_p++;
        }
        sep_rebuild_utils_validate_event_log_errors();

        /* Turn on automatic sparing. */
        fbe_api_control_automatic_hot_spare(FBE_TRUE);
    }

    return;

} //  End bender_test_rebuild_hs_drive_failure()

/*!****************************************************************************
 *  bender_test_diff_rebuild_drive_failure
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds for a RAID-6 or RAID-10 RAID group.  It
 *   does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in position 0 in the RG
 *   - Removes the drive in position 2 in the RG
 *   - Reinserts the drive in position 0
 *   - Waits for rebuild to start on that drive
 *   - Removes and reinserts the drive in position 0
 *   - Reinserts the drive in position 2
 *   - Waits for rebuild to start on that drive
 *   - Removes and reinserts the drive in position 2
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 * @param drives_to_remove   - array of the number of drives to remove
 *
 * @return  None
 *
 *****************************************************************************/
static void bender_test_diff_rebuild_drive_failure(fbe_test_rg_configuration_t *in_rg_config_p, 
                                                   void                        *context_p, 
                                                   fbe_u32_t                   *drives_to_remove)
{
    fbe_api_terminator_device_handle_t  drive_info[BENDER_TEST_CONFIGS][2];                   //  drive info needed for reinsertion
    fbe_status_t                        status;
    fbe_u32_t                           i;
    fbe_u32_t                           rg_index;
    fbe_u32_t                           drive_slots_to_be_removed[BENDER_TEST_CONFIGS][2];
    fbe_object_id_t                     raid_group_object_id[BENDER_TEST_CONFIGS];
    fbe_lba_t                           target_checkpoint[BENDER_TEST_CONFIGS];
    fbe_test_rg_configuration_t*        rg_config_p;
    fbe_u32_t*                          drives_to_remove_p;

    /* Print message as to where the test is at */
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if (fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
        /* Get the number of physical objects in existence at this point in time.  This number is 
         * used when checking if a drive has been removed or has been reinserted.
         */
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* go through all the RG for which user has provided configuration data. */
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            /* Write a seed pattern to the RG */
            sep_rebuild_utils_write_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
                drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
                drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
            }
            else
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
                sep_rebuild_utils_get_random_drives(rg_config_p, *drives_to_remove_p, drive_slots_to_be_removed[rg_index]);
            }
            rg_config_p++;
            drives_to_remove_p++;
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            for (i=0;i<*drives_to_remove_p;i++)
            {
                /* go through all the RG for which user has provided configuration data. */
                sep_rebuild_utils_number_physical_objects_g -= 1;

                /* Remove a single drive in the RG.  Check the object states change correctly and 
                 * that rb logging is marked.
                 */
                sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
            }
            rg_config_p++;
            drives_to_remove_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
        sep_rebuild_utils_read_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
            sep_rebuild_utils_write_new_data(rg_config_p, &fbe_bender_test_context_g[0], 0x200);
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            fbe_object_id_t mirror_obj_id = FBE_OBJECT_ID_INVALID;
            fbe_u32_t adjusted_position = 0;
    
            /* RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
             * object. Otherwise use the object id already found and the input position.
             */
            if (in_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                                      &mirror_obj_id, &adjusted_position);
                raid_group_object_id[rg_index] = mirror_obj_id;
            }
            else
            {
                /* Get the RG's object id. */
                fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);
            }

            target_checkpoint[rg_index] = 0x800;

            mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...obj: 0x%x, chkpt: 0x%llx", 
                       raid_group_object_id[rg_index], target_checkpoint[rg_index]);

            status = fbe_test_add_debug_hook_active(raid_group_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      target_checkpoint[rg_index],
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_PAUSE);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Re-inserting drives...");

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            sep_rebuild_utils_number_physical_objects_g += 1;

            /* Reinsert the drive and wait for the rebuild to start */
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][0], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][0]);
            sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][0]);

            rg_config_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Removing drives...");

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            sep_rebuild_utils_number_physical_objects_g -= 1;

            /* Remove a single drive in the RG.  Check the object states change correctly 
             * and that rb logging is marked.
             */
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][0], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][0]);
            sep_rebuild_utils_verify_rb_logging(rg_config_p, drive_slots_to_be_removed[rg_index][0]);
            sep_rebuild_utils_check_for_reb_restart(rg_config_p, drive_slots_to_be_removed[rg_index][0]);

            rg_config_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Reinserting drives...");

        drives_to_remove_p = drives_to_remove;
        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            for (i=0;i<*drives_to_remove_p;i++)
            {
                sep_rebuild_utils_number_physical_objects_g += 1;
                sep_rebuild_utils_reinsert_removed_drive_no_check(rg_config_p->rg_disk_set[drive_slots_to_be_removed[rg_index][i]].bus,
                                                                  rg_config_p->rg_disk_set[drive_slots_to_be_removed[rg_index][i]].enclosure,
                                                                  rg_config_p->rg_disk_set[drive_slots_to_be_removed[rg_index][i]].slot,
                                                                  &drive_info[rg_index][i]);
            }

            for (i=0;i<*drives_to_remove_p;i++)
            {
                sep_rebuild_utils_verify_reinserted_drive(rg_config_p,
                                                          drive_slots_to_be_removed[rg_index][i],
                                                          sep_rebuild_utils_number_physical_objects_g,
                                                          &drive_info[rg_index][i]);
            }
            drives_to_remove_p++;
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            /* No need to wait for the hook.. just delete it. */
            mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...obj: 0x%x, chkpt: 0x%llx", 
                       raid_group_object_id[rg_index], target_checkpoint[rg_index]);
            status = fbe_test_del_debug_hook_active(raid_group_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      target_checkpoint[rg_index],
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_PAUSE);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            for (i=0;i<*drives_to_remove_p;i++)
            {
                /* Wait until the rebuilds finish */
                sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
            }

            sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

            /* Verify the SEP objects is in expected lifecycle State  */
            status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
                                                             FBE_LIFECYCLE_STATE_READY,
                                                             BENDER_WAIT_MSEC,
                                                             FBE_PACKAGE_ID_SEP_0);

            /* Wait for all lun objects of this raid group to be ready */
            status = fbe_test_sep_util_wait_for_all_lun_objects_ready(rg_config_p,
                                                                      fbe_api_transport_get_target_server());
            if (status != FBE_STATUS_OK)
            {
                /* Failure, break out and return the failure. */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                           rg_config_p->raid_group_id);
                break;
            }

            /* Read the data and make sure it matches the new seed pattern */
            sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_bender_test_context_g[0], 0x200);
            rg_config_p++;
            drives_to_remove_p++;
        }
        sep_rebuild_utils_validate_event_log_errors();
    }

    return;

} //  End bender_test_diff_rebuild_drive_failure()

/*!****************************************************************************
 *  bender_test_rebuild_after_rg_shutdown
 ******************************************************************************
 *
 * @brief
 *   This function tests if the raid group shuts down with a rebuild in progress,
 *   when we restart the rebuild we pick up from where we left off for a RAID group.
 *   It does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in position 0 in the RG
 *   - Removes the drive in position 1 in the RG
 *   - Writes a new seed pattern
 *   - Reinserts the drive in position 0
 *   - Waits for rebuild to start on that drive
 *   - inserts a hook to pause the rebuild
 *   - gets the current checkpoint of the rebuild
 *   - Removes the drive in position 2
 *   - Reinserts the drive in position 1
 *   - Reinserts the drive in position 2
 *   - gets the current checkpoint and ensures that it is equal to the previous one.
 *   - remove the hook
 *   - Waits for the rebuilds of all drives to finish
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 * @param drives_to_remove   - array of the number of drives to remove
 *
 * @return  None
 *
 *****************************************************************************/
static void bender_test_rebuild_after_rg_shutdown(fbe_test_rg_configuration_t *in_rg_config_p,
                                                  void                        *context_p,
                                                  fbe_u32_t                   *drives_to_remove)
{
    fbe_api_terminator_device_handle_t  drive_info[BENDER_TEST_CONFIGS][3];                   //  drive info needed for reinsertion
    fbe_status_t                        status;
    fbe_u32_t                           i;
    fbe_u32_t                           rg_index;
    fbe_u32_t                           drive_slots_to_be_removed[BENDER_TEST_CONFIGS][3];
    fbe_object_id_t                     raid_group_object_id[BENDER_TEST_CONFIGS];
    fbe_lba_t                           checkpoint[BENDER_TEST_CONFIGS];
    fbe_lba_t                           target_checkpoint[BENDER_TEST_CONFIGS];
    fbe_lba_t                           new_checkpoint[BENDER_TEST_CONFIGS];
    fbe_test_rg_configuration_t*        rg_config_p;
    fbe_u32_t*                          drives_to_remove_p;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if (fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
       //  Get the number of physical objects in existence at this point in time.  This number is
        //  used when checking if a drive has been removed or has been reinserted.
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* go through all the RG for which user has provided configuration data. */
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            //  Write a seed pattern to the RG
            sep_rebuild_utils_write_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
                drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
                drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
                drive_slots_to_be_removed[rg_index][2] = SEP_REBUILD_UTILS_POSITION_0;
            }
            else
            {
                mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
                sep_rebuild_utils_get_random_drives(rg_config_p, (*drives_to_remove_p)+1, drive_slots_to_be_removed[rg_index]);
            }
            rg_config_p++;
            drives_to_remove_p++;
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            fbe_object_id_t mirror_obj_id = FBE_OBJECT_ID_INVALID;
            fbe_u32_t adjusted_position = 0;
    
            /* RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
             * object. Otherwise use the object id already found and the input position.
             */
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                                      &mirror_obj_id, &adjusted_position);
                raid_group_object_id[rg_index] = mirror_obj_id;
            }
            else
            {
                /* Get the RG's object id. */
                fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);
            }
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            for (i=0;i<*drives_to_remove_p;i++)
            {
                /* go through all the RG for which user has provided configuration data. */
                sep_rebuild_utils_number_physical_objects_g -= 1;

                //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
                //  is marked.
                sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
            }
            rg_config_p++;
            drives_to_remove_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
        sep_rebuild_utils_read_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
            sep_rebuild_utils_write_new_data(rg_config_p, &fbe_bender_test_context_g[0], 0x200);

            /* Write to something outside the first chunk. 
             * This ensures that it takes more than just reaching the end of the first chunk for the rebuild to get 
             * completed. 
             * We choose to write starting at the beginning of the third LUN. 
             */
            sep_rebuild_utils_write_new_data(rg_config_p, &fbe_bender_test_context_g[0], 
                                             rg_chunk_stripe_blocks[rg_index] * BENDER_CHUNKS_PER_LUN * 2);
            rg_config_p++;
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            target_checkpoint[rg_index] = 0x800 * fbe_test_sep_util_get_chunks_per_rebuild();

            mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...obj: 0x%x, chkpt: 0x%llx", 
                       raid_group_object_id[rg_index], target_checkpoint[rg_index]);

            status = fbe_test_add_debug_hook_active(raid_group_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      target_checkpoint[rg_index],
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_PAUSE);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Reinserting (1) drive but hook stops rebuild (i.e. rg is degraded)...");
        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            sep_rebuild_utils_number_physical_objects_g += 1;

            /* Reinsert the drive and wait for the rebuild to start */
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][0], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][0]);
            sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][0]);

            /*  Get the checkpoint, this will be used for comparison later */
            sep_rebuild_utils_get_reb_checkpoint(rg_config_p, drive_slots_to_be_removed[rg_index][0], &checkpoint[rg_index]);

            rg_config_p++;
            drives_to_remove_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Removing and reinserting drives 1 and 2...");
        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            sep_rebuild_utils_number_physical_objects_g -= 1;

            /* Remove another drive in the RG.  Check the object states change correctly and 
             * that rb logging is marked.
             */
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][*drives_to_remove_p], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][*drives_to_remove_p]);

            /* For a R10 you atleast need to pull 2 drives to expect the RG to go FAIL state..
             * Here we are just pulling only one drive..
             */
            if(rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
            {
                /* Verify the SEP objects is in expected lifecycle State  */
                status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
                                                                 FBE_LIFECYCLE_STATE_FAIL,
                                                                 BENDER_WAIT_MSEC,
                                                                 FBE_PACKAGE_ID_SEP_0);
    
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }

            /* Reinsert the other drive */
            sep_rebuild_utils_number_physical_objects_g +=1;
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][*drives_to_remove_p], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][*drives_to_remove_p]);

            if (*drives_to_remove_p == 2)
            {
                sep_rebuild_utils_number_physical_objects_g += 1;
                sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][1], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][1]);
            }

            /* Get the checkpoint again and compare to see that it is the same 
             * as we had before the RG went broken..
             */
            sep_rebuild_utils_get_reb_checkpoint(rg_config_p, drive_slots_to_be_removed[rg_index][0], &new_checkpoint[rg_index]);
            MUT_ASSERT_UINT64_EQUAL_MSG(checkpoint[rg_index], new_checkpoint[rg_index], "The checkpoint before the RG went broken is not equal to the new checkpoint.");
            rg_config_p++;
            drives_to_remove_p++;
        }

        rg_config_p = in_rg_config_p;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");
            status = fbe_test_wait_for_debug_hook_active(raid_group_object_id[rg_index], 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                SCHEDULER_CHECK_VALS_LT, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                target_checkpoint[rg_index], 
                                                NULL);

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...obj: 0x%x, chkpt: 0x%llx", 
                       raid_group_object_id[rg_index], target_checkpoint[rg_index]);
            status = fbe_test_del_debug_hook_active(raid_group_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      target_checkpoint[rg_index],
                                                      NULL,
                                                      SCHEDULER_CHECK_VALS_LT,
                                                      SCHEDULER_DEBUG_ACTION_PAUSE);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            rg_config_p++;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Waiting for rebuilds to complete...");
        rg_config_p = in_rg_config_p;
        drives_to_remove_p = drives_to_remove;
        for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
        {
            for (i=0;i<*drives_to_remove_p;i++)
            {
                /* Wait until the rebuilds finish */
                sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
            }

            sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

            /* Verify the SEP objects is in expected lifecycle State  */
            status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
                                                             FBE_LIFECYCLE_STATE_READY,
                                                             BENDER_WAIT_MSEC,
                                                             FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Wait for all lun objects of this raid group to be ready */
            status = fbe_test_sep_util_wait_for_all_lun_objects_ready(rg_config_p,
                                                                      fbe_api_transport_get_target_server());
            if (status != FBE_STATUS_OK)
            {
                /* Failure, break out and return the failure. */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                           rg_config_p->raid_group_id);
                break;
            }

            /* Read the data and make sure it matches the new seed pattern */
            sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_bender_test_context_g[0], 0x200);
            rg_config_p++;
            drives_to_remove_p++;
        }

        sep_rebuild_utils_validate_event_log_errors();
    }

    return;

} //  End bender_test_rebuild_after_rg_shutdown()

/*!***************************************************************************
 *          bender_test_remove_reinsert_drive_validate_spare_delay()
 *****************************************************************************
 *
 * @brief   This test disables automatic hot-sparing (ahs) then removes (1) 
 *          drive from each of the raid group, validates that no spare is 
 *          swapped in then re-inserts the removed drive.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  05/07/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bender_test_remove_reinsert_drive_validate_spare_delay(fbe_test_rg_configuration_t *rg_config_p,
                                                                   void *context_p,
                                                                   fbe_u32_t *drives_to_remove)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t target_invoked_on = FBE_SIM_SP_A;
    fbe_api_terminator_device_handle_t  drive_info[BENDER_TEST_CONFIGS][3];                   //  drive info needed for reinsertion
    fbe_u32_t                           rg_index;
    fbe_scheduler_debug_hook_t          hook;
    fbe_u32_t                           seconds_to_wait_before_validating_failed_state;
    fbe_u32_t                           drive_slots_to_be_removed[BENDER_TEST_CONFIGS][3];
    fbe_object_id_t                     raid_group_object_id[BENDER_TEST_CONFIGS];
    fbe_object_id_t                     vd_object_id[BENDER_TEST_CONFIGS];
    fbe_test_rg_configuration_t        *current_rg_config_p = rg_config_p;
    fbe_u32_t                          *drives_to_remove_p;
    fbe_object_id_t                     mirror_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           adjusted_position = 0;
    fbe_notification_info_t             notification_info;

    /* Print message as to where the test is at
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    /* Turn OFF Automatic hot-sparing (`ahs')
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_control_automatic_hot_spare(FBE_FALSE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_control_automatic_hot_spare(FBE_FALSE);
    fbe_api_sim_transport_set_target_server(target_invoked_on);

    /* Set the trigger spare timer to a minimal value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);
    seconds_to_wait_before_validating_failed_state = (5 * 4);
    
    /* Print the test start message
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "==%s Validate that virtual drive waits: %d (%d is trigger time) seconds before existing failed ==",
               __FUNCTION__, seconds_to_wait_before_validating_failed_state,
               FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);


    /* For each raid group:
     *  o Configure drive_slots_to_be_removed
     *  o raid_group_object_id
     *  o vd_object_id
     */
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Use default */
        mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
        drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_0;
        drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_1;
        drive_slots_to_be_removed[rg_index][2] = SEP_REBUILD_UTILS_POSITION_2;

        /* RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
         * object. Otherwise use the object id already found and the input position.
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(current_rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                                  &mirror_obj_id, &adjusted_position);
            raid_group_object_id[rg_index] = mirror_obj_id;
        }
        else
        {
            /* Get the RG's object id. */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);
        }

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(raid_group_object_id[rg_index], 
                                                                           drive_slots_to_be_removed[rg_index][0], 
                                                                           &vd_object_id[rg_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    /* Get the number of physical objects in existence at this point in time.  
     * This number is used when checking if a drive has been removed or has 
     * been reinserted.
     */
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        //  Write a seed pattern to the RG
        sep_rebuild_utils_write_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    }

    /* Remove (1) drive from each raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Set debug hooks and remove drive...");
    current_rg_config_p = rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* go through all the RG for which user has provided configuration data. */
        sep_rebuild_utils_number_physical_objects_g -= 1;

        /* Set up failed notification
         */
        status = fbe_test_common_set_notification_to_wait_for(vd_object_id[rg_index],
                                                              FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                              FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Remove a single drive in the RG.  Check the object states change 
         * correctly and that rb logging is marked.
         */
        sep_rebuild_utils_remove_drive_and_verify(current_rg_config_p, 
                                                  drive_slots_to_be_removed[rg_index][0], 
                                                  sep_rebuild_utils_number_physical_objects_g, 
                                                  &drive_info[rg_index][0]);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! Set a debug hooks:
         *  o Validate that we don't enter Activate with `job in-progress'
         *  o Validate that we don't enter Activate with healthy
         *
         * @note There are cases where we can go disabled so we cannot add these
         *       before we enter the Failed state.
         */

        /* Set a debug hook to log the vd if it enters the activate state and job in-progress */
        status = fbe_test_add_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS,
                                                0, 0,              
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_add_debug_hook_passive(vd_object_id[rg_index],
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS,
                                                 0, 0,              
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set a debug hook to log the vd if it enters the activate state */
        status = fbe_test_add_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                                0, 0,              
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_add_debug_hook_passive(vd_object_id[rg_index],
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                                 0, 0,              
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
        drives_to_remove_p++;
    }

    /* Validate background pattern
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
    sep_rebuild_utils_read_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    /* Now wait for some time to validate we stay in the failed state*/
    mut_printf(MUT_LOG_TEST_STATUS, 
               "==%s Validate that vd obj stays in failed state for: %d seconds ==",
               __FUNCTION__, seconds_to_wait_before_validating_failed_state);
    fbe_api_sleep((seconds_to_wait_before_validating_failed_state * 1000));

    /* Validate that the failed virutal drives did not enter the
     * Activate state
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Validate that we did not encounter the `job in-progress' condition
         */
        status = fbe_test_get_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(hook.counter == 0);
        status = fbe_test_get_debug_hook_passive(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(hook.counter == 0);

        /* Validate that we did not enter activate.
        */
        status = fbe_test_get_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(hook.counter == 0);
        status = fbe_test_get_debug_hook_passive(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(hook.counter == 0);

        current_rg_config_p++;
    }

    /* Write a new background pattern while degraded.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
        sep_rebuild_utils_write_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 0x200);

        /* Write to something outside the first chunk. 
         * This ensures that it takes more than just reaching the end of the first chunk for the rebuild to get 
         * completed. 
         * We choose to write starting at the beginning of the third LUN. 
         */
        sep_rebuild_utils_write_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 
                                         rg_chunk_stripe_blocks[rg_index] * BENDER_CHUNKS_PER_LUN * 2);
        current_rg_config_p++;
    }

    /* Re-insert the removed drives and validate that we enter activate
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Re-insert removed drive and validate Activate ==");
    current_rg_config_p = rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Now remove the Activate hook (re-insert and verify expects the
         * virtual drive to become Ready)
         */
        status = fbe_test_del_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_del_debug_hook_passive(vd_object_id[rg_index],
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                                 0, 0, 
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        sep_rebuild_utils_number_physical_objects_g += 1;

        /* Reinsert the drive and wait for the Activate hook.
         */
        sep_rebuild_utils_reinsert_drive_and_verify(current_rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                    sep_rebuild_utils_number_physical_objects_g, 
                                                    &drive_info[rg_index][0]);

        current_rg_config_p++;
        drives_to_remove_p++;
    }

    /* Remove Activate hook and validate drives rebuild
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove Activate hook and validate rebuild ==");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Validate that we did not encounter the `job in-progress' condition
         */
        status = fbe_test_get_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(hook.counter == 0);
        status = fbe_test_get_debug_hook_passive(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(hook.counter == 0);

        /* Now remove the Activate hook
         */
        status = fbe_test_del_debug_hook_active(vd_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS,
                                                0, 0, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_del_debug_hook_passive(vd_object_id[rg_index],
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS,
                                                 0, 0, 
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate rebuild completes.
         */
        sep_rebuild_utils_wait_for_rb_comp(current_rg_config_p, drive_slots_to_be_removed[rg_index][0]);

        sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

        /* Verify the SEP objects is in expected lifecycle State  */
        status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
                                                         FBE_LIFECYCLE_STATE_READY,
                                                         BENDER_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Wait for all lun objects of this raid group to be ready */
        status = fbe_test_sep_util_wait_for_all_lun_objects_ready(current_rg_config_p,
                                                                  fbe_api_transport_get_target_server());
        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure. */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                       current_rg_config_p->raid_group_id);
            break;
        }

        /* Read the data and make sure it matches the new seed pattern */
        sep_rebuild_utils_verify_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 0x200);
        current_rg_config_p++;
    }

    sep_rebuild_utils_validate_event_log_errors();

    /* Restore the default spare trigger time
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Turn on automatic sparing. */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_control_automatic_hot_spare(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_control_automatic_hot_spare(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(target_invoked_on);

    return;
}
/**************************************************************
 * end bender_test_remove_reinsert_drive_validate_spare_delay()
 **************************************************************/

/*!****************************************************************************
 *          bender_test_glitch_drive_during_io()
 ******************************************************************************
 *
 * @brief
 *   This test validates that the proper lun rebuild notifications are generated.
 *   It test both single drive rebuilds and two drive rebuilds (it expects the
 *   percentage rebuilt to be adjusted accordingly).
 * 
 *   Steps:
 *   - Degrade the raid groups (depending on raid type one or two drive are removed)
 *   - Generate `distributed' writes to all raid groups
 *   - Register for the lun rebuild notifications
 *   - Re-insert the removed drives
 *   - Validate the proper notifications are generated
 *   - Generate `distributed' reads - validate data is correct
 *
 * @param   rg_config_p     - pointer to the RG configuration info
 * @param   context_p          - pointer to the context
 * @param   drives_to_remove   - array of the number of drives to remove
 *
 * @return  None
 * 
 * @author
 *  05/02/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static void bender_test_glitch_drive_during_io(fbe_test_rg_configuration_t *rg_config_p,
                                               void                        *context_p,
                                               fbe_u32_t                   *drives_to_remove)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       default_io_threads = 5;
    fbe_u32_t       default_io_seconds = 0;

    /* Unreferenced parameters
     */
    FBE_UNREFERENCED_PARAMETER(context_p);
    FBE_UNREFERENCED_PARAMETER(drives_to_remove);

    /* make sure system verify complete */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* `Glitch' (remove and immediately re-insert) (1) drive from each raid
     * group with I/O running.
     */
    status = fbe_test_sep_io_configure_background_operations(FBE_TRUE, /* Enable background ops */
                                                             FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_DRIVE_GLITCH /* Glitch a drive */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Increase the number of threads and the I/O seconds in order to test
     * the quiesce code.
     */
    default_io_threads = fbe_test_sep_io_get_threads(FBE_U32_MAX);
    fbe_test_sep_io_set_threads(24);
    default_io_seconds = fbe_test_sep_io_get_io_seconds();
    fbe_test_sep_io_set_io_seconds(30);
    
    /* Run standard I/O test suite.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD,
                                      BENDER_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      BENDER_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_FALSE,                            /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      FBE_TRUE,                             /* bender only runs as a dualsp test */
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Restore the I/O threads and seconds back to default.
     */
    fbe_test_sep_io_set_threads(default_io_threads);
    fbe_test_sep_io_set_io_seconds(default_io_seconds);

    /* Now disable `glitch' drive background operation.
     */
    status = fbe_test_sep_io_configure_background_operations(FBE_FALSE, /* Disable background ops */
                                                             FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID /* Disable all */);   
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    return;
}
/******************************************* 
 * end bender_test_glitch_drive_during_io()
 *******************************************/

/*!****************************************************************************
 *  bender_test_validate_lun_rebuild_notifications()
 ******************************************************************************
 *
 * @brief
 *   This test validates that the proper lun rebuild notifications are generated.
 *   It test both single drive rebuilds and two drive rebuilds (it expects the
 *   percentage rebuilt to be adjusted accordingly).
 * 
 *   Steps:
 *   - Degrade the raid groups (depending on raid type one or two drive are removed)
 *   - Generate `distributed' writes to all raid groups
 *   - Register for the lun rebuild notifications
 *   - Re-insert the removed drives
 *   - Validate the proper notifications are generated
 *   - Generate `distributed' reads - validate data is correct
 *
 * @param   rg_config_p     - pointer to the RG configuration info
 * @param   context_p          - pointer to the context
 * @param   drives_to_remove   - array of the number of drives to remove
 *
 * @return  None
 * 
 * @author
 *  05/02/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static void bender_test_validate_lun_rebuild_notifications(fbe_test_rg_configuration_t *rg_config_p,
                                                           void                        *context_p,
                                                           fbe_u32_t                   *drives_to_remove)
{
    fbe_api_terminator_device_handle_t  drive_info[BENDER_TEST_CONFIGS][3];                   //  drive info needed for reinsertion
    fbe_status_t                        status;
    fbe_u32_t                           i;
    fbe_u32_t                           rg_index;
    fbe_u32_t                           drive_slots_to_be_removed[BENDER_TEST_CONFIGS][3];
    fbe_object_id_t                     raid_group_object_id[BENDER_TEST_CONFIGS];
    fbe_lba_t                           rebuild_start_checkpoint[BENDER_TEST_CONFIGS];
    fbe_lba_t                           rebuild_progress_checkpoint[BENDER_TEST_CONFIGS];
    fbe_lba_t                           rebuild_end_checkpoint[BENDER_TEST_CONFIGS];
    fbe_test_rg_configuration_t        *current_rg_config_p = rg_config_p;
    fbe_u32_t                          *drives_to_remove_p;
    fbe_notification_info_t             notification_info;
    fbe_object_id_t                     rg_object_id;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_object_id_t                     lun_object_id;
    fbe_lba_t                           chunk_size = 0;
    fbe_object_id_t                     mirror_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           adjusted_position = 0;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    /* Lower the peer update checkpoint interval from default.
     */
    fbe_test_sep_util_set_update_peer_checkpoint_interval(1);

    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Use default */
        mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
        drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_0;
        drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_1;
        drive_slots_to_be_removed[rg_index][2] = SEP_REBUILD_UTILS_POSITION_2;

        /* RAID-10 raid groups need their object ID and disk position adjusted to reflect the underlying mirror
         * object. Otherwise use the object id already found and the input position.
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(current_rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                                  &mirror_obj_id, &adjusted_position);
            raid_group_object_id[rg_index] = mirror_obj_id;
        }
        else
        {
            /* Get the RG's object id. */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);
        }
        current_rg_config_p++;
    }

    //  Get the number of physical objects in existence at this point in time.  This number is
    //  used when checking if a drive has been removed or has been reinserted.
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        //  Write a seed pattern to the RG
        sep_rebuild_utils_write_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
    }

    current_rg_config_p = rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        for (i=0;i<*drives_to_remove_p;i++)
        {
            /* go through all the RG for which user has provided configuration data. */
            sep_rebuild_utils_number_physical_objects_g -= 1;

            //  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
            //  is marked.
            sep_rebuild_utils_remove_drive_and_verify(current_rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
        }
        current_rg_config_p++;
        drives_to_remove_p++;
    }

    /* Validate background pattern
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
    sep_rebuild_utils_read_bg_pattern(&fbe_bender_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the chunk size
         */
        if (chunk_size == 0)
        {
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            chunk_size = rg_info.lun_align_size / rg_info.num_data_disk;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
        sep_rebuild_utils_write_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 0x200);

        /* Write to something outside the first chunk. 
         * This ensures that it takes more than just reaching the end of the first chunk for the rebuild to get 
         * completed. 
         * We choose to write starting at the beginning of the third LUN. 
         */
        sep_rebuild_utils_write_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 
                                         rg_chunk_stripe_blocks[rg_index]);
        sep_rebuild_utils_write_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 
                                         rg_chunk_stripe_blocks[rg_index] * BENDER_CHUNKS_PER_LUN * 2 );
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        /* Add a debug hook when the raid group starts the rebuild.
         */
        rebuild_start_checkpoint[rg_index] = 0x0;
        mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...obj: 0x%x rebuild start: 0x%llx", 
                   raid_group_object_id[rg_index], 
                   (unsigned long long)rebuild_start_checkpoint[rg_index]);
        status = fbe_test_add_debug_hook_active(raid_group_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                0,
                                                0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Add a debug hook when the raid group has rebuilt (1) chunk (physical)
         * chunk of user space.
         */
        rebuild_progress_checkpoint[rg_index] = chunk_size;
        mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...obj: 0x%x rebuild progress: 0x%llx", 
                   raid_group_object_id[rg_index],
                   (unsigned long long)rebuild_progress_checkpoint[rg_index]);
        status = fbe_test_add_debug_hook_active(raid_group_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                rebuild_progress_checkpoint[rg_index],
                                                NULL,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Add a debug hook when the raid group has rebuilt all the user space
         */
        rebuild_end_checkpoint[rg_index] = (chunk_size * BENDER_CHUNKS_PER_LUN * fbe_test_sep_util_get_chunks_per_rebuild());
        mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...obj: 0x%x rebuild end: 0x%llx", 
                   raid_group_object_id[rg_index],
                   (unsigned long long)rebuild_end_checkpoint[rg_index]);
        status = fbe_test_add_debug_hook_active(raid_group_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                rebuild_end_checkpoint[rg_index],
                                                NULL,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate lun reconstruction start notification ==");
    current_rg_config_p = rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        sep_rebuild_utils_number_physical_objects_g += 1;

        /* Pick the first lun on the raid group
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Setup notification to wait for.
         */
        status = fbe_test_common_set_notification_to_wait_for(lun_object_id,
                                                              FBE_TOPOLOGY_OBJECT_TYPE_LUN,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Reinsert the drive and wait for the rebuild to start */
        sep_rebuild_utils_reinsert_drive_and_verify(current_rg_config_p, drive_slots_to_be_removed[rg_index][0], 
                                                    sep_rebuild_utils_number_physical_objects_g, 
                                                    &drive_info[rg_index][0]);
        if (*drives_to_remove_p == 2)
        {
            fbe_api_sleep(500);
            sep_rebuild_utils_number_physical_objects_g += 1;
            sep_rebuild_utils_reinsert_drive_and_verify(current_rg_config_p, 
                                                        drive_slots_to_be_removed[rg_index][1], 
                                                        sep_rebuild_utils_number_physical_objects_g, 
                                                        &drive_info[rg_index][1]);
        }

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                       30000, 
                                                       &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct start notification.
         */
        MUT_ASSERT_INT_EQUAL(0, notification_info.notification_data.data_reconstruction.percent_complete);
        MUT_ASSERT_INT_EQUAL(DATA_RECONSTRUCTION_START, notification_info.notification_data.data_reconstruction.state);

        current_rg_config_p++;
        drives_to_remove_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate lun reconstruction progress notification ==");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
        /* If this raid group is not enabled goto the next
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Pick the first lun on the raid group
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Setup notification to wait for.
         */
        status = fbe_test_common_set_notification_to_wait_for(lun_object_id,
                                                              FBE_TOPOLOGY_OBJECT_TYPE_LUN,
                                                              FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                              FBE_STATUS_OK,
                                                              FBE_JOB_SERVICE_ERROR_NO_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove the rebuilt start debug hook and wait for the rebuild
         * checkpoint debug hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...obj: 0x%x rebuild start", 
                   raid_group_object_id[rg_index]);
        status = fbe_test_del_debug_hook_active(raid_group_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                0,
                                                0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        sep_rebuild_utils_wait_for_rb_to_start(current_rg_config_p, drive_slots_to_be_removed[rg_index][0]);

        mut_printf(MUT_LOG_TEST_STATUS, "Wait for rebuild checkpoint Debug Hook...");
        status = fbe_test_wait_for_debug_hook_active(raid_group_object_id[rg_index], 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                            SCHEDULER_CHECK_VALS_GT, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE, 
                                            rebuild_progress_checkpoint[rg_index], 
                                            NULL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate the reconstruct progress and the correct percent rebuilt (20 %)
         */
        MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.percent_complete >= 2) &&
                        (notification_info.notification_data.data_reconstruction.percent_complete < 50)    );
        MUT_ASSERT_INT_EQUAL(DATA_RECONSTRUCTION_IN_PROGRESS, notification_info.notification_data.data_reconstruction.state);


        current_rg_config_p++;
    }

    /* Wait for end notification
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate lun reconstruction end notification ==");
    current_rg_config_p = rg_config_p;
    drives_to_remove_p = drives_to_remove;
    for (rg_index = 0; rg_index < BENDER_TEST_CONFIGS; rg_index++)
    {
       /* If this raid group is not enabled goto the next
        */
       if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
       {
           current_rg_config_p++;
           continue;
       }

       /* Pick the first lun on the raid group
        */
       status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number, &lun_object_id);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

       /* Setup notification to wait for.
        */
       status = fbe_test_common_set_notification_to_wait_for(lun_object_id,
                                                             FBE_TOPOLOGY_OBJECT_TYPE_LUN,
                                                             FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION,
                                                             FBE_STATUS_OK,
                                                             FBE_JOB_SERVICE_ERROR_NO_ERROR);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Remove the rebuild progress hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...obj: 0x%x rebuild progress", 
                   raid_group_object_id[rg_index]);
        status = fbe_test_del_debug_hook_active(raid_group_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                rebuild_progress_checkpoint[rg_index],
                                                NULL,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


        /* Wait for the notification. */
        status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate either progress or end
         */
        if (notification_info.notification_data.data_reconstruction.state == DATA_RECONSTRUCTION_END)
        {
            MUT_ASSERT_INT_EQUAL(notification_info.notification_data.data_reconstruction.percent_complete, 100);
        }
        else
        {
            MUT_ASSERT_TRUE((notification_info.notification_data.data_reconstruction.percent_complete >= 6) &&
                            (notification_info.notification_data.data_reconstruction.percent_complete < 100)    );
            MUT_ASSERT_INT_EQUAL(DATA_RECONSTRUCTION_IN_PROGRESS, notification_info.notification_data.data_reconstruction.state);
        }

        /* Remove the rebuild end hook.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...obj: 0x%x rebuild end", 
                   raid_group_object_id[rg_index]);
        status = fbe_test_del_debug_hook_active(raid_group_object_id[rg_index],
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                rebuild_end_checkpoint[rg_index],
                                                NULL,
                                                SCHEDULER_CHECK_VALS_GT,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Validate rebuild completes.
         */
        for (i=0;i<*drives_to_remove_p;i++)
        {
            /* Wait until the rebuilds finish */
            sep_rebuild_utils_wait_for_rb_comp(current_rg_config_p, drive_slots_to_be_removed[rg_index][i]);
        }

        sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

        /* Verify the SEP objects is in expected lifecycle State  */
        status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
                                                         FBE_LIFECYCLE_STATE_READY,
                                                         BENDER_WAIT_MSEC,
                                                         FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Wait for all lun objects of this raid group to be ready */
        status = fbe_test_sep_util_wait_for_all_lun_objects_ready(current_rg_config_p,
                                                                  fbe_api_transport_get_target_server());
        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure. */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                       current_rg_config_p->raid_group_id);
            break;
        }

        /* Read the data and make sure it matches the new seed pattern */
        sep_rebuild_utils_verify_new_data(current_rg_config_p, &fbe_bender_test_context_g[0], 0x200);
        current_rg_config_p++;
        drives_to_remove_p++;
    }

    sep_rebuild_utils_validate_event_log_errors();
    
    /* Set the peer update checkpoint interval back to the default
     */
    fbe_test_sep_util_set_update_peer_checkpoint_interval(FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_SECS);

    return;

}
/*******************************************************
 * end bender_test_validate_lun_rebuild_notifications()
 *******************************************************/


/************************* 
 * end file bender_test.c
 *************************/

