#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_winddk.h"

/*
fbe_status_t fbe_api_drive_configuration_interface_get_port_table_entry(fbe_drive_configuration_handle_t handle, fbe_drive_configuration_port_record_t *entry);
*/
void fbe_api_drive_configuration_interface_get_handles_test(void)
{
	fbe_status_t 								status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_drive_config_get_handles_list_t	*	get_handles = NULL;
    fbe_u32_t									handle_count = 0;
    
	get_handles = (fbe_api_drive_config_get_handles_list_t *)fbe_api_allocate_memory (sizeof(fbe_api_drive_config_get_handles_list_t));
	MUT_ASSERT_TRUE(get_handles != NULL);
    
    /*let first read all the handles*/
	status = fbe_api_drive_configuration_interface_get_handles(get_handles, FBE_API_HANDLE_TYPE_DRIVE);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*make sure we have at least one entry*/
	MUT_ASSERT_TRUE(get_handles->total_count > 0);

    /*now with NULL pointer*/
	status = fbe_api_drive_configuration_interface_get_handles(NULL, FBE_API_HANDLE_TYPE_DRIVE);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_drive_configuration_interface_get_handles(get_handles, FBE_API_HANDLE_TYPE_INVALID);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	fbe_api_free_memory(get_handles);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;
}

