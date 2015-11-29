/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_rdgen_test_config.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid objects.
 *
 * @version
 *   7/14/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_database.h"
#include "sep_dll.h"
#include "physical_package_dll.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"

/* We need to pull this in order to pull in the functions to load the physical and logical 
 * packages. 
 */
#include "../../../../fbe_test/lib/package_config/fbe_test_package_config.c"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!*******************************************************************
 * @def FBE_RDGEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief We have 1 board, 1 port, 1 enclosure, 5 pdo
 *
 *********************************************************************/
#define FBE_RDGEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS 8


/*!*******************************************************************
 * @def FBE_RDGEN_TEST_PVD_OBJECT_ID
 *********************************************************************
 * @brief We start with the object id 0 since there are no other objects
 *        when we start the test.
 *********************************************************************/
#define FBE_RDGEN_TEST_PVD_OBJECT_ID 0

/*!*******************************************************************
 * @def FBE_RDGEN_TEST_DRIVE_COUNT
 *********************************************************************
 * @brief We arbitrarily create 5 drives.
 *
 *********************************************************************/
#define FBE_RDGEN_TEST_DRIVE_COUNT 5

static fbe_status_t fbe_rdgen_test_generic_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


static fbe_status_t 
fbe_rdgen_test_generic_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

void fbe_rdgen_test_config_and_load_packages(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t     board_info;
    fbe_terminator_sas_port_info_t  sas_port;
    fbe_terminator_sas_encl_info_t  sas_encl;
    fbe_terminator_sas_drive_info_t sas_drive;

    fbe_u32_t                           object_handle = 0;
    fbe_u32_t                           port_idx = 0, enclosure_idx = 0, drive_idx = 0;
    /*fbe_port_number_t                   port_number;*/
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle;
    fbe_terminator_api_device_handle_t  drive_handle;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Enable the IO mode.  This will cause the terminator to create
     * disk files as drive objects are created.
     */ 
    status = fbe_terminator_api_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
    status  = fbe_terminator_api_insert_board(&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.sas_address = (fbe_u64_t)0x0000000000000001;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 1;
    sas_port.portal_number = 2;
    sas_port.backend_number = 0;
    status  = fbe_terminator_api_insert_sas_port(&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1;    // some unique ID.
    sas_encl.sas_address = (fbe_u64_t)0x0000000000000002;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_terminator_api_insert_sas_enclosure(port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 0   */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654321;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive(encl0_0_handle, 0, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 1   */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654322;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive(encl0_0_handle, 1, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 2   */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654323;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive(encl0_0_handle, 2, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 3   */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654324;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive(encl0_0_handle, 3, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a sas_drive to port 0 encl 0 slot 4   */
    sas_drive.backend_number = 0;
    sas_drive.encl_number = 0;
    sas_drive.sas_address = (fbe_u64_t)0x987654325;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    sas_drive.block_size = 520;
    status  = fbe_terminator_api_insert_sas_drive(encl0_0_handle, 4, &sas_drive, &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical pacakge===\n");
    fbe_test_load_physical_package();

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");

    status = fbe_api_common_init_sim();/*this would cause the map to be populated*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, physical_package_io_entry, physical_package_control_entry);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(FBE_RDGEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a fleet board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == FBE_RDGEN_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    status = fbe_test_load_neit_package(NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_NEIT, NULL, neit_package_control_entry);
    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");
    return;
}


/*!**************************************************************
 * fbe_rdgen_test_teardown()
 ****************************************************************
 * @brief
 *  Destroy the objects needed to run the rdgen test.
 *
 * @param  None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_test_teardown(void)
{
    fbe_status_t status;
    fbe_u32_t critical_error_trace_count = 0;
    fbe_u32_t error_trace_count = 0;
    mut_printf(MUT_LOG_LOW, "=== destroying configuration ===\n");

    status  = fbe_api_common_destroy_sim();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_unload_neit_package();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Tear down the physical package and all the objects it contains. */
    status = fbe_test_unload_physical_package();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Destroy the terminator.*/
    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************
 * end fbe_rdgen_test_teardown()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_setup()
 ****************************************************************
 * @brief
 *  Setup the objects needed to run the rdgen test.
 *
 * @param  None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_setup(void)
{
    /* Creates and loads the physical and logical packages. 
     */
    fbe_rdgen_test_config_and_load_packages();

    return;
}
/******************************************
 * end fbe_rdgen_test_setup()
 ******************************************/
