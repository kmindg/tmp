/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  io related functionality.
 * 
 * @ingroup logical_drive_class_tests
 * 
 * @version
 *   11/21/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_trace.h"

#include "fbe_service_manager.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_base_object.h"
#include "fbe_topology.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_interface.h"
/* #include "fbe_topology_discovery_lib.h" */
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"


/*!*******************************************************************
 * @def FBE_LDO_LIFECYCLE_TIME_MSEC
 *********************************************************************
 * @brief Time to wait for lifecycle transitions.
 *        The time is arbitrary and just ensures that we eventually
 *        make it to this state.
 *
 *********************************************************************/
#define FBE_LDO_LIFECYCLE_TIME_MSEC 60000 /* timeout = 60 sec */

/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/

/* fbe_ldo_test_read_write_io_case_array
 *
 * This structure defines all the io test cases.
 * An IO test case has
 *  *) Block size configuration information
 *  *) A range of lbas to test
 *  *) A range of I/O sizes to test
 *
 * The combination of all the above allows us to be flexible when
 * we specify an I/O test to be run.
 *
 */
fbe_api_rdgen_test_io_case_t fbe_ldo_test_read_write_io_case_array[] =
{
    {520,       520,        0,    20,   1,       10,       1,       1,        1},

    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    /* Normal 512/520 cases.  This case covers a complete optimal block.
     * We no longer support 512 byte/sector block sizes
     */

#ifdef  BLOCKSIZE_512_SUPPORTED 
    {520,       512,       0,    67,   1,       67,       1,       64,       65},

    /* Large block counts for 512. These cases cover two optimal blocks.
     * We no longer support 512 byte/sector block sizes
     */
    {520,       512,       0,    129,  100,     129,      1,       64,       65},
    {520,       512,       0,    129,  250,     258,      1,       64,       65},
    {520,       512,       0,    129,  500,     520,      1,       64,       65},
    {520,       512,       0,    129, 1000,    1030,      1,       64,       65},
#endif
    
    /* For 4k, do the entire optimal block for block counts of 1..67. This
     * allows us to cover the entire optimal block and optimal block boundary 
     * conditions. 
     */
    {520,       4096,      0,    530,  1,       67,       1,       512,      65},
    /* Next, cover some large block counts.
     */
    {520,       4096,      0,    67,   127,     129,      1,       512,      65},
    {520,       4096,      0,    67,   255,     257,      1,       512,      65},
    {520,       4096,      0,    67,  511,     513,      1,       512,      65},
    {520,       4096,      0,    67,  1023,   1025,      1,       512,      65},
    /* Large block counts with optimal block boundary.
     */
    {520,       4096,      500,  520,   127,     129,      1,       512,      65},
    {520,       4096,      500,  520,   255,     257,      1,       512,      65},
    {520,       4096,      500,  520,  511,     513,      1,       512,      65},
    {520,       4096,      500,  520,  1023,   1025,      1,       512,      65},

    {520,       4160,      0,    24,   1,       129,      1,       8,        1},
    {520,       520,       0,    20,   1,       10,       1,       1,        1},

    /* Lossy cases.
     * We no longer support 512 byte/sector block sizes
     */
    {520,       4096,      0,    15,  1,       15,     1,       7,        1},
    {520,       4096,      0,    31,  1,       31,     1,       15,       2},
    
#ifdef  BLOCKSIZE_512_SUPPORTED    
    {520,       512,       0,    16,   1,       0x10,     1,       2,        3},
    {520,       512,       0,    16,   1,       0x10,     1,       4,        5},
    {520,       512,       0,    16,   1,       0x10,     1,       6,        7},
#endif    

    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0,         0,    0,    0,       0, 0},

};
/* end fbe_ldo_test_read_write_io_case_array global structure */

