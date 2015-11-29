/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file performance_test.c
 ***************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *   05/04/2011:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"

#include "fbe/fbe_api_terminator_interface.h"
#include "pp_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_utils.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_test_common_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#define FBE_PERFORMANCE_TEST_DUAL_SP 0 /* no dual sp for now */

static fbe_sim_transport_connection_target_t active_connection_target = FBE_SIM_INVALID_SERVER;

static 	fbe_api_rdgen_context_t	rdgen_context[10];

enum performance_test_constants_e{
	PERFORMANCE_TEST_RDGEN_CAPACITY = 0x100000,
};

static void performance_test_part_1(void);
static void performance_test_part_2(void);
static void performance_test_part_3(void);
static void performance_test_part_4(void);
static void performance_test_part_5(void);
static void performance_test_part_6(fbe_rdgen_operation_t rdgen_opcode);
static void performance_test_part_7(void);

static void performance_stripe_lock_test(void);
static fbe_status_t performance_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context);


/*************************
 *   TEST DESCRIPTION
 *************************/
char * performance_test_short_desc = "I/O performance test";
char * performance_test_long_desc = "\
It tests the performance of IOs.\n\
\n\
\n"
"\n\
STEP 1: configure all raid groups and LUNs.\n\
        - It has one R5 RG with 520 block size.\n\
        - The RG has one lun.\n\
        - Runs in single SP mode.\n\
\n\
STEP 2: Peform the I/O test sequence.\n\
        - Perform sequential write IOs for 10 passes.\n\
        - The IOs covers a region that spans from 0 to certain max lba.\n\
        - Each write IO writes 400 blocks of data.\n\
        - Runs in single thread.\n\
\n\
STEP 3: Prints out the performance results.\n\
		- Once the IOs are completed, calculate the IOPS.\n\
		- Calculate the bandwidth \n\
\n\
STEP 4: Destroy raid groups and LUNs.\n\
\n\
Test Specific Switches:\n\
          -none.\n\
\n\
Outstanding issues: - These were enumerated as part 1, 2 and 3. Explanation wasn't sufficient\n\
    - One and Two PDOs Small I/O  write, read.\n\
    - PVD Small I/O  write, write, read.\n\
	- Single Mirror, Single LUN, Single thread, Small I/O.\n\
    - I see performace_test_part1, part2,part3, part4 not being used.\n\
    - Need to have header for the functions.\n\
\n\
Description last updated: 9/28/2011.\n";


void performance_test_init_single_sp(void)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
	fbe_u32_t i;
	fbe_object_id_t object_id;

    /* before loading the physical package we need to initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert port (0) */
    fbe_test_pp_util_insert_sas_pmc_port(1, 2, 0, &port_handle); /* io port, portal, backend number */ 											

    /* Insert an enclosure (0,0) */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port_handle, &encl_handle);

	/* Insert a SAS drives (0,0,i) */
    for (i = 0; i < 4; i++){
        fbe_test_pp_util_insert_sas_drive(0, 0, i, 520, /*10 * */(fbe_u64_t)TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY , &drive_handle);
    }

    for (i = 4; i < 10; i++){
        fbe_test_pp_util_insert_sas_drive(0, 0, i, 520, (fbe_u64_t)TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY / 100 , &drive_handle);
    }

	fbe_test_pp_util_insert_sas_drive(0, 0, 10, 520, (fbe_u64_t)0x1400000 /* 10GB*/ , &drive_handle);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(14, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

	for (i = 0; i < 10; i++){
		object_id = FBE_OBJECT_ID_INVALID;
		while(object_id == FBE_OBJECT_ID_INVALID) {
			status = fbe_api_get_physical_drive_object_id_by_location(0,0,i, &object_id);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
			if(object_id == FBE_OBJECT_ID_INVALID){
				mut_printf(MUT_LOG_LOW, "No LDO 0_0_%d\n", i);
				fbe_api_sleep(1000);
			}
		}
		
		status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 60000, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	}

	sep_config_load_sep_and_neit();
    fbe_test_wait_for_all_pvds_ready();

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_io_flags();

	return;
}

