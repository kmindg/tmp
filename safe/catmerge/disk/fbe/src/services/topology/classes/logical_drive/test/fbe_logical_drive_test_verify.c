/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test functions for the logical drive's verify handling.
 * 
 *  Verifies get handled fairly simply by filling out the next payload and
 *  sending the packet down.
 * 
 *  Completion is also straightforward, we simply copy the block status to the
 *  client's payload and return.
 * 
 * @ingroup logical_drive_class_tests
 * 
 * HISTORY
 *   5/16/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
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

/*!***************************************************************
 * @fn fbe_ldo_test_state_setup_verify(
 *                        fbe_ldo_test_state_t * const test_state_p,
 *                        fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function sets up the test state object for verify.
 *
 * @param test_state_p - The state object to use.  This includes
 *                       the io cmd and the io packet.
 * @param task_p - This includes the lba, block count and other
 *                 block size parameters for this operation.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *
 * HISTORY:
 *  05/16/08- Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_test_state_setup_verify(fbe_ldo_test_state_t * const test_state_p,
                                fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    /* Init the state object.
     */
    fbe_ldo_test_state_init(test_state_p);

    /* Setup the number of sgs we need per sg entry max.
     */
    fbe_ldo_test_set_max_blks_per_sg(test_state_p, task_p->max_bytes_per_sg_entry);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Initialize our logical drive block size and optimum block size.
     */
    fbe_ldo_setup_block_sizes( logical_drive_p, task_p->imported_block_size );
    
    /* Setup io packet.
     */
    fbe_ldo_build_sub_packet( io_packet_p,
                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY,
                              task_p->lba,    /* lba */
                              task_p->blocks,    /* blocks */
                              task_p->exported_block_size,    /* exported block size */
                              task_p->exported_opt_block_size,
                              NULL, /* No sg ptr needed. */
                              /* Context is the io cmd. */
                              io_cmd_p );

    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_ldo_test_increment_stack_level(io_packet_p);

    /* The I/O packet sg is left NULL.
     */

    /* Make sure check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_state_setup_verify() */

/*!***************************************************************
 * @fn fbe_ldo_test_verify(fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests verify.
 *
 * @param test_state_p - The state object to use.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/18/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_verify(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *mock_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(mock_p, io_cmd_p, FBE_STATUS_OK, 0);
    
    /* Test the handle verify code.
     */
    status = fbe_ldo_handle_verify(logical_drive_p, io_packet_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK,
                             "LDO: return status not expected");
    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: verify send packet mock not called");
    /* Make sure we have the right number of bytes in this sg.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_list_bytes, 0,
                             "LDO: verify sg list bytes incorrect");

    /* Make sure the packet has a Verify Cmd.
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_opcode(io_packet_p), 
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY, 
                             "verify opcode not verify");

    /* Make sure the block size is the same as imported and optimal is 1,
     * since the physical drive only accepts an imported bl size that is equal 
     * to exported block size. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_block_size(io_packet_p), 
                             fbe_ldo_get_imported_block_size(logical_drive_p), 
                             "verify block size unexpected");
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_optimum_block_size(io_packet_p), 
                             1, 
                             "verify optimum block size unexpected");

    return status;  
}
/* end fbe_ldo_test_verify() */

/*!***************************************************************
 * @fn fbe_logical_drive_verify_state_machine_test_case(
 *                  fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param task_p - The task object to use for this test.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_verify_state_machine_test_case(
    fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the read state object and set it up for verify. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();

    /* Execute verify test.
     */
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_verify,
                                       fbe_ldo_test_verify,
                                       task_p);

    return status;
}
/* end fbe_logical_drive_verify_state_machine_test_case() */

/*!***************************************************************
 * @fn fbe_ldo_verify_state_machine_tests(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_verify_state_machine_tests(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Test all the logical drive read cases for these test tasks.
     */
    status = fbe_logical_drive_state_machine_tests(fbe_ldo_test_io_tasks,
                                                   fbe_logical_drive_verify_state_machine_test_case);
    return status;
}
/* end fbe_ldo_verify_state_machine_tests() */

/*************************
 * end file fbe_logical_drive_test_verify.c
 *************************/


