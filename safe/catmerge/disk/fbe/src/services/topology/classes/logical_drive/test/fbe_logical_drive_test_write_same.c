/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_write_same.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  write same state machine.
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


fbe_status_t 
fbe_ldo_test_state_setup_write_same(fbe_ldo_test_state_t * const test_state_p,
                                    fbe_ldo_test_state_machine_task_t *const task_p);

/*! @struct fbe_ldo_test_write_same_tasks 
 *  @brief This structure contains the test cases for testing a
 *         range of write same I/Os, each with a specific block size
 *         configuration.
 */
static fbe_test_io_task_t fbe_ldo_test_write_same_tasks[] = 
{
    /*
     * start end        start  end           exp  exp  imp  imp increment 
     * lba   lba        blocks blocks        blk  opt  blk  opt blocks
     *                                            blk  size blk
     */
    {0,   (64 * 10),    64,    (64 * 200),   520, 64,  512,  65,  64},
    {0,   (512 * 10),   512,   (512 * 200),  520, 512, 4096, 65, 512},
    {0,   (8 * 10),     8,     (8 * 200),    520, 8,   4160, 1,  8},

    /* Lossy cases where the imported optimal block does not
     * match the exported optimal block. 
     */
    {0,   20,           2,     20,           520,  2,  512,  3,  2},
    {0,   40,           4,     40,           520,  4,  512,  5,  4},
    {0,   512,          7,     512,          520,  7,  4096, 1,  7},
    {0,   512,         15,     512,          520,  15, 4096, 2,  15},

    /* Insert any new test cases above this point.
     * FBE_LDO_TEST_INVALID_FIELD, means end
     */  
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0}
};

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_state0(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write same state machine state0.
 *
 * @param test_state_p - Already setup state object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/22/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_write_same_state0( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    
    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Execute state 0.
     */
    state_status = fbe_ldo_io_cmd_write_same_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: write same state0 return status unexpected");

    /* We always goto state 10.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_same_state10,
                        "write same io command state unexpected");
    return status;
}
/* end fbe_ldo_test_write_same_state0() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_state0_error(
 *                        fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write same state machine state0
 *  for the case when there is an error because the sg
 *  list does not have enough data.
 *
 * @param test_state_p - Already setup state object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/22/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_write_same_state0_error( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_payload_ex_t *payload_p;
    
    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );  

    /* Next, NULL out the sg ptr to force a validation failure.
     */
    payload_p = fbe_transport_get_payload_ex(fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_payload_ex_set_sg_list(payload_p, NULL, 0);

    /* Execute state 0.  We expect this to fail.
     */
    state_status = fbe_ldo_io_cmd_write_same_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: write same state0 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "write same io command state after state0 unexpected");

    /* Restore the sg ptr for the client.
     */
    fbe_payload_ex_set_sg_list(payload_p, fbe_ldo_test_state_get_io_packet_sg(test_state_p), 0);
    return status;
}
/* end fbe_ldo_test_write_same_state0_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_state10(
 *                        fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write same state machine state10.
 *
 * PARAMETERS:
 *  test_state_p, [I] - Already setup state object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/22/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_write_same_state10( fbe_ldo_test_state_t * const test_state_p )
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
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(mock_p, io_cmd_p, FBE_STATUS_OK, 0);

    /* Execute state 10.
     */
    state_status = fbe_ldo_io_cmd_write_same_state10(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_WAITING,
                        "LDO: write same state10 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_same_state20,
                        "write same io command state after state 10 unexpected");

    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: write same state10 send packet mock not called");

    /* Make sure the state was set to state 30 before we sent out the io packet.
     */
    MUT_ASSERT_TRUE_MSG(mock_p->io_cmd_state == fbe_ldo_io_cmd_write_same_state20,
                        "write same state10 did not transition to next state before sending io packet");

    /* Make sure the sub io packet has a write same command.
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_opcode(io_packet_p), 
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                             "write same state 10 opcode not write");
    return status;
}
/* end fbe_ldo_test_write_same_state10() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_state20(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write same state20.
 *
 *  @param test_state_p - Already setup state object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_same_state20( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Make sure the client packet status is bad.
     */
    fbe_transport_set_status(io_packet_p, FBE_STATUS_INVALID, 0);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(io_packet_p) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(io_packet_p) == 0);

    /* Increment stack level since when a packet has been sent this was already
     * incremented. 
     */
    fbe_ldo_test_allocate_and_increment_stack_level(io_packet_p);
    fbe_transport_set_status(io_packet_p, FBE_STATUS_OK, 0x12345678);

    /* Execute state 20.
     */
    state_status = fbe_ldo_io_cmd_write_same_state20(io_cmd_p);

    /* Make sure the client packet status and qualifier is set to the sub packet
     * status and qualifier
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(io_packet_p),
                             FBE_STATUS_OK,
                             "LDO: Client packet's status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(io_packet_p),
                             0x12345678,
                             "LDO: Client packet's qualifier not set from sub packet");

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: write same state20 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "write same state20 io command state unexpected");

    return status;
}
/* end fbe_ldo_test_write_same_state20() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_state30(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write same state30.
 *
 *  @param test_state_p - Already setup state object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_same_state30( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Make sure the client packet status is bad.
     */
    fbe_transport_set_status(io_packet_p, FBE_STATUS_INVALID, 0x1234578);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(io_packet_p) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(io_packet_p) == 0x1234578);

    /* Execute state 30.
     */
    state_status = fbe_ldo_io_cmd_write_same_state30(io_cmd_p);

    /* Make sure the client packet status and qualifier is set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(io_packet_p),
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: Client packet's status not set as expected in state 40.");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(io_packet_p),
                             0,
                             "LDO: Client packet's qualifier not set as expected.");

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: write same state30 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "write same state30 io command state unexpected");

    return status;
}
/* end fbe_ldo_test_write_same_state30() */