void performance_test_init(void)
{
	fbe_status_t status;

#if FBE_PERFORMANCE_TEST_DUAL_SP
	mut_printf(MUT_LOG_LOW, "=== Init SPB ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	performance_test_init_single_sp();
	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif

	mut_printf(MUT_LOG_LOW, "=== Init SPA ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	performance_test_init_single_sp();
	status = fbe_test_sep_util_wait_for_database_service(20000);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

void performance_test_destroy_single_sp(void)
{
    
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}

void performance_test_destroy(void)
{
	mut_printf(MUT_LOG_LOW, "=== Kozma Prutkov destroy SPA ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	performance_test_destroy_single_sp();

#if FBE_PERFORMANCE_TEST_DUAL_SP
	mut_printf(MUT_LOG_LOW, "=== Kozma Prutkov destroy SPB ===\n");
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	performance_test_destroy_single_sp();
#endif

    return;
}

void performance_test(void)
{
	fbe_status_t status;
	fbe_object_id_t pvd_object_id;	
	//fbe_cmi_service_get_info_t spa_cmi_info;
	//fbe_cmi_service_get_info_t spb_cmi_info;
	fbe_u32_t i;

#if FBE_PERFORMANCE_TEST_DUAL_SP
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
			mut_printf(MUT_LOG_LOW, "SPA is ACTIVE\n");
		}

		if(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE){
			active_connection_target = FBE_SIM_SP_B;
			passive_connection_target = FBE_SIM_SP_A;
			mut_printf(MUT_LOG_LOW, "SPB is ACTIVE\n");
		}
	}while(active_connection_target != FBE_SIM_SP_A && active_connection_target != FBE_SIM_SP_B);
#else
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	active_connection_target = FBE_SIM_SP_A;
#endif

	mut_printf(MUT_LOG_LOW, "Wait for PVD objects to be ready on ACTIVE side. \n");
	fbe_api_sim_transport_set_target_server(active_connection_target);
	for(i = 5; i < 10; i++){
		status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, i, &pvd_object_id);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	
		status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	}

#if FBE_PERFORMANCE_TEST_DUAL_SP
	mut_printf(MUT_LOG_LOW, "Wait for PVD objects to be ready on PASSIVE side. \n");
	fbe_api_sim_transport_set_target_server(passive_connection_target);
	for(i = 5; i < 10; i++){
		status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, i, &pvd_object_id);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		/* Why it is taking so long for passive PVD to come up ? */
		status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 15000, FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	}
#endif

	//performance_test_part_1();
	//performance_test_part_2();
	//performance_test_part_5();
    //performance_test_part_6(FBE_RDGEN_OPERATION_WRITE_ONLY);
    //performance_test_part_6(FBE_RDGEN_OPERATION_READ_ONLY);
	//performance_test_part_7();
	performance_stripe_lock_test();
	return;

	mut_printf(MUT_LOG_LOW, "fbe_payload_ex_t %d \n", (int)sizeof(fbe_payload_ex_t));
	mut_printf(MUT_LOG_LOW, "fbe_payload_control_operation_t %d \n", (int)sizeof(fbe_payload_control_operation_t));
	mut_printf(MUT_LOG_LOW, "fbe_payload_block_operation_t %d \n", (int)sizeof(fbe_payload_block_operation_t));
	mut_printf(MUT_LOG_LOW, "fbe_payload_metadata_operation_t %d \n", (int)sizeof(fbe_payload_metadata_operation_t));
	mut_printf(MUT_LOG_LOW, "fbe_payload_stripe_lock_operation_t %d \n", (int)sizeof(fbe_payload_stripe_lock_operation_t));

	mut_printf(MUT_LOG_LOW, "fbe_raid_iots_t %d \n", (int)sizeof(fbe_raid_iots_t));
	mut_printf(MUT_LOG_LOW, "fbe_raid_siots_t %d \n", (int)sizeof(fbe_raid_siots_t));

	mut_printf(MUT_LOG_LOW, "fbe_payload_discovery_operation_t %d \n", (int)sizeof(fbe_payload_discovery_operation_t));
	mut_printf(MUT_LOG_LOW, "fbe_payload_smp_operation_t %d \n", (int)sizeof(fbe_payload_smp_operation_t));


	mut_printf(MUT_LOG_LOW, "fbe_payload_cdb_operation_t %llu \n",
		   (unsigned long long)sizeof(fbe_payload_cdb_operation_t));
	mut_printf(MUT_LOG_LOW, "fbe_payload_fis_operation_t %llu \n",
		   (unsigned long long)sizeof(fbe_payload_fis_operation_t));


	mut_printf(MUT_LOG_LOW, "fbe_payload_dmrb_operation_t %llu \n",
		   (unsigned long long)sizeof(fbe_payload_dmrb_operation_t));


	mut_printf(MUT_LOG_LOW, "fbe_payload_physical_drive_transaction_t %llu \n",
		   (unsigned long long)sizeof(fbe_payload_physical_drive_transaction_t));

    return;
}

