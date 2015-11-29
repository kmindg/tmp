/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_pre_read_descriptor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions for our pre read descriptor manipulation.
 *
 * HISTORY
 *   9/10/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_transport.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "fbe_logical_drive_test_state_object.h"
#include "fbe_logical_drive_test_alloc_mem_mock_object.h"
#include "fbe_logical_drive_test_send_io_packet_mock_object.h"
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* This structure contains the test cases for validating pre-read descriptors
 * that are not large enough and thus will provoke an error if we try to use it.
 */
fbe_ldo_test_pre_read_desc_task_t fbe_ldo_test_pre_read_desc_too_small_cases[] = 
{
    /* There are no aligned at start and end cases, since 
     * a pre-read descriptor is not needed and thus is always considered valid. 
     */

    /* Start unaligned.
     * Pre-read   Exp Exp  Imp   Import  Imp Imp Exp Exp 
     * lba blocks     Opt        Opt     Bl  Opt Lba Blocks 
     */
    {1,    63,    520, 64,  512, 65,      1, 64, 1, 63},
    {0,     0,    520, 64,  512, 65,      1, 64, 1, 63},
    {1,    1,     520, 64,  512, 65,      1, 64, 1, 63},
    {2,    63,    520, 64,  512, 65,      1, 64, 1, 63},
    {1,    64,    520, 64,  512, 65,      1, 64, 1, 63},

    {2,    63,    520, 64,  512, 65,      2, 63, 2, 62},
    {3,    62,    520, 64,  512, 65,      2, 63, 2, 62},
    {4,    62,    520, 64,  512, 65,      2, 63, 2, 62},
    {5,    61,    520, 64,  512, 65,      2, 63, 2, 62},

    {3,    61,    520, 64,  512, 65,      3, 62, 3, 61},
    {3,    60,    520, 64,  512, 65,      3, 62, 3, 61},
    {4,    62,    520, 64,  512, 65,      3, 62, 3, 61},
    {5,    62,    520, 64,  512, 65,      3, 62, 3, 61},
    {6,    60,    520, 64,  512, 65,      3, 62, 3, 61},

    {4,    60,    520, 64,  512, 65,      4, 61, 4, 60},
    {4,    60,    520, 64,  512, 65,      4, 61, 4, 60},
    {5,    61,    520, 64,  512, 65,      4, 61, 4, 60},
    {5,    60,    520, 64,  512, 65,      4, 61, 4, 60},

    {5,    59,    520, 64,  512, 65,      5, 60, 5, 59},
    {6,    60,    520, 64,  512, 65,      5, 60, 5, 59},
    {7,    60,    520, 64,  512, 65,      5, 60, 5, 59},
    {5,    60,    520, 64,  512, 65,      5, 60, 5, 59},

    {6,    58,    520, 64,  512, 65,      6, 59, 6, 58},
    {6,    57,    520, 64,  512, 65,      6, 59, 6, 58},
    {7,    59,    520, 64,  512, 65,      6, 59, 6, 58},
    {6,    59,    520, 64,  512, 65,      6, 59, 6, 58},


    {59,   5,    520, 64,  512, 65,      59, 6, 59, 5},
    {59,   1,    520, 64,  512, 65,      59, 6, 59, 5},
    {57,   1,    520, 64,  512, 65,      59, 6, 59, 5},
    {59,   6,    520, 64,  512, 65,      59, 6, 59, 5},
    {60,   6,    520, 64,  512, 65,      59, 6, 59, 5},

    {61,   1,    520, 64,  512, 65,      63, 2, 63, 1},
    {64,   1,    520, 64,  512, 65,      63, 2, 63, 1},
    {63,   1,    520, 64,  512, 65,      63, 2, 63, 1},
 
    /* End unaligned.
     */
    {0,    63,   520, 64,  512, 65,       0, 64, 0, 63},
    {1,    62,   520, 64,  512, 65,       0, 64, 0, 63},

    {0,    62,   520, 64,  512, 65,       0, 63, 0, 62},
    {1,    61,   520, 64,  512, 65,       0, 63, 0, 62},
    {2,    60,   520, 64,  512, 65,       0, 63, 0, 62},

    {0,    61,   520, 64,  512, 65,       0, 62, 0, 61}, 
    {1,    60,   520, 64,  512, 65,       0, 62, 0, 61},
    {1,    59,   520, 64,  512, 65,       0, 62, 0, 61},

    {0,    60,   520, 64,  512, 65,       0, 61, 0, 60},
    {1,    59,   520, 64,  512, 65,       0, 61, 0, 60},
    {1,    58,   520, 64,  512, 65,       0, 61, 0, 60},

    {0,    59,   520, 64,  512, 65,       0, 60, 0, 59},
    {1,    58,   520, 64,  512, 65,       0, 60, 0, 59},
    {1,    57,   520, 64,  512, 65,       0, 60, 0, 59},

    {0,    58,   520, 64,  512, 65,       0, 59, 0, 58},
    {1,    57,   520, 64,  512, 65,       0, 59, 0, 58},
    {1,    56,   520, 64,  512, 65,       0, 59, 0, 58},

    {0,    57,   520, 64,  512, 65,       0, 58, 0, 57},
    {1,    56,   520, 64,  512, 65,       0, 58, 0, 57},
    {1,    55,   520, 64,  512, 65,       0, 58, 0, 57},

    {0,    10,   520, 64,  512, 65,       0, 11, 0, 10},
    {1,     9,   520, 64,  512, 65,       0, 11, 0, 10},
    {1,     8,   520, 64,  512, 65,       0, 11, 0, 10},

    {0,     9,   520, 64,  512, 65,       0, 10, 0,  9},
    {1,     8,   520, 64,  512, 65,       0, 10, 0,  9},
    {1,     7,   520, 64,  512, 65,       0, 10, 0,  9},

    {0,     8,   520, 64,  512, 65,       0,  9, 0,  8},
    {1,     7,   520, 64,  512, 65,       0,  9, 0,  8},
    {1,     6,   520, 64,  512, 65,       0,  9, 0,  8},

    {0,     7,   520, 64,  512, 65,       0,  8, 0,  7},
    {1,     6,   520, 64,  512, 65,       0,  8, 0,  7},
    {1,     5,   520, 64,  512, 65,       0,  8, 0,  7},

    {0,     6,   520, 64,  512, 65,       0,  7, 0,  6},
    {1,     5,   520, 64,  512, 65,       0,  7, 0,  6},
    {1,     4,   520, 64,  512, 65,       0,  7, 0,  6},

    {0,     5,   520, 64,  512, 65,       0,  6, 0,  5},
    {1,     4,   520, 64,  512, 65,       0,  6, 0,  5},
    {1,     3,   520, 64,  512, 65,       0,  6, 0,  5},

    {0,     4,   520, 64,  512, 65,       0,  5, 0,  4},
    {1,     3,   520, 64,  512, 65,       0,  5, 0,  4},
    {1,     2,   520, 64,  512, 65,       0,  5, 0,  4},

    {0,     3,   520, 64,  512, 65,       0,  4, 0,  3},
    {1,     2,   520, 64,  512, 65,       0,  4, 0,  3},
    {1,     1,   520, 64,  512, 65,       0,  4, 0,  3},

    {0,     2,   520, 64,  512, 65,       0,  3, 0,  2},
    {1,     1,   520, 64,  512, 65,       0,  3, 0,  2},
    {0,     1,   520, 64,  512, 65,       0,  3, 0,  2},

    {0,     1,   520, 64,  512, 65,       0,  2, 0,  1},
    {2,     1,   520, 64,  512, 65,       0,  2, 0,  1},
    {2,     2,   520, 64,  512, 65,       0,  2, 0,  1},

    {0,     0,   520, 64,  512, 65,       0,  1, 0,  1},
    {2,     1,   520, 64,  512, 65,       0,  1, 0,  1},
    {2,     2,   520, 64,  512, 65,       0,  1, 0,  1},

    /* Insert any new test cases above this point.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0}      /* -1, zero means end. */
};
/* end fbe_ldo_test_pre_read_desc_too_small_cases
 */