/*!**************************************************************
 * @fn fbe_ldo_write_same_state_machine_test_case(
 *                fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  Execute the state machine states for a given test case.
 *  
 * @param task_p - The test case to execute.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/31/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_write_same_state_machine_test_case(fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the write same state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();

    /* Make sure we can setup and destroy a write.
     */
    status = fbe_ldo_test_state_setup_write_same(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    /* test state0.
     */
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write_same,
                                       fbe_ldo_test_write_same_state0,
                                       task_p);

    /* test state0 with error.
     */
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write_same,
                                       fbe_ldo_test_write_same_state0_error,
                                       task_p);

    /* test state10.
     */
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write_same,
                                       fbe_ldo_test_write_same_state10,
                                       task_p);

    /* test state20.
     */
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write_same,
                                       fbe_ldo_test_write_same_state20,
                                       task_p);

    /* test state30.
     */
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write_same,
                                       fbe_ldo_test_write_same_state30,
                                       task_p);
    return status;
}
/**************************************
 * end fbe_ldo_write_same_state_machine_test_case()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_state_machine_cases(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the write same state machine.
 *
 * @param - None.
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  02/22/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_same_state_machine_cases(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Test all the logical drive write same cases for these test tasks.
     */
    status = fbe_logical_drive_state_machine_tests(fbe_ldo_test_write_same_tasks,
                                                   fbe_ldo_write_same_state_machine_test_case);
    
    return status;
}
/* end fbe_ldo_test_write_same_state_machine_cases() */

/*!***************************************************************
 * @fn fbe_ldo_test_state_setup_write_same(
 *                     fbe_ldo_test_state_t * const test_state_p,
 *                     fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function sets up the test object for a write same command.
 * 
 * @param test_state_p - The state of the test including the
 *                       io cmd object, packet and sgs.
 * @param task_p - Contains the parameters for this test.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  01/29/2008:  Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_test_state_setup_write_same(fbe_ldo_test_state_t * const test_state_p,
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
                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                              task_p->lba,    /* lba */
                              task_p->blocks,    /* blocks */
                              task_p->exported_block_size,    /* exported block size */
                              task_p->exported_opt_block_size, /* Optimal block size */
                              fbe_ldo_test_state_get_io_packet_sg(test_state_p),
                              /* Context is the io cmd. */
                              io_cmd_p );

    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_ldo_test_increment_stack_level(io_packet_p);

    /* Setup io packet sg.
     * Note that a write same inputs only a single exported block of data.
     */
    fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_io_packet_sg(test_state_p),
                                fbe_ldo_test_state_get_io_packet_sg_memory(test_state_p),
                                (fbe_u32_t) task_p->exported_block_size, /* Bytes */
                                (fbe_u32_t) fbe_ldo_test_get_max_blks_per_sg(test_state_p) );
    
    /* Call generate to init the remainder of the fields.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_generate(io_cmd_p) == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO:  io cmd generate did not return executing");

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_same_state0,
                        "LDO:  io cmd state not return write same state 0");

    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_state_setup_write_same() */

