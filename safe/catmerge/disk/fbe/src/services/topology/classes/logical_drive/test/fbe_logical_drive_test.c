/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include <windows.h>
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_trace.h"
#include "fbe_transport_memory.h"

#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_base_object.h"
#include "fbe_topology.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "fbe/fbe_terminator_api.h"
#include "mut.h"
#include "fbe_interface.h"
/* #include "fbe_topology_discovery_lib.h" */
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_package_config.h"

/* This is the object ID we use for testing.
 */
fbe_object_id_t fbe_ldo_test_logical_drive_object_id_array[FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX];
fbe_object_id_t fbe_ldo_test_logical_drive_object_id;

/* This is the drive id of the drive we created programatically.
 */
fbe_terminator_api_device_handle_t fbe_ldo_test_drive_id_array[FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX];

/* This is the set of block sizes we will use for testing. 
 * We will have one drive with each of these block sizes. 
 * We no longer support 512 b/s drives
 */
const fbe_block_size_t fbe_ldo_test_supported_block_sizes[FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX] = {
    520,
#ifdef BLOCKSIZE_512_SUPPORTED
    512,
#endif    
    4096,
    4160
};

/* This structure contains the negotiate block sizes for our drives.
 * We no longer support 512 byte/sector block sizes
 */
fbe_ldo_test_negotiate_block_size_case_t fbe_ldo_test_default_drive_negotiate_data[] = 
{
    /* Imported Imported Req    Req    Failure   Expected Negotiate Data
     * Capacity Bl size. Bl Sz  Opt Sz Expected
     */
    {0x10b5c730, 520,     520,   1,     0,       0x10b5c730, 520, 1,  0, 520, 1, 520, 1},
#ifdef BLOCKSIZE_512_SUPPORTED    
    {0x10b5c730, 512,     520,   1,     0,       0x1073F740, 520, 64, 0, 512, 65, 520, 1},
#endif    
    {0x10b5c730, 4096,    520,   1,     0,       0x839FBA00, 520, 512, 0, 4096, 65, 520, 1},
    {0x10b5c730, 4160,    520,   1,     0,       0x85AE3980, 520, 8,   0, 4160, 1, 520, 1},

    /* Insert any new test cases above this point.
     */
    {0x0, 0, 0, 0, 0, 0, 0, 0, 0, 0}      /* zero means end. */
};

/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/


/*!**************************************************************
 * @fn fbe_ldo_test_get_device_id_for_drive_block_size(
 *                                   fbe_block_size_t block_size)
 ****************************************************************
 * @brief
 *  Determine the device id of a drive with the object id we
 *  wish to test with.
 *
 * @param  block_size - The block size of the drive we are looking for.         
 *
 * @return object id of the drive with this block size.   
 *
 * HISTORY:
 *  11/17/2008 - Created. RPF
 *
 ****************************************************************/

fbe_terminator_api_device_handle_t fbe_ldo_test_get_device_id_for_drive_block_size(fbe_block_size_t block_size)
{
    fbe_u32_t drive_index;

    /* Set device id to invalid initially.  If we don't find the block size 
     * in our array, then we return invalid to indicate error. 
     */
    fbe_terminator_api_device_handle_t device_id = FBE_OBJECT_ID_INVALID;

    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        if (block_size == fbe_ldo_test_supported_block_sizes[drive_index])
        {
            device_id = fbe_ldo_test_drive_id_array[drive_index];
            break;
        }
    } /* end for all drives */
    return device_id;
}
/******************************************
 * end fbe_ldo_test_get_device_id_for_drive_block_size()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_object_id_for_drive_block_size(
 *                                   fbe_block_size_t block_size)
 ****************************************************************
 * @brief
 *  Determine the object id of a drive with the object id we
 *  wish to test with.
 *
 * @param  block_size - The block size of the drive we are looking for.         
 *
 * @return object id of the drive with this block size.   
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

fbe_object_id_t fbe_ldo_test_get_object_id_for_drive_block_size(fbe_block_size_t block_size)
{
    fbe_u32_t drive_index;

    /* Set object id to invalid initially.  If we don't find the block size 
     * in our array, then we return invalid to indicate error. 
     */
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        if (block_size == fbe_ldo_test_supported_block_sizes[drive_index])
        {
            object_id = fbe_ldo_test_logical_drive_object_id_array[drive_index];
            break;
        }
    }
    return object_id;
}
/******************************************
 * end fbe_ldo_test_get_object_id_for_drive_block_size()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_block_size_for_drive_index(fbe_u32_t index)
 ****************************************************************
 * @brief
 *  Determine the block size for a given index of supported drive.
 *
 * @param  index - The index of this supported drive.
 *                 This is only relative to this test.
 *
 * @return block size of the drive with this supported index.
 *         We return 0 on error.
 *
 * HISTORY:
 *  09/09/2008 - Created. RPF
 *
 ****************************************************************/

