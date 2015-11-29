/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_unit_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main function for the parity unit tests.
 *
 * @revision
 *   9/1/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library_test_proto.h"
#include "mut.h"
#include "mut_assert.h"
#include "fbe_parity_test_private.h"

/******************************
 ** Imported functions.
 ******************************/
 
/*!***************************************************************
 * @fn fbe_parity_unit_tests_setup(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_parity_unit_tests_setup(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_transport_init();
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);

    /* Must configure the raid memory before initializing the raid library
     */
    fbe_raid_group_test_init_memory();

    /* Now initialize the raid library*/
    status = fbe_raid_library_initialize(FBE_RAID_LIBRARY_DEBUG_FLAG_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/* end fbe_parity_unit_tests_setup() */

/*!***************************************************************
 * @fn fbe_parity_unit_tests_teardown(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_parity_unit_tests_teardown(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_transport_destroy();
    /* Set the trace level back to the default.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_MEDIUM);

    /*! @note We might have modified the FBE infrastructure current methods, 
     * let's set them back to the defaults. 
     */
    //fbe_raid_group_reset_methods();

    /*! @note Make sure the handle count did not grow.  This would indicate that we 
     * leaked something. 
     */
    //fbe_raid_group_check_handle_count();

    status = fbe_raid_library_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_raid_group_test_destroy_memory();
    return;
}
/* end fbe_parity_unit_tests_teardown() */

/*!***************************************************************
 * fbe_parity_add_unit_tests()
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @param suite_p - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  05/21/09 - Created. RPF
 *
 ****************************************************************/
void fbe_parity_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);
    fbe_parity_test_generate_add_tests(suite_p);
    return;
}
/* end fbe_parity_add_unit_tests() */


/*****************************************
 * end fbe_parity_unit_test.c
 *****************************************/
