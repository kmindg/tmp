/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * key_west.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Key West scenario.
 * 
 *  This scenario is for a 1 port, 3 enclosure configuration, aka the naxos.xml
 *  configuration.
 * 
 *  The test run in this case is:
 *	- 
 *	- 
 *
 * HISTORY
 *   02/14/2009:  Created. Arun Joseph
 *   11/23/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"

#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_package.h"
#include "pp_utils.h"
#include "fbe/fbe_notification_lib.h"


/* Enable/Disable additional tracing.*/
#define _KEY_WEST_BEBUG_TRACE_	1

#define FBE_ZEUS_KEY_WEST_MAX_DRIVES_PER_ENCLOSURE	25
#define FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES			 3
typedef struct key_west_drive_info_s{	
	fbe_u32_t						drive_enclosure_number;
	fbe_u32_t						drive_slot_number;	
	fbe_u32_t					    activate_pending_state_transition_count;
	fbe_u32_t					    ready_state_transition_count;
	fbe_api_terminator_device_handle_t  drive_handle;
	fbe_terminator_sas_drive_info_t drive_info;
}key_west_drive_info_t;
typedef struct key_west_enclosure_info_s{
	fbe_terminator_sas_encl_info_t enclosure_info;
	fbe_api_terminator_device_handle_t enclosure_handle;
	fbe_u32_t					   activate_pending_state_transition_count;
	fbe_u32_t					   ready_state_transition_count;
	key_west_drive_info_t		   drive_info_array[FBE_ZEUS_KEY_WEST_MAX_DRIVES_PER_ENCLOSURE];
}key_west_enclosure_info_t;

typedef struct key_west_ns_context_s
{
    fbe_semaphore_t sem;
}key_west_ns_context_t;

static key_west_enclosure_info_t  key_west_enclosure_info[FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES];
static fbe_notification_registration_id_t  reg_id;
/*************************
 *   MACRO DEFINITIONS
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void key_west_state_change_callback (update_object_msg_t *update_msg, VOID *context);
/****************************************************************
 * key_west_remove_insert_enclosure()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *
 ****************************************************************/

