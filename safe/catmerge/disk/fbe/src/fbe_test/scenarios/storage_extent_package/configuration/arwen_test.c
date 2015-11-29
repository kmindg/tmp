/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file arwen_test.c
 ***************************************************************************
 *
 * @brief
 *   This file test pvd garbage collection.  
 *
 * @version
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "sep_rebuild_utils.h"                      
#include "mut.h"                                    
#include "fbe/fbe_types.h"                          
#include "sep_tests.h"                              
#include "sep_test_io.h"                            
#include "fbe/fbe_api_rdgen_interface.h"            
#include "fbe_test_configurations.h"                
#include "fbe/fbe_api_raid_group_interface.h"       
#include "fbe/fbe_api_database_interface.h"         
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_utils.h"                      
#include "pp_utils.h"                               
#include "fbe_private_space_layout.h"               
#include "fbe/fbe_api_provision_drive_interface.h"    
#include "fbe/fbe_api_common.h"                      
#include "fbe/fbe_api_terminator_interface.h"        
#include "fbe/fbe_api_sim_server.h"                 
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "sep_hook.h"


//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* arwen_short_desc = "PVD Garbage collection";
char* arwen_long_desc =
    "\n"
    "\n"
    "The test is for pvd garbage collection.\n"
    "\n"
    "STEP 1: set time threshold to 0 (day),trigger pvd garbage destroy logic\n"
    "\n"
    "STEP 2: create a RG.\n"
    "\n"
    "STEP 3: remove a drive in the RG,destroy RG.\n"
    "\n"
    "STEP 4: verify the pvd is destroyed.\n" 
    "\n"
    "STEP 5: insert a drive.\n" 
    "\n"
    "STEP 6: create a RG.\n" 
    "\n"
    "STEP 7: removed the drive,destroy the RG.\n"
    "\n"
    "STEP 8: verify the pvd is destroyed.\n"
    "\n"
"\n"
"Desription last updated: 1/17/2012.\n"    ;

char* arwen_dualsp_short_desc = "PVD Garbage collection dualsp test";
char* arwen_dualsp_long_desc =
    "\n"
    "\n"
    "The test is for pvd garbage collection dualsp test.\n"
    "\n"
    "STEP 1: set time threshold to 0 (day),trigger pvd garbage destroy logic\n"
    "\n"
    "STEP 2: create a RG.\n"
    "\n"
    "STEP 3: remove a drive in the RG,destroy RG.\n"
    "\n"
    "STEP 4: verify the pvd is destroyed.\n" 
    "\n"
    "STEP 5: insert a drive.\n" 
    "\n"
    "STEP 6: create a RG.\n" 
    "\n"
    "STEP 7: removed the drive,destroy the RG.\n"
    "\n"
    "STEP 8: verify the pvd is destroyed.\n"
    "\n"
"\n"
"Desription last updated: 1/17/2012.\n"    ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

                                                        
#define ARWEN_TEST_RAID_GROUP_COUNT 1
#define ARWEN_JOB_SERVICE_CREATION_TIMEOUT 150000 // in ms
#define ARWEN_WAIT_MSEC 300000 /* This time should be larger than 1 minute */
#define FBE_JOB_SERVICE_DESTROY_TIMEOUT    120000 /*ms*/

static fbe_notification_registration_id_t          reg_id;

/*!*******************************************************************
 * @def arwen_rg_config_1
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t arwen_rg_config_1[ARWEN_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          0,
    /* rg_disk_set */
    {{0,0,5}, {0,0,6}, {0,0,7}}
};
/*!*******************************************************************
 * @def arwen_rg_config_2
 *********************************************************************
 * @brief  user RG.
 *
 *********************************************************************/
static fbe_test_rg_configuration_t arwen_rg_config_2[ARWEN_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id,    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        0,          0,
    /* rg_disk_set */
    {{0,1,0}, {0,1,2}, {0,3,0}}
};

/*!*******************************************************************
 * @def ELMO_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 4 encl + 59 pdo) 
 *
 *********************************************************************/
 #define ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS 65  
