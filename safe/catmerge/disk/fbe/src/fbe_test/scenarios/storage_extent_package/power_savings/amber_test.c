/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file amber_test.c
 ***************************************************************************
 *
 * @brief
 *  The Amber Scenario tests what happens when we swap-in Hibernating Hot 
 *  spare drive to degraded RAID group.
 *
 * @version
 *   12/09/2009 - Created. Dhaval Patel
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe_test_configurations.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "fbe_test_common_utils.h"

char * amber_short_desc = "Swap-In Hibernating Hot Spare drive.";
char * amber_long_desc =
    "\n"
    "\n"
    "The Amber Scenario tests what happens when we swap-in Hibernating Hot spare drive to degraded RAID group.\n"
    "\n"
    "It covers the following case:\n"
    "\t- Removal of the drive from configured RAID group selects Hibernating Hot spare drive for swap-in.\n"
    "\n"
    "Starting Config: \n"
    "\t[PP] an armada board\n"
    "\t[PP] a SAS PMC port\n"
    "\t[PP] a viper enclosure\n"
    "\t[PP] four SAS drives (PDO-A to PDO-D)\n"
    "\t[PP] four logical drives (LDO-A to LDO-D)\n"
    "\t[SEP] four provision drives (PVD-A to PVD-D)\n"
    "\t[SEP] two virtual drives (VD-A and VD-B)\n"
    "\t[SEP] a RAID-1 object (RG)\n"
    "\t[SEP] a LU object (LU)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "\t- Create and verify the initial physical package config.\n"
    "\t- Create the provision drives with I/O edges to the logical drives.\n"
    "\t- Verify that the provision drives are in the READY state.\n"
    "\t- Create virtual drives (VD-A and VD-B) with I/O edges to PVD-A and PVD-B.\n"
    "\t- Verify that VD-A and VD-B are in the READY state.\n"
    "\t- Create a RAID-1 object (RG) with I/O edges to VD-A and VD-B.\n"
    "\t- Verify that RG is in the READY state.\n"
    "\t- Create an LU object (LU) with an I/O edge to RG.\n"
    "\n"
    "STEP 2: Sends an external Hibernate request to PVD-C and PVD-D objects.\n"
    "\t- Verify that PVD-C and PVD-D object changes its state to Hibernate.\n"
    "\n"
    "STEP 3: Remove the physical drive associated with PVD-B object using FBE API interface.\n"
    "\t- Verify that VD-B object changes its state to FAIL and start sparing operation.\n"
    "\t- Verify that VD-B object swap-in Hibernated Hot spare PVD object (PVD-C) with its second edge.\n"
    "\t- Verify that VD-B object finds newly configured edge (edge-1) with downstream object PVD-C in SLUMBER state.\n" 
    "\t- Verify that VD-B object sends an unhibernate usurpur command to PVD-C object.\n"
    "\t- Verify that PVD-C object sends unhibernate request down and waits for the drive to hibernate.\n"
    "\t- Verify that PVD-C object come out of the hibernate state and changes its state to READY.\n"
    "\t- Verify that edge between VD-B object and PVD-C object changes its state to ENABLED.\n"
    "\t- Verify that VD-B object changes its state to ACTIVATE state.\n"
    "\n"
    "\n"
    "Description last updated: 10/28/2011.\n";

/*!*******************************************************************
 * @def AMBER_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief chunks per lun for the qual test. 
 *
 *********************************************************************/
#define AMBER_TEST_CHUNKS_PER_LUN                      3

/* The number of LUNs in our raid group.
 */
#define AMBER_TEST_LUNS_PER_RAID_GROUP                 1

/* Maximum number of hot spares. */
#define AMBER_TEST_HS_COUNT                            2

/* Element size of our LUNs.
 */
#define AMBER_TEST_ELEMENT_SIZE                       128

