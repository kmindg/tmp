/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * los_vilos_sata.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the test functions for the "los vilos sata" scenario.
 * 
 *  This scenario creates a 1 board, 1 port, 1 enclosure configuration with
 *  - 12 512 byte per block SATA drives.
 * 
 *  The scenario:
 *  - Creates and validates the configuration.
 *  - Runs write/read/check I/O to the 512 byte per block SATA drives.
 *  - Destroys the configuration.
 *
 * HISTORY
 *   1/12/09 Ported almost directly from the los_vilos test that
 *           was originally created by RPF.
 *   11/23/2009:  Migrated from zeus_ready_test. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_sim_server.h" 
#include "pp_utils.h"

/* This is the number of drives in the scenario. 
 * 1 board 
 * 1 port 
 * 1 enclosure 
 * 12 physical drives 
 * 12 logical drives 
 */
#define LOS_VILOS_SATA_NUMBER_OF_OBJECTS 27

/* We have one enclosure with 12 SATA 512 byte/block drives.
 */

#define LOS_VILOS_SATA_512_DRIVE_COUNT 12
#define LOS_VILOS_SATA_512_DRIVE_NUMBER_FIRST 0
#define LOS_VILOS_SATA_512_DRIVE_NUMBER_LAST 11

/* This is the max number of drives we use in this test.
 */
#define LOS_VILOS_SATA_MAX_DRIVES 12


 /*************************
  *   FUNCTION DEFINITIONS
  *************************/


/*! @var fbe_los_vilos_sata_512_io_case_array 
 *  @brief This is the set of write/read/compare I/O test cases to be run
 *         against the 512 byte/block SATA drives in this test. We try to
 *         test cases that will run I/O over all combinations of the
 *         optimal block size. 
 *  
 * The max block counts of 1024 or 0x5000 are arbitrary.
 *  
 * We intentionally try some ranges that start at something other than 0..64. 
 */
 
fbe_test_io_case_t fbe_los_vilos_sata_512_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       512,       0,    33,     1,       1024,      1,       64,       65},
    {520,       512,       32,   65,     1,       1024,      1,       64,       65},
    {520,       512,       64,   129,    1,       1024,      1,       64,       65},

    /* Try some ranges that use larger lbas. 
     * These drives are typically 32 mb, so we are limited at present to an lba 
     * of 0..0xFC0F. 
     */
    {520,       512,   0xFF0,  0x1047,   1,       1024,      1,       64,       65},
    {520,       512,   0x3570, 0x35F5,   1,       1024,      1,       64,       65},
    {520,       512,   0x8FF0, 0x9045,   1,       1024,      1,       64,       65},
    {520,       512,   0xBFF3, 0xC041,   1,       1024,      1,       64,       65},

    /* For some other ranges try some very large block counts.
     */
    {520,       512,    0,      0x10,    0x80,    0x5000,  0x80,      64,       65},
    {520,       512,    1,      0x11,    0x80,    0x5000,  0x80,      64,       65},

    /* Lossy cases.
     * In these cases we are wasting a bit of space.  For example in the first 
     * case we have 2 520s mapped to 3 512s. 
     */
    {520,       512,       0,    16,   1,       0x10,     1,       2,        3},
    {520,       512,       0,    16,   1,       0x10,     1,       4,        5},
    {520,       512,       0,    16,   1,       0x10,     1,       6,        7},

    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};


/*! @var fbe_los_vilos_sata_512_write_same_io_case_array
 * 
 *  @brief This is the set of write same, read, compare cases to be run
 *         against the 512 byte per block SATA drives.
 * 
 *         Note that all the test cases need to be multiples of 64, since
 *         all write sames need to be aligned to the optimal block size of 64.
 */
fbe_test_io_case_t fbe_los_vilos_sata_512_write_same_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       512,     0,     4096,   64,      1024,    64,       64,      65},
    {520,       512,     0,     64,     1024,    51200,   1024,     64,      65},

    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD,         0,         0,    0,    0,       0},

};
/* end fbe_los_vilos_sata_512_write_same_io_case_array global structure */


/*!**************************************************************
 * @fn los_vilos_sata_get_drive_error_count(fbe_u32_t port_number, fbe_api_terminator_device_handle_t encl_id, fbe_u32_t slot_number)
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
 *   1/12/09 Ported almost directly from the los_vilos test that
 *           was originally created by RPF.
 *
 ****************************************************************/

