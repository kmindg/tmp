/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_logical_error_injection_tests.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests for the logical error injection service.
 *
 * @version
 *   12/21/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe_api_neit_interface_test.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"

void fbe_api_logical_error_injection_enable_test(void)
{
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Make sure it is disabled to start.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);

    /* Enable error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: enable logical error", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure it was enabled.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);

    /* Disable error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: disable logical error", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure it was disabled.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);

    return;
}

void fbe_api_logical_error_injection_add_remove_record_test(void)
{
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_logical_error_injection_get_records_t get_records;
    fbe_api_logical_error_injection_record_t create_record = {0};

    /* Make sure it is disabled to start.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);

    /* Add a record.
     */
    create_record.pos_bitmap = 1;
    create_record.lba = 0x50;
    create_record.blocks = 0x500;
    create_record.err_type = FBE_XOR_ERR_TYPE_CRC;
    create_record.err_mode = FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS;

    status = fbe_api_logical_error_injection_create_record(&create_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* make sure the record was created properly.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);

    status = fbe_api_logical_error_injection_get_records(&get_records);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(get_records.num_records, 1);
    MUT_ASSERT_INT_EQUAL(get_records.records[0].pos_bitmap, 1);
    MUT_ASSERT_TRUE(get_records.records[0].lba == 0x50);
    MUT_ASSERT_TRUE(get_records.records[0].blocks == 0x500);
    MUT_ASSERT_TRUE(get_records.records[0].err_type == FBE_XOR_ERR_TYPE_CRC);
    MUT_ASSERT_TRUE(get_records.records[0].err_mode == FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS);

    return;
}

void fbe_api_logical_error_injection_enable_remove_object_test(void)
{
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    status = fbe_api_enumerate_class(FBE_CLASS_ID_LOGICAL_DRIVE, FBE_PACKAGE_ID_PHYSICAL, &obj_list_p, &num_objects);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(num_objects, 4);

    /* Make sure it is disabled to start.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 0);

    status = fbe_api_logical_error_injection_enable_object(obj_list_p[0],
                                                           FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    /* Now remove the object via remove edge hook.*/
    status = fbe_api_logical_error_injection_remove_object(obj_list_p[0],
                                                           FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure it is disabled
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 0);

    fbe_api_free_memory(obj_list_p);
    return;
}

void fbe_api_logical_error_injection_enable_class_test(void)
{
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Make sure it is disabled to start.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 0);

    status = fbe_api_logical_error_injection_enable_class(FBE_CLASS_ID_LOGICAL_DRIVE,
                                                          FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 4);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 4);

    /* Enable something so we can test loading a table with error injection enabled.
     */
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 4);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 4);
    return;
}

void fbe_api_logical_error_injection_load_table_test(void)
{
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Make sure it is disabled to start.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 0);

    /* Enable something so we can test loading a table with error injection enabled.
     */
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 0);

    /* Make sure that loading a table while error injection is enabled gives us an error. 
     * We do not allow this operation. 
     */
    status = fbe_api_logical_error_injection_load_table(0, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Try loading an invalid table.  The load should fail.
     */
    status = fbe_api_logical_error_injection_load_table(99, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* Standard enable case.  This load should work.
     */
    status = fbe_api_logical_error_injection_load_table(0, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have records, but not be enabled.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 128);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 0);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 0);

    /* Enable something so we can test enabling with a loaded table.
     */
    status = fbe_api_logical_error_injection_enable_class(FBE_CLASS_ID_LOGICAL_DRIVE,
                                                          FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 128);
    MUT_ASSERT_INT_EQUAL(stats.num_objects, 4);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 4);

    return;
}

void fbe_api_logical_error_injection_add_tests(mut_testsuite_t *suite_p)
{
    MUT_ADD_TEST(suite_p, fbe_api_logical_error_injection_enable_test, 
                 fbe_api_neit_test_setup, fbe_api_neit_test_teardown)
    MUT_ADD_TEST(suite_p, fbe_api_logical_error_injection_add_remove_record_test, 
                 fbe_api_neit_test_setup, fbe_api_neit_test_teardown)
    MUT_ADD_TEST(suite_p, fbe_api_logical_error_injection_enable_remove_object_test, 
                 fbe_api_neit_test_setup, fbe_api_neit_test_teardown)
    MUT_ADD_TEST(suite_p, fbe_api_logical_error_injection_enable_class_test, 
                 fbe_api_neit_test_setup, fbe_api_neit_test_teardown)
    MUT_ADD_TEST(suite_p, fbe_api_logical_error_injection_load_table_test, 
                 fbe_api_neit_test_setup, fbe_api_neit_test_teardown)
    return;
}

/*************************
 * end file fbe_api_logical_error_injection_tests.c
 *************************/