/*!*******************************************************************
 * @def AMBER_TEST_MAX_RETRIES
 *********************************************************************
 * @brief  max retry count to verify power save state.
 * (number of times to loop)
 *
 *********************************************************************/
#define AMBER_TEST_MAX_RETRIES     20

/*!*******************************************************************
 * @def AMBER_TEST_RETRY_TIME
 *********************************************************************
 * @brief  wait time in ms, in between retries 
 *
 *********************************************************************/
#define AMBER_TEST_RETRY_TIME                    250


typedef struct amber_test_reinsert_ns_context_s
{
    fbe_semaphore_t sem;
    fbe_u32_t slot;
    fbe_u32_t encl;
    fbe_u32_t port;
}
amber_test_reinsert_ns_context_t;

static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;
static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_lifecycle_state_t    expected_state = FBE_LIFECYCLE_STATE_INVALID;


/*!*******************************************************************
 * @var amber_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t amber_raid_group_config[] = 
{
        /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,     520,             0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

fbe_api_rdgen_context_t      amber_test_context[AMBER_TEST_LUNS_PER_RAID_GROUP * 2];

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void amber_test_sep_util_send_job_service_command(fbe_object_id_t  object_id,
                                                  fbe_job_service_control_t  job_control,
                                                  fbe_raid_group_type_t  raid_group_type);

//  A wrapper function over fbe_test_sep_util_send_job_service_command.
void amber_test_drive_insertion_and_verification(fbe_test_rg_configuration_t *  current_rg_configuration_p,
                                                 fbe_u32_t  position_to_reinsert,
                                                 fbe_object_id_t pvd_object_id);

static void amber_swap_in_hibernating_hot_spare_test(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position_to_remove,
                                                     fbe_u32_t number_of_drives_to_remove);

static void amber_bring_spare_drives_to_hibernate_state(fbe_test_raid_group_disk_set_t disk_set[],
                                                        fbe_u32_t num_of_disks);

static void amber_test_enable_system_power_saving(void);

static void amber_test_disable_system_power_saving(void);

static void amber_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);

static void amber_test_verify_power_save_state(fbe_object_id_t object_id);

extern fbe_status_t shaggy_test_config_hotspares(fbe_test_raid_group_disk_set_t *disk_set_p,
                                                 fbe_u32_t no_of_spares);



/*!****************************************************************************
 *  amber_run_test
 ******************************************************************************
 *
 * @brief
 *   Run Amber test for different RAID groups, to test Swap-In of a hibernating
 *   Hot Spare drive.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  9/19/2011 - Created. Sandeep Chaudhari
 *****************************************************************************/
void amber_run_test(fbe_test_rg_configuration_t *rg_config_ptr,void * context_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       rg_index = 0;
    fbe_u32_t       num_raid_groups = 0;
    fbe_test_rg_configuration_t *      rg_config_p = NULL;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    for(rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Create hot spare configuration. */
            status = shaggy_test_config_hotspares(rg_config_p->extra_disk_set,
                                                  rg_config_p->num_of_extra_drives);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            amber_swap_in_hibernating_hot_spare_test(rg_config_p, 0, 1);
        }
    }
}
/******************************************************************************
 * End amber_run_test()
 ******************************************************************************/
 
