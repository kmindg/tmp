/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * sealink.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This test creates Magnum board object, inserts a Derringer enclosure
 * into the topology, which contains to SAS drives.  The test validates
 * the objects within physical package at the end.
 * 
 * HISTORY
 *   07/12/2010:  Created. hud1
 *
 ***************************************************************************/


#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_utils.h"
#include "pp_utils.h"
#include "fbe_private_space_layout.h"


#define STATIC_NUMBER_OF_OBJ 80
#define ZEUS_READY_TEST_PORT_SAS_ADDRESS_BASE  0x5000097A7800903A
#define ZEUS_READY_TEST_ENCL_SAS_ADDRESS_BASE  0x5000097A8800903A
#define ZEUS_READY_TEST_DRIVE_SAS_ADDRESS_BASE 0x5000097A9800903A
#define ZEUS_READY_TEST_DRIVE_SERIAL_BASE      0x6000


/* Macro BUILD_PORT_SAS_ADDRESS - Builds port SAS address.
 */
#define ZRT_BUILD_PORT_SAS_ADDRESS(port_num) \
            (ZEUS_READY_TEST_PORT_SAS_ADDRESS_BASE + \
            (port_num * 0x100))
/* Macro BUILD_ENCL_SAS_ADDRESS - Builds enclosure SAS address.
 */
#define ZRT_BUILD_ENCL_SAS_ADDRESS(port_num, encl_num) \
            (ZEUS_READY_TEST_ENCL_SAS_ADDRESS_BASE + \
             (((port_num * FBE_API_ENCLOSURES_PER_BUS) + encl_num) * 0x100))

/* Macro BUILD_DISK_SAS_ADDRESS - Builds SAS address for a disk.
 */
#define ZRT_BUILD_DISK_SAS_ADDRESS(port_num, encl_num, slot_num) \
            (ZEUS_READY_TEST_DRIVE_SAS_ADDRESS_BASE + \
             (((((port_num * FBE_API_ENCLOSURES_PER_BUS) + encl_num) * \
             FBE_API_ENCLOSURE_SLOTS) + slot_num) * 0x100))

/* Macro BUILD_DISK_SERIAL_NUM - Builds serial number of a disk for derringer only.
 */
#define ZRT_BUILD_DISK_SERIAL_NUM(disk_serial_num, port, encl, slot) \
            _snprintf(disk_serial_num, \
                      FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, \
                      "%4X%4X", \
                      ZEUS_READY_TEST_DRIVE_SERIAL_BASE, \
                      (((port * FBE_API_ENCLOSURES_PER_BUS) + encl) * 25) + slot)


void sealink(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t      sas_port;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_api_terminator_device_handle_t  port_handle, encl_id;
	fbe_api_terminator_device_handle_t  drive_id;
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
    sas_port.sas_address = ZRT_BUILD_PORT_SAS_ADDRESS(0);
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;

    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for (enclosure_idx = 0; enclosure_idx <=2; enclosure_idx++)
    {
        /*insert an enclosure*/
        sas_encl.backend_number = sas_port.backend_number;
        sas_encl.encl_number = enclosure_idx;
        sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_DERRINGER;
        sas_encl.sas_address = ZRT_BUILD_ENCL_SAS_ADDRESS(0,enclosure_idx) ;
        status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        for(drive_idx = 0; drive_idx <25; drive_idx++)
        {

            sas_drive.backend_number = sas_port.backend_number;
            sas_drive.encl_number = enclosure_idx;
            sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
            sas_drive.block_size = 520;
            sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
            sas_drive.sas_address = ZRT_BUILD_DISK_SAS_ADDRESS(0,enclosure_idx,drive_idx);
            ZRT_BUILD_DISK_SERIAL_NUM(sas_drive.drive_serial_number, 0,enclosure_idx,drive_idx);
            status = fbe_api_terminator_insert_sas_drive (encl_id, drive_idx, &sas_drive, &drive_id);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    //fbe_zrt_start_physical_package_and_fbe_api();
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
    status = fbe_api_wait_for_number_of_objects(STATIC_NUMBER_OF_OBJ, 60000, FBE_PACKAGE_ID_PHYSICAL);
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
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
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

            for(enclosure_idx = 0; enclosure_idx <=2; enclosure_idx++)
            {
                /* Validate that the enclosure is in READY state */
                status = fbe_zrt_wait_enclosure_status(port_idx, enclosure_idx,
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                for(drive_idx = 0; drive_idx <25; drive_idx++)
                {
                    /* Validate that the physical drives are in READY state */
                    status = fbe_test_pp_util_verify_pdo_state(port_idx, enclosure_idx, drive_idx, 
                                    FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

                }
            }
        }
        else 
        {
            /* Get the handle of the port and validate port exists or not*/
            status = fbe_api_get_port_object_id_by_location (port_idx, &object_handle);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle == FBE_OBJECT_ID_INVALID);
        }
    }

    /*let's see how many objects we have, at this stage we should have 7 objects:
    1 board
    1 port
    3 enclosure
    75 physical drives
    75 logical drives
    */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == STATIC_NUMBER_OF_OBJ);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    /*TODO, check here for the existanse of other objects, their SAS address and do on, using the FBE API*/
    
    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");
    return;
}

