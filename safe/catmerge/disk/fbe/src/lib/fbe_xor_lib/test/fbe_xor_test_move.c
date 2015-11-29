/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_xor_test_move.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions for the xor checksum library.
 *
 * @version
 *  1/17/2011 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_xor_api.h"
#include "fbe_xor_private.h"
#include "mut.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_api_common_transport.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def XOR_TEST_MOVE_MAX_BLOCKS
 *********************************************************************
 * @brief Number of blocks we will move.
 *
 *********************************************************************/
#define XOR_TEST_MOVE_MAX_BLOCKS 64

/*!*******************************************************************
 * @def XOR_TEST_MOVE_MAX_SG_ELEMENTS
 *********************************************************************
 * @brief Length of sg list we will use.
 *
 *********************************************************************/
#define XOR_TEST_MOVE_MAX_SG_ELEMENTS 2

/*!*******************************************************************
 * @def XOR_TEST_SG_LIST_COUNT
 *********************************************************************
 * @brief Number of sg lists to allocate.  We allocate one for
 *        each possible fru.
 *
 *********************************************************************/
#define XOR_TEST_SG_LIST_COUNT FBE_XOR_MAX_FRUS

/*!*******************************************************************
 * @def XOR_TEST_SECTOR_COUNT
 *********************************************************************
 * @brief Number of blocks we will allocate in total.
 *
 *********************************************************************/
#define XOR_TEST_SECTOR_COUNT (FBE_XOR_MAX_FRUS * XOR_TEST_MOVE_MAX_BLOCKS + 1)

/*!*******************************************************************
 * @var fbe_xor_test_move_lbas
 *********************************************************************
 * @brief These are the lba cases we will test for. 
 *
 *********************************************************************/
static fbe_lba_t fbe_xor_test_move_lbas[] = 
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

    /* Right now we do not test with lbas beyond 48 bits since the data pattern library 
     * doesn't support it for lba pass data pattern. 
     */

    /* Terminator tells the test this is the end of the test cases.
     */
    0xFFFFFFFFFFFFFFFF,
};
/* end fbe_xor_test_move_lbas */

/*!***************************************************************
 *          fbe_xor_test_move_setup_sg_list()
 ****************************************************************
 * @brief
 *  This function sets up an sg list based on the input parameters.
 *
 * @param sg_p - Sg list to setup.
 * @param memory_p - Pointer to memory to use in sg.
 * @param bytes - Bytes to setup in sg.
 * @param max_bytes_per_sg - Maximum number of bytes per sg entry.
 *                           Allows us to setup longer sg lists.
 * @param lba - Start lba for sg list.
 * @param block_size - Size of each block in the sg.
 *
 * @return
 *  None.
 *
 * @author:
 *  1/17/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_test_move_setup_sg_list(fbe_sg_element_t *const sg_p,
                                     fbe_u8_t *memory_p,
                                     fbe_u32_t bytes,
                                     fbe_u32_t max_bytes_per_sg,
                                     fbe_lba_t lba,
                                     fbe_block_size_t block_size,
                                     fbe_data_pattern_t data_pattern)
{
    fbe_u32_t current_bytes;
    fbe_u32_t bytes_remaining = bytes;
    fbe_sg_element_t *current_sg_p = sg_p;
    fbe_data_pattern_info_t data_pattern_info = {0};
    fbe_u64_t  header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_status_t status;
    fbe_u32_t pass_count = 0xFF;
    
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

        /* fill data pattern information structure
         */
        header_array[0] = FBE_U32_MAX;
        header_array[1] = FBE_OBJECT_ID_INVALID;
        status = fbe_data_pattern_build_info(&data_pattern_info,
                                             data_pattern,
                                             0, /* No special data pattern flags */
                                             lba,
                                             pass_count,
                                             2,    /*num_header_words*/
                                             header_array);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK );

        status = fbe_data_pattern_fill_sg_list(current_sg_p,
                                               &data_pattern_info,
                                               block_size,
                                               0, /* Corrupted blocks bitmap. */
                                               XOR_TEST_MOVE_MAX_SG_ELEMENTS /* Max sg elements */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK );
        fbe_sg_element_increment(&current_sg_p);
        memory_p += current_bytes;
        bytes_remaining -= current_bytes;

    } /* end while bytes remaining != 0 */

    /* Terminate the list.
     */
    fbe_sg_element_terminate(current_sg_p);
    return;
}
/**************************************
 * end fbe_xor_test_move_setup_sg_list()
 **************************************/
