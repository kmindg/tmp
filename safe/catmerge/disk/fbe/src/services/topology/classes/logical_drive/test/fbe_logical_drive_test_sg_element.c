/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_sg_element.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions for sg elements.
 *  The goal is to test all the functions in fbe_sg_element.c and .h
 *
 * HISTORY
 *   6/10/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_sg_element.h"
#include "mut.h"

/*************************
 *   LOCAL DEFINITIONS
 *************************/

/* @def FBE_TEST_MAX_SG_ELEMENTS 
 * This is the max length of an sg list that we will test. This is based on 
 * the fact that flare sg elements are a max of 128. 
 *  We start with something a bit larger than 128 for our testing
 */
#define FBE_TEST_MAX_SG_ELEMENTS 135

/* @def FBE_TEST_MAX_BYTES_PER_SG
 * This is the max number of bytes per entry of the sg we will test. 
 * This is arbitrary, but limits the size of each sg list. 
 * Since we will be testing every possible offset and count for an sg list, a 
 * relatively small max bytes per sg element limits the number of iterations 
 * required for testing. 
 */
#define FBE_TEST_MAX_BYTES_PER_SG 5

/* This is our static test array of sg entries.
 * +1 for terminator 
 */
static fbe_sg_element_t fbe_test_sg_element_sg_array[FBE_TEST_MAX_SG_ELEMENTS + 1];

/* This is our static test array of sg entries for the destination when we are
 * testing copying sg lists. 
 * +1 for terminator 
 */
static fbe_sg_element_t fbe_test_sg_element_sg_array_dest[FBE_TEST_MAX_SG_ELEMENTS + 1];

/* This is the static memory we use to allocate for sg lists. 
 * It needs to have as much memory as our longest sg list under test. 
 */
static fbe_u8_t fbe_test_sg_element_sg_memory[FBE_TEST_MAX_BYTES_PER_SG * FBE_TEST_MAX_SG_ELEMENTS];

static void 
fbe_test_sg_element_validate_sg_entries(fbe_sg_element_t *const sg_p,
                                        const fbe_u32_t max_bytes_per_entry );
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!***************************************************************
 * @fn fbe_test_sg_element_count()
 ****************************************************************
 * @brief
 *  This is the unit test for sg element counting.
 *  The goal here is to test these functions:
 *      fbe_sg_element_count_entries()
 *      fbe_sg_element_count_list_bytes()
 *      fbe_sg_element_count_entries_with_offset()
 *
 * @param - None.
 *
 * @return - None.
 *
 * HISTORY:
 *  6/10/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_u32_t
fbe_test_sg_element_count(void)
{
    fbe_sg_element_t *sg_p = fbe_test_sg_element_sg_array;
    fbe_u32_t sg_entries;

    /* First make sure that an obviously empty list returns 0 entries.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(NULL), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(NULL), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(NULL, 5, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(NULL, 1, 4), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(NULL, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(NULL, 1), 0);

    /* Next, init the sg list to have one entry with a terminator, and
     * make sure it also comes back with a length of 0. 
     */
    fbe_sg_element_terminate(sg_p);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(sg_p), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(sg_p, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 5, 0), 0);

    /* Next, init the sg list to have one entry with a count, but a null
     * address, and make sure it also comes back with a length of 0. 
     */
    fbe_sg_element_init(sg_p, 5, NULL);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(sg_p), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(sg_p, 1), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 5, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 2), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 5, 2), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 0, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 0, 2), 0);

    /* Even if the offset is non-zero, we should get back a zero since there are 0 bytes
     * in the sg. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 4), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 5), 0);

    /* Loop over all possible sg list lengths.
     */
    for (sg_entries = 1; sg_entries < FBE_TEST_MAX_SG_ELEMENTS; sg_entries++)
    {
        fbe_u32_t current_sg_entries;
        fbe_u32_t max_bytes_per_sg_entry = FBE_TEST_MAX_BYTES_PER_SG;
        fbe_u8_t *memory_p = fbe_test_sg_element_sg_memory;

        /* First, init the sg list.
         */
        for (current_sg_entries = 0; current_sg_entries < sg_entries; current_sg_entries++)
        {
            /* Init this single sg element and increment to the next entry.
             */
            fbe_sg_element_init(sg_p, max_bytes_per_sg_entry, memory_p);
            fbe_sg_element_increment(&sg_p);
            memory_p += max_bytes_per_sg_entry;
        }
        fbe_sg_element_terminate(sg_p);
    
        /* Reset the ptr to the original sg.
         */
        sg_p = fbe_test_sg_element_sg_array;

        /* Make sure the sg list has the right number of entries and bytes.
         */
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(sg_p), sg_entries);
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 
                             (sg_entries * max_bytes_per_sg_entry));
    
        /* Make sure our counting functions detect the right number of entries.
         */
        fbe_test_sg_element_validate_sg_entries(sg_p, max_bytes_per_sg_entry);

    } /* for sg_entries < FBE_TEST_MAX_SG_ELEMENTS */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sg_element_count()
 ******************************************/

