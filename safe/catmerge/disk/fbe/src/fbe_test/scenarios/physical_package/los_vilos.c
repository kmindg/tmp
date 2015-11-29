/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file los_vilos.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the test functions for the "los vilos" scenario.
 * 
 *  This scenario creates a 1 board, 1 port, 1 enclosure configuration with
 *  - 6 520 byte per block SAS drives.
 *  - 6 512 byte per block SAS drives.
 * 
 *  The scenario:
 *  - Creates and validates the configuration.
 *  - Runs write/read/check I/O to the 520 byte per block drives.
 *  - Runs write/read/check I/O to the 512 byte per block drives.
 *  - Destroys the configuration.
 *
 * @version
 *   8/21/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_sim_server.h" 
#include "pp_utils.h"

char * los_vilos_short_desc = "Logical Drive I/O Test for 520 and 512 byte per block drives.";
char * los_vilos_long_desc ="\
The Los Vilos scenario tests I/Os against the logical drive.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] 6 SAS drives\n\
        [PP] 6 SATA drives\n\
        [PP] 12 logical drives\n\
\n"\
"STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 2: Run write/read/compare rdgen test to 520 drives.\n\
STEP 3: Run write/read/compare rdgen test to 512 drives.\n\
STEP 4: Run write-same/read/compare rdgen test to 520 drives.\n\
STEP 5: Run write-same/read/compare rdgen test to 512 drives.\n";

/* This is the number of drives in the scenario. 
 * 1 board 
 * 1 port 
 * 1 enclosure 
 * 12 physical drives 
 */
#define LOS_VILOS_NUMBER_OF_OBJECTS (3 + LOS_VILOS_MAX_DRIVES)

/* We have one enclosure split between 520 and 512 drives.
 * 520 is 0..5 
 * 512 is 6..11 
 */
#define LOS_VILOS_520_DRIVE_COUNT 6
#define LOS_VILOS_512_DRIVE_COUNT 6
#define LOS_VILOS_520_DRIVE_NUMBER_FIRST 0
#define LOS_VILOS_520_DRIVE_NUMBER_LAST 5
#define LOS_VILOS_512_DRIVE_NUMBER_FIRST 6
#define LOS_VILOS_512_DRIVE_NUMBER_LAST 11

/* This is the max number of drives we use in this test.
 */
#define LOS_VILOS_MAX_DRIVES 12

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/*! @var fbe_los_vilos_520_io_case_array 
 *  
 *  @brief This is the set of write/read/compare test cases to be run against
 *         520 drives in this test. These ranges are typically arbitrary.  We
 *         try to cover interesting combination of lbas and block counts. The
 *         max block counts of 1024 or 0x5000 are arbitrary. 
 */
fbe_api_rdgen_test_io_case_t fbe_los_vilos_520_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       520,    0x0,    0x3,     1,       1024,     1,       1,        1},
    {520,       520,    0x100,  0x110,   1,       50,       1,       1,        1},

    /* Try some very large block counts. Do not go beyond 0x800 since this is the max
     * the backend can take at this time (1 MB = 0x800 or 2048 blocks). 
     */
    {520,       520,    0,      0x5,    0x80,    0x800,  0x80,       1,        1},

    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_API_RDGEN_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};
/* end fbe_los_vilos_520_io_case_array global structure */

/*! @var fbe_los_vilos_512_io_case_array 
 *  @brief This is the set of write/read/compare I/O test cases to be run
 *         against 512 drives in this test. We try to test cases that will run
 *         I/O over all combinations of the optimal block size. 
 *  
 * The max block counts of 1024 or 0x5000 are arbitrary.
 *  
 * We intentionally try some ranges that start at something other than 0..64. 
 */
