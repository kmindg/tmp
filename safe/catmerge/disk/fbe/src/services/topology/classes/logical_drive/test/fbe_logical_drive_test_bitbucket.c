/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_bitbucket.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for testing the logical drive's bitbucket
 *  functionality.
 * 
 *  All the functions used for the logical drive's bitbucket are tested here.
 * 
 *
 * HISTORY
 *   6/24/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!**************************************************************
 * @fn fbe_ldo_test_bitbucket_init()
 ****************************************************************
 * @brief
 *  This function tests the bitbucket init function.
 *
 * @param - none
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  06/24/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_bitbucket_init(void)
{
    fbe_ldo_bitbucket_t *bitbucket_p = fbe_ldo_get_write_bitbucket_ptr();
    fbe_u32_t bitbucket_index;

    /* Set all the bytes in the bitbucket to non-zero.
     */
    for ( bitbucket_index = 0; 
          bitbucket_index < fbe_ldo_get_bitbucket_bytes(); 
          bitbucket_index++ )
    {
        bitbucket_p->bytes[bitbucket_index] = 0xFF;
    }

    /* Init the bitbucket.
     */
    fbe_ldo_bitbucket_init();

    /* Make sure the bitbucket is really zeroed.
     * Test all bytes. 
     */
    for ( bitbucket_index = 0;
          bitbucket_index < fbe_ldo_get_bitbucket_bytes();
          bitbucket_index++ )
    {
        MUT_ASSERT_INT_EQUAL(bitbucket_p->bytes[bitbucket_index], 0);
    }

    /* Make sure that the read bitbucket is sane.
     */
    MUT_ASSERT_NOT_NULL_MSG(fbe_ldo_get_read_bitbucket_ptr(), 
                            "LDO read bitbucket must not be null");
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_bitbucket_init() */

/*!**************************************************************
 * @fn fbe_ldo_test_fill_sg_with_bitbucket(void)
 ****************************************************************
 * @brief
 *  This function executes tests the bitbucket fill sg function.
 *
 * @param - none
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  06/24/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_fill_sg_with_bitbucket(void)
{
    fbe_ldo_bitbucket_t *bitbucket_p = 
        fbe_ldo_get_write_bitbucket_ptr();
    fbe_sg_element_t sg[20];
    fbe_u32_t bitbucket_bytes = fbe_ldo_get_bitbucket_bytes();
    fbe_u8_t memory = 0;

    /* Fill out the sg list with something, it should not get changed for these
     * first tests. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_terminate(&sg[1]);

    /* Test all combinations where one of the sg or bitbucket is null.
     */
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(NULL, 0, NULL) == NULL);
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(NULL, 1, NULL) == NULL);
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(NULL, 0, bitbucket_p) == NULL);
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(NULL, 1, bitbucket_p) == NULL);
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, 0, NULL) == NULL);
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, 1, NULL) == NULL);

    /* Make sure the first few cases did not change the sg.
     */
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == 20);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == &memory);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == 0);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == NULL);

    /* Test case where zero is passed in for byte count. 
     * Sg should be unchanged and NULL should be returned. 
     */
    fbe_sg_element_terminate(sg);
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, 0, bitbucket_p) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == 0);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == NULL);

    /* Init the sg for the next test. Make a 1 element sg list that contains 20
     * bytes. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_terminate(&sg[1]);

    /* Now test the combinations where the sg gets filled in with one byte.
     * In this case we should end up with one sg entry with one byte pointing to 
     * the bitbucket. 
     */
    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, 1, bitbucket_p) == &sg[1]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == 1);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == 0);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == NULL);

    /* The next tests assume the bitbucket is
     * more than one byte, so assert it now.
     */ 
    MUT_ASSERT_TRUE(fbe_ldo_get_bitbucket_bytes() > 1);

    /* Init the sg for the next test where we will test the case where there are
     * more than one but less than bitbucket bytes.  We init one sg element 
     * since we expect one sg element to be modified.  In this case we should 
     * end up with one sg entry with 2 bytes pointing to the bitbucket. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_terminate(&sg[1]);

    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, 2, bitbucket_p) == &sg[1]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == 2);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == 0);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == NULL);

    /* Init sg for the test where there are bitbucket bytes in the sg.
     * In this case we should end up with one sg entry with bitbucket bytes 
     * pointing to the bitbucket. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_terminate(&sg[1]);

    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, bitbucket_bytes, bitbucket_p) == &sg[1]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == bitbucket_bytes);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == 0);

    /* Init sg for the test where there are bitbucket bytes - 1 in the sg.
     * In this case we should end up with one sg entry with bitbucket bytes - 1
     * pointing to the bitbucket. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_terminate(&sg[1]);

    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, bitbucket_bytes - 1, bitbucket_p) == &sg[1]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == bitbucket_bytes - 1);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == 0);

    /* Init the sg for the next test where there are bitbucket bytes + 1 in the
     * sg. 
     * In this case we should end up with two sg entries with bitbucket bytes + 
     * 1 pointing to the bitbucket. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_init(&sg[1], 20, &memory);
    fbe_sg_element_terminate(&sg[2]);

    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, bitbucket_bytes+1, bitbucket_p) == &sg[2]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == bitbucket_bytes);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == 1);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[2]) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[2]) == 0);

    /* Init the sg for the next test where there are 2 * bitbucket bytes in the
     * sg. 
     * In this case we should end up with two sg entries with bitbucket bytes * 2
     * pointing to the bitbucket. 
     */
    fbe_sg_element_init(sg, 20, &memory);
    fbe_sg_element_init(&sg[1], 20, &memory);
    fbe_sg_element_terminate(&sg[2]);

    MUT_ASSERT_TRUE(fbe_ldo_fill_sg_with_bitbucket(sg, bitbucket_bytes*2, bitbucket_p) == &sg[2]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg) == bitbucket_bytes);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[1]) == bitbucket_bytes);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[1]) == (fbe_u8_t*)bitbucket_p);
    MUT_ASSERT_TRUE(fbe_sg_element_address(&sg[2]) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_count(&sg[2]) == 0);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_fill_sg_with_bitbucket */

