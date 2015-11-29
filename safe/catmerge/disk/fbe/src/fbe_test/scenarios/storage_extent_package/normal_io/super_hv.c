
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file super_hv.c
 ***************************************************************************
 *
 * @brief   This file contains the 4K (a.k.a. Super High Video) block size
 *          test for the PDO (Physical Drive Object).
 *
 * @version
 *  11/19/2013  Ron Proulx  - Created.
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
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "pp_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"

char * super_hv_short_desc = "PDO 4K drive block size tests.";
char * super_hv_long_desc =
    "\n"
    "\n"
    "After bring up the physical package, insert a simulated 4K drive and validate 520-bps I/O.\n"
    ;

/*!*******************************************************************
 * @def     SUPER_HV_PHYSICAL_BLOCK_SIZE
 *********************************************************************
 * @brief   Size in bytes of a super hv drive.
 *********************************************************************/
#define SUPER_HV_PHYSICAL_BLOCK_SIZE                (4160)

/*!*******************************************************************
 * @def     SUPER_HV_ALIGNED_REQUEST_SIZE
 *********************************************************************
 * @brief   The minimum number of blocks required to generate an 
 *          aligned request.
 *********************************************************************/
#define SUPER_HV_ALIGNED_REQUEST_SIZE               (SUPER_HV_PHYSICAL_BLOCK_SIZE / FBE_BE_BYTES_PER_BLOCK)

/*!*******************************************************************
 * @def SUPER_HV_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT
 *********************************************************************
 * @brief Default number of blocks to adjust capacity to.
 *********************************************************************/
#define SUPER_HV_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT  0x10000

/*!*******************************************************************
 * @def     SUPER_HV_RDGEN_MAX_OBJECTS_TO_TEST
 *********************************************************************
 * @brief   Maximum number of objects that are under test at a time.
 *********************************************************************/
#define SUPER_HV_RDGEN_MAX_OBJECTS_TO_TEST          1

/*!*******************************************************************
 * @def SUPER_HV_IO_COUNT
 *********************************************************************
 * @brief Number of IOs to issue to each PDO for this test.
 *********************************************************************/
#define SUPER_HV_IO_COUNT                           100

/*!*******************************************************************
 * @def SUPER_HV_FAILED_IO_COUNT
 *********************************************************************
 * @brief Number of IOs to issue to each PDO for this test.
 *********************************************************************/
#define SUPER_HV_FAILED_IO_COUNT                    5

/*!*******************************************************************
 * @def     SUPER_HV_RDGEN_MIN_BLOCK_COUNT
 *********************************************************************
 * @brief   This is the minimum blocks per I/O for this test.
 *********************************************************************/
#define SUPER_HV_RDGEN_MIN_BLOCK_COUNT              1

/*!*******************************************************************
 * @def     SUPER_HV_RDGEN_MAX_BLOCK_COUNT
 *********************************************************************
 * @brief   This is the maximum blocks per I/O for this test.
 *********************************************************************/
#define SUPER_HV_RDGEN_MAX_BLOCK_COUNT              (FBE_PROVISION_DRIVE_DEFAULT_CHUNK_SIZE)

/*!*******************************************************************
 * @def     SUPER_HV_RDGEN_MIN_ALIGNED_BLOCK_COUNT
 *********************************************************************
 * @brief   This is the minimum blocks per aligned I/O for this test.
 *********************************************************************/
#define SUPER_HV_RDGEN_MIN_ALIGNED_BLOCK_COUNT      (SUPER_HV_ALIGNED_REQUEST_SIZE)

/*!*******************************************************************
 * @def     SUPER_HV_RDGEN_MAX_ALIGNED_BLOCK_COUNT
 *********************************************************************
 * @brief   This is the maximum blocks per I/O for this test.
 *********************************************************************/
#define SUPER_HV_RDGEN_MAX_ALIGNED_BLOCK_COUNT      ((SUPER_HV_RDGEN_MAX_BLOCK_COUNT / SUPER_HV_ALIGNED_REQUEST_SIZE) * SUPER_HV_ALIGNED_REQUEST_SIZE)

/*!*******************************************************************
 * @def     SUPER_HV_RDGEN_STD_ALIGNED_BLOCK_COUNT
 *********************************************************************
 * @brief   This is the standard blocks per I/O for this test.
 *********************************************************************/