static void performance_test_print_results(fbe_u32_t n, fbe_u64_t* iops_p)
{
	fbe_u32_t i;
	fbe_u64_t iops = 0;
	fbe_u64_t mbps = 0;

	for(i = 0; i < n; i++){
		MUT_ASSERT_INT_EQUAL(rdgen_context[i].start_io.status.status, FBE_STATUS_OK);
		MUT_ASSERT_INT_EQUAL(rdgen_context[i].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

        mut_printf(MUT_LOG_LOW, "fbe_raid_iots_t %llu \n",
		   (unsigned long long)sizeof(fbe_raid_iots_t));
		iops += (fbe_u64_t)((double)rdgen_context[i].start_io.statistics.io_count / ((double)rdgen_context[i].start_io.statistics.elapsed_msec / 1000.0));
		//mbps += (fbe_u64_t)(((double)rdgen_context[i].start_io.specification.max_blocks * 520.0 * (double)iops) / (1024.0 * 1024.0));
		/*
		mut_printf(MUT_LOG_TEST_STATUS, "io_count: %lld; time: %lld; IOPS: %lld; MBPS: %lld",
																		rdgen_context[i].start_io.statistics.io_count,
																		rdgen_context[i].start_io.statistics.elapsed_msec,
																		iops, mbps);
		*/
	}

	mbps = (fbe_u64_t)(((double)rdgen_context[0].start_io.specification.max_blocks * 520.0 * (double)iops) / (1024.0 * 1024.0));
	mut_printf(MUT_LOG_TEST_STATUS, "N: %d; IOPS: %llud; MBPS: %llud", n,
		   (unsigned long long)iops, (unsigned long long)mbps);
    if (iops_p != NULL)
    {
        *iops_p = iops;
    }
}

static void test_part_1(fbe_u32_t n_drives, fbe_object_id_t	* object_id, fbe_rdgen_operation_t rdgen_operation, fbe_u64_t io_size)
{
	fbe_u32_t i;
	fbe_status_t status;

	for(i = 0; i < n_drives; i++){
		fbe_zero_memory(&rdgen_context[i], sizeof(fbe_api_rdgen_context_t));
		status = fbe_api_rdgen_test_context_init(&rdgen_context[i], 
												object_id[i],
												FBE_CLASS_ID_INVALID,
												FBE_PACKAGE_ID_PHYSICAL,
												rdgen_operation,
												FBE_RDGEN_PATTERN_LBA_PASS,
												100, // 10 full sequential passes
												0, // num ios not used 
												0, // time not used
												1, // threads 
												FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
												0, // start lba
												0, // min lba 
												PERFORMANCE_TEST_RDGEN_CAPACITY, // max lba 
												FBE_RDGEN_BLOCK_SPEC_CONSTANT,
												io_size,   // number of stripes to write
												io_size); // max blocks 

		fbe_api_rdgen_io_specification_set_options(&rdgen_context[i].start_io.specification, FBE_RDGEN_OPTIONS_PERFORMANCE);
		fbe_api_rdgen_io_specification_set_affinity(&rdgen_context[i].start_io.specification, FBE_RDGEN_AFFINITY_FIXED, 2);
	}

	/* Run our I/O test. */
	mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
	status = fbe_api_rdgen_run_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, n_drives);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void test_part_destroy(fbe_u32_t n_drives)
{
	fbe_u32_t i;
	fbe_status_t status;

	for(i = 0; i < n_drives; i++){
		status = fbe_api_rdgen_test_context_destroy(&rdgen_context[i]);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	}
}

static void performance_test_part_1(void)
{
	fbe_status_t	status;
	fbe_u32_t		i;
	fbe_object_id_t	pdo_object_id[4];
	fbe_u64_t		io_size;

	/* Run Small I/O test */
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

	fbe_api_sim_transport_set_target_server(active_connection_target);

	for(i = 0; i < 4; i++){
		status = fbe_api_get_physical_drive_object_id_by_location(0,0,i+5, &pdo_object_id[i]);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	
	}

	/* Let's allocate the memory for the drives first */
	
	test_part_1(4, pdo_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, 0x800);
	test_part_destroy(4);

	for(io_size = 0x100; io_size < 0x800; io_size *= 2){
		mut_printf(MUT_LOG_TEST_STATUS, "Multiple LDO write size: %llX",
			   (unsigned long long)io_size);
		for(i = 0; i < 4; i++){
			test_part_1(i+1, pdo_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, io_size);
			performance_test_print_results(i+1, NULL);
			test_part_destroy(i+1);
		}

		mut_printf(MUT_LOG_TEST_STATUS, "Multiple LDO read size: %llX",
			   (unsigned long long)io_size);
		for(i = 0; i < 4; i++){
			test_part_1(i+1, pdo_object_id, FBE_RDGEN_OPERATION_READ_ONLY, io_size);
			performance_test_print_results(i+1, NULL);
			test_part_destroy(i+1);
		}
	}
}


static void test_part_2(fbe_u32_t n_drives, fbe_object_id_t	* object_id, fbe_rdgen_operation_t rdgen_operation, fbe_u64_t io_size)
{
	fbe_u32_t i;
	fbe_status_t status;

	for(i = 0; i < n_drives; i++){
		fbe_zero_memory(&rdgen_context[i], sizeof(fbe_api_rdgen_context_t));
		status = fbe_api_rdgen_test_context_init(&rdgen_context[i], 
												object_id[i],
												FBE_CLASS_ID_INVALID,
												FBE_PACKAGE_ID_SEP_0,
												rdgen_operation,
												FBE_RDGEN_PATTERN_LBA_PASS,
												100, // 10 full sequential passes
												0, // num ios not used 
												0, // time not used
												1, // threads 
												FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
												0, // start lba
												0, // min lba 
												PERFORMANCE_TEST_RDGEN_CAPACITY, // max lba 
												FBE_RDGEN_BLOCK_SPEC_CONSTANT,
												io_size,   // number of stripes to write
												io_size); // max blocks 

		fbe_api_rdgen_io_specification_set_options(&rdgen_context[i].start_io.specification, FBE_RDGEN_OPTIONS_PERFORMANCE);
		fbe_api_rdgen_io_specification_set_affinity(&rdgen_context[i].start_io.specification, FBE_RDGEN_AFFINITY_FIXED, 2);
	}
  
	/* Run our I/O test. */
	//mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
	status = fbe_api_rdgen_run_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, n_drives);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static void performance_test_part_2(void)
{
	fbe_status_t status;
	fbe_object_id_t	pvd_object_id;

	fbe_api_sim_transport_set_target_server(active_connection_target);
	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 5, &pvd_object_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);	

	mut_printf(MUT_LOG_TEST_STATUS, "PVD with zeroing");
	test_part_2(1, &pvd_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, 0x200);
	performance_test_print_results(1, NULL);
	test_part_destroy(1);

	mut_printf(MUT_LOG_TEST_STATUS, "PVD without zeroing");
	test_part_2(1, &pvd_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, 0x200);
	performance_test_print_results(1, NULL);
	test_part_destroy(1);

	mut_printf(MUT_LOG_TEST_STATUS, "PVD without locking");
	fbe_api_provision_drive_set_zero_checkpoint(pvd_object_id, FBE_LBA_INVALID);
	test_part_2(1, &pvd_object_id, FBE_RDGEN_OPERATION_WRITE_ONLY, 0x200);
	performance_test_print_results(1, NULL);
	test_part_destroy(1);
}


