/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_alloc_io_cmd_mock_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  allocate io command mock object.
 *
 * HISTORY
 *   11/05/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_logical_drive_test_alloc_io_cmd_mock_object.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/*! @var fbe_ldo_test_alloc_io_cmd_mock_object
 *  @brief This is our concrete mock object this is used for allocating io cmds.
 */
static fbe_ldo_test_alloc_io_cmd_mock_t fbe_ldo_test_alloc_io_cmd_mock_object;

/*! @fn fbe_ldo_test_get_alloc_io_cmd_mock
 *  @brief Accessor to get the mock object for allocation of memory.
 */
fbe_ldo_test_alloc_io_cmd_mock_t * fbe_ldo_test_get_alloc_io_cmd_mock(void)
{
    return &fbe_ldo_test_alloc_io_cmd_mock_object;
}

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_alloc_io_cmd_mock_validate(fbe_ldo_test_alloc_io_cmd_mock_t *const mock_p)
 ****************************************************************
 * @brief
 *  This function of the ldo allocate mock validates if the
 *  values in the mock are correct.
 *
 * @param mock_p, [I] - The mock object to validate.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_alloc_io_cmd_mock_validate(fbe_ldo_test_alloc_io_cmd_mock_t *const mock_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Check that all of the values were filled in when we were called back.
     */
    MUT_ASSERT_TRUE_MSG(mock_p->completion_function != NULL,
                        "LDO: completion function not set");
    MUT_ASSERT_TRUE_MSG(mock_p->completion_context != NULL,
                        "LDO: completion context not set");
    MUT_ASSERT_TRUE_MSG(mock_p->memory_request_p != NULL,
                        "LDO: memory_request not set");

    /* Check that we were called back only once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->allocate_count, 
                             1,
                             "LDO: allocate memory mock not called"); 
    return status;
}
/* end fbe_ldo_alloc_io_cmd_mock_validate() */

/*!***************************************************************
 * @fn fbe_ldo_alloc_io_cmd_mock_validate_not_called(
 *      fbe_ldo_test_alloc_io_cmd_mock_t *const mock_p)
 ****************************************************************
 * @brief
 *  This function of the ldo allocate mock validates if the
 *  mock was not called.
 *
 *  The mock should just appear initialized.
 *
 * @param mock_p - The mock object to validate.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_alloc_io_cmd_mock_validate_not_called(
    fbe_ldo_test_alloc_io_cmd_mock_t *const mock_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (mock_p->completion_function != NULL)
    {
        mut_printf(MUT_LOG_HIGH, "LDO: completion function set unexpectedly %d function: %s\n", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else if (mock_p->completion_context != 0)
    {
        mut_printf(MUT_LOG_HIGH, "LDO: completion context set unexpectedly %d function: %s\n", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else if (mock_p->memory_request_p != NULL)
    {
        mut_printf(MUT_LOG_HIGH, "LDO: memory request set unexpectedly %d function: %s\n", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    /* Make sure we were not called. 
     */
    else if (mock_p->allocate_count != 0)
    {
        mut_printf(MUT_LOG_HIGH, "LDO: mock called unexpectedly %d function: %s\n", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/* end fbe_ldo_alloc_io_cmd_mock_validate_not_called() */

/*!***************************************************************
 * @fn fbe_ldo_alloc_io_cmd_mock_init(fbe_ldo_test_alloc_io_cmd_mock_t * const mock_p)
 ****************************************************************
 * @brief
 *  This function initializes our mock object for sending io packets.
 *
 * @param mock_p - Mock object pointer.
 *
 * @return - None.
 *
 * HISTORY:
 *  11/05/08 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_alloc_io_cmd_mock_init(fbe_ldo_test_alloc_io_cmd_mock_t * const mock_p)
{
    mock_p->allocate_count = 0;
    mock_p->memory_request_p = NULL;
    mock_p->completion_function = NULL;
    mock_p->completion_context = 0;
    return;
}
/* end fbe_ldo_alloc_io_cmd_mock_init */

/*!***************************************************************
 * @fn fbe_ldo_test_alloc_io_cmd_mock(
 *           fbe_packet_t *const packet_p,
 *           fbe_memory_completion_function_t completion_function,
 *           fbe_memory_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function is our mock io cmd allocate function.
 *  This function gets set into the function table for the logical
 *  drive and will get called instead of the normal io cmd.
 *
 * @param packet_p, - Packet to allocate for.
 * @param completion_function - Fn to call when mem is
 *                              available.
 * @param completion_context - Context to use when mem is
 *                             available.
 *
 * @return fbe_status_t
 *  FBE_STATUS_OK
 *
 * HISTORY:
 *  11/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_alloc_io_cmd_mock(fbe_packet_t *const packet_p,
                                            fbe_memory_completion_function_t completion_function,
                                            fbe_memory_completion_context_t completion_context)
{
    /* Get our pointer to the mock object.
     */
    fbe_memory_request_t *memory_request_p = 
        fbe_transport_get_memory_request(packet_p);
    fbe_ldo_test_alloc_io_cmd_mock_t * mock_p = 
        fbe_ldo_test_get_alloc_io_cmd_mock();
    
    /* Save the input params for later validation.
     */
    mock_p->memory_request_p = memory_request_p;
    mock_p->completion_function = completion_function;
    mock_p->completion_context = completion_context;

    /* Increment to indicate that our mock was called.
     */
    mock_p->allocate_count++;

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_alloc_io_cmd_mock() */

/*****************************************************
 * end fbe_logical_drive_test_alloc_io_cmd_mock_object.c
 *****************************************************/