/*!*******************************************************************
 * @def DIEGO_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
 #define DIEGO_DRIVES_PER_ENCL 15
 typedef  struct  arwen_test_notification_context_s
 {
     fbe_semaphore_t     sem;
     fbe_object_id_t     object_id;
     fbe_job_service_info_t    job_srv_info;
 }arwen_test_notification_context_t;

arwen_test_notification_context_t notification_context;
//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
 
static void arwen_test_remove_drive_and_verify(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void arwen_test_reboot_verify(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void arwen_test_newly_created_pvd_can_be_destroyed_after_reboot(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);
static void arwen_dualsp_test_reboot_verify(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot);

static void arwen_test_create_rg_1(void);
static void arwen_test_create_rg_2(void);
void  arwen_test_notification_callback_function(update_object_msg_t* update_obj_msg,
                                                     void* context);
static void arwen_test_destroy_rg(fbe_test_rg_configuration_t *rg_config_p);

//-----------------------------------------------------------------------------
//  FUNCTIONS:


/*!****************************************************************************
 *  arwen_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the arwen test.  The test does the 
 *   following:
 *
 *   - set time threshold to 1 (minute) 
 *   - remove a drive verify the pvd is destroyed
 *   - insert a drive, remove it and reboot the SEP 
 *      then verify the pvd is destroyed
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void arwen_test(void)
{    
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_system_time_threshold_info_t        out_time_threshold;

	
	fbe_system_time_threshold_info_t     in_time_threshold;
	in_time_threshold.time_threshold_in_minutes = 1;
    mut_printf(MUT_LOG_TEST_STATUS, "Set time threshold ...\n");
    status = fbe_api_set_system_time_threshold_info(&in_time_threshold);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Set time threshold Successfully...\n");

    /*get the system time threshold*/
    mut_printf(MUT_LOG_TEST_STATUS, "Get time threshold ...\n");
    status = fbe_api_get_system_time_threshold_info(&out_time_threshold);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "get time threshold : %d\n", 
                   (int)out_time_threshold.time_threshold_in_minutes);
	/*make sure we set the time shreshold successfully*/
    MUT_ASSERT_TRUE(in_time_threshold.time_threshold_in_minutes == out_time_threshold.time_threshold_in_minutes);
    /* Register for a notification with a context. */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED, 
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0, 
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                                  arwen_test_notification_callback_function, 
                                                                  &notification_context, 
                                                                  &reg_id);
    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test a newly created PVD can be destroyed when remove the drive and reboot the system */ 
    arwen_test_newly_created_pvd_can_be_destroyed_after_reboot(0, 0, 8);

    /* test a pvd can be destroyed when remove a drive */
    arwen_test_remove_drive_and_verify(0,0,5);

    /*test a pvd can be destroyed when remove a drive and reboot the system*/
    arwen_test_reboot_verify(0,3,0);


    /* Unregister the notification. */
    status = fbe_api_notification_interface_unregister_notification(arwen_test_notification_callback_function, 
                                                                    reg_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
} 

/*!**************************************************************
 * arwen_create_physical_config()
 ****************************************************************
 * @brief
 *  Configure test's physical configuration. Note that
 *  only 4160 block SAS and 520 block SAS drives are supported.
 *  We make all the disks(0_0_x) have the same capacity 
 *
 * @param b_use_4160 - TRUE if we will use both 4160 and 520.              
 *
 * @return None.
 *
 * @author
 *  10/19/2011 - Created. Zhangy
 *
 ****************************************************************/

