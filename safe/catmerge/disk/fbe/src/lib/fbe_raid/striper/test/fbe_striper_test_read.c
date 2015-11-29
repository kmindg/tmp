/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_striper_test_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the striper read state machine.
 *
 * @version
 *   7/30/2009:  Created. Rob Foley
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
#include "fbe_striper_test_prototypes.h"
#include "fbe_striper_io_private.h"
#include "mut.h"


/*************************
 *   Globals
 *************************/
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 *      fbe_striper_test_initialize_striper_object()
 ****************************************************************
 * @brief   Initialize the mirror object for use in a testing 
 *          environment that doesn't setup the entire fbe infrastrcture.
 * 
 * @param   geometry_p - Pointer to raid geometry to populate
 * @param   block_edge_p - Pointer to block edge for this raid group
 * @param   width - The width of this raid group
 * @param   capacity - Exported capacity of this raid group
 *
 * @return   The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  07/30/2009  Rob Foley  - Created from fbe_raid_group_test_initialize_mirror
 *
 ****************************************************************/
fbe_status_t fbe_striper_test_initialize_striper_object(fbe_raid_geometry_t *raid_geometry_p,
                                                        fbe_block_edge_t *block_edge_p,
                                                        fbe_u32_t width,
                                                        fbe_lba_t capacity)
{
    fbe_raid_library_test_initialize_geometry(raid_geometry_p,
                                              block_edge_p, 
                                              FBE_RAID_GROUP_TYPE_RAID0,
                                              width,  
                                              FBE_RAID_SECTORS_PER_ELEMENT,
                                              0,
                                              capacity,
                                              (fbe_raid_common_state_t)fbe_striper_generate_start);

    return FBE_STATUS_OK;
}
/* end fbe_striper_test_initialize_striper_object() */

/*!***************************************************************
 * fbe_striper_test_setup_read
 ****************************************************************
 * @brief
 *  This function sets up for a read state machine test.
 *
 * @param test_state_p - The test state object to use in the test.
 * @param task_p - The test task to use.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author:
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_striper_test_setup_read(fbe_raid_group_test_state_t * const test_state_p,
                                         fbe_raid_group_test_state_machine_task_t *const task_p)
{
    fbe_raid_state_status_t raid_state_status = FBE_RAID_STATE_STATUS_INVALID;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_test_state_get_raid_geometry(test_state_p);

    /* Init the striper.
     */
    fbe_striper_test_initialize_striper_object(raid_geometry_p,
                                               fbe_raid_group_test_state_get_block_edge(test_state_p), 
                                               task_p->width, 
                                               task_p->capacity);

    /* Init the state object.
     */
    fbe_raid_test_state_setup(test_state_p, task_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);

    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);
    MUT_ASSERT_NOT_NULL_MSG( siots_p, "siots is NULL");

    /* Generate the read.
     */
    raid_state_status = fbe_striper_generate_start(siots_p);   
    MUT_ASSERT_INT_EQUAL_MSG(raid_state_status, FBE_RAID_STATE_STATUS_EXECUTING, 
                             "striper: bad status from generate");
    MUT_ASSERT_TRUE_MSG((siots_p->common.state == (fbe_raid_common_state_t)fbe_striper_read_state0), 
                        "striper: bad status from generate");
    MUT_ASSERT_TRUE_MSG((siots_p->algorithm == RAID0_RD), "striper: bad status from generate");

    return FBE_STATUS_OK;
}
/* end fbe_striper_test_setup_read() */

/*!***************************************************************
 * fbe_striper_test_read_state0()
 ****************************************************************
 * @brief
 *  This function tests read state machine state0 when the command is aligned.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                         Other values imply there was an error.
 *
 * @author:
 *  07/30/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_striper_test_read_state0(fbe_raid_group_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status;
    fbe_packet_t *packet_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;

    packet_p = fbe_raid_group_test_state_get_io_packet(test_state_p);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    MUT_ASSERT_NOT_NULL( packet_p );
    MUT_ASSERT_NOT_NULL( siots_p );

    /* Call state 10 with our io command.
     */
    state_status = fbe_striper_read_state0(siots_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_RAID_STATE_STATUS_EXECUTING,
                        "striper: return status not executing");
    MUT_ASSERT_TRUE_MSG(fbe_raid_siots_get_state(siots_p) == fbe_striper_read_state3,
                        "striper: siots state unexpected");
    return status;  
}
/* end fbe_striper_test_read_state0() */

