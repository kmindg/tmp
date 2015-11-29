/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file pooh_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of hibernation with rebuild on a single SP.  
 *
 * @version
 *   12/01/2009 - Created. Jean Montes
 *   11/15/2010 - Implement the test
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "mut.h"                                    //  MUT unit testing infrastructure 
#include "fbe/fbe_types.h"                          //  general types
#include "sep_tests.h"                              //  for common test functions (sep_config_load_sep_and_neit...)
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_power_saving_interface.h"     //  for fbe_api_power_save_...
#include "fbe/fbe_api_database_interface.h"    //  for fbe_api_database_...
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait...
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "sep_rebuild_utils.h"       //  SEP rebuild utils
#include "fbe_test_common_utils.h" 



#include "pp_utils.h"                              //  for common test functions (fbe_test_pp_util_pull_drive...)

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* pooh_short_desc = "RAID Group Hibernation and Rebuild - Single SP";
char* pooh_long_desc =
    "\n"
    "\n"
    "The Pooh Scenario tests interactions between hibernation and rebuild, for a\n"
    "RAID-5.  It tests that the RAID group will continue rebuilding if a hibernate\n"
    "request is sent to it, and only hibernate after the rebuild completes.\n"
    "\n"
    "Dependencies:\n"
    "   - RAID group and other objects support hibernation (Aurora scenario)\n"
    "   - RAID group object supports rebuild (Diego scenario)\n"
    "   - RAID group object supports degraded I/O for a RAID-5 (Big Bird  \n"
    "     scenario)\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada boards\n"
    "    [PP] 1 - SAS PMC port\n"
    "    [PP] 3 - viper enclosure\n"
    "    [PP] 60 - SAS drives (PDOs)\n"
    "    [PP] 60 - logical drives (LDs)\n"
    "    [SEP] 60 - provision drives (PVDs)\n"
    "    [SEP] 1 - RAID object (RAID-5)\n"
    "    [SEP] 1 - LUN object \n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Configure a Hot Spare drive\n"
    "\n"
    "STEP 3: Set idle time to hibernate to 10 seconds\n" 
    "\n"
    "STEP 4: Remove one of the drives in the RG (drive A)\n"
    "    - Remove the physical drive (PDO-A)\n"
    "    - Verify that VD-A is now in the FAIL state\n"  
    "    - Verify that the RAID object is in the READY state\n"
    "\n"
    "STEP 5: Verify that the Hot Spare swaps in for drive A\n"
    "    - Verify that VD-A is now in the READY state\n"  
    "\n"
    "STEP 6: Verify that rebuild is in progress\n" 
    "    - Verify that the RAID object updates the rebuild checkpoint as needed\n"  
    "\n"
    "STEP 7: Wait 6 seconds and verify the RAID object does not hibernate\n"
    "    - Verify that the RAID object is in the READY state\n"
    "\n"
    "STEP 8: Wait for the rebuild to complete\n" 
    "    - Verify that the Needs Rebuild chunk count is 0\n"
    "    - Verify that the rebuild checkpoint is set to LOGICAL_EXTENT_END\n"
    "\n"
    "STEP 9: Wait 6 seconds and verify the RAID object enters hibernate\n"
    "    - Verify that the RAID object is in the HIBERNATE state\n"
    "\n"
    "STEP 10: Cleanup\n"
    "    - Destroy objects\n"
    "\n"
    "\n"
    "*** Pooh Phase II ********************************************************* \n"
    "\n"
    "- Test all RAID types\n"
    "\n"
    "\n"
    "Description last updated: 10/27/2011.\n"
    "\n"
    "\n";


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:


//  Configuration parameters
#define POOH_TEST_RAID_GROUP_WIDTH 3
#define POOH_TEST_RAID_GROUP_COUNT 1
#define POOH_TEST_HS_COUNT 1
#define POOH_TEST_LUNS_PER_RAID_GROUP 1
#define POOH_TEST_LUN_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY - 0x1000)
#define POOH_TEST_ELEMENT_SIZE 128
#define POOH_TEST_POSITION_TO_REMOVE 0
#define POOH_TIME_TO_HIBERNATE 15
#define POOH_TIME_WAIT_FOR_STATE 200000
//  Set of disks to be used - disks 0-0-4 through 0-0-6
fbe_test_raid_group_disk_set_t fbe_pooh_disk_set[POOH_TEST_RAID_GROUP_WIDTH] = 
{
    {0, 0, 4},     // 0-0-4
    {0, 0, 5},     // 0-0-5
    {0, 0, 6}      // 0-0-6
};  

/*!*******************************************************************
 * @var pooh_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t pooh_raid_group_config[] = 
{
    /* width,                          capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {POOH_TEST_RAID_GROUP_WIDTH,       TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

fbe_api_rdgen_context_t      pooh_test_context[POOH_TEST_LUNS_PER_RAID_GROUP];


static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
static void pooh_logical_config(void);
static void pooh_enable_power_saving(fbe_test_rg_configuration_t *  current_rg_configuration_p);
static void pooh_pull_drive(fbe_test_raid_group_disk_set_t * drive_to_remove_p,
                            fbe_test_rg_configuration_t *  current_rg_configuration_p);
static void pooh_verify_hs_swap_in(fbe_test_rg_configuration_t *  current_rg_config_p);
static void pooh_verify_rebuild_in_progress(fbe_test_rg_configuration_t *  current_rg_config_p);
static void pooh_verify_rg_state(fbe_test_rg_configuration_t *  current_rg_config_p, 
                                 fbe_lifecycle_state_t expected_lifecycle_state, 
                                 fbe_u32_t timeout_ms);
static void pooh_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);

//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  pooh_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Pooh test.  The test does the 
 *   following:
 *
 *   - Puts the RG and physical drives into Hibernation
 *   - Removes one of the drives and verifies the objects are in the correct state (RG/drives out of hiberation)
 *   - Verify that a HS swaps in for the failed drive and is also in the correct state
 *   - Verify that during Rebuild objects in correct state
 *   - Verify that Rebuild completes successfully
 *   - Verify that RG and drives go back into Hibernation
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void pooh_test(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                     raid_group_id;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &pooh_raid_group_config[0];

    status = fbe_api_database_lookup_raid_group_by_number(pooh_raid_group_config[0].raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* for debug purpose, register state change notification, so we can tell the timing*/
    fbe_semaphore_init(&sem, 0, 1);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY| FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP,
                                                                  pooh_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    /* STEP 1: Set idle time to hibernate and wait for RG and physical drive to get into Hibernation */
    pooh_enable_power_saving(&pooh_raid_group_config[0]);

    /* STEP 2: Remove one of the drives in the RG (drive A)
     *     - Remove the physical drive (PDO-A)
     *     - Verify that VD-A is now in the FAIL state
     *     - Verify that the RAID object is in the READY state
     */ 
	pooh_pull_drive(&(rg_config_p->rg_disk_set[POOH_TEST_POSITION_TO_REMOVE]),
                    &pooh_raid_group_config[0]);
    /* STEP 3: Verify that the Hot Spare swaps in for drive A
     *     - Verify that VD-A is now in the READY state
     */
    /* Speed up VD hot spare */
	fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */
    pooh_verify_hs_swap_in(&pooh_raid_group_config[0]);

    /* STEP 4: Verify that rebuild is in progress
     *    - Verify that the RAID object updates the rebuild checkpoint as needed
     */
    pooh_verify_rebuild_in_progress(&pooh_raid_group_config[0]);

    /* STEP 5: Wait and verify the RAID object does not hibernate
     *     - Verify that the RAID object is in the READY state
     */
    pooh_verify_rg_state(&pooh_raid_group_config[0], FBE_LIFECYCLE_STATE_READY, POOH_TIME_TO_HIBERNATE);

    /* STEP 6: Wait for the rebuild to complete
     *     - Verify that the Needs Rebuild chunk count is 0
     *     - Verify that the rebuild checkpoint is set to LOGICAL_EXTENT_END
     */
    sep_rebuild_utils_wait_for_rb_comp(&pooh_raid_group_config[0], POOH_TEST_POSITION_TO_REMOVE);
    mut_printf(MUT_LOG_TEST_STATUS, "== rebuild completed ==");

    sep_rebuild_utils_check_bits(raid_group_id);


    /* STEP 7: Verify the RAID object enters hibernate
     *     - Verify that the RAID object is in the HIBERNATE state
     */
    pooh_verify_rg_state(&pooh_raid_group_config[0], FBE_LIFECYCLE_STATE_HIBERNATE, POOH_TIME_TO_HIBERNATE);

    status = fbe_api_notification_interface_unregister_notification(pooh_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Turn off delay for I/O completion in the terminator */
    fbe_api_terminator_set_io_global_completion_delay(0);

    fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */

    return;

} //  End pooh_test()