void arwen_create_physical_config(fbe_bool_t b_use_4160)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  encl0_2_handle;
    fbe_api_terminator_device_handle_t  encl0_3_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t secondary_block_size;

    if (b_use_4160)
    {
        secondary_block_size = 4160;
    }
    else
    {
        secondary_block_size = 520;
    }

    mut_printf(MUT_LOG_LOW, "=== %s configuring terminator ===", __FUNCTION__);
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port0_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 2, port0_handle, &encl0_2_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 3, port0_handle, &encl0_3_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
       
       //disks 0_0_x should have the same capacity 
       fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /*! @note Native SATA drives are no longer supported.
     */
    for ( slot = 0; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 2, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    /* @note Native SATA drives are no longer supported.
     */
     /*skipp (0,3,0) for inserting it in the test in order to test pvd can be created in modified lifecycle condition */
    for ( slot = 1; slot < DIEGO_DRIVES_PER_ENCL; slot++)
    {
         fbe_test_pp_util_insert_sas_drive(0, 3, slot, secondary_block_size, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end arwen_create_physical_config()
 *******************************************


/*!****************************************************************************
 *  arwen_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the arwen test.  It creates the  
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void arwen_setup(void)
{  

    /* Initialize any required fields and perform cleanup if required
     */
    if (fbe_test_util_is_simulation())
    {
        /*  Load the physical package and create the physical configuration. */
        arwen_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        /* Load the logical packages */
        sep_config_load_sep_and_neit();
    }

    fbe_test_common_util_test_setup_init();

    fbe_semaphore_init(&notification_context.sem, 0, 1);
} 


/*!****************************************************************************
*    arwen_test_remove_drive_and_verify
******************************************************************************
*
* @brief
*    This function remove a drive  in arwen_rg_config_1
*    destroy RG 1,and verified PVD can be destroyed.
*     it tests the modified fbe_provision_drive_downstream_health_broken_cond_function
*    
* @param     None 
*
* @return    None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/
static void arwen_test_remove_drive_and_verify(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t                status;                             // fbe status
    fbe_object_id_t             pvd_object_id;                        // pvd's object id    
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_sim_transport_connection_target_t active_target;

    /*Create a RG */
    arwen_test_create_rg_1();
    
    /* Get the PVD object id before we remove the drive */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Destroy RG will trigger pvd destroy*/
    status = fbe_test_sep_util_destroy_raid_group(arwen_rg_config_1[0].raid_group_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	if (fbe_test_sep_util_get_dualsp_test_mode())
    {
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		/* Make sure the RAID object successfully destroyed on SPB*/
		status = fbe_api_database_lookup_raid_group_by_number(arwen_rg_config_1[0].raid_group_id, &rg_object_id);
		MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
		mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroyed successfully ON SPB! ===");
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, slot);
	if (fbe_test_sep_util_get_dualsp_test_mode())
    {
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	   status = fbe_test_pp_util_pull_drive_hw(bus, enclosure, slot);
       MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	   
	   /* Verify that the PVD  are in the FAIL state */
	  // sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	   
	   /* get the active SP, Make sure SPA is active in the beginning */
	   fbe_test_sep_get_active_target_id(&active_target);
	   MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);
	   
    }
    status = fbe_test_pp_util_pull_drive_hw(bus, enclosure, slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);                 
    
    /* Print message as to where the test is at */
    mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
               bus, enclosure, slot, pvd_object_id);
        
    /* Verify that the PVD  are in the FAIL state */
   //sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification ",__FUNCTION__);
    notification_context.object_id = pvd_object_id;
    status = fbe_semaphore_wait_ms(&notification_context.sem, ARWEN_WAIT_MSEC);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_TIMEOUT);
	
	if (fbe_test_sep_util_get_dualsp_test_mode())
    {
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		/* Make sure the PVD successfully destroyed */
		status = fbe_api_provision_drive_get_obj_id_by_location(bus,
																enclosure,
																slot,
																&pvd_object_id);
		MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
       fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    /* Make sure the PVD successfully destroyed */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

    return;
}

/*!****************************************************************************
*    arwen_test_reboot_verify
******************************************************************************
*
* @brief
*    This function 
*     1. insert a drive.
*     2. create a RG.
*     3. remove a drive.
*     4. reboot the system.
*     5. verified PVD can be destroyed in downstream_health_not_optimal cond.
*    it tests lifecycle fbe_provision_drive_downstream_health_not_optimal_cond_function
* @param     None 
*
* @return    None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/
static void arwen_test_reboot_verify(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t                 status;                             // fbe status
    fbe_object_id_t              pvd_object_id;                        // pvd's object id
    fbe_api_terminator_device_handle_t    drive_handle;
    fbe_u32_t                             total_objects = 0;
    
    /* insert a dirve to test pvd can be created under modified lifecycle condition*/
    fbe_test_pp_util_insert_sas_drive(bus,enclosure, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    
    /* Create a RG */
    arwen_test_create_rg_2();
    
    /*    Get the PVD object id before we remove the drive */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	
	 mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, slot);
	 
	 status = fbe_test_pp_util_pull_drive_hw(bus, enclosure, slot);
	 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);				  
	 
    fbe_api_sleep(10000);
	 /* Print message as to where the test is at */
	 mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
				bus, enclosure, slot, pvd_object_id);
	
	 /*    Verify that the PVD	are in the FAIL state */
	sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* Destroy RG will trigger pvd destroy*/
    status = fbe_test_sep_util_destroy_raid_group(arwen_rg_config_2[0].raid_group_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification ",__FUNCTION__);
    notification_context.object_id = pvd_object_id;
    status = fbe_semaphore_wait_ms(&notification_context.sem, ARWEN_WAIT_MSEC);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_TIMEOUT);
    /* Make sure the PVD successfully destroyed */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);


    return;
}
/*!****************************************************************************
 *  arwen_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Diego test. 
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void arwen_cleanup(void)
{
    /* Destroy the semaphore. */
    fbe_semaphore_destroy(&notification_context.sem); 

    if (fbe_test_util_is_simulation())
    {
         fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

}

