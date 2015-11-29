
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file scooter_test.c
 ***************************************************************************
 *
 * @brief
 *   This file includes tests for testing extent pool. 
 *
 * @version
 *   06/13/2014 - Created.  Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_random.h"

#include "fbe_test_configurations.h"
#include "fbe_test_common_utils.h"


char * scooter_short_desc = "tests extent pool functionalities";
char * scooter_long_desc =
    "\n"
    "\n"
    "tests extent pool functionalities.\n"
    "\n"
    "tests extent pool, extent pool lun and extent pool metadata lun\n "
    "\n";

/*************************
 *  DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def SCOOTER_TEST_EXTENT_POOL_COUNT
 *********************************************************************
 * @brief Max number of POOLs to create
 *
 *********************************************************************/
#define SCOOTER_TEST_EXTENT_POOL_COUNT           1

/*!*******************************************************************
 * @def SCOOTER_TEST_EXTENT_POOL_LUN_COUNT
 *********************************************************************
 * @brief Max number of POOL LUNs to create
 *
 *********************************************************************/
#define SCOOTER_TEST_EXTENT_POOL_LUN_COUNT       3

/*!*******************************************************************
 * @def SCOOTER_TEST_EXTENT_POOL_ID
 *********************************************************************
 * @brief pool ID of the pool
 *
 *********************************************************************/
#define SCOOTER_TEST_EXTENT_POOL_ID              0x0

/*!*******************************************************************
 * @def SCOOTER_TEST_EXTENT_POOL_DRIVE_COUNT
 *********************************************************************
 * @brief drive count in the pool
 *
 *********************************************************************/
#define SCOOTER_TEST_EXTENT_POOL_DRIVE_COUNT     40

/*!*******************************************************************
 * @def SCOOTER_TEST_EXTENT_POOL_LUN_CAPACITY
 *********************************************************************
 * @brief extent pool lun capacity
 *
 *********************************************************************/
#define SCOOTER_TEST_EXTENT_POOL_LUN_CAPACITY    0x10000


/*************************
 *  FORWARD DECLARATIONS
 *************************/



/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * scooter_test_create_extent_pool()
 ****************************************************************
 * @brief
 *  This function create an exent pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
scooter_test_create_extent_pool(fbe_u32_t pool_id, fbe_u32_t drive_count, fbe_drive_type_t drive_type)
{
    fbe_api_job_service_create_extent_pool_t create_pool;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"scooter_test: creating extent pool 0x%x\n", pool_id);

    create_pool.drive_count = drive_count;
    create_pool.drive_type = drive_type;
    create_pool.pool_id = pool_id;
	status = fbe_api_job_service_create_extent_pool(&create_pool);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end scooter_test_create_extent_pool()
 ******************************************/

/*!**************************************************************
 * scooter_test_create_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function create an exent pool lun.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
scooter_test_create_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t lun_id, fbe_lba_t capacity)
{
    fbe_api_job_service_create_ext_pool_lun_t create_lun;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"scooter_test: creating extent pool lun 0x%x\n", lun_id);

    create_lun.lun_id = lun_id;
    create_lun.pool_id = pool_id;
    create_lun.capacity = capacity;
	status = fbe_api_job_service_create_ext_pool_lun(&create_lun);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end scooter_test_create_extent_pool_lun()
 ******************************************/

/*!**************************************************************
 * scooter_test_check_extent_pool()
 ****************************************************************
 * @brief
 *  This function checks an exent pool after creation.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
scooter_test_check_extent_pool(fbe_u32_t pool_id)
{
    fbe_status_t status;
    fbe_object_id_t pool_object_id;

    status = fbe_api_database_lookup_ext_pool_by_number(pool_id, &pool_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pool_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end scooter_test_check_extent_pool()
 ******************************************/

/*!**************************************************************
 * scooter_test_check_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function checks an exent pool lun after creation.
 *
 * @param lun_id - lun id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
scooter_test_check_extent_pool_lun(fbe_u32_t lun_id)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;

    status = fbe_api_database_lookup_ext_pool_lun_by_number(lun_id, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end scooter_test_check_extent_pool_lun()
 ******************************************/

/*!**************************************************************
 * scooter_test_destroy_extent_pool()
 ****************************************************************
 * @brief
 *  This function destroy an exent pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
scooter_test_destroy_extent_pool(fbe_u32_t pool_id)
{
    fbe_api_job_service_destroy_extent_pool_t destroy_pool;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"scooter_test: destroying extent pool 0x%x\n", pool_id);

    destroy_pool.pool_id = pool_id;
	status = fbe_api_job_service_destroy_extent_pool(&destroy_pool);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end scooter_test_destroy_extent_pool()
 ******************************************/

