/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file phineas_and_ferb.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the test for the NEW persistance service (persist)
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_common_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void phineas_and_ferb_test_create_lun(fbe_lun_number_t lun_number);
static void phineas_and_ferb_test_destroy_lun(fbe_lun_number_t lun_number);
static void phineas_and_ferb_test_set_lun(fbe_lun_number_t lun_number);
static void phineas_and_ferb_test_unset_lun(void);
static void phineas_and_ferb_test_set_to_db_lun(void);

static void phineas_and_ferb_test_start_transaction(fbe_persist_transaction_handle_t *transact_handle);
static void phineas_and_ferb_test_abort_transaction(fbe_persist_transaction_handle_t transact_handle);
static void phineas_and_ferb_test_write_entry(fbe_persist_transaction_handle_t tran_handle);
static void phineas_and_ferb_test_init_buffer(void);
static void phineas_and_ferb_test_fill_all_entries(fbe_persist_sector_type_t target_sector);
static void phineas_and_ferb_test_fill_multiple_entries(void);
static void phineas_and_ferb_test_delete_entrie(void);
static fbe_status_t phineas_and_ferb_commit_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context);
static void phineas_and_ferb_test_commit(void);
static fbe_status_t phineas_and_ferb_set_lun_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context);
static void phineas_and_ferb_test_read(void);
static fbe_status_t phineas_and_ferb_read_completion_function(fbe_status_t read_status, fbe_persist_entry_id_t next_entry, fbe_u32_t entries_read, fbe_persist_completion_context_t context);
static void phineas_and_ferb_test_delete_all_entries(void);
static void phineas_and_ferb_test_verify_read_buffer(fbe_u32_t entries, fbe_u32_t write_data_offset);
static void phineas_and_ferb_dual_sp_test(void);
static void phineas_and_ferb_test_single_read(void);
static fbe_status_t phineas_and_ferb_read_single_entry_completion_function(fbe_status_t read_status, fbe_persist_completion_context_t context,fbe_persist_entry_id_t next_entry);
static void phineas_and_ferb_test_reboot_on_single_sp(void);
static void phineas_and_ferb_get_layout_info (void);
static void phineas_and_ferb_test_single_entry_operations(void);
static fbe_status_t phineas_and_ferb_test_single_operation_completion(fbe_status_t read_status,
																	  fbe_persist_entry_id_t entry_id,
																	  fbe_persist_completion_context_t context);
static fbe_u64_t end_address = 0;

/*************************
 *   LOCAL DEFINITIONS
 *************************/
typedef struct phineas_and_ferb_bogus_struct_s{
	fbe_persist_user_header_t	persist_header;/*can be used with any data that is persisted*/
	fbe_u64_t					number;/* <-- this is where the data we care about starts*/
	fbe_u8_t					some_data[18];
	fbe_u8_t					unused [(512 * 4) - 18 - (sizeof(fbe_u64_t) * 2)];/*pad to use a full 2K buffer, but w/o the persist header*/
	fbe_u64_t					tail_pattern;/*to make sure our read are good all the way, we put this at the end*/
}phineas_and_ferb_bogus_struct_t;

typedef struct phineas_and_ferb_read_context_s{
	fbe_semaphore_t sem;
	fbe_persist_entry_id_t	next_entry_id;
	fbe_u32_t				entries_read;
}phineas_and_ferb_read_context_t;

typedef struct phineas_and_ferb_single_entry_context_s{
	fbe_semaphore_t sem;
	fbe_persist_entry_id_t	received_entry_id;
	fbe_bool_t				expect_error;
}phineas_and_ferb_single_entry_context_t;

static phineas_and_ferb_bogus_struct_t	* 	my_data, *my_data_head;
static fbe_u8_t *							phineas_and_ferb_read_buffer = NULL;

#define PHINEAS_AND_FERB_MAX_ENTRY FBE_PERSIST_TRAN_ENTRY_MAX
/*************************
 *   TEST DESCRIPTION
 *************************/
char * phineas_and_ferb_short_desc = "Verifies interfaces for persist service";
char * phineas_and_ferb_long_desc =
"This scenario is simulating a client using the persistance service and will make sure we can start a\n"
"transaction, delete it, persist and read back a bunch of entried\n"
;



/**********************************************************************************/

void phineas_and_ferb_test_init(void)
{
	fbe_test_rg_configuration_t *rg_config_p = NULL;
	/* create a r5 rg */
    sep_standard_logical_config_get_rg_configuration(FBE_TEST_SEP_RG_CONFIG_TYPE_ONE_R5, &rg_config_p);

	sep_standard_logical_config_create_rg_with_lun(rg_config_p, 0);  /* just create a RG */
    return;
}

void phineas_and_ferb_test_destroy(void)
{
	fbe_test_sep_util_destroy_neit_sep_physical();
	return;
}

