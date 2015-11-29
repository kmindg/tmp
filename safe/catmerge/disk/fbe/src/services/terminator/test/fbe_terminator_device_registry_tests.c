/**********************************/
/*        include files           */
/**********************************/
#include "fbe_terminator_device_registry.h"
#include "terminator_test.h"

#define HANDLE_COUNT 16

/* handles */
tdr_device_ptr_t device_ptr[ HANDLE_COUNT ] = 
{
    (tdr_device_ptr_t)0x00010000,
    (tdr_device_ptr_t)0x00020000,
    (tdr_device_ptr_t)0x00030000,
    (tdr_device_ptr_t)0x00040000,
    (tdr_device_ptr_t)0x00050000,
    (tdr_device_ptr_t)0x00060000,
    (tdr_device_ptr_t)0x00070000,
    (tdr_device_ptr_t)0x00080000,
    (tdr_device_ptr_t)0x00090000,
    (tdr_device_ptr_t)0x000A0000,
    (tdr_device_ptr_t)0x000B0000,
    (tdr_device_ptr_t)0x000C0000,
    (tdr_device_ptr_t)0x000D0000,
    (tdr_device_ptr_t)0x000E0000,
    (tdr_device_ptr_t)0x000F0000,
    (tdr_device_ptr_t)0x0000FFFF
};

tdr_device_handle_t handle_buffer[ HANDLE_COUNT ];

/* types */
tdr_device_type_t types[ HANDLE_COUNT ] = 
{
    (tdr_device_type_t)1,
    (tdr_device_type_t)1,
    (tdr_device_type_t)1,
    (tdr_device_type_t)1,
    (tdr_device_type_t)2,
    (tdr_device_type_t)2,
    (tdr_device_type_t)2,
    (tdr_device_type_t)2,
    (tdr_device_type_t)3,
    (tdr_device_type_t)3,
    (tdr_device_type_t)3,
    (tdr_device_type_t)3,
    (tdr_device_type_t)4,
    (tdr_device_type_t)4,
    (tdr_device_type_t)4,
    (tdr_device_type_t)4
};

/* type and handle registry should not know about */
tdr_device_type_t UNKNOWN_TYPE = (tdr_device_type_t)31415926;
tdr_device_handle_t UNKNOWN_HANDLE = (tdr_device_handle_t)0x10000001000003ff;
tdr_device_ptr_t UNKNOWN_PTR = (tdr_device_ptr_t)-1LL;

tdr_device_handle_t handles[ HANDLE_COUNT ];

tdr_device_type_t type;

tdr_status_t status;

/* common setup */
void terminator_device_registry_test_setup(void)
{
    /* initialize registry and check it was successful */
    status = fbe_terminator_device_registry_init(HANDLE_COUNT);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}
