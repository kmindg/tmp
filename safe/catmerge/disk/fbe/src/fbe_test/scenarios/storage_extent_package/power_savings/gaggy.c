/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file gaggy.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test for power saving on two RGs
 *
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
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_test_common_utils.h" 
#include "fbe/fbe_random.h"

//  Configuration parameters
#define GAGGY_TEST_RAID_GROUP_WIDTH 3


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   TEST DESCRIPTION
 *************************/
char * gaggy_short_desc = "verify power saving logic on 2 RG";
char * gaggy_long_desc =
"This covers the use case where there are multiple system raid groups and a user raid group with no shared disks.\n"
"  We will assume we have two raid groups, raid group 0 and raid group 1.  \n"
"We will assume a single drive raid group for to simplify the example.  Raid group 0 is a system raid group that does not support hibernate."
"*	Hibernate would be disabled for the lun on raid group 0, so this lun would never attempt to hibernate.\n"
"*	lun 1 could hibernate however.\n"
"*	When lun 1 hibernates, it will also cause the raid group 1 to hibernate.\n"
"*	While in the hibernate state, RG 1 will send a hibernate usurper command to VD0 through VD4.\n"
"*	VD0 through VD 2 have two upstream edges.\n"
"*	When VD0 through VD2 receive a hibernate usurper command, they will mark the edge from RG 1 with the path state of slumber and will set the hibernate condition.\n"
"*	The hibernate condition will see that not all upstream edges are marked with the path state of slumber, and will not attempt to transition to hibernate.\n"
"*	Thus since RG 0 will never try to hibernate, then VD 0-2 will never hibernate.\n"
"*	Last Update:11/10/11.\n"
;

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_lifecycle_state_t    expected_state = FBE_LIFECYCLE_STATE_INVALID;
static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;

/*!*******************************************************************
 * @var gaggy_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t gaggy_raid_group_config[] = 
{
    /* width,                          capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {GAGGY_TEST_RAID_GROUP_WIDTH,       0xffff,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
    {GAGGY_TEST_RAID_GROUP_WIDTH,		0xffff,	   FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520, 		   1,		   0},    
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


static void gaggy_test_create_lun(fbe_lun_number_t lun_number, fbe_u32_t rg_id);
static void gaggy_test_destroy_lun(fbe_lun_number_t lun_number);
static void gaggy_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);
static void gaggy_test_run_logic_test(void);
static void gaggy_test_create_rg(fbe_u32_t id, fbe_u32_t start_drive);
static void gaggy_destroy_rg(fbe_u32_t id);
static void gaggy_load_physical_config(void);
static void gaggy_test_fbe_api_destroy_rg(fbe_u32_t in_rgID);

/**********************************************************************************/

void gaggy_test_init(void)
{
    mut_printf(MUT_LOG_LOW, "=== gaggy setup starts ===\n");
    /* Only load config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        /* Load sep and neit packages */
	    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	    gaggy_load_physical_config();
	    sep_config_load_sep_and_neit();
    }

    mut_printf(MUT_LOG_LOW, "=== gaggy setup completes ===\n");    
    return;
}

void gaggy_test_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== gaggy destroy starts ===\n");	
    /* Only destroy conig in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "=== gaggy destroy configuration ===\n");
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    mut_printf(MUT_LOG_LOW, "=== gaggy destroy completes ===\n");
    return;
}

void gaggy_test(void)
{
    fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &gaggy_raid_group_config[0];

    /*init the raid group configuration*/
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    /*find all power save capable drive information from disk*/
    fbe_test_sep_util_discover_all_power_save_capable_drive_information(&drive_locations);

    /*fill up the drive information to raid group configuration*/
    fbe_test_sep_util_setup_rg_config(rg_config_p, &drive_locations);

    gaggy_test_create_rg(0, 0);
    gaggy_test_create_rg(1, 1);
    
    gaggy_test_create_lun(123, 0);/*hook up to RG 0*/
    gaggy_test_create_lun(124, 1);/*hook up to RG 1*/

    fbe_semaphore_init(&sem, 0, 1);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  gaggy_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    gaggy_test_run_logic_test();

    status = fbe_api_notification_interface_unregister_notification(gaggy_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

    gaggy_test_destroy_lun(123);
    gaggy_test_destroy_lun(124);
    
    /* gaggy_destroy_rg destroys the RG via fbe_api_job_serivce_destroy_rg call. */
    gaggy_destroy_rg(0);

    /* gaggy_test_fbe_api_destroy_rg destroys the RG via fbe_api_destroy_rg call. */
    gaggy_test_fbe_api_destroy_rg(1);
}