static void performance_test_part_3(void)
{
	fbe_api_job_service_raid_group_create_t     rg_create_req;
	fbe_api_job_service_raid_group_destroy_t	rg_destroy_request;
	fbe_api_job_service_lun_create_t            lun_create_req;
	fbe_api_job_service_lun_destroy_t			lun_destroy_req;

	fbe_status_t status;
	fbe_u32_t i;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;
	fbe_object_id_t                             rg_object_id;
	fbe_object_id_t								lun_object_id;

	fbe_database_lun_info_t				        lun_info;

	fbe_u64_t iops;

	/* Create mirror */
    /* Create a RAID group object with RAID group create API. */
    fbe_zero_memory(&rg_create_req, sizeof(fbe_api_job_service_raid_group_create_t));
    rg_create_req.b_bandwidth = 0;
    rg_create_req.capacity = 2 * 1024 * 1024; /* 1 GB */
    //rg_create_req.explicit_removal = 0;
    rg_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    rg_create_req.is_private = FBE_TRI_FALSE;
    rg_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    rg_create_req.raid_group_id = 1;
    rg_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID1;
    rg_create_req.drive_count = 2; /* It's a Mirror */
    rg_create_req.job_number = 0;
    rg_create_req.wait_ready  = FBE_FALSE;
    rg_create_req.ready_timeout_msec = 20000;

    for (i = 0; i < rg_create_req.drive_count; i++) {
        rg_create_req.disk_array[i].bus = 0;
        rg_create_req.disk_array[i].enclosure = 0;
        rg_create_req.disk_array[i].slot = i + 5;
    }

    /* Create a RAID group using Job service API to create a RAID group. */
    status = fbe_api_job_service_raid_group_create(&rg_create_req);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group create job sent successfully! ==");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX",
	       (unsigned long long)rg_create_req.job_number);

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_create_req.job_number,
                                         10000, /* 10 sec timeout */
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Find the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_create_req.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* verify the raid group get to ready state in reasonable time */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(FBE_TRUE,
																		 rg_object_id, 
																		 FBE_LIFECYCLE_STATE_READY, 
																		 20000,
																		 FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Create LUN */
    /* Fill up the logical unit information. */
    /* @!TODO: need to enhance this function so all data comes from caller and not hardcoded as shown
     * below.  This function is currently only used by one test script while other tests use their own 
     * similar utility function.
     * Need to clean this up.
     */
    lun_create_req.raid_type = rg_create_req.raid_type;
    lun_create_req.raid_group_id = rg_create_req.raid_group_id; 
    lun_create_req.lun_number = 0;
    lun_create_req.capacity = (fbe_lba_t)(rg_create_req.capacity * 0.9); /* 10% for raid metadata */
    lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    lun_create_req.ndb_b = FALSE;
    lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    lun_create_req.job_number = 0;
    lun_create_req.wait_ready = FBE_FALSE;
    lun_create_req.ready_timeout_msec = 20000;

    /* create a Logical Unit using job service fbe api. */
    status = fbe_api_job_service_lun_create(&lun_create_req);    

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== Job_service_lun_create request sent successfully! ===");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX",
	       (unsigned long long)lun_create_req.job_number);

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(lun_create_req.job_number,
                                         10000, /* 10 sec timeout */
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created LUN");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN creation failed");

    status = fbe_api_database_lookup_lun_by_number(lun_create_req.lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
    
    /* verify the lun get to ready state in reasonable time. */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(FBE_TRUE, 
																			lun_object_id, 
																			FBE_LIFECYCLE_STATE_READY, 
																			10000, 
																			FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* verify the lun connect to BVD in reasonable time. */
    status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(FBE_TRUE,
                                                               lun_object_id,
                                                               10000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_sim_transport_set_target_server(active_connection_target);

	fbe_zero_memory(&lun_info, sizeof(fbe_database_lun_info_t));
	lun_info.lun_object_id = lun_object_id;
	status = fbe_api_database_get_lun_info(&lun_info);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  
	mut_printf(MUT_LOG_TEST_STATUS, "LUN capacity %llX",
		   (unsigned long long)lun_info.capacity);

	/* Run Small I/O test */
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

	fbe_api_sim_transport_set_target_server(active_connection_target);

	fbe_zero_memory(&rdgen_context[0], sizeof(fbe_api_rdgen_context_t));

    status = fbe_api_rdgen_test_context_init(&rdgen_context[0], 
											lun_object_id,
											FBE_CLASS_ID_INVALID,
											FBE_PACKAGE_ID_SEP_0,
											FBE_RDGEN_OPERATION_WRITE_ONLY/*FBE_RDGEN_OPERATION_WRITE_READ_CHECK*/,
											FBE_RDGEN_PATTERN_LBA_PASS,
											1, // one full sequential pass
											0, // num ios not used 
											0, // time not used
											1, // threads 
											FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
											0, // start lba
											0, // min lba 
											lun_info.capacity, // max lba 
											FBE_RDGEN_BLOCK_SPEC_CONSTANT,
											0x100,   // number of stripes to write
											0x100); // max blocks 

    /* Run our I/O test. */
	mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
	status = fbe_api_rdgen_test_context_run_all_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_context[0].start_io.status.status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(rdgen_context[0].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

	iops = (fbe_u64_t)(rdgen_context[0].start_io.statistics.io_count / (rdgen_context[0].start_io.statistics.elapsed_msec / 1000));
	mut_printf(MUT_LOG_TEST_STATUS, "io_count: %u; time: %llu; IOPS: %llu",
		   rdgen_context[0].start_io.statistics.io_count,
		   (unsigned long long)rdgen_context[0].start_io.statistics.elapsed_msec,
		   (unsigned long long)iops);

	/* Print preformance */
	fbe_test_sep_util_print_metadata_statistics(rg_object_id);

	/* Hey, let's do it again */
	fbe_zero_memory(&rdgen_context[0], sizeof(fbe_api_rdgen_context_t));

    status = fbe_api_rdgen_test_context_init(&rdgen_context[0], 
											lun_object_id,
											FBE_CLASS_ID_INVALID,
											FBE_PACKAGE_ID_SEP_0,
											FBE_RDGEN_OPERATION_WRITE_ONLY/*FBE_RDGEN_OPERATION_WRITE_READ_CHECK*/,
											FBE_RDGEN_PATTERN_LBA_PASS,
											1, // one full sequential pass
											0, // num ios not used 
											0, // time not used
											1, // threads 
											FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
											0, // start lba
											0, // min lba 
											lun_info.capacity, // max lba 
											FBE_RDGEN_BLOCK_SPEC_CONSTANT,
											0x100,   // number of stripes to write
											0x100); // max blocks 

    /* Run our I/O test. */
	mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
	status = fbe_api_rdgen_test_context_run_all_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(rdgen_context[0].start_io.status.status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(rdgen_context[0].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

	iops = (fbe_u64_t)(rdgen_context[0].start_io.statistics.io_count / (rdgen_context[0].start_io.statistics.elapsed_msec / 1000));
	mut_printf(MUT_LOG_TEST_STATUS, "io_count: %u; time: %llu; IOPS: %llu",
		   rdgen_context[0].start_io.statistics.io_count,
		   (unsigned long long)rdgen_context[0].start_io.statistics.elapsed_msec,
		   (unsigned long long)iops);

	/* Print preformance */
	fbe_test_sep_util_print_metadata_statistics(rg_object_id);

	/* Destroy LUN */
    /* Destroy a LUN */
    lun_destroy_req.lun_number = lun_create_req.lun_number;
    lun_destroy_req.job_number = 0;
    lun_destroy_req.wait_destroy = FBE_FALSE;
    lun_destroy_req.destroy_timeout_msec = 20000;

    status = fbe_api_job_service_lun_destroy(&lun_destroy_req);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroy job sent successfully! ===");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX",
	       (unsigned long long)lun_destroy_req.job_number);

    status = fbe_api_common_wait_for_job(lun_destroy_req.job_number ,
                                         10000,
                                         &job_error_code,
                                         &job_status,
                                         NULL);
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy LUN");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "LUN destruction failed");


    /* Make sure the LUN Object successfully destroyed */

    /* Make sure the LUN has destroyed itself.
     * We wait for an invalid state and expect to get back an error.
     */
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, 
                                                     FBE_LIFECYCLE_STATE_NOT_EXIST, 
                                                     10000, 
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Destroy Mirror */
    /* Destroy a RAID group */
    rg_destroy_request.raid_group_id = rg_create_req.raid_group_id;
    rg_destroy_request.job_number = 0;
    rg_destroy_request.wait_destroy = FBE_FALSE;
    rg_destroy_request.destroy_timeout_msec = 60000;

    status = fbe_api_job_service_raid_group_destroy(&rg_destroy_request);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroy job sent successfully! ===");
    status = fbe_api_common_wait_for_job(rg_destroy_request.job_number ,
                                         10000, /* 10 sec timeout */
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to destroy RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG destruction failed");

    /* Make sure the RAID object successfully destroyed */
    status = fbe_api_database_lookup_raid_group_by_number(rg_create_req.raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group destroyed successfully! ===");
}

static void performance_test_part_4(void)
{
	fbe_u32_t i;
	fbe_time_t start_time;
	fbe_time_t end_time;
	fbe_semaphore_t sem;
	fbe_spinlock_t lock;

	/* Do some semaphore Create / Destroy performance test */
	mut_printf(MUT_LOG_LOW, "Semaphore (CSX Native)\n");
	start_time = fbe_get_time();
	for(i = 0; i < 10000; i++){
		fbe_semaphore_init(&sem, 0, 0x0FFFFFFF);
		fbe_semaphore_destroy(&sem);
	}
	end_time = fbe_get_time();
	mut_printf(MUT_LOG_LOW, "10000 iteractions took %llu ms\n",
		   (unsigned long long)(end_time - start_time));

	start_time = fbe_get_time();
	for(i = 0; i < 100000; i++){
		fbe_semaphore_init(&sem, 0, 0x0FFFFFFF);
		fbe_semaphore_destroy(&sem);
	}
	end_time = fbe_get_time();
	mut_printf(MUT_LOG_LOW, "100000 iteractions took %llu ms\n",
		   (unsigned long long)(end_time - start_time));

	mut_printf(MUT_LOG_LOW, "Spin Lock Init/Destroy\n");
	start_time = fbe_get_time();
	for(i = 0; i < 10000; i++){
		fbe_spinlock_init(&lock);
		fbe_spinlock_destroy(&lock);
	}
	end_time = fbe_get_time();
	mut_printf(MUT_LOG_LOW, "10000 iteractions took %llu ms\n",
		   (unsigned long long)(end_time - start_time));

	start_time = fbe_get_time();
	for(i = 0; i < 100000; i++){
		fbe_spinlock_init(&lock);
		fbe_spinlock_destroy(&lock);
	}
	end_time = fbe_get_time();
	mut_printf(MUT_LOG_LOW, "100000 iteractions took %llu ms\n",
		   (unsigned long long)(end_time - start_time));

	mut_printf(MUT_LOG_LOW, "Spin Lock (CSX Native)\n");
	mut_printf(MUT_LOG_LOW, "Spin Lock Lock/Unlock\n");
	fbe_spinlock_init(&lock);

	start_time = fbe_get_time();
	for(i = 0; i < 10000; i++){
		fbe_spinlock_lock(&lock);
		fbe_spinlock_unlock(&lock);
	}
	end_time = fbe_get_time();
	mut_printf(MUT_LOG_LOW, "10000 iteractions took %llu ms\n",
		   (unsigned long long)(end_time - start_time));

	start_time = fbe_get_time();
	for(i = 0; i < 100000; i++){
		fbe_spinlock_lock(&lock);
		fbe_spinlock_unlock(&lock);
	}
	end_time = fbe_get_time();
	mut_printf(MUT_LOG_LOW, "100000 iteractions took %llu ms\n",
		   (unsigned long long)(end_time - start_time));
}

/* RAID 5 (4+1) test */
fbe_test_rg_configuration_t performance_test_rg_config[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,       0x32000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/* RAID 1 (1+1) test */
fbe_test_rg_configuration_t performance_test_rg_mirror_config[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,       0x32000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


fbe_test_raid_group_disk_set_t performance_test_disk_set[] = {
    /* width = 5 */
    {0,0,4}, {0,0,5}, {0,0,6}, {0,0,7}, {0,0,8},

    /* terminator */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}
};

static void performance_test_part_5(void)
{
    fbe_u32_t  raid_group_count = 0;
	fbe_object_id_t rg_object_id;
	fbe_object_id_t lun_object_id[8]; /* Up to 8 Luns */
	fbe_status_t status;
	fbe_test_rg_configuration_t * rg_config_p = performance_test_rg_config;
	fbe_u32_t i;
	fbe_u32_t max_cores = 1;
	fbe_memory_dps_statistics_t dps_stats;
	fbe_memory_dps_statistics_t new_dps_stats;
    fbe_u32_t       pool_idx = 0;
	fbe_cpu_id_t	core_count;
	fbe_cpu_id_t	cpu_id;

    raid_group_count = fbe_test_get_rg_array_length(performance_test_rg_config);

    fbe_test_sep_util_fill_raid_group_disk_set(rg_config_p, 1, performance_test_disk_set, NULL);

    /* Setup the lun capacity with the given number of chunks for each lun. */
    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
													  raid_group_count,
                                                      128, /* chunks_per_lun,  */
                                                      8);

    /* Kick of the creation of all the raid group with Logical unit configuration. */
    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	for(i = 0; i < max_cores; i++){
		status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[i].lun_number, &lun_object_id[i]);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

		/* verify the lun get to ready state in reasonable time. */
		status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[i], FBE_LIFECYCLE_STATE_READY, 6000, FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

		fbe_zero_memory(&rdgen_context[i], sizeof(fbe_api_rdgen_context_t));
		status = fbe_api_rdgen_test_context_init(&rdgen_context[i], 
												lun_object_id[i],
												FBE_CLASS_ID_INVALID,
												FBE_PACKAGE_ID_SEP_0,
												FBE_RDGEN_OPERATION_WRITE_ONLY,
												FBE_RDGEN_PATTERN_LBA_PASS,
												1, //  full sequential passes
												0, // num ios not used 
												0, // time not used
												1, // threads 
												FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
												0, // start lba
												0, // min lba 
												0x400 * 1000,/* PERFORMANCE_TEST_RDGEN_CAPACITY, */// max lba 
												FBE_RDGEN_BLOCK_SPEC_CONSTANT,
												0x400,   // number of stripes to write
												0x400); // max blocks 

		fbe_api_rdgen_io_specification_set_options(&rdgen_context[i].start_io.specification, FBE_RDGEN_OPTIONS_PERFORMANCE );
		fbe_api_rdgen_io_specification_set_affinity(&rdgen_context[i].start_io.specification, FBE_RDGEN_AFFINITY_FIXED, i);
	}

	mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
	status = fbe_api_rdgen_run_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, max_cores);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	performance_test_print_results(max_cores, NULL);
	test_part_destroy(max_cores);

	for(i = 0; i < max_cores; i++){
		status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[i].lun_number, &lun_object_id[i]);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

		/* verify the lun get to ready state in reasonable time. */
		status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[i], FBE_LIFECYCLE_STATE_READY, 6000, FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

		fbe_zero_memory(&rdgen_context[i], sizeof(fbe_api_rdgen_context_t));
		status = fbe_api_rdgen_test_context_init(&rdgen_context[i], 
												lun_object_id[i],
												FBE_CLASS_ID_INVALID,
												FBE_PACKAGE_ID_SEP_0,
												FBE_RDGEN_OPERATION_WRITE_ONLY,
												FBE_RDGEN_PATTERN_LBA_PASS,
												100, //  full sequential passes
												0, // num ios not used 
												0, // time not used
												1, // threads 
												FBE_RDGEN_LBA_SPEC_RANDOM,/*FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,*/
												0, // start lba
												0, // min lba 
												0x400 * 1000,/* PERFORMANCE_TEST_RDGEN_CAPACITY, */// max lba 
												FBE_RDGEN_BLOCK_SPEC_CONSTANT,
												0x200,   // number of stripes to write
												0x200); // max blocks 

		fbe_api_rdgen_io_specification_set_options(&rdgen_context[i].start_io.specification, 
													FBE_RDGEN_OPTIONS_PERFORMANCE |
													FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC |
													FBE_RDGEN_OPTIONS_PERFORMANCE_BYPASS_TERMINATOR);

		fbe_api_rdgen_io_specification_set_affinity(&rdgen_context[i].start_io.specification, FBE_RDGEN_AFFINITY_FIXED, 2);
	}

	status = fbe_api_dps_memory_get_dps_statistics(&dps_stats, FBE_PACKAGE_ID_SEP_0);

	mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
	status = fbe_api_rdgen_run_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, max_cores);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	performance_test_print_results(max_cores, NULL);

	test_part_destroy(max_cores);
	status = fbe_api_dps_memory_get_dps_statistics(&new_dps_stats, FBE_PACKAGE_ID_SEP_0);


	core_count = fbe_get_cpu_count();
    mut_printf(MUT_LOG_TEST_STATUS, "DPS FAST POOL STATISTICS DIFF\n");
	for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */
		for(cpu_id = 0; cpu_id < core_count; cpu_id++){
			mut_printf(MUT_LOG_TEST_STATUS, "Core %d, chunks %d free_cunks %d\n", cpu_id, 
                       new_dps_stats.fast_pool_number_of_chunks[pool_idx][cpu_id],
                       new_dps_stats.fast_pool_number_of_free_chunks[pool_idx][cpu_id]);
		}
	}

	for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */


		for(cpu_id = 0; cpu_id < core_count; cpu_id++){
		mut_printf(MUT_LOG_TEST_STATUS, "Queue id %d, request count %llu \n", pool_idx,  
                   (long long)new_dps_stats.request_count[pool_idx][cpu_id] - dps_stats.request_count[pool_idx][cpu_id]);
			mut_printf(MUT_LOG_TEST_STATUS, "Fast pool: Core %d, request count %llu \n", cpu_id, 
				(long long)new_dps_stats.fast_pool_request_count[pool_idx][cpu_id] - dps_stats.fast_pool_request_count[pool_idx][cpu_id]);
		}
	}

	return;
}