/*!**************************************************************
 * fbe_xor_test_move_check_sgs()
 ****************************************************************
 * @brief
 *  Check the sg list passed in for a pattern.
 *
 * @param sg_p - Scatter gather list to check.
 * @param blocks - Number of blocks to check in list.
 * @param max_bytes_per_sg - Max we allow in an sg list.
 * @param lba - Logical block to use as the pattern.
 * @param block_size - Size of the blocks in bytes we are checking.
 *
 * @return None.
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_test_move_check_sgs(fbe_sg_element_t *const sg_p,
                                 fbe_block_count_t blocks,
                                 fbe_u32_t max_bytes_per_sg,
                                 fbe_lba_t lba,
                                 fbe_block_size_t block_size)
{

    fbe_data_pattern_info_t data_pattern_info = {0};
    fbe_u64_t  header_array[FBE_DATA_PATTERN_MAX_HEADER_PATTERN];
    fbe_status_t status;
    fbe_data_pattern_t data_pattern = FBE_DATA_PATTERN_LBA_PASS;
    /*
     * fill data pattern information structure
     */
    header_array[0] = FBE_U32_MAX;
    header_array[1] = FBE_OBJECT_ID_INVALID;
    status = fbe_data_pattern_build_info(&data_pattern_info,
                                         data_pattern,
                                         0, /* No special data pattern flags */
                                         lba,
                                         0xFF,
                                         2, /*num_header_words*/
                                         header_array);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_data_pattern_check_sg_list(sg_p,
                                            &data_pattern_info,
                                            block_size,
                                            0,    /* no corrupt bitmask */
                                            FBE_OBJECT_ID_INVALID,
                                            XOR_TEST_MOVE_MAX_SG_ELEMENTS,
                                            FBE_FALSE    /* do not panic on data miscompare */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_xor_test_move_check_sgs()
 ******************************************/
/*!**************************************************************
 * fbe_xor_test_move_520_512()
 ****************************************************************
 * @brief
 *  This is a single test for a 520 -> 512 move.
 *
 * @param num_frus - Number of frus to move for.
 * @param lba - Lba to move for each fru.
 * @param blocks - Number of blocks for each fru.
 * @param source_p - Ptr to source memory.
 * @param dest_p - Ptr to destination memory.
 * @param src_sg_p - Ptr to source sg.
 * @param dest_sg_p - Ptr to destination sg.
 *
 * @return None.
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_move_520_512(fbe_u32_t num_frus,
                               fbe_lba_t lba,
                               fbe_u32_t blocks,
                               fbe_sector_t *source_p,
                               fbe_sector_t *dest_p,
                               fbe_sg_element_t *src_sg_p,
                               fbe_sg_element_t *dest_sg_p)
{
    fbe_status_t status;
	fbe_xor_mem_move_t xor_mem_move;

    /* We only allocate XOR_TEST_MOVE_MAX_BLOCKS total blocks to test with.
     */
    MUT_ASSERT_TRUE(blocks <= XOR_TEST_MOVE_MAX_BLOCKS);

    /* Setup the move operation.
     * We will move from 520 to 512. 
     * The source 520 sg is filled with lba seeded pattern. 
     * The dest 512 sg is filled with zeros. 
     * When the move is complete, we check the dest 512 
     * sg for the seeded pattern. 
     */
	fbe_zero_memory(&xor_mem_move, sizeof(xor_mem_move));
	xor_mem_move.status = FBE_XOR_STATUS_NO_ERROR;
    /* Currently we don't validate the checksum.  I.E. We don't set 
     * FBE_XOR_OPTION_CHK_CRC.
     */
    xor_mem_move.option = 0;
	xor_mem_move.disks = num_frus;
    xor_mem_move.fru[0].count     = blocks;
    fbe_xor_test_move_setup_sg_list(src_sg_p,
                                    (fbe_u8_t*)source_p,
                                    (blocks * FBE_BE_BYTES_PER_BLOCK),
                                    (XOR_TEST_MOVE_MAX_BLOCKS * FBE_BE_BYTES_PER_BLOCK),
                                    lba,
                                    FBE_BE_BYTES_PER_BLOCK,
                                    FBE_DATA_PATTERN_LBA_PASS);
    fbe_xor_test_move_setup_sg_list(dest_sg_p,
                                    (fbe_u8_t*)dest_p,
                                    (blocks * FBE_BYTES_PER_BLOCK),
                                    (XOR_TEST_MOVE_MAX_BLOCKS * FBE_BYTES_PER_BLOCK),
                                    lba,
                                    FBE_BYTES_PER_BLOCK,
                                    FBE_DATA_PATTERN_ZEROS);

    xor_mem_move.fru[0].source_sg_p = src_sg_p;
    xor_mem_move.fru[0].dest_sg_p = dest_sg_p;
	
	status = fbe_xor_mem_move(&xor_mem_move,FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM512,
                              0,1,0,0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_XOR_STATUS_NO_ERROR, xor_mem_move.status);

    /* Check to make sure the pattern from the 520 sg ended up in the 512 sg.
     */
    fbe_xor_test_move_check_sgs(xor_mem_move.fru[0].dest_sg_p,
                                blocks,
                                (XOR_TEST_MOVE_MAX_BLOCKS * FBE_BE_BYTES_PER_BLOCK),
                                lba,
                                FBE_BYTES_PER_BLOCK);
    return;
}
/******************************************
 * end fbe_xor_test_move_520_512()
 ******************************************/

