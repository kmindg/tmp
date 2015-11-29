/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_xor_test_checksum.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions for the xor parity library.
 *
 * @version
 *  3/18/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_parity.h"
#include "fbe_xor_private.h"
#include "mut.h"
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#define XOR_TEST_PARITY_MAX_BLOCKS 1
#define XOR_TEST_PARITY_MAX_SG_ELEMENTS 2

static fbe_sector_t fbe_xor_test_parity_sectors[FBE_XOR_MAX_FRUS][XOR_TEST_PARITY_MAX_BLOCKS];
static fbe_sg_element_t fbe_xor_test_parity_sgs[FBE_XOR_MAX_FRUS][XOR_TEST_PARITY_MAX_SG_ELEMENTS+1];


/*!***************************************************************************
 * fbe_xor_test_mr3_case()
 *****************************************************************************
 * @brief
 *  Call the xor mr3 routines.
 *
 * @param width - total width of the group.
 * @param num_parity - Number of parity drives.
 * @param b_check_bad_memory - FBE_TRUE - Don't calculate CRC and thus expect a
 *                                  Xor status of: FBE_XOR_STATUS_BAD_MEMORY
 *                             FBE_FALSE - Generate the proper checksum for
 *                                  data being written.
 *
 * @return None.
 *
 * @author
 *  3/18/2010 - Created. Rob Foley
 *
 *****************************************************************************/

void fbe_xor_test_mr3_case(fbe_u32_t width, fbe_u32_t num_parity, fbe_bool_t b_check_bad_memory)
{
    fbe_status_t        status;
    fbe_xor_status_t    expected_xor_status = FBE_XOR_STATUS_NO_ERROR;
    fbe_u32_t           pos;
    fbe_xor_mr3_write_t mr3_write;

    fbe_status_t fbe_xor_lib_parity_mr3_write(fbe_xor_mr3_write_t *const xor_write_p);

    /* Validate width */
    if ((fbe_s32_t)(width - num_parity) < FBE_XOR_MR3_WRITE_MIN_DATA_DISKS)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s width: %d must be at least: %d greater than num_parity: %d",
                   __FUNCTION__, width, 
                   FBE_XOR_MR3_WRITE_MIN_DATA_DISKS, num_parity);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return;
    }

    for (pos = 0; pos < width; pos++)
    {
        fbe_sg_element_init(&fbe_xor_test_parity_sgs[pos][0], FBE_BE_BYTES_PER_BLOCK, &fbe_xor_test_parity_sectors[pos][0]);
        fbe_sg_element_terminate(&fbe_xor_test_parity_sgs[pos][1]);

        mr3_write.fru[pos].bitkey = 1 << pos;
        mr3_write.fru[pos].count = 1;
        mr3_write.fru[pos].data_position = pos;
        mr3_write.fru[pos].start_lda = 0;
        mr3_write.fru[pos].end_lda = 1;
        mr3_write.fru[pos].sg = &fbe_xor_test_parity_sgs[pos][0];
    }

    /*! @note The `check checksum' option must be set for parity write requests.
     */
    mr3_write.option = FBE_XOR_OPTION_CHK_CRC;
    mr3_write.data_disks = width - num_parity;
    mr3_write.parity_disks = num_parity;
    mr3_write.status = FBE_XOR_STATUS_INVALID;

    mr3_write.eboard_p = NULL;
    mr3_write.eregions_p = NULL;
    mr3_write.offset = 0;

    /* Set the expected Xor status
     */
    if (b_check_bad_memory == FBE_TRUE)
    {
        expected_xor_status = FBE_XOR_STATUS_BAD_MEMORY;
    }

    /*! @note We don't expect any errors.
     */
    status = fbe_xor_lib_parity_mr3_write(&mr3_write);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*! @note Validate the Xor status
     */
    MUT_ASSERT_INT_EQUAL(mr3_write.status, expected_xor_status);
    return;
}
/******************************************
 * end fbe_xor_test_mr3_case()
 ******************************************/
/*!**************************************************************
 * fbe_xor_test_display_stamps()
 ****************************************************************
 * @brief
 *  Display all the stamps for the sectors.  Used when we 
 *  are trying to debug.
 *
 * @param num_frus - total width of the group.
 *
 * @return None.
 *
 * @author
 *  3/18/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_display_stamps(fbe_u32_t num_frus)
{
    fbe_sector_t *sector_p = NULL;
    fbe_u32_t pos;

    for (pos = 0; pos < num_frus - 2; pos++)
    {
        sector_p = fbe_xor_test_parity_sectors[pos];

        mut_printf(MUT_LOG_TEST_STATUS,
                   "xor data %d) stamps: 0x%08x 0x%08x 0x%08x 0x%08x",
                   pos,
                   sector_p->time_stamp, sector_p->crc, 
                   sector_p->lba_stamp, sector_p->write_stamp);
    }
    sector_p = fbe_xor_test_parity_sectors[num_frus - 2];
    mut_printf(MUT_LOG_TEST_STATUS,
               "xor row parity stamps: 0x%08x 0x%08x 0x%08x 0x%08x",
               sector_p->time_stamp, sector_p->crc, sector_p->lba_stamp, sector_p->write_stamp);

    sector_p = fbe_xor_test_parity_sectors[num_frus - 1];
    mut_printf(MUT_LOG_TEST_STATUS,
               "xor diag parity stamps: 0x%08x 0x%08x 0x%08x 0x%08x",
               sector_p->time_stamp, sector_p->crc, sector_p->lba_stamp, sector_p->write_stamp);
}
/******************************************
 * end fbe_xor_test_display_stamps()
 ******************************************/

