/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_set_condition_mock_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's set
 *  condition, mock object.
 * 
 *  This object allows our testing code to "mock" the setting of a lifecycle
 *  condition, and thus test code that has calls to the lifecycle set.
 *
 * HISTORY
 *   10/21/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_logical_drive_test_set_condition_mock_object.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/*! @var fbe_ldo_test_set_condition_mock 
 *  @brief This is our concrete mock object this is used for setting
 *         conditions. The getter function below @ref
 *         fbe_ldo_test_get_set_condition_mock should be used for all accesses
 *         to this object.
 */
static fbe_ldo_test_set_condition_mock_t fbe_ldo_test_set_condition_mock;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_test_get_set_condition_mock(void)
 ****************************************************************
 * @brief
 *  This function is a getter for the global instance of the
 *  set condition mock object.
 *
 * @param - None.
 *
 * @return
 *  fbe_ldo_test_set_condition_mock_t * This is the pointer to
 *  the single instance of the mock object.
 *
 * HISTORY:
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_test_set_condition_mock_t * fbe_ldo_test_get_set_condition_mock(void)
{
    return &fbe_ldo_test_set_condition_mock;
}
/**************************************
 * end fbe_ldo_test_get_set_condition_mock()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_set_condition_mock(fbe_lifecycle_const_t * class_const_p,
 *                                struct fbe_base_object_s * object_p,
 *                                fbe_lifecycle_cond_id_t cond_id)
 ****************************************************************
 * @brief
 *  This function is the mock for the setting of a condition.
 *  This allows us to mock the setting of conditions in order
 *  to unit test our lifecycle code.
 * 
 * @param class_const_p - Class constant pointer.
 * @param object_p - Ptr to object to set the condition on.
 * @param cond_id - The condition id being set.
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE
 *
 * HISTORY:
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_set_condition_mock(fbe_lifecycle_const_t * class_const_p,
                                        struct fbe_base_object_s * object_p,
                                        fbe_lifecycle_cond_id_t cond_id)
{    
    fbe_ldo_test_set_condition_mock_t *mock_p = fbe_ldo_test_get_set_condition_mock();

    /* Now validate the input param.
     */
    MUT_ASSERT_NOT_NULL(object_p);
    MUT_ASSERT_NOT_NULL(class_const_p);
    MUT_ASSERT_INT_NOT_EQUAL(cond_id, 0);

    /* Save the input values.
     */
    mock_p->object_p = object_p;
    mock_p->class_const_p = class_const_p; 
    mock_p->cond_id = cond_id;

    /* Increment the counter so we know a callback occurred.
     */
    mock_p->called_count++;

    /* We return failure in order to exercise the code that 
     * tests the return value. 
     */
    return FBE_STATUS_GENERIC_FAILURE;
}
/* end fbe_ldo_set_condition_mock() */

/*!***************************************************************
 * @fn fbe_ldo_set_condition_mock_init(
 *             fbe_ldo_test_set_condition_mock_t * const mock_p)
 ****************************************************************
 * @brief
 *  This function initializes our mock object for setting conditions.
 *  We setup these values to defaults.
 * 
 *  Most important we set the called count to 0 so that it
 *  is set that we have not been called.
 * 
 *  This count and the object_p values will get validated by the
 *  user of the mock.
 *
 * @param mock_p - Mock object pointer that we will initialize.
 *                 No assumptions are made about the contents of the object.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_set_condition_mock_init(fbe_ldo_test_set_condition_mock_t * const mock_p)
{
    mock_p->called_count = 0;
    mock_p->object_p = NULL;
    mock_p->class_const_p = NULL; 
    mock_p->cond_id = 0;
    return;
}
/* end fbe_ldo_set_condition_mock_init */

/*****************************************************
 * end fbe_logical_drive_test_set_condition_mock_object.c
 *****************************************************/
