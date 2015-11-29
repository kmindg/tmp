#include "terminator_miniport_sas_plugin_test.h"
#include "fbe_terminator_miniport_sas_device_table.h"

terminator_miniport_sas_device_table_t table;

void miniport_sas_plugin_device_table_init_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_miniport_sas_device_table_init(MAX_DEVICES_PER_SAS_PORT, &table);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}

void miniport_sas_plugin_device_table_destroy_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_miniport_sas_device_table_destroy(&table);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}

void miniport_sas_plugin_device_table_double_init_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_miniport_sas_device_table_init(MAX_DEVICES_PER_SAS_PORT, &table);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}

void miniport_sas_plugin_device_table_add_record_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index;

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the second record, so the index should be MAX_DEVICES_PER_SAS_PORT-2 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-2, index);

    /*add a record to an existing valid record, it should fail*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_GENERIC_FAILURE, status);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}

void miniport_sas_plugin_device_table_add_remove_record_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index;

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    /* remove the record*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    status = terminator_miniport_sas_device_table_remove_record(&table, index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* the first record is removed, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);


    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}

void miniport_sas_plugin_device_table_get_device_id_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index;
    fbe_miniport_device_id_t device_id;

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 
     * and generation id should be 1*/
    MUT_ASSERT_INTEGER_EQUAL(0x10000+(MAX_DEVICES_PER_SAS_PORT-1), device_id);

    /* remove the record*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    status = terminator_miniport_sas_device_table_remove_record(&table, index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* the first record is removed, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 
     * and generation id should be 2*/
    MUT_ASSERT_INTEGER_EQUAL(0x20000+(MAX_DEVICES_PER_SAS_PORT-1), device_id);

    /* remove the record*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    status = terminator_miniport_sas_device_table_remove_record(&table, index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    status = terminator_miniport_sas_device_table_add_record(&table, NULL, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* the first record is removed, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 
     * and generation id should be 3*/
    MUT_ASSERT_INTEGER_EQUAL(0x30000+(MAX_DEVICES_PER_SAS_PORT-1), device_id);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}

void miniport_sas_plugin_device_table_reserve_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t index;
    fbe_miniport_device_id_t device_id;
    fbe_terminator_device_ptr_t device_handle;

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    device_handle = &device_id;
    status = terminator_miniport_sas_device_table_add_record(&table, device_handle, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    status = terminator_miniport_sas_device_table_get_device_handle(&table,index,&device_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(&device_id, device_handle);

    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 
     * and generation id should be 1*/
    MUT_ASSERT_INTEGER_EQUAL(0x10000+(MAX_DEVICES_PER_SAS_PORT-1), device_id);

    /*add a new record*/
    index = INVALID_TMSDT_INDEX;
    device_handle = &index;
    status = terminator_miniport_sas_device_table_add_record(&table, device_handle, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* the first record is removed, so the index should be MAX_DEVICES_PER_SAS_PORT-2 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-2, index);
    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is the first record, so the index should be MAX_DEVICES_PER_SAS_PORT-1 
     * and generation id should be 1*/
    MUT_ASSERT_INTEGER_EQUAL(0x10000+(MAX_DEVICES_PER_SAS_PORT-2), device_id);
    status = terminator_miniport_sas_device_table_get_device_handle(&table,index,&device_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(&index, device_handle);

    /* reserve the record*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    status = terminator_miniport_sas_device_table_reserve_record(&table, index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the record*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    status = terminator_miniport_sas_device_table_remove_record(&table, index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*add a normal record, which should take the next available slot, not the reserved one*/
    index = INVALID_TMSDT_INDEX;
    device_handle = &device_id;
    status = terminator_miniport_sas_device_table_add_record(&table, device_handle, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* this is a normal record, so the index should be MAX_DEVICES_PER_SAS_PORT-3 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-3, index);
    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(0x10000+(MAX_DEVICES_PER_SAS_PORT-3), device_id);

    /*add a record to the reserved slot*/
    index = MAX_DEVICES_PER_SAS_PORT-1;
    device_handle = &index + 1;
    status = terminator_miniport_sas_device_table_add_record(&table, device_handle, &index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* he index should be MAX_DEVICES_PER_SAS_PORT-1 */
    MUT_ASSERT_INT_EQUAL(MAX_DEVICES_PER_SAS_PORT-1, index);

    status = terminator_miniport_sas_device_table_get_device_id(&table, index, &device_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* so the index should be MAX_DEVICES_PER_SAS_PORT-1 
     * and generation id should be 2*/
    MUT_ASSERT_INTEGER_EQUAL(0x20000+(MAX_DEVICES_PER_SAS_PORT-1), device_id);

    status = terminator_miniport_sas_device_table_get_device_handle(&table,index,&device_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INTEGER_EQUAL(&index+1, device_handle);

    mut_printf(MUT_LOG_HIGH, "%s %s test completed!",__FILE__, __FUNCTION__);
}