/*!***************************************************************
 * @fn fbe_test_sg_element_count_remainder()
 ****************************************************************
 * @brief
 *  This is the unit test for sg element counting.
 *  The goal here is to test these functions:
 *      fbe_sg_element_count_entries_for_bytes_with_remainder()
 *
 * @param - None.
 *
 * @return - None.
 *
 * HISTORY:
 *  6/10/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_u32_t
fbe_test_sg_element_count_remainder(void)
{
    fbe_sg_element_t *sg_p = fbe_test_sg_element_sg_array;
    fbe_u32_t remainder;
    fbe_u8_t *memory_p = fbe_test_sg_element_sg_memory;

    /* First make sure that an obviously empty list returns 0 entries.
     * Test case with and without remainder ptr. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(NULL, 0, &remainder), 0);
    MUT_ASSERT_INT_EQUAL(remainder, 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(NULL, 0, NULL), 0);

    /* Next, init the sg list to have one entry with a terminator, and
     * make sure it also comes back with a length of 0. 
     */
    fbe_sg_element_terminate(sg_p);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(sg_p, 0, &remainder), 0);
    MUT_ASSERT_INT_EQUAL(remainder, 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(sg_p, 0, NULL), 0);

    /* Next, init the sg list to have one entry with a count and address.
     * Make sure the remainder comes back correct. 
     */
    fbe_sg_element_init(sg_p, 5, memory_p);
    fbe_sg_element_terminate(&sg_p[1]);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(sg_p, 3, &remainder), 1);
    MUT_ASSERT_INT_EQUAL(remainder, 1);

    /* Also test null remainder case.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(sg_p, 3, NULL), 1);

    /* Init next sg so we can test case where one sg is entirely satisfied by 
     * byte count, but next sg has information. 
     */
    fbe_sg_element_init(&sg_p[1], 5, memory_p);
    fbe_sg_element_terminate(&sg_p[2]);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(sg_p, 5, &remainder), 1);
    MUT_ASSERT_INT_EQUAL(remainder, 1);

    /* Also test null remainder case.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes_with_remainder(sg_p, 5, NULL), 1);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sg_element_count_remainder()
 ******************************************/
/*!***************************************************************
 * @fn fbe_test_sg_element_validate_sg_entries(fbe_sg_element_t *const sg_p,
 *                                      const fbe_u32_t max_bytes_per_entry )
 ****************************************************************
 * @brief
 *  Make sure that for a given sg list, we see valid results 
 *  for numbers of sg entries returned by our counting functions.
 *
 * @param sg_p - The sg list to validate.
 * @param max_bytes_per_entry - Max allowable bytes for each
 *                              sg entry.
 *
 * @return - None.  
 *
 * HISTORY:
 *  6/10/2008 - Created. RPF
 *
 ****************************************************************/