void phineas_and_ferb_test(void)
{
    fbe_persist_transaction_handle_t	tran_handle, new_tran_handle;

    /*allocate a buffer for the data we want to send down*/
    phineas_and_ferb_test_init_buffer();

    /*create the lu we will write to*/
    phineas_and_ferb_test_create_lun(101);

	phineas_and_ferb_test_unset_lun();
    /*let persist use it*/
    phineas_and_ferb_test_set_lun(101);
    
	/*is db layout ok*/
	phineas_and_ferb_get_layout_info();

    /*start a transaction*/
	phineas_and_ferb_test_start_transaction(&tran_handle);

	/*abort before doing anything*/
	phineas_and_ferb_test_abort_transaction(tran_handle);

	/*start a new one*/
	phineas_and_ferb_test_start_transaction(&new_tran_handle);
	MUT_ASSERT_TRUE(new_tran_handle != tran_handle);    

    /*write an entry into the transaction*/
	phineas_and_ferb_test_write_entry(new_tran_handle);
	phineas_and_ferb_test_abort_transaction(new_tran_handle);

	/*test a full transaction write with all entries*/
	phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS);
    phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
	phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS);
	phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA);
	phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION);
	phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_SCRATCH_PAD);
	phineas_and_ferb_test_fill_all_entries(FBE_PERSIST_SECTOR_TYPE_DIEH_RECORD);
	
	/*do a few transactions one after the other so we make the tables inside the service switch to new 64 bit entries*/
	phineas_and_ferb_test_fill_multiple_entries();

	/*test handling of entry deletion*/
	phineas_and_ferb_test_delete_entrie();

	/*test a transaction commit*/
	phineas_and_ferb_test_commit();

	/*test a read of a single entry*/
	phineas_and_ferb_test_single_read();

    /*test single entry operations*/
	phineas_and_ferb_test_single_entry_operations();

	/*test a full sector read*/
	phineas_and_ferb_test_read();

	/*test how we work after a reboot*/
	phineas_and_ferb_test_reboot_on_single_sp();

	/*dual sp testing*/
	//phineas_and_ferb_dual_sp_test();
	phineas_and_ferb_test_unset_lun();
    phineas_and_ferb_test_set_to_db_lun();

	phineas_and_ferb_test_destroy_lun(101);
    free(my_data_head);
	free(phineas_and_ferb_read_buffer);

	return;

}

static void phineas_and_ferb_test_create_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                    status;
    fbe_api_lun_create_t        		fbe_lun_create_req;
	fbe_object_id_t					    lu_id;
    fbe_job_service_error_type_t        job_error_type;
   
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    fbe_lun_create_req.raid_group_id = 5; /*sep_standard_logical_config_one_r5 is 5 so this is what we use*/
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

    mut_printf(MUT_LOG_TEST_STATUS, " %s: LUN 0x%X created successfully!", __FUNCTION__, lu_id);

    return;

} 

static void phineas_and_ferb_test_destroy_lun(fbe_lun_number_t lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t       			fbe_lun_destroy_req;
    fbe_job_service_error_type_t            job_error_type;
    
    /* Destroy a LUN */
    fbe_lun_destroy_req.lun_number = lun_number;

    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE, 100000, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, " LUN destroyed successfully!");

    return;
}