#define SUPER_HV_RDGEN_STD_ALIGNED_BLOCK_COUNT      (SUPER_HV_RDGEN_MAX_ALIGNED_BLOCK_COUNT / 2)

/*!*******************************************************************
 * @def SUPER_HV_THREAD_COUNT
 *********************************************************************
 * @brief Thread count for rdgen contexts in this test
 *********************************************************************/
#define SUPER_HV_THREAD_COUNT                       1

/*!*******************************************************************
 * @var     super_hv_rdgen_context
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t super_hv_rdgen_context[SUPER_HV_RDGEN_MAX_OBJECTS_TO_TEST];

/*!***************************************************************************
 *          super_hv_test_insert_4k_drive()  
 *****************************************************************************
 *
 * @brief   Using the existing APIs insert and validate a simulated drive that
 *          has a block size of 4K (520 * 8 = 4160-bps). 
 *
 * @param   bus_to_insert_at - Bus to insert drive at
 * @param   enclosure_to_insert_at - Enclosure to insert at  
 * @param   slot_to_insert_at - The slot (in bus_encl_slot) to insert the drive at
 * @param   drive_handle_p - Pointer to handle to populate
 * @param   pdo_object_id_p - Address to populate resulting pdo object id.
 *
 * @return - None.
 *
 * @author
 *  11/19/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t super_hv_test_insert_4k_drive(fbe_u32_t bus_to_insert_at,
                                                  fbe_u32_t enclosure_to_insert_at,
                                                  fbe_u32_t slot_to_insert_at,
                                                  fbe_api_terminator_device_handle_t *drive_handle_p, 
                                                  fbe_object_id_t *pdo_object_id_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_sas_address_t   new_sas_address;
    fbe_lba_t           capacity;
    fbe_terminator_sas_drive_info_t drive_info;
    fbe_object_id_t     object_id;

    /* Initialize the object id to invalid.
     */
    *pdo_object_id_p = FBE_OBJECT_ID_INVALID;

    /* Insert the drive in the next available slot.  The capacity
     * is in terms of client block size (FBE_BE_BYTES_PER_BLOCK)
     */
    capacity = (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY + SUPER_HV_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    new_sas_address = fbe_test_pp_util_get_unique_sas_address();
    fbe_test_pp_util_insert_sas_drive_extend(bus_to_insert_at, enclosure_to_insert_at, slot_to_insert_at, 
                                             SUPER_HV_PHYSICAL_BLOCK_SIZE, 
                                             capacity,
                                             new_sas_address, 
                                             FBE_SAS_DRIVE_UNICORN_4160, 
                                             drive_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait a second before we attempt to get the drive object id.
     */
    fbe_api_sleep(1000);

    /* Get the serial number that was generated.
     */
    status = fbe_api_terminator_get_sas_drive_info(*drive_handle_p, &drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the object id from the serial number.
     */
    status  = fbe_api_physical_drive_get_id_by_serial_number(&drive_info.drive_serial_number[0],
                                                             TERMINATOR_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, 
                                                             &object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s - 4K drive obj: 0x%x (%d_%d_%d) created.",
               __FUNCTION__, object_id,
               bus_to_insert_at, enclosure_to_insert_at, slot_to_insert_at);

    /* Now wait for the drive to become ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     READY_STATE_WAIT_TIME, 
                                                     FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Success
     */
    *pdo_object_id_p = object_id;
    return status;
}
/*************************************
 * end super_hv_test_insert_4k_drive()
 *************************************/

/*!***************************************************************************
 *          super_hv_test_4k_aligned_io()  
 *****************************************************************************
 *
 * @brief   Run aligned (multiple of 8 - 520-bps blocks) random I/O.  All pdo
 *          client have a block size of 520-bps.
 *
 * @param   pdo_object_id - The object id to run I/O against
 * @param   rdgen_operation - rdgen operation to use for the random I/O
 *
 * @return  status
 *
 * @author
 *  11/19/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t super_hv_test_4k_aligned_io(fbe_object_id_t pdo_object_id,
                                                fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &super_hv_rdgen_context[0];

    /* Generate the requested I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s rdgen op: %d pdo obj: 0x%x",
               __FUNCTION__, rdgen_operation, pdo_object_id);
    status = fbe_api_rdgen_test_context_init(context_p,                     //context
                                             pdo_object_id,                 //pdo object
                                             FBE_CLASS_ID_INVALID,          //class ID not needed
                                             FBE_PACKAGE_ID_PHYSICAL,       //testing Physical and down
                                             rdgen_operation,               //rdgen operation
                                             FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                             0,                             //number of passes (not needed because of manual stop)
                                             SUPER_HV_IO_COUNT,             //number of IOs 
                                             0,                             //time before stop (not needed because of manual stop)
                                             SUPER_HV_THREAD_COUNT,         //thread count
                                             FBE_RDGEN_LBA_SPEC_RANDOM,     //seek pattern
                                             0x0,                           //starting LBA offset
                                             0x0,                           //min LBA is 0
                                             FBE_LBA_INVALID,               //stroke the entirety of the PDO
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,   // random I/O size (set `align' below)
                                             SUPER_HV_RDGEN_MIN_BLOCK_COUNT,  //min IO size
                                             SUPER_HV_RDGEN_MAX_BLOCK_COUNT); //max IO size, same as min for constancy
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

    /* Align the I/Os to the optimal request size.
     */ 
    status = fbe_api_rdgen_io_specification_set_alignment_size(&context_p->start_io.specification,
                                                               SUPER_HV_ALIGNED_REQUEST_SIZE); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If peer I/O is enabled load balance
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp load balance", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Use the best random lba.
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                        (FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM  |
                                                         FBE_RDGEN_OPTIONS_DISABLE_BREAKUP   ));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reset stats since we want to look at stats for just this test case.
     */
    status = fbe_api_rdgen_reset_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start the I/O
     */
    status = fbe_api_rdgen_start_tests(context_p,
                                       FBE_PACKAGE_ID_NEIT,
                                       1);  
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Wait for I/O to complete.
     */
    status = fbe_api_rdgen_wait_for_ios(context_p,
                                        FBE_PACKAGE_ID_NEIT,
                                        1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Validate no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, 1, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Free the resources
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't destroy rdgen context!");

    /* Return the status
     */
    return status;
}
/************************************ 
 * end super_hv_test_4k_aligned_io()
 ************************************/

/*!***************************************************************************
 *          super_hv_test_4k_rdgen_aligned_io()
 *****************************************************************************
 *
 * @brief   Run random lba and block size I/O.  The expectation is that rdgen
 *          will generate aligned writes.  This emulates the cache behavior
 *          where it can only send I/Os that are aligned to the 
 *          `physical_block_size'.
 *
 * @param   pdo_object_id - The object id to run I/O against
 * @param   rdgen_operation - rdgen operation to use for the random I/O
 *
 * @return  status
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t super_hv_test_4k_rdgen_aligned_io(fbe_object_id_t pdo_object_id,
                                                      fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &super_hv_rdgen_context[0];

    /* Generate the requested I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s rdgen op: %d pdo obj: 0x%x",
               __FUNCTION__, rdgen_operation, pdo_object_id);
    status = fbe_api_rdgen_test_context_init(context_p,                     //context
                                             pdo_object_id,                 //pdo object
                                             FBE_CLASS_ID_INVALID,          //class ID not needed
                                             FBE_PACKAGE_ID_PHYSICAL,       //testing Physical and down
                                             rdgen_operation,               //rdgen operation
                                             FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                             0,                             //number of passes (not needed because of manual stop)
                                             SUPER_HV_IO_COUNT,             //number of IOs 
                                             0,                             //time before stop (not needed because of manual stop)
                                             SUPER_HV_THREAD_COUNT,         //thread count
                                             FBE_RDGEN_LBA_SPEC_RANDOM,     //seek pattern
                                             0x0,                           //starting LBA offset
                                             0x0,                           //min LBA is 0
                                             FBE_LBA_INVALID,               //stroke the entirety of the PDO
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,   // random I/O size
                                             SUPER_HV_RDGEN_MIN_BLOCK_COUNT,  //min IO size
                                             SUPER_HV_RDGEN_MAX_BLOCK_COUNT); //max IO size, same as min for constancy
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

    /* Use the best random lba.
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                        (FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM  |
                                                         FBE_RDGEN_OPTIONS_DISABLE_BREAKUP   )); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If peer I/O is enabled load balance
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp load balance", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Reset stats since we want to look at stats for just this test case.
     */
    status = fbe_api_rdgen_reset_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start the I/O
     */
    status = fbe_api_rdgen_start_tests(context_p,
                                       FBE_PACKAGE_ID_NEIT,
                                       1);  
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Wait for I/O to complete.
     */
    status = fbe_api_rdgen_wait_for_ios(context_p,
                                        FBE_PACKAGE_ID_NEIT,
                                        1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Validate no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, 1, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Free the resources
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't destroy rdgen context!");

    /* Return the status
     */
    return status;
}
/*****************************************
 * end super_hv_test_4k_rdgen_aligned_io()
 *****************************************/

/*!***************************************************************************
 *          super_hv_test_4k_unaligned_io()
 *****************************************************************************
 *
 * @brief   Run random lba and block size I/O.  This test disables any 
 *          alignment that rdgne would provide (it simulates cache alignment).
 *          The expectation is that every I/O will fail with the expected
 *          block status: `Invalid Request/Unaligned Request'.
 *
 * @param   pdo_object_id - The object id to run I/O against
 * @param   rdgen_operation - rdgen operation to use for the random I/O
 *
 * @return  status
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t super_hv_test_4k_unaligned_io(fbe_object_id_t pdo_object_id,
                                                  fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &super_hv_rdgen_context[0];

    /* Sending unaligned request will result in error traces.  Change the limits.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                           0,    /* Num errors. */
                                           FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_TRACE_ERROR_LIMIT_ACTION_INVALID,
                                           0,    /* Num errors. */
                                           FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable backtrace
     */
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Generate the requested I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s rdgen op: %d pdo obj: 0x%x",
               __FUNCTION__, rdgen_operation, pdo_object_id);
    status = fbe_api_rdgen_test_context_init(context_p,                     //context
                                             pdo_object_id,                 //pdo object
                                             FBE_CLASS_ID_INVALID,          //class ID not needed
                                             FBE_PACKAGE_ID_PHYSICAL,       //testing Physical and down
                                             rdgen_operation,               //rdgen operation
                                             FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                             SUPER_HV_IO_COUNT,             //number of passes (needed since I/O count will not be incremented)
                                             0,                             //number of IOs 
                                             0,                             //time before stop (not needed because of manual stop)
                                             SUPER_HV_THREAD_COUNT,         //thread count
                                             FBE_RDGEN_LBA_SPEC_RANDOM,     //seek pattern
                                             0x0,                           //starting LBA offset
                                             0x0,                           //min LBA is 0
                                             FBE_LBA_INVALID,               //stroke the entirety of the PDO
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,   // random I/O size
                                             SUPER_HV_RDGEN_MIN_BLOCK_COUNT,  //min IO size
                                             SUPER_HV_RDGEN_MAX_BLOCK_COUNT); //max IO size, same as min for constancy
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

    /* Tell rdgen to no align the I/Os to the physical (optimal) block size.
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                        (FBE_RDGEN_OPTIONS_DO_NOT_ALIGN_TO_OPTIMAL | 
                                                         FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM          |
                                                         FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR       |
                                                         FBE_RDGEN_OPTIONS_DISABLE_BREAKUP           ));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reset stats since we want to look at stats for just this test case.
     */
    status = fbe_api_rdgen_reset_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start the I/O
     */
    status = fbe_api_rdgen_start_tests(context_p,
                                       FBE_PACKAGE_ID_NEIT,
                                       1);  
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Wait for I/O to complete.
     */
    status = fbe_api_rdgen_wait_for_ios(context_p,
                                        FBE_PACKAGE_ID_NEIT,
                                        1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Validate errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, 1, FBE_STATUS_OK, /* Continue on error was set */ 
                                                   context_p->start_io.statistics.error_count, /* Includes retries */
                                                   FBE_FALSE,   /* Don't check I/O count (since it will not match) */ 
                                                   FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_REQUEST);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note Validate the invalid and error counts.
     *
     *        Assumption for pure random LBA / block count is that best case 
     *        1 in 8 will be aligned.  Therefore checking for 1 in 4 being 
     *        unaligned is valid.
     */
    MUT_ASSERT_TRUE(context_p->start_io.statistics.invalid_request_err_count >= (SUPER_HV_IO_COUNT / 4));
    MUT_ASSERT_TRUE(context_p->start_io.statistics.error_count >= context_p->start_io.statistics.invalid_request_err_count);

    /* Free the resources
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't destroy rdgen context!");

    /* Put the error limits back on.
     */
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Put the trace level to stop on error.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Enable backtrace
     */
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Return the status
     */
    return status;
}
/*****************************************
 * end super_hv_test_4k_unaligned_io()
 *****************************************/