/* This structure contains the test cases for validating legal pre-read
 * descriptors. 
 */
fbe_ldo_test_pre_read_desc_task_t fbe_ldo_test_pre_read_desc_normal_cases[] = 
{
    /* Pre-read   Exp Exp  Imp   Import  Imp Imp Exp Exp
     * lba blocks     Opt        Opt     Bl  Opt Lba Blocks
     */
    /* Aligned.
     */
    {0,    64,    520, 64,  512, 65,    0x0, 65, 0, 64},    
    /* If the start and is aligned, then we allow any pre-read descriptor,
     * since it is not used. 
     */
    {0,    63,    520, 64,  512, 65,    0x0, 65, 0, 64},
    {0,    62,    520, 64,  512, 65,    0x0, 65, 0, 64},
    {0,    1,     520, 64,  512, 65,    0x0, 65, 0, 64},
    {1,    63,    520, 64,  512, 65,    0x0, 65, 0, 64},
    {1,    64,    520, 64,  512, 65,    0x0, 65, 0, 64},
    {1,    65,    520, 64,  512, 65,    0x0, 65, 0, 64},
    {63,   1,     520, 64,  512, 65,    0x0, 65, 0, 64},

    /* Start unaligned.
     */
    {0,    64,    520, 64,  512, 65,      1, 64, 1, 63},
    {0,    63,    520, 64,  512, 65,      1, 64, 1, 63},
    {0,     1,    520, 64,  512, 65,      1, 64, 1, 63},
    {0,     2,    520, 64,  512, 65,      1, 64, 1, 63},
    {0,     3,    520, 64,  512, 65,      1, 64, 1, 63},
    {1,    63,    520, 64,  512, 65,      2, 63, 2, 62},
    {1,     1,    520, 64,  512, 65,      2, 63, 2, 62},
    {2,    62,    520, 64,  512, 65,      3, 62, 3, 61},
    {2,     1,    520, 64,  512, 65,      3, 62, 3, 61},
    {3,    61,    520, 64,  512, 65,      4, 61, 4, 60},
    {3,     1,    520, 64,  512, 65,      4, 61, 4, 60},
    {4,    60,    520, 64,  512, 65,      5, 60, 5, 59},
    {4,     1,    520, 64,  512, 65,      5, 60, 5, 59},
    {5,    59,    520, 64,  512, 65,      6, 59, 6, 58},
    {5,     1,    520, 64,  512, 65,      6, 59, 6, 58},


    {58,   6,    520, 64,  512, 65,      59, 6, 59, 5},
    {58,   1,    520, 64,  512, 65,      59, 6, 59, 5},
    {59,   5,    520, 64,  512, 65,      60, 5, 60, 4},
    {59,   1,    520, 64,  512, 65,      60, 5, 60, 4},
    {60,   4,    520, 64,  512, 65,      61, 4, 61, 3},
    {60,   1,    520, 64,  512, 65,      61, 4, 61, 3},
    {61,   3,    520, 64,  512, 65,      62, 3, 62, 2},
    {61,   1,    520, 64,  512, 65,      62, 3, 62, 2},
    {61,   2,    520, 64,  512, 65,      62, 3, 62, 2},
    {62,   2,    520, 64,  512, 65,      63, 2, 63, 1},
    {62,   1,    520, 64,  512, 65,      63, 2, 63, 1},

    {64,   64,    520, 64,  512, 65,     66, 64, 66, 63},
    {65,   63,    520, 64,  512, 65,     67, 63, 67, 62},
    {66,   62,    520, 64,  512, 65,     68, 62, 68, 61},
    {67,   61,    520, 64,  512, 65,     69, 61, 69, 60},
    {68,   60,    520, 64,  512, 65,     70, 60, 70, 59},
    {69,   59,    520, 64,  512, 65,     71, 59, 71, 58},
 
    /* End unaligned.
     */
    {0,    64,   520, 64,  512, 65,       0, 64, 0, 63},
    {0,    63,   520, 64,  512, 65,       0, 63, 0, 62},
    {0,    62,   520, 64,  512, 65,       0, 62, 0, 61},
    {0,    61,   520, 64,  512, 65,       0, 61, 0, 60},
    {0,    60,   520, 64,  512, 65,       0, 60, 0, 59},
    {0,    59,   520, 64,  512, 65,       0, 59, 0, 58},
    {0,    58,   520, 64,  512, 65,       0, 58, 0, 57},

    {0,    11,   520, 64,  512, 65,       0, 11, 0, 10},
    {0,    10,   520, 64,  512, 65,       0, 10, 0, 9},
    {0,     9,   520, 64,  512, 65,       0,  9, 0, 8},
    {0,     8,   520, 64,  512, 65,       0,  8, 0, 7},
    {0,     7,   520, 64,  512, 65,       0,  7, 0, 6},
    {0,     6,   520, 64,  512, 65,       0,  6, 0, 5},
    {0,     5,   520, 64,  512, 65,       0,  5, 0, 4},
    {0,     4,   520, 64,  512, 65,       0,  4, 0, 3},
    {0,     3,   520, 64,  512, 65,       0,  3, 0, 2},
    {0,     2,   520, 64,  512, 65,       0,  2, 0, 1},

    /* Aligned.
     */
    {0,    2,    520, 2,   512, 3,      0x0,  3, 0, 2},
    {0,    4,    520, 4,   512, 5,      0x0,  5, 0, 4},
    {0,    6,    520, 6,   512, 7,      0x0,  7, 0, 6},
    {0,    8,    520, 8,   512, 9,      0x0,  9, 0, 8},
    {0,   32,    520, 32,  512, 33,     0x0, 33, 0, 32},

    /* Aligned with extra Beginning.
     */
    {0,    6,    520, 2,   512, 3,      3,  3, 2, 2},
    {0,    12,   520, 4,   512, 5,      10, 5, 8, 4},
    {0,    6,    520, 6,   512, 7,      7,  7, 6, 6},
    {0,    22,   520, 2,   512, 3,     30,  3, 20, 2},

    /* Insert any new test cases above this point.
     */
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0}      /* -1, zero means end. */
};
/* end fbe_ldo_test_pre_read_desc_normal_cases
 */

