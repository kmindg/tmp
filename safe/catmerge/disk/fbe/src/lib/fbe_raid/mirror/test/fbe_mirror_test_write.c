/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_test_write.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the raid mirror write state machine.
 *
 * @version
 *  9/11/2009   Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe_trace.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_library_test_state_object.h"
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_test_prototypes.h"
#include "fbe_mirror_io_private.h"
#include "mut.h"


/*************************
 *   Globals
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************
 *      fbe_mirror_test_setup_write
 ****************************************************************
 * @brief
 *  This function sets up for a write state machine test.
 *
 * @param test_state_p - The test state object to use in the test.
 * @param task_p - The test task to use.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author:
 *  09/11/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_test_setup_write(fbe_raid_group_test_state_t * const test_state_p,
                                         fbe_raid_group_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_state_status_t raid_state_status = FBE_RAID_STATE_STATUS_INVALID;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    raid_geometry_p = fbe_raid_group_test_state_get_raid_geometry(test_state_p);

    /* Init the striper.
     */
    fbe_mirror_test_initialize_mirror_object(raid_geometry_p,
                                             fbe_raid_group_test_state_get_block_edge(test_state_p), 
                                             task_p->width, 
                                             task_p->capacity);

    /* Init the state object.
     */
    fbe_raid_group_test_state_init(test_state_p);
    
    /* Setup the number of sgs we need per sg entry max.
     */
    fbe_raid_group_test_set_max_blks_per_sg(test_state_p, task_p->max_bytes_per_sg_entry);

    packet_p = fbe_raid_group_test_state_get_io_packet(test_state_p);
    iots_p = fbe_raid_group_test_state_get_iots(test_state_p);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    MUT_ASSERT_NOT_NULL_MSG( packet_p, "packet is NULL");
    MUT_ASSERT_NOT_NULL_MSG( iots_p, "iots is NULL");

    /*! @note we could initialize our logical drive block size and optimum block size.
     */
    //fbe_ldo_setup_block_sizes( logical_drive_p, task_p->imported_block_size);
    
    /* Setup packet.
     */
    fbe_raid_group_test_build_packet( packet_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      fbe_raid_group_test_state_get_block_edge(test_state_p),
                                      task_p->lba,    /* lba */
                                      task_p->blocks,    /* blocks */
                                      task_p->exported_block_size,    /* exported block size */
                                      task_p->exported_opt_block_size,
                                      fbe_raid_group_test_state_get_io_packet_sg(test_state_p),
                                      /* Context is the io cmd. */
                                      iots_p );

    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_raid_group_test_increment_stack_level(packet_p);

    /* Setup io packet sg.
     */
    fbe_raid_group_test_setup_sg_list( fbe_raid_group_test_state_get_io_packet_sg(test_state_p),
                                       fbe_raid_group_test_state_get_io_packet_sg_memory(test_state_p),
                                       (fbe_u32_t)(task_p->blocks * task_p->exported_block_size), /* Bytes */
                                       /* Max bytes per sg entries */
                                       (fbe_u32_t)fbe_raid_group_test_get_max_blks_per_sg(test_state_p) );

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_raid_group_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Get the block operation.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Initialize the iots from the io packet.
     */
    status = fbe_raid_iots_init(iots_p, packet_p, block_operation_p, raid_geometry_p, 
                                task_p->lba, task_p->blocks);

    /* Call generate to init the remainder of the fields.
     */
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, 
                             "mirror: bad status from fbe_raid_iots_init");

    MUT_ASSERT_TRUE_MSG(fbe_raid_iots_get_state(iots_p) == fbe_raid_iots_generate_siots,
                        "mirror: generate start state unexpected");

    status = fbe_raid_siots_embedded_init(siots_p, iots_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "mirror: bad status from fbe_raid_siots_embedded_init");
    
    /* Start the write and confirm that we transition to state0.
    */
    raid_state_status = fbe_mirror_generate_start(siots_p);

    MUT_ASSERT_INT_EQUAL_MSG(raid_state_status, FBE_RAID_STATE_STATUS_EXECUTING, 
                             "mirror: bad status from generate");
    MUT_ASSERT_TRUE_MSG((siots_p->common.state == (fbe_raid_common_state_t)fbe_mirror_write_state0), 
                        "mirror: bad status from generate");
    MUT_ASSERT_TRUE_MSG((siots_p->algorithm == MIRROR_WR), "mirror: bad status from generate");

    return FBE_STATUS_OK;
}
/* end fbe_mirror_test_setup_read() */

/*!***************************************************************
 *      fbe_mirror_test_write_state0()
 ****************************************************************
 * @brief
 *  This function tests write state machine state0 when the command is aligned.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                         Other values imply there was an error.
 *
 * @author:
 *  09/11/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_test_write_state0(fbe_raid_group_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status;
    fbe_packet_t *packet_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;

    packet_p = fbe_raid_group_test_state_get_io_packet(test_state_p);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    MUT_ASSERT_NOT_NULL( packet_p );
    MUT_ASSERT_NOT_NULL( siots_p );

    /* Call state 0 with our io command.
     */
    state_status = fbe_mirror_write_state0(siots_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_RAID_STATE_STATUS_EXECUTING,
                        "mirror: return status not executing");
    MUT_ASSERT_TRUE_MSG(fbe_raid_siots_get_state(siots_p) == fbe_mirror_write_state1,
                        "mirror: siots state unexpected");
    return status;  
}
/* end fbe_mirror_test_write_state0() */

/*!***************************************************************
 * fbe_mirror_test_write_state_machine_test_case()
 ****************************************************************
 * @brief
 *  This function executes tests of the write state machine.
 *
 * @param task_p - The test task to execute for the write state machine.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author:
 *  09/11/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_test_write_state_machine_test_case(fbe_raid_group_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the read state object. 
     */
    fbe_raid_group_test_state_t * const test_state_p = fbe_raid_group_test_get_state_object();

    /* Make sure we can setup a read and destroy it.
     */
    status = fbe_mirror_test_setup_write(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_raid_group_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    fbe_raid_group_test_execute_state_machine(test_state_p,
                                              fbe_mirror_test_setup_write,
                                              fbe_mirror_test_write_state0,
                                              fbe_raid_group_test_state_destroy,
                                              task_p);
    return status;
}
/* end fbe_mirror_test_write_state_machine_test_case() */

/*!***************************************************************
 *          fbe_mirror_test_write_state_machine_tests()
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param  - None
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author
 *  09/11/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_test_write_state_machine_tests(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Test all the raid mirror object read cases for these test tasks.
     */
    status = fbe_raid_group_state_machine_tests(fbe_mirror_test_common_io_tasks,
                                                fbe_mirror_test_write_state_machine_test_case);
    
    return status;
}
/* end fbe_mirror_test_write_state_machine_tests() */

/*!**************************************************************
 * fbe_mirror_test_write_add_unit_tests()
 ****************************************************************
 * @brief
 *  Add the write state machine tests to the current suite.
 *
 * @param  suite_p - suite to add the tests to.               
 *
 * @return None   
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_mirror_test_write_add_unit_tests(mut_testsuite_t * const suite_p)
{
    MUT_ADD_TEST(suite_p, 
                 fbe_mirror_test_write_state_machine_tests,
                 fbe_mirror_unit_tests_setup, 
                 fbe_mirror_unit_tests_teardown); /* SAFEBUG *//*RCA-NPTF*/
    return;
}
/******************************************
 * end fbe_mirror_test_write_add_unit_tests()
 ******************************************/


/*********************************
 * end file fbe_mirror_test_write.c
 *********************************/


