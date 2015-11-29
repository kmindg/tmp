/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_test_state_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for the raid group object's
 *  test state object.
 *
 * @version
 *   07/30/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_raid_library_test_state_object.h"
#include "fbe_raid_library_proto.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/*! @var fbe_raid_group_test_state_object
 *  @brief This is the global instance of the state object. 
 */
fbe_raid_group_test_state_t fbe_raid_group_test_state_object;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * fbe_raid_group_test_get_state_object
 ****************************************************************
 * @brief
 *  This is used to get access to the global instance of the state object.
 *
 * @param - None.
 *
 * @return
 *  fbe_raid_group_test_state_t * - The global instance of the state object.
 *
 * @version
 *  12/03/07 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_group_test_state_t *fbe_raid_group_test_get_state_object(void)
{
    return &fbe_raid_group_test_state_object;
}
/* end fbe_raid_group_test_get_state_object */

/*!***************************************************************
 * @fn fbe_raid_group_test_state_init_check_words(fbe_raid_group_test_state_t * const state_p)
 ****************************************************************
 * @brief
 *  This function initializes the check words that are located in the test state
 *  object.
 * 
 *  These check words make sure that we do not overrun an allocation
 *  into the next allocation such as would occur if we indexed past
 *  the end of an array.
 * 
 *  We initialize these check words here and check them in
 *  @ref fbe_raid_group_test_state_validate_check_words
 *  We expect the check words to be unchanged.
 *
 * @param state_p - State object to check.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_test_state_init_check_words(fbe_raid_group_test_state_t * const state_p)
{
    /* Simply setup the check words to the correct values.
     */
    state_p->check_word_0 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_1 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_2 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_3 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_4 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_5 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_6 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_7 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_8 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_9 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_10 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_11 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_12 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    state_p->check_word_13 = FBE_RAID_GROUP_TEST_STATE_CHECK_WORD;
    return;
}
/**************************************
 * end fbe_raid_group_test_state_init_check_words()
 **************************************/

/*!***************************************************************
 * fbe_raid_group_test_state_validate_check_words()
 ****************************************************************
 * @brief
 *  This function validates the check words that are located in the
 *  test state object.
 * 
 *  These check words make sure that we do not overrun an allocation
 *  into the next allocation such as would occur if we indexed past
 *  the end of an array.
 * 
 *  We initialize these check words in
 *  @ref fbe_raid_group_test_state_init_check_words, and expect them to
 *  retain the same values always.
 *
 * @param state_p - State object to check.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_test_state_validate_check_words(fbe_raid_group_test_state_t * const state_p)
{
    /* Simply make sure the check words are correct.
     */
    MUT_ASSERT_TRUE(state_p->check_word_0 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_1 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_2 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_3 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_4 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_5 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_6 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_7 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_8 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_9 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_10 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_11 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_12 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_13 == FBE_RAID_GROUP_TEST_STATE_CHECK_WORD);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_test_state_validate_check_words()
 **************************************/

/*!***************************************************************
 * fbe_raid_group_free_iots_mock()
 ****************************************************************
 * @brief
 *  This is the mock free for the io cmd memory.
 * 
 *  Since this is a mock, we do not do anything here.
 *
 * @param iots_p - This is the io object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_free_iots_mock(fbe_raid_iots_t *const iots_p)
{
    /* Don't do anything since we statically allocated the memory.
     */
    return;
}
/* end fbe_raid_group_free_sg_mock() */

/*!***************************************************************
 *          fbe_raid_group_test_state_init()
 ****************************************************************
 * @brief
 *  This function inits the test state object.
 *
 * @param state_p - State object to setup.
 * @param raid_group_p - Pointer to raid group
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_test_state_init(fbe_raid_group_test_state_t * const state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Clear out the memory for this structure.
     */
    //fbe_zero_memory(state_p, sizeof(fbe_raid_group_test_state_t));

    //fbe_raid_group_test_set_max_blks_per_sg(state_p, FBE_LDO_STATE_MAX_BLKS_PER_SG);

    /* Setup the check words in the state object.
     * We will validate these periodically.
     */
    fbe_raid_group_test_state_init_check_words(state_p);
    
    state_p->io_packet_p = &state_p->io_packet_memory;

    /* Initialize this packets for use.
     * Whenever we allocate a packet we initialize it right away.
     */ 
    fbe_transport_initialize_packet(state_p->io_packet_p);

    /* Setup the siots and iots ptr in the state object from the packet payload.
     */
    fbe_raid_group_test_state_set_iots(state_p);
    fbe_raid_group_test_state_set_siots(state_p);

    return status;
}
/* end fbe_raid_group_test_state_init() */

/*!***************************************************************
 * fbe_raid_group_test_state_destroy()
 ****************************************************************
 * @brief
 *  This function destroys the test state object.
 *
 * @param state_p - State object to setup.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_test_state_destroy(fbe_raid_group_test_state_t * const state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_iots_t *iots_p = fbe_raid_group_test_state_get_iots(state_p);
    fbe_raid_siots_t *siots_p = fbe_raid_group_test_state_get_siots(state_p);
    /* Make sure our check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_raid_group_test_state_validate_check_words(state_p) == FBE_STATUS_OK);

    /* Destroy client io packet.
     */
    status = fbe_transport_destroy_packet(fbe_raid_group_test_state_get_io_packet(state_p));
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Destroy the iots.
     */
    status = fbe_raid_iots_destroy(iots_p);

    /* Destroy the siots if it was used.
     */
    if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS))
    {
        status = fbe_raid_siots_destroy_resources(siots_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_raid_siots_destroy(fbe_raid_group_test_state_get_siots(state_p));
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    /* Currently we do not destroy the io cmd, since there is nothing in the
     * io cmd that needs to be destroyed, as the resources for the io cmd
     * all get allocated as part of the state object.
     */
    fbe_raid_memory_mock_validate_memory_leaks();

    return status;
}
/* end fbe_raid_group_test_state_destroy() */

/*****************************************************
 * end fbe_raid_group_test_state_object.c
 *****************************************************/
