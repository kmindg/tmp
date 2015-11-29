/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * aansi.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This test creates Armada board object, inserts a Bunker enclosure
 * into the topology, which contains to SAS drives.  The test validates
 * the objects within physical package at the end.
 * 
 * HISTORY
 *   07/12/2010:  Created. hud1
 *
 ***************************************************************************/

#include "physical_package_tests.h"
#include "fbe_test_configurations.h"

#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_private_space_layout.h"
#include "pp_utils.h"

#define STATIC_NUMBER_OF_OBJ 5
void aansi(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_api_terminator_device_handle_t  port_handle, encl_id1, drive_id1, drive_id2;
    fbe_u32_t                           object_handle = 0;
    fbe_u32_t                           port_idx = 0, enclosure_idx = 0, drive_idx = 0;
    fbe_port_number_t                   port_number;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;

    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure*/
    sas_encl.backend_number = sas_port.backend_number;
    sas_encl.encl_number = 0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_BUNKER;
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_id1);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*insert a drive to the enclosure in slot 0*/
    sas_drive.backend_number = sas_port.backend_number;
    sas_drive.encl_number = 0;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456780;
    sas_drive.block_size = 520;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    memset(&sas_drive.drive_serial_number, '1' , sizeof(sas_drive.drive_serial_number));
    status = fbe_api_terminator_insert_sas_drive (encl_id1, 0, &sas_drive, &drive_id1);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert a drive to the enclosure in slot 1*/
    sas_drive.backend_number = sas_port.backend_number;
    sas_drive.encl_number = 0;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.sas_address = (fbe_u64_t)0x443456781;
    sas_drive.block_size = 520;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    memset(&sas_drive.drive_serial_number, '2' , sizeof(sas_drive.drive_serial_number));
    status = fbe_api_terminator_insert_sas_drive (encl_id1, 1, &sas_drive, &drive_id2);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(STATIC_NUMBER_OF_OBJ, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /*we inserted a fleet board so we check for it*/
    /* board is always object id 0 ??? Maybe we need an interface to get the board object id*/
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port. */
    for (port_idx = 0; port_idx < FBE_API_PHYSICAL_BUS_COUNT; port_idx++) 
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

        } else {
            /*get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying port existance, state and number ...Passed");

    /* We should have exactly 1 enclosure on the port. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            if (port_idx == 0 && enclosure_idx < 1) {
                /*get the handle of the port and validate enclosure exists*/
                /* Validate that the enclosure is in READY state */
                status = fbe_zrt_wait_enclosure_status(port_idx, enclosure_idx,
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
          } else {
            /*get the handle of the port and validate enclosure exists or not*/
            status = fbe_api_get_enclosure_object_id_by_location(port_idx, enclosure_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
          }
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying enclosure existance and state....Passed");

    /* Check for the physical drives on the enclosure. */
    for (port_idx = 0; port_idx < FBE_TEST_PHYSICAL_BUS_COUNT; port_idx++) {
        for (enclosure_idx = 0; enclosure_idx < FBE_TEST_ENCLOSURES_PER_BUS; enclosure_idx++) {
            for (drive_idx = 0; drive_idx < FBE_TEST_ENCLOSURE_SLOTS; drive_idx++) {
                if ((port_idx < 1) && (enclosure_idx < 1) && (drive_idx < 2))
                {
                    /*get the handle of the port and validate drive exists*/
                    status = fbe_test_pp_util_verify_pdo_state(port_idx, enclosure_idx, drive_idx, 
                                                                FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                } else {
                    /*get the handle of the drive and validate drive exists*/
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, enclosure_idx, drive_idx, &object_handle);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
                }
            }
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying physical drive existance and state....Passed");

    /*let's see how many objects we have, at this stage we should have 7 objects:
    1 board
    1 port
    1 enclosure
    2 physical drives
    2 logical drives
    */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == STATIC_NUMBER_OF_OBJ);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    /*TODO, check here for the existanse of other objects, their SAS address and do on, using the FBE API*/
    
    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");
    return;
}

