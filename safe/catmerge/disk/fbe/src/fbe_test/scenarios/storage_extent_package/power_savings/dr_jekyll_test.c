/*!*************************************************************************
 * @file dr_jekyll_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for Creating Raid Groups on hibernating PVDS
 *
 * @version
 *   05/05/2010 - Created. Jesus medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_utils.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_test_common_utils.h"  
#include "fbe/fbe_api_scheduler_interface.h"
#include "sep_hook.h"


char * dr_jekyll_short_desc = "API Raid Group creation on hibernating PVDs test";
char * dr_jekyll_long_desc =
"\n"
"\n"
"The Dr Jekyll Scenario tests the infrastructure needed for Raid Group\n"
"creation on Hybernating PVDs.\n\n"
"This scenario tests the creation of Raid Group using PVDs that are hibernating\n"
"\n"
"Starting Config: \n"
"\t[PP] an armada board\n"
"\t[PP] a SAS PMC port\n"
"\t[PP] a viper enclosure\n"
"\t[PP] four SAS drives (PDO-A, PDO-B, PDO-C and PDO-D)\n"
"\t[PP] four logical drives (LDO-A, LDO-B, LDO-C and LDO-D)\n"
"\t[SEP] four provision drives (PVD-A, PVD-B, PVD-C and PVD-D)\n"
"\t[SEP] four virtual drives (VD-A, VD-B, VD-C and VD-D)\n"
"\t[SEP] a RAID-5 object, connected to the four VDs\n"
"\n"
"Test 1: Create RG with Drives in Hibernation state\n"
"\tSTEP 1: Bring up the initial topology.\n"
"\t\t- create and verify the initial physical package config.\n"
"\t\t- create four provision drives with I/O edges to the logical drives.\n"
"\t\t- verify that provision drives are in the READY state.\n"
"\tSTEP 2: call SEP API to hibernate PVDs.\n"
"\t\t- set all PVDs to hibernate\n"
"\t\t- wait until al PVDs are hibernating\n"
"\tSTEP 3: Create RG on hibernating PVDs.\n"
"\t\t- use job service to create RG 5 type\n"
"\t\t- verify RG has been successfully created\n"
"\tSTEP 4: destroy raid group\n"
"\t\t- call jb service to destroy raid group\n"
"\t\t- verify that rg object has been destroyed\n"
"\n"
"Test 2: Create RG with Drives in Activate State \n"
"\tSTEP 1: Bring up the initial topology.\n"
"\t\t- create and verify the initial physical package config.\n"
"\t\t- create four provision drives with I/O edges to the logical drives.\n"
"\t\t- verify that provision drives are in the READY state.\n"
"\tSTEP 2: call SEP API to hibernate PVDs.\n"
"\t\t- set all PVDs to hibernate\n"
"\t\t- wait until al PVDs are hibernating\n"
"\tSTEP 3: Add Hook to when drives are out of Hibernation and in Activate state\n"
"\t\t- Create RG on Activate PVDs.\n"
"\t\t- use job service to create RG 5 type\n"
"\t\t- delete hook and let PVDs come to RDY.\n"
"\t\t- verify RG has been successfully created\n"
"\tSTEP 4: destroy raid group\n"
"\t\t- call jb service to destroy raid group\n"
"\t\t- verify that rg object has been destroyed\n"
"\n"
"\n"
"Description last updated: 10/28/2011.\n";

/*****************************************
 * GLOBAL DATA
 *****************************************/

/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a function
 *         register
 */
#define DR_JEKYLL_TIMEOUT  15000 /*wait maximum of 15 seconds*/
#define DR_JEKYLL_PVD_SIZE TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY
#define DR_JEKYLL_RAID_GROUP_WIDTH      4

/*!*******************************************************************
 * @def DR_JEKYLL_NUMBER_OF_DRIVES_SIM
 *********************************************************************
 * @brief number of drives we configure in the simulator.
 *
 *********************************************************************/
#define DR_JEKYLL_NUMBER_OF_DRIVES_SIM 10

enum dr_jekyll_sep_object_ids_e {
    DR_JEKYLL_RAID_ID = 21,
    DR_JEKYLL_MAX_ID = 22
};