fbe_api_rdgen_test_io_case_t fbe_los_vilos_512_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       512,       0,    33,     1,       65,      1,       64,       65},
    {520,       512,       32,   65,     1,       65,      1,       64,       65},
    {520,       512,       64,   129,    1,       65,      1,       64,       65},

    /* Try some ranges that use larger lbas. 
     * These drives are typically 32 mb, so we are limited at present to an lba 
     * of 0..0xFC0F. 
     */
    {520,       512,   0xFF0,  0x1047,   1,       32,      1,       64,       65},

    /* For some other ranges try some very large block counts.  0x800 is the max xfer size
     * of the backend.  We use -2 so that the total area with pr does not exceed 0x800. 
     */
    {520,       512,    5,      7,    0x80,    0x800 - 2,  0x80,      64,       65},
    {520,       512,    100,    103,    0x80,    0x800 - 2,  0x80,      64,       65},
#if 0
    /* Lossy cases.
     * In these cases we are wasting a bit of space.  For example in the first 
     * case we have 2 520s mapped to 3 512s. 
     */
    {520,       512,       0,    16,   1,       0x10,     1,       2,        3},
    {520,       512,       0,    16,   1,       0x10,     1,       4,        5},
    {520,       512,       0,    16,   1,       0x10,     1,       6,        7},
#endif
    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_API_RDGEN_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};
/* end fbe_los_vilos_512_io_case_array global structure */

/*! @var fbe_los_vilos_520_write_same_io_case_array
 * 
 *  @brief This is the set of write same, read, compare cases to be run
 *         against the 520 byte per block drives.
 * 
 *         We have a few test cases to test small write sames and large write
 *         sames.
 */
fbe_api_rdgen_test_io_case_t fbe_los_vilos_520_write_same_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       520,     0,     5,     1,       128,        1,     1,       1},
    {520,       520,     0,     5,    512,     0x800,      512,     1,       1},
    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_API_RDGEN_TEST_IO_INVALID_FIELD,         0,         0,    0,    0,       0},

};
/* end fbe_los_vilos_520_write_same_io_case_array global structure */

/*! @var fbe_los_vilos_512_write_same_io_case_array
 * 
 *  @brief This is the set of write same, read, compare cases to be run
 *         against the 512 byte per block drives.
 * 
 *         Note that all the test cases need to be multiples of 64, since
 *         all write sames need to be aligned to the optimal block size of 64.
 */
fbe_api_rdgen_test_io_case_t fbe_los_vilos_512_write_same_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,       512,     0,     5,     64,      1024,    64,       64,      65},
    {520,       512,     0,     5,     1024,    2048,   1024,     64,      65},

    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_API_RDGEN_TEST_IO_INVALID_FIELD,         0,         0,    0,    0,       0},

};
/* end fbe_los_vilos_512_write_same_io_case_array global structure */

/*!**************************************************************
 * los_vilos_get_drive_error_count()
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
 * @author
 *  11/17/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t los_vilos_get_drive_error_count(fbe_u32_t port_number, 
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
 * end los_vilos_get_drive_error_count()
 **************************************/

/*!**************************************************************
 * @fn los_vilos()
 ****************************************************************
 * @brief
 *  This function executes the los vilos scenario.
 * 
 *  This scenario will kick off a write/read/check test on 1 520 bytes per block
 *  drive and 1 512 bytes per block drive.
 * 
 *  Once that completes, we kick off a write smae/read/check test on 1 520 bytes
 *  per block drive and 1 512 bytes per block drive.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  08/18/2008  Created.  RPF. 
 ****************************************************************/

