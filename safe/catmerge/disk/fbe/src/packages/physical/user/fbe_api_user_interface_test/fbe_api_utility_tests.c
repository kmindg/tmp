#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "fbe/fbe_api_utils.h"


void fbe_api_wait_for_object_lifecycle_state_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
	fbe_lifecycle_state_t lifecycle_state;

	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p == 1);

	/* wait for the lifecycle state */
	lifecycle_state = FBE_LIFECYCLE_STATE_READY;
	status = fbe_api_wait_for_object_lifecycle_state(object_handle_p, lifecycle_state, 1000, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	/* wait for the lifecycle state */
	lifecycle_state = FBE_LIFECYCLE_STATE_READY;
	status = fbe_api_wait_for_object_lifecycle_state(object_handle_p, lifecycle_state, 1000, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	/* wait for the lifecycle state */
	lifecycle_state = FBE_LIFECYCLE_STATE_READY;
	status = fbe_api_wait_for_object_lifecycle_state(object_handle_p, lifecycle_state, 1000, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Passed fbe_api_wait_for_object_lifecycle_state_test successfully!!! ===\n");
	return;
}
void fbe_api_wait_for_number_of_objects_test(void)
{
	fbe_status_t status;
	fbe_u32_t expected_number;

	/* this is the correct number of objects */
	/* 1b + 1p + 3e + 5pd  */
	expected_number = 10;
	status = fbe_api_wait_for_number_of_objects(expected_number, 100);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/* this is the an incorrect number of objects */
	expected_number = 11;
	status = fbe_api_wait_for_number_of_objects(expected_number, 100);
	MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Passed fbe_api_wait_for_number_of_objects_test successfully!!! ===\n");
	return;
}