/*!****************************************************************************
 * amber_swap_in_hibernating_hot_spare_test()
 ******************************************************************************
 * @brief
 *  Run Amber swap-in of hibernating pvd test for the specific RAID group.
 *
 *  The test performs the following tasks:
 *      -Enables the system power-saving mode.
 *      -Brings the Spare drives into hibernating mode.
 *      -Removes a drive from the bound RAID group.
 *      -Verifies that the spare drive comes out of hibernating mode
 *       and the hibernating PVD gets Swapped-In into the RAID group.
 *      -Then, it also verifies that rebuild completes successfully
 *       on drive which has Swapped-In.
 *
 * @param rg_index - Index of the current raid group to be tested.
 *
 * @param position_to_remove - Position of the drive to be removed,
 *                             in the RAID group.
 * @param number_of_drives_to_remove - The number of drives to be
 *                                     removed.
 *
 * @return FBE_STATUS_OK - If test was successful.
 *
 * @author
 *  5/17/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void amber_swap_in_hibernating_hot_spare_test(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position_to_remove,
                                                     fbe_u32_t number_of_drives_to_remove)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_raid_group_disk_set_t *    drive_to_remove_p = NULL;
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     vd_object_id = FBE_OBJECT_ID_INVALID;
    
    fbe_semaphore_init(&sem, 0, 1);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  amber_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start for RAID group:%d....\n", __FUNCTION__, rg_config_p->raid_group_id);

    /* Get the position to remove disk set. */
    drive_to_remove_p = &rg_config_p->rg_disk_set[position_to_remove];

    /* Get the RAID group object-id and virtual drive object-id of the position to reinsert. */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_id);
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus, drive_to_remove_p->enclosure,
                                                            drive_to_remove_p->slot, &pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);
 
    /* Write the background pattern. */
    sep_rebuild_utils_write_bg_pattern(&amber_test_context[0], AMBER_TEST_ELEMENT_SIZE);

    amber_test_sep_util_send_job_service_command(vd_object_id,
                                                 FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
                                                 rg_config_p->raid_type);
    
    /* Enable System power saving mode.
     */
    amber_test_enable_system_power_saving();

    /* Issue hibernate command for the extra disks (i.e. spare drives)
     * This will make the drives to transition to hibernate state.
     */
    amber_bring_spare_drives_to_hibernate_state(rg_config_p->extra_disk_set, rg_config_p->num_of_extra_drives);

    /* Remove a drive from the raid group to let swap in start.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove the Drive, port %d, encl %d, drive %d",
                __FUNCTION__, drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot);

    /* Pull the specific drive from the enclosure. */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                             drive_to_remove_p->enclosure, 
                                             drive_to_remove_p->slot,
                                             &rg_config_p->drive_handle[position_to_remove]);
    }
    else
    {
        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                 drive_to_remove_p->enclosure, 
                                                 drive_to_remove_p->slot);
    }
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify the different object states after drive removal...", __FUNCTION__);

    /* Verify that VD object changed its state to FAIL state before we allow
     * to swap-in the drive.
     * The RAID object will continue to be in ready state.
     */
    shaggy_verify_sep_object_state (vd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    shaggy_verify_sep_object_state (rg_object_id, FBE_LIFECYCLE_STATE_READY);

    /* Hot spare will be swapped-in. */
    amber_test_drive_insertion_and_verification(rg_config_p,
                                                position_to_remove,
                                                pvd_object_id);

    /* Verify whether VD and RAID objects are in READY state. */
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_READY);
    shaggy_verify_sep_object_state(rg_object_id, FBE_LIFECYCLE_STATE_READY);
    
    /* wait for the rebuild completion. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position_to_remove);

    sep_rebuild_utils_check_bits(rg_object_id);

    /*we want to clean and go out w/o leaving the power saving persisted on the disks for next test*/
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system wide power saving \n", __FUNCTION__);
    amber_test_disable_system_power_saving();
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s completed successfully. ****\n", __FUNCTION__);

    status = fbe_api_notification_interface_unregister_notification(amber_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);
}
/******************************************************************************
 * End amber_swap_in_hibernating_hot_spare_test()
 ******************************************************************************/
 
static void amber_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t         *sem = (fbe_semaphore_t *)context;
    fbe_lifecycle_state_t   state;

    fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);

    if (update_object_msg->object_id == expected_object_id && expected_state == state) {
        fbe_semaphore_release(sem, 0, 1 ,FALSE);
    }
}