void los_vilos(void)
{
    fbe_u32_t object_handle;
    fbe_object_id_t object_id_520;
    fbe_object_id_t object_id_512;
    fbe_status_t status;
    fbe_u32_t error_count;
    fbe_u32_t test_level = fbe_test_pp_util_get_extended_testing_level();
    fbe_u32_t test_case_count = FBE_U32_MAX;

    if (test_level == 0)
    {
        test_case_count = 1;
    }

    /* Verify if the drive exists.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, LOS_VILOS_520_DRIVE_NUMBER_FIRST, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    object_id_520 = (fbe_object_id_t)object_handle;

    /* Verify if the drive exists.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, LOS_VILOS_512_DRIVE_NUMBER_FIRST, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    object_id_512 = (fbe_object_id_t)object_handle;

    /* This scenario uses the "los_vilos" configuration, which is a 1 port, 1 
     * enclosure configuration with 6 520 bytes per block SAS drives and 6 512 
     * bytes per block SAS drives. 
     */
    mut_printf(MUT_LOG_LOW, "los_vilos:  Starting write/read/check test to 520 byte per block drive.");

    /* Kick off write/read/check I/O to the 520 bytes per block drive.
     */
    status = fbe_api_rdgen_test_io_run_write_read_check_test(fbe_los_vilos_520_io_case_array,
                                                   object_id_520,
                                                   test_case_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "los_vilos:  write/read/check test to 520 byte per block drive passed.");

    /* Make sure we did not take any errors in the terminator.
     */
    error_count = los_vilos_get_drive_error_count(0, 0, LOS_VILOS_520_DRIVE_NUMBER_FIRST);
    MUT_ASSERT_INT_EQUAL(error_count, 0);

    /* Kick off write/read/check I/O to the 512 bytes per block drive.
     */
    mut_printf(MUT_LOG_LOW, "los_vilos:  Starting write/read/check test to 512 byte per block drive.");
    status = fbe_api_rdgen_test_io_run_write_read_check_test(fbe_los_vilos_512_io_case_array,
                                                   object_id_512,
                                                   test_case_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "los_vilos:  write/read/check test to 512 byte per block drive passed.");

    /* Make sure we did not take any errors in the terminator.
     */
    error_count = los_vilos_get_drive_error_count(0, 0, LOS_VILOS_512_DRIVE_NUMBER_FIRST);
    MUT_ASSERT_INT_EQUAL(error_count, 0);

    /* Kick off Write Same I/O to the 520 bytes per block drive.
     */
    mut_printf(MUT_LOG_LOW, "los_vilos:  Starting write same/read/check test to 520 byte per block drive.");
    status = fbe_api_rdgen_test_io_run_write_same_test(fbe_los_vilos_520_write_same_io_case_array,
                                             object_id_520,
                                             test_case_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "los_vilos:  write same/read/check test to 520 byte per block drive passed.");

    /* Make sure we did not take any errors in the terminator.
     */
    error_count = los_vilos_get_drive_error_count(0, 0, LOS_VILOS_520_DRIVE_NUMBER_FIRST);
    MUT_ASSERT_INT_EQUAL(error_count, 0);

    /* Kick off Write Same I/O to the 512 bytes per block drive.
     */
    mut_printf(MUT_LOG_LOW, "los_vilos:  Starting write same/read/check test to 512 byte per block drive.");
    status = fbe_api_rdgen_test_io_run_write_same_test(fbe_los_vilos_512_write_same_io_case_array,
                                             object_id_512,
                                             test_case_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "los_vilos:  write/read/check test to 512 byte per block drive passed.");

    /* Make sure we did not take any errors in the terminator.
     */
    error_count = los_vilos_get_drive_error_count(0, 0, LOS_VILOS_512_DRIVE_NUMBER_FIRST);
    MUT_ASSERT_INT_EQUAL(error_count, 0);
    return;
}
/******************************************
 * end los_vilos()
 ******************************************/