/*!**************************************************************
 * @fn fbe_ldo_test_write_same_validate_sg_cases(   
 *                 const fbe_lba_t lba,   
 *                 const fbe_block_count_t blocks,   
 *                 const fbe_block_size_t requested_block_size,   
 *                 const fbe_block_size_t exported_opt_block_size,   
 *                 const fbe_block_size_t imported_block_size,   
 *                 const fbe_block_size_t imported_opt_block_size )   
 ****************************************************************
 * @brief
 *   This function executes tests of the write same validate function.   
 *   
 * @param lba - Logical Block Address.
 * @param blocks - Number of blocks.
 * @param requested_block_size - Size in bytes of block size requested.
 * @param exported_opt_block_size - Size in blocks of the optimal size.
 * @param imported_block_size - Size in bytes of the imported block size.
 * @param imported_opt_block_size - Size in blocks of the imported optimal size.
 * 
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  02/05/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ldo_test_write_same_validate_sg_cases( 
    const fbe_lba_t lba,
    const fbe_block_count_t blocks,
    const fbe_block_size_t requested_block_size,
    const fbe_block_size_t exported_opt_block_size,
    const fbe_block_size_t imported_block_size,
    const fbe_block_size_t imported_opt_block_size )
{
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_status_t status;
    fbe_ldo_test_state_machine_task_t task = {lba, blocks, 
        requested_block_size, exported_opt_block_size, imported_block_size, 
        imported_opt_block_size, FBE_U32_MAX, 0, 0};
    fbe_payload_ex_t *payload_p;

    /* Get the state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    
    /* Setup the test state to test with a write same.
     */
    status = fbe_ldo_test_state_setup_write_same(test_state_p, &task);

    /* Make sure we setup the write same correctly.
     */
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    /* Setup io packet sg.
     * Note that a write same should have a single exported block of data. 
     * We set the sg list size to be just under what it should be so that 
     * validate will fail. 
     */
    fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_io_packet_sg(test_state_p),
                                fbe_ldo_test_state_get_io_packet_sg_memory(test_state_p),
                                (fbe_u32_t) requested_block_size - 1, /* Bytes */
                                (fbe_u32_t) fbe_ldo_test_get_max_blks_per_sg(test_state_p) );

    /* Validate this command that we built.
     * We expect this to fail since the sg list is not long enough. 
     */
    status = fbe_ldo_io_cmd_validate_write_same(io_cmd_p);

    /* Check the status we received against the expected status.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(status, FBE_STATUS_OK, 
                                 "write same validate status should be bad for incomplete sg");

    /* Set the sg ptr in the io packet to NULL in order to force the validation
     * below to fail. 
     */
    payload_p = fbe_transport_get_payload_ex(fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_payload_ex_set_sg_list(payload_p, NULL, 0);

    /* Validate this command that we built.
     * We expect this to fail since the sg ptr is null 
     */
    status = fbe_ldo_io_cmd_validate_write_same(io_cmd_p);

    /* Check the status we received against the expected status.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(status, FBE_STATUS_OK, 
                                 "write same validate status should be bad for null sg");
        
    /* Destroy the state object and assert that all goes well.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_ldo_test_write_same_validate_sg_cases()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_validate_case(
 *                 const fbe_lba_t lba,   
 *                 const fbe_block_count_t blocks,   
 *                 const fbe_block_size_t requested_block_size,   
 *                 const fbe_block_size_t exported_opt_block_size,   
 *                 const fbe_block_size_t imported_block_size,   
 *                 const fbe_block_size_t imported_opt_block_size,
 *                 const fbe_status_t expected_status)
 ****************************************************************
 * @brief
 *  This function executes tests of the write same validate function.
 *  
 * @param lba - Logical Block Address.
 * @param blocks - Number of blocks.
 * @param requested_block_size - Size in bytes of block size requested.
 * @param exported_opt_block_size - Size in blocks of the optimal size.
 * @param imported_block_size - Size in bytes of the imported block size.
 * @param imported_opt_block_size - Size in blocks of the imported optimal size.  
 * @param expected_status - The status that we expect to be returned  
 *                          for this set of parameters.
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_test_write_same_validate_case( const fbe_lba_t lba,
                                       const fbe_block_count_t blocks,
                                       const fbe_block_size_t requested_block_size,
                                       const fbe_block_size_t exported_opt_block_size,
                                       const fbe_block_size_t imported_block_size,
                                       const fbe_block_size_t imported_opt_block_size,
                                       const fbe_status_t expected_status )
{
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_status_t status;
    fbe_ldo_test_state_machine_task_t task = {lba, blocks, 
        requested_block_size, exported_opt_block_size, imported_block_size, 
        imported_opt_block_size, FBE_U32_MAX, 0, 0};

    /* Get the state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    
    /* Setup the test state to test with a write same.
     */
    status = fbe_ldo_test_state_setup_write_same(test_state_p, &task);

    /* Make sure we setup the write same correctly.
     */
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    /* Validate this command that we built.
     */
    status = fbe_ldo_io_cmd_validate_write_same(io_cmd_p);

    /* Check the status we received against the expected status.
     */
    MUT_ASSERT_INT_EQUAL_MSG(status, expected_status, 
                             "status of validate write same not expected");
        
    /* Destroy the state object and assert that all goes well.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_write_same_validate_case() */

/*! @struct fbe_ldo_test_write_same_validate_task_t 
 *  @brief This structure represents a test case for a set of ws validate parameters.
 */                                                                  
typedef struct fbe_ldo_test_write_same_validate_task_s
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_status_t expected_status;
}
fbe_ldo_test_write_same_validate_task_t;