/*!***************************************************************
 * @fn fbe_ldo_test_validate_pre_read_desc_missing_params(void)
 ****************************************************************
 * @brief
 *  This function validates the cases where the edge descriptor
 *  is missing parameters.
 * 
 * @param  - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  12/11/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_validate_pre_read_desc_missing_params(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_pre_read_descriptor_t pre_read_desc;
    fbe_payload_pre_read_descriptor_t * pre_read_desc_p = &pre_read_desc;
    fbe_sg_element_t sg;
    fbe_lba_t lba = 0;
    fbe_block_count_t blocks = 5;
    
    /* For this case we send in a NULL for the edge descriptor,
     * and validate that an error is returned.
     */
    status = fbe_ldo_validate_pre_read_desc(NULL, 
                                            520,    /* exported block size */
                                            64,    /* exported optimal size */
                                            512,    /* imported block size */
                                            65,    /* imported optimal size */
                                            0,    /* exported lba */
                                            1,    /* exported blocks */
                                            0,    /* imported lba */
                                            2    /* imported blocks */ );
    MUT_ASSERT_FBE_STATUS_FAILED(status);
    
    /* For this case we send in 0 for the blocks,
     * and validate that an error is returned.
     */
    fbe_payload_pre_read_descriptor_set_lba(pre_read_desc_p, lba); 
    fbe_payload_pre_read_descriptor_set_block_count(pre_read_desc_p, 0);
    fbe_payload_pre_read_descriptor_set_sg_list(pre_read_desc_p, &sg);
    
    status = fbe_ldo_validate_pre_read_desc(pre_read_desc_p, 
                                            520,    /* exported block size */
                                            64,    /* exported optimal size */
                                            512,    /* imported block size */
                                            65,    /* imported optimal size */
                                            0,    /* exported lba */
                                            1,    /* exported blocks */
                                            0,    /* imported lba */
                                            2    /* imported blocks */ );
    MUT_ASSERT_FBE_STATUS_FAILED(status);

    /* For this case we send in NULL for the edge desc sg,
     * and validate that an error is returned.
     */
    fbe_payload_pre_read_descriptor_set_lba(pre_read_desc_p, lba); 
    fbe_payload_pre_read_descriptor_set_block_count(pre_read_desc_p, blocks);
    fbe_payload_pre_read_descriptor_set_sg_list(pre_read_desc_p, NULL);
    
    status = fbe_ldo_validate_pre_read_desc(pre_read_desc_p, 
                                            520,    /* exported block size */
                                            64,    /* exported optimal size */
                                            512,    /* imported block size */
                                            65,    /* imported optimal size */
                                            0,    /* exported lba */
                                            1,    /* exported blocks */
                                            0,    /* imported lba */
                                            2    /* imported blocks */ );
    MUT_ASSERT_FBE_STATUS_FAILED(status);
    
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_validate_pre_read_desc_missing_params() */

