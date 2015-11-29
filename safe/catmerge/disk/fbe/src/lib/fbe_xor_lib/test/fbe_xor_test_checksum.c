/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_xor_test_checksum.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions for the xor checksum library.
 *
 * @version
 *   8/10/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_xor_api.h"
#include "fbe_xor_private.h"
#include "mut.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_time.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#define XOR_TEST_CHECKSUM_MAX_BLOCKS 32
#define XOR_TEST_CHECKSUM_MAX_SG_ELEMENTS 2

static fbe_sector_t fbe_xor_test_checksum_sectors[FBE_XOR_MAX_FRUS][XOR_TEST_CHECKSUM_MAX_BLOCKS];
static fbe_sg_element_t fbe_xor_test_checksum_sgs[FBE_XOR_MAX_FRUS][XOR_TEST_CHECKSUM_MAX_SG_ELEMENTS+1];
static fbe_status_t fbe_xor_test_send_execute_stamps(fbe_u32_t num_frus,
                                           fbe_u32_t start_position,
                                           fbe_sg_element_t *sg_p[],
                                           fbe_xor_option_t options,
                                           fbe_xor_error_t *eboard_p,
                                           fbe_lba_t lba,
                                           fbe_u32_t blocks,
                                           fbe_xor_status_t *xor_status_p);

/* These are the lba values we will test with.
 */
static fbe_lba_t fbe_xor_test_checksum_lbas[] = 
{
    0,
    1, 
    2, 
    3,
    4,
    0x10,
    0x20,
    0x300,
    0x400,
    0x301,
    0x401,
    0x5210,
    0x6210,
    0x7310,
    0x8410,
    0xAB10,
    0xBB10,

    0x100000,
    0x200000,
    0x3000000,
    0x4000000,
    0x30100000,
    0x40100000,
    0xFFFFFFFF,

    0x5210100001,
    0x6210020001,
    0x7310003001,
    0x8410000401,
    0xAB10000051,
    0xBB10000061,

    0x521090000001,
    0x621008000001,
    0x731000700000,
    0x841000060001,
    0xAB1000005001,
    0xBB1000000401,
    0xFFFFFFFFFFFF,

    0x52101000000100,
    0x62100200000100,
    0x73100030000000,
    0x84100004000100,
    0xAB100000500100,
    0xBB100000060100,

    0xFFFFFFFFFFFFFF,

    0x5210200000010000,
    0x6210030000010000,
    0x7310004000000000,
    0x8410000500010000,
    0xAB10000060010000,
    0xBB10000007010000,
 
    0xFFFFFFFFFFFFFFFF,
};
/* end fbe_xor_test_checksum_lbas */

/*!***************************************************************
 *          fbe_xor_test_initialize_memory()
 ****************************************************************
 * @brief
 *  This function initializes memory before using data
 *
 * @param memory_p - Pointer to memory to use in sg.
 * @param bytes - Bytes to setup in sg.
 *
 * @return
 *  None.
 *
 ****************************************************************/
void fbe_xor_test_initialize_memory(fbe_u8_t *memory_p,
                                    fbe_u32_t bytes)
{
    fbe_set_memory((void *)memory_p, 'X', bytes);
}
/* end of */
/*!***************************************************************
 *          fbe_xor_test_setup_sg_list()
 ****************************************************************
 * @brief
 *  This function sets up an sg list based on the input parameters.
 *
 * @param sg_p - Sg list to setup.
 * @param memory_p - Pointer to memory to use in sg.
 * @param bytes - Bytes to setup in sg.
 * @param max_bytes_per_sg - Maximum number of bytes per sg entry.
 *                           Allows us to setup longer sg lists.
 *
 * @return
 *  None.
 *
 * @author:
 *  8/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_test_setup_sg_list(fbe_sg_element_t *const sg_p,
                                fbe_u8_t *memory_p,
                                fbe_u32_t bytes,
                                fbe_u32_t max_bytes_per_sg)
{
    fbe_u32_t current_bytes;
    fbe_u32_t bytes_remaining = bytes;
    fbe_sg_element_t *current_sg_p = sg_p;
    
    if (max_bytes_per_sg == 0)
    {
        /* If the caller did not specify the max bytes per sg,
         * then assume we can put as much as we want in each sg entry.
         */
        max_bytes_per_sg = FBE_U32_MAX;
    }

    /* Loop over all the bytes that we are adding to this sg list.
     */
    while (bytes_remaining != 0)
    {
        if (max_bytes_per_sg > bytes_remaining)
        {
            /* Don't allow current_bytes to be negative.
             */
            current_bytes = bytes_remaining;
        }
        else
        {
            current_bytes = max_bytes_per_sg;
        }

        /* Setup this sg element.
         */
        fbe_sg_element_init(current_sg_p, current_bytes, memory_p);
        fbe_xor_test_initialize_memory(memory_p, current_bytes);
        fbe_sg_element_increment(&current_sg_p);
        memory_p += current_bytes;
        bytes_remaining -= current_bytes;

    } /* end while bytes remaining != 0 */

    /* Terminate the list.
     */
    fbe_sg_element_terminate(current_sg_p);
    return;
}
/* end fbe_xor_test_setup_sg_list */