/* common teardown */
void terminator_device_registry_test_teardown(void)
{
    status = fbe_terminator_device_registry_destroy();
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

/* check that we can't do anything with uninitialized or destroyed registry */
void terminator_device_registry_should_not_do_it_before_init(void)
{
    /*  - add */
    status = fbe_terminator_device_registry_add_device(types[0], device_ptr[0], &handle_buffer[0]);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    MUT_ASSERT_INTEGER_EQUAL( handle_buffer[0], TDR_INVALID_HANDLE )

    /*  - remove */
    status = fbe_terminator_device_registry_remove_device_by_handle(handle_buffer[0]);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - enumerate all */
    status = fbe_terminator_device_registry_enumerate_all_devices(handle_buffer, HANDLE_COUNT );
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - enumerate by type */
    status = fbe_terminator_device_registry_enumerate_devices_by_type(handle_buffer, HANDLE_COUNT, types[0] );
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - counter of devices is 0 */
    MUT_ASSERT_INTEGER_EQUAL( fbe_terminator_device_registry_get_device_count(), 0 )
    /*  - counter of devices of particular type is also 0 */
    MUT_ASSERT_INTEGER_EQUAL( fbe_terminator_device_registry_get_device_count_by_type(types[0]), 0 )
    /*  - obtain device type */
    status = fbe_terminator_device_registry_get_device_type(handle_buffer[0], &type);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
}


void terminator_device_registry_init_test(void)
{
    /* registry is initialized in the common setup */
    /* check that we can't initialize it twice */
    status = fbe_terminator_device_registry_init(HANDLE_COUNT);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

     mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

void terminator_device_registry_add_test(void)
{
    int i;
    tdr_device_ptr_t additional_device = (tdr_device_ptr_t)0x10001000;
    tdr_device_type_t additional_device_type = 1; 

    tdr_device_handle_t returned_device_handle;
    /* check we cannot add invalid device type or device pointer */
    status = fbe_terminator_device_registry_add_device(UNKNOWN_TYPE, TDR_INVALID_PTR, &returned_device_handle);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    MUT_ASSERT_INTEGER_EQUAL( returned_device_handle, TDR_INVALID_HANDLE )

    status = fbe_terminator_device_registry_add_device(TDR_DEVICE_TYPE_INVALID, (tdr_device_ptr_t)UNKNOWN_PTR, &returned_device_handle);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    MUT_ASSERT_INTEGER_EQUAL( returned_device_handle, TDR_INVALID_HANDLE )

    /* check we can add devices no more than one time */
    for(i = 0; i < HANDLE_COUNT; i++)
    {
        /*obtain counters */
        tdr_u32_t old_all_counter = fbe_terminator_device_registry_get_device_count();
        tdr_u32_t old_counter_by_type = fbe_terminator_device_registry_get_device_count_by_type(types[i]);

        status = fbe_terminator_device_registry_add_device(types[i], device_ptr[i], &handles[i]);
        MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
        MUT_ASSERT_INTEGER_NOT_EQUAL( handles[i], TDR_INVALID_HANDLE )
        
        /* counters must be increased in case of success */
        MUT_ASSERT_INTEGER_EQUAL( old_all_counter + 1, fbe_terminator_device_registry_get_device_count() )
        MUT_ASSERT_INTEGER_EQUAL( old_counter_by_type + 1, fbe_terminator_device_registry_get_device_count_by_type(types[i]) )

        /* Add the same pointer, duplicate entry is not allowed */
        status = fbe_terminator_device_registry_add_device(types[i], device_ptr[i], &handles[i]);
        MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
        MUT_ASSERT_INTEGER_EQUAL( handles[i], TDR_INVALID_HANDLE )

        /* counters must not be increased in case of failure */
        MUT_ASSERT_INTEGER_EQUAL( old_all_counter + 1, fbe_terminator_device_registry_get_device_count() )
        MUT_ASSERT_INTEGER_EQUAL( old_counter_by_type + 1, fbe_terminator_device_registry_get_device_count_by_type(types[i]) )

    }

    /* check we cannot add device or handle to the full registry */
    status = fbe_terminator_device_registry_add_device(additional_device_type, additional_device, &returned_device_handle);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    MUT_ASSERT_INTEGER_EQUAL( returned_device_handle, TDR_INVALID_HANDLE )

	mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

void terminator_device_registry_remove_test(void)
{
    int i;
    /*obtain counters */
    tdr_u32_t old_all_counter; 
    tdr_u32_t old_counter_by_type; 
    tdr_device_handle_t handle_to_remove;
    tdr_device_ptr_t ptr_to_remove;

    /* Add the devices */
    for(i = 0; i < HANDLE_COUNT; i++)
    {
        handles[i] = 0;
        status = fbe_terminator_device_registry_add_device(types[i], device_ptr[i], &handles[i]);
        MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
        MUT_ASSERT_INTEGER_NOT_EQUAL( handles[i], TDR_INVALID_HANDLE )
    }

    /* test remove by handle */
    /* check we cannot remove invalid handle */
    status = fbe_terminator_device_registry_remove_device_by_handle(TDR_INVALID_HANDLE);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    /* check that we cannot remove handle that registry don't know about */
    status = fbe_terminator_device_registry_remove_device_by_handle(UNKNOWN_HANDLE);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    handle_to_remove = handles[5];
    old_all_counter = fbe_terminator_device_registry_get_device_count();
    old_counter_by_type = fbe_terminator_device_registry_get_device_count_by_type(types[5]);

    /* check that we can remove known device*/
    status = fbe_terminator_device_registry_remove_device_by_handle(handle_to_remove);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )

    /* counters must be decreased in case of success */
    MUT_ASSERT_INTEGER_EQUAL( old_all_counter - 1, fbe_terminator_device_registry_get_device_count() )
    MUT_ASSERT_INTEGER_EQUAL( old_counter_by_type - 1, fbe_terminator_device_registry_get_device_count_by_type(types[5]) )

    /* check that we can not remove the same device twice*/
    status = fbe_terminator_device_registry_remove_device_by_handle(handle_to_remove);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    /* check that after removing, device can be added again and got a different handle*/
    status = fbe_terminator_device_registry_add_device(types[5], device_ptr[5], &handles[5]);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    MUT_ASSERT_INTEGER_NOT_EQUAL( handles[5], handle_to_remove )

    /* test remove by ptr */
    /* check we cannot remove invalid handle */
    status = fbe_terminator_device_registry_remove_device_by_ptr(TDR_INVALID_PTR);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    /* check that we cannot remove handle that registry don't know about */
    status = fbe_terminator_device_registry_remove_device_by_ptr(UNKNOWN_PTR);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    ptr_to_remove = device_ptr[10];
    handle_to_remove = handles[10];
    old_all_counter = fbe_terminator_device_registry_get_device_count();
    old_counter_by_type = fbe_terminator_device_registry_get_device_count_by_type(types[10]);

    /* check that we can remove known device*/
    status = fbe_terminator_device_registry_remove_device_by_ptr(ptr_to_remove);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )

    /* counters must be decreased in case of success */
    MUT_ASSERT_INTEGER_EQUAL( old_all_counter - 1, fbe_terminator_device_registry_get_device_count() )
    MUT_ASSERT_INTEGER_EQUAL( old_counter_by_type - 1, fbe_terminator_device_registry_get_device_count_by_type(types[10]) )

    /* check that we can not remove the same device twice*/
    status = fbe_terminator_device_registry_remove_device_by_ptr(ptr_to_remove);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    /* check that after removing, device can be added again and got a different handle*/
    status = fbe_terminator_device_registry_add_device(types[10], device_ptr[10], &handles[10]);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    MUT_ASSERT_INTEGER_NOT_EQUAL( handles[10], handle_to_remove )

	mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

void terminator_device_registry_enumerate_all_test(void)
{
    int i, j, found = 0;
    /* Add the devices */
    for(i = 0; i < HANDLE_COUNT; i++)
    {
        handles[i] = 0;
        status = fbe_terminator_device_registry_add_device(types[i], device_ptr[i], &handles[i]);
        MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
        MUT_ASSERT_INTEGER_NOT_EQUAL( handles[i], TDR_INVALID_HANDLE )
    }

    /* it should not accept NULL buffer */
    status = fbe_terminator_device_registry_enumerate_all_devices(NULL, HANDLE_COUNT );
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /* it should not accept small buffer */
    status = fbe_terminator_device_registry_enumerate_all_devices(NULL, HANDLE_COUNT - 1);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    /* it should accept normal buffer */
    status = fbe_terminator_device_registry_enumerate_all_devices(handle_buffer, HANDLE_COUNT);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    /* all known handles must be present there */
    for(i = 0; i < HANDLE_COUNT; i++)
    {
        found = 0;
        for(j = 0; j < HANDLE_COUNT; j++)
        {
            if(handle_buffer[j] == handles[i])
            {
                found = 1;
                break;
            }
        }
        MUT_ASSERT_INTEGER_EQUAL( 1, found )
    }
	mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

void terminator_device_registry_enumerate_by_type_test(void)
{
    tdr_u32_t i, j;
    int found = 0;
    tdr_device_type_t Type = types[10];

    /* Add the devices */
    for(i = 0; i < HANDLE_COUNT; i++)
    {
        handles[i] = 0;
        status = fbe_terminator_device_registry_add_device(types[i], device_ptr[i], &handles[i]);
        MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
        MUT_ASSERT_INTEGER_NOT_EQUAL( handles[i], TDR_INVALID_HANDLE )
    }

    /* it should not accept NULL buffer */
    status = fbe_terminator_device_registry_enumerate_devices_by_type(NULL, HANDLE_COUNT, Type );
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /* it should not accept small buffer */
    status = fbe_terminator_device_registry_enumerate_devices_by_type(NULL, HANDLE_COUNT - 1, Type);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /* it should not accept invalid type */
    status = fbe_terminator_device_registry_enumerate_devices_by_type(handle_buffer, HANDLE_COUNT, TDR_DEVICE_TYPE_INVALID);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /* it should accept normal buffer */
    status = fbe_terminator_device_registry_enumerate_devices_by_type(handle_buffer, HANDLE_COUNT, Type);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    /* all handles with given type must be present there */
    /* no handle without given type must be present there */
    for(i = 0; i < HANDLE_COUNT; i++)
    {
        found = 0;
        for(j = 0; j < fbe_terminator_device_registry_get_device_count_by_type(Type); j++)
        {
            /*the type must match with desired type */
            status = fbe_terminator_device_registry_get_device_type(handle_buffer[j], &type);
            MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
            MUT_ASSERT_INTEGER_EQUAL( type, Type )
            if(handle_buffer[j] == handles[i])
            {
                found = 1;
                break;
            }
        }
        MUT_ASSERT_TRUE( (types[i] == Type && found == 1) || (types[i] != Type && found == 0) )
    }
	mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}


void terminator_device_registry_counters_test(void)
{
    /* It works. See upper. They are used there many times. */
	mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

void terminator_device_registry_get_type_test(void)
{
    /* It works. See upper. They are used there many times. */
	mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);
}

void terminator_device_registry_destroy_test(void)
{
    status = fbe_terminator_device_registry_destroy();
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_OK )
    /* check that we can't destroy it twice */
    status = fbe_terminator_device_registry_destroy();
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )

    mut_printf(MUT_LOG_HIGH, "End of %s", __FUNCTION__);

}


void terminator_device_registry_should_not_do_it_after_destroy(void)
{
    /*  - add */
    status = fbe_terminator_device_registry_add_device(types[0], device_ptr[0], &handles[0]);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - remove */
    status = fbe_terminator_device_registry_remove_device_by_handle(handles[0]);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - enumerate all */
    status = fbe_terminator_device_registry_enumerate_all_devices(handle_buffer, HANDLE_COUNT );
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - enumerate by type */
    status = fbe_terminator_device_registry_enumerate_devices_by_type(handle_buffer, HANDLE_COUNT, 1 );
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
    /*  - counter of devices is 0 */
    MUT_ASSERT_INTEGER_EQUAL( fbe_terminator_device_registry_get_device_count(), 0 )
    /*  - counter of devices of particular type is also 0 */
    MUT_ASSERT_INTEGER_EQUAL( fbe_terminator_device_registry_get_device_count_by_type(1), 0 )
    /*  - obtain device type */
    status = fbe_terminator_device_registry_get_device_type(handles[0], &type);
    MUT_ASSERT_INTEGER_EQUAL( status, TDR_STATUS_GENERIC_FAILURE )
}
// TODO: Add more test cases here 
