/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_test_native_write_same.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains testing functions for the logical drive object's native
 *  write same state machine.
 * 
 *  A native write same is a write same command where we will be able to issue
 *  a single write same command to the next object.
 *
 * HISTORY
 *   01/29/2008:  Created. RPF
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


/****************************************************************
 * fbe_ldo_test_native_ws_state0()
 ****************************************************************
 * DESCRIPTION:
 *  This function tests native write same state machine state0.
 *
 * PARAMETERS:
 *  test_state_p, [I] - Already setup state object.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/11/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_native_ws_state0( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_alloc_io_packet_mock_t *mock_p;
    fbe_sg_descriptor_t *sg_desc_p;
    
    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );
    
    /* Setup our mock allocate io packet function.
     */
    fbe_ldo_set_allocate_sg_fn(logical_drive_p, fbe_ldo_alloc_mem_mock_fn);
    mock_p = fbe_ldo_alloc_mem_mock_get_object();
    fbe_ldo_alloc_mem_mock_init(mock_p);

    /* Execute state 0.
     */
    state_status = fbe_ldo_io_cmd_native_write_same_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_WAITING,
                        "LDO: native ws state0 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_native_write_same_state10,
                        "native ws io command state after state 0 unexpected");

    /* Validate the values of the mock allocate object.
     */
    status = fbe_ldo_alloc_mem_mock_validate(mock_p);

    /* Next, NULL out the sg ptr to force a validation failure.
     */
    sg_desc_p = packet_io_block_get_block_cmd_sg_descriptor(
        fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_sg_descriptor_init_with_offset(sg_desc_p, 0, 0, NULL);

    /* Execute state 10 again.  We expect this to fail.
     */
    state_status = fbe_ldo_io_cmd_native_write_same_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: native ws state0 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_illegal_command,
                        "native ws io command state after state 0 unexpected");

    /* Restore the sg ptr for the client.
     */
    fbe_sg_descriptor_init_with_offset(sg_desc_p, 0, 0, 
                                       fbe_ldo_test_state_get_io_packet_sg(test_state_p));

    return status;
}
/* end fbe_ldo_test_native_ws_state0() */

/****************************************************************
 * fbe_ldo_test_native_ws_state10()
 ****************************************************************
 * DESCRIPTION:
 *  This function tests native write same state machine state10.
 *
 * PARAMETERS:
 *  test_state_p, [I] - Already setup state object.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_native_ws_state10( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *mock_p;
    
    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(logical_drive_p, fbe_ldo_send_io_packet_mock);
    mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(mock_p, io_cmd_p, FBE_STATUS_OK, 0);

    /* Set the sg ptr in the io command as if we allocated it already.
    */
    fbe_ldo_io_cmd_set_sg_ptr(io_cmd_p, 
                              fbe_ldo_test_state_get_io_cmd_sg(test_state_p));
    
    /* Setup the sub io packet as if it was allocated already.
     */
    fbe_ldo_io_cmd_set_sub_packet(io_cmd_p, 
                                  fbe_ldo_test_state_get_sub_io_packet(test_state_p));
    fbe_transport_initialize_packet(test_state_p->sub_io_packet_p);

    /* Execute state 10.
     */
    state_status = fbe_ldo_io_cmd_native_write_same_state10(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status,
                             FBE_LDO_STATE_STATUS_WAITING,
                             "LDO: native write same state10 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_native_write_same_state20,
                        "native write same io command state after state 10 unexpected");

    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: write same state10 send packet mock not called");

    /* Make sure we have the bytes in the sg.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_list_bytes,
                             (fbe_ldo_get_imported_block_size(logical_drive_p)),
                             "LDO: write same state10 sg list bytes incorrect");

    /* Make sure the sg size matches our current sub io packet size.
     */
    {
        fbe_u64_t bytes_transferred = fbe_ldo_io_cmd_sub_io_get_blocks(io_cmd_p) *
            fbe_ldo_io_cmd_sub_io_get_block_size(io_cmd_p);
        MUT_ASSERT_TRUE_MSG( 
            ( (bytes_transferred % fbe_ldo_get_imported_block_size(logical_drive_p)) == 0 ),
            "LDO: native write same io packet size not a multiple of imported block size" );
    }

    /* Make sure the sub io packet has a write same command.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_io_cmd_sub_io_get_opcode(io_cmd_p), 
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, 
                             "native write same state 10 opcode not write same");

    /* If there are not sufficient bytes in the sg list,
     * we will get back a zero for sg entries.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(mock_p->sg_entries, 0, 
                                 "native write same state 10 io packet sg entries are zero");

    /* Make sure we have the right number of sg entries in this sg.
     * We add one for the null terminator.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_entries + 1,
                             fbe_ldo_io_cmd_get_sg_entries(io_cmd_p),
                             "LDO: native write same state10 sg entries incorrect");

    /* Make sure the state was set to state 20 before we sent out the io packet.
     */
    MUT_ASSERT_TRUE_MSG(mock_p->io_cmd_state == fbe_ldo_io_cmd_native_write_same_state20,
                        "native write same state10 did not transition"
                        "to next state before sending io packet");

    return status;
}
/* end fbe_ldo_test_native_ws_state10() */

/****************************************************************
 * fbe_ldo_test_native_ws_state_machine()
 ****************************************************************
 * DESCRIPTION:
 *  This function tests write same state machine state0.
 *
 * PARAMETERS:
 *  test_state_p, [I] - Already setup state object.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_ldo_test_native_ws_state_machine( const fbe_lba_t lba,
                                      const fbe_block_count_t blocks,
                                      const fbe_block_size_t requested_block_size,
                                      const fbe_block_size_t exported_opt_block_size,
                                      const fbe_block_size_t imported_block_size,
                                      const fbe_block_size_t imported_opt_block_size,
                                      const fbe_u32_t max_bytes_per_sg_entry )
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();

    /* Setup the test state to test with a write same.
     */
    status = fbe_ldo_test_state_setup_write_same(test_state_p,
                                                 lba,    /* lba */
                                                 blocks,    /* blocks */
                                                 /* requested block size. */
                                                 requested_block_size,
                                                 exported_opt_block_size,
                                                 imported_block_size,
                                                 imported_opt_block_size,
                                                 max_bytes_per_sg_entry );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should only be here if this is a native write same operation.
     */
    MUT_ASSERT_TRUE(fbe_ldo_io_cmd_use_write_same_operation(fbe_ldo_test_state_get_io_cmd(test_state_p)));

    /* Test all the states for the native write same state machine.
     * Start testing with state 0, so that our command will be setup normally as 
     * if we are transitioning from ws_state0. 
     */ 
    status = fbe_ldo_test_ws_state0(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_ldo_test_native_ws_state0(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_ldo_test_native_ws_state10(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/* end fbe_ldo_test_native_ws_state_machine() */

/*****************************************
 * end fbe_logical_drive_test_write_same.c
 *****************************************/
