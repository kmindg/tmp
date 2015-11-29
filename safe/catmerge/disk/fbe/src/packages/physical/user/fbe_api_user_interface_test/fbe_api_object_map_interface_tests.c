#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_object_map_interface.h"

static void dummy_func (void *input)
{
	(void)input;
	return;
}

void fbe_api_object_map_interface_register_notification_test(void)
{
	fbe_status_t 		status = FBE_STATUS_GENERIC_FAILURE;
    
	status = fbe_api_object_map_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, (commmand_response_callback_function_t)dummy_func);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	/*unregister before we exit so we go out clean*/
    fbe_api_object_map_interface_unregister_notification((commmand_response_callback_function_t)dummy_func);
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;
}

void fbe_api_object_map_interface_unregister_notification_test(void)
{
	fbe_status_t 		status = FBE_STATUS_GENERIC_FAILURE;
    
	status = fbe_api_object_map_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, (commmand_response_callback_function_t)dummy_func);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_unregister_notification((commmand_response_callback_function_t)dummy_func);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_unregister_notification(NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);
	
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
	return;

}

void fbe_api_object_map_interface_init_test (void)
{
	fbe_status_t 		status = FBE_STATUS_GENERIC_FAILURE;

	/*second init should fail, we already called it once when we initialized)*/
	status = fbe_api_object_map_interface_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
}

void fbe_api_object_map_interface_destroy_test (void)
{
	fbe_status_t 		status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_object_map_interface_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*second destroy should fail*/
	status = fbe_api_object_map_interface_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_object_map_interface_init();
	status =  fbe_api_common_destroy();
    status =  physical_package_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
}

void fbe_api_object_map_interface_object_exists_test (void)
{
	fbe_object_id_t 				object_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE;
	fbe_bool_t						exists = FBE_FALSE;

	/*get some object that exists in the topology*/
	status = fbe_api_object_map_interface_get_port_obj_id(0, &object_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*and check if it's there*/
	exists = fbe_api_object_map_interface_object_exists(object_id);
	MUT_ASSERT_TRUE(exists == FBE_TRUE);

	exists = fbe_api_object_map_interface_object_exists(FBE_OBJECT_ID_INVALID);
	MUT_ASSERT_TRUE(exists == FBE_FALSE);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);

}

void fbe_api_object_map_interface_get_board_obj_id_test(void)
{
	fbe_object_id_t 				object_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE;
    
    status = fbe_api_object_map_interface_get_board_obj_id(&object_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);

	status = fbe_api_object_map_interface_get_board_obj_id(NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
}

void fbe_api_object_map_interface_get_total_logical_drives_test(void)
{
	fbe_object_id_t 				object_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE;
	fbe_u32_t						total_drives = 0;

	status = fbe_api_object_map_interface_get_total_logical_drives(0, &total_drives);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(total_drives, 5);/*our configuration must have 5 drives*/

	status = fbe_api_object_map_interface_get_total_logical_drives(45332, &total_drives);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_object_map_interface_get_total_logical_drives(0, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);

}

void fbe_api_object_map_interface_get_object_handle_test (void)
{
	fbe_object_id_t 				object_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_object_map_handle		handle = 0;
    
    status = fbe_api_object_map_interface_get_board_obj_id(&object_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	handle = fbe_api_object_map_interface_get_object_handle(object_id);
	MUT_ASSERT_TRUE(handle != 0);

	handle = fbe_api_object_map_interface_get_object_handle(FBE_OBJECT_ID_INVALID);
	MUT_ASSERT_TRUE(handle == 0);
    
	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);
}

void fbe_api_object_map_interface_get_logical_drive_object_handle_test (void)
{

	fbe_object_id_t 				object_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_object_map_handle 		handle;
    
	status = fbe_api_object_map_interface_get_logical_drive_object_handle(0, 0, 1, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
	status = fbe_api_object_map_interface_get_logical_drive_object_handle(55, 0, 1, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_object_map_interface_get_logical_drive_object_handle(0, 55, 1, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_object_map_interface_get_logical_drive_object_handle(0, 0, 65, &handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_object_map_interface_get_logical_drive_object_handle(0, 0, 1, NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n",__FUNCTION__);

}

void fbe_api_object_map_interface_get_obj_info_by_handle_test (void)
{
	fbe_object_id_t 				object_id = FBE_OBJECT_ID_INVALID;
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_object_map_handle		handle = 0;
	fbe_api_object_map_obj_info_t	info;

	status = fbe_api_object_map_interface_get_board_obj_id(&object_id);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	handle = fbe_api_object_map_interface_get_object_handle(object_id);
	MUT_ASSERT_TRUE(handle != 0);

	status = fbe_api_object_map_interface_get_obj_info_by_handle(handle, &info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(info.object_id == object_id);
	MUT_ASSERT_TRUE(info.object_state == FBE_LIFECYCLE_STATE_READY);

	status = fbe_api_object_map_interface_get_obj_info_by_handle((fbe_api_object_map_handle)NULL, &info);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

	status = fbe_api_object_map_interface_get_obj_info_by_handle(handle,NULL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_GENERIC_FAILURE);

}

