#include "fbe_api_user_interface_test.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_object_map_interface.h"

/* error_test */
void fbe_api_physical_drive_get_error_counts_test(void)
{
    fbe_status_t status;
    fbe_u32_t object_handle_p;
    fbe_api_drive_error_count_t error_counts;
    fbe_api_dieh_stats_t dieh_stats;

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_get_dieh_info(object_handle_p, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);//need to pass error structure
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 1, 0, &object_handle_p);//Test for enclosure 1
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_get_dieh_info(object_handle_p, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);//need to pass error structure
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;
}
void fbe_api_physical_drive_clear_error_counts_test(void)
{
    fbe_status_t status;
    fbe_u32_t object_handle_p;

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_handle_p);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_clear_error_counts(object_handle_p, FBE_PACKET_FLAG_NO_ATTRIB);//need to pass error structure
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 1, 0, &object_handle_p);//Test for enclosure 1
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_handle_p, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_clear_error_counts(object_handle_p, FBE_PACKET_FLAG_NO_ATTRIB);//need to pass error structure
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;
}

void fbe_api_physical_drive_is_wr_cache_enabled_test(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id;
    fbe_bool_t          enabled = TRUE;

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_id);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_is_wr_cache_enabled(object_id, &enabled, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(enabled == FBE_FALSE);


    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;
}

void fbe_api_physical_drive_get_drive_information_test(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     object_id;
    fbe_physical_drive_information_t get_drive_information;

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_id);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_physical_drive_get_drive_information(object_id, &get_drive_information, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;
}

void fbe_api_physical_drive_set_get_qdepth_test(void)
{
	fbe_status_t      	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t   	object_id;
	fbe_u32_t			qdepth = -1;
    
    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_id);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, FBE_OBJECT_ID_INVALID);

	/*start with a reasonable value of 5*/
    status = fbe_api_physical_drive_set_object_queue_depth(object_id, 5, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*expect failue with 0*/
    status = fbe_api_physical_drive_set_object_queue_depth(object_id, 0, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

	/*expect failue with 25*/
    status = fbe_api_physical_drive_set_object_queue_depth(object_id, 25, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);

	/*we must get the 5 we set before*/
	status = fbe_api_physical_drive_get_object_queue_depth(object_id, &qdepth, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(qdepth == 5);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;

}

void fbe_api_physical_drive_set_get_timeout_test(void)
{
	fbe_status_t      	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t   	object_id;
    fbe_time_t			timeout;

    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 0, 0, &object_id);//Test for enclosure 0
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, FBE_OBJECT_ID_INVALID);

	/*start with a reasonable value of 3400*/
    status = fbe_api_physical_drive_set_default_io_timeout(object_id, 3400, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	/*expect failue with 0*/
    status = fbe_api_physical_drive_set_default_io_timeout(object_id, 0, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
	

	/*we must get the 5 we set before*/
	status = fbe_api_physical_drive_get_default_io_timeout(object_id, &timeout, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_TRUE(timeout == 3400);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;

}

void fbe_api_physical_drive_set_sata_write_uncorrectable_test(void)
{
    fbe_status_t      	status = FBE_STATUS_GENERIC_FAILURE;
	fbe_object_id_t   	object_id;
 
    /* Test for bus 0 enclosure 2 slot 0. */
    status = fbe_api_object_map_interface_get_physical_drive_obj_id(0, 2, 0, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(object_id, FBE_OBJECT_ID_INVALID);

    /*Set lba to 0x11080*/
    status = fbe_api_physical_drive_set_write_uncorrectable(object_id, 0x11080, FBE_PACKET_FLAG_NO_ATTRIB );
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "=== !!! Execute %s successfully!!! ===\n", __FUNCTION__);
    return;

}
