/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the raid mirror object.
 *
 * @version
 *  09/11/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_mirror.h"
#include "fbe_trace.h"
#include "fbe_transport_memory.h"
#include "mut.h"
#include "fbe_interface.h"
#include "fbe_raid_library_test_state_object.h"
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_test_prototypes.h"

/*************************
 *   Globals
 *************************/

/* This structure contains the test cases for testing a 
 * range of I/Os, each with a specific block size configuration. 
 *  
 * This table is `common' in the fact that it can be used with
 * multiple opcodes.
 */
fbe_raid_group_test_io_range_t fbe_mirror_test_common_io_tasks[] = 
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
     * start end     start  end     exp  exp  imp  imp  Inc  Start End   Start End
     * lba   lba     blocks blocks  blk  opt  blk  opt  blks Width Width Ele   Ele
     *                                   blk  size blk                   Size  Size
     */
    /* These are lossy cases for 512 imported. 
     * We cover several full optimal blocks for each of these cases.
     */
    {0,      100,    0x1,   100,    520, 2,   512,  3,  1,   1,    3,    128,  128},
    /* Insert any new test cases above this point.
     * FBE_RAID_GROUP_TEST_INVALID_FIELD, means end
     */  
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0} 
};

/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/

/*!***************************************************************
 *  fbe_mirror_test_setup()
 ****************************************************************
 * @brief
 *  This function performs generic setup for raid mirror object test suite.  
 *  We setup the framework necessary for testing the mirror by initting the
 *  logical package.  
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_mirror_test_setup(void) 
{
    fbe_status_t status = FBE_STATUS_OK;
    return status;
}
/* end fbe_mirror_test_setup() */

/*!**************************************************************
 *      fbe_mirror_test_lifecycle()
 ****************************************************************
 * @brief
 *  Validates the raid mirror object's lifecycle.
 *
 * @param - none.          
 *
 * @return none
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/

void fbe_mirror_test_lifecycle(void)
{
    fbe_status_t status;
    fbe_status_t fbe_mirror_monitor_load_verify(void);

    /* Make sure this data is valid.
     */
    status = fbe_mirror_monitor_load_verify();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_mirror_test_lifecycle()
 ******************************************/

/*!***************************************************************
 *      fbe_mirror_test_teardown()
 ****************************************************************
 * @brief
 *  This function performs generic cleanup for 
 *  raid mirror object test suite.  We clean up the framework 
 *  necessary for testing the pdo by destroying the
 *  logical package.  
 *
 * @param None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/
fbe_u32_t fbe_mirror_test_teardown(void) 
{
    fbe_status_t status = FBE_STATUS_OK;

    return status;
}
/* end fbe_mirror_test_teardown() */

/*!***************************************************************
 *      fbe_mirror_unit_tests_setup()
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_mirror_unit_tests_setup(void)
{
    fbe_status_t status;
    fbe_transport_init();
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);
    fbe_raid_group_test_init_memory();
    /* Turn down the warning level to limit the trace to the display.
     */
    FBE_ASSERT_AT_COMPILE_TIME(FBE_MIRROR_MIN_WIDTH == 1);
    FBE_ASSERT_AT_COMPILE_TIME(FBE_MIRROR_MAX_WIDTH == 3);
    status = fbe_raid_library_initialize(FBE_RAID_LIBRARY_DEBUG_FLAG_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/* end fbe_mirror_unit_tests_setup() */

/*!***************************************************************
 *  fbe_mirror_unit_tests_teardown()
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  9/11/2009   Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_mirror_unit_tests_teardown(void)
{
    fbe_status_t status;
    fbe_transport_destroy();
    /* Set the trace level back to the default.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_MEDIUM);

    /* We might have modified the logical drive's current methods, 
     * let's set them back to the defaults. 
     */
    //fbe_mirror_reset_methods();

    status = fbe_raid_library_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_raid_group_test_destroy_memory();
    return;
}
/* end fbe_mirror_unit_tests_teardown() */

/*!***************************************************************
 *      fbe_mirror_add_unit_tests()
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @param
 *  suite_p, [I] - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  09/11/2009  Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_mirror_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    fbe_mirror_test_generate_add_unit_tests(suite_p);
    fbe_mirror_test_read_add_unit_tests(suite_p);
    fbe_mirror_test_write_add_unit_tests(suite_p);
    return;
}
/* end fbe_mirror_add_unit_tests() */

/*!***************************************************************
 *      fbe_mirror_test_initialize_mirror_object()
 ****************************************************************
 * @brief   Initialize the mirror object for use in a testing 
 *          environment that doesn't setup the entire fbe infrastrcture.
 *
 * @param   geometry_p - Pointer to raid geometry to populate
 * @param   block_edge_p - Pointer to block edge for this raid group
 * @param   width - The width of this raid group
 * @param   capacity - Exported capacity of this raid group
 *
 * @return  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  09/11/2009  Ron Proulx  - Created from fbe_striper_test_initialize_striper_object
 *
 ****************************************************************/
fbe_status_t fbe_mirror_test_initialize_mirror_object(fbe_raid_geometry_t *geometry_p,
                                                      fbe_block_edge_t *block_edge_p, 
                                                      fbe_u32_t width,
                                                      fbe_lba_t capacity)
{
    fbe_raid_library_test_initialize_geometry(geometry_p,
                                              block_edge_p,
                                              FBE_RAID_GROUP_TYPE_RAID1,
                                              width, 
                                              FBE_RAID_SECTORS_PER_ELEMENT,
                                              1,
                                              capacity,
                                              (fbe_raid_common_state_t)fbe_mirror_generate_start);

    return FBE_STATUS_OK;
}
/* end fbe_mirror_test_initialize_mirror_object() */

/*****************************************
 * end fbe_mirror_test.c
 *****************************************/
