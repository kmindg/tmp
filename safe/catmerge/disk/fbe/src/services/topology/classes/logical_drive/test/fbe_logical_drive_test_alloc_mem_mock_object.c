
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_test_alloc_mem_mock_object.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains testing functions for the logical drive object's
 *  allocate memory mock object.
 *
 * HISTORY
 *   11/28/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_logical_drive_test_alloc_mem_mock_object.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/* This is our concrete mock object this is used for sending io packets.
 */
static fbe_ldo_test_alloc_io_packet_mock_t fbe_ldo_test_alloc_packet_mock;

/* Accessor to get the mock object for allocation of memory.
 */
fbe_ldo_test_alloc_io_packet_mock_t * fbe_ldo_alloc_mem_mock_get_object(void)
{
    return &fbe_ldo_test_alloc_packet_mock;
}

/*************************
 *   FUNCTIONS
 *************************/

/****************************************************************
 * fbe_ldo_alloc_mem_mock_validate()
 ****************************************************************
 * DESCRIPTION:
 *  This function of the ldo allocate mock validates if the
 *  values in the mock are correct..
 *
 * PARAMETERS:
 *  mock_p, [I] - The mock object to validate.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/27/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_alloc_mem_mock_validate(fbe_ldo_test_alloc_io_packet_mock_t *const mock_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Check that all of the values were filled in when we were called back.
     */
    MUT_ASSERT_TRUE_MSG(mock_p->completion_function != NULL,
                        "LDO: completion function not set");
    MUT_ASSERT_TRUE_MSG(mock_p->completion_context != NULL,
                        "LDO: completion context not set");
    MUT_ASSERT_TRUE_MSG(mock_p->memory_request_p != NULL,
                        "LDO: memory request not allocated.");

    /* Check that we were called back only once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->allocation_callback_count, 
                             1,
                             "LDO: allocate memory mock not called"); 
    return status;
}
/* end fbe_ldo_alloc_mem_mock_validate() */

/****************************************************************
 * fbe_ldo_alloc_mem_mock_validate_not_called()
 ****************************************************************
 * DESCRIPTION:
 *  This function of the ldo allocate mock validates if the
 *  values in the mock are correct for the case where
 *  the mock was not called.
 *
 *  The mock should just appear initialized.
 *
 * PARAMETERS:
 *  mock_p, [I] - The mock object to validate.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/27/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_alloc_mem_mock_validate_not_called(
    fbe_ldo_test_alloc_io_packet_mock_t *const mock_p)
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
    else if (mock_p->allocation_callback_count != 0)
    {
        mut_printf(MUT_LOG_HIGH, "LDO: mock called unexpectedly %d function: %s\n", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/* end fbe_ldo_alloc_mem_mock_validate_not_called() */

/****************************************************************
 * fbe_ldo_alloc_mem_mock_init()
 ****************************************************************
 * DESCRIPTION:
 *  This function initializes our mock object for sending io packets.
 *
 * PARAMETERS:
 *  mock_p, [I] - Mock object pointer.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_alloc_mem_mock_init(fbe_ldo_test_alloc_io_packet_mock_t * const mock_p)
{
    mock_p->allocation_callback_count = 0;
    mock_p->memory_request_p = NULL;
    mock_p->completion_function = NULL;
    mock_p->completion_context = 0;
    return;
}
/* end fbe_ldo_alloc_mem_mock_init */

/****************************************************************
 * fbe_ldo_alloc_mem_mock_fn()
 ****************************************************************
 * DESCRIPTION:
 *  This function is our mock io packet allocate function.
 *  This will get setup into the logical drive object and
 *  will be called when a logical drive needs to send an io packet.
 *
 * PARAMETERS:
 *  io_packet_p_p, [I] - Packet to allocate for.
 *  completion_function, [I] - Fn to call when mem is available.
 *  completion_context, [I] - Context to use when mem is available.
 *
 * RETURNS:
 *  FBE_STATUS_OK
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_alloc_mem_mock_fn(fbe_packet_t *io_packet_p,
                                       fbe_memory_completion_function_t completion_function,
                                       fbe_memory_completion_context_t completion_context,
                                       fbe_u32_t number_to_allocate)
{
    /* Get our pointer to the mock object.
     */
    fbe_memory_request_t *memory_request_p = 
        fbe_transport_get_memory_request(io_packet_p);
    fbe_ldo_test_alloc_io_packet_mock_t * mock_p = 
        fbe_ldo_alloc_mem_mock_get_object();
    
    /* Save the input params for later validation.
     */
    mock_p->memory_request_p = memory_request_p;
    mock_p->completion_function = completion_function;
    mock_p->completion_context = completion_context;

    /* Increment to indicate that our mock was called.
     */
    mock_p->allocation_callback_count++;

    return FBE_STATUS_OK;
}
/* end fbe_ldo_alloc_mem_mock_fn() */
/*****************************************************
 * end fbe_logical_drive_test_alloc_mem_mock_object.c
 *****************************************************/