/*!****************************************************************************
 *  amber_bring_spare_drives_to_hibernate_state()
 ******************************************************************************
 * @brief
 *  This function brings the spare drive into hibernating state.
 *
 *  The function loops thru each spare disk drive and performs
 *  the following tasks:
 *      -Gets Object ID of the provision drive.
 *      -Sets the Power Save idle time of the PVD to a
 *       small unit (i.e.2 sec)
 *      -Waits for the PVD to go into Hibernating state.
 *      -Then, verifies that the Object's life cycle state
 *       is set to FBE_LIFECYCLE_STATE_HIBERNATE.
 *      -and, the objects power_save_state is set to 
 *       FBE_POWER_SAVE_STATE_SAVING_POWER.
 *
 * @param hs_configuration - The list of spare disk drives.
 *
 * @param num_of_disks - Number of disks in the above list (i.e. the list of hot spares)
 *
 * @author
 *  5/17/2010 - Created. Dhaval Patel
 *  9/19/2011 - Modified Sandeep Chaudhari 
 ******************************************************************************/
static void amber_bring_spare_drives_to_hibernate_state(fbe_test_raid_group_disk_set_t disk_set[],
                                                        fbe_u32_t num_of_disks)
{
    fbe_u32_t                                   disk_index;
    fbe_status_t                                status;
    fbe_object_id_t                             object_id;
    fbe_lifecycle_state_t                       object_state;
    DWORD                                       dwWaitResult;

    for (disk_index = 0; disk_index < num_of_disks; disk_index++ )
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_set[disk_index].bus,
                                                                disk_set[disk_index].enclosure,
                                                                disk_set[disk_index].slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the object_id and state for which notification is expected.
         */
        expected_object_id = object_id;/*we want to get notification from this drive*/
        expected_state = FBE_LIFECYCLE_STATE_HIBERNATE;

        /* Set the objects power save idle time to a small unit (2).
         */
        status = fbe_api_set_object_power_save_idle_time(object_id, 2);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "=== waiting for unbound PVD to be in hibernate in ~2 minute ===\n");

        /* wait about 2 minutes more from the timer we enabled hibernation for all
         * so unbound PVDs will get to hibernation in 2 minutes.
         */
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 130000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
        mut_printf(MUT_LOG_TEST_STATUS, " === Verified unbound PVD is hibernating in 2 minutes ===\n");
    }
    
    /* Now verify that PVD of each spare drive is in hibernating state.
     */
    for (disk_index = 0; disk_index < num_of_disks; disk_index++ )
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_set[disk_index].bus,
                                                                disk_set[disk_index].enclosure,
                                                                disk_set[disk_index].slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);

        /* Verify that the power saving state has been set for the object.
         */
        amber_test_verify_power_save_state(object_id);
    }
}
/******************************************************************************
 * End amber_bring_spare_drives_to_hibernate_state()
 ******************************************************************************/

/*!****************************************************************************
 *  amber_test_enable_system_power_saving
 ******************************************************************************
 *
 * @brief
 *   This function enables the overall system power saving feature and
 *   then make sure that overall system power saving feature is enabled.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  9/19/2011 - Created. Sandeep Chaudhari
 *****************************************************************************/
static void amber_test_enable_system_power_saving(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;
    
    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);
}
/******************************************************************************
 * End amber_test_enable_system_power_saving()
 ******************************************************************************/

/*!****************************************************************************
 *  amber_test_disable_system_power_saving
 ******************************************************************************
 *
 * @brief
 *   This function disables the overall system power saving feature and
 *   then make sure that overall system power saving feature is disabled.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  9/19/2011 - Created. Sandeep Chaudhari
 *****************************************************************************/
static void amber_test_disable_system_power_saving(void)
{
    fbe_system_power_saving_info_t              power_save_info;
    fbe_status_t                                status;
    
    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Disabling system wide power saving ===\n", __FUNCTION__);

    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_FALSE);
}
/******************************************************************************
 * End amber_test_disable_system_power_saving()
 ******************************************************************************/
 