/*!**************************************************************
 * scooter_test_destroy_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function destroy an exent pool lun.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void 
scooter_test_destroy_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t lun_id)
{
    fbe_api_job_service_destroy_ext_pool_lun_t destroy_lun;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"scooter_test: destroying extent pool lun 0x%x\n", lun_id);

    destroy_lun.lun_id = lun_id;
    destroy_lun.pool_id = pool_id;
	status = fbe_api_job_service_destroy_ext_pool_lun(&destroy_lun);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end scooter_test_create_extent_pool_lun()
 ******************************************/


/*!**************************************************************
 * scooter_stripe_lock_completion()
 ****************************************************************
 * @brief
 *  Completion function for sending usurper to stripe lock service.
 *
 * @param packet - Pointer to packet.               
 *
 * @return fbe_status_t.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
scooter_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t *)completion_context;
	fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}
/******************************************
 * end scooter_stripe_lock_completion()
 ******************************************/

/*!**************************************************************
 * scooter_stripe_lock_test()
 ****************************************************************
 * @brief
 *  This function tests dualsp stripe locks for ext pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void scooter_stripe_lock_test(fbe_u32_t pool_id)
{
	fbe_status_t status;
	fbe_u32_t i;
    fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t passive_connection_target = FBE_SIM_SP_B;

	fbe_api_metadata_debug_lock_t debug_lock[4];
	fbe_packet_t * packet[4];
	fbe_semaphore_t sem;
    fbe_object_id_t pool_object_id;

    status = fbe_api_database_lookup_ext_pool_by_number(pool_id, &pool_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Stripe Lock Test Start ... \n");

	fbe_semaphore_init(&sem, 0, 1);

	for(i = 0; i < 4; i++){
		packet[i] = malloc(sizeof(fbe_packet_t));
		fbe_transport_initialize_sep_packet(packet[i]);
	}

	mut_printf(MUT_LOG_LOW, "We will work with ext pool %d \n", pool_object_id);

	fbe_api_sim_transport_set_target_server(active_connection_target);

	for(i = 0 ; i < 4;i++){
		debug_lock[i].object_id = pool_object_id;
		debug_lock[i].b_allow_hold = FBE_TRUE; 
		debug_lock[i].packet_ptr = NULL;
		debug_lock[i].stripe_number = 0;		
		debug_lock[i].stripe_count = 1;
	}

	mut_printf(MUT_LOG_LOW, "Step 1 (Send write lock on active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	/* Send write lock on active side */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
	
    fbe_transport_set_completion_function(packet[0], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 2 (Send another write lock to passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	/* Send another write lock and see that it is stuck */
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
	debug_lock[1].stripe_count = 2;

    fbe_transport_set_completion_function(packet[1], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_LOW, "Step 3 (Send write unlock to active side) ... \n");

	fbe_api_sim_transport_set_target_server(active_connection_target);

	/* Send write unlock */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
    fbe_transport_set_completion_function(packet[0], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 4 (Wait for completion of second write lock) ... \n");
	/* Wait for completion of second write lock */
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[1].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 5 (Send second write unlock on passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	/* Send second write unlock */
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
    fbe_transport_set_completion_function(packet[1], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

#if 1 /* Shared dual SP lock is temporarily disabled */
	mut_printf(MUT_LOG_LOW, "Step 6 (Send read lock on active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK;		
    fbe_transport_set_completion_function(packet[0], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);


	mut_printf(MUT_LOG_LOW, "Step 7 (Send another read lock to passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK;			
    fbe_transport_set_completion_function(packet[1], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[1].packet_ptr != NULL);


	mut_printf(MUT_LOG_LOW, "Step 8 (Send read unlock to active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK;
    fbe_transport_set_completion_function(packet[0], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 9 (Send write lock to active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	/* Send write lock and see that it is stuck */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
    fbe_transport_set_completion_function(packet[0], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);


	mut_printf(MUT_LOG_LOW, "Step 10 (Send read unlock to passive side) ... \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK;			
    fbe_transport_set_completion_function(packet[1], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


	mut_printf(MUT_LOG_LOW, "Step 11 (Wait for completion of write lock) ... \n");
	/* Wait for completion of second write lock */
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 12 (Send write unlock to active side) ... \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;		
    fbe_transport_set_completion_function(packet[0], scooter_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_ext_pool_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif
	for(i = 0; i < 4; i++){
		fbe_transport_destroy_packet(packet[i]);
		free(packet[i]);
	}

    fbe_semaphore_destroy(&sem); /* SAFEBUG - needs destroy */
	
	mut_printf(MUT_LOG_LOW, "Stripe Lock Test Test Completed\n");

}
/******************************************
 * end scooter_stripe_lock_test()
 ******************************************/

/*!**************************************************************
 * scooter_api_test()
 ****************************************************************
 * @brief
 *  This function tests RG api for ext pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  06/13/2014 - Created. Lili Chen
 *
 ****************************************************************/
static void scooter_api_test(fbe_u32_t pool_id)
{
    fbe_status_t status;
    fbe_object_id_t pool_object_id;
    fbe_u32_t total_rgs, returned_rgs;
    fbe_database_raid_group_info_t *    rg_info;
    fbe_u32_t total_rgs_expected = fbe_private_space_layout_get_number_of_system_raid_groups() + 1;
    fbe_u32_t                           total_luns, returned_luns;
    fbe_database_lun_info_t *           lun_info;
    fbe_api_lun_create_t                lun_create_job;
    fbe_api_lun_destroy_t               lun_destroy_job;
    fbe_object_id_t                     object_id;
    fbe_job_service_error_type_t        job_error_type;

    status = fbe_api_database_lookup_raid_group_by_number(pool_id, &pool_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects_of_all_raid_groups(FBE_PACKAGE_ID_SEP_0, &total_rgs);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to enumerate all raid group class!!!"); 
    MUT_ASSERT_INT_EQUAL_MSG(total_rgs, total_rgs_expected, "failed to find extent pool!!!"); 

    rg_info = (fbe_database_raid_group_info_t *)fbe_api_allocate_memory(total_rgs * sizeof(fbe_database_raid_group_info_t));
    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(rg_info, NULL, "failed to allocate memory!"); 
    fbe_zero_memory(rg_info, total_rgs * sizeof(fbe_database_raid_group_info_t));
    status = fbe_api_database_get_all_raid_groups(rg_info, total_rgs, &returned_rgs);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get all raid groups"); 
    MUT_ASSERT_INT_EQUAL_MSG(returned_rgs, total_rgs, "raid group count mismatch"); 

    fbe_api_free_memory(rg_info);

    /* Test the interface that is getting all the luns in one shot */
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to enumerate lun class!!!"); 
    lun_info = (fbe_database_lun_info_t *)fbe_api_allocate_memory(total_luns * sizeof(fbe_database_lun_info_t));
    MUT_ASSERT_POINTER_NOT_EQUAL_MSG(lun_info, NULL, "failed to allocate memory!"); 
    fbe_zero_memory(lun_info, total_luns * sizeof(fbe_database_lun_info_t));
    status = fbe_api_database_get_all_luns(lun_info, total_luns, &returned_luns);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "failed to get all luns"); 
    MUT_ASSERT_INT_EQUAL_MSG(returned_luns, total_luns, "lun count mismatch"); 
    MUT_ASSERT_INT_EQUAL_MSG(lun_info->lifecycle_state, FBE_LIFECYCLE_STATE_READY,"lun lifecycle mismatch"); /*lifecycle for the first lun is the list*/

    fbe_api_free_memory(lun_info);

    /* Create a lun using lun interface */
    lun_create_job.lun_number = 0x10;
    lun_create_job.raid_group_id = SCOOTER_TEST_EXTENT_POOL_ID;
    lun_create_job.capacity = 0x10000;
    lun_create_job.world_wide_name.bytes[0] = fbe_random() & 0xf;
    lun_create_job.world_wide_name.bytes[1] = fbe_random() & 0xf;
    status = fbe_api_create_lun(&lun_create_job, FBE_TRUE, 30000, &object_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_EXTENT_POOL_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(total_luns, SCOOTER_TEST_EXTENT_POOL_LUN_COUNT + 1);

    fbe_api_sleep(2000);
    /* Destroy a lun using lun interface */
    lun_destroy_job.lun_number = 0x10;
    status = fbe_api_destroy_lun(&lun_destroy_job, FBE_TRUE, 30000, &job_error_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_EXTENT_POOL_LUN, FBE_PACKAGE_ID_SEP_0, &total_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(total_luns, SCOOTER_TEST_EXTENT_POOL_LUN_COUNT);

}
/******************************************
 * end scooter_api_test()
 ******************************************/

/*!****************************************************************************
 *  scooter_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the scooter test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooter_test_config(void)
{
    fbe_status_t status;
    fbe_u32_t i;

    /* Create extent pool */
    scooter_test_create_extent_pool(SCOOTER_TEST_EXTENT_POOL_ID, SCOOTER_TEST_EXTENT_POOL_DRIVE_COUNT, FBE_DRIVE_TYPE_INVALID);
    scooter_test_check_extent_pool(SCOOTER_TEST_EXTENT_POOL_ID);

    /* Create extent pool luns */
    for (i = 0; i < SCOOTER_TEST_EXTENT_POOL_LUN_COUNT; i++) 
    {
        scooter_test_create_extent_pool_lun(SCOOTER_TEST_EXTENT_POOL_ID, i+1, SCOOTER_TEST_EXTENT_POOL_LUN_CAPACITY);
        scooter_test_check_extent_pool_lun(i+1);
    }

    scooter_api_test(SCOOTER_TEST_EXTENT_POOL_ID);

    fbe_api_sleep(2000);
    if(fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_test_common_reboot_both_sps();
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    } else {
        fbe_test_sep_util_destroy_neit_sep();
        sep_config_load_sep_and_neit();
        status = fbe_test_sep_util_wait_for_database_service(120000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_api_database_set_load_balance(FBE_TRUE);
        fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
        fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);
    }

    scooter_test_check_extent_pool(SCOOTER_TEST_EXTENT_POOL_ID);
    for (i = 0; i < SCOOTER_TEST_EXTENT_POOL_LUN_COUNT; i++) 
    {
        scooter_test_check_extent_pool_lun(i+1);
    }

    if(fbe_test_sep_util_get_dualsp_test_mode())
    {
        scooter_stripe_lock_test(SCOOTER_TEST_EXTENT_POOL_ID);
    }

    /* Destroy ext pool and luns */
    for (i = 0; i < SCOOTER_TEST_EXTENT_POOL_LUN_COUNT; i++) 
    {
        scooter_test_destroy_extent_pool_lun(SCOOTER_TEST_EXTENT_POOL_ID, i+1);
    }
    scooter_test_destroy_extent_pool(SCOOTER_TEST_EXTENT_POOL_ID);

    fbe_api_sleep(2000);
    /* Create again */
    scooter_test_create_extent_pool(SCOOTER_TEST_EXTENT_POOL_ID, SCOOTER_TEST_EXTENT_POOL_DRIVE_COUNT, FBE_DRIVE_TYPE_INVALID);
    scooter_test_check_extent_pool(SCOOTER_TEST_EXTENT_POOL_ID);
    for (i = 0; i < SCOOTER_TEST_EXTENT_POOL_LUN_COUNT; i++) 
    {
        scooter_test_create_extent_pool_lun(SCOOTER_TEST_EXTENT_POOL_ID, i+1, SCOOTER_TEST_EXTENT_POOL_LUN_CAPACITY);
        scooter_test_check_extent_pool_lun(i+1);
    }

    return;
} /* End scooter_test() */

/*!****************************************************************************
 *  scooter_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the scooter test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooter_test(void)
{
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    scooter_test_config();

    return;
} /* End scooter_test() */

/*!****************************************************************************
 *  scooter_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the scooter test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooter_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for scooter test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    elmo_physical_config();

    /* Setup the slice size to be appropriate for simulation.
     */
    fbe_test_set_ext_pool_slice_blocks_sim();
    
    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    /* If we don't wait for the PVDs to be ready then we will fail during reads.
     */
    fbe_test_wait_for_all_pvds_ready();
    return;

} /* End scooter_setup() */


/*!****************************************************************************
 *  scooter_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Shrek test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooter_cleanup(void)
{  
    /* Give sometime to let the object go away */
    EmcutilSleep(1000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for scooter test ===\n");
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;

} /* End scooter_cleanup() */


/*!**************************************************************
 * scooter_dualsp_test()
 ****************************************************************
 * @brief
 *  Run extent pool dualsp test.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  6/18/2014 - Created. Lili Chen
 *
 ****************************************************************/

void scooter_dualsp_test(void)
{
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    scooter_test_config();

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
} /* End scooter_dualsp_test() */

/*!****************************************************************************
 *  scooter_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the scooter test. It is responsible
 *  for loading the physical and logical configuration for this test suite.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooter_dualsp_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for scooter test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    elmo_physical_config();

    /* Setup the slice size to be appropriate for simulation.
     */
    fbe_test_set_ext_pool_slice_blocks_sim();
    
    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* Load the physical configuration */
    elmo_physical_config();

    /* Setup the slice size to be appropriate for simulation.
     */
    fbe_test_set_ext_pool_slice_blocks_sim();
    
    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* If we don't wait for the PVDs to be ready then we will fail during reads.
     */
    fbe_test_wait_for_all_pvds_ready();
    return;

} /* End scooter_dualsp_setup() */


/*!****************************************************************************
 *  scooter_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Shrek test.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooter_dualsp_cleanup(void)
{  
    /* Give sometime to let the object go away */
    EmcutilSleep(1000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for scooter test ===\n");
   
    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    return;

} /* End scooter_dualsp_cleanup() */