/*!**************************************************************
 * arwen_test_create_rg_1()
 ****************************************************************
 * @brief
 *  Configure the arwen test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void arwen_test_create_rg_1(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
    fbe_object_id_t						rg_object_id_from_job = FBE_OBJECT_ID_INVALID;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(arwen_rg_config_1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(arwen_rg_config_1[0].job_number,
                                         ARWEN_JOB_SERVICE_CREATION_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(arwen_rg_config_1[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	/* Verify the object id returned from job is correct */
    MUT_ASSERT_INT_EQUAL(rg_object_id, rg_object_id_from_job);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);
	
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    /* Wait RAID object to be ready on SPB */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		/* Verify the object id of the raid group */
		status = fbe_api_database_lookup_raid_group_by_number(arwen_rg_config_1[0].raid_group_id, &rg_object_id);
		
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
		
		/* Verify the raid group get to ready state in reasonable time */
		status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
														 FBE_LIFECYCLE_STATE_READY, 20000,
														 FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object ON SPB %d\n", rg_object_id);

		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End arwen_test_create_rg_1() */

/*!**************************************************************
 * arwen_test_create_rg_2()
 ****************************************************************
 * @brief
 *  Configure the arwen test logical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void arwen_test_create_rg_2(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t        job_error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_status_t                        job_status = FBE_STATUS_OK;
	fbe_object_id_t                     rg_object_id_from_job = FBE_OBJECT_ID_INVALID;

    fbe_test_wait_for_rg_pvds_ready(arwen_rg_config_2);

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(arwen_rg_config_2);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(arwen_rg_config_2[0].job_number,
                                         ARWEN_JOB_SERVICE_CREATION_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         &rg_object_id_from_job);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(arwen_rg_config_2[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	MUT_ASSERT_INT_EQUAL(rg_object_id, rg_object_id_from_job);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
    /* Wait RAID object to be ready on SPB */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		/* Verify the object id of the raid group */
		status = fbe_api_database_lookup_raid_group_by_number(arwen_rg_config_2[0].raid_group_id, &rg_object_id);
		
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
		
		/* Verify the raid group get to ready state in reasonable time */
		status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
														 FBE_LIFECYCLE_STATE_READY, 20000,
														 FBE_PACKAGE_ID_SEP_0);
		
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		
		mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object on SPB %d\n", rg_object_id);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End arwen_test_create_rg() */

void arwen_test_notification_callback_function(update_object_msg_t * update_obj_msg,void * context)
{
    arwen_test_notification_context_t*  notification_context = (arwen_test_notification_context_t*)context;

    MUT_ASSERT_TRUE(NULL != update_obj_msg);
    MUT_ASSERT_TRUE(NULL != context);

    if((update_obj_msg->object_id == notification_context->object_id)&&(update_obj_msg->notification_info.notification_data.job_service_error_info.job_type == FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE))
    {
        fbe_semaphore_release(&notification_context->sem, 0, 1, FALSE);
    }
}

/*!**************************************************************
 * arwen_dualsp_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/17/2012 - Created. 
 *
 ****************************************************************/
void arwen_dualsp_setup(void)
{
     fbe_sim_transport_connection_target_t sp;

    if (fbe_test_util_is_simulation()) {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for arwen dualsp test ===\n");

        /* set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "sp B");
        /* Load the physical configuration */
        
        arwen_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
        /* set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "sp B");
        /* Load the physical configuration */
        arwen_create_physical_config(FBE_FALSE /* Do not use 4160 block drives */);
       

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* Load sep and neit packages */
        sep_config_load_sep_and_neit_both_sps();
        
    }
    fbe_test_common_util_test_setup_init();

    /* Initialize the semaphore.. */
    fbe_semaphore_init(&notification_context.sem, 0, 1);

    return;
}
/**************************************
 * end ezio_dualsp_setup()
 **************************************/


