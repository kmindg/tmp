#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_object_map_interface.h"
#include "fbe/fbe_api_utils.h"

/* drive_reset */
void fbe_api_drive_reset_test(void)
{
	fbe_status_t status;
	fbe_u32_t object_handle_p;

	status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

	status = fbe_api_physical_drive_reset(object_handle_p, FBE_PACKET_FLAG_NO_ATTRIB);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*make sure the drive is back in ready*/
	status = fbe_api_wait_for_object_lifecycle_state(object_handle_p, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
	return;
}

void fbe_api_physical_drive_power_cycle_test(void)
{
    fbe_status_t      	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t   	object_id;

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 1, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, FBE_OBJECT_ID_INVALID);

    /* Send a power-cycle command. */
    status = fbe_api_physical_drive_power_cycle(object_id, 1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check the drive goes to ACTIVATE state. */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_ACTIVATE, 5000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check the drive is in READY state. */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 180000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;
}