fbe_block_size_t fbe_ldo_test_get_block_size_for_drive_index(fbe_u32_t index)
{
    /* Block size to invalid initially.  If this index is invalid, we just
     * return 0. 
     */
    fbe_block_size_t block_size = 0;

    if (index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX)
    {
        block_size = fbe_ldo_test_supported_block_sizes[index];
    }
    return block_size;
}
/******************************************
 * end fbe_ldo_test_get_block_size_for_drive_index()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_get_sas_drive_type_for_block_size(fbe_block_size_t block_size)
 ****************************************************************
 * @brief
 *  Returns the correct sas drive type for a given input block size.
 *
 * @param  block_size - the number of bytes per block.               
 *
 * @return fbe_sas_drive_type_t - the drive type for this block size.   
 *
 * HISTORY:
 *  8/21/2008 - Created. RPF
 *
 ****************************************************************/

fbe_sas_drive_type_t 
fbe_ldo_test_get_sas_drive_type_for_block_size(fbe_block_size_t block_size)
{
    /* Default to the CHEETA, which is a 520 drive.
     */
    fbe_sas_drive_type_t drive_type = FBE_SAS_DRIVE_CHEETA_15K;

    switch (block_size)
    {
        case 512:
            drive_type = FBE_SAS_DRIVE_UNICORN_512;
            break;
        case 4096:
            drive_type = FBE_SAS_DRIVE_UNICORN_4096;
            break;
        case 4160:
            drive_type = FBE_SAS_DRIVE_UNICORN_4160;
            break;
        case 520:
        default:
            drive_type = FBE_SAS_DRIVE_CHEETA_15K;
            break;
    };
    return drive_type;
}
/******************************************
 * end fbe_ldo_test_get_sas_drive_type_for_block_size()
 ******************************************/
/*!**************************************************************
 * @fn fbe_ldo_test_insert_drives(fbe_port_number_t port,
 *                               fbe_terminator_api_device_handle_t encl_id)
 ****************************************************************
 * @brief
 *  Inserts a set of drives for the logical drive tests.
 *
 * @param port - the port number to insert for.    
 * @param encl_id - the enclosure id to insert for.               
 *
 * @return none   
 *
 * HISTORY:
 *  8/21/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_insert_drives(fbe_u32_t port_number, fbe_u32_t encl_number, fbe_terminator_api_device_handle_t encl_id)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_terminator_api_device_handle_t drive_id = 0;
    fbe_u32_t slot = 0;
    fbe_u32_t drive_index = 0;

    /* Setup some basic info for our drive to insert.
     */
    sas_drive.backend_number = port_number;
    sas_drive.encl_number = encl_number;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
    sas_drive.capacity = 0x10b5c730;

    sas_drive.sas_address = (fbe_u64_t)0x443456780;
	memset(&sas_drive.drive_serial_number, 0, sizeof(sas_drive.drive_serial_number));
	memset(&sas_drive.drive_serial_number, 'A', sizeof(sas_drive.drive_serial_number)-1);
	sas_drive.block_size = 520;
	sas_drive.capacity = 0x1d00;
    
    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        /* Get the drive type to use based on the block size.
         */
        sas_drive.drive_type = 
            fbe_ldo_test_get_sas_drive_type_for_block_size(fbe_ldo_test_supported_block_sizes[drive_index]);
        sas_drive.slot_number = drive_index;
        sas_drive.block_size = fbe_ldo_test_supported_block_sizes[drive_index];
        sas_drive.capacity = fbe_ldo_test_default_drive_negotiate_data[drive_index].imported_capacity;

        status = fbe_terminator_api_insert_sas_drive (encl_id, drive_index, &sas_drive, &drive_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        fbe_ldo_test_drive_id_array[drive_index] = drive_id;

        /* Increment the sas address so that we will have a different addr. 
         */ 
        sas_drive.sas_address++;
        slot++;
    }
    return;
}
/******************************************
 * end fbe_ldo_test_insert_drives()
 ******************************************/
