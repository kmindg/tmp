#include "fbe_api_user_interface_test.h"
/* This test will check get_board_object_handle.*/
#include "fbe/fbe_api_object_map_interface.h"

void fbe_api_get_board_object_handle_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
		
	status = fbe_api_object_map_interface_get_board_obj_id(&object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_board_object_handle successfully!!! ===\n");

	return;
}
/* This test will check get_port_object_handle.*/
void fbe_api_get_port_object_handle_test(void)
{
	fbe_u32_t object_handle_p;
	fbe_status_t status;
		
	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID);
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_port_object_handle successfully!!! ===\n");

	return;
}
void fbe_api_object_map_interface_get_enclosure_obj_id_test(void)
{
	fbe_u32_t  object_handle_p;
	fbe_status_t status;
	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_enclosure_obj_id(0, 1, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_object_map_interface_get_enclosure_obj_id successfully!!! ===\n");
	return;
}

void fbe_api_get_physical_drive_object_handle_test(void)
{
	fbe_u32_t  object_handle_p;
	fbe_status_t status;
	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 1, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 1, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 1, 1, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_get_physical_drive_object_handle successfully!!! ===\n");
	return;
}

void fbe_api_get_logical_drive_object_handle_test(void)
{
	fbe_u32_t  object_handle_p;
	fbe_status_t status;
	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 1, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 1, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 1, 1, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_handle_p != FBE_OBJECT_ID_INVALID); /* we use a static configuration, so hard coded this id for now */

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Verify fbe_api_object_map_interface_get_logical_drive_obj_id successfully!!! ===\n");
	return;
}