/*!***************************************************************
 * fbe_striper_test_state_destroy()
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
fbe_status_t fbe_striper_test_state_destroy(fbe_raid_group_test_state_t * const state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_test_state_destroy(state_p);

    return status;
}
/* end fbe_striper_test_state_destroy() */

/*!***************************************************************
 * fbe_striper_test_read_state_machine_test_case()
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param task_p - The test task to execute for the read state machine.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author:
 *  07/30/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_test_read_state_machine_test_case(fbe_raid_group_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the read state object. 
     */
    fbe_raid_group_test_state_t * const test_state_p = fbe_raid_group_test_get_state_object();

    /* Make sure we can setup a read and destroy it.
     */
    status = fbe_striper_test_setup_read(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_striper_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    fbe_raid_group_test_execute_state_machine(test_state_p,
                                              fbe_striper_test_setup_read,
                                              fbe_striper_test_read_state0,
                                              fbe_striper_test_state_destroy,
                                              task_p);
    return status;
}
/* end fbe_striper_test_read_state_machine_test_case() */

/* This structure contains the test cases for testing a 
 * range of I/Os, each with a specific block size configuration. 
 *  
 * This table is expected to be used by integration style I/O 
 * tests which are trying to test the logical drive. 
 */
fbe_raid_group_test_io_range_t fbe_raid_group_test_io_tasks[] = 
{
    /* Relatively short run tests.
     * These cases attempt to test a reasonable sub-set of the 
     * possible test cases. 
     * First we always test with the exported block size of 520 
     * since this is what the system currently uses. 
     *  
     * We also choose a reasonable set of start lba and block 
     * counts to cover.  When choosing lbas and block counts we 
     * try to cover a full optimal block for each of the tests. 
     */
    /*
     * start end     start  end     exp  exp  imp  imp  Inc  
     * lba   lba     blocks blocks  blk  opt  blk  opt  blks
     *                                   blk  size blk
     */
    /* These are lossy cases for 512 imported. 
     * We cover several full optimal blocks for each of these cases.
     */
    {0,      100,    0x1,   100,    520, 2,   512,  3,  1, 1, 16, 128, 128},
    /* Insert any new test cases above this point.
     * FBE_RAID_GROUP_TEST_INVALID_FIELD, means end
     */  
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0} 
};
/*!***************************************************************
 * @fn fbe_striper_test_read_state_machine_tests(void)
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
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_test_read_state_machine_tests(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Test all the logical drive read cases for these test tasks.
     */
    status = fbe_raid_group_state_machine_tests(fbe_raid_group_test_io_tasks,
                                                fbe_striper_test_read_state_machine_test_case);
    
    return status;
}
/* end fbe_striper_test_read_state_machine_tests() */

/*!**************************************************************
 * fbe_striper_test_read_add_unit_tests()
 ****************************************************************
 * @brief
 *  Add the read state machine tests to the current suite.
 *
 * @param  suite_p - suite to add the tests to.               
 *
 * @return None   
 *
 * @author
 *  7/30/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_striper_test_read_add_unit_tests(mut_testsuite_t * const suite_p)
{
    MUT_ADD_TEST(suite_p, 
                 fbe_striper_test_read_state_machine_tests,
                 fbe_striper_unit_tests_setup, 
                 fbe_striper_unit_tests_teardown);/* SAFEBUG *//*RCA-NPTF*/
    return;
}
/******************************************
 * end fbe_striper_test_read_add_unit_tests()
 ******************************************/
/*************************
 * end file fbe_striper_test_read.c
 *************************/