/*!**************************************************************
 * super_hv_write_background_pattern()
 ****************************************************************
 * @brief
 *  Write background pattern to a PVD.
 *
 * @param pdo_object_id - PVD object id.               
 *
 * @return None.
 *
 * @author
 *  04/10/2014 - Created. Lili Chen
 *
 ****************************************************************/
void super_hv_write_background_pattern(fbe_object_id_t pdo_object_id)
{
    fbe_api_rdgen_context_t    *context_p = &super_hv_rdgen_context[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                         pdo_object_id,
                                         FBE_CLASS_ID_INVALID, 
                                         FBE_PACKAGE_ID_PHYSICAL,
                                         FBE_RDGEN_OPERATION_WRITE_ONLY,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         1,    /* We do one full sequential pass. */
                                         0,    /* num ios not used */
                                         0,    /* time not used*/
                                         1,    /* threads */
                                         FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                         0,    /* Start lba*/
                                         0,    /* Min lba */
                                         FBE_LBA_INVALID,    /* max lba */
                                         FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                         0x100,    /* Number of stripes to write. */
                                         0x100    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);

    return;
}
/******************************************
 * end super_hv_write_background_pattern()
 ******************************************/

/*!***************************************************************************
 * super_hv_test_4k_unaligned_read()
 *****************************************************************************
 * @brief 
 *  Run with unaligned reads to test that the PDO does the correct alignment.
 *
 * @param   pdo_object_id - The object id to run I/O against
 * @param   rdgen_operation - rdgen operation to use for the random I/O
 *
 * @return  status
 *
 * @author
 *  3/25/2014 - Created from unaligned io test. Rob Foley 
 *  
 *****************************************************************************/