/* Number of physical objects in the test configuration. 
*/
fbe_u32_t   dr_jekyll_test_number_physical_objects_g;

fbe_test_rg_configuration_t dr_jekyll_raid_group_config[] = 
{
    /*             width,                              capacity ,                     raid type,                             class,                 block size            RAID-id,              bandwidth.*/
    {DR_JEKYLL_RAID_GROUP_WIDTH, FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,   DR_JEKYLL_RAID_ID,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


fbe_test_raid_group_disk_set_t dr_jekyllDiskSet[5] = { 
    {0, 0 , 4},
    {0, 0 , 5},
    {0, 0 , 6},
    {0, 0 , 7},
    {0, 0 , 8}
};


static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_lifecycle_state_t    expected_state = FBE_LIFECYCLE_STATE_INVALID;
static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;
static void dr_jekyll_response_callback_function (update_object_msg_t * update_object_msg, void *context);

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
void dr_jekyll_create_pvds(fbe_object_id_t object_id,
                           fbe_lba_t capacity,
                           fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size,
                           fbe_u32_t port,
                           fbe_u32_t enclosure,
                           fbe_u32_t slot);

fbe_status_t fbe_dr_jekyll_build_control_element(
        fbe_job_queue_element_t        *job_queue_element,
        fbe_job_service_control_t      control_code,
        fbe_object_id_t                object_id,
        fbe_u8_t                       *command_data_p,
        fbe_u32_t                      command_size);


static void dr_jekyll_set_trace_level(fbe_trace_type_t trace_type,
        fbe_u32_t id,
        fbe_trace_level_t level);

static void dr_jekyll_create_raid_group(fbe_object_id_t raid_group_id,
                                        fbe_lba_t raid_group_capacity,
                                        fbe_u32_t drive_count, 
                                        fbe_raid_group_type_t raid_group_type);

static void dr_jekyll_remove_raid_group(fbe_raid_group_number_t rg_id);

/* dr_jekyll step functions */

/* enable system wide power saving */
static void step1(void);

/* wait for all PVDS to hibernate */
static void step2(void);

/* Create RG on hibernating PVDs */
static void step3(void);

/* destroy raid group */
static void step4(fbe_u32_t rg_count_before_create);

/* add hook and disable hibernation */
static void step5(void);

/* Create RG with PVDs in Activate state */
static void step6(void);

static void dr_jekyll_create_raid_group_with_pvds_in_activate_state(fbe_object_id_t raid_group_id,
                                     fbe_lba_t raid_group_capacity,
                                     fbe_u32_t drive_count, 
                                     fbe_raid_group_type_t raid_group_type);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!**************************************************************
 * dr_jekyll_test()
 ****************************************************************
 * @brief
 * main entry point for all steps involved in the dr_jekyll
 * test: API Raid Group creation on hibernating PVDs
 * 
 * @param  - none
 *
 * @return - none
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void dr_jekyll_test(void)
{
    fbe_status_t                   status;
    fbe_u32_t                      disk = 0;
    fbe_u32_t                      rg_count_before_create;
    fbe_lifecycle_state_t		   object_state; 
    fbe_object_id_t                object_id;
    fbe_test_discovered_drive_locations_t       drive_locations;   // available drive location details
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_count_before_create = 0;
	
    mut_printf(MUT_LOG_LOW, "=== dr_jekyll Test starts ===\n");

	
    /* fill out power save capable disk set at run time from available disk pool */
    fbe_test_sep_util_discover_all_power_save_capable_drive_information(&drive_locations);
	
    /*fill up the drive information to raid group configuration*/
    fbe_test_sep_util_setup_rg_config(dr_jekyll_raid_group_config, &drive_locations);
  
    rg_config_p = &dr_jekyll_raid_group_config[0];

    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, 
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_before_create);

    fbe_semaphore_init(&sem, 0, 1);

    if (fbe_test_util_is_simulation())
    {
       /* Wait for this number of PVD objects to exist.
            */
        status = fbe_api_wait_for_number_of_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                                  DR_JEKYLL_NUMBER_OF_DRIVES_SIM, 
                                                  120000, 
                                                  FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    fbe_test_wait_for_objects_of_class_state(FBE_CLASS_ID_PROVISION_DRIVE, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_LIFECYCLE_STATE_READY,
                                             120000);

    mut_printf(MUT_LOG_LOW, "=== Waiting until all drives reach READY state ===\n");

    for (disk = 0; disk < DR_JEKYLL_RAID_GROUP_WIDTH; disk++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* verify PVD objects get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, 20000,
                                                         FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s: object id %d, now at READY state %d \n", 
                __FUNCTION__,
                object_id,
                object_state);

        /* Disable background zeroing so that drives go into power saving mode*/
        status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_LOW, "=== All drives at READY state ===\n");

    /* -------  Test 1: Create RG with Drives in Hibernation state  ------------*/
    /* - Enable System Wide power saving ---------------------------------------*/
    /* - Wait for all PVDs in Hibernating mode ---------------------------------*/
    /* - Create RG on hibernating PVDs -----------------------------------------*/
    /* - Destroy RG ------------------------------------------------------------*/
    /* -------------------------------------------------------------------------*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test 1 started  ===\n",__FUNCTION__);

    /* enable system wide power saving */
    step1();

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  dr_jekyll_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    /* wait for all PVDS to hibernating */
    step2();
    
    status = fbe_api_notification_interface_unregister_notification(dr_jekyll_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    fbe_api_sleep(4000);

    /* Create RG on hibernating PVDs */
    step3();
    
    /* destroy raid group */
    step4(rg_count_before_create);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test 1 completed successfully ===\n",__FUNCTION__);

    /* ---------- Test 2: Create RG with Drives in Activate State --------------*/
    /* - Enable System Wide power saving ---------------------------------------*/
    /* - Wait for all PVDs in Hibernating mode ---------------------------------*/
    /* - Set debug hook before drives disable hibernation on the drives so that-*/
    /* ---- the drives stay in Activate during RG creation ---------------------*/
    /* - Create RG on hibernating PVDs -----------------------------------------*/
    /* - Delete the Hook -------------------------------------------------------*/
    /* - Destroy RG ------------------------------------------------------------*/
    /* -------------------------------------------------------------------------*/
    /* fill out power save capable disk set at run time from available disk pool */
    fbe_test_sep_util_discover_all_power_save_capable_drive_information(&drive_locations);

    /*fill up the drive information to raid group configuration*/
    fbe_test_sep_util_setup_rg_config(dr_jekyll_raid_group_config, &drive_locations);

    rg_config_p = &dr_jekyll_raid_group_config[0];

    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, 
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_before_create);

    fbe_semaphore_init(&sem, 0, 1);

    if (fbe_test_util_is_simulation())
    {
       /* Wait for this number of PVD objects to exist.
            */
        status = fbe_api_wait_for_number_of_class(FBE_CLASS_ID_PROVISION_DRIVE, 
                                                  DR_JEKYLL_NUMBER_OF_DRIVES_SIM, 
                                                  120000, 
                                                  FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    fbe_test_wait_for_objects_of_class_state(FBE_CLASS_ID_PROVISION_DRIVE, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_LIFECYCLE_STATE_READY,
                                             120000);

    mut_printf(MUT_LOG_LOW, "=== Waiting until all drives reach READY state ===\n");

    for (disk = 0; disk < DR_JEKYLL_RAID_GROUP_WIDTH; disk++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* verify PVD objects get to ready state in reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, 20000,
                                                         FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_get_object_lifecycle_state(object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s: object id %d, now at READY state %d \n", 
                __FUNCTION__,
                object_id,
                object_state);

        /* Disable background zeroing so that drives go into power saving mode*/
        status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_LOW, "=== All drives at READY state ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test 2 started ===\n",__FUNCTION__);

    /* enable system wide power saving */
    step1();

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  dr_jekyll_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    /* wait for all PVDS to hibernating */
    step2();
    
    status = fbe_api_notification_interface_unregister_notification(dr_jekyll_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    fbe_api_sleep(4000);

    /* Create RG on hibernating PVDs */
    step5();
    
    /* destroy raid group */
    step4(rg_count_before_create);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test 2 completed successfully ===\n",__FUNCTION__);

    return;
}

/*!**************************************************************
 * step1()
 ****************************************************************
 * @brief
 *  This function enables power saving system wide
 *
 * @param void
 *
 * @return void
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 *
 ****************************************************************/

void step1(void)
{
    fbe_object_id_t                object_id;
    fbe_system_power_saving_info_t power_save_info;
    fbe_status_t                   status;
    fbe_u32_t                      disk = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &dr_jekyll_raid_group_config[0];
    power_save_info.enabled = FBE_TRUE;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 1: enable system wide power savings, starts ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Enabling system wide power saving ===\n", __FUNCTION__);
   
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);

    /* set PVD idle time to 20 to accelerate test */
    for (disk = 0; disk < DR_JEKYLL_NUMBER_OF_DRIVES_SIM-4 ; disk++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                         FBE_LIFECYCLE_STATE_READY, 
                                                         20000,
                                                         FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 1: enable system wide power savings, end ===\n");
 }

/*!**************************************************************
 * step2()
 ****************************************************************
 * @brief
 *  This function waits until all unbound PVDs go to hibernate
 *
 * @param void
 *
 * @return void
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 *
 ****************************************************************/

void step2(void)
{
    fbe_object_id_t     object_id;
    DWORD               dwWaitResult;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           i;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &dr_jekyll_raid_group_config[0];

    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 2: wait for all PVDs to hibernate, starts ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting for unbound PVD's to be in hibernate in ~2.3 minute ===\n");
#if 0 /* SAFEBUG - already initialized */
    fbe_semaphore_init(&sem, 0, 1);
#endif
 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    i = 4;
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    expected_object_id = object_id; /*we want to get notification from this drive*/
    expected_state = FBE_LIFECYCLE_STATE_HIBERNATE;
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait about 1.5 minutes more from the timer we enabled hibernation 
     * for all so unbound PVDs will get to hibernation in 2 minutes
     */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 230000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    for (i = 0; i < 4; i++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[i].bus, rg_config_p->rg_disk_set[i].enclosure, rg_config_p->rg_disk_set[i].slot, &object_id);

        /* verify the PVD object is set to hibernate in a reasonable time */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                         FBE_LIFECYCLE_STATE_HIBERNATE, 
                                                         20000,
                                                         FBE_PACKAGE_ID_SEP_0);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_TEST_STATUS, " === Verified unbound PVDs are hibernating in around ~2.3 minutes ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 2: wait for al PVDs to hibernate, end ===\n");
 }

/*!**************************************************************
 * step3()
 ****************************************************************
 * @brief
 * This function creates a RG on hibernating PVDS
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void step3(void)
{
    fbe_u32_t        rg_count_before_create, rg_count_after_create;
    fbe_status_t     status;

    mut_printf(MUT_LOG_LOW, "=== Dr Jekyll step 3: creating RG on hibernating PVDs, starts ===\n");
    dr_jekyll_set_trace_level (FBE_TRACE_TYPE_DEFAULT,
                                  FBE_OBJECT_ID_INVALID,
                                  FBE_TRACE_LEVEL_DEBUG_LOW);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, 
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_before_create);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Raid Group Type 5... ===\n");

    /* Call Job Service API to create Raid Group type 5 object. */
    dr_jekyll_create_raid_group(DR_JEKYLL_RAID_ID,
                                FBE_LBA_INVALID,
                                DR_JEKYLL_RAID_GROUP_WIDTH, 
                                FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, 
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+1, 
            "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully! ===\n");
    mut_printf(MUT_LOG_LOW, "=== Dr Jekyll step 3: creating RG on hibernating PVDs, ends ===\n");
 }

/*!**************************************************************
 * step4()
 ****************************************************************
 * @brief
 * This function destroys a Raid group
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void step4(fbe_u32_t rg_count_before_create)
{
    fbe_u32_t        rg_count_after_delete;
    fbe_status_t     status;

    mut_printf(MUT_LOG_LOW, "=== Dr Jekyll step 4: Destroy a raid group, starts ===\n");
    dr_jekyll_set_trace_level (FBE_TRACE_TYPE_DEFAULT,
                                  FBE_OBJECT_ID_INVALID,
                                  FBE_TRACE_LEVEL_DEBUG_LOW);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Raid Group Type 5... ===\n");
 
    /* Call Job Service API to destroy Raid Group object. */
    dr_jekyll_remove_raid_group(DR_JEKYLL_RAID_ID); 
  
    /* Verify that the Raid components have been destroyed */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY,
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_after_delete);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to enumerate raid group object");
    MUT_ASSERT_INT_EQUAL(rg_count_before_create, rg_count_after_delete);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 deleted successfully! ===\n");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 4: Destroy a raid group, starts ===\n");
 
}

