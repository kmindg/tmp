/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_state_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for the logical drive object's
 *  test state object.
 *
 * @version
 *   12/03/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_logical_drive_test_state_object.h"
#include "fbe_logical_drive_test_private.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/*! @var fbe_ldo_test_state_object
 *  @brief This is the global instance of the state object. 
 */
fbe_ldo_test_state_t fbe_ldo_test_state_object;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_test_get_state_object(void) 
 ****************************************************************
 * @brief
 *  This is used to get access to the global instance of the state object.
 *
 * @param - None.
 *
 * @return
 *  fbe_ldo_test_state_t * - The global instance of the state object.
 *
 * @version
 *  12/03/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_test_state_t *fbe_ldo_test_get_state_object(void)
{
    return &fbe_ldo_test_state_object;
}
/* end fbe_ldo_test_get_state_object */

/*!***************************************************************
 * @fn fbe_ldo_test_state_init_check_words(fbe_ldo_test_state_t * const state_p)
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
 *  @ref fbe_ldo_test_state_validate_check_words
 *  We expect the check words to be unchanged.
 *
 * @param state_p - State object to check.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_test_state_init_check_words(fbe_ldo_test_state_t * const state_p)
{
    /* Simply setup the check words to the correct values.
     */
    state_p->check_word_0 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_1 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_2 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_3 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_4 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_5 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_6 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_7 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_8 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_9 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_10 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_11 = FBE_LDO_TEST_STATE_CHECK_WORD;
    state_p->check_word_12 = FBE_LDO_TEST_STATE_CHECK_WORD;
    return;
}
/**************************************
 * end fbe_ldo_test_state_init_check_words()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_state_validate_check_words(fbe_ldo_test_state_t * const state_p)
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
 *  @ref fbe_ldo_test_state_init_check_words, and expect them to
 *  retain the same values always.
 *
 * @param state_p - State object to check.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_state_validate_check_words(fbe_ldo_test_state_t * const state_p)
{
    /* Simply make sure the check words are correct.
     */
    MUT_ASSERT_TRUE(state_p->check_word_0 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_1 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_2 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_3 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_4 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_5 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_6 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_7 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_8 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_9 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_10 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_11 == FBE_LDO_TEST_STATE_CHECK_WORD);
    MUT_ASSERT_TRUE(state_p->check_word_12 == FBE_LDO_TEST_STATE_CHECK_WORD);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_test_state_validate_check_words()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_free_io_cmd_mock(fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This is the mock free for the io cmd memory.
 * 
 *  Since this is a mock, we do not do anything here.
 *
 * @param io_cmd_p - This is the io command object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_free_io_cmd_mock(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    /* Don't do anything since we statically allocated the memory.
     */
    return;
}
/* end fbe_ldo_free_sg_mock() */

/*!***************************************************************
 * @fn fbe_ldo_test_state_init(fbe_ldo_test_state_t * const state_p)
 ****************************************************************
 * @brief
 *  This function inits the test state object.
 *
 * @param state_p - State object to setup.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  12/03/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_state_init(fbe_ldo_test_state_t * const state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_t *logical_drive_p = &state_p->logical_drive;
    
    /* Clear out the memory for this structure.
     */
    fbe_zero_memory(state_p, sizeof(fbe_ldo_test_state_t));

    fbe_ldo_test_set_max_blks_per_sg(state_p, FBE_LDO_STATE_MAX_BLKS_PER_SG);

    /* Setup the check words in the state object.
     * We will validate these periodically.
     */
    fbe_ldo_test_state_init_check_words(state_p);
    
    state_p->io_packet_p = &state_p->io_packet_memory;

    /* Initialize this packets for use.
     * Whenever we allocate a packet we initialize it right away.
     */ 
    fbe_transport_initialize_packet(state_p->io_packet_p);

    /* Initialize the io command from the io packet.
     */
    fbe_ldo_io_cmd_init(fbe_ldo_test_state_get_io_cmd(state_p),
                        (fbe_object_handle_t)&state_p->logical_drive, 
                        state_p->io_packet_p);

    /* Simply call our init function.
     */
    fbe_logical_drive_initialize_for_test(logical_drive_p);
    return status;
}
/* end fbe_ldo_test_state_init() */

/*!***************************************************************
 * @fn fbe_ldo_test_state_destroy(fbe_ldo_test_state_t * const state_p)
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
 *  12/03/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_state_destroy(fbe_ldo_test_state_t * const state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p = fbe_ldo_test_state_get_io_cmd(state_p);
    fbe_logical_drive_t *logical_drive_p =
        fbe_ldo_test_state_get_logical_drive(state_p);

    /* Make sure our check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(state_p) == FBE_STATUS_OK);

    /* Destroy the base class.  Must be done explicitly because of interface.
     */
    fbe_spinlock_destroy(&(fbe_ldo_test_state_get_logical_drive(state_p))->base_discovered.base_object.object_lock);
    fbe_spinlock_destroy(&(fbe_ldo_test_state_get_logical_drive(state_p))->base_discovered.base_object.terminator_queue_lock);

    /* Destroy logical drive.
     */
    fbe_logical_drive_destroy((fbe_object_handle_t)logical_drive_p);

    /* Destroy client io packet.
     */
    fbe_transport_destroy_packet(fbe_ldo_test_state_get_io_packet(state_p));

    /* Destroy io cmd.
     * First add the mock free functions scatter gather and the io cmd, since 
     * these are allocated statically in the state object and should not be 
     * freed by the destroy. 
     */
    fbe_ldo_set_free_io_cmd_fn(fbe_ldo_free_io_cmd_mock);
    fbe_ldo_io_cmd_destroy(io_cmd_p);

    /* Currently we do not destroy the io cmd, since there is nothing in the
     * io cmd that needs to be destroyed, as the resources for the io cmd
     * all get allocated as part of the state object.
     */

    return status;
}
/* end fbe_ldo_test_state_destroy() */

/*****************************************************
 * end fbe_logical_drive_test_state_object.c
 *****************************************************/