void performance_test_rg_wait_for_zeroing_complete(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status; 
    fbe_object_id_t                 pvd_object_id;

	fbe_lba_t                       zero_checkpoint = 0;

    fbe_u32_t index;
	fbe_u16_t		zeroing_percentage = 0;
	fbe_u32_t		counter = 0;
	fbe_time_t		start_time;

    /* get provision drive object id for first drive */
    status = fbe_api_provision_drive_get_obj_id_by_location( 0,0,10, &pvd_object_id);

    mut_printf(MUT_LOG_LOW, "=== Wait for rg config to complete zeroing. ===\n");	

    for (index = 0 ; index < rg_config_p->width; index++)
    {
        /* get provision drive object id for first drive */
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[index].bus,
                                                                 rg_config_p->rg_disk_set[index].enclosure,
                                                                 rg_config_p->rg_disk_set[index].slot,
                                                                 &pvd_object_id);
        
		mut_printf(MUT_LOG_LOW, "=== Wait for PVD %d_%d_%d to complete zeroing. ===\n",rg_config_p->rg_disk_set[index].bus,
																						 rg_config_p->rg_disk_set[index].enclosure,
																						 rg_config_p->rg_disk_set[index].slot);
		start_time = fbe_get_time();
		while(zeroing_percentage < 100){
			status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_checkpoint);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

			status = fbe_api_provision_drive_get_zero_percentage(pvd_object_id, &zeroing_percentage);
			MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
			if(!(counter % 10)){
				mut_printf(MUT_LOG_TEST_STATUS, "zero_checkpoint 0x%llX, %d%%", (unsigned long long)zero_checkpoint, zeroing_percentage);
			}
			counter++;
			fbe_api_sleep(100);
		}
		mut_printf(MUT_LOG_TEST_STATUS, "zero_checkpoint 0x%llX, %d%%", (unsigned long long)zero_checkpoint, zeroing_percentage);
		mut_printf(MUT_LOG_TEST_STATUS, "Zeroing time %d sec.\n", (fbe_u32_t)((fbe_get_time() - start_time) / 1000) );
    }

    mut_printf(MUT_LOG_LOW, "=== zeroing completed. ===\n");	

}