void fbe_xor_test_setup_sgs(fbe_u32_t num_frus,
                            fbe_u32_t blocks,
                            fbe_u32_t max_blocks_per_sg)
{
    fbe_u32_t fru_index;
    for (fru_index = 0; fru_index < num_frus; fru_index++)
    {
        fbe_xor_test_setup_sg_list(&fbe_xor_test_checksum_sgs[fru_index][0],
                                   (fbe_u8_t*)&fbe_xor_test_checksum_sectors[fru_index][0],
                                   (blocks * FBE_BE_BYTES_PER_BLOCK),
                                   (max_blocks_per_sg * FBE_BE_BYTES_PER_BLOCK));
    }
    return;
}
void fbe_xor_test_execute_stamps_case(fbe_u32_t num_frus,
                                      fbe_u32_t start_pos,
                                      fbe_lba_t lba,
                                      fbe_u32_t blocks,
                                      fbe_xor_error_t *eboard_p,
                                      fbe_status_t expected_execute_stamps_status,
                                      fbe_xor_status_t expected_xor_status,
                                      fbe_bool_t b_corrupt_crc,
                                      fbe_bool_t b_corrupt_lba)
{
    fbe_status_t        execute_stamps_status = FBE_STATUS_INVALID;
    fbe_xor_status_t    gen_xor_status = expected_xor_status;
    fbe_xor_status_t    xor_status = FBE_XOR_STATUS_INVALID;
    fbe_xor_option_t    gen_options = (FBE_XOR_OPTION_GEN_CRC | FBE_XOR_OPTION_GEN_LBA_STAMPS);
    fbe_xor_option_t    chk_options = (FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS);
    fbe_u32_t           fru_index;
    fbe_sg_element_t   *sg_array[FBE_XOR_MAX_FRUS];

    /* We only allocate XOR_TEST_CHECKSUM_MAX_BLOCKS total blocks to test with.
     */
    MUT_ASSERT_TRUE(blocks <= XOR_TEST_CHECKSUM_MAX_BLOCKS);

    /* Setup sgs with at least 2 sg elements.
     */
    fbe_xor_test_setup_sgs(num_frus,
                           XOR_TEST_CHECKSUM_MAX_BLOCKS,
                           (XOR_TEST_CHECKSUM_MAX_BLOCKS / XOR_TEST_CHECKSUM_MAX_SG_ELEMENTS));

    /* Populate sg array.
     */
    for (fru_index = 0; fru_index < num_frus; fru_index++)
    {
        sg_array[fru_index] = &fbe_xor_test_checksum_sgs[fru_index][0];
    }

    /* Perform the generate checksum.
     */
    if (b_corrupt_crc)
    { 
        gen_options &= ~FBE_XOR_OPTION_GEN_CRC;
        chk_options &= ~FBE_XOR_OPTION_CHK_LBA_STAMPS;
        gen_xor_status = FBE_XOR_STATUS_NO_ERROR; 
    } 
    else if (b_corrupt_lba)
    { 
        gen_options &= ~FBE_XOR_OPTION_GEN_LBA_STAMPS;
        chk_options &= ~FBE_XOR_OPTION_CHK_CRC; 
        gen_xor_status = FBE_XOR_STATUS_NO_ERROR; 
    }
    execute_stamps_status = fbe_xor_test_send_execute_stamps(num_frus,
                                              start_pos,
                                              &sg_array[0],
                                              gen_options, eboard_p, 
                                              lba, blocks, &xor_status);
    MUT_ASSERT_INT_EQUAL(execute_stamps_status, expected_execute_stamps_status);
    MUT_ASSERT_INT_EQUAL(xor_status, gen_xor_status);

    /* Perform the check checksum.
     */
    execute_stamps_status = fbe_xor_test_send_execute_stamps(num_frus,
                                              start_pos,
                                              &sg_array[0],
                                              chk_options, eboard_p, 
                                              lba, blocks, &xor_status);

    /* Make sure no errors.
     */
    MUT_ASSERT_INT_EQUAL(execute_stamps_status, expected_execute_stamps_status);
    MUT_ASSERT_INT_EQUAL(xor_status, expected_xor_status);

    return;
}
void fbe_xor_test_execute_stamps_normal(void)
{
    fbe_u32_t       num_frus;
    fbe_u32_t       start_pos;
    fbe_lba_t       lba;
    fbe_u32_t       lba_index;
    fbe_u32_t       blocks;
    fbe_xor_error_t eboard;

    /* Create a valid eboard
     */
    fbe_xor_init_eboard(&eboard, 0x123, 0x1000);

    /* Perform one test case for each number of frus.
     */
    for (num_frus = 1; num_frus <= FBE_XOR_MAX_FRUS; num_frus++)
    {
        for (start_pos = 0; start_pos < FBE_XOR_MAX_FRUS; start_pos++)
        {
            lba = 0;
            for (lba_index = 0; (lba != -1); lba_index++)
            {
                lba = fbe_xor_test_checksum_lbas[lba_index];

                for (blocks = 1; blocks <= XOR_TEST_CHECKSUM_MAX_BLOCKS; blocks += 8)
                {
                    /* Perform the check checksum.
                     */
                    fbe_xor_test_execute_stamps_case(num_frus, start_pos, lba, blocks, &eboard,
                                                     FBE_STATUS_OK, FBE_XOR_STATUS_NO_ERROR,
                                                     FBE_FALSE, FBE_FALSE);

                }    /* for each different block count. */
            }    /* for each different lba test case. */
        }    /* for each different start pos. */
    }    /* For each different num_frus */
    return;
}

