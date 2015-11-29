/**************************************************************************
 * Copyright (C) EMC Corporation 2008 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
***************************************************************************/

#include "hw_tests.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_user_server.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"

char * static_config_hw_short_desc = "static configuration HW test";
char * static_config_hw_long_desc = "\n\tVerify device objects on real hardware.\n";
void static_config_hw_test(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           object_handle = 0;
    fbe_u32_t                           port_idx = 0, drive_idx = 0;
    fbe_port_number_t                   port_number;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_object_id_t *           obj_list = NULL;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t board_count = 0, port_count = 0, encl_count = 0, physical_drive_count = 0;

    /* set control entry for drivers on SP server side */
    status = fbe_api_user_server_set_driver_entries();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* register notification elements on SP server side */
    status = fbe_api_user_server_register_notifications();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < 8; port_idx++)
    {
        if (port_idx < 1)
        {
            /* Get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state (object_handle,
                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number (object_handle, &port_number);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Validate that the enclosure is in READY state */
            status = fbe_zrt_wait_enclosure_status(port_idx, 0,
                                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            for (drive_idx = 0; drive_idx < 2; drive_idx++)
            {
                /* Validate that the physical drives are in READY state */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, 0, drive_idx,
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            }
        }
        else
        {
            /* Get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    /*let's see how many objects we have, at this stage we should have 7 objects:
    1 board
    1 port
    1 enclosure
    2 physical drives
    2 logical drives
    */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects > 0); // we cannot tell how may on HW
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    mut_printf(MUT_LOG_LOW, "Total number of object is %d\n", total_objects);

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    MUT_ASSERT_TRUE(obj_list != NULL);

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num ,FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK) {
            fbe_api_free_memory(obj_list);
        }
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        switch(obj_type)
        {
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            ++board_count;
            break;
        case FBE_TOPOLOGY_OBJECT_TYPE_PORT:
            ++port_count;
            break;
        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            ++encl_count;
            break;
        case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
            ++physical_drive_count;
            break;
        default:
            break;
        }
    }

    mut_printf(MUT_LOG_LOW, "Total number of board is %d\n", board_count);
    mut_printf(MUT_LOG_LOW, "Total number of port is %d\n", port_count);
    mut_printf(MUT_LOG_LOW, "Total number of enclosure is %d\n", encl_count);
    mut_printf(MUT_LOG_LOW, "Total number of physical drive is %d\n", physical_drive_count);

    fbe_api_free_memory(obj_list);

    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");
    return;
}