/*!**************************************************************
 * @fn fbe_ldo_test_insert_devices()
 ****************************************************************
 * @brief
 *  Inserts all of the devices including board, port, enclosure
 *  and a set of drives of different block sizes.
 *
 * @param  - None.               
 *
 * @return None.   
 *
 * HISTORY:
 *  8/21/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_insert_devices(void)
{
    fbe_status_t status;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t sas_port;
    fbe_terminator_sas_encl_info_t sas_encl;
    fbe_terminator_api_device_handle_t port_handle = 0;
    fbe_terminator_api_device_handle_t encl_id = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_CORSAIR_HW_TYPE;
    status  = fbe_terminator_api_insert_board (&board_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* insert a port
     */
    sas_port.sas_address = (fbe_u64_t)0x123456789;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 1;
    sas_port.backend_number = 0;
    
    status  = fbe_terminator_api_insert_sas_port (&sas_port, &port_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* insert an enclosure
     */
    sas_encl.backend_number = sas_port.backend_number;
    sas_encl.encl_number = 0;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    sas_encl.sas_address = (fbe_u64_t)0x123456780;
    sas_encl.connector_id = 0;
    status  = fbe_terminator_api_insert_sas_enclosure (port_handle, &sas_encl, &encl_id);/*add encl on port 0*/
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Now insert the drives.
     */
    fbe_ldo_test_insert_drives(sas_encl.backend_number, sas_encl.encl_number, encl_id);
    return;
}
/******************************************
 * end fbe_ldo_test_insert_devices()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_get_drive_error_count(fbe_u32_t port_number, fbe_terminator_api_device_handle_t encl_id, fbe_u32_t slot_number)
 ****************************************************************
 * @brief
 *  Simply get the device id of this drive and then
 *  fetch and return the error count from the terminator.
 *  
 * @param port_number
 * @param encl_id - The enclosure id.
 * @param slot_number
 * 
 * @return fbe_u32_t - Number of errors encountered.
 *
 * HISTORY:
 *  11/17/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_ldo_get_drive_error_count(fbe_block_size_t block_size)
{
    fbe_status_t status;
    fbe_u32_t error_count;

    /* Next get the number of errors encountered.
     */
    status = fbe_terminator_api_get_drive_error_count(
        fbe_ldo_test_get_device_id_for_drive_block_size(block_size),
        &error_count);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return error_count;
}
/**************************************
 * end fbe_ldo_get_drive_error_count()
 **************************************/
/*!**************************************************************
 * @fn fbe_ldo_test_validate_drives_exist()
 ****************************************************************
 * @brief
 *  Validate that the drives exist and are in the ready state.
 *
 * @param - None.               
 *
 * @return None.
 *
 * HISTORY:
 *  8/21/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_validate_drives_exist(void)
{
    fbe_status_t status;
    fbe_port_number_t port = 0;
    fbe_enclosure_number_t enclosure = 0;
    fbe_u32_t slot = 0;
    fbe_u32_t drive_index = 0;

    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        fbe_u32_t object_handle;

        /* Verify if the drive exists.
         */
        status = fbe_api_get_logical_drive_object_id_by_location(port, enclosure, slot, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

        /* Use the handle we just received to check the lifecycle state.
         */
        status = fbe_api_wait_for_object_lifecycle_state (object_handle, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Object id obtained.  Save it for later use.
         */
        fbe_ldo_test_logical_drive_object_id_array[drive_index] = (fbe_object_id_t)object_handle;
        slot++;
    }
    return;
}
/******************************************
 * end fbe_ldo_test_validate_drives_exist()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_validate_drive_block_sizes()
 ****************************************************************
 * @brief
 *  Validate that the drives we created have the correct
 *  block sizes.
 *
 * @param - None.               
 *
 * @return None.   
 *
 * HISTORY:
 *  8/21/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_validate_drive_block_sizes(void)
{
    fbe_status_t status;
    fbe_u32_t drive_index = 0;
    fbe_payload_block_operation_status_t block_status;

    /* Loop over all the drives we support.
     * Check that the block sizes of the drives are as expected. 
     */
    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        /* Setup the block size for this drive.
         */
        status = fbe_ldo_test_send_and_validate_negotiate_block_size(
            fbe_ldo_test_logical_drive_object_id_array[drive_index],
            fbe_ldo_test_default_drive_negotiate_data[drive_index].requested_block_size,
            fbe_ldo_test_default_drive_negotiate_data[drive_index].requested_opt_block_size,
            &fbe_ldo_test_default_drive_negotiate_data[drive_index].negotiate_data_expected,
            &block_status);

        /* The transport status should always be OK since we do not expect there
         * to be issues with delivering packets. 
         */
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* We also expect the payload status to be OK since this negotiate 
         * should succeed. 
         */
        MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }
    return;
}
/******************************************
 * end fbe_ldo_test_validate_drive_block_sizes()
 ******************************************/