/*!***************************************************************
 * @fn fbe_ldo_test_validate_pre_read_desc_status(
 *                fbe_lba_t lba,
 *                fbe_block_count_t blocks,
 *                fbe_lba_t pre_read_lba,
 *                fbe_block_count_t pre_read_blocks,
 *                fbe_block_size_t exported_block_size,
 *                fbe_block_size_t exported_opt_block_size,
 *                fbe_block_size_t imported_block_size,
 *                fbe_block_size_t imported_opt_block_size,
 *                fbe_lba_t imported_lba,
 *                fbe_block_count_t imported_blocks,
 *                fbe_bool_t b_success )
 ****************************************************************
 * DESCRIPTION:
 *  This function tests a single case against fbe_ldo_validate_pre_read_desc() and
 *  validates the results.
 *
 * @param lba - The exported logical block address for this transfer.
 * @param blocks - The total exported blocks in the transfer.
 * @param pre_read_lba - The logical block address where the pre-read start.
 * @param pre_read_blocks - The total exported blocks in the pre-read.
 * @param exported_block_size - The size in bytes of the exported block.
 * @param exported_opt_block_size - The size in blocks (exported blocks) of the
 *                                  exported optimum block.
 * @param imported_block_size - The size in bytes of the physical device.
 * @param imported_opt_block_size - The size in blocks of the
 *                                  imported optimum block size.
 * @param imported_lba - Logical block address for imported block.
 * @param imported_blocks - Logical block count for imported block.
 * @param b_success - TRUE if success expected FALSE otherwise.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  12/11/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_validate_pre_read_desc_status(
    fbe_lba_t lba,
    fbe_block_count_t blocks,
    fbe_lba_t pre_read_lba,
    fbe_block_count_t pre_read_blocks,
    fbe_block_size_t exported_block_size,
    fbe_block_size_t exported_opt_block_size,
    fbe_block_size_t imported_block_size,
    fbe_block_size_t imported_opt_block_size,
    fbe_lba_t imported_lba,
    fbe_block_count_t imported_blocks,
    fbe_bool_t b_success )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_pre_read_descriptor_t pre_read_desc;
    fbe_payload_pre_read_descriptor_t * pre_read_desc_p = &pre_read_desc;
    fbe_sg_element_t sg;
    
    /* Create the edge descriptor with the given values.
     */
    fbe_payload_pre_read_descriptor_set_lba(pre_read_desc_p, pre_read_lba); 
    fbe_payload_pre_read_descriptor_set_block_count(pre_read_desc_p, pre_read_blocks);
    fbe_payload_pre_read_descriptor_set_sg_list(pre_read_desc_p, &sg);

    /* Call our function under test with the input values.
     */
    status = fbe_ldo_validate_pre_read_desc(pre_read_desc_p, 
                                            exported_block_size,
                                            exported_opt_block_size,
                                            imported_block_size,
                                            imported_opt_block_size,
                                            lba,
                                            blocks,
                                            imported_lba,
                                            imported_blocks );

    /* Make sure the return status was expected.
     */
    if (b_success)
    {
        MUT_ASSERT_FBE_STATUS_OK(status);
    }
    else
    {
        MUT_ASSERT_FBE_STATUS_FAILED(status);
    }
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_validate_pre_read_desc_status() */