static void performance_test_part_6(fbe_rdgen_operation_t rdgen_opcode)
{
    fbe_u32_t  raid_group_count = 0;
	fbe_object_id_t rg_object_id;
	fbe_object_id_t lun_object_id[8]; /* Up to 8 Luns */
	fbe_status_t status;
	fbe_test_rg_configuration_t * rg_config_p = performance_test_rg_config;
	fbe_u32_t i;
	fbe_u32_t max_cores = 1;
    fbe_u32_t loops = 0;
    fbe_u64_t iops;
    fbe_u64_t running_iops_sum = 0;
    fbe_u32_t max_iterations = 3;
    const fbe_char_t *operation_string_p = NULL;
	fbe_u32_t 	perf_test_time;
	fbe_u32_t 	perf_test_threads;

    fbe_api_rdgen_get_operation_string(rdgen_opcode, &operation_string_p);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting %s for opcode %s\n", __FUNCTION__, operation_string_p);
    raid_group_count = fbe_test_get_rg_array_length(performance_test_rg_config);

    fbe_test_sep_util_fill_raid_group_disk_set(rg_config_p, 1, performance_test_disk_set, NULL);

    /* Setup the lun capacity with the given number of chunks for each lun. */
    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
													  raid_group_count,
                                                      128, /* chunks_per_lun,  */
                                                      8);

    /* Kick of the creation of all the raid group with Logical unit configuration. */
    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    performance_test_rg_wait_for_zeroing_complete(rg_config_p);
    //status = fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	perf_test_time = 10; /* 10 sec. */
	fbe_test_sep_util_get_cmd_option_int("-fbe_io_seconds", &perf_test_time);
	mut_printf(MUT_LOG_TEST_STATUS, "perf_test_time: %d sec.", perf_test_time);

	/*Convert to milisec. */
	perf_test_time *= 1000;
	perf_test_threads = 1; /* 1 thread */
	fbe_test_sep_util_get_cmd_option_int("-fbe_threads", &perf_test_threads);
	mut_printf(MUT_LOG_TEST_STATUS, "perf_test_threads: %d", perf_test_threads);

    for (loops = 0; loops < max_iterations; loops++)
    {
        for(i = 0; i < max_cores; i++){
            status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list[i].lun_number, &lun_object_id[i]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
            /* verify the lun get to ready state in reasonable time. */
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id[i], FBE_LIFECYCLE_STATE_READY, 6000, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
            fbe_zero_memory(&rdgen_context[i], sizeof(fbe_api_rdgen_context_t));
            status = fbe_api_rdgen_test_context_init(&rdgen_context[i], 
                                                    lun_object_id[i],
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    rdgen_opcode,
                                                    FBE_RDGEN_PATTERN_LBA_PASS,
                                                    0, /* fbe_test_sep_io_get_small_io_count(20000), *///  pass count
                                                    0, // num ios not used 
                                                    perf_test_time, // time 
                                                    perf_test_threads, // threads 
                                                    FBE_RDGEN_LBA_SPEC_RANDOM, /* FBE_RDGEN_LBA_SPEC_FIXED, */
                                                    0, // start lba
                                                    0, // min lba 
                                                    0x400 * 1000,/* PERFORMANCE_TEST_RDGEN_CAPACITY, */// max lba 
                                                    FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                                    16,   // min blocks
                                                    16); // max blocks 
    
            fbe_api_rdgen_io_specification_set_options(&rdgen_context[i].start_io.specification, 
                                                        FBE_RDGEN_OPTIONS_PERFORMANCE |
                                                        FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC/* |
                                                        FBE_RDGEN_OPTIONS_PERFORMANCE_BYPASS_TERMINATOR*/);
    
            fbe_api_rdgen_io_specification_set_affinity(&rdgen_context[i].start_io.specification, FBE_RDGEN_AFFINITY_FIXED, 1);
        }
    
        mut_printf(MUT_LOG_TEST_STATUS, "Start RDGEN");
        status = fbe_api_rdgen_test_context_run_all_tests(&rdgen_context[0], FBE_PACKAGE_ID_NEIT, max_cores);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        performance_test_print_results(max_cores, &iops);
        running_iops_sum += iops;
		test_part_destroy(max_cores);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Average iops: %lld\n", (long long)(running_iops_sum/max_iterations));
	

	return;
}