/*!**************************************************************
 * step5()
 ****************************************************************
 * @brief
 *  This function adds debug hook and disables hibernation to cause
 *  the PVDs remain in Activate state before creating RG.
 *
 * @param void
 *
 * @return void
 *
 * @author
 *
 ****************************************************************/

void step5(void)
{
    fbe_object_id_t                object_id;
    fbe_status_t                   status;
    fbe_u32_t                      disk = 0;
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_scheduler_debug_hook_t     hook;
    fbe_lifecycle_state_t		   object_lifecycle_state;

    rg_config_p = &dr_jekyll_raid_group_config[0];
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 5: Add debug hook and disable hibernation ===\n");
   
    /* set PVD idle time to 20 to accelerate test */
    for (disk = 0; disk < DR_JEKYLL_RAID_GROUP_WIDTH; disk++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, 
                                                                rg_config_p->rg_disk_set[disk].enclosure, 
                                                                rg_config_p->rg_disk_set[disk].slot, 
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Adding Debug Hook: PVD Obj: 0x%x ===", object_id);

        /* Adding hook */
        status = fbe_api_scheduler_add_debug_hook(object_id,
                                SCHEDULER_MONITOR_STATE_BASE_CONFIG_OUT_OF_HIBERNATE,
                                FBE_BASE_CONFIG_SUBSTATE_OUT_OF_HIBERNATION,
                                0,
                                0,
                                SCHEDULER_CHECK_STATES,
                                SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Disable Power Saving to cause it to come out and in Activtate state */
        status = fbe_api_disable_object_power_save(object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get Hook info */
        status = fbe_test_get_debug_hook(object_id,
                                         SCHEDULER_MONITOR_STATE_BASE_CONFIG_OUT_OF_HIBERNATE,
                                         FBE_BASE_CONFIG_SUBSTATE_OUT_OF_HIBERNATION, 
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                         0,0, &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "=== BC hook was hit %llu times ===",
               (unsigned long long)hook.counter);


        status = fbe_api_get_object_lifecycle_state(object_id, &object_lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "=== ....PVD: 0x%X, now at state: %d ===\n", 
                object_id,
                object_lifecycle_state);
    }

    /* Create RG with PVDs in Activate */
    step6();

    mut_printf(MUT_LOG_TEST_STATUS, "=== Dr Jekyll step 5: Add debug hook and disable hibernation ends ===\n");
 }

/*!**************************************************************
 * step6()
 ****************************************************************
 * @brief
 * This function creates a RG on the drives that just come out of 
 * hibernation.
 * 
 * @param - none 
 *
 * @return - none
 *
 * @author
 * 
 ****************************************************************/

void step6(void)
{
    fbe_u32_t        rg_count_before_create, rg_count_after_create;
    fbe_status_t     status;

    mut_printf(MUT_LOG_LOW, "=== Dr Jekyll step 6: creating RG with PVDs in Activate State, starts ===\n");
    dr_jekyll_set_trace_level (FBE_TRACE_TYPE_DEFAULT,
                                  FBE_OBJECT_ID_INVALID,
                                  FBE_TRACE_LEVEL_DEBUG_LOW);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* get the number of RGss in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, 
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_before_create);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating Raid Group Type 5... ===\n");

    /* Call Job Service API to create Raid Group type 5 object. */
    dr_jekyll_create_raid_group_with_pvds_in_activate_state(DR_JEKYLL_RAID_ID,
                                FBE_LBA_INVALID,
                                DR_JEKYLL_RAID_GROUP_WIDTH, 
                                FBE_RAID_GROUP_TYPE_RAID5);

    /* get the number of RGs in the system and compare with before create */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PARITY, 
                                                FBE_PACKAGE_ID_SEP_0,  
                                                &rg_count_after_create);

    MUT_ASSERT_INT_EQUAL_MSG(rg_count_after_create, rg_count_before_create+1, 
            "Found unexpected number of RGs!");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group Type 5 created successfully! ===\n");
    mut_printf(MUT_LOG_LOW, "=== Dr Jekyll step 6: creating RG on hibernating PVDs, ends ===\n");


}