void fbe_xor_test_execute_stamps_for_lba_one_drive(fbe_block_count_t blocks, fbe_sg_element_t *sg_p)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;
    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    execute_stamps.fru[0].sg = sg_p;
    execute_stamps.fru[0].seed = 0;
    execute_stamps.fru[0].count = blocks;
    execute_stamps.fru[0].offset = 0;
    execute_stamps.fru[0].position_mask = 0;

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = 4;
    execute_stamps.eboard_p = NULL;
    execute_stamps.eregions_p = NULL;
    execute_stamps.option = FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS;
    status = fbe_xor_lib_execute_stamps(&execute_stamps);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
void fbe_xor_test_lba_stamps_one_drive(fbe_block_count_t blocks, fbe_sg_element_t *sg_p)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;

    execute_stamps.fru[0].sg = sg_p;
    execute_stamps.fru[0].seed = 0;
    execute_stamps.fru[0].count = blocks;
    status = fbe_xor_lib_check_lba_stamp(&execute_stamps);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

void fbe_xor_test_execute_stamps_for_lba_drives(fbe_block_count_t blocks, fbe_u32_t drives)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;
    fbe_u32_t fru_index;
    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    for (fru_index = 0; fru_index < drives; fru_index++)
    {
        execute_stamps.fru[fru_index].sg = &fbe_xor_test_checksum_sgs[fru_index][0];
        execute_stamps.fru[fru_index].seed = 0;
        execute_stamps.fru[fru_index].count = blocks;
        execute_stamps.fru[fru_index].offset = 0;
        execute_stamps.fru[fru_index].position_mask = (1 << fru_index);
    }
    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = drives;
    execute_stamps.array_width = FBE_XOR_MAX_FRUS;
    execute_stamps.eboard_p = NULL;
    execute_stamps.eregions_p = NULL;
    execute_stamps.option = FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS;
    status = fbe_xor_lib_execute_stamps(&execute_stamps);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