/****************************************************************
 * fbe_logical_drive_setup()
 ****************************************************************
 * DESCRIPTION:
 *  This function performs generic setup for 
 *  fbe_logical_drive test suite.  We setup the framework 
 *  necessary for testing the ldo by initting the
 *  physical package.  
 *
 * PARAMETERS:
 *  None.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t fbe_logical_drive_setup(void) 
{
    fbe_status_t status;
    fbe_u32_t total_objects = 0;
    fbe_u32_t retries;
    void fbe_test_io_set_read_use_sg_offset(fbe_bool_t b_use_offset);
    void fbe_test_io_set_write_use_sg_offset(fbe_bool_t b_use_offset);
    fbe_test_io_set_read_use_sg_offset(FALSE);
    fbe_test_io_set_write_use_sg_offset(FALSE);

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    mut_printf(MUT_LOG_MEDIUM, "%s %s ",__FILE__, __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_terminator_api_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Enable the IO mode.  This will cause the terminator to create
     * disk files and allow I/O to these disk files.
     */ 
    status = fbe_terminator_api_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Insert all the devices.
     */
    fbe_ldo_test_insert_devices();

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical packakge ===\n");

    /* Initialize the physical package.  We need the physical package
     * to be initialized so that the logical drive objects are
     * created and available for use.
     */
    status = fbe_test_load_physical_package();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Initialize the fbe_API. */
	mut_printf(MUT_LOG_TEST_STATUS, "%s: starting fbe_api...", __FUNCTION__);
    status  = fbe_api_common_init_sim();
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, physical_package_io_entry, physical_package_control_entry);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Must set IO and control entries when using FBE API");

    status = fbe_test_load_neit_package(NULL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_NEIT, NULL, neit_package_control_entry);
    mut_printf(MUT_LOG_LOW, "=== Configuration verification successful !!!!! ===\n");

    /* Wait for the correct number of objects to be created. 
     * We will time out after polling for minute.
     */
	total_objects = 0;
    retries = 0;
    while (retries < 60)
	{
		status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (total_objects == FBE_LDO_TEST_NUMBER_OF_OBJECTS)
        {
            break;
        }
		EmcutilSleep(1000);
		mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for discovery completion... %d/%d objects created.", 
			__FUNCTION__,
			total_objects, 
			FBE_LDO_TEST_NUMBER_OF_OBJECTS);

        /* The correct number of objects should be found within 60 retries, 
         * which is 60 seconds. 
         */
        retries++;
    }
    MUT_ASSERT_INT_EQUAL(total_objects, FBE_LDO_TEST_NUMBER_OF_OBJECTS);

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Make sure the drives exist and are in the ready state. 
     */
    fbe_ldo_test_validate_drives_exist();
    mut_printf(MUT_LOG_TEST_STATUS, "Verifying logical drive existance and state....Passed");

    /* Make sure the drives have block sizes set as we expect.
     */
    fbe_ldo_test_validate_drive_block_sizes();
    mut_printf(MUT_LOG_TEST_STATUS, "=== Configuration verification successful !!!!! ===\n");

    /* Setup the send packet function for a true unit test.
     * Uncomment the below line to allow the logical drive to 
     * test I/O in isolation. 
     */
    //fbe_logical_drive_initialize_send_packet_fn_for_test();
    return status;
}
/* end fbe_logical_drive_setup() */

/*!**************************************************************
 * @fn fbe_ldo_test_lifecycle()
 ****************************************************************
 * @brief
 *  Validates the logical drive's lifecycle.
 *
 * @param - none.          
 *
 * @return none
 *
 * HISTORY:
 *  7/16/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_lifecycle(void)
{
    fbe_status_t status;
    fbe_status_t fbe_logical_drive_monitor_load_verify(void);

    /* Make sure this data is valid.
     */
    status = fbe_logical_drive_monitor_load_verify();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_ldo_test_lifecycle()
 ******************************************/