/*!****************************************************************************
 *  pooh_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the Pooh test.  It creates the objects 
 *   for a RAID-5 RG based on the defined configuraton.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void pooh_setup(void)
{ 
    /* Only load physical config in simulation.
      */
    if (fbe_test_util_is_simulation())
    {
    /* this function loads 2 enclosures of 520 SAS drives
     * and 2 enclosures of 512 SAS drive.
     */
       elmo_physical_config();
    }
    pooh_logical_config();
    return;
} //  End pooh_setup()


/*!****************************************************************************
 *  pooh_cleanup
 ******************************************************************************
 *
 * @brief
 *   This function unloads the logical and physical packages and destroy the configuration.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void pooh_cleanup(void)
{
    // Only destroy config in simulation  
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "===  pooh cleanup starts ===\n");
        fbe_test_sep_util_destroy_neit_sep_physical();
        mut_printf(MUT_LOG_LOW, "=== pooh cleanup completes ===\n");
    }
    return;
}
/***************************************************************
 * end pooh_cleanup()
 ***************************************************************/



/*!****************************************************************************
 *  pooh_logical_config
 ******************************************************************************
 *
 * @brief
 *   This sets up the logical configuration used by the test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_logical_config(void)
{   
	fbe_test_discovered_drive_locations_t drive_locations;
	
    /* Only load config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
    /* Load the logical packages. 
     */
       sep_config_load_sep_and_neit();
    }

    /* Set the trace level to info. */
    fbe_test_sep_util_set_trace_level(FBE_TRACE_LEVEL_INFO, FBE_PACKAGE_ID_SEP_0);

    /* Fill up the disk and LU information to raid group configuration. */
    //fbe_test_sep_util_fill_raid_group_disk_set(&pooh_raid_group_config[0], POOH_TEST_RAID_GROUP_COUNT, 
                                               //fbe_pooh_disk_set, NULL);
    /*init the raid group configuration*/
    fbe_test_sep_util_init_rg_configuration(pooh_raid_group_config);

    /*find all power save capable drive information from disk*/
    fbe_test_sep_util_discover_all_power_save_capable_drive_information(&drive_locations);

    /*fill up the drive information to raid group configuration*/
    fbe_test_sep_util_setup_rg_config(pooh_raid_group_config, &drive_locations);

    /* Fill up the logical unit configuration for each RAID group of this test index. */
    fbe_test_sep_util_fill_raid_group_lun_configuration(&pooh_raid_group_config[0],
                                                        POOH_TEST_RAID_GROUP_COUNT,
                                                        0,         /* first lun number for this RAID group. */
                                                        POOH_TEST_LUNS_PER_RAID_GROUP,
                                                        POOH_TEST_LUN_CAPACITY);

    /* Kick of the creation of all the raid group with Logical unit configuration. */
    fbe_test_sep_util_create_raid_group_configuration(&pooh_raid_group_config[0],POOH_TEST_RAID_GROUP_COUNT);      

    return;
} //  End pooh_logical_config()