/*!**************************************************************************
 *  amber_test_verify_power_save_state
 ****************************************************************************
 * @brief
 *   This function checks PVD's power save state in wait loop. 
 * 
 *
 * @param   None
 *
 * @return  None 
 *
 ***************************************************************************/
static void amber_test_verify_power_save_state(fbe_object_id_t object_id)
{

    fbe_status_t                        status;                         //  fbe status
    fbe_u32_t                           count;                          //  loop count
    fbe_u32_t                           max_retries;                    //  maximum number of retries
    fbe_base_config_object_power_save_info_t    object_ps_info;


    /*  Set the max retries in a local variable. 
     *  (This allows it to be changed within the debugger if needed.)
     */
    max_retries = AMBER_TEST_MAX_RETRIES;

        
    /*  Loop until the object state set to power save */
    for (count = 0; count < max_retries; count++)
    {
        /* get the PVD power save info */
        status = fbe_api_get_object_power_save_info(object_id, &object_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*  Check if PVD object state  */
        if (object_ps_info.power_save_state == FBE_POWER_SAVE_STATE_SAVING_POWER)
        {
            mut_printf(MUT_LOG_LOW, "=== Object state is saving power  ===\n"); 
            return;
        }
        /*  Wait before trying again */
        fbe_api_sleep(AMBER_TEST_RETRY_TIME);
    }

    /*  power save state was not set in the the time limit - assert */
    MUT_ASSERT_TRUE(0);

    /*  Return (should never get here)  */
    return;

}   
/******************************************************************************
 * end amber_test_verify_power_save_state()
 ******************************************************************************/

/*!****************************************************************************
 * amber_test_reinsert_drive_callback_function()
 ******************************************************************************
 * @brief
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void amber_test_reinsert_drive_callback_function(update_object_msg_t * update_object_msg,
                                                  void *context2)
{
    amber_test_reinsert_ns_context_t *     amber_reinsert_context_p = (amber_test_reinsert_ns_context_t * )context2;
    fbe_u32_t                               port_number = 0;
    fbe_u32_t                               encl_number = 0;
    fbe_u32_t                               slot_number = 0;
    fbe_topology_object_type_t              object_type;
    fbe_status_t                            status;
    fbe_lifecycle_state_t                   lifecycle_state;

    status = fbe_api_get_object_port_number(update_object_msg->object_id, &port_number);
    status = fbe_api_get_object_enclosure_number(update_object_msg->object_id, &encl_number);
    status = fbe_api_get_object_drive_number(update_object_msg->object_id, &slot_number);
    status = fbe_api_get_object_type(update_object_msg->object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
    status = fbe_api_get_object_lifecycle_state(update_object_msg->object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    
    /* Release the semaphore only when specific logical drive object becomes ready.
     */
    if((object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) &&
       (port_number == amber_reinsert_context_p->port) &&
       (encl_number == amber_reinsert_context_p->encl) &&
       (slot_number == amber_reinsert_context_p->slot) &&
       (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
    {

        mut_printf(MUT_LOG_TEST_STATUS, "%s: Logical drive object (object-id=%d) is in READY state.",
            __FUNCTION__,
            update_object_msg->object_id);
        fbe_semaphore_release(&(amber_reinsert_context_p->sem), 0, 1, FALSE);
    }
    return ;
}
/******************************************************************************
 * end amber_test_reinsert_drive_callback_function()
 ******************************************************************************/

/*!****************************************************************************
 * amber_reinsert_drive()
 ******************************************************************************
 * @brief
 *  It is used to reinsert the same drive.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void amber_reinsert_drive(fbe_test_rg_configuration_t * rg_config_p,
                           fbe_u32_t position_to_insert)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    amber_test_reinsert_ns_context_t    amber_ns_context;
    fbe_notification_registration_id_t  notification_registration_id;
    fbe_test_raid_group_disk_set_t     *drive_to_insert_p = NULL;

    drive_to_insert_p = &rg_config_p->rg_disk_set[position_to_insert];

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinsert the Drive, port %d, encl %d, drive %d",
                __FUNCTION__, drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);

    /*Register with notification service for READY state.
     */
    amber_ns_context.port = drive_to_insert_p->bus;
    amber_ns_context.encl = drive_to_insert_p->enclosure;
    amber_ns_context.slot = drive_to_insert_p->slot;
    fbe_semaphore_init(&(amber_ns_context.sem), 0, 1);       

    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  amber_test_reinsert_drive_callback_function,
                                                                  &amber_ns_context,
                                                                  &notification_registration_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    /* Reinsert the drive. */
    if(fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                 drive_to_insert_p->enclosure, 
                                                 drive_to_insert_p->slot,
                                                 rg_config_p->drive_handle[position_to_insert]);
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                    drive_to_insert_p->enclosure, 
                                                    drive_to_insert_p->slot);
    }
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Waiting for Logical Drive Object to change state back to ready", __FUNCTION__);
    fbe_semaphore_wait(&(amber_ns_context.sem), NULL);


    status = fbe_api_notification_interface_unregister_notification(amber_test_reinsert_drive_callback_function,
                                                                    notification_registration_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&amber_ns_context.sem); /* SAFEBUG - needs destroy */

    /* Verify the logical drive object state. */
    shaggy_verify_pdo_state(drive_to_insert_p->bus,
                            drive_to_insert_p->enclosure,
                            drive_to_insert_p->slot,
                            FBE_LIFECYCLE_STATE_READY,
                            FBE_FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}
