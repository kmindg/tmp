/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file danny_din.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test to make sure notification and other 
 *  infrastructure related connections work properly
 *
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
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_trace_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * danny_din_short_desc = "test FBE API Infrastructure";
char * danny_din_long_desc =
    "\n"
    "\n"
    "The Danny Din test is used to test FBE Infrastructure for both dual and single SP\n"
    "\n"
    "It covers the following cases:\n"
    "    - registers for notification on SPA and makes sure a change in object state is received.\n"
    "    - Brings up SPB and does the same test on it while SPA is till on\n"
	"    - Test ownership models for object to verify only one side is ACTIVE at a time\n"
    "\n"
    "Dependencies:\n"
    "    - SEP must have at least one RAID group\n"
    "    - Physical Package must have at least one drive present\n"
    "\n"
    "Starting Config:\n"
    "    [PP]  4 fully populated enclosures configured\n"
    "    [SEP] 3 LUNs on a raid-0 object with 6 drives\n"
    "    [SEP] 3 LUNs on a raid-5 object with 5 drives\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create all raid objects with I/O edges attached to all VDs.\n"
    "    - Create all lun objects with an I/O edge attached to its RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Notification registration\n"
    "    - Register for Physical package and SEP notification using FBE API\n"
    "\n"
    "STEP 3: Change drive object state\n"
    "    - Set one drive object to ACTIVATE and expect to get ready back\n"
    "\n"
    "STEP 4: Verify READY from drive\n"
    "    - Completion function verifies the drive is in ready. A timed out semaphore is used to make sure test does not hang\n"
    "\n"
    "STEP 5: Bring SPB and repeat.\n"
    "    - Set the client swith to route packets to SPB and repeat all steps starting from step 2 on SPB\n"
    "\n"
    "STEP 6: Unregister\n"
    "    - Unregister from notifications\n"
	"STEP 7: Ownership testing\n"
    "    - Validate for a VD object that it is the ACTIVE on one of the SPS\n"
	"    - Take down the SP who's object is ACTIVE and make sure the other side went from PASSIVE to ACTIVE\n"
	"    - Return the SP we took down and make sure it comes up with a PASSIVE object and the other side stay ACTIVE\n"
	"STEP 8: State Override testing\n"
    "    - Check the ability to override the state of the objects on the SP\n"
	"    - Send FBE API to one of the SPs and make sure we can set all objects or a single object to PASSIVE or ACTIVE\n"
	"STEP 9: Scheduler credits test\n"
    "    - Use all FBE APIs to set and get the scehduler credits\n"
    "STEP 10: Persist API test (Abort API test so far)\n"
	"Last Update: 11/10/11\n"
    ;


static void danny_din_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context2);
static fbe_status_t danny_din_get_object_of_type(fbe_topology_object_type_t type, fbe_package_id_t package_id, fbe_object_id_t *obj_id);
static fbe_status_t danny_din_get_object_of_class(fbe_class_id_t class_id, fbe_package_id_t package_id, fbe_object_id_t *obj_id);
static fbe_status_t danny_din_run_notification_test(fbe_sim_transport_connection_target_t target_sp, fbe_semaphore_t *sem);
static fbe_status_t danny_din_run_object_metadata_ownership_test(void);
static fbe_status_t danny_din_run_sp_ownership_test(void);
static fbe_status_t danny_din_run_state_override_test(void);
static fbe_notification_registration_id_t  reg_id;
static void danny_din_logical_setup(void);
static fbe_status_t danny_din_run_scheduler_credits_interface_test(void);
static void danny_din_run_persist_api_test(void);

static fbe_object_id_t	expected_object_id = FBE_OBJECT_ID_INVALID;;