static void gaggy_test_create_lun(fbe_lun_number_t lun_number, fbe_u32_t rg_id)
{
    fbe_status_t                    status;
    fbe_api_lun_create_t        	fbe_lun_create_req;
    fbe_object_id_t					lu_id;
    fbe_job_service_error_type_t    job_error_type;
   
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = rg_id;
    fbe_lun_create_req.lun_number = lun_number;
    fbe_lun_create_req.capacity = 0x1000;
    fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;
    

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, 100000, &lu_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);	

    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully! \n", __FUNCTION__, lu_id);

    return;

} 

static void gaggy_test_destroy_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t       			fbe_lun_destroy_req;
    fbe_job_service_error_type_t            job_error_type;
    
    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 100000, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully! \n");

    return;
}

static void gaggy_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t *sem_p = (fbe_semaphore_t *)context;
	fbe_lifecycle_state_t	state;

	fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);
    
    if (update_object_msg->object_id == expected_object_id && expected_state == state) {
        fbe_semaphore_release(sem_p, 0, 1 ,FALSE);
    }
}


static void gaggy_test_run_logic_test(void)
{
    fbe_object_id_t                             lu1_object_id, rg1_object_id, rg2_object_id, lu2_object_id, object_id;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t								status;
    fbe_lun_set_power_saving_policy_t           lun_ps_policy;
    fbe_u32_t									disk;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &gaggy_raid_group_config[0];

    /*
    The following section will prepare the system to power save on the two RG/LU
    */
    lun_ps_policy.lun_delay_before_hibernate_in_sec = 5 ;/*this is for 5 seconds delay before hibernation*/
    lun_ps_policy.max_latency_allowed_in_100ns = (fbe_time_t)((fbe_time_t)60000 * (fbe_time_t)10000000);/*we allow 1000 minutes for the drives to wake up (default for drives is 100 minutes)*/
    
    /*and enable the power saving on the lun*/
    mut_printf(MUT_LOG_TEST_STATUS, "=== Enabling power saving on the RAID/LU/Drives ===\n");
    status = fbe_api_database_lookup_lun_by_number(123, &lu1_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(0, &rg1_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_lun_by_number(124, &lu2_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(1, &rg2_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*make sure it's not saving power */
    status = fbe_api_get_object_power_save_info(lu1_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_get_object_power_save_info(rg1_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    
    status = fbe_api_lun_set_power_saving_policy(lu1_object_id, &lun_ps_policy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_set_raid_group_power_save_idle_time(rg1_object_id, 20);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_enable_raid_group_power_save(rg1_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	for (disk = 0; disk < GAGGY_TEST_RAID_GROUP_WIDTH; disk++) 
	{
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Disable background zeroing so that drives go into power saving mode*/
        status = fbe_test_sep_util_provision_drive_disable_zeroing( object_id );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg1_object_id, disk, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    status = fbe_api_get_object_power_save_info(lu2_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);

    status = fbe_api_get_object_power_save_info(rg2_object_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    
    status = fbe_api_lun_set_power_saving_policy(lu2_object_id, &lun_ps_policy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_set_raid_group_power_save_idle_time(rg2_object_id, 20);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_enable_raid_group_power_save(rg2_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    rg_config_p = &gaggy_raid_group_config[1];

    for (disk = 0; disk < GAGGY_TEST_RAID_GROUP_WIDTH; disk++) 
    {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk].bus, rg_config_p->rg_disk_set[disk].enclosure, rg_config_p->rg_disk_set[disk].slot, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg1_object_id,disk, &object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_set_object_power_save_idle_time(object_id, 20);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* the following section will veruify we don't save power on a non-shared drive
     */
    rg_config_p = &gaggy_raid_group_config[0];	
    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting for physical drive %d,%d,%d to save power ===\n",rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot);
    /*let's wait for a while since RG propagate it to the LU and then power save on the physical drive*/
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_HIBERNATE, 120000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    rg_config_p = &gaggy_raid_group_config[1];
    mut_printf(MUT_LOG_TEST_STATUS, "=== verify physical drive %d,%d,%d is not power saving===\n",rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot);
    /*let's wait for a while since RG propagate it to the LU and then power save on the physical drive*/
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus, rg_config_p->rg_disk_set[0].enclosure, rg_config_p->rg_disk_set[0].slot, &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*we want to clean and go out w/o leaving the power saving persisted on the disks for next test*/
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system wide power saving \n", __FUNCTION__);
    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}

static void gaggy_test_create_rg(fbe_u32_t id, fbe_u32_t start_drive)
{
    fbe_api_raid_group_create_t	request;
    fbe_object_id_t 			rg_obj_id;
    fbe_status_t 				status;
    fbe_u32_t					i = 0;
    fbe_job_service_error_type_t job_error_type;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    rg_config_p = &gaggy_raid_group_config[start_drive];

    fbe_zero_memory(&request, sizeof(request));
    request.raid_group_id = id;
    request.drive_count = GAGGY_TEST_RAID_GROUP_WIDTH;
    request.power_saving_idle_time_in_seconds = 30 * 60/*idle*/;
    request.max_raid_latency_time_is_sec = 0xFFFFFFFF;
    request.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    //request.explicit_removal = FBE_FALSE;
    request.is_private = FBE_FALSE;
    request.b_bandwidth = FBE_FALSE;
    request.capacity = 0xffff;
	
	mut_printf(MUT_LOG_TEST_STATUS, "=== gaggy_test_create_rg start ===\n");

    for (i = 0; i < GAGGY_TEST_RAID_GROUP_WIDTH; ++i) {
        request.disk_array[i].bus = rg_config_p->rg_disk_set[i].bus;       
        request.disk_array[i].enclosure = rg_config_p->rg_disk_set[i].enclosure;
        request.disk_array[i].slot = rg_config_p->rg_disk_set[i].slot;	
        mut_printf(MUT_LOG_TEST_STATUS, "physical drive (%d,%d,%d) is going to build RG %d ",rg_config_p->rg_disk_set[i].bus, rg_config_p->rg_disk_set[i].enclosure, rg_config_p->rg_disk_set[i].slot, id);
    }

    status = fbe_api_create_rg(&request, FBE_TRUE, 10000, &rg_obj_id, &job_error_type);
    mut_printf(MUT_LOG_TEST_STATUS, "=== gaggy_test_create_rg completes ===\n");

}

static void gaggy_destroy_rg(fbe_u32_t id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_job_service_raid_group_destroy_t request;
    

    fbe_zero_memory(&request, sizeof (request));
    request.raid_group_id = id;
    request.wait_destroy = FBE_FALSE;
    request.destroy_timeout_msec = 60000;

    status = fbe_api_job_service_raid_group_destroy(&request);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_common_wait_for_job(request.job_number, 10000, NULL, NULL, NULL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

static void gaggy_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[1];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < 1; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < 1; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
           fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
            
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    /*1  board, 1 port, 1 encl, 15 pdo = 18*/
    status = fbe_api_wait_for_number_of_objects(18, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == 18);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End bubble_bass_test_load_physical_config() */


//-----------------------------------------------------------------------------
//                  gaggy_test_fbe_api_destroy_rg
//-----------------------------------------------------------------------------
//
//  Description:
//    This tests fbe_api_destroy_rg function. 
//
//  Parameters:
//    in_rgID          - RG ID
//
//  Returns: void
// 
//  Notes:
//
//-----------------------------------------------------------------------------
static void gaggy_test_fbe_api_destroy_rg(fbe_u32_t in_rgID)
{
    fbe_status_t          status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_destroy_rg_t  fbe_api_destroy_rg_request;
    fbe_job_service_error_type_t job_error_type;
    
    fbe_zero_memory(&fbe_api_destroy_rg_request, sizeof (fbe_api_destroy_rg_request));

    mut_printf(MUT_LOG_TEST_STATUS, " Starting destroying RG #%d\n", in_rgID);

    /* Destroy a RG */
    fbe_api_destroy_rg_request.raid_group_id = in_rgID;

    status = fbe_api_destroy_rg(&fbe_api_destroy_rg_request, FBE_TRUE, 100000, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " RG #%d destroyed successfully!\n", in_rgID);

    return;

} // end gaggy_test_fbe_api_destroy_rg()