/*!**************************************************************
 * fbe_xor_test_move_512_520()
 ****************************************************************
 * @brief
 *  Test a memory move from 512 -> 520.
 *
 * @param num_frus - Number of frus to move for.
 * @param lba - Lba to move for each fru.
 * @param blocks - Number of blocks for each fru.
 * @param source_p - Ptr to source memory.
 * @param dest_p - Ptr to destination memory.
 * @param src_sg_p - Ptr to source sg.
 * @param dest_sg_p - Ptr to destination sg.
 *
 * @return None.
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_move_512_520(fbe_u32_t num_frus,
                               fbe_lba_t lba,
                               fbe_u32_t blocks,
                               fbe_sector_t *source_p,
                               fbe_sector_t *dest_p,
                               fbe_sg_element_t *src_sg_p,
                               fbe_sg_element_t *dest_sg_p)
{
    fbe_status_t status;
	fbe_xor_mem_move_t xor_mem_move;

    /* We only allocate XOR_TEST_MOVE_MAX_BLOCKS total blocks to test with.
     */
    MUT_ASSERT_TRUE(blocks <= XOR_TEST_MOVE_MAX_BLOCKS);

    /* Setup the move operation.
     * We will move from 512 to 520. 
     * The 512 sg is filled with lba seeded pattern. 
     * The 520 sg is filled with zeros. 
     * When the move is complete, we check the 520 sg for 
     * the seeded pattern. 
     */
	fbe_zero_memory(&xor_mem_move, sizeof(xor_mem_move));
	xor_mem_move.status = FBE_XOR_STATUS_NO_ERROR;
    /* Currently we don't validate the checksum.  I.E. We don't set 
     * FBE_XOR_OPTION_CHK_CRC.
     */
    xor_mem_move.option = 0;
	xor_mem_move.disks = num_frus;
    xor_mem_move.fru[0].count     = blocks;
    fbe_xor_test_move_setup_sg_list(src_sg_p,
                                    (fbe_u8_t*)source_p,
                                    (blocks * FBE_BYTES_PER_BLOCK),
                                    (XOR_TEST_MOVE_MAX_BLOCKS * FBE_BYTES_PER_BLOCK),
                                    lba,
                                    FBE_BYTES_PER_BLOCK,
                                    FBE_DATA_PATTERN_LBA_PASS);
    fbe_xor_test_move_setup_sg_list(dest_sg_p,
                                    (fbe_u8_t*)dest_p,
                                    (blocks * FBE_BE_BYTES_PER_BLOCK),
                                    (XOR_TEST_MOVE_MAX_BLOCKS * FBE_BE_BYTES_PER_BLOCK),
                                    lba,
                                    FBE_BE_BYTES_PER_BLOCK,
                                    FBE_DATA_PATTERN_ZEROS);

    xor_mem_move.fru[0].source_sg_p = src_sg_p;
    xor_mem_move.fru[0].dest_sg_p = dest_sg_p;
	
	status = fbe_xor_mem_move(&xor_mem_move,FBE_XOR_MOVE_COMMAND_MEM512_TO_MEM520,
                              0,1,0,0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_XOR_STATUS_NO_ERROR, xor_mem_move.status);

    /* Check to make sure the pattern from the 512 sg ended up in the 
     * 520 sg. 
     */
    fbe_xor_test_move_check_sgs(xor_mem_move.fru[0].dest_sg_p,
                                blocks,
                                (XOR_TEST_MOVE_MAX_BLOCKS * FBE_BE_BYTES_PER_BLOCK),
                                lba,
                                FBE_BE_BYTES_PER_BLOCK);
    return;
}
/******************************************
 * end fbe_xor_test_move_512_520()
 ******************************************/