static fbe_status_t super_hv_test_4k_unaligned_read(fbe_object_id_t pdo_object_id,
                                                    fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &super_hv_rdgen_context[0];
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t max_lba = 16;
    fbe_rdgen_options_t options = (FBE_RDGEN_OPTIONS_DO_NOT_ALIGN_TO_OPTIMAL | 
                                   FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM          |
                                   FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR       |
                                   FBE_RDGEN_OPTIONS_DISABLE_BREAKUP           );
    mut_printf(MUT_LOG_TEST_STATUS, "== start %s ", __FUNCTION__);

    super_hv_write_background_pattern(pdo_object_id);

    /* First run all combinations from 0 to 16.  This gives us all combinations of pre/post sizes.
     */
    for (lba = 0; lba < max_lba; lba++) {

        for (blocks = 1; blocks < max_lba; blocks++) {
            mut_printf(MUT_LOG_TEST_STATUS, "io test for lba: 0x%llx blocks: 0x%llx", lba, blocks);
            status = fbe_api_rdgen_send_one_io(context_p, pdo_object_id, FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_PHYSICAL,
                                               FBE_RDGEN_OPERATION_READ_ONLY,
                                               FBE_RDGEN_PATTERN_LBA_PASS,
                                               lba, blocks,
                                               options,
                                               0,0, /* expiration/abort msecs. */
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
            status = fbe_api_rdgen_context_check_io_status(context_p, 1, FBE_STATUS_OK, 0,    /* err count*/
                                                           FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                           FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        }
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Generate the requested I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s rdgen op: %d pdo obj: 0x%x",
               __FUNCTION__, rdgen_operation, pdo_object_id);
    status = fbe_api_rdgen_test_context_init(context_p,                     //context
                                             pdo_object_id,                 //pdo object
                                             FBE_CLASS_ID_INVALID,          //class ID not needed
                                             FBE_PACKAGE_ID_PHYSICAL,       //testing Physical and down
                                             rdgen_operation,               //rdgen operation
                                             FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                             SUPER_HV_IO_COUNT,             //number of passes (needed since I/O count will not be incremented)
                                             0,                             //number of IOs 
                                             0,                             //time before stop (not needed because of manual stop)
                                             SUPER_HV_THREAD_COUNT,         //thread count
                                             FBE_RDGEN_LBA_SPEC_RANDOM,     //seek pattern
                                             0x0,                           //starting LBA offset
                                             0x0,                           //min LBA is 0
                                             FBE_LBA_INVALID,               //stroke the entirety of the PDO
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,   // random I/O size
                                             SUPER_HV_RDGEN_MIN_BLOCK_COUNT,  //min IO size
                                             // We use 2 * 7, since 7 is the most pre-read at beginning or end.
                                             0x800 - (2* 7)); //max IO size limit so we do not exceed 0x800 in any case.
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

    /* Tell rdgen to no align the I/Os to the physical (optimal) block size.
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reset stats since we want to look at stats for just this test case.
     */
    status = fbe_api_rdgen_reset_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start the I/O
     */
    status = fbe_api_rdgen_start_tests(context_p,
                                       FBE_PACKAGE_ID_NEIT,
                                       1);  
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Wait for I/O to complete.
     */
    status = fbe_api_rdgen_wait_for_ios(context_p,
                                        FBE_PACKAGE_ID_NEIT,
                                        1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

    /* Validate no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, 1, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Free the resources
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't destroy rdgen context!");

    /* Return the status
     */
    return status;
}
/*****************************************
 * end super_hv_test_4k_unaligned_read()
 *****************************************/

/*!***************************************************************************
 *          super_hv_test_pdo()
 *****************************************************************************
 *
 * @brief   This is the main test function of the PDO 4K block size support
 *          test.  It performs the following actions:
 *              o Creates a 4K simulated drive
 *              o Validates the PDO sees the drive and wait for it to become
 *                Ready.
 *              o Issues aligned (to 8 block boundry) 520-bps WRITE I/Os
 *              o Issues aligned (to 8 block boundry) 520-bps READ I/Os
 *              o Validates the data is correct
 *
 * @param   bus - bus that pdo is located at
 * @param   enclosure - enclosure pdo is located at
 * @param   slot - slot drive is located at
 *
 * @return - None.
 *
 * @author
 *  11/19/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void super_hv_test_pdo(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot)
{
    fbe_status_t                        status;
    fbe_object_id_t                     board_object_id;
    fbe_board_mgmt_platform_info_t      platform_info;
    fbe_u32_t                           pdo_count;
    fbe_u32_t                           total_cores;
    fbe_object_id_t                    *pdo_list;
    fbe_object_id_t                     pdo_object_id;
    fbe_object_id_t                     peer_pdo_object_id;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_api_terminator_device_handle_t  peer_drive_handle;
    fbe_sim_transport_connection_target_t this_sp;
    fbe_sim_transport_connection_target_t other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    /* Currently not used*/
    FBE_UNREFERENCED_PARAMETER(slot);

    //get core count
    mut_printf(MUT_LOG_TEST_STATUS, "Getting board object ID.");
    status = fbe_api_get_board_object_id(&board_object_id);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get board object ID!");

    mut_printf(MUT_LOG_TEST_STATUS,"Getting platform information.");
    status = fbe_api_board_get_platform_info(board_object_id, &platform_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get platform info!");

    if (FBE_IS_TRUE(platform_info.is_simulation))
    {
        total_cores = 6;
    }
    else
    {
        total_cores = (fbe_u32_t) platform_info.hw_platform_info.processorCount;
    }

    /*! @note Assume drives are located at 0_0_slot.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Getting PDO object list");
    status = fbe_api_enumerate_class(FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,FBE_PACKAGE_ID_PHYSICAL,&pdo_list,&pdo_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PDO object list!");
    mut_printf(MUT_LOG_TEST_STATUS, "PDO list head: 0x%X, pdo_count = %d", *pdo_list, pdo_count);

    /* Set the terminator drive debug */
    /* Test 1: Insert a 4k drive.
     */
    status = super_hv_test_insert_4k_drive(bus, enclosure, pdo_count,
                                           &drive_handle,
                                           &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        /* Insert the drive on the other SP.
         */
        fbe_api_sim_transport_set_target_server(other_sp);
        status = super_hv_test_insert_4k_drive(bus, enclosure, pdo_count,
                                               &peer_drive_handle,
                                               &peer_pdo_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(pdo_object_id, peer_pdo_object_id);
        fbe_api_sim_transport_set_target_server(this_sp);
    }

    /* Test 2: Run aligned W/R/C test.
     */
    status = super_hv_test_4k_aligned_io(pdo_object_id, 
                                         FBE_RDGEN_OPERATION_WRITE_READ_CHECK);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 3: Run rdgen aligned W/R/C test.
     */
    status = super_hv_test_4k_rdgen_aligned_io(pdo_object_id, 
                                               FBE_RDGEN_OPERATION_WRITE_READ_CHECK);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 4: Run unaligned write test.
     */
    status = super_hv_test_4k_unaligned_io(pdo_object_id, 
                                           FBE_RDGEN_OPERATION_WRITE_ONLY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 5: Run unaligned read test.
     */
    status = super_hv_test_4k_unaligned_read(pdo_object_id, 
                                             FBE_RDGEN_OPERATION_READ_ONLY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Success
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s - I/O testing of 4K drive: %d_%d_%d (pdo obj: 0x%x) completed successfully.",
               __FUNCTION__, 0, 0, pdo_count, pdo_object_id);

    return;
}
/**************************************
 * end super_hv_test_pdo()
 **************************************/

/*!***************************************************************************
 *          super_hv_test()
 *****************************************************************************
 *
 * @brief   Run the 4K PDO test in a single drive.
 *
 * @param   None
 *
 * @return  None.
 *
 * @author
 *  11/19/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
void super_hv_test(void)
{
    /* Run the test */
    super_hv_test_pdo(0, 0, 0);

    return;
}
/**************************************
 * end super_hv_test()
 **************************************/

/*!***************************************************************************
 *          super_hv_setup()
 *****************************************************************************
 *
 * @brief   This function created the terminator and simple (520-bps drives)
 *          physical package configuration.
 *
 * @param  - None.
 *
 * @return - None
 *
 * @author
 *  11/19/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
void super_hv_setup(void)
{
    /*! @note All physical block sizes must be a multiple of logical block 
     *        size (Block Size Virtualization is not longer supported).
     */
    FBE_ASSERT_AT_COMPILE_TIME((SUPER_HV_PHYSICAL_BLOCK_SIZE % FBE_BE_BYTES_PER_BLOCK) == 0);

    /* load pp and neit */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    physical_package_test_load_physical_package_and_neit();

    fbe_thread_delay(3000);

    /* Enable 4K drives */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, FBE_TRUE);

    return;
}
/******************************************
 * end super_hv_setup()
 ******************************************/

/*!**************************************************************
 *          super_hv_cleanup()
 ****************************************************************
 * @brief
 *  Common unload for Physical Package test suite.  
 *
 * @param none.
 *
 * @return none
 *
 * HISTORY:
 *  8/31/2009 - Created. VG
 *
 ****************************************************************/
void super_hv_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    physical_package_test_unload_physical_package_and_neit();

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}
/************************** 
 * end super_hv_cleanup()
 *************************/

/*!**************************************************************
 *          super_hv_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a 4K PDO on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void super_hv_dualsp_test(void)
{
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run the test */
    super_hv_test_pdo(0, 0, 0);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end super_hv_dualsp_test()
 ******************************************/

/*!**************************************************************
 * super_hv_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for dualsp PDO 4K block size test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void super_hv_dualsp_setup(void)
{
    /* Instantiate the drives on both SPs
     */
    physical_package_test_load_physical_package_and_neit_both_sps();

    fbe_thread_delay(3000);

    /* Enable 4K drives */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_4K_ENABLED, FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end super_hv_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * super_hv_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup PDO 4K block size test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/22/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void super_hv_dualsp_cleanup(void)
{
    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    physical_package_test_unload_physical_package_and_neit_both_sps();

    return;
}
/******************************************
 * end super_hv_dualsp_cleanup()
 ******************************************/


/****************************
 * end if file super_hv.c
 ***************************/