static void phineas_and_ferb_test_set_lun(fbe_lun_number_t lun_number)
{
	fbe_status_t		status;
	fbe_object_id_t		lu_object_id;
	fbe_semaphore_t		sem;
	fbe_status_t		wait_status;

	fbe_semaphore_init(&sem, 0, 1);

	status = fbe_api_database_lookup_lun_by_number(lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_persist_set_lun(lu_object_id,
									 phineas_and_ferb_set_lun_completion_function,
									 &sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);

	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	fbe_semaphore_destroy(&sem);

	mut_printf(MUT_LOG_TEST_STATUS, " Persist Set LUN successfull");
}

static void phineas_and_ferb_test_unset_lun(void)
{
	fbe_status_t		status;

    status = fbe_api_persist_unset_lun();
	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	mut_printf(MUT_LOG_TEST_STATUS, " Persist Unset LUN successfull");
}

static void phineas_and_ferb_test_set_to_db_lun(void)
{
	fbe_status_t		status;
	fbe_semaphore_t		sem;
	fbe_status_t		wait_status;

	fbe_semaphore_init(&sem, 0, 1);

    status = fbe_api_persist_set_lun(10,
									 phineas_and_ferb_set_lun_completion_function,
									 &sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);

	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	fbe_semaphore_destroy(&sem);

	mut_printf(MUT_LOG_TEST_STATUS, " Persist Set DB LUN successfull");

}


static void phineas_and_ferb_test_start_transaction(fbe_persist_transaction_handle_t *transact_handle)
{
	fbe_status_t 	status;

	status = fbe_api_persist_start_transaction(transact_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, " Persist Get handle successfull, handle:0x%llX", (unsigned long long)(*transact_handle));
}

static void phineas_and_ferb_test_abort_transaction(fbe_persist_transaction_handle_t transact_handle)
{
	fbe_status_t 	status;

	status = fbe_api_persist_abort_transaction(transact_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, " Persist Abort Trans. successfull, handle:0x%llX", (unsigned long long)transact_handle);

}


static void phineas_and_ferb_test_write_entry(fbe_persist_transaction_handle_t transact_handle)
{
	fbe_status_t 				status;

	my_data = my_data_head;
	/*lets write 100 bytes first(yes, we don't write the full data structure)*/
	status = fbe_api_persist_write_entry(transact_handle,
										 FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
										 (fbe_u8_t *)(&my_data->number),
										 100,
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, " Persist write short entry successfull, id:0x%llX", (unsigned long long)my_data->persist_header.entry_id);
    
	my_data++;
	/*now something that spans multiple 2 blocks (yes, we don't write the full data structure)*/
	status = fbe_api_persist_write_entry(transact_handle,
										 FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
										 (fbe_u8_t *)(&my_data->number),
										 560,
										 &my_data->persist_header.entry_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, " Persist write long entry successfull, id:0x%llX", (unsigned long long)my_data->persist_header.entry_id);
    
	my_data++;
	/*fill the whole entry*/
	status = fbe_api_persist_write_entry(transact_handle,
										 FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
										(fbe_u8_t *) (&my_data->number),
										 512 * 4,
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, " Persist write full entry successfull, id:0x%llX", (unsigned long long)my_data->persist_header.entry_id);
    
    /*modify the transaction we just wrote (we don't really modify data but just testing the interface*/
	status = fbe_api_persist_modify_entry(transact_handle,
										  (fbe_u8_t *)my_data,
										  512 * 4,
										  my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, " Persist modify entry successfull, id:0x%llX", (unsigned long long)my_data->persist_header.entry_id);
    
	my_data++;

	/*try to overflow*/
	status = fbe_api_persist_write_entry(transact_handle,
										 FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS,
										 (fbe_u8_t *)(&my_data->number),
										 (512 * 4)+ 1,
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);
    
	mut_printf(MUT_LOG_TEST_STATUS, " Persist write illegal entry successfull");

}

static void phineas_and_ferb_test_init_buffer()
{
	fbe_u32_t	count = 0;

    my_data = malloc (16 * PHINEAS_AND_FERB_MAX_ENTRY * sizeof (phineas_and_ferb_bogus_struct_t));/*create 16 * PHINEAS_AND_FERB_MAX_ENTRY entries*/
	MUT_ASSERT_TRUE(my_data != NULL);

	my_data_head = my_data;/*keep for later use*/
	fbe_zero_memory(my_data, 16 * PHINEAS_AND_FERB_MAX_ENTRY * sizeof (phineas_and_ferb_bogus_struct_t));

	end_address = (fbe_u64_t)my_data_head + (16 * PHINEAS_AND_FERB_MAX_ENTRY) * sizeof (phineas_and_ferb_bogus_struct_t);

	/*fill some data*/
	for (count = 0; count < (16 * PHINEAS_AND_FERB_MAX_ENTRY); count ++) {
		my_data->number = count;
		sprintf(my_data->some_data, "0x%llX" ,
			(unsigned long long)my_data->number);
		my_data->persist_header.entry_id = 0xFFFFFFFFFFFFFFFF;
		my_data->tail_pattern = 0xFFFFFFFFFFFFFFFF - count;
		my_data++;
	}

	/*init the read buffer*/
	phineas_and_ferb_read_buffer = malloc(FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);/*must use this size per API*/
	MUT_ASSERT_TRUE(phineas_and_ferb_read_buffer != NULL);

}

static void phineas_and_ferb_test_fill_all_entries(fbe_persist_sector_type_t target_sector)
{
	fbe_u32_t 							entry;
	fbe_status_t							status;
	fbe_persist_transaction_handle_t	handle;
	fbe_u32_t							max_entries = PHINEAS_AND_FERB_MAX_ENTRY;/*normal case*/

	if (FBE_PERSIST_SECTOR_TYPE_DIEH_RECORD == target_sector) {
		/*this is a small sector*/
		max_entries = 64;
	}
    
	my_data++;/*go to the next usable one*/

	status = fbe_api_persist_start_transaction(&handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	for (entry = 0; entry < max_entries; entry ++) {
		status = fbe_api_persist_write_entry(handle,
											 target_sector,
											 (fbe_u8_t *)(&my_data->number),
											 (512 * 2),
											 &my_data->persist_header.entry_id);

		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	}

	my_data++;

	/*before we get out, let's try to add another one and expect error*/
	status = fbe_api_persist_write_entry(handle,
										 target_sector,
										 (fbe_u8_t *)(&my_data->number),
									     (512 * 2),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);


	status = fbe_api_persist_abort_transaction(handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, " Persist write full transaction successfull, sector: %d", target_sector);
}

static void phineas_and_ferb_test_delete_entrie(void)
{
    fbe_status_t							status;
	fbe_persist_transaction_handle_t	handle;
    
	my_data++;/*go to the next usable one*/

	status = fbe_api_persist_start_transaction(&handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
	/*write an entry*/
	status = fbe_api_persist_write_entry(handle,
										 FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
										 (fbe_u8_t *)(&my_data->number),
									     (512 * 4),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
	my_data++;
	/*write an entry*/
	status = fbe_api_persist_write_entry(handle,
										 FBE_PERSIST_SECTOR_TYPE_ESP_OBJECTS,
										 (fbe_u8_t *)(&my_data->number),
									     (512 * 4),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /*delete it*/
	status = fbe_api_persist_delete_entry(handle,
										 my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*try to modify it(should fail)*/
	status = fbe_api_persist_modify_entry(handle,
										  (fbe_u8_t *)my_data,
										  512 * 4,
										  my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

    /*this one should work*/
	my_data--;
	status = fbe_api_persist_modify_entry(handle,
										  (fbe_u8_t *)my_data,
										  512 * 4,
										  my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
	status = fbe_api_persist_abort_transaction(handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, " Persist delete entry successfull");

}

static void phineas_and_ferb_test_commit(void)
{
	fbe_status_t							status;
	fbe_persist_transaction_handle_t	handle;
    fbe_semaphore_t						sem;
	fbe_status_t						wait_status;

	fbe_semaphore_init(&sem, 0, 1);

	my_data++;/*go to the next usable one*/

	status = fbe_api_persist_start_transaction(&handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


	/*write an entry*/
	status = fbe_api_persist_write_entry(handle,
										 FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
										(fbe_u8_t *) (my_data + sizeof(fbe_persist_user_header_t)),
										 (512 * 4),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	my_data++;
	/*write another one */
	status = fbe_api_persist_write_entry(handle,
										 FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION,
										(fbe_u8_t *) (my_data + sizeof(fbe_persist_user_header_t)),
										 (512 * 4),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*write an entry*/
	my_data++;
	status = fbe_api_persist_write_entry(handle,
										 FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
										 (fbe_u8_t *)(&my_data->number),
										 (512 * 4),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*delete it*/
	status = fbe_api_persist_delete_entry(handle,
										 my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, " Starting to commit transaction");

	/*now let's commit it*/
	status = fbe_api_persist_commit_transaction(handle,
												phineas_and_ferb_commit_completion_function,
												(fbe_persist_completion_context_t)&sem);

	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

    fbe_semaphore_destroy(&sem);

	mut_printf(MUT_LOG_TEST_STATUS, " Commit success");
}

static fbe_status_t phineas_and_ferb_commit_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context)
{
	fbe_semaphore_t *	sem = (fbe_semaphore_t *)context;
	MUT_ASSERT_TRUE(commit_status == FBE_STATUS_OK);
	fbe_semaphore_release(sem, 0, 1 ,FALSE);

	return FBE_STATUS_OK;
}

static fbe_status_t phineas_and_ferb_set_lun_completion_function (fbe_status_t commit_status, fbe_persist_completion_context_t context)
{
	fbe_semaphore_t *	sem = (fbe_semaphore_t *)context;
	MUT_ASSERT_TRUE(commit_status == FBE_STATUS_OK);
	fbe_semaphore_release(sem, 0, 1 ,FALSE);

	return FBE_STATUS_OK;
}

/*make sure we go over the 64 bit map entry and jump to the next one*/
static void phineas_and_ferb_test_fill_multiple_entries(void)
{
	fbe_u32_t 							entry;
	fbe_status_t						status;
	fbe_persist_transaction_handle_t	handle;
	fbe_u32_t							bitmpa_entry = 0;
	fbe_semaphore_t						sem;
	fbe_status_t						wait_status;
	fbe_u64_t							expected_entry_id = 0;
	fbe_persist_sector_type_t 			target_sector = FBE_PERSIST_SECTOR_TYPE_SEP_EDGES;

	my_data++;/*go to the next usable one*/

	fbe_semaphore_init(&sem, 0, 1);

	/*write PHINEAS_AND_FERB_MAX_ENTRY * 2 entried so we go over the 64 bits in each table entry in the service itself*/
    for (bitmpa_entry = 0; bitmpa_entry < 2; bitmpa_entry++) {
		/*start a new transaction of PHINEAS_AND_FERB_MAX_ENTRY entries*/
		status = fbe_api_persist_start_transaction(&handle);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

		for (entry = 0; entry < PHINEAS_AND_FERB_MAX_ENTRY; entry ++) {
			status = fbe_api_persist_write_entry(handle,
												 target_sector,
												 (fbe_u8_t *)(&my_data->number ),
												 (512 * 4),
												 &my_data->persist_header.entry_id);
	
			MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

			my_data++;
		}

		/*now let's commit it*/
		status = fbe_api_persist_commit_transaction(handle,
													phineas_and_ferb_commit_completion_function,
													(fbe_persist_completion_context_t)&sem);

		wait_status = fbe_semaphore_wait_ms(&sem, 10000);
		MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);
		
	}
    
	fbe_semaphore_destroy(&sem);
	
	my_data--;

	/*verify the entry ID matches what we expect*/
	expected_entry_id = 0x200000000;/*FBE_PERSIST_SECTOR_TYPE_SEP_EDGES is 2*/
	expected_entry_id |= ((2 * PHINEAS_AND_FERB_MAX_ENTRY) - 1);
	
	MUT_ASSERT_TRUE(my_data->persist_header.entry_id == expected_entry_id);
	
	mut_printf(MUT_LOG_TEST_STATUS, " Persist write multiple transactions successfull, sector: %d", target_sector);
}


static fbe_status_t phineas_and_ferb_read_completion_function(fbe_status_t read_status, fbe_persist_entry_id_t next_entry, fbe_u32_t entries_read, fbe_persist_completion_context_t context)
{
	phineas_and_ferb_read_context_t	*	read_context = (phineas_and_ferb_read_context_t	*)context;
	
	MUT_ASSERT_TRUE(read_status == FBE_STATUS_OK);
	read_context->next_entry_id= next_entry;
	read_context->entries_read = entries_read;
	fbe_semaphore_release(&read_context->sem, 0, 1 ,FALSE);
	return FBE_STATUS_OK;
}

static void phineas_and_ferb_test_read(void)
{

	fbe_u32_t 							entry;
	fbe_status_t						status;
	fbe_persist_transaction_handle_t	handle;
    fbe_u32_t							bitmpa_entry = 0;
    fbe_status_t						wait_status;
	phineas_and_ferb_read_context_t	*	read_context = (phineas_and_ferb_read_context_t *)malloc (sizeof(phineas_and_ferb_read_context_t));
	fbe_u32_t							total_entries_read = 0;
	fbe_u64_t							temp_entry_id = 0;

	read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;/*start to read from the first one*/
    
	fbe_semaphore_init(&read_context->sem, 0, 1);

	/*beofre we do the read test, let's delte all the entries we did so far so we can start with
	a fresh disk which we can compare to our buffer*/
	phineas_and_ferb_test_delete_all_entries();

	my_data = my_data_head;

	mut_printf(MUT_LOG_TEST_STATUS, " ==Writing sector with 16 full entries ==");
	/*write PHINEAS_AND_FERB_MAX_ENTRY * 16 entries so we write all of our entries*/
    for (bitmpa_entry = 0; bitmpa_entry < 16; bitmpa_entry++) {
		/*start a new transaction of PHINEAS_AND_FERB_MAX_ENTRY entries*/
		status = fbe_api_persist_start_transaction(&handle);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

		for (entry = 0; entry < PHINEAS_AND_FERB_MAX_ENTRY; entry ++) {
			/*our data actually starts after the header, we just lumped them together here for convenience*/
			status = fbe_api_persist_write_entry(handle,
												 FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
												 (fbe_u8_t *)(&my_data->number),
												 (512 * 4),
												 &my_data->persist_header.entry_id);
	
			MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

			my_data++;
		}

		/*now let's commit it*/
		status = fbe_api_persist_commit_transaction(handle,
													phineas_and_ferb_commit_completion_function,
													(fbe_persist_completion_context_t)&read_context->sem);

		MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
		wait_status = fbe_semaphore_wait_ms(&read_context->sem, 10000);
		MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);
		
	}
    
	mut_printf(MUT_LOG_TEST_STATUS, " == Read back sector for 16 entries, read size %llu bytes ==", (unsigned long long)FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
	/*now that we are done writing let's read from the start of the sector until we get FBE_PERSIST_NO_MORE_ENTRIES_TO_READ*/
	do {
		status = fbe_api_persist_read_sector(FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
											 phineas_and_ferb_read_buffer,
											 FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
											 read_context->next_entry_id,
											 phineas_and_ferb_read_completion_function,
											 (fbe_persist_completion_context_t)read_context);

		MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
		wait_status = fbe_semaphore_wait_ms(&read_context->sem, 10000);
		MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

		if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
			mut_printf(MUT_LOG_TEST_STATUS, " Read %d entries from sector: %d up to entry 0x%llX", read_context->entries_read, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES, (unsigned long long)(read_context->next_entry_id-1));
		}else{
			mut_printf(MUT_LOG_TEST_STATUS, " Read %d entries from sector: %d up to sector end", read_context->entries_read, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
		}

		/*before we verify the whole buffer we want to verify we got the first entry ID in the sector since we deleted everything before,
		this will make sure that the delete really works*/
		if (FBE_PERSIST_SECTOR_START_ENTRY_ID == read_context->next_entry_id) {
			temp_entry_id = (fbe_u64_t)phineas_and_ferb_read_buffer;
			MUT_ASSERT_TRUE(temp_entry_id != 0x200000000);
		}

		/*now let's verify the read buffer content comparing to the write buffer*/
        phineas_and_ferb_test_verify_read_buffer(read_context->entries_read, total_entries_read);
		total_entries_read+= read_context->entries_read;/*increment only AFTER comparing*/

	} while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);

    fbe_semaphore_destroy(&read_context->sem);
	free(read_context);

	mut_printf(MUT_LOG_TEST_STATUS, " Finished reading and compare sector:%d", FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
}

static void phineas_and_ferb_test_delete_all_entries(void)
{
	fbe_u32_t 							entry;
	fbe_status_t						status;
	fbe_persist_transaction_handle_t	handle;
    fbe_u32_t							bitmpa_entry = 0;
    fbe_semaphore_t						sem;
	fbe_status_t						wait_status;
	fbe_bool_t							deleted = FBE_FALSE;

	my_data = my_data_head;

	fbe_semaphore_init(&sem, 0, 1);
	
	/*go over all PHINEAS_AND_FERB_MAX_ENTRY * 16 entries so we delete all of our entries*/
    for (bitmpa_entry = 0; bitmpa_entry < 16; bitmpa_entry++) {
		/*start a new transaction of PHINEAS_AND_FERB_MAX_ENTRY entries*/
		deleted = FBE_FALSE;
		status = fbe_api_persist_start_transaction(&handle);
		MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

		for (entry = 0; entry < PHINEAS_AND_FERB_MAX_ENTRY; entry ++) {
			if (my_data->persist_header.entry_id != 0xFFFFFFFFFFFFFFFF) {
				/*this may cause failure trace, it's fine, we get it because we created entries in memory here but we later aborted the whole transaction
				but did not clear the entries*/
				fbe_api_persist_delete_entry(handle,my_data->persist_header.entry_id);
                deleted = FBE_TRUE;
				my_data->persist_header.entry_id = 0xFFFFFFFFFFFFFFFF;
			}

			my_data++;
		}

		/*now let's commit it*/
		if (FBE_IS_TRUE(deleted)) {
			status = fbe_api_persist_commit_transaction(handle,
														phineas_and_ferb_commit_completion_function,
														(fbe_persist_completion_context_t)&sem);
	
			MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
			wait_status = fbe_semaphore_wait_ms(&sem, 10000);
			MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);
		}else{
			status = fbe_api_persist_abort_transaction(handle);
			MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
		}
		
	}

	fbe_semaphore_destroy(&sem);
}

static void phineas_and_ferb_test_verify_read_buffer(fbe_u32_t elements_read, fbe_u32_t write_data_offset)
{
	fbe_u8_t * 							read_buffer = phineas_and_ferb_read_buffer;
	phineas_and_ferb_bogus_struct_t	* 	write_data = my_data_head;
	fbe_u32_t							elements = 0;
	fbe_persist_user_header_t *			user_header = NULL;
	fbe_u32_t							mem_equal = 0;

	write_data += write_data_offset;/*if it's not the first read, move forward*/

	/*let's go over the read buffer and see if it matches what we wrote. The read buffer is made off 400 elements,
	in front of each one is the entry id*/
	for (elements = 0; elements < elements_read; elements++) {
		if ((write_data_offset + elements) >= (16 * PHINEAS_AND_FERB_MAX_ENTRY)) {
			/*that's it, from now on it should be just empty entries read from the disk because we wrote (16 * PHINEAS_AND_FERB_MAX_ENTRY) entries*/
			user_header = (fbe_persist_user_header_t *)read_buffer;
			MUT_ASSERT_TRUE_MSG(user_header->entry_id == 0, "Miscompare in entry ID")
			
		}else{
			/*let's compare the entry ID first*/
			user_header = (fbe_persist_user_header_t *)read_buffer;
			MUT_ASSERT_INTEGER_EQUAL_MSG(write_data->persist_header.entry_id, user_header->entry_id, "Miscompare in entry ID")
	
			/*now the actual data*/
			read_buffer += sizeof(fbe_persist_user_header_t);
			mem_equal = memcmp(read_buffer, &write_data->number, (sizeof(phineas_and_ferb_bogus_struct_t) - sizeof(fbe_persist_user_header_t)));
	
			MUT_ASSERT_TRUE_MSG(mem_equal ==0, "Miscompare in Data");
		}

		/*go to next entry*/
		read_buffer += (sizeof(phineas_and_ferb_bogus_struct_t) - sizeof(fbe_persist_user_header_t));
		write_data ++;
	}
    
}

static void phineas_and_ferb_dual_sp_test(void)
{
#if 0
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_status_t						wait_status = FBE_STATUS_GENERIC_FAILURE;
	phineas_and_ferb_read_context_t	*	read_context = (phineas_and_ferb_read_context_t *)malloc (sizeof(phineas_and_ferb_read_context_t));
#endif

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

	/*let's wait for the SP to finish it's synch*/
	fbe_api_sleep(2000);

    /*and bring down SPA so SPB can take control*/
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

	
	#if 0 /*This test needs the disk to be the same one on both SPs.
		enable when we get rid of old persistence serive. Cuurently it needs a big disk space so a file system disk will not work
	   */

	/*at this stage, SPB is supposed to take over the persistence so if we run the read test again we should be
	able to read and compare exactly what we had on the disk when the peer wrote it*/
	read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;/*start to read from the first one*/
    
	fbe_semaphore_init(&read_context->sem, 0, 1);

	/*let's read from the start of the sector until we get FBE_PERSIST_NO_MORE_ENTRIES_TO_READ*/
	do {
		status = fbe_api_persist_read_sector(FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
											 phineas_and_ferb_read_buffer,
											 FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
											 read_context->next_entry_id,
											 phineas_and_ferb_read_completion_function,
											 (fbe_persist_completion_context_t)read_context);

		MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
		wait_status = fbe_semaphore_wait_ms(&read_context->sem, 10000);
		MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

		if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
			mut_printf(MUT_LOG_TEST_STATUS, " Read %d entries from sector: %d up to entry 0x%llX", read_context->entries_read, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES, read_context->next_entry_id-1 );
		}else{
			mut_printf(MUT_LOG_TEST_STATUS, " Read %d entries from sector: %d up to sector end", read_context->entries_read, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
		}

		/*now let's veryfy the read buffer content comparing to the write buffer*/
        phineas_and_ferb_test_verify_read_buffer(read_context->entries_read, total_entries_read);
		total_entries_read+= read_context->entries_read;/*increment only AFTER comparing*/

	} while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);

    fbe_semaphore_destroy(&read_context->sem);
	free(read_context);
#endif
	mut_printf(MUT_LOG_TEST_STATUS, "Peer SP recovered and finished reading and compare sector:%d ", FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
}


/*make sure we can read a single entry(not sure who's gonna use it though.....)*/
static void phineas_and_ferb_test_single_read(void)
{
	fbe_status_t						status;
	fbe_persist_transaction_handle_t	handle;
    fbe_status_t						wait_status;
	fbe_semaphore_t 					sem;
	fbe_u32_t							mem_equal;
    
    fbe_semaphore_init(&sem, 0, 1);

    my_data++; /*go to the next valid entry*/
   
	/*start a new transaction*/
	status = fbe_api_persist_start_transaction(&handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	
	/*our data actually starts after the header, we just lumped them together here for convenience*/
	status = fbe_api_persist_write_entry(handle,
										 FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
										 (fbe_u8_t *)(&my_data->number),
										 (512 * 4),
										 &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*now let's commit it*/
	status = fbe_api_persist_commit_transaction(handle,
												phineas_and_ferb_commit_completion_function,
												(fbe_persist_completion_context_t)&sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);
    

	/*now that we are done writing let's read */
	status = fbe_api_persist_read_single_entry(my_data->persist_header.entry_id,
											   phineas_and_ferb_read_buffer,
											   FBE_PERSIST_DATA_BYTES_PER_ENTRY,/*we have a bigger buffer but we just cheat*/
                                               phineas_and_ferb_read_single_entry_completion_function,
											   &sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_TEST_STATUS, " Read entry 0x%llX from sector: %d",
		   (unsigned long long)my_data->persist_header.entry_id,
		   FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA);
    
	/*now let's verify the read buffer content comparing to the write buffer*/
	mem_equal = memcmp(phineas_and_ferb_read_buffer, &my_data->number, (sizeof(phineas_and_ferb_bogus_struct_t) - sizeof(fbe_persist_user_header_t)));
    MUT_ASSERT_TRUE_MSG(mem_equal == 0, "Miscompare in Data");

	/*now let's test the API that writes and entry and then ask for the first 64 bits to be written with the entry ID*/

	my_data++; /*go to the next valid entry*/

	/*start a new transaction*/
	status = fbe_api_persist_start_transaction(&handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*our data actually starts after the header, we just lumped them together here for convenience*/
	status = fbe_api_persist_write_entry_with_auto_entry_id_on_top(handle,
																   FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA,
																   (fbe_u8_t *)(&my_data->number),
																   (512 * 4),
																   &my_data->persist_header.entry_id);

	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*now let's commit it*/
	status = fbe_api_persist_commit_transaction(handle,
												phineas_and_ferb_commit_completion_function,
												(fbe_persist_completion_context_t)&sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);
    

	/*now that we are done writing let's read */
	status = fbe_api_persist_read_single_entry(my_data->persist_header.entry_id,
											   phineas_and_ferb_read_buffer,
											   FBE_PERSIST_DATA_BYTES_PER_ENTRY,/*we have a bigger buffer but we just cheat*/
                                               phineas_and_ferb_read_single_entry_completion_function,
											   &sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_TEST_STATUS, " Read entry 0x%llX from sector: %d",
		   (unsigned long long)my_data->persist_header.entry_id,
		   FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA);
    
	/*now let's verify the first entry we read is not the 'number' but it's the entry ID*/
	mem_equal = memcmp(phineas_and_ferb_read_buffer, &my_data->persist_header.entry_id, sizeof(fbe_persist_entry_id_t));
    MUT_ASSERT_TRUE_MSG(mem_equal == 0, "Miscompare in Data");
    
    fbe_semaphore_destroy(&sem);

	mut_printf(MUT_LOG_TEST_STATUS, " Finished reading and compare single entry:0x%llX", (unsigned long long)my_data->persist_header.entry_id);

}

static fbe_status_t phineas_and_ferb_read_single_entry_completion_function(fbe_status_t read_status, 
																		   fbe_persist_completion_context_t context, 
																		   fbe_persist_entry_id_t next_entry)
{
	fbe_semaphore_t *	sem = (fbe_semaphore_t *)context;
	MUT_ASSERT_TRUE(read_status == FBE_STATUS_OK);
	fbe_semaphore_release(sem, 0, 1 ,FALSE);

	return FBE_STATUS_OK;
}

/*this test runs after the read test. We take the SP down, zero the read buffer and read again after the reboot.
Everything must match since we did not take downt he drives yet*/
static void phineas_and_ferb_test_reboot_on_single_sp(void)
{
    fbe_status_t						status;
    fbe_status_t						wait_status;
	phineas_and_ferb_read_context_t	*	read_context = (phineas_and_ferb_read_context_t *)malloc (sizeof(phineas_and_ferb_read_context_t));
	fbe_u32_t							total_entries_read = 0;

	mut_printf(MUT_LOG_TEST_STATUS, "=== Read reboot test ===");

    /* Shutdown the SEP */
    mut_printf(MUT_LOG_LOW, "%s === Stopping Storage Extent Package(SEP) ===", __FUNCTION__);
    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "unload sep failed");

    /* restart the SEP */
	sep_config_load_sep();

	/*let system stabilize a bit*/
	fbe_api_sleep(7000);

   	phineas_and_ferb_test_unset_lun();
    /*let persist use it*/
    phineas_and_ferb_test_set_lun(101);

    read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;/*start to read from the first one*/
	
	fbe_semaphore_init(&read_context->sem, 0, 1);

	fbe_zero_memory(phineas_and_ferb_read_buffer, FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);

    /*now that we are done booting let's read from the start of the sector until we get FBE_PERSIST_NO_MORE_ENTRIES_TO_READ*/
	do {
		status = fbe_api_persist_read_sector(FBE_PERSIST_SECTOR_TYPE_SEP_EDGES,
											 phineas_and_ferb_read_buffer,
											 FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
											 read_context->next_entry_id,
											 phineas_and_ferb_read_completion_function,
											 (fbe_persist_completion_context_t)read_context);

		MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
		wait_status = fbe_semaphore_wait_ms(&read_context->sem, 20000);
		MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

		if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
			mut_printf(MUT_LOG_TEST_STATUS, " Read %d entries from sector: %d up to entry 0x%llX", read_context->entries_read, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES, (unsigned long long)(read_context->next_entry_id-1));
		}else{
			mut_printf(MUT_LOG_TEST_STATUS, " Read %d entries from sector: %d up to sector end", read_context->entries_read, FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);
		}

		/*now let's veryfy the read buffer content comparing to the write buffer*/
        phineas_and_ferb_test_verify_read_buffer(read_context->entries_read, total_entries_read);
		total_entries_read+= read_context->entries_read;/*increment only AFTER comparing*/

	} while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);
	
    fbe_semaphore_destroy(&read_context->sem);
	free(read_context);

	mut_printf(MUT_LOG_TEST_STATUS, " Finished reboot reading and compare sector:%d", FBE_PERSIST_SECTOR_TYPE_SEP_EDGES);

}

static void phineas_and_ferb_get_layout_info (void)
{
	fbe_persist_control_get_layout_info_t		get_info;
	fbe_status_t								status;
	fbe_lba_t									total_blocks;

	status  = fbe_api_persist_get_total_blocks_needed(&total_blocks);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "Persist LUN will need 0x%llX blocks",
		   (unsigned long long)total_blocks);

	status = fbe_api_persist_get_layout_info(&get_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*make sure order is followed*/
	MUT_ASSERT_TRUE(get_info.lun_object_id != FBE_OBJECT_ID_INVALID);
	MUT_ASSERT_TRUE(get_info.journal_start_lba == FBE_PERSIST_START_LBA + 1);
	MUT_ASSERT_TRUE(get_info.sep_object_start_lba > get_info.journal_start_lba);
	MUT_ASSERT_TRUE(get_info.sep_edges_start_lba > get_info.sep_object_start_lba);
	MUT_ASSERT_TRUE(get_info.sep_admin_conversion_start_lba > get_info.sep_edges_start_lba);
	MUT_ASSERT_TRUE(get_info.esp_objects_start_lba > get_info.sep_admin_conversion_start_lba);
	MUT_ASSERT_TRUE(get_info.system_data_start_lba > get_info.esp_objects_start_lba);

	mut_printf(MUT_LOG_TEST_STATUS, " verified sector layout read and order");
}

static void phineas_and_ferb_test_single_entry_operations(void)
{
	
	fbe_status_t 								status;
	phineas_and_ferb_single_entry_context_t		single_entry_context;
	fbe_status_t								wait_status;
	fbe_u32_t									mem_equal = 0;

	my_data = my_data_head;
    
	mut_printf(MUT_LOG_TEST_STATUS, " Testing single write/modify/delete operations\n");

	fbe_semaphore_init(&single_entry_context.sem, 0, 1);
	single_entry_context.expect_error = FBE_FALSE;

	/*write some stuff to the persist service as a single shot*/
	status  = fbe_api_persist_write_single_entry(FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS,
												 (fbe_u8_t *) (&my_data->number),
												 512 * 4,
												 phineas_and_ferb_test_single_operation_completion,
												 &single_entry_context);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	/*now read it back to make sure we wrote it ok*/
	fbe_zero_memory(phineas_and_ferb_read_buffer, FBE_PERSIST_DATA_BYTES_PER_ENTRY);

	status = fbe_api_persist_read_single_entry(single_entry_context.received_entry_id,
											   phineas_and_ferb_read_buffer,
											   FBE_PERSIST_DATA_BYTES_PER_ENTRY,/*we have a bigger buffer but we just cheat*/
                                               phineas_and_ferb_read_single_entry_completion_function,
											   &single_entry_context.sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_TEST_STATUS, " Read entry 0x%llX from sector: %d",
		   (unsigned long long)single_entry_context.received_entry_id,
		   FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS);
    
	/*now let's verify the read buffer content comparing to the write buffer*/
	mem_equal = memcmp(phineas_and_ferb_read_buffer, &my_data->number, (sizeof(phineas_and_ferb_bogus_struct_t) - sizeof(fbe_persist_user_header_t)));
    MUT_ASSERT_TRUE_MSG(mem_equal == 0, "Miscompare in Data");

	mut_printf(MUT_LOG_TEST_STATUS, " Modifying single entry\n");
	/*let's modify it*/
	my_data++;
	status  = fbe_api_persist_modify_single_entry(single_entry_context.received_entry_id,
												 (fbe_u8_t *) (&my_data->number),
												 512 * 4,
												 phineas_and_ferb_test_single_operation_completion,
												 &single_entry_context);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	/*and read back to see it's good*/
	fbe_zero_memory(phineas_and_ferb_read_buffer, FBE_PERSIST_DATA_BYTES_PER_ENTRY);

	status = fbe_api_persist_read_single_entry(single_entry_context.received_entry_id,
											   phineas_and_ferb_read_buffer,
											   FBE_PERSIST_DATA_BYTES_PER_ENTRY,/*we have a bigger buffer but we just cheat*/
                                               phineas_and_ferb_read_single_entry_completion_function,
											   &single_entry_context.sem);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	mut_printf(MUT_LOG_TEST_STATUS, " Read entry 0x%llX from sector: %d",
		   (unsigned long long)single_entry_context.received_entry_id,
		   FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS);
    
	/*now let's verify the read buffer content comparing to the write buffer*/
	mem_equal = memcmp(phineas_and_ferb_read_buffer, &my_data->number, (sizeof(phineas_and_ferb_bogus_struct_t) - sizeof(fbe_persist_user_header_t)));
    MUT_ASSERT_TRUE_MSG(mem_equal == 0, "Miscompare in Data");

	/*let's delete it*/
	mut_printf(MUT_LOG_TEST_STATUS, " Deleting single entry\n");
	status  = fbe_api_persist_delete_single_entry(single_entry_context.received_entry_id,
												 phineas_and_ferb_test_single_operation_completion,
												 &single_entry_context);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);

	/*and try to delete it again and get an error*/
	single_entry_context.expect_error = FBE_TRUE;

	status  = fbe_api_persist_delete_single_entry(single_entry_context.received_entry_id,
												 phineas_and_ferb_test_single_operation_completion,
												 &single_entry_context);

	MUT_ASSERT_TRUE(status != FBE_STATUS_GENERIC_FAILURE);
	wait_status = fbe_semaphore_wait_ms(&single_entry_context.sem, 10000);
	MUT_ASSERT_TRUE(wait_status != FBE_STATUS_TIMEOUT);
	
    fbe_semaphore_destroy(&single_entry_context.sem); /* SAFEBUG - needs destroy */

	mut_printf(MUT_LOG_TEST_STATUS, " Testing single write/modify/delete operations - Success !!!\n");

}


static fbe_status_t phineas_and_ferb_test_single_operation_completion(fbe_status_t op_status,
																	  fbe_persist_entry_id_t entry_id,
																	  fbe_persist_completion_context_t context)
{
	phineas_and_ferb_single_entry_context_t *	single_entry_context = (phineas_and_ferb_single_entry_context_t *)context;
	if (single_entry_context->expect_error) {
		MUT_ASSERT_TRUE(op_status == FBE_STATUS_GENERIC_FAILURE);
	}else{
		MUT_ASSERT_TRUE(op_status == FBE_STATUS_OK);
	}
	single_entry_context->received_entry_id = entry_id;
	fbe_semaphore_release(&single_entry_context->sem, 0, 1 ,FALSE);

	return FBE_STATUS_OK;

}
