#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_api_object_map_interface.h"

/* Get_Set_Test */
void fbe_api_ldo_test_send_set_attributes_test(void)
{
    fbe_status_t status;
    fbe_u32_t object_handle_p;
    fbe_logical_drive_attributes_t drive_attribute;

	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 0, &object_handle_p);//Test for enclosure 0 drive 0
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_logical_drive_set_attributes(object_handle_p ,&drive_attribute);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 1, 0, &object_handle_p);//Test for enclosure 1 drive 0
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_logical_drive_set_attributes(object_handle_p ,&drive_attribute);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute set_attribute test successfully!!! ===\n");
    return;
}

void fbe_api_ldo_test_send_get_attributes_test(void)
{
    fbe_status_t status;
    fbe_u32_t object_handle_p;
    fbe_logical_drive_attributes_t drive_attribute;

	status = fbe_api_object_map_interface_get_logical_drive_obj_id(0, 0, 0, &object_handle_p);//Test for enclosure 0 drive 0
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_logical_drive_get_attributes(object_handle_p ,&drive_attribute);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute get_attribute test successfully!!! ===\n");
    return;
}