fbe_u32_t los_vilos_sata_get_drive_error_count(fbe_u32_t port_number, 
                                               fbe_u32_t enclosure_number, 
                                               fbe_u32_t slot_number)
{
    fbe_api_terminator_device_handle_t drive_device_id;
    fbe_api_terminator_device_handle_t enclosure_device_id;
    fbe_status_t status;
    fbe_u32_t error_count;

    /* First get the enclosure's device id.
     */
    status = fbe_api_terminator_get_enclosure_handle(port_number, enclosure_number, &enclosure_device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* First determine the device id for this port/encl/slot.
     */
    status = fbe_api_terminator_get_drive_handle(port_number, enclosure_number, slot_number, &drive_device_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Next get the number of errors encountered.
     */
    status = fbe_api_terminator_get_drive_error_count(drive_device_id, &error_count);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return error_count;
}
/**************************************
 * end los_vilos_sata_get_drive_error_count()
 **************************************/


/*!**************************************************************
 * @fn los_vilos_sata()
 ****************************************************************
 * @brief
 *  This function executes the los vilos sata scenario.
 * 
 *  This scenario will kick off a write/read/check test on 1
 *  512 bytes/block SATA drive.
 * 
 *  Once that completes, we kick off a write same/read/check test on 1
 *  512 bytes/block SATA drive.
 *
 * @param None.               
 *
 * @return None.
 *
 * HISTORY:
 *   1/12/09 Ported almost directly from the los_vilos test that
 *           was originally created by RPF.
 ****************************************************************/

void los_vilos_sata(void)
{
    fbe_u32_t object_handle;
    fbe_object_id_t object_id_512;
    fbe_status_t status;
    fbe_u32_t error_count;

    /* Verify if the drive exists.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, LOS_VILOS_SATA_512_DRIVE_NUMBER_FIRST, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    object_id_512 = (fbe_object_id_t)object_handle;

    /* This scenario uses the "los_vilos_sata" configuration, which is a 1 port, 1 
     * enclosure configuration with 12  512 bytes/block SATA drives. 
     */

    /* Kick off write/read/check I/O to the 512 bytes per block drive.
     */
    mut_printf(MUT_LOG_LOW, "los_vilos_sata:  Starting write/read/check test to 512 byte/block SATA drive.");
    status = fbe_test_io_run_write_read_check_test(fbe_los_vilos_sata_512_io_case_array,
                                                   object_id_512,
                                                   /* No max case to run.
                                                    * Just run all the test cases.  
                                                    */
                                                   FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "los_vilos_sata:  write/read/check test to 512 byte/block SATA drive passed.");

    /* Make sure we did not take any errors in the terminator.
     */
    error_count = los_vilos_sata_get_drive_error_count(0, 0, LOS_VILOS_SATA_512_DRIVE_NUMBER_FIRST);
    MUT_ASSERT_INT_EQUAL(error_count, 0);

    /* Kick off Write Same I/O to the 512 bytes/block SATA drive.
     */
    mut_printf(MUT_LOG_LOW, "los_vilos_sata:  Starting write same/read/check test to 512 byte/block SATA drive.");
    status = fbe_test_io_run_write_same_test(fbe_los_vilos_sata_512_write_same_io_case_array,
                                             object_id_512,
                                             /* No max case to run.
                                              * Just run all the test cases.  
                                              */
                                             FBE_TEST_IO_INVALID_CASE_COUNT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "los_vilos_sata:  write/read/check test to 512 byte/block SATA drive passed.");

    /* Make sure we did not take any errors in the terminator.
     */
    error_count = los_vilos_sata_get_drive_error_count(0, 0, LOS_VILOS_SATA_512_DRIVE_NUMBER_FIRST);
    MUT_ASSERT_INT_EQUAL(error_count, 0);
    return;
}
/******************************************
 * end los_vilos_sata()
 ******************************************/


/*!**************************************************************
 * @fn fbe_los_vilos_sata_validate_drive_negotiate_info()
 ****************************************************************
 * @brief
 *  Validate that the drives we created have the correct
 *  block size.
 *
 * @param None.               
 *
 * @return None.   
 *
 * HISTORY:
 *   1/12/09 Ported almost directly from the los_vilos test that
 *           was originally created by RPF.
 *
 ****************************************************************/

void fbe_los_vilos_sata_validate_drive_negotiate_info(void)
{
#if 0
    fbe_status_t status;
    fbe_u32_t drive_index = 0;
    fbe_port_number_t port = 0;
    fbe_enclosure_number_t enclosure = 0;

    /* Loop over all the 512 byte/block SATA drives.
     * Check that the block sizes of the drives are as expected. 
     */
    for ( drive_index = LOS_VILOS_SATA_512_DRIVE_NUMBER_FIRST; 
          drive_index <= LOS_VILOS_SATA_512_DRIVE_NUMBER_LAST; 
          drive_index++)
    {
        fbe_u32_t object_handle;
        fbe_object_id_t object_id;

        /* Verify if the drive exists.
         */
        status = fbe_api_get_logical_drive_object_id_by_location(port, enclosure, drive_index, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

        object_id = (fbe_object_id_t)object_handle;

        /* Validate the block size for this drive.
         * We expect a logical block size = 520 with optimal block size of 64. 
         * And a physical block size of 512 with physical optimal size of 65. 
         */
        status = fbe_test_io_negotiate_and_validate_block_size(object_id, 520, 64, 512, 65);

        /* If we cannot send a read capacity, then something went
         * wrong initializing the physical package.
         */
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
#endif
    return;
}
/******************************************
 * end fbe_los_vilos_sata_validate_drive_negotiate_info()
 ******************************************/


/*!**************************************************************
 * @fn los_vilos_sata_load_and_verify()
 ****************************************************************
 * @brief
 *  This loads the los vilos sata configuration and verifies all the objects
 *  were created.
 *
 * @param None.
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *   1/12/09 Ported almost directly from the los_vilos test that
 *           was originally created by RPF.
 *
 ****************************************************************/

fbe_status_t los_vilos_sata_load_and_verify(void)
{
	fbe_class_id_t		class_id;
	fbe_u32_t			object_handle = 0;
	fbe_u32_t			port_number = 0;
	fbe_u32_t			enclosure_number = 0;
	fbe_u32_t			drive_number;
	fbe_u32_t			total_objects = 0;
	fbe_status_t		status;

    /* There is some verbose tracing in the I/O path, so let's turn down 
     * the tracing level. 
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* Before initializing the physical package, initialize the terminator.
     */ 
	status = fbe_api_terminator_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	mut_printf(MUT_LOG_LOW, "%s: terminator initialized.", __FUNCTION__);

    /* Enable the IO mode.  This will cause the terminator to create
     * disk files as drive objects are created.
     */ 
    status = fbe_api_terminator_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Load the test config*/
    status = fbe_test_load_los_vilos_sata_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: los vilos sata config loaded.", __FUNCTION__);

	/* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects.
     */
    while (total_objects != LOS_VILOS_SATA_NUMBER_OF_OBJECTS)
    {
        status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
        EmcutilSleep(1000);
        mut_printf(MUT_LOG_LOW, "%s: Wait for discovery completion... %d/%d objects created.", 
                   __FUNCTION__,
                   total_objects, 
                   LOS_VILOS_SATA_NUMBER_OF_OBJECTS);
    }

    mut_printf(MUT_LOG_LOW, "%s: starting configuration verification...", __FUNCTION__);

    /* We inserted a fleet board so we check for it board is always object id 0
     * ??? Maybe we need an interface to get the board object id. 
     */  
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port.
     * Get the handle of the port and validate port exists.
     */  
    status = fbe_api_get_port_object_id_by_location (port_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Use the handle to get lifecycle state.
     */  
    status = fbe_api_wait_for_object_lifecycle_state (object_handle, 
                                                      FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME,
                                                      FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number.  Make sure the port exists and the
     * port number is zero (we only have one port). 
     */  
    status = fbe_api_get_object_port_number(object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == 0);
    mut_printf(MUT_LOG_LOW, "Verifying port existance, state and number ...Passed");

    /* We should have exactly one enclosure on the port.
     * Get the handle of the port and validate enclosure exists.
     */
    status = fbe_api_get_enclosure_object_id_by_location (port_number, enclosure_number, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    /* Use the handle to get lifecycle state*/
    status = fbe_zrt_wait_enclosure_status (port_number, enclosure_number, FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number*/
    status = fbe_api_get_object_port_number (object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == port_number);
	mut_printf(MUT_LOG_LOW, "Verifying enclosure existance and state....Passed");

    /* Check for the physical drives on the enclosure.
     */  
    for (drive_number = 0; drive_number < LOS_VILOS_SATA_MAX_DRIVES; drive_number++)
    {
        /* Get the handle of the port and validate drive exists.
         */  
        status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, drive_number, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

        /* Use the handle to get lifecycle state*/
        status = fbe_test_pp_util_verify_pdo_state(port_number, 
                                                    enclosure_number, 
                                                    drive_number, 
                                                    FBE_LIFECYCLE_STATE_READY, 
                                                    READY_STATE_WAIT_TIME);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Use the handle to get port number of the drive*/
        status = fbe_api_get_object_port_number (object_handle, &port_number);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(port_number == port_number);

    }  /* End for all drives. */

	mut_printf(MUT_LOG_LOW, "Verifying physical drive existance and state....Passed");

	/* Let's see how many objects we have...
	 * 1 board
	 * 1 port
	 * 1 enclosure
	 * 12 physical drives
	 * 12 logical drives
	 * 27 objects
     */
	status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(total_objects == LOS_VILOS_SATA_NUMBER_OF_OBJECTS);
	mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    /* Make sure the block sizes of the drives are as expected.
     */
	mut_printf(MUT_LOG_LOW, "Verifying negotiate block size data....");
    fbe_los_vilos_sata_validate_drive_negotiate_info();
	mut_printf(MUT_LOG_LOW, "Verifying negotiate block size data....Passed");
	mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

	return FBE_STATUS_OK;
}
/******************************************
 * end los_vilos_sata_load_and_verify()
 ******************************************/

/*************************
 * end file los_vilos_sata.c
 *************************/
 
 
