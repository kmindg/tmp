/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file kishkashta.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test for verifying the background activity manager can control 
 *  our background operations
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
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test_common_utils.h"

#define KISHKASHTA_MIN_DISK_COUNT 10

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_lba_t kishkashta_do_verify(void);
static void kishkashta_test_all_bgs_off(void);
fbe_status_t fbe_test_get_available_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                                    fbe_object_id_t* object_ids_p);


/*************************
 *   TEST DESCRIPTION
 *************************/
char * kishkashta_short_desc = "Verify Background Activity Manager can control background operations progress";
char * kishkashta_long_desc =
"The Background Activity Manager Scenario (kishkashta) tests the ability of the BAM to control the progress ofbackground operations "
"by applying a scale (range 0-100) that determines how fast the SEP background activity should progress in order to balance\n"
"the overall system performance \n"
"The test will start a verify on a few drives, see how long it takes on a scale of 100 (default\n"
"It then cahnges the scale to 50 and expects to see the progress to slow down\n"
;

fbe_object_id_t      object_ids[KISHKASHTA_MIN_DISK_COUNT];

void kishkashta_test_init(void)
{
    #if defined(_AMD64_)
    if(fbe_test_util_is_simulation())
    {
         bubble_bass_test_setup();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    #endif //(_AMD64_)
    return;
}

void kishkashta_test_destroy(void)
{
    #if defined(_AMD64_)
    if(fbe_test_util_is_simulation())
    {
        bubble_bass_test_cleanup();
    }
    #endif //(_AMD64_)
    return;
}

void kishkashta_test(void)
{
    #if defined(_AMD64_)/*this test works only at 64 bit mode since atomic exchange in the scheduler is affected by EMC pal 32 bit version*/
    fbe_lba_t					verify_full_speed, max_verify_throttle, verify_throttle;
    fbe_u64_t					percent_change;
    fbe_status_t 				status;
    fbe_scheduler_set_scale_t   set_scale;
    fbe_test_discovered_drive_locations_t   drive_locations;
    

    /* fill out  all available drive information
     */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /*Get minimum drives and there object ids */
    fbe_test_get_available_drive_location(&drive_locations,object_ids);
    

	/*what is the avergage checkpoint for full speed*/
	mut_printf(MUT_LOG_TEST_STATUS,"=== Start 20sec. verify on 15 drives Full speed===\n");
	verify_full_speed = kishkashta_do_verify();

	/*let's reduce speed by 50%*/
	mut_printf(MUT_LOG_TEST_STATUS,"=== Start 20sec. verify on 15 drives with 50%% throttle===\n");
	set_scale.scale = 50;
	status = fbe_api_scheduler_set_scale(&set_scale);

	/*and check again*/
	verify_throttle = kishkashta_do_verify();

	mut_printf(MUT_LOG_TEST_STATUS,"=== Start 20sec. verify on 15 drives with 100%% throttle===\n");
	set_scale.scale = 0;
	status = fbe_api_scheduler_set_scale(&set_scale);

	/*and check again*/
	max_verify_throttle = kishkashta_do_verify();

	mut_printf(MUT_LOG_TEST_STATUS,"=== no thr:%llu, thr:%llu, full thr:%llu===\n", verify_full_speed, verify_throttle, max_verify_throttle);

#if 0 /* Disabled untill scheduler credit system is tuned on hardware */
	/*this is not an exact science but we should see at least 20% decrease*/	
	MUT_ASSERT_TRUE(verify_throttle <= verify_full_speed * 0.8);

	/*this is not an exact science but we should see at least 60% decrease*/
	MUT_ASSERT_TRUE(max_verify_throttle <= verify_full_speed * 0.6);
#endif

	if(verify_full_speed != 0)
	{
		percent_change = (verify_throttle * 100) / verify_full_speed;
		mut_printf(MUT_LOG_TEST_STATUS,"=== Throttled verify was reduced to %llu%% of full speed verify===\n", percent_change);

		percent_change = (max_verify_throttle * 100) / verify_full_speed;
		mut_printf(MUT_LOG_TEST_STATUS,"=== MAX Throttled verify was reduced to %llu%% of full speed verify===\n", percent_change);
	}
	else
	{
		mut_printf(MUT_LOG_TEST_STATUS,"===  Warning:Full speed verify = 0  ===\n");
	}

	kishkashta_test_all_bgs_off();
	#endif //(_AMD64_)

}

static fbe_lba_t kishkashta_do_verify(void)
{
    fbe_status_t								status;
    fbe_u32_t									drive;
	fbe_provision_drive_get_verify_status_t		verify_status;
	fbe_lba_t									total_checkpoints = 0;

    /*start a verify on 10 drives*/
	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {

		/* Before starting the verify operation disable the background disk zeroing.
		*/
		status = fbe_test_sep_util_provision_drive_disable_zeroing(object_ids[drive]);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		/*start a verify*/
		status = fbe_api_provision_drive_set_verify_checkpoint(object_ids[drive], FBE_PACKET_FLAG_NO_ATTRIB, 0);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		status = fbe_test_sep_util_provision_drive_enable_verify(object_ids[drive]);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		
	}

    fbe_api_sleep(20000);/*let's verify for a while*/

	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {
		status = fbe_test_sep_util_provision_drive_disable_verify(object_ids[drive]);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	}

	/*go over the drives and see what is their average progress*/
	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {
		/*start a verify*/
		status = fbe_api_provision_drive_get_verify_status(object_ids[drive], FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		total_checkpoints += verify_status.verify_checkpoint;
	}

	return total_checkpoints /= KISHKASHTA_MIN_DISK_COUNT;/*get the avergage*/
}

static void kishkashta_test_all_bgs_off(void)
{
    fbe_status_t								status;
    fbe_u32_t									drive;
	fbe_provision_drive_get_verify_status_t		verify_status;
	fbe_lba_t									total_checkpoints = 0;

	mut_printf(MUT_LOG_TEST_STATUS,"=== Turn off all background services===\n");
	/*cut off the background services*/
	status = fbe_api_base_config_disable_all_bg_services();
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*start a verify on 10 drives*/
	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {
		/*start a verify*/
		status = fbe_api_provision_drive_set_verify_checkpoint(object_ids[drive], FBE_PACKET_FLAG_NO_ATTRIB, 0);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		status = fbe_test_sep_util_provision_drive_enable_verify( object_ids[drive] );
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		
	}

	mut_printf(MUT_LOG_TEST_STATUS,"=== Wait and see verify is not progressing===\n");

    fbe_api_sleep(20000);/*let's verify for a while*/

    /*go over the drives and see what is their average progress*/
	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {
		
		status = fbe_api_provision_drive_get_verify_status(object_ids[drive], FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_UINT64_EQUAL((fbe_lba_t)0, verify_status.verify_checkpoint);/*we don't want to see any progress*/
	}

	mut_printf(MUT_LOG_TEST_STATUS,"=== Turn On all background services===\n");
	/*Turn on the background services*/
	status = fbe_api_base_config_enable_all_bg_services();
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	mut_printf(MUT_LOG_TEST_STATUS,"=== Wait and see verify is progressing===\n");
	fbe_api_sleep(20000);/*let's verify for a while*/

	 /*go over the drives and see what is their average progress*/
	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {
		
		status = fbe_api_provision_drive_get_verify_status(object_ids[drive], FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_UINT64_NOT_EQUAL((fbe_lba_t)0, verify_status.verify_checkpoint);/*we want to see some progress*/
		total_checkpoints += verify_status.verify_checkpoint;
	}

	mut_printf(MUT_LOG_TEST_STATUS,"=== verified progress of verify (avg. 0x%llX)===\n", (unsigned long long)(total_checkpoints / KISHKASHTA_MIN_DISK_COUNT));

	for (drive = 0; drive < KISHKASHTA_MIN_DISK_COUNT; drive++ ) {
		status = fbe_test_sep_util_provision_drive_disable_verify( object_ids[drive]);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	}
    
}

fbe_status_t fbe_test_get_available_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                                   fbe_object_id_t    *object_ids)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       drive_index = 0;
    fbe_u32_t       index = 0;
    fbe_u32_t       block_index =0; 
    fbe_u32_t       drive_cnt = 0;
    fbe_u32_t       drive_count = 0;
    fbe_test_raid_group_disk_set_t disk_set;

    for(block_index = 1; block_index < FBE_TEST_BLOCK_SIZE_LAST; block_index++)
    {
        for(drive_index = 1; drive_index < FBE_DRIVE_TYPE_LAST; drive_index++)
        {
            drive_count = drive_locations_p->drive_counts[block_index][drive_index];
            for(drive_cnt = 0;drive_cnt < drive_count;drive_cnt++)
            {
                disk_set.bus = drive_locations_p->disk_set[block_index][drive_index][drive_cnt].bus;
                disk_set.enclosure = drive_locations_p->disk_set[block_index][drive_index][drive_cnt].enclosure;
                disk_set.slot = drive_locations_p->disk_set[block_index][drive_index][drive_cnt].slot;
                drive_locations_p->drive_counts[block_index][drive_index]--;

                /*Get the object id of the drive*/
                status = fbe_api_provision_drive_get_obj_id_by_location(disk_set.bus,disk_set.enclosure,disk_set.slot,&object_ids[index]);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                index++;
                /*We need only 10 drives to run this test*/    
                if(index >= KISHKASHTA_MIN_DISK_COUNT)
                    return FBE_STATUS_OK;
            }
            
        }
    }
    return FBE_STATUS_NO_OBJECT;
}