static void 
fbe_test_sg_element_validate_sg_entries(fbe_sg_element_t *const sg_p,
                                        const fbe_u32_t max_bytes_per_entry )
{
    fbe_u32_t total_list_bytes = fbe_sg_element_count_list_bytes(sg_p);
    fbe_u32_t offset;
    fbe_u32_t bytes_to_count;

    for (bytes_to_count = 1; bytes_to_count <= total_list_bytes; bytes_to_count ++)
    {
        /* Max sg entries assumes a consistent max bytes per entry.
         * First determine how many sg entries there are from the start of the sg list
         * to end of the bytes we are counting. 
         */
        fbe_u32_t max_sg_entries = (bytes_to_count / max_bytes_per_entry);

        if ( bytes_to_count % max_bytes_per_entry )
        {
            /* If we have a remainder add on one since we need to count the final 
             * entry. 
             */
            max_sg_entries++;
        }
        /* For valid byte counts we should get the expected number of entries.
         */
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(sg_p, bytes_to_count), 
                             max_sg_entries);

        for (offset = 0; offset + bytes_to_count <= total_list_bytes; offset++)
        {
            /* Max sg entries assumes a consistent max bytes per entry.
             * First determine how many sg entries there are from the start of the sg list
             * to end of the bytes we are counting. 
             */
            max_sg_entries = ((offset + bytes_to_count) / max_bytes_per_entry);

            if ( (offset + bytes_to_count) % max_bytes_per_entry)
            {
                /* If we have a remainder add on one since we need to count the final 
                 * entry. 
                 */
                max_sg_entries++;
            }
            /* Next decrement off any full sg elements that were in the offset.
             */
            max_sg_entries -= (offset / max_bytes_per_entry);

            /* Make sure that for valid byte+offset combination we count 
             * the expected number of bytes. 
             */
            MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 
                                                                          bytes_to_count, 
                                                                          offset), max_sg_entries);
        }/* end for all offsets */

        if ( bytes_to_count + offset > total_list_bytes )
        {
            /* Now offset is beyond a valid value, make sure we get back 0 for entries.
             */
            MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 
                                                                          bytes_to_count, 
                                                                          offset), 0);
        }
    }/* end for all bytes_to_count*/
    /* Any byte count beyond the sg length should return sg entry of zero.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(sg_p, total_list_bytes + 1), 0);

    return;
}
/******************************************
 * end fbe_test_sg_element_validate_sg_entries()
 ******************************************/

/*!***************************************************************
 * @fn fbe_test_sg_element_clear_sg(fbe_sg_element_t *const sg_p,
                                  const fbe_u32_t sg_entries_to_clear)
 ****************************************************************
 * @brief
 *  Clear out the sg list by clearing all the entries.
 * 
 * @param sg_p - sg list to clear.
 * @param sg_entries_to_clear - Number of sg entries to clear in the list.
 *
 * @return - None. 
 *
 * HISTORY:
 *  6/13/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_sg_element_clear_sg(fbe_sg_element_t *const sg_p,
                                  const fbe_u32_t sg_entries_to_clear)
{
    fbe_u32_t sg_entry_index;

    /* For all elements to clear, just terminate the sg list by clearing the 
     * entry. 
     */
    for(sg_entry_index = 0; sg_entry_index < sg_entries_to_clear; sg_entry_index++)
    {
        fbe_sg_element_terminate(&sg_p[sg_entry_index]);
    }
    return;
}
/******************************************
 * end fbe_test_sg_element_clear_sg()
 ******************************************/

/*!***************************************************************
 * @fn fbe_test_sg_element_copy_list_with_offset(fbe_sg_element_t *const sg_p,
 *                      fbe_sg_element_t *const dest_sg_p,
 *                      const fbe_u32_t max_sg_entries,
 *                      const fbe_u32_t bytes_to_copy,
 *                      const fbe_u32_t offset)
 ****************************************************************
 * @brief
 *  This is the test function for
 *  fbe_sg_element_copy_list_with_offset
 *
 * @param  sg_p - source sg list.
 * @param dest_sg_p - the destination sg list.
 * @param max_sg_entries - the max sg entries in the source sg.
 * @param bytes_to_copy - the bytes to copy from source to dest.
 * @param offset - the offset to use when copying
 *
 * @return - none   
 *
 * HISTORY:
 *  6/13/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_sg_element_copy_list_with_offset(fbe_sg_element_t *const sg_p,
                                               fbe_sg_element_t *const dest_sg_p,
                                               const fbe_u32_t max_sg_entries,
                                               const fbe_u32_t bytes_to_copy,
                                               const fbe_u32_t offset) 
{
    fbe_sg_element_t *new_dest_p;

    /* Clear sg so there are no leftover entries.
     */
    fbe_test_sg_element_clear_sg(dest_sg_p, FBE_TEST_MAX_SG_ELEMENTS);

    /* Make sure that for valid byte+offset combination we count 
     * the expected number of bytes. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 
                                                                  bytes_to_copy, 
                                                                  offset), max_sg_entries);

    /* Copy the source to destination for the given number of bytes.
     */
    new_dest_p = fbe_sg_element_copy_list_with_offset(sg_p, dest_sg_p, 
                                                      bytes_to_copy, offset);

    /* Make sure we did not take an error.
     */
    MUT_ASSERT_TRUE(new_dest_p != NULL);

    /* Make sure the right number of entries were added.
     */
    MUT_ASSERT_TRUE(new_dest_p > dest_sg_p);
    MUT_ASSERT_TRUE(new_dest_p - dest_sg_p == max_sg_entries);

    /* Make sure the list has the right number of entries.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(dest_sg_p), bytes_to_copy);

    /* For valid byte counts we should get the expected number of entries
     * for destination. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(dest_sg_p, bytes_to_copy), 
                         max_sg_entries);

    /* For valid byte counts we should get the expected number of entries
     * for destination. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(dest_sg_p), 
                         max_sg_entries);

    return;
}
/******************************************
 * end fbe_test_sg_element_copy_list_with_offset()
 ******************************************/