static void performance_test_part_7(void)
{
    fbe_status_t    status; 
    fbe_object_id_t pvd_object_id;
	fbe_lba_t		zero_checkpoint = 0;
	fbe_u16_t		zeroing_percentage = 0;
	fbe_u32_t		counter = 0;
	fbe_time_t		start_time;

    mut_printf(MUT_LOG_LOW, "=== Wait for rg config to complete zeroing. ===\n");	

    /* get provision drive object id for first drive */
    status = fbe_api_provision_drive_get_obj_id_by_location( 0,0,10, &pvd_object_id);
	start_time = fbe_get_time();
	while(zeroing_percentage < 100){
		status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_checkpoint);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

		status = fbe_api_provision_drive_get_zero_percentage(pvd_object_id, &zeroing_percentage);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		if(!(counter % 10)){
			mut_printf(MUT_LOG_TEST_STATUS, "zero_checkpoint 0x%llX, %d%%", (unsigned long long)zero_checkpoint, zeroing_percentage);
		}
		counter++;
		fbe_api_sleep(100);
	}
	mut_printf(MUT_LOG_TEST_STATUS, "zero_checkpoint 0x%llX, %d%%", (unsigned long long)zero_checkpoint, zeroing_percentage);
	mut_printf(MUT_LOG_TEST_STATUS, "Zeroing time %d sec.\n", (fbe_u32_t)((fbe_get_time() - start_time) / 1000) );

}