/* fbe_ldo_test_write_same_io_case_array
 *
 * This structure defines all the io test cases.
 * An IO test case has
 *  *) Block size configuration information
 *  *) A range of lbas to test
 *  *) A range of I/O sizes to test
 *
 * The combination of all the above allows us to be flexible when
 * we specify an I/O test to be run.
 *
 */
fbe_api_rdgen_test_io_case_t fbe_ldo_test_write_same_io_case_array[] =
{
    /* 
     * exported imported start  end     start    end     increment exp opt   imp opt
     *  bl size block    lba    lba     blocks   blocks  blocks    bl sizes  bl size
     */
    {520,     520,       0,      10,         1,      10,         1,      1,       1},
    {520,     520,       0,      1024,       8,      1024,     128,      1,       1},
    
   /* We no longer support 512 byte/sector block sizes */
#ifdef BLOCKSIZE_512_SUPPORTED    
    {520,     512,       0,      (64 * 34),  64,     2048,     64,     64,      65},
#endif    
    
    {520,     4096,      0,      (512 * 18), 512,    2048,    512,    512,      65},
    {520,     4160,      0,      (8 * 21),   8,      2048,      8,      8,       1},
    
    /* We no longer support 512 byte/sector block sizes */
#ifdef BLOCKSIZE_512_SUPPORTED     
    /* Lossy cases.
     */
    {520,     512,       0,     20,     2,        10,     2,         2,       3},
    {520,     512,       0,     20,     4,        16,     4,         4,       5},
    {520,     512,       0,     18,     6,        36,     6,         6,       7},
#endif    
    
    {520,     4096,      0,     21,     7,        21,     7,         7,       1},
    {520,     4096,      0,     45,    15,        45,    15,        15,       2},
    
    /* Insert new records here.
     */

    /* This is the terminator record, it always should be zero.
     */
    {FBE_LDO_TEST_INVALID_FIELD,         0,         0,    0,    0,       0},

};
/* end fbe_ldo_test_write_same_io_case_array global structure */