/*!**************************************************************
 * dr_jekyll_create_raid_group()
 ****************************************************************
 * @brief
 *  This function builds and sends a Raid group creation request
 *  to the Job service
 *
 * @param raid_group_id - id of raid group
 * @param raid_group_capacity - capacity of raid group
 * @param drive_count - number of drives in raid group
 * @param raid_group_type - type of raid group
 *
 * @return void
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 *
 ****************************************************************/

void dr_jekyll_create_raid_group(fbe_object_id_t raid_group_id,
                                     fbe_lba_t raid_group_capacity,
                                     fbe_u32_t drive_count, 
                                     fbe_raid_group_type_t raid_group_type)
{
    fbe_status_t status;
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request;
    fbe_u32_t                                         index = 0;
    fbe_object_id_t                                   raid_group_id1 = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                                   raid_group_id1_from_job = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &dr_jekyll_raid_group_config[0];

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__, raid_group_type);
    
    fbe_zero_memory(&fbe_raid_group_create_request,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_raid_group_create_request.drive_count = drive_count;
    fbe_raid_group_create_request.raid_group_id = raid_group_id;

    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        fbe_raid_group_create_request.disk_array[index].bus       = rg_config_p->rg_disk_set[index].bus;
        fbe_raid_group_create_request.disk_array[index].enclosure = rg_config_p->rg_disk_set[index].enclosure;
        fbe_raid_group_create_request.disk_array[index].slot      = rg_config_p->rg_disk_set[index].slot;
    }

    /* b_bandwidth = true indicates 
     * FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 1024 for element_size 
     */
    fbe_raid_group_create_request.b_bandwidth           = FBE_TRUE; 
    fbe_raid_group_create_request.capacity              = raid_group_capacity; 
    fbe_raid_group_create_request.raid_type             = raid_group_type;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    //fbe_raid_group_create_request.explicit_removal      = 0;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = DR_JEKYLL_TIMEOUT;

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");

    /* wait for notification */
   status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
                                         DR_JEKYLL_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &raid_group_id1_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");


    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Call to create Raid Group type %d object returned error %d\n",
            __FUNCTION__,
            raid_group_type, job_error_code);
    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, 
            &raid_group_id1);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(raid_group_id1_from_job, raid_group_id1);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id1);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id1, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: test_object_id %d\n",
            __FUNCTION__,
            raid_group_id1);
 
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__,
            raid_group_type);
}