/*!**************************************************************
 * arwen_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the arwen test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/17/2012 - Created. 
 *
 ****************************************************************/

void arwen_dualsp_cleanup(void)
{
    /* Destroy the semaphore. */
    fbe_semaphore_destroy(&notification_context.sem); 

    /* Clear the dual sp mode*/
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if ((fbe_test_util_is_simulation())) {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();

        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end arwen_dualsp_cleanup()
 ******************************************/
 
 /*!**************************************************************
  * arwen_dualsp_test()
  ****************************************************************
  * @brief
  *   - enable dualsp mode
  *   - set time threshold to 0 (day) to trigger pvd destroy logic
  *   - remove a drive verify the pvd is destroyed
  *   - insert a drive, remove it and reboot the SEP 
  *      then verify the pvd is destroyed
  *
  * @param None.               
  *
  * @return None.
  *
  * @author
  *  1/17/2012
  *
  ****************************************************************/
 void arwen_dualsp_test(void)
 {
     fbe_sim_transport_connection_target_t active_target;
     fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
     fbe_system_time_threshold_info_t        out_time_threshold;
	 fbe_system_time_threshold_info_t	  in_time_threshold;

     /* Enable dualsp mode */
     fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
 
     /* get the active SP, Make sure SPA is active in the beginning */
     fbe_test_sep_get_active_target_id(&active_target);
     MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);
 
     /* Test start */
     mut_printf(MUT_LOG_TEST_STATUS, "---Start arwen_dualdp_test---\n");
	 in_time_threshold.time_threshold_in_minutes = 1;
	 mut_printf(MUT_LOG_TEST_STATUS, "Set time threshold ...\n");
	 status = fbe_api_set_system_time_threshold_info(&in_time_threshold);
	 MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	 mut_printf(MUT_LOG_TEST_STATUS, "Set time threshold Successfully...\n");


    
    /* set the target server to SPB then get the time threshold */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "Database get time threshold ...\n");
    status = fbe_api_get_system_time_threshold_info(&out_time_threshold);
    (&out_time_threshold);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "get time threshold : %d\n", 
                   (int)out_time_threshold.time_threshold_in_minutes);
	/*make sure we set the time shreshold successfully*/
    MUT_ASSERT_TRUE(in_time_threshold.time_threshold_in_minutes == out_time_threshold.time_threshold_in_minutes);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
    /* get the active SP, Make sure SPA is active in the beginning */
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);
    
    /* Register for a notification with a context. */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED, 
                                                               FBE_PACKAGE_NOTIFICATION_ID_SEP_0, 
                                                               FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                               arwen_test_notification_callback_function, 
                                                               &notification_context.sem, 
                                                               &reg_id);
    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* test a pvd can be destroyed when remove a drive */
    arwen_test_remove_drive_and_verify(0,0,7);
    
    /* test a pvd can be destroyed when remove a drive and reboot the system */
    arwen_dualsp_test_reboot_verify(0,3,0);

    /* Unregister the notification. */
    status = fbe_api_notification_interface_unregister_notification(arwen_test_notification_callback_function, 
                                                                 reg_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;
}