/*!***************************************************************
 * @fn fbe_ldo_test_validate_pre_read_desc_array(
 *                fbe_ldo_test_pre_read_desc_task_t *task_array_p,
 *                fbe_bool_t b_expected_status)
 ****************************************************************
 * @brief
 *  This function executes a set of tasks against the
 *  fbe_ldo_test_validate_pre_read_desc_status() function.
 *
 * @param task_array_p - The pointer to the test case.
 * @param b_expected_status - The status expected on this command.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  12/11/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_validate_pre_read_desc_array( fbe_ldo_test_pre_read_desc_task_t *task_array_p,
                                                    fbe_bool_t b_expected_status )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    /* First loop over all the tasks.  Stop when we hit the end of the table
     * (where the lba is marked invalid).  Also stop when the status is not expected.
     */
    while (task_array_p[index].pre_read_desc_lba != FBE_LDO_TEST_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        status = fbe_ldo_test_validate_pre_read_desc_status(
            task_array_p[index].exported_lba,
            task_array_p[index].exported_blocks,
            task_array_p[index].pre_read_desc_lba,
            task_array_p[index].pre_read_desc_blocks,
            task_array_p[index].exported_block_size,
            task_array_p[index].exported_opt_block_size,
            task_array_p[index].imported_block_size,
            task_array_p[index].imported_opt_block_size,
            task_array_p[index].imported_lba,
            task_array_p[index].imported_blocks,
            b_expected_status);

        /* Make sure the status is good.
         */
        MUT_ASSERT_FBE_STATUS_OK(status);
        index++;

    } /* end while not at end of task table and status is expected. */
    
    return status;
}
/* end fbe_ldo_test_validate_pre_read_desc_array() */