/*!**************************************************************
 * @fn fbe_ldo_test_bitbucket_get_sg_entries()
 ****************************************************************
 * @brief
 *  This function executes the tests of the
 *  fbe_ldo_bitbucket_get_sg_entries() function.
 *
 * @param  - None
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  06/24/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_bitbucket_get_sg_entries(void)
{
    /* Make sure that for zero bytes we count zero sgs.
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_bitbucket_get_sg_entries(0), 0);

    /* Make sure that for 1 byte, we count 1 sg, since 1 byte should fit inside
     * the bitbucket.
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_bitbucket_get_sg_entries(1), 1);

    /* Make sure that for two bytes we also count 1 sg, again since two bytes
     * should fit inside the bitbucket. 
     * We also make sure that the bitbucket is greater than 2 bytes so that 2 
     * bitbucket bytes can fit in 1 sg. 
     */
    MUT_ASSERT_TRUE(fbe_ldo_get_bitbucket_bytes() > 2);
    MUT_ASSERT_INT_EQUAL(fbe_ldo_bitbucket_get_sg_entries(2), 1);

    /* Make sure that for bitbucket bytes we also count 1 sg, since this is the
     * max amount that will fit in 1 bitbucket. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_bitbucket_get_sg_entries(fbe_ldo_get_bitbucket_bytes()), 1);

    /* Make sure that for bitbucket bytes + 1 we count 2 sgs, since this should
     * be split into 2 bitbuckets. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_bitbucket_get_sg_entries(fbe_ldo_get_bitbucket_bytes() + 1), 2);

     /* Make sure that for bitbucket bytes * 2 we count 2 sgs, since this should
     * be split into 2 bitbuckets. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_bitbucket_get_sg_entries(fbe_ldo_get_bitbucket_bytes() * 2), 2);
    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_bitbucket_get_sg_entries */

/*!***************************************************************
 * @fn fbe_ldo_test_add_bitbucket_tests(mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  This function adds the logical drive bitbucket tests to
 *  the given input test suite.
 *
 * @param suite_p - the suite to add bitbucket tests to.
 *
 * @return None.
 *
 * HISTORY:
 *  08/19/08 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_bitbucket_tests(mut_testsuite_t * const suite_p)
{
    /* Simply setup the bitbucket tests in this suite.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_bitbucket_init,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_fill_sg_with_bitbucket,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_bitbucket_get_sg_entries,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/* end fbe_ldo_test_add_bitbucket_tests() */
/*************************
 * end file fbe_logical_drive_test_bitbucket.c
 *************************/