/*!**************************************************************
 * fbe_xor_test_mr3_write()
 ****************************************************************
 * @brief
 *  Loop over all the mr3 test cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/18/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_xor_test_mr3_write(void)
{
    fbe_u32_t word;
    fbe_u32_t pos;
    fbe_u32_t width;
    fbe_u16_t crc;

    /* Fill the block with a random pattern and good crc
     */
    for (word = 0; word < FBE_WORDS_PER_BLOCK; word++)
    {
        for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
        {
            fbe_xor_test_parity_sectors[pos][0].data_word[word] = fbe_random();
        }
    }
    for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
    {
         crc = fbe_xor_lib_calculate_checksum((fbe_u32_t*)&fbe_xor_test_parity_sectors[pos][0]);
         fbe_xor_test_parity_sectors[pos][0].crc = crc;
    }

    /* Now execute the test case with random data and good crc.
     */
    for (width = FBE_PARITY_R5_MIN_WIDTH; width <= FBE_PARITY_MAX_WIDTH; width++)
    {
        fbe_xor_test_mr3_case(width, 1 /* r5 has 1 parity */, FBE_FALSE /* Check valid crc*/);
        if (width >= FBE_PARITY_R6_MIN_WIDTH)
        {
            fbe_xor_test_mr3_case(width, 2 /* r6 has 2 parity */, FBE_FALSE /* Check valid crc*/);
        }
    }

    /* Fill the block with a fixed pattern.
     */
    for (word = 0; word < FBE_WORDS_PER_BLOCK; word++)
    {
        for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
        {
            fbe_xor_test_parity_sectors[pos][0].data_word[word] = 0xFF;
        }
    }
    for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
    {
         crc = fbe_xor_lib_calculate_checksum((fbe_u32_t*)&fbe_xor_test_parity_sectors[pos][0]);
         fbe_xor_test_parity_sectors[pos][0].crc = crc;
    }
    /* Now execute the test case with a fixed pattern and good crc.
     */
    for (width = FBE_PARITY_R5_MIN_WIDTH; width <= FBE_PARITY_MAX_WIDTH; width++)
    {
        fbe_xor_test_mr3_case(width, 1 /* r5 has 1 parity */, FBE_FALSE /* Check valid crc*/);
        if (width >= FBE_PARITY_R6_MIN_WIDTH)
        {
            fbe_xor_test_mr3_case(width, 2 /* r6 has 2 parity */, FBE_FALSE /* Check valid crc*/);
        }
    }

    /* Fill the block with a fixed pattern and then corrupt crc
     */
    for (word = 0; word < FBE_WORDS_PER_BLOCK; word++)
    {
        for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
        {
            fbe_xor_test_parity_sectors[pos][0].data_word[word] = 0xBADC4C;
        }
    }
    for (pos = 0; pos < FBE_XOR_MAX_FRUS; pos++)
    {
        /* Now corrupt the crc
         */
        xorlib_invalidate_checksum((void *)&fbe_xor_test_parity_sectors[pos][0]);
    }
    /* Now execute the test case with a fixed pattern and bad crc.
     */
    for (width = FBE_PARITY_R5_MIN_WIDTH; width <= FBE_PARITY_MAX_WIDTH; width++)
    {
        fbe_xor_test_mr3_case(width, 1 /* r5 has 1 parity */, FBE_TRUE /* Check for bad crc*/);
        if (width >= FBE_PARITY_R6_MIN_WIDTH)
        {
            fbe_xor_test_mr3_case(width, 2 /* r6 has 2 parity */, FBE_TRUE /* Check for bad crc*/);
        }
    }

    return;
}
/******************************************
 * end fbe_xor_test_mr3_write()
 ******************************************/
/*!***************************************************************
 * fbe_xor_test_parity_add_tests()
 ****************************************************************
 * @brief
 *  This function adds the xor parity unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  3/18/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_test_parity_add_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);


    MUT_ADD_TEST(suite_p, 
                 fbe_xor_test_mr3_write,
                 NULL,
                 NULL);

    return;
}
/* end fbe_xor_test_parity_add_tests() */
/*************************
 * end file fbe_xor_test_checksum.c
 *************************/