void fbe_xor_test_lba_stamps_drives(fbe_block_count_t blocks, fbe_u32_t drives)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;
    fbe_u32_t fru_index;

    for (fru_index = 0; fru_index < drives; fru_index++)
    {
        execute_stamps.fru[0].sg = &fbe_xor_test_checksum_sgs[fru_index][0];
        execute_stamps.fru[0].seed = 0;
        execute_stamps.fru[0].count = blocks;
        status = fbe_xor_lib_check_lba_stamp(&execute_stamps);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static fbe_bool_t get_cmd_option_int(char *option_name_p, fbe_u32_t *value_p)
{
    char *value = mut_get_user_option_value(option_name_p);

    if (value != NULL)
    {
        *value_p = strtoul(value, 0, 0);
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
void fbe_xor_test_execute_stamps_for_lba_test(void)
{
    fbe_u32_t iterations = 1000000;
    fbe_u32_t loop_count;
    fbe_u32_t repeat_count = 1;
    fbe_u32_t repeat_index;
    fbe_u32_t blocks = 1;
    fbe_xor_error_t eboard;

    /* Create a valid eboard
     */
    fbe_xor_init_eboard(&eboard, 0x456, 0x2000);
    if (get_cmd_option_int("-block_count", &blocks))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using block count of: %d", blocks);
    }
    if (get_cmd_option_int("-repeat_count", &repeat_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using repeat_count of: %d", repeat_count);
    }
    if (get_cmd_option_int("-iterations", &iterations))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using iterations of: %d", iterations);
    }
    fbe_xor_test_setup_sgs(1, /* num frus */
                           XOR_TEST_CHECKSUM_MAX_BLOCKS,
                           (XOR_TEST_CHECKSUM_MAX_BLOCKS / XOR_TEST_CHECKSUM_MAX_SG_ELEMENTS));

    /* Initialize the area with good lba stamps.
     */
    fbe_xor_test_execute_stamps_case(1, /* Num frus */ 0 /* start_pos */, 0, /* start lba */ blocks, &eboard,
                                                     FBE_STATUS_OK, FBE_XOR_STATUS_NO_ERROR,
                                                     FBE_FALSE, FBE_FALSE);
    
    for (repeat_index = 0; repeat_index < repeat_count; repeat_index++)
    {
        fbe_time_t start_time;
        fbe_u32_t ms_execute;
        fbe_u32_t ms_check_lba;
        fbe_u32_t ratio;

        start_time = fbe_get_time();
        for (loop_count = 0; loop_count < iterations; loop_count++)
        {
            fbe_xor_test_execute_stamps_for_lba_one_drive(blocks, &fbe_xor_test_checksum_sgs[0][0]);
        }
        ms_execute = fbe_get_elapsed_milliseconds(start_time);

        start_time = fbe_get_time();
        for (loop_count = 0; loop_count < iterations; loop_count++)
        {
            fbe_xor_test_lba_stamps_one_drive(blocks, &fbe_xor_test_checksum_sgs[0][0]);
        }
        ms_check_lba = fbe_get_elapsed_milliseconds(start_time);
        if (ms_check_lba != 0)
        {
            ratio = (ms_execute / ms_check_lba);
        }
        else
        {
            ratio = 0;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%d) %d iterations  ms_execute: %d ms_check: %d ratio: %d", repeat_index, iterations, 
                   ms_execute, ms_check_lba, ratio );
    }
}

void fbe_xor_test_execute_stamps_for_lba_drives_test(void)
{
    fbe_u32_t iterations = 1000000;
    fbe_u32_t loop_count;
    fbe_u32_t repeat_count = 1;
    fbe_u32_t repeat_index;
    fbe_u32_t blocks = 1;
    fbe_xor_error_t eboard;
    fbe_u32_t drives = 2;

    /* Create a valid eboard
     */
    fbe_xor_init_eboard(&eboard, 0x456, 0x2000);
    if (get_cmd_option_int("-block_count", &blocks))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using block count of: %d", blocks);
    }
    if (get_cmd_option_int("-data_drives", &drives))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using data drives count of: %d", drives);
    }
    if (get_cmd_option_int("-repeat_count", &repeat_count))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using repeat_count of: %d", repeat_count);
    }
    if (get_cmd_option_int("-iterations", &iterations))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "using iterations of: %d", iterations);
    }
    fbe_xor_test_setup_sgs(1, /* num frus */
                           XOR_TEST_CHECKSUM_MAX_BLOCKS,
                           (XOR_TEST_CHECKSUM_MAX_BLOCKS / XOR_TEST_CHECKSUM_MAX_SG_ELEMENTS));

    /* Initialize the area with good lba stamps.
     */
    fbe_xor_test_execute_stamps_case(16, /* Num frus */ 0 /* start_pos */, 0, /* start lba */ blocks, &eboard,
                                                     FBE_STATUS_OK, FBE_XOR_STATUS_NO_ERROR,
                                                     FBE_FALSE, FBE_FALSE);
    
    for (repeat_index = 0; repeat_index < repeat_count; repeat_index++)
    {
        fbe_time_t start_time;
        fbe_u32_t ms_execute;
        fbe_u32_t ms_check_lba;
        fbe_u32_t ratio;

        start_time = fbe_get_time();
        for (loop_count = 0; loop_count < iterations; loop_count++)
        {
            fbe_xor_test_execute_stamps_for_lba_drives(blocks, drives);
        }
        ms_execute = fbe_get_elapsed_milliseconds(start_time);

        start_time = fbe_get_time();
        for (loop_count = 0; loop_count < iterations; loop_count++)
        {
            fbe_xor_test_lba_stamps_drives(blocks, drives);
        }
        ms_check_lba = fbe_get_elapsed_milliseconds(start_time);
        if (ms_check_lba != 0)
        {
            ratio = (ms_execute / ms_check_lba);
        }
        else
        {
            ratio = 0;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%d) %d iterations  ms_execute: %d ms_check: %d ratio: %d", repeat_index, iterations, 
                   ms_execute, ms_check_lba, ratio );
    }
}
void fbe_xor_test_execute_stamps_errors(void)
{
    fbe_u32_t       num_frus = 1;
    fbe_u32_t       start_pos = 0;
    fbe_lba_t       lba = 0x1234;
    fbe_u32_t       blocks = 1;
    fbe_xor_error_t eboard;

    /* Create a valid eboard
     */
    fbe_xor_init_eboard(&eboard, 0x456, 0x2000);

    /* Error Test 1: Corrupt CRC
     */
    fbe_xor_test_execute_stamps_case(num_frus, start_pos, lba, blocks, &eboard,
                                     FBE_STATUS_OK, FBE_XOR_STATUS_CHECKSUM_ERROR,
                                     FBE_TRUE, FBE_FALSE);

    /* Error Test 2: Corrupt LBA Stamp (currently error is reported as a 
     *                                  checksum error)
     */
    fbe_xor_test_execute_stamps_case(num_frus, start_pos, lba, blocks, &eboard,
                                     FBE_STATUS_OK, FBE_XOR_STATUS_CHECKSUM_ERROR,
                                     FBE_FALSE, FBE_TRUE);

    /* Error Test 3: Corrupt eboard
     */
    fbe_xor_destroy_eboard(&eboard);
    fbe_xor_test_execute_stamps_case(num_frus, start_pos, lba, blocks, &eboard,
                                     FBE_STATUS_GENERIC_FAILURE, FBE_XOR_STATUS_INVALID,
                                     FBE_FALSE, FBE_FALSE);

    return;
}

