/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the striper object.
 *
 * @brief
 *   07/30/2009: Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#define I_AM_NATIVE_CODE
#include <windows.h>
#include "fbe/fbe_winddk.h"

#include "fbe_trace.h"
#include "fbe_transport_memory.h"

//#include "fbe_striper_test_private.h"
#include "mut.h"
#include "fbe_interface.h"
#include "fbe_raid_library_test_state_object.h"
#include "fbe_raid_library_proto.h"
#include "fbe_striper_test_prototypes.h"


/******************************
 ** LOCAL FUNCTION PROTOTYPES *
 ******************************/

/*!**************************************************************
 * @fn fbe_striper_test_lifecycle()
 ****************************************************************
 * @brief
 *  Validates the logical drive's lifecycle.
 *
 * @param - none.          
 *
 * @return none
 *
 * @author
 *  7/16/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_striper_test_lifecycle(void)
{
    fbe_status_t status;
    fbe_status_t fbe_striper_monitor_load_verify(void);

    /* Make sure this data is valid.
     */
    status = fbe_striper_monitor_load_verify();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_striper_test_lifecycle()
 ******************************************/

/*!***************************************************************
 * @fn fbe_striper_unit_tests_setup(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
void fbe_striper_unit_tests_setup(void)
{
    fbe_status_t status;
    fbe_transport_init();

    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_ERROR);

    /* Must configure the raid memory before initializing the raid library
     */
    fbe_raid_group_test_init_memory();

    /* Now initialize the raid library*/
    status = fbe_raid_library_initialize(FBE_RAID_LIBRARY_DEBUG_FLAG_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/* end fbe_striper_unit_tests_setup() */

/*!***************************************************************
 * @fn fbe_striper_unit_tests_teardown(void)
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * @author
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
void fbe_striper_unit_tests_teardown(void)
{
    fbe_status_t status;
    fbe_transport_destroy();
    /* Set the trace level back to the default.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_MEDIUM);

    /* We might have modified the logical drive's current methods, 
     * let's set them back to the defaults. 
     */
    //fbe_striper_reset_methods();

    status = fbe_raid_library_destroy();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_raid_group_test_destroy_memory();

    return;
}
/* end fbe_striper_unit_tests_teardown() */

/****************************************************************
 * fbe_striper_add_unit_tests()
 ****************************************************************
 * @brief
 *  This function adds unit tests to the input suite.
 *
 * PARAMETERS:
 *  suite_p, [I] - Suite to add to.
 *
 * @return
 *  None.
 *
 * @author
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_striper_add_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Turn down the warning level to limit the trace to the display.
     */
    //fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    fbe_striper_test_read_add_unit_tests(suite_p);
    fbe_striper_test_generate_add_unit_tests(suite_p);
    return;
}
/* end fbe_striper_add_unit_tests() */


/*****************************************
 * end fbe_striper_test.c
 *****************************************/