/*!****************************************************************************
*    arwen_dualsp_test_reboot_verify
******************************************************************************
*
* @brief
*    This function 
*     1. insert a drive.
*     2. create a RG.
*     3. remove a drive.
*     4. reboot the system.
*     5. verified PVD can be destroyed in downstream_health_not_optimal cond.
*    
* @param     None 
*
* @return    None 
*
* 9/27/2011 - Created by zhangy
*****************************************************************************/
static void arwen_dualsp_test_reboot_verify(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t                 status;                             // fbe status
    fbe_object_id_t             pvd_object_id;                        // pvd's object id
    fbe_api_terminator_device_handle_t    drive_handle;
    fbe_u32_t                            total_objects = 0;
    fbe_sim_transport_connection_target_t active_target;
    
    fbe_object_id_t                                 rg_object_id;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_test_pp_util_insert_sas_drive(bus,enclosure, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
    
    /* Make sure we have the expected number of objects.*/
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* insert a dirve to test pvd can be created under modified lifecycle condition*/
    status = fbe_test_pp_util_insert_sas_drive(bus,enclosure, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
    
    /* Make sure we have the expected number of objects.*/
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == ARWEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);
    /* Create a RG */
    arwen_test_create_rg_2();
    
    /*    Get the PVD object id before we remove the drive */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                        enclosure,
                                                        slot,
                                                        &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	mut_printf(MUT_LOG_TEST_STATUS, "removing drive: %d - %d - %d\n", bus, enclosure, slot);
    /* Verify that the PVD  are in the READY state */
    sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY);
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_test_pp_util_pull_drive_hw(bus, enclosure, slot);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	   
	/* Verify that the PVD  are in the FAIL state*/
    //sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	   
	status = fbe_test_pp_util_pull_drive_hw(bus, enclosure, slot);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 				
	   
	/* Print message as to where the test is at */
	mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", bus, enclosure, slot, pvd_object_id);
	   
	   /* Verify that the PVD  are in the FAIL state*/
	sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    /*reboot sep*/
    mut_printf(MUT_LOG_TEST_STATUS, " --reboot dual sp -- \n");
    fbe_test_sep_util_destroy_neit_sep_both_sps();
    sep_config_load_sep_and_neit_both_sps();
    
    
   
   /* get the active SP, Make sure SPA is active in the beginning */
   fbe_test_sep_get_active_target_id(&active_target);
   MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);
   /* Destroy RG will trigger pvd destroy*/
   status = fbe_test_sep_util_destroy_raid_group(arwen_rg_config_2[0].raid_group_id);
   MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
   fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
   /* Make sure the RAID object successfully destroyed on SPB*/
   status = fbe_api_database_lookup_raid_group_by_number(arwen_rg_config_2[0].raid_group_id, &rg_object_id);
   MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
   MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
   mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroyed successfully on SPB! Object ID: %X ===",rg_object_id);
   fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
   
   
   
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification ",__FUNCTION__);
    notification_context.object_id = pvd_object_id;
    status = fbe_semaphore_wait_ms(&notification_context.sem, ARWEN_WAIT_MSEC);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_TIMEOUT);
    
	 fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	 /* Make sure the PVD successfully destroyed on SPB*/
	 status = fbe_api_provision_drive_get_obj_id_by_location(bus,
															 enclosure,
															 slot,
															 &pvd_object_id);
	 MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	/* Make sure the PVD successfully destroyed on SPA*/
	status = fbe_api_provision_drive_get_obj_id_by_location(bus,
															enclosure,
															slot,
															&pvd_object_id);
	MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    return;
}//End arwen_test_reboot_verify