/*! @var fbe_ldo_test_write_same_validate_tasks 
 *  @brief This structure contains the test cases for testing 
 *         a single case of write same validate. 
 */
fbe_ldo_test_write_same_validate_task_t fbe_ldo_test_write_same_validate_tasks[] = 
{
    /*
     * lba   blocks       exp     exp     imp     imp 
     *                    blk     opt blk blk     opt blk    status
     *                            size    size    size
     */
    /* Simple one opt blk size case. */
    {0,      64,          520,    64,     512,    65,         FBE_STATUS_OK},

    /* Cases where the size of the write same is too small and therefore
     * unaligned to the optimal block size. 
     */
    {0,      63,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {1,      63,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {63,      1,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {128,     1,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {256,     1,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},

    /* Crossing opt blk size boundary. */
    {0,      65,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {0,     129,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {0,     257,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {1,      64,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {1,      65,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {63,      2,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {64,     63,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {64,     65,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {128,    65,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},
    {256,   129,          520,    64,     512,    65,         FBE_STATUS_GENERIC_FAILURE},

    /* Even Multiples of opt blk size. */
    {0,     (64 * 2),     520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 3),     520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 4),     520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 5),     520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 10),    520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 16),    520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 24),    520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 32),    520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 50),    520,    64,     512,    65,         FBE_STATUS_OK},
    {0,     (64 * 200),   520,    64,     512,    65,         FBE_STATUS_OK},

    {64,     64,          520,    64,     512,    65,         FBE_STATUS_OK},
    {64,    (64 * 2),     520,    64,     512,    65,         FBE_STATUS_OK},
    {64,    (64 * 3),     520,    64,     512,    65,         FBE_STATUS_OK},
    {64,    (64 * 4),     520,    64,     512,    65,         FBE_STATUS_OK},
    {64,    (64 * 5),     520,    64,     512,    65,         FBE_STATUS_OK},

    /* Simple one opt blk size case. */
    {0,     512,          520,    512,    4096,   65,        FBE_STATUS_OK},

    /* Cases where the size of the write same is too small and therefore
     * unaligned to the optimal block size. 
     */
    {0,     511,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {1,     511,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {511,     1,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {512,     1,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {1024,    1,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},

    /* Crossing opt blk size boundary. */
    {0,     513,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {0,    1025,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {0,    2049,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {1,     512,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {1,     513,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {511,     2,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {512,   511,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {512,   513,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {1024,  513,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},
    {1024, 1025,          520,    512,    4096,   65,         FBE_STATUS_GENERIC_FAILURE},

    /* Even Multiples of opt blk size. */
    {0,    (512 *2),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *3),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *4),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *5),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *10),    520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *16),    520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *24),    520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *32),    520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *50),    520,    512,    4096,   65,         FBE_STATUS_OK},
    {0,    (512 *200),   520,    512,    4096,   65,         FBE_STATUS_OK},

    {512,    512,         520,    512,    4096,   65,         FBE_STATUS_OK},
    {512,   (512 *2),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {512,   (512 *3),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {512,   (512 *4),     520,    512,    4096,   65,         FBE_STATUS_OK},
    {512,   (512 *5),     520,    512,    4096,   65,         FBE_STATUS_OK},

    /* Insert any new test cases above this point.
     * FBE_LDO_TEST_INVALID_FIELD, means end.
     */  
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0}
};

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_validate_cases(
 * fbe_ldo_test_write_same_validate_task_t *const task_array_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the write same validate function.
 *
 * @param task_array_p - The set of test tasks to execute.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_same_validate_cases(
    fbe_ldo_test_write_same_validate_task_t *const task_array_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    /* First loop over all the tasks.  Stop when we hit the end of the table
     * (where the lba is marked invalid).  Also stop when the status is not expected.
     */
    while (task_array_p[index].lba != FBE_LDO_TEST_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        status = fbe_ldo_test_write_same_validate_case(
            task_array_p[index].lba,
            task_array_p[index].blocks,
            task_array_p[index].exported_block_size,
            task_array_p[index].exported_opt_block_size,
            task_array_p[index].imported_block_size,
            task_array_p[index].imported_opt_block_size,
            task_array_p[index].expected_status);
        
        index++;

    } /* end while not at end of task table and status is expected. */
    
    return status;
}
/* end fbe_ldo_test_write_same_validate_cases() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_same_validate(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the write same validate function.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  02/06/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_same_validate(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_ldo_test_write_same_validate_cases(fbe_ldo_test_write_same_validate_tasks);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "LDO: test_ws_validate_cases failed");

    /* Test cases where we have an invalid sg list.
     * Test case for 512 byte blocks. 
     */
    status = fbe_ldo_test_write_same_validate_sg_cases( 0,  /* lba */ 
                                                64, /* blocks */
                                                520, /* requested block size */
                                                64,
                                                512, /* imported block size */
                                                65   /* imported optimal block size */ );
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "LDO: test_ws_validate_sg_cases failed 512");

    /* Test case for 4096 byte blocks.
     */
    status = fbe_ldo_test_write_same_validate_sg_cases( 0,  /* lba */ 
                                                512, /* blocks */
                                                520, /* requested block size */
                                                64,
                                                4096, /* imported block size */
                                                512 /* imported optimal block size */ );
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "LDO: test_ws_validate_sg_cases failed 4096");

    /* Test case for 4160 byte blocks.
     */
    status = fbe_ldo_test_write_same_validate_sg_cases( 0,  /* lba */ 
                                                8, /* blocks */
                                                520, /* requested block size */
                                                8,
                                                4160, /* imported block size */
                                                1 /* imported optimal block size */ );
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "LDO: test_ws_validate_sg_cases failed 4160");

    return status;
}
/* end fbe_ldo_test_write_same_validate() */

/*!***************************************************************
 * @fn fbe_ldo_test_add_write_same_tests(mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  This function adds the logical drive write same tests to
 *  the given input test suite.
 *
 * @param suite_p - the suite to add write same tests to.
 *
 * @return None.
 *
 * HISTORY:
 *  08/19/08 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_write_same_tests(mut_testsuite_t * const suite_p)
{
    /* Simply setup the bitbucket tests in this suite.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_same_validate,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_same_state_machine_cases,
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/* end fbe_ldo_test_add_write_same_tests() */

/*****************************************
 * end fbe_logical_drive_test_write_same.c
 *****************************************/