/*!**************************************************************
 * @fn fbe_ldo_test_io_run_io_cases(fbe_api_rdgen_test_io_case_t *const cases_p,
 *                                  fbe_ldo_test_io_case_fn test_fn)
 ****************************************************************
 * @brief
 *  This function performs an io test by first finding the
 *  appropriate device that matches the block size of this I/O test,
 *  and then executing the I/O test.
 *
 * @param cases_p - The test cases including the I/O range
 *                  and block sizes to use in this test.
 * @param test_fn - The test function that should be called
 *                  to execute the test case.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  08/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_io_run_io_cases(fbe_api_rdgen_test_io_case_t *const cases_p,
                                          fbe_ldo_test_io_case_fn test_fn)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    /* Loop over all of our test cases.
     */
    while (cases_p[index].exported_block_size != FBE_TEST_IO_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        /* First get the appropriate block size for the test case.
         */
        fbe_block_size_t block_size = cases_p[index].imported_block_size;
        fbe_object_id_t object_id = fbe_ldo_test_get_object_id_for_drive_block_size(block_size);
        MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);

        /* Execute this test function for this single test case. 
         * We do this one test case at a time since different 
         * test cases might need to use different block sizes and hence 
         * different drives. 
         */
        test_fn(&cases_p[index], object_id, 1);

        /* Make sure there were no errors.
         */
        MUT_ASSERT_INT_EQUAL(fbe_ldo_get_drive_error_count(block_size), 0);
        index++;
    } /* End while cases_p[index].exported_block_size != 0 */

    return status;
}
/* end fbe_ldo_test_io_run_io_cases() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_io(void)
 ****************************************************************
 * @brief
 *  This function performs a read io test.
 *
 * @param - None.
 *
 * @return fbe_status_t
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_io(void)
{
    /* Simply launch the test against our object id.
     */
    return fbe_ldo_test_io_run_io_cases(fbe_ldo_test_read_write_io_case_array,
                                        fbe_api_rdgen_test_io_run_read_only_test);
}
/* end fbe_ldo_test_read_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_single_write_io(void)
 ****************************************************************
 * @brief
 *  This function performs a write of a single I/O.  We then
 *  read it back and check the status.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  11/18/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_single_write_io(void)
{
    fbe_payload_block_operation_status_t write_status;
    fbe_payload_block_operation_status_t read_status;
    fbe_payload_block_operation_qualifier_t write_qualifier;
    fbe_payload_block_operation_qualifier_t read_qualifier;
    fbe_status_t status;

    /* Use a 520 byte block device for this test.
     */
    fbe_object_id_t object_id = fbe_ldo_test_get_object_id_for_drive_block_size(520);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);

    /* Simply issue an I/O to lba 0 for 1 block. 
     */
    status = fbe_test_io_write_read_check( object_id,
                                           0, /* lba */
                                           1, /* blocks */
                                           520, /* exported block size */
                                           1, /* optimal block size*/
                                           520, /* Imported optimal block size */
                                           &write_status,
                                           &write_qualifier,
                                           &read_status,
                                           &read_qualifier );
    MUT_ASSERT_INT_EQUAL(write_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(write_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    MUT_ASSERT_INT_EQUAL(read_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(read_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_single_write_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_io(void)
 ****************************************************************
 * @brief
 *  This function performs a write io test.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_io(void)
{
    /* Simply launch the test against our object id.
     */
    return fbe_ldo_test_io_run_io_cases(fbe_ldo_test_read_write_io_case_array,
                                        fbe_api_rdgen_test_io_run_write_read_check_test);
}
/* end fbe_ldo_test_write_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_verify_io(void)
 ****************************************************************
 * @brief
 *  This function performs a write verify io test.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_verify_io(void)
{
    /* Simply launch the test against our object id.
     */
    return fbe_ldo_test_io_run_io_cases(fbe_ldo_test_read_write_io_case_array,
                                        fbe_api_rdgen_test_io_run_write_verify_read_check_test);
}
/* end fbe_ldo_test_write_verify_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_only_io(void)
 ****************************************************************
 * @brief
 *  This function performs a write io test.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_only_io(void)
{
    /* Simply launch the test against our object id.
     */
    return fbe_ldo_test_io_run_io_cases(fbe_ldo_test_read_write_io_case_array,
                                        fbe_api_rdgen_test_io_run_write_only_test);
}
/* end fbe_ldo_test_write_only_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_io(void)
 ****************************************************************
 * @brief
 *  This function performs a write same io test.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_same_io(void)
{
    /* Simply launch the test against our object id.
     */
    return fbe_ldo_test_io_run_io_cases(fbe_ldo_test_write_same_io_case_array,
                                        fbe_api_rdgen_test_io_run_write_same_test);
}
/* end fbe_ldo_test_write_same_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_verify_io(void)
 ****************************************************************
 * @brief
 *  This function performs a verify io test.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_verify_io(void)
{
    /* Simply launch the test against our object id.
     */
    return fbe_ldo_test_io_run_io_cases(fbe_ldo_test_read_write_io_case_array,
                                        fbe_api_rdgen_test_io_run_verify_test);
}
/* end fbe_ldo_test_verify_io() */

/*!***************************************************************
 * @fn fbe_ldo_test_pass_through_io(void)
 ****************************************************************
 * @brief
 *  This function performs pass through i/o.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  06/18/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_pass_through_io(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;
    fbe_sg_element_t *sg_p;
    fbe_api_block_status_values_t block_status;
    
    /* We no longer support 512 byte/sector block sizes */
#ifdef BLOCKSIZE_512_SUPPORTED
    sg_p = fbe_test_io_alloc_memory_with_sg(512);    
#else    
    sg_p = fbe_test_io_alloc_memory_with_sg(520);