fbe_status_t fbe_xor_test_send_execute_stamps(fbe_u32_t num_frus,
                                           fbe_u32_t start_position,
                                           fbe_sg_element_t *sg_p[],
                                           fbe_xor_option_t options,
                                           fbe_xor_error_t *eboard_p,
                                           fbe_lba_t lba,
                                           fbe_u32_t blocks,
                                           fbe_xor_status_t *xor_status_p)
{
    fbe_u32_t fru_index = 0;
    fbe_status_t status;
    fbe_u32_t position = start_position;
    fbe_xor_execute_stamps_t execute_stamps;

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /***************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/
    for (fru_index = 0; fru_index < num_frus; fru_index++)
    {
        execute_stamps.fru[fru_index].sg = sg_p[fru_index];
        execute_stamps.fru[fru_index].seed = lba;
        execute_stamps.fru[fru_index].count = blocks;
        execute_stamps.fru[fru_index].offset = 0;
        execute_stamps.fru[fru_index].position_mask = (1 << position);
        position++;
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = num_frus;
    execute_stamps.eboard_p = eboard_p;
#if 0
    if (b_check_checksum)
    {
        execute_stamps.option = FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS;
    }
    else
    {
        /* If we are not checking checksums, then we still need to check lba
         * stamps. 
         */
        execute_stamps.option = FBE_XOR_OPTION_CHK_LBA_STAMPS;
    }
#endif
    execute_stamps.option = options;

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps);

    /* Update the xor status and return the status
     */
    *xor_status_p = execute_stamps.status;
    return(status);
}
/*!***************************************************************
 * fbe_xor_test_checksum_add_tests()
 ****************************************************************
 * @brief
 *  This function adds the I/O unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  05/26/2009  Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_xor_test_checksum_add_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_execute_stamps_normal,
                 NULL,
                 NULL);
    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_execute_stamps_errors,
                 NULL,
                 NULL);
    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_execute_stamps_for_lba_test,
                 NULL,
                 NULL);
    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_execute_stamps_for_lba_drives_test,
                 NULL,
                 NULL);

    return;
}
/* end fbe_xor_test_checksum_add_tests() */
/*************************
 * end file fbe_xor_test_checksum.c
 *************************/