/****************************************************************
 * fbe_logical_drive_teardown()
 ****************************************************************
 * DESCRIPTION:
 *  This function performs generic cleanup for 
 *  fbe_logical_drive test suite.  We clean up the framework 
 *  necessary for testing the ldo by destroying the
 *  physical package.  
 *
 * PARAMETERS:
 *  None.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t fbe_logical_drive_teardown(void) 
{
    fbe_status_t status;

    mut_printf(MUT_LOG_MEDIUM, "%s %s ",__FILE__, __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "=== destroying all configurations ===\n");

    /* Destroy the API first.
     */
	status = fbe_api_common_destroy_sim();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_unload_neit_package();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = fbe_test_unload_physical_package();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_terminator_api_destroy();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/* end fbe_logical_drive_teardown() */

/*!***************************************************************
 * @fn fbe_logical_drive_unit_tests_setup(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * HISTORY:
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
void 
fbe_logical_drive_unit_tests_setup(void)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);
    return;
}
/* end fbe_logical_drive_unit_tests_setup() */

/*!***************************************************************
 * @fn fbe_logical_drive_unit_tests_teardown(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * HISTORY:
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
void 
fbe_logical_drive_unit_tests_teardown(void)
{
    /* Set the trace level back to the default.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_MEDIUM);

    /* We might have modified the logical drive's current methods, 
     * let's set them back to the defaults. 
     */
    fbe_logical_drive_reset_methods();

    return;
}
/* end fbe_logical_drive_unit_tests_teardown() */

/****************************************************************
 * fbe_logical_drive_add_unit_tests()
 ****************************************************************
 * DESCRIPTION:
 *  This function adds unit tests to the input suite.
 *
 * PARAMETERS:
 *  suite_p, [I] - Suite to add to.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
void fbe_logical_drive_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    fbe_ldo_test_add_bitbucket_tests(suite_p);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_edge_sizes,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_lifecycle,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    fbe_ldo_test_add_library_tests(suite_p);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_io_cmd_pool,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    fbe_ldo_test_add_monitor_tests(suite_p);
    
    /* Before we get to any state machine tests, test 
     * the entry point to the state machines. 
     */
    MUT_ADD_TEST(suite_p,
                 fbe_ldo_test_get_first_state, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_verify_state_machine_tests, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    fbe_ldo_test_add_read_unit_tests(suite_p);

    fbe_ldo_add_pre_read_descriptor_tests(suite_p);
    fbe_ldo_test_add_write_unit_tests(suite_p);
    fbe_ldo_test_add_write_same_tests(suite_p);

    /* We run the sg element tests here since it is not run anywhere else.
     */
    fbe_ldo_test_add_sg_element_tests(suite_p);

    /* Like the sg element tests, we run the 
     * edge tap unit test here since it is not run anywhere else. 
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_media_edge_tap_unit_test, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    /* This is the first test that uses the full physical package. 
     * We start small and simply make sure the logical drive can 
     * get created and destroyed properly. 
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_get_attributes_cases, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
    fbe_ldo_test_add_negotiate_block_size_tests(suite_p);

    return;
}
/* end fbe_logical_drive_add_unit_tests() */

/****************************************************************
 * fbe_logical_drive_add_io_tests()
 ****************************************************************
 * DESCRIPTION:
 *  This function adds io tests to the input suite.
 *
 * PARAMETERS:
 *  suite_p, [I] - Suite to add to.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
void fbe_logical_drive_add_io_tests(mut_testsuite_t * const suite_p)
{
    MUT_ADD_TEST(suite_p,
                 fbe_ldo_test_beyond_capacity_error, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
    MUT_ADD_TEST(suite_p,
                 fbe_ldo_test_empty_sg_error,
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
    MUT_ADD_TEST(suite_p,
                 fbe_ldo_test_pdo_failure,
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_pass_through_io,
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
    
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_zero_block_sizes,
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    //MUT_ADD_TEST(suite_p, 
    //             fbe_ldo_test_block_size_too_large,
    //             fbe_logical_drive_setup, 
    //             fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_single_write_io,
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_verify_io, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_read_io, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_io, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_verify_io, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
#if 0
/* Commented out for now since the above write io test does a good job of
 *  testing this.  
 */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_only_io,
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
#endif
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_same_io, 
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_thread_ios,  
                 fbe_logical_drive_setup, 
                 fbe_logical_drive_teardown);
    return;
}
/* end fbe_logical_drive_add_io_tests() */

fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_PHYSICAL;
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_logical_drive_test.c
 *****************************************/