#endif    
    
    if (sg_p == NULL)
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }
    /* Simply do a read to a 512 byte block device where the requested block 
     * size is also 512.  This is a pass through since the imported and 
     * exported sizes are the same.
     * We no longer support 512 byte/sector block sizes - changing to 520 b/s
     */
    
#ifdef BLOCKSIZE_512_SUPPORTED
    status = fbe_api_logical_drive_interface_send_block_payload(fbe_ldo_test_get_object_id_for_drive_block_size(520),
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                            0x100,    /* lba */
                                            0x1,    /* blocks */
                                            512,    /* block size */
                                            1,    /* opt block size */
                                            sg_p,    /* Sg element */
                                            NULL,    /* edge descriptor */
                                            &qualifier,
                                            &block_status);    
#else    
    status = fbe_api_logical_drive_interface_send_block_payload(fbe_ldo_test_get_object_id_for_drive_block_size(520),
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                            0x100,    /* lba */
                                            0x1,    /* blocks */
                                            520,    /* block size */
                                            1,    /* opt block size */
                                            sg_p,    /* Sg element */
                                            NULL,    /* edge descriptor */
                                            &qualifier,
                                            &block_status);
#endif    
    
    /* Make sure the execute cdb worked.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(block_status.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_status.qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    fbe_test_io_free_memory_with_sg(&sg_p);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_pass_through_io() */

/*!**************************************************************
 * @fn fbe_ldo_test_send_block_size( 
 *   fbe_payload_block_operation_opcode_t opcode,    
 *   fbe_block_size_t block_size,
 *   fbe_block_size_t requested_block_size,
 *   fbe_block_size_t requested_opt_bl_size,
 *   fbe_status_t expected_status,
 *   fbe_payload_block_operation_status_t expected_block_status,
 *   fbe_payload_block_operation_qualifier_t expected_block_qualifier)
 ****************************************************************
 * @brief
 *   Test sending a specific block size and optimal block size.
 *   We use this for easily testing invalid block size cases.
 * 
 *   We intentionally use the same lba, 0x100 and blocks 0x1.
 *   We do not believe that varying the lba, block count matters for
 *   this test, and thus we do not allow the client to specify it.
 *  
 * @param opcode - Operation to send.
 * @param block_size  - Size in bytes of physical block size.
 * @param requested_block_size - Size requested in bytes
 * @param requested_opt_bl_size - Number of blocks in optimal block.
 * @param expected_status - fbe_status_t status we exptect.
 * @param expected_block_status - Block status expected.
 * @param expected_block_qualifier - Block qualifier expected.
 *
 * @return - None.  
 *
 * HISTORY:
 *  12/11/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_send_block_size( 
    fbe_payload_block_operation_opcode_t opcode,    
    fbe_block_size_t block_size,
    fbe_block_size_t requested_block_size,
    fbe_block_size_t requested_opt_bl_size,
    fbe_status_t expected_status,
    fbe_payload_block_operation_status_t expected_block_status,
    fbe_payload_block_operation_qualifier_t expected_block_qualifier)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;
    fbe_sg_element_t *sg_p;
    fbe_api_block_status_values_t block_status;
    
    sg_p = fbe_test_io_alloc_memory_with_sg(requested_block_size);
    
    MUT_ASSERT_NOT_NULL(sg_p);

    /* Simply do a read where the block size is set to zero.
     */
    status = fbe_api_logical_drive_interface_send_block_payload(
        fbe_ldo_test_get_object_id_for_drive_block_size(block_size),
        opcode,
        0x100,    /* lba */
        0x1,    /* blocks */
        requested_block_size,    /* block size */
        requested_opt_bl_size,    /* opt block size */
        sg_p,    /* Sg element */
        NULL,    /* edge descriptor */
        &qualifier,
        &block_status);
    /* Make sure the execute cdb worked.
     */
    MUT_ASSERT_INT_EQUAL(status, expected_status);
    MUT_ASSERT_INT_EQUAL(block_status.status, expected_block_status);
    MUT_ASSERT_INT_EQUAL(block_status.qualifier, expected_block_qualifier);

    fbe_test_io_free_memory_with_sg(&sg_p);

    return;
}
/**************************************
 * end fbe_ldo_test_send_block_size()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_zero_block_sizes(void)
 ****************************************************************
 * @brief
 *  This function tests various invalid block size cases.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  12/11/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_zero_block_sizes(void)
{

    /* We choose a single opcode to test for each of the cases. 
     * We intentionally choose different opcodes for some of these tests. 
     * For each test, one of the parameters is intentionally zero. 
     * We also intentionally vary the physical block size. 
     */

    /* 520 case: Block size is 0.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 520, /* physical block size */
                                 0, /* requested block size */
                                 1, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
    /* 512 case: Block size is 0.
     * We no longer support 512 byte/sector block sizes
     */
    