/*!**************************************************************
 * dr_jekyll_create_raid_group_with_pvds_in_activate_state()
 ****************************************************************
 * @brief
 *  This function builds and sends a Raid group creation request
 *  to the Job service
 *
 * @param raid_group_id - id of raid group
 * @param raid_group_capacity - capacity of raid group
 * @param drive_count - number of drives in raid group
 * @param raid_group_type - type of raid group
 *
 * @return void
 *
 * @author
 *
 ****************************************************************/

void dr_jekyll_create_raid_group_with_pvds_in_activate_state(fbe_object_id_t raid_group_id,
                                     fbe_lba_t raid_group_capacity,
                                     fbe_u32_t drive_count, 
                                     fbe_raid_group_type_t raid_group_type)
{
    fbe_api_job_service_raid_group_create_t           fbe_raid_group_create_request;
    fbe_u32_t                                         index = 0;
    fbe_object_id_t                                   raid_group_id1 = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t                      job_error_code;
    fbe_status_t                                      job_status;
    fbe_test_rg_configuration_t                       *rg_config_p = NULL;
    fbe_object_id_t                                   pvd_obj = FBE_OBJECT_ID_INVALID;
    fbe_base_config_object_power_save_info_t          object_ps_info;
    fbe_lifecycle_state_t		                      object_lifecycle_state;
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;


    rg_config_p = &dr_jekyll_raid_group_config[0];

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__, raid_group_type);

    fbe_zero_memory(&fbe_raid_group_create_request,sizeof(fbe_api_job_service_raid_group_create_t));

    fbe_raid_group_create_request.drive_count = drive_count;
    fbe_raid_group_create_request.raid_group_id = raid_group_id;

    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        fbe_raid_group_create_request.disk_array[index].bus       = rg_config_p->rg_disk_set[index].bus;
        fbe_raid_group_create_request.disk_array[index].enclosure = rg_config_p->rg_disk_set[index].enclosure;
        fbe_raid_group_create_request.disk_array[index].slot      = rg_config_p->rg_disk_set[index].slot;
    }


    /* b_bandwidth = true indicates 
     * FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 1024 for element_size 
     */
    fbe_raid_group_create_request.b_bandwidth           = FBE_TRUE; 
    fbe_raid_group_create_request.capacity              = raid_group_capacity; 
    fbe_raid_group_create_request.raid_type             = raid_group_type;
    fbe_raid_group_create_request.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_request.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    //fbe_raid_group_create_request.explicit_removal      = 0;
    fbe_raid_group_create_request.is_private            = 0;
    fbe_raid_group_create_request.wait_ready            = FBE_FALSE;
    fbe_raid_group_create_request.ready_timeout_msec    = DR_JEKYLL_TIMEOUT;

    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus, 
                                                                rg_config_p->rg_disk_set[index].enclosure, 
                                                                rg_config_p->rg_disk_set[index].slot, 
                                                                &pvd_obj);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* get the PVD power save info */
        status = fbe_api_get_object_power_save_info(pvd_obj, &object_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        switch (object_ps_info.power_save_state) {
            case FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE:
                mut_printf(MUT_LOG_LOW, "=== Object state is FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE ===\n"); 
                break;
            case FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE:
                mut_printf(MUT_LOG_LOW, "=== Object state is FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE  ===\n"); 
                break;
            case FBE_POWER_SAVE_STATE_NOT_SAVING_POWER:
                mut_printf(MUT_LOG_LOW, "=== Object state is FBE_POWER_SAVE_STATE_NOT_SAVING_POWER  ===\n"); 
                break;
            case FBE_POWER_SAVE_STATE_SAVING_POWER:
                mut_printf(MUT_LOG_LOW, "=== Object state is FBE_POWER_SAVE_STATE_SAVING_POWER ===\n"); 
                break;
            default:
                mut_printf(MUT_LOG_LOW, "=== Object state is FBE_POWER_SAVE_STATE_INVALID ===\n"); 
                break;

        }
        status = fbe_api_get_object_lifecycle_state(pvd_obj, &object_lifecycle_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "=== ...PVD: 0x%x state %d ===\n", 
                pvd_obj,
                object_lifecycle_state);
        status = fbe_test_get_debug_hook(pvd_obj,
                                         SCHEDULER_MONITOR_STATE_BASE_CONFIG_OUT_OF_HIBERNATE,
                                         FBE_BASE_CONFIG_SUBSTATE_OUT_OF_HIBERNATION, 
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                         0,0, &hook);
         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
         mut_printf(MUT_LOG_TEST_STATUS, "Base Config hook was hit %llu times",
                (unsigned long long)hook.counter);


    }

    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, 
            status, "fbe_api_job_service_raid_group_create failed");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group create call sent successfully! ===\n");


    /* fill in b,e,s values */
    for (index = 0;  index < fbe_raid_group_create_request.drive_count; index++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus, 
                                                                rg_config_p->rg_disk_set[index].enclosure, 
                                                                rg_config_p->rg_disk_set[index].slot, 
                                                                &pvd_obj);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Deleting Debug Hook: PVD obj id 0x%X ===", pvd_obj);

        status = fbe_api_scheduler_del_debug_hook(pvd_obj,
                                SCHEDULER_MONITOR_STATE_BASE_CONFIG_OUT_OF_HIBERNATE,
                                FBE_BASE_CONFIG_SUBSTATE_OUT_OF_HIBERNATION,
                                0,
                                0,
                                SCHEDULER_CHECK_STATES,
                                SCHEDULER_DEBUG_ACTION_PAUSE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }


    /* wait for notification */
    status = fbe_api_common_wait_for_job(fbe_raid_group_create_request.job_number,
                                         DR_JEKYLL_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to create RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");


    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Call to create Raid Group type %d object returned error %d\n",
            __FUNCTION__,
            raid_group_type, job_error_code);
    /* find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_create_request.raid_group_id, 
            &raid_group_id1);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id1);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(raid_group_id1, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: test_object_id %d\n",
            __FUNCTION__,
            raid_group_id1);

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: end -  call Job Service API to create Raid Group type %d object\n",
            __FUNCTION__,
            raid_group_type);
}

/*!**************************************************************
 * dr_jekyll_remove_raid_group()
 ****************************************************************
 * @brief
 * This function removes a raid group
 *
 * @param - rg_id id of raid group to remove 
 *
 * @return - none
 *
 * @author
 * 05/05/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

void dr_jekyll_remove_raid_group(fbe_raid_group_number_t rg_id)
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_destroy_t fbe_raid_group_destroy_request;
    fbe_job_service_error_type_t                    job_error_code;
    fbe_status_t                                    job_status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: start - call Job Service API to destroy Raid Group object, START\n",
            __FUNCTION__);

    fbe_raid_group_destroy_request.raid_group_id = rg_id;
    fbe_raid_group_destroy_request.wait_destroy = FBE_FALSE;
    fbe_raid_group_destroy_request.destroy_timeout_msec = DR_JEKYLL_TIMEOUT;
    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to destroy raid group");
    mut_printf(MUT_LOG_TEST_STATUS, "=== Raid Group destroy call sent successfully! ===\n");

     /* wait for notification */
   status = fbe_api_common_wait_for_job(fbe_raid_group_destroy_request.job_number,
                                         DR_JEKYLL_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG destruction failed");

    mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: call Job Service API to destroy Raid Group object, END\n",
            __FUNCTION__);
    return;
}