void danny_din_test(void)
{
	fbe_semaphore_t 						sem;
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;

    fbe_semaphore_init(&sem, 0, 1);

    mut_printf(MUT_LOG_LOW, "=== danny_din notification test Starting on SPB ===");
	status = danny_din_run_notification_test(FBE_SIM_SP_B, &sem);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "=== danny_din notification test Starting on SPA ===");
	status = danny_din_run_notification_test(FBE_SIM_SP_A, &sem);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem);

	/*let's run the ownership tests
	* I M P O R T A N T - Do not change the order of the tests below or the order inside them unless you know what you do.
	The logic of the tests assume a certain state of the system before during and after the test to make sure all behavior
	is as expected*/
    status = danny_din_run_object_metadata_ownership_test();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = danny_din_run_sp_ownership_test();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = danny_din_run_state_override_test();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*validate the scheduler credits interface*/
	status = danny_din_run_scheduler_credits_interface_test();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    danny_din_run_persist_api_test();
	mut_printf(MUT_LOG_LOW, "=== danny_din_test ended successfully===");

}

extern void sep_config_load_sep_package(void);
extern void sep_config_load_sep_setup_package_entries(void);

static void danny_din_dualsp_load_sep(void)
{
    sep_config_load_balance_load_sep();
}

/*!**************************************************************
 * danny_din_singlesp_test()
 ****************************************************************
 * @brief
 *  Run an fbe_infrastructure and notification verification test. 
 *  For now, this is a placeholder for a version to test single SP
 *  is created.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/18/2011 - Created. Arun S
 *
 ****************************************************************/

void danny_din_singlesp_test(void)
{
    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Run the test */
    danny_din_test();

    return;
}

/******************************************
 * end danny_din_test()
 ******************************************/

/*!**************************************************************
 * danny_din_dualsp_test()
 ****************************************************************
 * @brief
 *  Run an fbe_infrastructure and notification verification test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/18/2011 - Created. Arun S
 *
 ****************************************************************/

void danny_din_dualsp_test(void)
{
    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run the test */
    danny_din_test();

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end danny_din_dualsp_test()
 ******************************************/

/*!**************************************************************
 * danny_din_setup()
 ****************************************************************
 * @brief
 *  Setup for danny_din test. This is a placeholder for a version
 *  of the test to run single SP is created. 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/18/2011 - Created. Arun S
 *
 ****************************************************************/

void danny_din_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Danny Din test ===\n");

    if (fbe_test_util_is_simulation())
    {
        /*Load the physical package and create the physical configuration.
    	 We are leveraging the function from the Elmo test to do so.  */
        elmo_physical_config();

        danny_din_dualsp_load_sep();
    }
}

/*!**************************************************************
 * danny_din_dualsp_setup()
 ****************************************************************
 * @brief
 *  Run an fbe_infrastructure and notification verification test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/18/2011 - Created. Arun S
 *
 ****************************************************************/
void danny_din_dualsp_setup(void)
{
	fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;
	fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
	fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;

	/* we do the setup on SPB side*/
	mut_printf(MUT_LOG_LOW, "=== danny_din_test Loading Physical Config on SPB ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /*Load the physical package and create the physical configuration.
	 We are leveraging the function from the Elmo test to do so.  */
    elmo_physical_config();

	danny_din_dualsp_load_sep();

	/* we do the setup on SPA side*/
	mut_printf(MUT_LOG_LOW, "=== danny_din_test Loading Physical Config on SPA ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	
    /*Load the physical package and create the physical configuration.
	 We are leveraging the functionfrom the Elmo test to do so.*/
    elmo_physical_config();

	danny_din_dualsp_load_sep();

	do {
		fbe_api_sleep(100);
		/* We have to figure out whitch SP is ACTIVE */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		fbe_api_cmi_service_get_info(&spa_cmi_info);
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		fbe_api_cmi_service_get_info(&spb_cmi_info);

		if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_A;
			passive_connection_target = FBE_SIM_SP_B;
		}

		if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_B;
			passive_connection_target = FBE_SIM_SP_A;
		}
	}while(active_connection_target != FBE_SIM_SP_A && active_connection_target != FBE_SIM_SP_B);

	/* we do the setup on ACTIVE side*/
	mut_printf(MUT_LOG_LOW, "=== danny_din_test Loading Config on ACTIVE side (%s) ===", active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
	fbe_api_sim_transport_set_target_server(active_connection_target);/*just to be on the safe side, even though it's the default*/
    
}

static void danny_din_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;
    
	if (update_object_msg->object_id == expected_object_id) {
		fbe_semaphore_release(sem, 0, 1 ,FALSE);
	}
}