#ifdef BLOCKSIZE_512_SUPPORTED    
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                 512, /* physical block size */
                                 0, /* requested block size */
                                 1, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
#endif    
    
    /* 4096 case: Block size is 0.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY,
                                 4096, /* physical block size */
                                 0, /* requested block size */
                                 1, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
    /* 4160 case: Block size is 0.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                 4160, /* physical block size */
                                 0, /* requested block size */
                                 1, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* 520 case: Requested optimal block size is 0.
     * We intentionally make the requested size different from the 
     * physical size, since if they are thte same, the request just gets passed through. 
     * For most of the cases we just use a requested size of 520, but for this case we 
     * need to use something different than 520, so we arbitrarily chose 512. 
     * We no longer support 512 byte/sector block sizes
     */

#ifdef BLOCKSIZE_512_SUPPORTED    
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 520, /* physical block size */
                                 512, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
    /* 512 case: Requested optimal block size is 0.
     * We intentionally make the requested size different from the 
     * physical size, since if they are thte same, the request just gets passed through.
     * We no longer support 512 byte/sector block sizes
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                 512, /* physical block size */
                                 520, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
#endif    
    
    /* 4096 case: Requested optimal block size is 0.
     * We intentionally make the requested size different from the 
     * physical size, since if they are thte same, the request just gets passed through. 
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY,
                                 4096, /* physical block size */
                                 520, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
    /* 4160 case: Requested optimal block size is 0.
     * We intentionally make the requested size different from the 
     * physical size, since if they are thte same, the request just gets passed through. 
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                 4160, /* physical block size */
                                 520, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* 520 case: Requested block size and requested optimal block size are 0.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 520, /* physical block size */
                                 0, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
    /* 512 case: Requested block size and requested optimal block size are 0.
     * We no longer support 512 byte/sector block sizes
     */
    
#ifdef BLOCKSIZE_512_SUPPORTED    
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                 512, /* physical block size */
                                 0, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
#endif    
    
    /* 4096 case: Requested block size and requested optimal block size are 0.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY,
                                 4096, /* physical block size */
                                 0, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
    /* 4160 case: Requested block size and requested optimal block size are 0.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                 4160, /* physical block size */
                                 0, /* requested block size */
                                 0, /* Optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* Any of the above calls will assert if they fail.
     */
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_zero_block_sizes() */