static fbe_status_t performance_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context);

static void performance_stripe_lock_test(void)
{
	fbe_status_t status;
	fbe_object_id_t object_id;
	fbe_u32_t i;

	fbe_api_metadata_debug_lock_t debug_lock[4];
	fbe_packet_t * packet[4];
	fbe_semaphore_t sem;

	mut_printf(MUT_LOG_LOW, "Stripe Lock Test Start ... \n");

	fbe_semaphore_init(&sem, 0, 1);

	fbe_api_base_config_disable_all_bg_services();
	
	fbe_api_sleep(1000);

	for(i = 0; i < 4; i++){
		packet[i] = malloc(sizeof(fbe_packet_t));
		fbe_transport_initialize_sep_packet(packet[i]);
	}

    status = fbe_api_provision_drive_get_obj_id_by_location( 0, 0, 5, &object_id );
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 5000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


	mut_printf(MUT_LOG_LOW, "We will work with PVD %d \n", object_id);

	for(i = 0 ; i < 4;i++){
		debug_lock[i].object_id = object_id;
		debug_lock[i].b_allow_hold = FBE_TRUE; 
		debug_lock[i].packet_ptr = NULL;
		debug_lock[i].stripe_number = 0;		
		debug_lock[i].stripe_count = 1;
	}

	/* Send start lock command */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_START;
    fbe_transport_set_completion_function(packet[0], performance_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	//getchar();

	mut_printf(MUT_LOG_LOW, "Step 1 (Send write lock ) ... \n");
	/* Send write lock on active side */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;			
    fbe_transport_set_completion_function(packet[0], performance_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[0].packet_ptr != NULL);

	//getchar();

	mut_printf(MUT_LOG_LOW, "Step 2 (Send another write lock ) ... \n");
	/* Send another write lock and see that it is stuck */
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK;		
	debug_lock[1].stripe_count = 2;

    fbe_transport_set_completion_function(packet[1], performance_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 1000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_LOW, "Step 3 (Send write unlock ) ... \n");
	/* Send write unlock */
	debug_lock[0].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
    fbe_transport_set_completion_function(packet[0], performance_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[0], &debug_lock[0]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Step 4 (Wait for completion of second write lock) ... \n");
	/* Wait for completion of second write lock */
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_TRUE(debug_lock[1].packet_ptr != NULL);

	mut_printf(MUT_LOG_LOW, "Step 5 (Send second write unlock ) ... \n");
	/* Send second write unlock */
	debug_lock[1].opcode = FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK;
    fbe_transport_set_completion_function(packet[1], performance_stripe_lock_completion,&sem);
	fbe_api_metadata_debug_lock(packet[1], &debug_lock[1]);
	status = fbe_semaphore_wait_ms(&sem, 0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	for(i = 0; i < 4; i++){
		fbe_transport_destroy_packet(packet[i]);
		free(packet[i]);
	}

    fbe_semaphore_destroy(&sem);
	
	mut_printf(MUT_LOG_LOW, "Stripe Lock Test Test Completed\n");

}

static fbe_status_t 
performance_stripe_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t completion_context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t *)completion_context;
	fbe_semaphore_release(sem, 0, 1, FALSE);
	mut_printf(MUT_LOG_LOW, "performance_stripe_lock_completion\n");
	return FBE_STATUS_OK;
}