/*!**************************************************************
 * dr_jekyll_request_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  dr_jekyll request
 * 
 * @param packet_p - packet
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 * 
 ****************************************************************/

static fbe_status_t dr_jekyll_request_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end dr_jekyll_request_completion_function()
 ********************************************************/

/*!**************************************************************
 * dr_jekyll_test_completion_function()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the 
 *  dr_jekyll request sent to the Job Service
 * 
 * @param packet_p - packet
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/28/2009 - Created. Jesus Medina
 * 
 ****************************************************************/

static fbe_status_t dr_jekyll_test_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t in_context)
{
    fbe_semaphore_t* sem = (fbe_semaphore_t * )in_context;
    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end dr_jekyll_test_completion_function()
 ********************************************************/

/*********************************************************************
 * dr_jekyll_set_trace_level()
 *********************************************************************
 * @brief
 * sets the trace level for this test
 * 
 * @param - id object to set trace level for
 * @param - trace level 
 *
 * @return fbe_status_t
 *
 * @author
 * 01/13/2010 - Created. Jesus Medina
 *    
 *********************************************************************/
static void dr_jekyll_set_trace_level(fbe_trace_type_t trace_type,
        fbe_u32_t id,
        fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/***************************************
 * end dr_jekyll_set_trace_level()
 **************************************/

static void dr_jekyll_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
	fbe_lifecycle_state_t	state;

	fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);

    if (update_object_msg->object_id == expected_object_id &&
            expected_state == state)
    {
        fbe_semaphore_release(sem, 0, 1 ,FALSE);
    }
}

/*********************************************
 * end fbe_dr_jekyll_test file
 **********************************************/