static fbe_status_t danny_din_get_object_of_type(fbe_topology_object_type_t type, fbe_package_id_t package_id, fbe_object_id_t *obj_id)
{
	fbe_status_t				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_object_id_t	*			obj_array = NULL;
	fbe_u32_t					total = 0;
	fbe_topology_object_type_t	obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
	fbe_u32_t					object_count = 0;

	status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
		return status;
	}

	obj_array = (fbe_object_id_t *)fbe_api_allocate_memory (sizeof(fbe_object_id_t) * object_count);

	status = fbe_api_enumerate_objects(obj_array, object_count, &total, package_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	for (total -= 1;total > 0 ; total--) {
		do{
			status = fbe_api_get_object_type(obj_array[total], &obj_type, package_id);
		}while (status == FBE_STATUS_BUSY);

		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
		if (obj_type == type) {
			*obj_id = obj_array[total];
			fbe_api_free_memory(obj_array);
			return FBE_STATUS_OK;
		}
	}

	fbe_api_free_memory(obj_array);
	return FBE_STATUS_GENERIC_FAILURE;/*if we got here it's bad*/
}

static fbe_status_t danny_din_get_object_of_class(fbe_class_id_t class_id, fbe_package_id_t package_id, fbe_object_id_t *obj_id)
{
	fbe_status_t				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_object_id_t	*			obj_array = NULL;
	fbe_u32_t					total = 0;
	fbe_class_id_t 				obj_class = FBE_CLASS_ID_INVALID;
	fbe_u32_t					object_count = 0;

	status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
		return status;
	}

	obj_array = (fbe_object_id_t *)fbe_api_allocate_memory (sizeof(fbe_object_id_t) * object_count);

	status = fbe_api_enumerate_objects(obj_array, object_count, &total, package_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
	for (total -= 1;total > 0 ; total--) {
		do{
			status = fbe_api_get_object_class_id(obj_array[total], &obj_class, package_id);
			fbe_api_sleep(1000);
		}while (status == FBE_STATUS_BUSY);

		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
		if (class_id == obj_class) {
			*obj_id = obj_array[total];
			fbe_api_free_memory(obj_array);
			return FBE_STATUS_OK;
		}
	}

	fbe_api_free_memory(obj_array);

	return FBE_STATUS_GENERIC_FAILURE;/*if we got here it's bad*/
}