/*!**************************************************************
 * fbe_xor_test_move_case()
 ****************************************************************
 * @brief
 *  This tests a single case of memory moves.
 *  We will test both a 512 -> 520 move and a 520 -> 512 move.
 *
 * @param num_frus - Number of frus to move for.
 * @param lba - Lba to move for each fru.
 * @param blocks - Number of blocks for each fru.
 * @param source_p - Ptr to source memory.
 * @param dest_p - Ptr to destination memory.
 * @param src_sg_p - Ptr to source sg.
 * @param dest_sg_p - Ptr to destination sg.
 *
 * @return None.
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_move_case(fbe_u32_t num_frus,
                            fbe_lba_t lba,
                            fbe_u32_t blocks,
                            fbe_sector_t *source_p,
                            fbe_sector_t *dest_p,
                            fbe_sg_element_t *src_sg_p,
                            fbe_sg_element_t *dest_sg_p)
{

    /* We only allocate XOR_TEST_MOVE_MAX_BLOCKS total blocks to test with.
     */
    MUT_ASSERT_TRUE(blocks <= XOR_TEST_MOVE_MAX_BLOCKS);

    /* Test a 512 -> 520 move.
     */
    fbe_xor_test_move_512_520(num_frus, lba, blocks,
                              source_p, dest_p, src_sg_p, dest_sg_p);

    /* Test a 520 -> 512 move.
     */
    fbe_xor_test_move_520_512(num_frus, lba, blocks,
                              source_p, dest_p, src_sg_p, dest_sg_p);

    return;
}
/******************************************
 * end fbe_xor_test_move_case()
 ******************************************/

