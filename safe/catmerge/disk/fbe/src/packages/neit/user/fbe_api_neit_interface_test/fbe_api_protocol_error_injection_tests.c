#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe_api_neit_interface_test.h"

void fbe_api_protocol_error_injection_add_remove_record_test(void)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_protocol_error_injection_error_record_t new_record;
	fbe_protocol_error_injection_record_handle_t record_handle;

	new_record.frequency = 1;
	new_record.lba_start = 0;
	new_record.lba_end = 10;
	new_record.max_times_hit_timestamp = 10;
	new_record.num_of_times_to_insert = 10;
	new_record.num_of_times_to_reactivate = 10;
	new_record.object_id = 10;
	new_record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
	new_record.secs_to_reactivate = 10;
	new_record.times_inserted = 10;
	new_record.times_reset = 10;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: add_record", __FUNCTION__);
	status = fbe_api_protocol_error_injection_add_record(&new_record, &record_handle);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_NOT_NULL(record_handle);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: remove_record", __FUNCTION__);
	status = fbe_api_protocol_error_injection_remove_record(record_handle);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}
