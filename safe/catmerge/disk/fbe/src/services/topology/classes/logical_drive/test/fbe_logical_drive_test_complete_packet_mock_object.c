/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_complete_packet_mock_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's complete
 *  packet mock object.
 * 
 *  This object allows our testing code to "mock" the completion and thus test
 *  code that has calls to our completion function.
 *
 * @version
 *   10/17/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_logical_drive_test_complete_packet_mock_object.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/*! @var fbe_ldo_test_complete_packet_mock 
 *  @brief This is our concrete mock object this is used for completing packets.
 *         The getter function below @ref fbe_ldo_test_get_complete_packet_mock
 *         should be used for all accesses to this object.
 */
static fbe_ldo_test_complete_packet_mock_t fbe_ldo_test_complete_packet_mock;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_test_get_complete_packet_mock(void)
 ****************************************************************
 * @brief
 *  This function is a getter for the global instance of the
 *  complete packet mock object.
 *
 * @param - None.
 *
 * @return
 *  fbe_ldo_test_complete_packet_mock_t * This is the pointer to
 *  the single instance of the mock object.
 *
 * @author
 *  10/17/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_test_complete_packet_mock_t * fbe_ldo_test_get_complete_packet_mock(void)
{
    return &fbe_ldo_test_complete_packet_mock;
}
/**************************************
 * end fbe_ldo_test_get_complete_packet_mock()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_complete_packet_mock(fbe_packet_t *io_packet_p)
 ****************************************************************
 * @brief
 *  This function is the mock for the completion of a packet.
 *
 *  Here we will validate certain conditions and save information
 *  on the packet that is being completed.
 *
 * @param io_packet_p - io packet that is being completed.
 *                      We intercept this completion and save values from the
 *                      packet..
 *
 * @return
 *  FBE_STATUS_PENDING
 *
 * @author
 *  10/17/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_complete_packet_mock(fbe_packet_t *io_packet_p)
{    
    fbe_ldo_test_complete_packet_mock_t *mock_p = fbe_ldo_test_get_complete_packet_mock();
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    /* Now validate the input param.
     */
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");

    /* Get our payload and sg ptr.
     */
    payload_p = fbe_transport_get_payload_ex(io_packet_p);
    MUT_ASSERT_NOT_NULL_MSG(payload_p, "payload should be set for mock complete.");
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    MUT_ASSERT_NOT_NULL_MSG(block_operation_p, 
                            "block operation should be set for mock complete.");

    /* Set the appropriate status into the io packet.
     */
    mock_p->packet_status = fbe_transport_get_status_code(io_packet_p);
    mock_p->packet_status_qualifier = fbe_transport_get_status_qualifier(io_packet_p);

    /* Increment the counter so we know a callback occurred.
     */
    mock_p->packet_completed_count++;
    return FBE_STATUS_PENDING;
}
/* end fbe_ldo_complete_packet_mock() */

/*!***************************************************************
 * @fn fbe_ldo_complete_packet_mock_init(
 *             fbe_ldo_test_complete_packet_mock_t * const mock_p)
 ****************************************************************
 * @brief
 *  This function initializes our mock object for completing packets.
 *  We setup these values to defaults.
 * 
 *  Most important we set the packet completed count to 0 so that it
 *  is clear that no packets have been completed.
 * 
 *  This count and the saved status values will get validated by the
 *  user of the mock.
 *
 * @param mock_p - Mock object pointer that we will initialize.
 *                 No assumptions are made about the contents of the object.
 *
 * @return
 *  None.
 *
 * @author
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_complete_packet_mock_init(fbe_ldo_test_complete_packet_mock_t * const mock_p)
{
    mock_p->packet_completed_count = 0;
    mock_p->packet_status = FBE_STATUS_INVALID;
    mock_p->packet_status_qualifier = 0;
    return;
}
/* end fbe_ldo_complete_packet_mock_init */

/*****************************************************
 * end fbe_logical_drive_test_complete_packet_mock_object.c
 *****************************************************/