/*!****************************************************************************
 *  pooh_enable_power_saving
 ******************************************************************************
 *
 * @brief
 *   This function enables power savings for this test on the given configuration.
 *
 * @param   current_rg_configuration_p  - the test config
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_enable_power_saving(fbe_test_rg_configuration_t *  current_rg_configuration_p)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t           disk_index;
    fbe_lun_set_power_saving_policy_t           lun_ps_policy;

    mut_printf(MUT_LOG_TEST_STATUS, "=== setting power saving policy on LUN %d ===\n", 
           current_rg_configuration_p->logical_unit_configuration_list[0].lun_number);

    /* Get the LUN object ID */
    status = fbe_api_database_lookup_lun_by_number(current_rg_configuration_p->logical_unit_configuration_list[0].lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* Set the LUN power-saving policy */
    lun_ps_policy.lun_delay_before_hibernate_in_sec = POOH_TIME_TO_HIBERNATE;/*this is for delay before hibernation*/
    lun_ps_policy.max_latency_allowed_in_100ns = (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000);/*we allow 1000 minutes for the drives to wake up (default for drives is 100 minutes)*/
    status = fbe_api_lun_set_power_saving_policy(object_id, &lun_ps_policy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start for RAID group:%d ...", __FUNCTION__, current_rg_configuration_p->raid_group_id);

    /* Get the RAID group object-id */
    status = fbe_api_database_lookup_raid_group_by_number(current_rg_configuration_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Set the power-savings idle time on the RG */
    status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id, POOH_TIME_TO_HIBERNATE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Enable power-saving */
    fbe_test_sep_util_enable_system_power_saving();

    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set power-saving idle time on the disks in the RG */
    for (disk_index = 0; disk_index < POOH_TEST_RAID_GROUP_WIDTH ; disk_index++) {
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_configuration_p->rg_disk_set[disk_index].bus, 
                                                                current_rg_configuration_p->rg_disk_set[disk_index].enclosure,
                                                                current_rg_configuration_p->rg_disk_set[disk_index].slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		/* Disable background zeroing so that drives go into power saving mode*/
        status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, POOH_TIME_TO_HIBERNATE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, disk_index, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, POOH_TIME_TO_HIBERNATE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    
    /* Make sure the RG is in the hibernate state */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                     POOH_TIME_WAIT_FOR_STATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure the drives are in the hibernate state */
    for (disk_index = 0; disk_index < POOH_TEST_RAID_GROUP_WIDTH ; disk_index++) {
        status = fbe_api_get_physical_drive_object_id_by_location(current_rg_configuration_p->rg_disk_set[disk_index].bus, 
                                                                current_rg_configuration_p->rg_disk_set[disk_index].enclosure,
                                                                current_rg_configuration_p->rg_disk_set[disk_index].slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                         POOH_TIME_WAIT_FOR_STATE, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: PDO %d-%d-%d is in Hibernate state", __FUNCTION__, 
                   current_rg_configuration_p->rg_disk_set[disk_index].bus, 
                   current_rg_configuration_p->rg_disk_set[disk_index].enclosure,
                   current_rg_configuration_p->rg_disk_set[disk_index].slot);
    }

    return;
}


/*!****************************************************************************
 *  pooh_pull_drive
 ******************************************************************************
 *
 * @brief
 *   This function pulls the given drive to simulate a drive failure and ensures it is in the correct state
 *
 * @param   drive_to_remove_p       - the drive to remove
 *          current_rg_config_p     - the test config
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_pull_drive(fbe_test_raid_group_disk_set_t * drive_to_remove_p,
                            fbe_test_rg_configuration_t *  current_rg_config_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t     vd_object_id = FBE_OBJECT_ID_INVALID;

     /* Pull the specific drive from the enclosure. */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                             drive_to_remove_p->enclosure, 
                                             drive_to_remove_p->slot,
                                             &current_rg_config_p->drive_handle[POOH_TEST_POSITION_TO_REMOVE]);

    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                 drive_to_remove_p->enclosure, 
                                                 drive_to_remove_p->slot);
    }

	mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s removed drive %d-%d-%d successful ==", 
               __FUNCTION__,
               drive_to_remove_p->bus, 
               drive_to_remove_p->enclosure, 
               drive_to_remove_p->slot);

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, POOH_TEST_POSITION_TO_REMOVE, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     POOH_TIME_WAIT_FOR_STATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: VD obj-0x%x is in failed state! ==", __FUNCTION__, vd_object_id);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     POOH_TIME_WAIT_FOR_STATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: RG obj-0x%x is in ready state! ==", __FUNCTION__, rg_object_id);
    return;
}