/*!**************************************************************
 * @fn fbe_los_vilos_validate_drive_negotiate_info()
 ****************************************************************
 * @brief
 *  Validate that the drives we created have the correct
 *  block sizes.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  8/21/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_los_vilos_validate_drive_negotiate_info(void)
{
#if 0
    fbe_status_t status;
    fbe_u32_t drive_index = 0;
    fbe_port_number_t port = 0;
    fbe_enclosure_number_t enclosure = 0;

    /* Loop over all the 520 drives we support.
     * Check that the block sizes of the drives are as expected. 
     */
    for ( drive_index = LOS_VILOS_520_DRIVE_NUMBER_FIRST; 
          drive_index <= LOS_VILOS_520_DRIVE_NUMBER_LAST; 
          drive_index++)
    {
        fbe_u32_t object_handle;
        fbe_object_id_t object_id;

        /* Verify if the drive exists.
         */
        status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, drive_index, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

        object_id = (fbe_object_id_t)object_handle;

        /* Validate the block size for this drive. 
         * We expect a logical block size = 520 with optimal block size of 1 
         * And a physical block size of 520 with physical optimal size of 1.
         */
        status = fbe_api_physical_drive_negotiate_and_validate_block_size(object_id, 520, 1, 520, 1);

        /* If we cannot send a negotiate, then something went
         * wrong initializing the physical package.
         */
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Loop over all the 512 drives we support.
     * Check that the block sizes of the drives are as expected. 
     */
    for ( drive_index = LOS_VILOS_512_DRIVE_NUMBER_FIRST; 
          drive_index <= LOS_VILOS_512_DRIVE_NUMBER_LAST; 
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
        status = fbe_api_logical_drive_negotiate_and_validate_block_size(object_id, 520, 64, 512, 65);

        /* If we cannot send a read capacity, then something went
         * wrong initializing the physical package.
         */
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
#endif
    return;
}
/******************************************
 * end fbe_los_vilos_validate_drive_negotiate_info()
 ******************************************/
/*!**************************************************************
 * @fn los_vilos_load_and_verify()
 ****************************************************************
 * @brief
 *  This loads the los vilos configuration and verifies all the objects
 *  were created.
 *
 * @param None.
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  8/25/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t los_vilos_load_and_verify(void)
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

    /* Load the test config */
    status = fbe_test_load_los_vilos_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s: los_vilos config loaded.", __FUNCTION__);

    /* Initialize the fbe_api on server. */ 
    mut_printf(MUT_LOG_LOW, "%s: starting fbe_api...", __FUNCTION__);
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the physical package. */
    mut_printf(MUT_LOG_LOW, "%s: starting physical package...", __FUNCTION__);
    fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    
    fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects.
     */
    status = fbe_api_wait_for_number_of_objects(LOS_VILOS_NUMBER_OF_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

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
                                                      FBE_LIFECYCLE_STATE_READY, 
                                                      READY_STATE_WAIT_TIME,
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

    /* Use the handle to get lifecycle state.
     */  
    status = fbe_api_wait_for_object_lifecycle_state (object_handle, 
                                                      FBE_LIFECYCLE_STATE_READY, 
                                                      READY_STATE_WAIT_TIME,
                                                      FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Use the handle to get port number.
     */  
    status = fbe_api_get_object_port_number (object_handle, &port_number);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(port_number == port_number);
	mut_printf(MUT_LOG_LOW, "Verifying enclosure existance and state....Passed");

    /* Check for the physical drives on the enclosure.
     */  
    for (drive_number = 0; drive_number < LOS_VILOS_MAX_DRIVES; drive_number++)
    {
        /* Get the handle of the port and validate drive exists.
         */  
        status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, drive_number, &object_handle);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

        /* Use the handle to get lifecycle state.
         */  
        status = fbe_api_wait_for_object_lifecycle_state (object_handle, 
                                                          FBE_LIFECYCLE_STATE_READY, 
                                                          READY_STATE_WAIT_TIME,
                                                          FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Use the handle to get port number of the drive.
         */ 
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
	MUT_ASSERT_TRUE(total_objects == LOS_VILOS_NUMBER_OF_OBJECTS);
	mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    /* Make sure the block sizes of the drives are as expected.
     */
	mut_printf(MUT_LOG_LOW, "Verifying negotiate block size data....");
    fbe_los_vilos_validate_drive_negotiate_info();
	mut_printf(MUT_LOG_LOW, "Verifying negotiate block size data....Passed");
	mut_printf(MUT_LOG_LOW, "%s: configuration verification SUCCEEDED!", __FUNCTION__);

	return FBE_STATUS_OK;
}
/******************************************
 * end los_vilos_load_and_verify()
 ******************************************/
/*!**************************************************************
 * @fn los_vilos_unload()
 ****************************************************************
 * @brief
 *  Unload for los vilos.
 *
 * @param none.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * @date
 *  6/11/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t los_vilos_unload(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "unload neit failed");

    status = fbe_test_physical_package_tests_config_unload();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*************************
 * end file los_vilos.c
 *************************/
 
 