/*!***************************************************************
 * @fn fbe_ldo_test_validate_pre_read_descriptor(void)
 ****************************************************************
 * @brief
 *  This function validates the cases where the edge descriptor is
 *  too small and it validates the cases where the edge descriptor
 *  is just right.
 *
 * @param  - none.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  12/11/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_validate_pre_read_descriptor(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Validate all cases where the edge desc is valid.
     * We send in TRUE for the expected status since we expect these cases to 
     * succeed. 
     */
    status = fbe_ldo_test_validate_pre_read_desc_array(
        fbe_ldo_test_pre_read_desc_normal_cases,
        TRUE /* expected status */ );
    MUT_ASSERT_FBE_STATUS_OK(status);
    
    /* Validate all cases where the edge desc does not
     * have enough data for the imported I/O.
     * We send in FALSE, since we expect these cases to fail.
     */
    status = fbe_ldo_test_validate_pre_read_desc_array(
        fbe_ldo_test_pre_read_desc_too_small_cases,
        FALSE /* Expected status. */ );
    MUT_ASSERT_FBE_STATUS_OK(status);
    return status;
}
/* end fbe_ldo_test_validate_pre_read_descriptor() */

/*!***************************************************************
 * @fn fbe_ldo_add_pre_read_descriptor_tests(mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  This function adds the logical drive pre-read descriptor tests to
 *  the given input test suite.
 *
 * @param suite_p - the suite to add pre-read descriptor tests to.
 *
 * @return None.
 *
 * HISTORY:
 *  08/19/08 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_add_pre_read_descriptor_tests(mut_testsuite_t * const suite_p)
{
    /* Simply setup the pre-read descriptor tests.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_validate_pre_read_desc_missing_params,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_validate_pre_read_descriptor,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/* end fbe_ldo_add_pre_read_descriptor_tests() */

/*************************
 * end file fbe_logical_drive_test_pre_read_descriptor.c
 *************************/