/*!***************************************************************
 * @fn fbe_ldo_test_block_size_too_large(void)
 ****************************************************************
 * @brief
 *  This function tests various invalid block size cases.
 *  We test cases where the block size requested
 *  (combination of requested block size and optimal block size)
 *  is too small.
 *
 * @param - None.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @version
 *  12/11/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_block_size_too_large(void)
{
    /* 520 Case:  Requested block size is less than the 
     *            physical block size and the requested optimal block size is 1. 
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 520, /* physical block size */
                                 512, /* requested block size */
                                 /* requested optimal block size */
                                 (FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE / 512) + 1,
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* 512 Case:  Requested block size is less than the 
     *            physical block size and the requested optimal block size is 1. 
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 512, /* physical block size */
                                 520, /* requested block size */
                                 /* requested optimal block size */
                                 (FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE / 520) + 1,
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* 4096 Case:  Test case where we have a slightly bigger optimal block size than the
     * ldo allows for this block size.
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 4096, /* physical block size */
                                 520, /* requested block size */
                                 /* requested optimal block size */
                                 (FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE / 520) + 1, 
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* 4160 Case:  Requested block size is less than the 
     *             physical block size and the requested optimal block size is 1. 
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 4160, /* physical block size */
                                 520, /* requested block size */
                                 /* requested optimal block size */
                                 (FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE / 520) + 1, 
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* Case where the requested block size is more than our max optimal size and our
     * max optimal size is 1. 
     */
    fbe_ldo_test_send_block_size(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                 4160, /* physical block size */
                                 FBE_LDO_MAX_OPTIMUM_BLOCK_SIZE + 1, /* requested block size */
                                 1,/* requested optimal block size */
                                 FBE_STATUS_OK, /* expected status */
                                 /* expected block status */
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 /* expected block qualifier */
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

    /* Any of the above calls will assert if they fail.
     */
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_block_size_too_large() */

/*!***************************************************************
 * @fn fbe_ldo_test_beyond_capacity_error(void)
 ****************************************************************
 * @brief
 *  This function performs an I/O beyond the capacity and expects an error.
 *
 * @param  - None.
 *
 * @return fbe_status_t - the status of the test.
 *
 * @version
 *  06/18/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_beyond_capacity_error(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;
    fbe_sg_element_t *sg_p;
    fbe_api_block_status_values_t block_status;

    /* First get some memory to run I/O with.
     */
    sg_p = fbe_test_io_alloc_memory_with_sg(520);
    
    if (sg_p == NULL)
    {
        /* If we cannot allocate memory, just return an error.
         */
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    /* Issue the block command to the 520 byte block device.
     * Note that we set the lba intentionally too large. 
     */
    status = fbe_api_logical_drive_interface_send_block_payload(fbe_ldo_test_get_object_id_for_drive_block_size(520),
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                            0x10b60000,    /* lba */
                                            0x1,   /* blocks */
                                            520,   /* block size */
                                            1,     /* optimum block size */
                                            sg_p,
                                            NULL, /* pre read descriptor not used on read */
                                            &qualifier,
                                            &block_status);
    /* The transport status is good, since the packet was delivered OK. 
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* We expect the block operation to fail with a specific opcode and
     * qualifier. 
     */
    MUT_ASSERT_INT_EQUAL(block_status.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(block_status.qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED);

    fbe_test_io_free_memory_with_sg(&sg_p);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_beyond_capacity_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_empty_sg_error(void)
 ****************************************************************
 * @brief
 *  This function performs an I/O that has a null sg list
 *  and expects an error.
 *
 * @param  - None.
 *
 * @return fbe_status_t - the status of the test.
 *
 * @version
 *  11/17/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_empty_sg_error(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;
    fbe_api_block_status_values_t block_status;

    /* Issue the block command to the 520 byte block device.
     * Note that we set the sg to be empty to cause an error.
     */
    status = fbe_api_logical_drive_interface_send_block_payload(
        fbe_ldo_test_get_object_id_for_drive_block_size(520),
        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
        0x0,    /* lba */
        0x1,    /* blocks */
        520,    /* block size */
        1,    /* optimum block size */
        NULL,    /* empty sg to cause an error. */
        NULL,    /* pre read descriptor not used on read */
        &qualifier,
        &block_status);
    /* The transport status is good, since the packet was delivered OK. 
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We expect the block operation to fail with a specific opcode and
     * qualifier. 
     */
    MUT_ASSERT_INT_EQUAL(block_status.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(block_status.qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    /* We expect there to have been a single error.
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_get_drive_error_count(520), 1);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_empty_sg_error() */

/*!**************************************************************
 * @fn fbe_ldo_test_get_first_state()
 ****************************************************************
 * @brief
 *  Test the code that determines the first state of the io cmd.
 *
 * @param  - None.               
 *
 * @return fbe_status_t   
 *
 * @version
 *  8/28/2008 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t fbe_ldo_test_get_first_state(void)
{
    fbe_status_t status;
    fbe_ldo_io_cmd_state_t state;
    fbe_payload_block_operation_t block_operation;

    /* When we call get first state, we expect this to return an error (invalid 
     * opcode state), since identify is not handled. 
     */
    status = fbe_payload_block_build_operation(&block_operation,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY,
                                               0,0,0,0,NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    state = fbe_ldo_io_cmd_get_first_state(&block_operation);
    MUT_ASSERT_TRUE(state == fbe_ldo_io_cmd_invalid_opcode);

    /* First fill out an I/O block with an invalid block command.
     * When we call get first state, we expect this to return an error (invalid 
     * opcode state). 
     */
    status = fbe_payload_block_build_operation(&block_operation,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,
                                               0,0,0,0,NULL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    state = fbe_ldo_io_cmd_get_first_state(&block_operation);
    MUT_ASSERT_TRUE(state == fbe_ldo_io_cmd_invalid_opcode);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_test_get_first_state()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_get_attributes(fbe_object_id_t object_id,
 *   fbe_logical_drive_attributes_t *expected_attributes_p)
 ****************************************************************
 * @brief
 *  This function tests the get attributes for an object.
 *
 * @param object_id - Object to test. 
 * @param expected_attributes_p - The expected values for attributes.
 *
 * @return:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  09/09/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_test_get_attributes( fbe_object_id_t object_id,
                             fbe_logical_drive_attributes_t *expected_attributes_p)
{
    fbe_status_t status;
    fbe_logical_drive_attributes_t received_attributes;

    /* Next, get the attributes of the device..
     */  
    status = fbe_api_logical_drive_get_attributes(object_id, &received_attributes);

    /* We either expect success or that failure was expected. 
     */ 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (status == FBE_STATUS_OK)
    {
        mut_logging_t level = MUT_LOG_MEDIUM;

        /* Check to make sure that the sizes were filled in.
         */
        MUT_ASSERT_TRUE(received_attributes.imported_block_size == 
                        expected_attributes_p->imported_block_size);
        MUT_ASSERT_TRUE(received_attributes.imported_capacity == 
                        expected_attributes_p->imported_capacity);

        mut_printf(level, "logical drive: imported block size:     0x%x\n", 
                   received_attributes.imported_block_size);
        mut_printf(level, "logical drive: imported capacity :     0x%llx\n", 
                   (unsigned long long)received_attributes.imported_capacity);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return status;
}
/* end fbe_ldo_test_get_attributes */

/*!**************************************************************
 * @fn fbe_ldo_test_get_attributes_cases(void)
 ****************************************************************
 * @brief
 *  Test get attributes for all drives we support.
 * 
 *  We test all the possible block sizes we support.
 *
 * @return fbe_u32_t - status of the test. 
 *
 * @version
 *  8/26/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_ldo_test_get_attributes_cases(void)
{
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_u32_t drive_index;

    /* For every block size we support, test the negotiate with invalid data.
     */
    for (drive_index = 0; drive_index < FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX; drive_index++)
    {
        fbe_block_size_t block_size = fbe_ldo_test_get_block_size_for_drive_index(drive_index);
        fbe_logical_drive_attributes_t attributes;

        /* We expect all these cases to fail.
         */
        object_id = fbe_ldo_test_get_object_id_for_drive_block_size(block_size);

        /* Set the expected attributes.
         */
        attributes.imported_block_size = block_size;
        attributes.imported_capacity = FBE_LDO_DEFAULT_FILE_SIZE_IN_BLOCKS;

        /* Issue the get attributes with the expected values.
         */
        status = fbe_ldo_test_get_attributes(object_id, &attributes);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return status;
}
/******************************************
 * end fbe_ldo_test_get_attributes_cases()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_pdo_failure(void)
 ****************************************************************
 * @brief
 *  Test case where PDO goes into Fail state.
 *
 * @return fbe_u32_t - status of the test. 
 *
 * @version
 *  1/13/2009 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_ldo_test_pdo_failure(void)
{
    fbe_status_t status;
    fbe_object_id_t pdo_obj_id;
    fbe_object_id_t ldo_obj_id;
    fbe_media_error_edge_tap_object_t *tap_p = NULL;
    fbe_api_set_edge_hook_t hook_info;
    fbe_u32_t qualifier;
    fbe_api_block_status_values_t block_status_values;
    fbe_lba_t error_lba;
    fbe_block_count_t error_blocks;
    fbe_api_edge_tap_create_info_t edge_tap_create_info;

    /* Create our edge tap object.
     */
    fbe_media_error_edge_tap_object_create(&edge_tap_create_info);
    tap_p = edge_tap_create_info.edge_tap_object_p;

    /* Get the physical drive object id.
     * We are using 0,0,0 since we know this is a 520 byte block device. 
     */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &pdo_obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Verify if the drive exists.
     */
    status = fbe_api_get_logical_drive_object_id_by_location(0, 0, 0, &ldo_obj_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ldo_obj_id != FBE_OBJECT_ID_INVALID);

    /* Set the hook on to physical drive ssp edge.
     * We add just enough media errors in a row to force the PDO to fail. 
     */
    error_lba = 0;
    error_blocks = 9;
    status = fbe_media_error_edge_tap_object_add(tap_p, 
                                                 error_lba, /* lba */
                                                 error_blocks, /* blocks */
                                                 FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    hook_info.hook = edge_tap_create_info.edge_tap_fn;
    status = fbe_api_set_ssp_edge_hook(pdo_obj_id, &hook_info);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: successfully set edge hook to object %d. ===\n", __FUNCTION__, pdo_obj_id);
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Issuing write to force pdo to the fail state.  ===", __FUNCTION__);

    /* Issue write.
     * We expect this to fail because of the number of media errors injected. 
     * We also expect the PDO to transition to a failed state. 
     * This will cause the LDO to transition to a failed state. 
     */
    status = fbe_test_io_write_pattern(ldo_obj_id,
                                       FBE_IO_TEST_TASK_PATTERN_ADDRESS_PASS_COUNT,
                                       0x55,    /* pass count, arbitrary */
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                       0,    /* lba */
                                       100,    /* blocks*/
                                       520,    /* exported block size */
                                       1,    /* exported optimal block size */
                                       520,    /* imported block size */
                                       &block_status_values,
                                       &qualifier);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(block_status_values.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    MUT_ASSERT_INT_EQUAL(block_status_values.qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);

    /* Make sure the LDO and PDO are failed.
     */
    status = fbe_api_wait_for_object_lifecycle_state(pdo_obj_id, 
                                                     FBE_LIFECYCLE_STATE_FAIL, 
                                                     FBE_LDO_LIFECYCLE_TIME_MSEC,
                                                     FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(ldo_obj_id, 
                                                     FBE_LIFECYCLE_STATE_FAIL, 
                                                     FBE_LDO_LIFECYCLE_TIME_MSEC,
                                                     FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear out the edge tap records.
     */
    status = fbe_media_error_edge_tap_object_clear(tap_p, 
                                                   error_lba, /* lba */
                                                   error_blocks/* blocks */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove the hook on physical drive ssp edge.
     */
    hook_info.hook = NULL;
    status = fbe_api_set_ssp_edge_hook(pdo_obj_id, &hook_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_test_pdo_failure()
 ******************************************/
/*****************************************
 * end fbe_logical_drive_test_io.c
 *****************************************/