/*!***************************************************************
 * @fn fbe_test_sg_element_copy_list(fbe_sg_element_t *const sg_p,
 *                      fbe_sg_element_t *const dest_sg_p,
 *                      const fbe_u32_t max_sg_entries,
 *                      const fbe_u32_t bytes_to_copy)
 ****************************************************************
 * @brief
 *  This is the test function for fbe_sg_element_copy_list.
 *
 * @param  sg_p - source sg list.
 * @param dest_sg_p - the destination sg list.
 * @param max_sg_entries - the max sg entries in the source sg.
 * @param bytes_to_copy - the bytes to copy from source to dest.
 *
 * @return - none   
 *
 * HISTORY:
 *  6/13/2008 - Created. RPF
 *
 ****************************************************************/
void fbe_test_sg_element_copy_list(fbe_sg_element_t *const sg_p,
                                  fbe_sg_element_t *const dest_sg_p,
                                  const fbe_u32_t max_sg_entries,
                                  const fbe_u32_t bytes_to_copy) 
{
    fbe_sg_element_t *new_dest_p;

    /* For valid byte counts we should get the expected number of entries.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(sg_p, bytes_to_copy), 
                         max_sg_entries);

    /* Clear sg so there are no leftover entries.
     */
    fbe_test_sg_element_clear_sg(dest_sg_p, FBE_TEST_MAX_SG_ELEMENTS);

    /* Copy the source to destination for the given number of bytes.
     */
    new_dest_p = fbe_sg_element_copy_list(sg_p, dest_sg_p, bytes_to_copy);

    /* Make sure we did not take an error.
     */
    MUT_ASSERT_TRUE(new_dest_p != NULL);

    /* Make sure the right number of entries were added.
     */
    MUT_ASSERT_TRUE(new_dest_p > dest_sg_p);
    MUT_ASSERT_TRUE(new_dest_p - dest_sg_p == max_sg_entries);

    /* Make sure the list has the right number of entries.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(dest_sg_p), bytes_to_copy);

    /* For valid byte counts we should get the expected number of entries
     * for destination. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(dest_sg_p, bytes_to_copy), 
                         max_sg_entries);

    /* For valid byte counts we should get the expected number of entries
     * for destination. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(dest_sg_p), 
                         max_sg_entries);
    return;
}
/**************************************
 * end fbe_test_sg_element_copy_list
 **************************************/

/*!***************************************************************
 * fbe_test_sg_element_validate_copy(fbe_sg_element_t *const sg_p,
 *                                fbe_sg_element_t *const dest_sg_p, const
 *                                fbe_u32_t max_bytes_per_entry )
 ****************************************************************
 * @brief
 *  This is the unit test for sg element copying.
 *  This function tests these functions:
 *    fbe_test_sg_element_copy_list()
 *    fbe_test_sg_element_copy_list_with_offset()
 * 
 * @param  sg_p - source sg list.
 * @param dest_sg_p - the destination sg list.
 * @param max_bytes_per_entry - Max number of bytes in each sg entry.
 *
 * @return - None.
 *
 * HISTORY:
 *  6/10/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_sg_element_validate_copy(fbe_sg_element_t *const sg_p,
                                       fbe_sg_element_t *const dest_sg_p,
                                       const fbe_u32_t max_bytes_per_entry )
{
    fbe_u32_t total_list_bytes = fbe_sg_element_count_list_bytes(sg_p);
    fbe_u32_t offset;
    fbe_u32_t bytes_to_copy;

    for (bytes_to_copy = 1; bytes_to_copy <= total_list_bytes; bytes_to_copy ++)
    {
        /* Max sg entries assumes a consistent max bytes per entry.
         * First determine how many sg entries there are from the start of the sg list
         * to end of the bytes we are counting. 
         */
        fbe_u32_t max_sg_entries = (bytes_to_copy / max_bytes_per_entry);

        if ( bytes_to_copy % max_bytes_per_entry )
        {
            /* If we have a remainder add on one since we need to count the final 
             * entry. 
             */
            max_sg_entries++;
        }

        fbe_test_sg_element_copy_list(sg_p, dest_sg_p, max_sg_entries, bytes_to_copy);

        for (offset = 0; offset + bytes_to_copy <= total_list_bytes; offset++)
        {
            /* Max sg entries assumes a consistent max bytes per entry.
             * First determine how many sg entries there are from the start of the sg list
             * to end of the bytes we are counting. 
             */
            max_sg_entries = ((offset + bytes_to_copy) / max_bytes_per_entry);

            if ( (offset + bytes_to_copy) % max_bytes_per_entry)
            {
                /* If we have a remainder add on one since we need to count the final 
                 * entry. 
                 */
                max_sg_entries++;
            }
            /* Next decrement off any full sg elements that were in the offset.
             */
            max_sg_entries -= (offset / max_bytes_per_entry);

            /* Call our test function to test copying with offsets.
             */
            fbe_test_sg_element_copy_list_with_offset(sg_p, 
                                                      dest_sg_p, 
                                                      max_sg_entries,
                                                      bytes_to_copy,
                                                      offset);

        }/* end for all offsets */

        if ( bytes_to_copy + offset > total_list_bytes )
        {
            /* Clear sg so there are no leftover entries.
             */
            fbe_test_sg_element_clear_sg(dest_sg_p, FBE_TEST_MAX_SG_ELEMENTS);

            /* Since the offset + bytes is beyond the end of the source list, we
             * expect to get back a NULL, indicating error. 
             */
            MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(sg_p, dest_sg_p, 
                                                                 bytes_to_copy, offset)
                            == NULL);
        }
    }/* end for all bytes_to_count*/
    /* Any byte count beyond the sg length should return sg entry of zero.
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_for_bytes(sg_p, total_list_bytes + 1), 0);

    return;
}
/******************************************
 * end fbe_test_sg_element_validate_copy()
 ******************************************/