static fbe_status_t key_west_remove_insert_enclosure(fbe_u32_t port, fbe_u32_t encl,
                                                 fbe_u32_t max_drives,fbe_u32_t max_enclosures,fbe_u64_t drive_mask)
{
    fbe_status_t                                     status;
    fbe_api_terminator_device_handle_t      enclosure_handle;

    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Removing Enclosure %d", encl);

    /* Validate that the enclosure and all its drives are in READY state. */
    status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 10, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Retrieve info about the enclosure and drives.*/	
    status = fbe_api_terminator_get_enclosure_handle(port, encl, &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_terminator_force_logout_device(enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* a. Wait for the old enclosure to be removed.*/
    /* Note Enclosure will go in to glitch ride thriugh and hence trnasition to ACTIVATE state.*/
    status = naxos_verify (port, encl, FBE_LIFECYCLE_STATE_ACTIVATE, READY_STATE_WAIT_TIME, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_terminator_force_login_device(enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* b. Wait for a new drive object to be created.*/
    status = naxos_verify (port, encl, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_enclosure_handle(port, encl, &enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait 2.5 sec */
    EmcutilSleep(2500);

    /* Get the enclosure and its drives status (Verify if it is in READY state) */
    status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 5000, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, 
                "Key West Scenario: Removed Enclosure %d", encl);

    return FBE_STATUS_OK;
}


/****************************************************************
 * key_west_remove_insert_drives()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *
 ****************************************************************/

static fbe_status_t key_west_remove_insert_drives(fbe_u32_t port, fbe_u32_t encl,fbe_u32_t slot)
{
    fbe_status_t						status;
	fbe_api_terminator_device_handle_t      drive_handle,enclosure_handle;	

    key_west_ns_context_t           key_west_ns_context;
	fbe_u32_t						activate_pending_state_transition_count;
	fbe_u32_t					    ready_state_transition_count;


    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Removing Drive. Encl %d Slot %d", encl,slot);


    /* Validate that the enclosure is in READY state. */
    status = fbe_zrt_wait_enclosure_status(port, encl, FBE_LIFECYCLE_STATE_READY, 10);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Encl %d is READY.", encl);

	status = fbe_api_terminator_get_enclosure_handle(port, encl, &enclosure_handle);	
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Retrieve info about the drive.*/
    status = fbe_test_pp_util_verify_pdo_state(port, encl, slot,
                                               FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_LOW, "Encl %d Slot %d is READY.", encl,slot);

	status = fbe_api_terminator_get_drive_handle(port, encl, slot,&drive_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_zero_memory(&key_west_enclosure_info,sizeof(key_west_enclosure_info_t)*FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES);
	activate_pending_state_transition_count = 	ready_state_transition_count = 0;

    fbe_semaphore_init(&key_west_ns_context.sem, 0, FBE_SEMAPHORE_MAX);

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE |FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY, 
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  key_west_state_change_callback,
                                                                  &key_west_ns_context,
																  &reg_id);


	mut_printf(MUT_LOG_LOW, "Encl %d Slot %d is force logout.", encl,slot);
	status = fbe_api_terminator_force_logout_device(drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* a. Wait for the old drive to be removed.*/
    status = fbe_test_pp_util_verify_pdo_state(port, encl, slot,
                                                FBE_LIFECYCLE_STATE_ACTIVATE, READY_STATE_WAIT_TIME);
	if (status != FBE_STATUS_OK){
		mut_printf(MUT_LOG_LOW, "Encl %d Slot %d not in ACTIVATE state.", encl,slot);
		mut_printf(MUT_LOG_LOW, "ACTIVATE state transitions %d.", key_west_enclosure_info[encl].drive_info_array[slot].activate_pending_state_transition_count);
		mut_printf(MUT_LOG_LOW, "READY state transitions %d.",key_west_enclosure_info[encl].drive_info_array[slot].ready_state_transition_count );
	}
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Encl %d Slot %d is force login.", encl,slot);
	status = fbe_api_terminator_force_login_device(drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* b. Wait for a new drive object to be created.*/
    status = fbe_test_pp_util_verify_pdo_state(port, encl, slot,
                                               FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
	if (status != FBE_STATUS_OK){
		mut_printf(MUT_LOG_LOW, "Encl %d Slot %d not in ACTIVATE state.", encl,slot);
		mut_printf(MUT_LOG_LOW, "ACTIVATE state transitions %d.", key_west_enclosure_info[encl].drive_info_array[slot].activate_pending_state_transition_count);
		mut_printf(MUT_LOG_LOW, "READY state transitions %d.",key_west_enclosure_info[encl].drive_info_array[slot].ready_state_transition_count );
	}
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_drive_handle(port, encl, slot,&drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_wait(&key_west_ns_context.sem, NULL);
	status = fbe_api_notification_interface_unregister_notification(key_west_state_change_callback, reg_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "ACTIVATE state transitions %d.", key_west_enclosure_info[encl].drive_info_array[slot].activate_pending_state_transition_count);
	mut_printf(MUT_LOG_LOW, "READY state transitions %d.",key_west_enclosure_info[encl].drive_info_array[slot].ready_state_transition_count );
	/* Confirm that the device went through a READY -> ACTIVATE -> READY transition.*/
	MUT_ASSERT_TRUE((key_west_enclosure_info[encl].drive_info_array[slot].activate_pending_state_transition_count >  activate_pending_state_transition_count));
	MUT_ASSERT_TRUE((key_west_enclosure_info[encl].drive_info_array[slot].ready_state_transition_count > ready_state_transition_count));     

    mut_printf(MUT_LOG_LOW, 
                "Key West Scenario: Re-inserted drive Encl %d Slot %d", encl,slot);

    fbe_semaphore_destroy(&key_west_ns_context.sem); /* SAFEBUG - needs destroy */

    return FBE_STATUS_OK;
}

/****************************************************************
 * key_west_remove_insert_drive_test()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *
 ****************************************************************/

static fbe_status_t key_west_remove_insert_drive_test(fbe_u32_t port,fbe_u32_t max_drives,fbe_u32_t max_enclosures,fbe_u64_t drive_mask)
{
    fbe_status_t       status;
    fbe_u32_t           encl;


    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Starting drive remove re-insert test");

    /* Validate that the enclosure and all its drives are in READY state. */
    for (encl = 0; encl < FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES; encl++){
        status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 10, max_drives, drive_mask, max_enclosures);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }


    key_west_remove_insert_drives(0, 2, 4);
    key_west_remove_insert_drives(0, 0, 0);
    key_west_remove_insert_drives(0, 1, 7);
    key_west_remove_insert_drives(0, 2, 2);
    key_west_remove_insert_drives(0, 1, 5);
    key_west_remove_insert_drives(0, 0, 11);

    EmcutilSleep(5000);
    /* Get the enclosure and its drives status (Verify if it is in READY state) */
    for (encl = 0; encl < FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES; encl++){
        status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 5000, max_drives, drive_mask, max_enclosures);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Finished drive remove re-insert test");

    return FBE_STATUS_OK;
}

/****************************************************************
 * key_west_update_drive_entry_verify()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *
 ****************************************************************/

static fbe_status_t key_west_update_drive_entry_verify(fbe_u32_t port, fbe_u32_t encl,fbe_u32_t slot, fbe_u32_t max_drives,fbe_u32_t max_enclosures,fbe_u64_t drive_mask)
{
    fbe_status_t             status;
    fbe_api_terminator_device_handle_t  drive_handle,enclosure_handle;

    key_west_ns_context_t           key_west_ns_context;
    fbe_u32_t                             activate_pending_state_transition_count;
    fbe_u32_t                             ready_state_transition_count;
    
    fbe_miniport_device_id_t		cpd_device_id;

    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Removing Drive. Encl %d Slot %d", encl,slot);

    /* Validate that the enclosure and all its drives are in READY state. */
    status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 10, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_terminator_get_enclosure_handle(port, encl, &enclosure_handle);	
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Retrieve info about the drive.*/
	status = fbe_api_terminator_get_drive_handle(port, encl, slot,&drive_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get current cpd ID of drive. */
	status = fbe_api_terminator_get_device_cpd_device_id(drive_handle, &cpd_device_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	fbe_zero_memory(&key_west_enclosure_info,sizeof(key_west_enclosure_info_t)*FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES);
	activate_pending_state_transition_count = 	ready_state_transition_count = 0;

    fbe_semaphore_init(&key_west_ns_context.sem, 0, 1);

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE |FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY, 
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  key_west_state_change_callback,
                                                                  &key_west_ns_context,
																  &reg_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    status = fbe_api_terminator_reserve_miniport_sas_device_table_index(0, cpd_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_api_terminator_miniport_sas_device_table_force_remove(drive_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /* Add the terminator entry of drive with new cpd device ID.*/
    status = fbe_api_terminator_miniport_sas_device_table_force_add(drive_handle, cpd_device_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Force a log in of the new terminator entry for the drive.*/
    /* This will be seen as a new log in for the drive location with a different cpd device ID.*/
    /* This should trigger a log out of the old device and the log in of a new drive in that slot.*/
	status = fbe_api_terminator_force_login_device(drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Confirm that the device went through a READY -> ACTIVATE -> READY transition.*/
    
    /* b. Wait for a new drive object to be created.*/
    status = fbe_test_pp_util_verify_pdo_state(port, encl, slot,
                                               FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_drive_handle(port, encl, slot,&drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Confirm that the device went through a READY -> ACTIVATE -> READY transition.*/
	fbe_semaphore_wait(&key_west_ns_context.sem, NULL);

	MUT_ASSERT_TRUE((key_west_enclosure_info[encl].drive_info_array[slot].activate_pending_state_transition_count >
						activate_pending_state_transition_count));

	MUT_ASSERT_TRUE((key_west_enclosure_info[encl].drive_info_array[slot].ready_state_transition_count >
						ready_state_transition_count));
     /**/

	status = fbe_api_notification_interface_unregister_notification(key_west_state_change_callback, reg_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&key_west_ns_context.sem);
    mut_printf(MUT_LOG_LOW, 
                "Key West Scenario: Re-inserted drive Encl %d Slot %d", encl,slot);

    return FBE_STATUS_OK;
}

/****************************************************************
 * key_west_update_enclosure_entry_verify()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *
 ****************************************************************/

static fbe_status_t key_west_update_enclosure_entry_verify(fbe_u32_t port, fbe_u32_t encl,fbe_u32_t max_drives,fbe_u32_t max_enclosures,fbe_u64_t drive_mask)
{
    fbe_status_t					status;
	fbe_api_terminator_device_handle_t  enclosure_handle;
	fbe_u32_t						activate_pending_state_transition_count;
	fbe_u32_t					    ready_state_transition_count,slot;
	fbe_miniport_device_id_t		cpd_device_id;
	fbe_api_terminator_device_handle_t	drive_handle_array[FBE_ZEUS_KEY_WEST_MAX_DRIVES_PER_ENCLOSURE];

    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Removing Enclosure. Encl %d ",encl);

    /* Validate that the enclosure and all its drives are in READY state. */
    status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 10, max_drives, drive_mask, max_enclosures);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_terminator_get_enclosure_handle(port, encl, &enclosure_handle);	
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	activate_pending_state_transition_count = key_west_enclosure_info[encl].activate_pending_state_transition_count;
	ready_state_transition_count = key_west_enclosure_info[encl].ready_state_transition_count;

	for(slot = 0; slot < FBE_ZEUS_KEY_WEST_MAX_DRIVES_PER_ENCLOSURE; slot++){
		status = fbe_test_pp_util_verify_pdo_state(port, encl, slot,
										FBE_LIFECYCLE_STATE_READY, 10);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

		
		/* Retrieve info about the drive.*/
		status = fbe_api_terminator_get_drive_handle(port, encl, slot,&(drive_handle_array[slot]));
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

		status = fbe_api_terminator_get_device_cpd_device_id(drive_handle_array[slot], &cpd_device_id);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

		status = fbe_api_terminator_reserve_miniport_sas_device_table_index(0, cpd_device_id);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    
		

		status = fbe_api_terminator_miniport_sas_device_table_force_remove(drive_handle_array[slot]);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	    
		status = fbe_api_terminator_miniport_sas_device_table_force_add(drive_handle_array[slot], cpd_device_id);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	}

	status = fbe_api_terminator_get_device_cpd_device_id(enclosure_handle, &cpd_device_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_reserve_miniport_sas_device_table_index(0, cpd_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);	

    status = fbe_api_terminator_miniport_sas_device_table_force_remove(enclosure_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    status = fbe_api_terminator_miniport_sas_device_table_force_add(enclosure_handle, cpd_device_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_terminator_force_login_device(enclosure_handle);

	EmcutilSleep(5000);

    status = fbe_api_terminator_get_enclosure_handle(port, encl,&enclosure_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* Confirm that the device went through a READY -> ACTIVATE -> READY transition.*/
	
	MUT_ASSERT_TRUE((key_west_enclosure_info[encl].activate_pending_state_transition_count ==
						(activate_pending_state_transition_count + 1)));

	MUT_ASSERT_TRUE((key_west_enclosure_info[encl].ready_state_transition_count ==
						(ready_state_transition_count + 1)));
						


    mut_printf(MUT_LOG_LOW, 
                "Key West Scenario: Re-inserted Encl %d ", encl);

    return FBE_STATUS_OK;
}

/****************************************************************
 * key_west_dev_table_update_test()
 ****************************************************************
 * Description:
 *  Function to remove an enclosure. This function calls
 * 
 *  
 *  naxos_verify to check:
 *  - The enclosure and all its drives transition to READY state.
 *
 * Return - FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *
 ****************************************************************/

static fbe_status_t key_west_dev_table_update_test(fbe_u32_t port,fbe_u32_t max_drives,fbe_u32_t max_enclosures,fbe_u64_t drive_mask)
{
    fbe_status_t       status;
    fbe_u32_t           encl;


    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Starting drive remove re-insert test");

    /* Validate that the enclosure and all its drives are in READY state. */
    for (encl = 0; encl < FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES; encl++){
        status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 10, max_drives, drive_mask, max_enclosures);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    key_west_update_drive_entry_verify(0, 2, 4, max_drives, max_enclosures, drive_mask);
    key_west_update_drive_entry_verify(0, 0, 0, max_drives, max_enclosures, drive_mask);
    key_west_update_drive_entry_verify(0, 1, 7, max_drives, max_enclosures, drive_mask);
    key_west_update_drive_entry_verify(0, 2, 2, max_drives, max_enclosures, drive_mask);
    key_west_update_drive_entry_verify(0, 1, 5, max_drives, max_enclosures, drive_mask);
    key_west_update_drive_entry_verify(0, 0, 11, max_drives, max_enclosures, drive_mask);

    EmcutilSleep(5000);
    /* Get the enclosure and its drives status (Verify if it is in READY state) */
    for (encl = 0; encl < FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES; encl++){
        status = naxos_verify(port, encl, FBE_LIFECYCLE_STATE_READY, 5000, max_drives, drive_mask, max_enclosures);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_LOW,
                "Key West Scenario: Finished drive remove re-insert test");

    return FBE_STATUS_OK;
}

static void key_west_state_change_callback (update_object_msg_t *update_msg, VOID *context)
{

    fbe_u32_t encl,drive;
    fbe_u32_t port;
    fbe_topology_object_type_t  object_type;
    fbe_status_t                status;
    key_west_ns_context_t       *ns_context = (key_west_ns_context_t *)context;
	fbe_lifecycle_state_t 		state;


    MUT_ASSERT_TRUE(context != NULL);
#if _KEY_WEST_BEBUG_TRACE_
    mut_printf(MUT_LOG_LOW,
        "Key West Scenario: %s called",__FUNCTION__);	
#endif

    if (update_msg->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE){

        status = fbe_api_get_object_port_number (update_msg->object_id, &port);
        status = fbe_api_get_object_enclosure_number (update_msg->object_id, &encl);
        status = fbe_api_get_object_drive_number (update_msg->object_id, &drive);
        status = fbe_api_get_object_type(update_msg->object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);

		status = fbe_notification_convert_notification_type_to_state(update_msg->notification_info.notification_type,
																	 &state);

        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

#if _KEY_WEST_BEBUG_TRACE_

	mut_printf(MUT_LOG_LOW,
		"Key West Scenario: Notification type %llu. Life cycle state %d",
		(unsigned long long)update_msg->notification_info.notification_type,
                state);	
	mut_printf(MUT_LOG_LOW,
		"Key West Scenario: Port %d Encl %d Drive %d", port, encl, drive);	
#endif

		if (object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE){
			MUT_ASSERT_TRUE((port == 0) &&
							(encl < FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES) &&
							(drive < FBE_ZEUS_KEY_WEST_MAX_DRIVES_PER_ENCLOSURE));

			/* ASSERT above should be sufficient.. But to make coverity happy...*/
			if((port != 0) || (encl >= FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES) ||
				(drive >= FBE_ZEUS_KEY_WEST_MAX_DRIVES_PER_ENCLOSURE)){
#if _KEY_WEST_BEBUG_TRACE_
					mut_printf(MUT_LOG_LOW,
						"Key West Scenario: Invalid drive info. Encl %d Slot %d",encl,drive);	
#endif
					return;
				}


			if (state == FBE_LIFECYCLE_STATE_ACTIVATE){
				key_west_enclosure_info[encl].drive_info_array[drive].activate_pending_state_transition_count++;
#if _KEY_WEST_BEBUG_TRACE_
				mut_printf(MUT_LOG_LOW,
					"Key West Scenario: State Change. PENDING_ACTIVATE. Encl %d Slot %d",encl,drive);	
#endif
			}
			if (state == FBE_LIFECYCLE_STATE_READY){
				key_west_enclosure_info[encl].drive_info_array[drive].ready_state_transition_count++;
                fbe_semaphore_release(&(ns_context->sem), 0, 1 ,FALSE);
#if _KEY_WEST_BEBUG_TRACE_
				mut_printf(MUT_LOG_LOW,
					"Key West Scenario: State Change. READY. Encl %d Slot %d",encl,drive);	
#endif
			}
		}else if (object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE){
				MUT_ASSERT_TRUE((port == 0) &&
								(encl < FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES));

			/* ASSERT above should be sufficient.. But to make coverity happy...*/
			if((port != 0) || (encl >= FBE_ZEUS_KEY_WEST_MAX_ENCLOSURES)){
#if _KEY_WEST_BEBUG_TRACE_
					mut_printf(MUT_LOG_LOW,
						"Key West Scenario: Invalid enclosure info. Encl %d ",encl);	
#endif
					return;
				}

				if (state == FBE_LIFECYCLE_STATE_ACTIVATE){
					key_west_enclosure_info[encl].activate_pending_state_transition_count++;
#if _KEY_WEST_BEBUG_TRACE_
				mut_printf(MUT_LOG_LOW,
					"Key West Scenario: State Change. PENDING_ACTIVATE. Encl %d ",encl);	
#endif
				}
				if (state == FBE_LIFECYCLE_STATE_READY){
					key_west_enclosure_info[encl].ready_state_transition_count++;
#if _KEY_WEST_BEBUG_TRACE_
				mut_printf(MUT_LOG_LOW,
					"Key West Scenario: State Change. READY. Encl %d ",encl);	
#endif

				}
		}
	}
	
	return;
}



/*!***************************************************************
 * @fn key_west_run()
 ****************************************************************
 * @brief
 *  This scenario tests 
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 *
 ****************************************************************/

static fbe_status_t key_west_run(fbe_test_params_t *test)
{
    fbe_status_t    status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "Key WestScenario: Start");


    /* Step 1: Insert and remove enclosure quickly and verify whether the topolgy is sane.*/
    if(test->encl_type != FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)
    {
        /* Remove enclosure, immediately add it back. Check whether the topology is changed accordingly.*/
        status = key_west_remove_insert_enclosure(test->backend_number, 2, test->max_drives, test->max_enclosures,test->drive_mask);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
        EmcutilSleep(1500);
        status = key_west_remove_insert_enclosure(test->backend_number, 2,test->max_drives,test->max_enclosures,test->drive_mask);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
        /* Remove 6 drives from different locations.*/
        /* Add them back immediately.*/
        /* Verify that the cpd_id and the topology are updated.*/
        key_west_remove_insert_drive_test(test->backend_number,test->max_drives,test->max_enclosures,test->drive_mask);
        
        key_west_dev_table_update_test(test->backend_number,test->max_drives,test->max_enclosures,test->drive_mask);
    }
    mut_printf(MUT_LOG_LOW, "Key West Scenario: End");

    return status;
}

/*!***************************************************************
 * @fn key_west()
 ****************************************************************
 * @brief:
 *  Function to load, unload the config and run the 
 *  Bora Bora scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 *
 ****************************************************************/
void key_west(void)
{
    fbe_status_t run_status = FBE_STATUS_OK;
    fbe_u32_t test_num;
    fbe_test_params_t *key_west_test; 
    fbe_u32_t max_entries ;

    max_entries = fbe_test_get_naxos_num_of_tests() ;

    for(test_num = 0; test_num < max_entries; test_num++)
    {
        /* Load the configuration for test
        */
        key_west_test = fbe_test_get_naxos_test_table(test_num) ; 
        naxos_load_and_verify_table_driven(key_west_test);
        run_status = key_west_run(key_west_test);
        MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}