void fbe_api_drive_configuration_interface_add_table_entry_test(void)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_drive_configuration_record_t *		record = NULL, *new_record = NULL;
	fbe_drive_configuration_handle_t		handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	fbe_bool_t								equal = FBE_FALSE;

    
	record = (fbe_drive_configuration_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(record != NULL);

	new_record = (fbe_drive_configuration_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(new_record != NULL);

	fbe_zero_memory(record, sizeof(fbe_drive_configuration_record_t));
	fbe_zero_memory(new_record, sizeof(fbe_drive_configuration_record_t));

	/*populate some data that would give us a hanlde back*/
	record->drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
	record->drive_info.drive_vendor = FBE_DRIVE_VENDOR_SIMULATION;
	fbe_sprintf(record->drive_info.fw_revision, strlen("A01"), "A01");
	fbe_sprintf(record->drive_info.part_number, strlen("AABBCCDD"), "AABBCCDD");
	fbe_sprintf(record->drive_info.serial_number_start, strlen("00001"), "00001");
	fbe_sprintf(record->drive_info.serial_number_end, strlen("10001"), "10001");

	record->threshold_info.data_stat.interval = 100;
	record->threshold_info.hardware_stat.weight = 200;

	/*let's try to call it without START first, it should fail*/
	status  = fbe_api_drive_configuration_interface_add_drive_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*now the right way*/
	fbe_api_drive_configuration_interface_start_update();
	status  = fbe_api_drive_configuration_interface_add_drive_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_api_drive_configuration_interface_end_update();

	/*try to get the same record and make sure we have a match with what we entered*/
	fbe_api_drive_configuration_interface_get_drive_table_entry(handle, new_record);

	equal = fbe_equal_memory(new_record, record, sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(equal == FBE_TRUE);

	/*now with some bad stuff*/
	status  = fbe_api_drive_configuration_interface_add_drive_table_entry (NULL, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status  = fbe_api_drive_configuration_interface_add_drive_table_entry (record, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	fbe_api_free_memory(record);
	fbe_api_free_memory(new_record);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}

void fbe_api_drive_configuration_interface_get_table_entry_test(void)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_drive_configuration_record_t *		record = NULL, *new_record = NULL;
	fbe_drive_configuration_handle_t		handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	fbe_bool_t								equal = FBE_FALSE;

    
	record = (fbe_drive_configuration_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(record != NULL);

	new_record = (fbe_drive_configuration_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(new_record != NULL);

	fbe_zero_memory(record, sizeof(fbe_drive_configuration_record_t));
	fbe_zero_memory(new_record, sizeof(fbe_drive_configuration_record_t));

	/*populate some data that would give us a hanlde back*/
	record->drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
	record->drive_info.drive_vendor = FBE_DRIVE_VENDOR_SIMULATION;
	fbe_sprintf(record->drive_info.fw_revision, strlen("B01"), "B01");
	fbe_sprintf(record->drive_info.part_number, strlen("CABBCCDD"), "CABBCCDD");
	fbe_sprintf(record->drive_info.serial_number_start, strlen("20001"), "20001");
	fbe_sprintf(record->drive_info.serial_number_end, strlen("40001"), "40001");

	record->threshold_info.data_stat.interval = 100;
	record->threshold_info.hardware_stat.weight = 200;

	/*put the record in*/
	fbe_api_drive_configuration_interface_start_update();
	status  = fbe_api_drive_configuration_interface_add_drive_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_api_drive_configuration_interface_end_update();

	/*try to get the same record and make sure we have a match with what we entered*/
	fbe_api_drive_configuration_interface_get_drive_table_entry(handle, new_record);

	equal = fbe_equal_memory(new_record, record, sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(equal == FBE_TRUE);

	/*call with a handle that is valid but should be empty*/
	status  = fbe_api_drive_configuration_interface_get_drive_table_entry (99, record);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*call with invalid handle*/
	status  = fbe_api_drive_configuration_interface_get_drive_table_entry (-1, record);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*call with invalid pointer*/
	status  = fbe_api_drive_configuration_interface_get_drive_table_entry (0, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);


	fbe_api_free_memory(record);
	fbe_api_free_memory(new_record);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}

void fbe_api_drive_configuration_interface_change_thresholds_test(void)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_drive_configuration_record_t *		record = NULL, *new_record = NULL;
	fbe_drive_configuration_handle_t		handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	fbe_bool_t								equal = FBE_FALSE;
    
	record = (fbe_drive_configuration_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(record != NULL);

	new_record = (fbe_drive_configuration_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_record_t));
	MUT_ASSERT_TRUE(new_record != NULL);

	fbe_zero_memory(record, sizeof(fbe_drive_configuration_record_t));
	fbe_zero_memory(new_record, sizeof(fbe_drive_configuration_record_t));

	/*populate some data that would give us a hanlde back*/
	record->drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
	record->drive_info.drive_vendor = FBE_DRIVE_VENDOR_SIMULATION;
	fbe_sprintf(record->drive_info.fw_revision, strlen("B01"), "B01");
	fbe_sprintf(record->drive_info.part_number, strlen("CABBCCDD"), "CABBCCDD");
	fbe_sprintf(record->drive_info.serial_number_start, strlen("20001"), "20001");
	fbe_sprintf(record->drive_info.serial_number_end, strlen("40001"), "40001");

	record->threshold_info.data_stat.interval = 100;
	record->threshold_info.hardware_stat.weight = 200;

	/*put the record in*/
	fbe_api_drive_configuration_interface_start_update();
	status  = fbe_api_drive_configuration_interface_add_drive_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_api_drive_configuration_interface_end_update();

	new_record->threshold_info.data_stat.interval = 0xFFBBEE;

	/*try to change the same record and make sure we have a match with what we entered*/
	fbe_api_drive_configuration_interface_change_drive_thresholds(handle, &new_record->threshold_info);

	/*read it again*/
	fbe_api_drive_configuration_interface_get_drive_table_entry(handle, record);

    equal = fbe_equal_memory(&new_record->threshold_info, &record->threshold_info, sizeof(fbe_drive_stat_t));
	MUT_ASSERT_TRUE(equal == FBE_TRUE);

	/*and some invalid things*/
	status  = fbe_api_drive_configuration_interface_change_drive_thresholds (99, &record->threshold_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	
	status  = fbe_api_drive_configuration_interface_change_drive_thresholds (-1, &record->threshold_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	
	status  = fbe_api_drive_configuration_interface_change_drive_thresholds (0, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);


	fbe_api_free_memory(record);
	fbe_api_free_memory(new_record);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}

void fbe_api_drive_configuration_interface_add_port_table_entry_test(void)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_drive_configuration_port_record_t *	record = NULL, *new_record = NULL;
	fbe_drive_configuration_handle_t		handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	fbe_bool_t								equal = FBE_FALSE;

    
	record = (fbe_drive_configuration_port_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(record != NULL);

	new_record = (fbe_drive_configuration_port_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(new_record != NULL);

	fbe_zero_memory(record, sizeof(fbe_drive_configuration_port_record_t));
	fbe_zero_memory(new_record, sizeof(fbe_drive_configuration_port_record_t));

	/*populate some data that would give us a hanlde back*/
	record->port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
	record->threshold_info.enclosure_stat.interval = 100;
	record->threshold_info.io_stat.weight = 200;

	/*let's try to call it without START first, it should fail*/
	status  = fbe_api_drive_configuration_interface_add_port_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*now the right way*/
	fbe_api_drive_configuration_interface_start_update();
	status  = fbe_api_drive_configuration_interface_add_port_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_api_drive_configuration_interface_end_update();

	/*try to get the same record and make sure we have a match with what we entered*/
	fbe_api_drive_configuration_interface_get_port_table_entry(handle, new_record);

	equal = fbe_equal_memory(new_record, record, sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(equal == FBE_TRUE);

	/*now with some bad stuff*/
	status  = fbe_api_drive_configuration_interface_add_port_table_entry (NULL, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status  = fbe_api_drive_configuration_interface_add_port_table_entry (record, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	fbe_api_free_memory(record);
	fbe_api_free_memory(new_record);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}

void fbe_api_drive_configuration_interface_change_port_thresholds_test(void)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_drive_configuration_port_record_t *		record = NULL, *new_record = NULL;
	fbe_drive_configuration_handle_t		handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	fbe_bool_t								equal = FBE_FALSE;

    
	record = (fbe_drive_configuration_port_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(record != NULL);

	new_record = (fbe_drive_configuration_port_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(new_record != NULL);

	fbe_zero_memory(record, sizeof(fbe_drive_configuration_port_record_t));
	fbe_zero_memory(new_record, sizeof(fbe_drive_configuration_port_record_t));

	/*populate some data that would give us a hanlde back*/
	record->port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
	record->threshold_info.enclosure_stat.interval = 100;
	record->threshold_info.io_stat.weight = 200;

	/*put the record in*/
	fbe_api_drive_configuration_interface_start_update();
	status  = fbe_api_drive_configuration_interface_add_port_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_api_drive_configuration_interface_end_update();

    /*try to change the same record and make sure we have a match with what we entered*/
	fbe_api_drive_configuration_interface_change_port_thresholds(handle, &new_record->threshold_info);

	/*read it again*/
	fbe_api_drive_configuration_interface_get_port_table_entry(handle, record);

    equal = fbe_equal_memory(&new_record->threshold_info, &record->threshold_info, sizeof(fbe_port_stat_t));
	MUT_ASSERT_TRUE(equal == FBE_TRUE);

	/*and some invalid things*/
	status  = fbe_api_drive_configuration_interface_change_port_thresholds (99, &record->threshold_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	
	status  = fbe_api_drive_configuration_interface_change_port_thresholds (-1, &record->threshold_info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	
	status  = fbe_api_drive_configuration_interface_change_port_thresholds (0, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);


	fbe_api_free_memory(record);
	fbe_api_free_memory(new_record);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}

void fbe_api_drive_configuration_interface_get_port_handles_test(void)
{
    fbe_status_t 								status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_drive_config_get_handles_list_t	*	get_handles = NULL;
    fbe_u32_t									handle_count = 0;
    
	get_handles = (fbe_api_drive_config_get_handles_list_t *)fbe_api_allocate_memory (sizeof(fbe_api_drive_config_get_handles_list_t));
	MUT_ASSERT_TRUE(get_handles != NULL);
    
    /*let first read all the handles*/
	status = fbe_api_drive_configuration_interface_get_handles(get_handles, FBE_API_HANDLE_TYPE_PORT);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*make sure we have at least one entry*/
	MUT_ASSERT_TRUE(get_handles->total_count > 0);
     
	/*now with NULL pointer*/
	status = fbe_api_drive_configuration_interface_get_handles(NULL, FBE_API_HANDLE_TYPE_PORT);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_drive_configuration_interface_get_handles(get_handles, FBE_API_HANDLE_TYPE_INVALID);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);


	fbe_api_free_memory(get_handles);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;
}


void fbe_api_drive_configuration_interface_get_port_table_entry_test(void)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_drive_configuration_port_record_t *		record = NULL, *new_record = NULL;
	fbe_drive_configuration_handle_t		handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	fbe_bool_t								equal = FBE_FALSE;

    
	record = (fbe_drive_configuration_port_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(record != NULL);

	new_record = (fbe_drive_configuration_port_record_t *)fbe_api_allocate_memory (sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(new_record != NULL);

	fbe_zero_memory(record, sizeof(fbe_drive_configuration_port_record_t));
	fbe_zero_memory(new_record, sizeof(fbe_drive_configuration_port_record_t));

	/*populate some data that would give us a hanlde back*/
	record->port_info.port_type = FBE_PORT_TYPE_SAS_PMC;
	record->threshold_info.enclosure_stat.interval = 100;
	record->threshold_info.io_stat.weight = 200;
    
	/*put the record in*/
	fbe_api_drive_configuration_interface_start_update();
	status  = fbe_api_drive_configuration_interface_add_port_table_entry (record, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	fbe_api_drive_configuration_interface_end_update();

	/*try to get the same record and make sure we have a match with what we entered*/
	fbe_api_drive_configuration_interface_get_port_table_entry(handle, new_record);

	equal = fbe_equal_memory(new_record, record, sizeof(fbe_drive_configuration_port_record_t));
	MUT_ASSERT_TRUE(equal == FBE_TRUE);

	/*call with a handle that is valid but should be empty*/
	status  = fbe_api_drive_configuration_interface_get_port_table_entry (99, record);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*call with invalid handle*/
	status  = fbe_api_drive_configuration_interface_get_port_table_entry (-1, record);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*call with invalid pointer*/
	status  = fbe_api_drive_configuration_interface_get_port_table_entry (0, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);


	fbe_api_free_memory(record);
	fbe_api_free_memory(new_record);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}