/*!****************************************************************************
*    arwen_test_newly_created_pvd_can_be_destroyed_after_reboot
******************************************************************************
*
* @brief
*    This function 
*    1. remove drive bus_enclosure_slot
*    2. Wait for the PVD of this drive destroyed
*    3. insert the drive again
*    4. Add hook function to stop non-paged metadata initializing
*    5. remove the drive and delete the hook function 
*    6. note that this PVD is not initialized completed. check if it is destroyed.
*    7. The PVD should be destroyed, or there is a bug
*
* @param     None 
*
* @return    None 
*
*****************************************************************************/
static void arwen_test_newly_created_pvd_can_be_destroyed_after_reboot(fbe_u32_t bus,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t                 status;                             // fbe status
    fbe_object_id_t              pvd_object_id;                        // pvd's object id
    fbe_api_terminator_device_handle_t    drive_handle;
    fbe_lifecycle_state_t object_state = FBE_LIFECYCLE_STATE_SPECIALIZE;
    
    /* Get the PVD object id */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Set hook function */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: add hook function in nonpaged init condition ",__FUNCTION__);
    status = fbe_api_scheduler_add_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

     /* Pull out this drive */
	 status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, &drive_handle);
	 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);				  
	 /* Print message as to where the test is at */
	 mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
				bus, enclosure, slot, pvd_object_id);
	 
	 /* Verify that the PVD	are in the FAIL state */
	sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification ",__FUNCTION__);
    notification_context.object_id = pvd_object_id;
    status = fbe_semaphore_wait_ms(&notification_context.sem, ARWEN_WAIT_MSEC);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for destroy PVD, status = %d ",__FUNCTION__, status);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_TIMEOUT);
    
    /* Make sure the PVD successfully destroyed */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: get PVD(%d) by b_e_s(%d_%d_%d) status = %d ",__FUNCTION__, 
               pvd_object_id, bus, enclosure, slot, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: pvd_object_id %d was destroyed ",__FUNCTION__, pvd_object_id);

    
    /* reinsert drive 0_0_6 */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: insert drive again ",__FUNCTION__);
    //fbe_test_pp_util_insert_sas_drive(bus,enclosure, slot, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY, &drive_handle);
    status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot, drive_handle);
    
    /* Wait for Hook */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for the debug hook ",__FUNCTION__);
    status = fbe_test_wait_for_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                              0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: get pvd_object_id's lifecycle_state ",__FUNCTION__);
    status = fbe_api_get_object_lifecycle_state(pvd_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " PVD(%d) lifecycle_state is %d \n", pvd_object_id, object_state);

     /* Pull out this drive */
	 status = fbe_test_pp_util_pull_drive(bus, enclosure, slot, &drive_handle);
	 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);				  
	 /* Print message as to where the test is at */
	 mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X\n", 
				bus, enclosure, slot, pvd_object_id);
    
    /* clear Hook to let PVD SPEC rotary continue */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: delete the debug hook and let pvd lifecycle continue ",__FUNCTION__);
    status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,
                                              FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,
                                              0,
                                              0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    

    mut_printf(MUT_LOG_TEST_STATUS, "%s: get pvd_object_id's lifecycle_state ",__FUNCTION__);
    status = fbe_api_get_object_lifecycle_state(pvd_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, " PVD(%d) lifecycle_state is %d \n", pvd_object_id, object_state);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the pvd destroy notification ",__FUNCTION__);
    notification_context.object_id = pvd_object_id;
    status = fbe_semaphore_wait_ms(&notification_context.sem, ARWEN_WAIT_MSEC);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for destroy PVD, status = %d ",__FUNCTION__, status);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: get pvd_object_id's lifecycle_state ",__FUNCTION__);
    status = fbe_api_get_object_lifecycle_state(pvd_object_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_NO_OBJECT, status);
    mut_printf(MUT_LOG_TEST_STATUS, " Get lifecycle of pvd(%d), status = %d \n", pvd_object_id, status);

    /* Make sure the PVD successfully destroyed */
    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                            enclosure,
                                                            slot,
                                                            &pvd_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: get PVD(%d) by b_e_s(%d_%d_%d) status = %d ",__FUNCTION__, 
               pvd_object_id, bus, enclosure, slot, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: insert drive again ",__FUNCTION__);
    status = fbe_test_pp_util_reinsert_drive(bus, enclosure, slot, drive_handle);
    return;
}
/*!****************************************************************************
 * arwen_test_destroy_rg()
 ******************************************************************************
 * @brief
 *  This function destroy the raid group.
 *
 * @param rg_config_p - pointer to raid group configuration table..
 *
 * @return None.
 *
 ******************************************************************************/
static void arwen_test_destroy_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                     status;
    fbe_api_job_service_raid_group_destroy_t         fbe_raid_group_destroy_request;
    fbe_object_id_t                                  rg_object_id;
    fbe_job_service_error_type_t                     job_error_code;
    fbe_status_t                                     job_status;
  
    /* Validate that the raid group exist */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
        
    /* Destroy a RAID group */
    fbe_raid_group_destroy_request.raid_group_id = rg_config_p->raid_group_id;
    fbe_raid_group_destroy_request.job_number = 0;
    fbe_raid_group_destroy_request.wait_destroy = FBE_FALSE;
    fbe_raid_group_destroy_request.destroy_timeout_msec = FBE_JOB_SERVICE_DESTROY_TIMEOUT;
    
    status = fbe_api_job_service_raid_group_destroy(&fbe_raid_group_destroy_request);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroy job sent successfully! ===");
    mut_printf(MUT_LOG_TEST_STATUS, 
            "Raid group id: %d Object Id: 0x%x Job number 0x%llX", 
            rg_config_p->raid_group_id, rg_object_id,
	    (unsigned long long)fbe_raid_group_destroy_request.job_number);
    
    status = fbe_api_common_wait_for_job(fbe_raid_group_destroy_request.job_number ,
                                             FBE_JOB_SERVICE_DESTROY_TIMEOUT,
                                             &job_error_code,
                                             &job_status,
                                             NULL);
    
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG destruction failed");
    MUT_ASSERT_TRUE_MSG((job_error_code == FBE_JOB_SERVICE_ERROR_NO_ERROR), "RG destruction failed");
    /* Make sure the RAID object successfully destroyed */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroyed successfully! ===");

}