/*!***************************************************************
 * @fn fbe_test_sg_element_copy(void)
 ****************************************************************
 * @brief
 *  This is the unit test for copying sg elements.
 *  This function tests the following functions:
 *              fbe_sg_element_copy_list()
 *              fbe_sg_element_copy_list_with_offset()
 *
 * @param - None.
 *
 * @return - None.
 *
 * HISTORY:
 *  6/10/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_test_sg_element_copy(void)
{
    fbe_sg_element_t *sg_p = fbe_test_sg_element_sg_array;
    fbe_sg_element_t *dest_sg_p = fbe_test_sg_element_sg_array_dest;
    fbe_u32_t sg_entries;

    /* First null out the sgs.
     */
    fbe_sg_element_terminate(sg_p);
    fbe_sg_element_terminate(dest_sg_p);

    /* First make sure that sending NULL for source or destination returns an
     * error. 
     */
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list(NULL, dest_sg_p, 1) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list(sg_p, NULL, 1) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list(NULL, NULL, 1) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(NULL, NULL, 1, 0) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(sg_p, NULL, 1, 0) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(NULL, dest_sg_p, 1, 0) == NULL);

    /* First make sure that an empty list returns an error.
     */
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list(sg_p, dest_sg_p, 1) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(sg_p, dest_sg_p, 1, 0) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(sg_p, dest_sg_p, 1, 1) == NULL);

    /* Make sure copying 0 bytes returns no error.
     */
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list(sg_p, dest_sg_p, 0) == dest_sg_p);
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(sg_p, dest_sg_p, 0, 0) == dest_sg_p);

    /* Copying 0 bytes with an offset of 1 and no entries in source sg should
     * return error. 
     */
    MUT_ASSERT_TRUE(fbe_sg_element_copy_list_with_offset(sg_p, dest_sg_p, 0, 1) == NULL);

    /* Next, init the sg list to have one entry with a terminator, and
     * make sure it also comes back with a length of 0. 
     */
    fbe_sg_element_terminate(sg_p);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(sg_p), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 0), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 5, 0), 0);

    /* Even if the offset is non-zero, we should get back a zero since there are 0 bytes
     * in the sg. 
     */
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 4), 0);
    MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries_with_offset(sg_p, 1, 5), 0);

    /* Loop over all possible sg list lengths.
     */
    for (sg_entries = 1; sg_entries < FBE_TEST_MAX_SG_ELEMENTS; sg_entries++)
    {
        fbe_u32_t current_sg_entries;
        fbe_u32_t max_bytes_per_sg_entry = FBE_TEST_MAX_BYTES_PER_SG;
        fbe_u8_t *memory_p = fbe_test_sg_element_sg_memory;

        /* First, init the sg list that we will use as a source.
         */
        for (current_sg_entries = 0; current_sg_entries < sg_entries; current_sg_entries++)
        {
            /* Init this single sg element and increment to the next entry.
             */
            fbe_sg_element_init(sg_p, max_bytes_per_sg_entry, memory_p);
            fbe_sg_element_increment(&sg_p);
            memory_p += max_bytes_per_sg_entry;
        }
        fbe_sg_element_terminate(sg_p);
    
        /* Reset the ptr to the original sg.
         */
        sg_p = fbe_test_sg_element_sg_array;

        /* Make sure the sg list has the right number of entries and bytes.
         */
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_entries(sg_p), sg_entries);
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 
                             (sg_entries * max_bytes_per_sg_entry));
    
        /* Make sure our counting functions detect the right number of entries.
         */
        fbe_test_sg_element_validate_copy(sg_p, dest_sg_p, max_bytes_per_sg_entry);

    } /* for sg_entries < FBE_TEST_MAX_SG_ELEMENTS */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sg_element_copy()
 ******************************************/