/******************************************************************************
 * end amber_reinsert_drive()
 ******************************************************************************/

/*!****************************************************************************
 *  amber_test_drive_insertion_and_verification
 ******************************************************************************
 * @brief
 *   This function performs the tasks required after a drive has been 
 *   removed and verifies for the expected status.
 *   The status expected and action to be performed, depends on raid type.
 *
 *   For a non-retundand raid type (RAID0), sparing does not takes place
 *   so, the same drive is inserted back and verified.
 *
 *   For a retundant raid type (RAID1/RAID5), the spare drive swaps in
 *   and and edge between VD object and newly swapped-in PVD object
 *   becomes enabled.
 * 
 * @return void
 *
 * @version
 *   4-Feb-2010 - Created. Anuj Singh
 *
 ******************************************************************************/
void amber_test_drive_insertion_and_verification(fbe_test_rg_configuration_t *  current_rg_configuration_p,
                                                 fbe_u32_t  position_to_reinsert,
                                                 fbe_object_id_t pvd_object_id)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_raid_group_disk_set_t *    drive_to_reinsert_p = NULL;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     vd_object_id = FBE_OBJECT_ID_INVALID;
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start for RAID group:%d....", __FUNCTION__, current_rg_configuration_p->raid_group_id);

    /* Get the position to remove disk set. */
    drive_to_reinsert_p = &current_rg_configuration_p->rg_disk_set[position_to_reinsert];

    /* Get the RAID group object-id and virtual drive object-id of the position to reinsert. */
    fbe_api_database_lookup_raid_group_by_number(current_rg_configuration_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_reinsert, &vd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_id);

    if(current_rg_configuration_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        /* Insert the drive for non redundant RAID group. */
        amber_reinsert_drive(current_rg_configuration_p, position_to_reinsert);

        /* Verify PVD object state */
        shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
    }
    else if((current_rg_configuration_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
            (current_rg_configuration_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5))
    {
        fbe_edge_index_t                    edge_index;
        fbe_api_get_block_edge_info_t       edge_info;

        /* Enable the job service queue which allows job service to process the
         * swap commands.
         */
        status = fbe_test_sep_util_send_job_service_command (vd_object_id,
                                                            FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

        /* get the edge index where the permanent spare gets swap-in. */
        fbe_test_sep_drive_get_permanent_spare_edge_index(vd_object_id, &edge_index);
        
        /* Verify that permanent spare gets swapped-in and edge between VD object
         * and newly swapped-in PVD object becomes enabled.
         */
        status = fbe_api_wait_for_block_edge_path_state (vd_object_id, edge_index,
                                                         FBE_PATH_STATE_ENABLED,
                                                         FBE_PACKAGE_ID_SEP_0,
                                                         70000);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

        /* Get the spare edge information. */
        edge_index = 0;
        status = fbe_api_get_block_edge_info(vd_object_id, edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
        pvd_object_id = edge_info.server_id;

        /* Reinsert removed drive.
        */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Reinsert removed drive, port %d, encl %d, drive %d",
                                            __FUNCTION__,
                                            drive_to_reinsert_p->bus,
                                            drive_to_reinsert_p->enclosure,
                                            drive_to_reinsert_p->slot);

        if (fbe_test_util_is_simulation())
        {
            status = fbe_test_pp_util_reinsert_drive(drive_to_reinsert_p->bus,
                                                    drive_to_reinsert_p->enclosure,
                                                    drive_to_reinsert_p->slot,
                                                    current_rg_configuration_p->drive_handle[position_to_reinsert]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            status = fbe_test_pp_util_reinsert_drive_hw(drive_to_reinsert_p->bus, 
                                                        drive_to_reinsert_p->enclosure, 
                                                        drive_to_reinsert_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    return;
}
/******************************************************************************
 * End amber_test_drive_insertion_and_verification()
 ******************************************************************************/
 
/*!****************************************************************************
 *  amber_test_sep_util_send_job_service_command
 ******************************************************************************
 * @brief
 *   This function disables the job service queue for a retundant raid type.
 * 
 * @return void
 *
 * @version
 *   4-Feb-2010 - Created. Anuj Singh
 *
 ******************************************************************************/
void amber_test_sep_util_send_job_service_command(fbe_object_id_t  object_id,
                                                  fbe_job_service_control_t  job_control,
                                                  fbe_raid_group_type_t  raid_group_type)
{
    if(raid_group_type == FBE_RAID_GROUP_TYPE_RAID1 || 
       raid_group_type == FBE_RAID_GROUP_TYPE_RAID5)
    {
        fbe_status_t    status = FBE_STATUS_OK;

        /* Disable the job service queue before we remove the drive, it will hold
         * the processing of job service command related to sparing.
         */
        status = fbe_test_sep_util_send_job_service_command(object_id, job_control);
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    }
    return;
}
/******************************************************************************
 * End amber_test_sep_util_send_job_service_command()
 ******************************************************************************/
 
/*!****************************************************************************
 *  amber_test
 ******************************************************************************
 *
 * @brief
 *   Run Amber test for different RAID groups, to test Swap-In of a hibernating
 *   Hot Spare drive.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  5/17/2010 - Created. Dhaval Patel
 *****************************************************************************/
void amber_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &amber_raid_group_config[0];

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Start ...", __FUNCTION__);

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, amber_run_test,
                                                   AMBER_TEST_LUNS_PER_RAID_GROUP,
                                                   AMBER_TEST_CHUNKS_PER_LUN);
    return;
} 
/******************************************************************************
 * End amber_test()
 ******************************************************************************/
 
/*!****************************************************************************
 *  amber_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the Amber test.
 *   
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void amber_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &amber_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        
        /* Creates and loads the physical and logical packages. 
         */
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* Load the logical packages. 
         */
        sep_config_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* Speed up VD hot spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************************************************
 * end amber_setup()
 ******************************************************************************/

/*!****************************************************************************
 *  amber_cleanup
 ******************************************************************************
 * @brief
 *  Cleanup the amber test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/19/2011 - Created. Sandeep Chaudhari
 *
 ******************************************************************************/

void amber_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    mut_printf(MUT_LOG_LOW, "=== %s completed ===\n", __FUNCTION__);
    return;
}
/******************************************************************************
 * end amber_cleanup()
 ******************************************************************************/