static fbe_status_t danny_din_run_notification_test(fbe_sim_transport_connection_target_t target_sp, fbe_semaphore_t *sem)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t							obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u8_t								sn[]= {"5000097B80000004"};
	fbe_u32_t								sn_length = (fbe_u32_t)strlen(sn);

    fbe_api_sim_transport_set_target_server(target_sp);

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL | FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
																  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE | FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
																  danny_din_commmand_response_callback_function,
																  sem,
																  &reg_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /*the folowing is a very long way to get the physical drive object id
	but I did it just to make sure we test a few of the fbe APIs.
	based on the configuration we use this should be the fifth drive in the first enclosure ( skipping over the
     system drives )*/
	
	status  = fbe_api_physical_drive_get_id_by_serial_number(sn, sn_length, &obj_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	expected_object_id = obj_id;

	mut_printf(MUT_LOG_LOW, "=== danny_din_test setting physical 0x%X drive to activate and waiting for notification===", obj_id);

	status = fbe_api_set_object_to_state(obj_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_semaphore_wait_ms(sem, 20000);

	MUT_ASSERT_TRUE(status != FBE_STATUS_TIMEOUT);
	mut_printf(MUT_LOG_LOW, "=== danny_din_test got notification from physical drive===");

	if (!fbe_test_sep_util_get_dualsp_test_mode())
	{
	mut_printf(MUT_LOG_LOW, "=== danny_din_test setting Provisioned 0x%X to activate and waiting for notification===", obj_id);

	expected_object_id = obj_id;

	status = fbe_api_set_object_to_state(obj_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	status = fbe_semaphore_wait_ms(sem, 20000);

	MUT_ASSERT_TRUE(status != FBE_STATUS_TIMEOUT);
	mut_printf(MUT_LOG_LOW, "=== danny_din_test got notification from Provisioned===\n");
	}

	status = fbe_api_notification_interface_unregister_notification(danny_din_commmand_response_callback_function, reg_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	return FBE_STATUS_OK;
}

void danny_din_dualsp_destroy()
{
    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
    	/*start with SPB so we leave the switch at SPA when we exit*/
    	mut_printf(MUT_LOG_LOW, "=== danny_din_test UNLoading Config from SPB ===");
    	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    	fbe_test_sep_util_destroy_sep_physical();
    
    	/*back to A*/
    	mut_printf(MUT_LOG_LOW, "=== danny_din_test UNLoading Config from SPA ===");
    	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    	fbe_test_sep_util_destroy_sep_physical();
    }

    return;
}

/*!**************************************************************
 * danny_din_destroy()
 ****************************************************************
 * @brief
 *  Cleanup for danny_din test. This is a placeholder for a version
 *  of the test to run single SP is created. 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/18/2011 - Created. Arun S
 *
 ****************************************************************/
void danny_din_destroy()
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;
}

static fbe_status_t danny_din_run_object_metadata_ownership_test(void)
{
	fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
	fbe_object_id_t			obj_id = FBE_OBJECT_ID_INVALID;
	fbe_metadata_info_t		sp_a_metadata_info, sp_b_metadata_info;
	fbe_cmi_service_get_info_t spb_cmi_info;


	/*this test will make sure only one side is active and when an SP gos down the PASSIVE side turns ACTIVE
	after that, the SP will be brough up and the ACTIVE side should stay active*/

	/*step 1. verify we have only one passive and active sides we read it from both A and B side. */
	mut_printf(MUT_LOG_LOW, "=== danny_din_test checking ACTIVE/PASSIVE for PVD on slot 0_0_4 ===");

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 4, &obj_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	do {
		status  = fbe_api_base_config_metadata_get_info(obj_id, &sp_a_metadata_info);
		fbe_api_sleep(1000);
	} while (status == FBE_STATUS_BUSY);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

	
	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 4, &obj_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	do {
		status  = fbe_api_base_config_metadata_get_info(obj_id, &sp_b_metadata_info);
		fbe_api_sleep(1000);
	} while (status == FBE_STATUS_BUSY);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_api_cmi_service_get_info(&spb_cmi_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(spb_cmi_info.sp_state != FBE_CMI_STATE_BUSY);
	MUT_ASSERT_TRUE(sp_a_metadata_info.metadata_element_state != FBE_METADATA_ELEMENT_STATE_INVALID);

    
	if (spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE) {
		MUT_ASSERT_TRUE(sp_a_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);
		MUT_ASSERT_TRUE(sp_b_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);
	}else{
		MUT_ASSERT_TRUE(sp_a_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);
		MUT_ASSERT_TRUE(sp_b_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);
	}

	mut_printf(MUT_LOG_LOW, "=== danny_din_test Verified ACTIVE/PASSIVE Step 1 ===");
    
	/*return to default*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	return FBE_STATUS_OK;

}

static fbe_status_t danny_din_run_sp_ownership_test(void)
{
	fbe_cmi_service_get_info_t	cmi_info_spa, cmi_info_spb;
	fbe_status_t				status = FBE_STATUS_GENERIC_FAILURE;

	mut_printf(MUT_LOG_LOW, "=== danny_din_test Starting ACTIVE/PASSIVE Step 2 ===");

	/*to start with, we need to make sure we have only one active SP so let's check with the CMI service
	since we started SPB before SPA, we expect a certain state which we would later change*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_cmi_service_get_info(&cmi_info_spa);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_api_cmi_service_get_info(&cmi_info_spb);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*verify everything is what we want it to be*/
	MUT_ASSERT_TRUE(cmi_info_spa.peer_alive == FBE_TRUE);
	MUT_ASSERT_TRUE(cmi_info_spb.peer_alive == FBE_TRUE);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_id == FBE_CMI_SP_ID_A);
	MUT_ASSERT_TRUE(cmi_info_spb.sp_id == FBE_CMI_SP_ID_B);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_state == FBE_CMI_STATE_PASSIVE);
	MUT_ASSERT_TRUE(cmi_info_spb.sp_state == FBE_CMI_STATE_ACTIVE);

	/*now let's take down SPB*/
	mut_printf(MUT_LOG_LOW, "=== danny_din_test UNLoading Config from SPB ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	fbe_test_sep_util_destroy_sep_physical();

	fbe_thread_delay(2000);

    /*and see if SPA now thinks he is the master*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_cmi_service_get_info(&cmi_info_spa);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    MUT_ASSERT_TRUE(cmi_info_spa.peer_alive == FBE_FALSE);
    MUT_ASSERT_TRUE(cmi_info_spa.sp_id == FBE_CMI_SP_ID_A);
    MUT_ASSERT_TRUE(cmi_info_spa.sp_state == FBE_CMI_STATE_ACTIVE);

	/*now let's bring up SPB and see things match what we expect*/
	fbe_thread_delay(2000);
	mut_printf(MUT_LOG_LOW, "=== danny_din_test Loading Config on SPB ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	
	/*FIXME - for now we load logical after physical, and do not run any configuration service
	we need to make sure SPB is updated with the correct configuration*/
	elmo_physical_config();
	danny_din_dualsp_load_sep();

	fbe_thread_delay(1000);

	/*to start with, we need to make sure we have only one active SP so let's check with the CMI service
	since we started SPB after SPA, we expect a certain state which we would later change*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_cmi_service_get_info(&cmi_info_spa);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_api_cmi_service_get_info(&cmi_info_spb);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*verify everything is what we want it to be*/
	MUT_ASSERT_TRUE(cmi_info_spa.peer_alive == FBE_TRUE);
	MUT_ASSERT_TRUE(cmi_info_spb.peer_alive == FBE_TRUE);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_id == FBE_CMI_SP_ID_A);
	MUT_ASSERT_TRUE(cmi_info_spb.sp_id == FBE_CMI_SP_ID_B);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_state == FBE_CMI_STATE_ACTIVE);
	MUT_ASSERT_TRUE(cmi_info_spb.sp_state == FBE_CMI_STATE_PASSIVE);

	/*and one last combination is to take down the passive SP and see that active stay the same*/
	mut_printf(MUT_LOG_LOW, "=== danny_din_test UNLoading Config from SPB ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	fbe_test_sep_util_destroy_sep_physical();
	fbe_thread_delay(500);

	/*and see if SPA now thinks he is still the master*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_cmi_service_get_info(&cmi_info_spa);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(cmi_info_spa.peer_alive == FBE_FALSE);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_id == FBE_CMI_SP_ID_A);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_state == FBE_CMI_STATE_ACTIVE);

	fbe_thread_delay(2000);

	/*bring up spb so we can shut both of them down in the destroy function*/
	mut_printf(MUT_LOG_LOW, "=== danny_din_test Loading Config on SPB ===");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	elmo_physical_config();
	danny_din_dualsp_load_sep();

	fbe_thread_delay(1000);

	/*verify everything is what we want it to be*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	status = fbe_api_cmi_service_get_info(&cmi_info_spa);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	status = fbe_api_cmi_service_get_info(&cmi_info_spb);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(cmi_info_spa.peer_alive == FBE_TRUE);
	MUT_ASSERT_TRUE(cmi_info_spb.peer_alive == FBE_TRUE);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_id == FBE_CMI_SP_ID_A);
	MUT_ASSERT_TRUE(cmi_info_spb.sp_id == FBE_CMI_SP_ID_B);
	MUT_ASSERT_TRUE(cmi_info_spa.sp_state == FBE_CMI_STATE_ACTIVE);
	MUT_ASSERT_TRUE(cmi_info_spb.sp_state == FBE_CMI_STATE_PASSIVE);

	mut_printf(MUT_LOG_LOW, "=== danny_din_test Ended ACTIVE/PASSIVE Step 2 ===");

	/*return to default*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	return FBE_STATUS_OK;
}

static fbe_status_t danny_din_run_state_override_test(void)
{
	fbe_object_id_t			obj_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
	fbe_metadata_info_t		sp_a_metadata_info;

	mut_printf(MUT_LOG_LOW, "=== danny_din_test Starting state_override_test ===");

	/*after the last test, SPA supposed to be ACTIVE so let's try and set all objects on it to PASSIVE
	but before that we verify the state*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

	/*and verify at least one of the objects is active*/
	status =  danny_din_get_object_of_class(FBE_CLASS_ID_PROVISION_DRIVE, FBE_PACKAGE_ID_SEP_0, &obj_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status  = fbe_api_base_config_metadata_get_info(obj_id, &sp_a_metadata_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(sp_a_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);

	/*now turn all of them to passive*/
	status = fbe_api_metadata_set_all_objects_state(FBE_METADATA_ELEMENT_STATE_PASSIVE);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
    status  = fbe_api_base_config_metadata_get_info(obj_id, &sp_a_metadata_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(sp_a_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE);

    /*back to active*/
    status = fbe_api_metadata_set_single_object_state(obj_id, FBE_METADATA_ELEMENT_STATE_ACTIVE);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status  = fbe_api_base_config_metadata_get_info(obj_id, &sp_a_metadata_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	MUT_ASSERT_TRUE(sp_a_metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);

	mut_printf(MUT_LOG_LOW, "=== danny_din_test state_override_test ended successfully ===");

	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);/*return to default*/

	return FBE_STATUS_OK;


}

static fbe_status_t danny_din_run_scheduler_credits_interface_test(void)
{
	fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
	fbe_u32_t										c = 0;
	fbe_scheduler_set_credits_t						set_credits;
	fbe_scheduler_change_cpu_credits_all_cores_t	change_all;
	fbe_scheduler_change_cpu_credits_per_core_t		change_core;
	fbe_scheduler_change_memory_credits_t			change_memory;

	mut_printf(MUT_LOG_LOW, "=== danny_din_test Starting Scheduler credits API test ===");

	fbe_api_base_config_disable_all_bg_services();

	/*set some initial credits*/
	for (c = 0; c < FBE_SCHEDULER_MAX_CORES; c++) {
		set_credits.cpu_operations_per_second[c] = 100;
	}

	set_credits.mega_bytes_consumption = 1000;

	status = fbe_api_scheduler_set_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*and take them out*/
	change_all.cpu_operations_per_second = 20;
	status = fbe_api_scheduler_remove_cpu_credits_all_cores(&change_all);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*now let's read and see it was changed*/
	status = fbe_api_scheduler_get_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(set_credits.cpu_operations_per_second[0] == 80)

	/*add it back*/
	change_all.cpu_operations_per_second = 40;
	status = fbe_api_scheduler_add_cpu_credits_all_cores(&change_all);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*now let's read and see it was changed*/
	status = fbe_api_scheduler_get_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(set_credits.cpu_operations_per_second[1] == 120)

	/*operations per core*/
	change_core.core = 1;
	change_core.cpu_operations_per_second = 20;
	status = fbe_api_scheduler_remove_cpu_credits_per_core(&change_core);

	/*now let's read and see it was changed*/
	status = fbe_api_scheduler_get_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(set_credits.cpu_operations_per_second[1] == 100)

	/*operations per core*/
	change_core.core = 0;
	change_core.cpu_operations_per_second = 20;
	status = fbe_api_scheduler_add_cpu_credits_per_core(&change_core);

	/*now let's read and see it was changed*/
	status = fbe_api_scheduler_get_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(set_credits.cpu_operations_per_second[0] == 140)

	/*memory credits operations*/
	change_memory.mega_bytes_consumption_change = 200;
	status = fbe_api_scheduler_add_memory_credits(&change_memory);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*now let's read and see it was changed*/
	status = fbe_api_scheduler_get_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(set_credits.mega_bytes_consumption == 1200)

	/*memory credits operations*/
	change_memory.mega_bytes_consumption_change = 400;
	status = fbe_api_scheduler_remove_memory_credits(&change_memory);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*now let's read and see it was changed*/
	status = fbe_api_scheduler_get_credits(&set_credits);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(set_credits.mega_bytes_consumption == 800)

	fbe_api_base_config_enable_all_bg_services();
	mut_printf(MUT_LOG_LOW, "=== danny_din_test Ended Scheduler credits API test===");

	return FBE_STATUS_OK;
}

/****************************************************************************************************
 ***************************** Persist API test case ************************************************
 ****************************************************************************************************/
typedef struct read_context_s{
    fbe_semaphore_t           sem;
    fbe_persist_entry_id_t    next_entry_id;
    fbe_u32_t                 entries_read;
    fbe_u8_t                  buffer[FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE];
} read_context_t;
typedef struct single_entry_context_s{
    fbe_semaphore_t         sem;
    fbe_persist_entry_id_t  received_entry_id;
    fbe_u8_t                buffer[FBE_PERSIST_DATA_BYTES_PER_ENTRY];
    fbe_status_t            return_status;
}single_entry_context_t;

/**************************************************************
 * danny_din_persist_read_completion_function()
 ****************************************************************
 * @brief
 *  This is the completion function for sector read operation.
 *
 * @param    read_status - status of the read operation
 * @param     next_entry
 * @param....entries_read
 * @param    context       - completion context
 *
 *
 *@return fbe_status_t status of the operation.
 *
 * @revision
 *  01/08/2015: created: Jamin Kang
 *
 ****************************************************************/
static fbe_status_t danny_din_persist_read_completion_function(fbe_status_t read_status,
                                                               fbe_persist_entry_id_t next_entry,
                                                               fbe_u32_t entries_read,
                                                               fbe_persist_completion_context_t context)
{
    read_context_t * read_context = (read_context_t *)context;

    read_context->next_entry_id= next_entry;
    read_context->entries_read = entries_read;
    fbe_semaphore_release(&read_context->sem, 0, 1 ,FALSE);
    return FBE_STATUS_OK;
}

/**************************************************************
 * danny_din_persist_single_operation_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for single entry operation.
 *
 * @param    op_status - status of the operation
 * @param    entry_id
 * @param    context       - completion context
 *
 *
 *@return fbe_status_t status of the operation.
 *
 * @revision
 *  01/08/2015: created:  Jamin Kang
 *
 ****************************************************************/
static fbe_status_t danny_din_persist_single_operation_completion(fbe_status_t op_status,
                                                                  fbe_persist_entry_id_t entry_id,
                                                                  fbe_persist_completion_context_t context)
{
    single_entry_context_t *single_entry_context = (single_entry_context_t *)context;

    single_entry_context->return_status = op_status;
    single_entry_context->received_entry_id = entry_id;
    fbe_semaphore_release(&single_entry_context->sem, 0, 1 ,FALSE);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * danny_din_persist_read()
 ****************************************************************
 * @brief
 *  Test persist read API
 *
 * @param context: the context for read operation
 *
 * @return None.
 *
 * @author
 *  01/08/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void danny_din_persist_read(read_context_t *context)
{
    fbe_status_t status;
    fbe_status_t wait_status;

    memset(context, 0, sizeof(*context));
    fbe_semaphore_init(&context->sem, 0, 1);
    context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
    status = fbe_api_persist_read_sector(FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
                                         context->buffer,
                                         sizeof(context->buffer),
                                         context->next_entry_id,
                                         danny_din_persist_read_completion_function,
                                         (fbe_persist_completion_context_t)context);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK || status == FBE_STATUS_PENDING);
    wait_status = fbe_semaphore_wait_ms(&context->sem, 10000);
    MUT_ASSERT_TRUE(wait_status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(context->entries_read > 0);
    fbe_semaphore_destroy(&context->sem);
}

/*!**************************************************************
 * danny_din_persist_modify()
 ****************************************************************
 * @brief
 *  Modify an entry
 *
 * @param context: the context for modify operation
 *
 * @return None
 *
 * @author
 *  01/08/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void danny_din_persist_modify(single_entry_context_t *context, fbe_persist_entry_id_t entry_id)
{
    fbe_status_t status;
    fbe_status_t wait_status;

    fbe_semaphore_init(&context->sem, 0, 1);
    context->received_entry_id = entry_id;
    context->return_status = FBE_STATUS_GENERIC_FAILURE;
    status  = fbe_api_persist_modify_single_entry(context->received_entry_id,
                                                  context->buffer,
                                                  sizeof(context->buffer),
                                                  danny_din_persist_single_operation_completion,
                                                  context);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK || status == FBE_STATUS_PENDING);
    wait_status = fbe_semaphore_wait_ms(&context->sem, 10000);
    MUT_ASSERT_TRUE(wait_status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&context->sem);
}


/*!**************************************************************
 * danny_din_persist_read()
 ****************************************************************
 * @brief
 *  Test persist read API
 *
 * @param context: the context for read operation
 *
 * @return None.
 *
 * @author
 *  01/08/2015 - Created. Jamin Kang
 *
 ****************************************************************/
static void danny_din_persist_abort_test(void)
{
    fbe_persist_user_header_t *hdr;
    fbe_u8_t *data;
    read_context_t *context = malloc(sizeof(*context));
    single_entry_context_t *single_entry_context = malloc(sizeof(*single_entry_context));
    fbe_persist_sector_type_t               target_sector =  FBE_PERSIST_SECTOR_TYPE_INVALID;
    fbe_persist_entry_id_t                  entry = -1;


    /* Read the first persist entry */
    danny_din_persist_read(context);
    hdr = (fbe_persist_user_header_t *)&context->buffer[0];
    data = &context->buffer[sizeof(*hdr)];

    memcpy(&single_entry_context->buffer, data, sizeof(single_entry_context->buffer));
    mut_printf(MUT_LOG_TEST_STATUS, "=== Test Persist abort by modifying invalid entry id ===");
    /* Issue modify operation on invalid entry id. This will cause abort on persist service */
    entry = (((fbe_u64_t)target_sector) << 32) | entry;
    danny_din_persist_modify(single_entry_context, entry);
    MUT_ASSERT_TRUE(single_entry_context->return_status != FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Test Persist recover from abort state ===");
    danny_din_persist_modify(single_entry_context, hdr->entry_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, single_entry_context->return_status);
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);

    free(context);
    free(single_entry_context);
}

static void danny_din_run_persist_api_test(void)
{
	fbe_cmi_service_get_info_t spa_cmi_info;
	fbe_cmi_service_get_info_t spb_cmi_info;
	fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;
	fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_INVALID_SERVER;
	do {
		fbe_api_sleep(100);
		/* We have to figure out whitch SP is ACTIVE */
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
		fbe_api_cmi_service_get_info(&spa_cmi_info);
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		fbe_api_cmi_service_get_info(&spb_cmi_info);

		if(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_A;
			passive_connection_target = FBE_SIM_SP_B;
		}

		if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_B;
			passive_connection_target = FBE_SIM_SP_A;
		}
	}while(active_connection_target != FBE_SIM_SP_A && active_connection_target != FBE_SIM_SP_B);

    mut_printf(MUT_LOG_TEST_STATUS, "=== danny_din persist test on ACTIVE side (%s) ===",
               active_connection_target == FBE_SIM_SP_A ? "SP A" : "SP B");
	fbe_api_sim_transport_set_target_server(active_connection_target);
    fbe_test_sep_util_wait_for_database_service(30000);
    danny_din_persist_abort_test();
}