/*!**************************************************************
 * fbe_xor_test_move()
 ****************************************************************
 * @brief
 *  Unit test the memory move code.
 *  We will loop over various parameters to fully test this code.
 *
 * @param None.              
 *
 * @return None. 
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_move(void)
{
    fbe_u32_t       num_frus;
    fbe_lba_t       lba;
    fbe_u32_t       lba_index;
    fbe_u32_t       blocks;
    fbe_xor_error_t eboard;
    fbe_u8_t *src_p = NULL;
    fbe_u8_t *dest_p = NULL;
    fbe_sg_element_t *src_sg_p = NULL;
    fbe_sg_element_t *dest_sg_p = NULL;
    /* Create a valid eboard
     */
    fbe_xor_init_eboard(&eboard, 0x123, 0x1000);

    src_p = malloc(XOR_TEST_SECTOR_COUNT * sizeof(fbe_sector_t));
    dest_p = malloc(XOR_TEST_SECTOR_COUNT * sizeof(fbe_sector_t));

    src_sg_p = malloc(XOR_TEST_SG_LIST_COUNT * XOR_TEST_MOVE_MAX_SG_ELEMENTS * 
                                                  sizeof(fbe_sg_element_t));
    dest_sg_p = malloc(XOR_TEST_SG_LIST_COUNT * XOR_TEST_MOVE_MAX_SG_ELEMENTS * 
                                                   sizeof(fbe_sg_element_t));

    MUT_ASSERT_TRUE(src_p != NULL);
    MUT_ASSERT_TRUE(dest_p != NULL);
    MUT_ASSERT_TRUE(dest_sg_p != NULL);
    MUT_ASSERT_TRUE(src_sg_p != NULL);

    fbe_zero_memory(src_sg_p, XOR_TEST_SG_LIST_COUNT * XOR_TEST_MOVE_MAX_SG_ELEMENTS * 
                                                  sizeof(fbe_sg_element_t));
    fbe_zero_memory(dest_sg_p, XOR_TEST_SG_LIST_COUNT * XOR_TEST_MOVE_MAX_SG_ELEMENTS * 
                                                  sizeof(fbe_sg_element_t));

    /* Perform one test case for each number of frus.
     */
    for (num_frus = 1; num_frus <= 1; num_frus++)
    {
        lba = 0;
        for (lba_index = 0; (lba != -1); lba_index++)
        {
            lba = fbe_xor_test_move_lbas[lba_index];

            for (blocks = 1; blocks <= XOR_TEST_MOVE_MAX_BLOCKS; blocks += 8)
            {
                fbe_u32_t source_offset;
                fbe_u32_t dest_offset;
#define XOR_TEST_MOVE_MAX_OFFSET 32
#define XOR_TEST_MOVE_OFFSET 8

                /* Perform the moves with different offsets.
                 */
                for (source_offset = 0; source_offset < XOR_TEST_MOVE_MAX_OFFSET; source_offset += XOR_TEST_MOVE_OFFSET)
                {

                    for (dest_offset = 0; dest_offset < XOR_TEST_MOVE_MAX_OFFSET; dest_offset += XOR_TEST_MOVE_OFFSET)
                    {
                        fbe_xor_test_move_case(num_frus, lba, blocks, 
                                               (fbe_sector_t*)(src_p + source_offset),
                                               (fbe_sector_t*)(dest_p + dest_offset),
                                               src_sg_p,
                                               dest_sg_p);
                    }
                }
            }    /* for each different block count. */
        }    /* for each different lba test case. */
    }    /* For each different num_frus */

    free(src_p);
    free(dest_p);
    free(src_sg_p);
    free(dest_sg_p);
    
    return;
}
/******************************************
 * end fbe_xor_test_move()
 ******************************************/
/*!**************************************************************
 * fbe_xor_test_move_setup()
 ****************************************************************
 * @brief
 *  This is our unit test setup function.
 *  Just get the xor library initialized.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_move_setup(void)
{
    fbe_xor_library_init();
}
/******************************************
 * end fbe_xor_test_move_setup()
 ******************************************/
/*!**************************************************************
 * fbe_xor_test_move_teardown()
 ****************************************************************
 * @brief
 *  Destroy function for our unit test.
 *  Just destroy the library we initialized during the setup.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_move_teardown(void)
{
    fbe_xor_library_destroy();
}
/******************************************
 * end fbe_xor_test_move_teardown()
 ******************************************/
/*!***************************************************************
 * fbe_xor_test_move_add_tests()
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
 *  1/17/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_test_move_add_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_move,
                 fbe_xor_test_move_setup,
                 fbe_xor_test_move_teardown);

    return;
}
/* end fbe_xor_test_move_add_tests() */
/*************************
 * end file fbe_xor_test_move.c
 *************************/