/*!***************************************************************
 * @fn fbe_test_sg_element_init()
 ****************************************************************
 * @brief
 *  This is the unit test for the functions that initialize
 *  sg elements.
 *
 *  The purpose of this function is to test initialization of
 *  sg elements using fbe_sg_element_init()
 *                    fbe_sg_element_increment()
 *                and fbe_sg_element_terminate()
 *
 * @param - None. There are no parameters.
 *
 * @return - None.  There is no return value.
 *
 * HISTORY:
 *  6/10/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_test_sg_element_init(void)
{
    fbe_sg_element_t sg[5];
    fbe_sg_element_t *sg_p = sg;

    /* Create a simple two element sg list. 
     * Make sure it gets setup correctly. 
     */
    fbe_sg_element_init(sg_p, 5, fbe_test_sg_element_sg_memory);
    fbe_sg_element_increment(&sg_p);
    fbe_sg_element_init(sg_p, 4, &fbe_test_sg_element_sg_memory[5]);
    fbe_sg_element_increment(&sg_p);
    fbe_sg_element_terminate(sg_p);

    sg_p = sg;

    /* Make sure it was setup correctly. 
     */
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg_p) == fbe_test_sg_element_sg_memory);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg_p) == 5);
    fbe_sg_element_increment(&sg_p);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg_p) == &fbe_test_sg_element_sg_memory[5]);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg_p) == 4);
    fbe_sg_element_increment(&sg_p);
    MUT_ASSERT_TRUE(fbe_sg_element_address(sg_p) == NULL);
    MUT_ASSERT_TRUE(fbe_sg_element_count(sg_p) == 0);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sg_element_init()
 ******************************************/
/*!**************************************************************
 * @fn fbe_ldo_test_add_sg_element_tests(mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  Add the sg elements unit tests to the input suite.
 *
 * @param suite_p - suite to add tests to.               
 *
 * @return none.
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_sg_element_tests(mut_testsuite_t * const suite_p)
{
    /* Just add the tests we want to run to the suite.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_test_sg_element_init, 
                 NULL, 
                 NULL);
    MUT_ADD_TEST(suite_p, 
                 fbe_test_sg_element_count, 
                 NULL, 
                 NULL);
    MUT_ADD_TEST(suite_p, 
                 fbe_test_sg_element_copy, 
                 NULL, 
                 NULL);
    MUT_ADD_TEST(suite_p, 
                 fbe_test_sg_element_count_remainder, 
                 NULL, 
                 NULL);
    return;
}
/******************************************
 * end fbe_ldo_test_add_sg_element_tests() 
 ******************************************/
/*************************
 * end file fbe_logical_drive_test_sg_element.c
 *************************/