/*!****************************************************************************
 *  pooh_verify_hs_swap_in
 ******************************************************************************
 *
 * @brief
 *   This function verifies a hot-spare has been swapped in for a failed drive
 *
 * @param   current_rg_config_p     - the test config
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_verify_hs_swap_in(fbe_test_rg_configuration_t *  current_rg_config_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t     vd_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, POOH_TEST_POSITION_TO_REMOVE, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     POOH_TIME_WAIT_FOR_STATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: HS swap-in, VD obj-0x%x is in ready state! ==", __FUNCTION__, vd_object_id);
    return;
}


/*!****************************************************************************
 *  pooh_verify_rebuild_in_progress
 ******************************************************************************
 *
 * @brief
 *   This function verifies that a rebuild is in progress on the given test config
 *
 * @param   current_rg_config_p     - the test config
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_verify_rebuild_in_progress(fbe_test_rg_configuration_t *  current_rg_config_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_raid_group_get_info_t   raid_group_info;                //  rg information from "get info" command
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    //  Get the raid group information
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    //  Rebuild is in progress, if the rebuild checkpoint is set to the logical marker for the given disk. 
    MUT_ASSERT_INTEGER_NOT_EQUAL(FBE_LBA_INVALID, raid_group_info.rebuild_checkpoint[POOH_TEST_POSITION_TO_REMOVE]);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s: rebuild is in progress! checkpoint is 0x%llx ==", __FUNCTION__, 
        (unsigned long long)raid_group_info.rebuild_checkpoint[POOH_TEST_POSITION_TO_REMOVE]);
    return;
}


/*!****************************************************************************
 *  pooh_verify_rg_state
 ******************************************************************************
 *
 * @brief
 *   This function verifies that the given RG is in the given state within the time allotted
 *
 * @param   current_rg_config_p         - the test config
 *          expected_lifecycle_state    - the expected state
 *          timeout_ms                  - max time to wait for the RG to be in the given state
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_verify_rg_state(fbe_test_rg_configuration_t *  current_rg_config_p, 
                                 fbe_lifecycle_state_t expected_lifecycle_state, 
                                 fbe_u32_t timeout_ms)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, expected_lifecycle_state,
                                                     POOH_TIME_WAIT_FOR_STATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s: RG obj-0x%x is in %s! ==", 
               __FUNCTION__, 
               rg_object_id, 
               fbe_api_state_to_string(expected_lifecycle_state));
    return;
}


/*!****************************************************************************
 *  pooh_commmand_response_callback_function
 ******************************************************************************
 *
 * @brief
 *   This function is the callback function for registering for state change notifications
 *
 * @param   update_object_msg       - the object message for the state change
 *          context                 - context
 *
 * @return  None 
 *
 *****************************************************************************/
static void pooh_commmand_response_callback_function(update_object_msg_t * update_object_msg, void *context)
{
	fbe_lifecycle_state_t	state;

	fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);

    mut_printf(MUT_LOG_LOW, "== Pooh received notificiation from SEP: Object 0x%x in %s ==",
               update_object_msg->object_id, fbe_api_state_to_string(state));
    return;
}

