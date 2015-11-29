/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_send_io_packet_mock_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  send io packet mock object.
 *
 * HISTORY
 *   12/03/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe_logical_drive_test_send_io_packet_mock_object.h"
#include "mut.h"

/*************************
 *   GLOBALS
 *************************/

/*! @var fbe_ldo_test_send_packet_mock 
 *  @brief This is our concrete mock object this is used for sending io packets.
 */
static fbe_ldo_test_send_io_packet_mock_t fbe_ldo_test_send_packet_mock;

/* Imported function.
 * We need this include since we can't include the prototypes.h file. 
 */
void fbe_ldo_test_increment_stack_level(fbe_packet_t *const packet_p);

/*!**************************************************************
 * @fn fbe_ldo_test_get_send_packet_mock(void)
 ****************************************************************
 * @brief
 *  Return the global instance of this send packet mock object.
 * 
 * @return fbe_ldo_test_send_io_packet_mock_t*
 * 
 * HISTORY:
 *  11/21/2008 - Created. RPF
 *
 ****************************************************************/

fbe_ldo_test_send_io_packet_mock_t * fbe_ldo_test_get_send_packet_mock(void)
{
    return &fbe_ldo_test_send_packet_mock;
}
/**************************************
 * end fbe_ldo_test_get_send_packet_mock()
 **************************************/


/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_send_io_packet_mock(
 *                fbe_logical_drive_t * const logical_drive_p,
 *                fbe_packet_t *io_packet_p)
 ****************************************************************
 * @brief
 *  This function is the mock for the send of an io packet.
 *
 *  Here we will validate certain conditions and save information
 *  on the io packet we received.
 *
 *  We also setup the return status from the values in the mock.
 *
 * @param logical_drive_p - Logical drive to send for.
 * @param io_packet_p - io packet to send.
 *
 * @return
 *  FBE_STATUS_PENDING.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_send_io_packet_mock(fbe_logical_drive_t * const logical_drive_p,
                                         fbe_packet_t *io_packet_p)
{    
    fbe_u32_t io_packet_bytes;
    fbe_u32_t sg_entries;
    fbe_ldo_test_send_io_packet_mock_t *mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_sg_element_t *sg_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_sg_descriptor_t *sg_desc_p = NULL;

    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");

    /* We need to increment the stack level for this packet since 
     * normally this is something the transport does. 
     */
    fbe_ldo_test_increment_stack_level(io_packet_p);

    /* Get our payload and sg ptr.
     */
    payload_p = fbe_transport_get_payload_ex(io_packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    /* Now validate the input params.
     */
    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");

    io_packet_bytes = (fbe_u32_t) (ldo_packet_get_block_cmd_blocks(io_packet_p) *
                                   ldo_packet_get_block_cmd_block_size(io_packet_p));
    mock_p->io_packet_bytes = io_packet_bytes;

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_sg_descriptor(block_operation_p, &sg_desc_p);

    if (sg_desc_p->repeat_count == 1)
    {
        /* If there is no repeat count, then we can count this list.
         */
        sg_entries = fbe_sg_element_count_entries(sg_p);
        
        /* Save the sg entry count, total sg bytes, and transfer bytes for later validation.
         */
        mock_p->sg_entries = sg_entries;
        mock_p->sg_list_bytes = 
            fbe_sg_element_count_list_bytes(sg_p);
    }

    /* Set the appropriate status into the io packet.
     */
    fbe_transport_set_status(io_packet_p, 
                             mock_p->packet_status_to_set, 
                             mock_p->packet_status_qualifier_to_set);

    /* Save the state of the io cmd. The io_cmd_p is an optional argument to the
     * constructor, so only save the state if it is not NULL. 
     * We can use this to validate the io cmd was in the right state when the io 
     * packet was sent. 
     */
    if (mock_p->io_cmd_p != NULL)
    {
        mock_p->io_cmd_state = fbe_ldo_io_cmd_get_state(mock_p->io_cmd_p);
    }

    /* Increment the counter so we know a callback occurred.
     */
    mock_p->packet_sent_count++;
    return FBE_STATUS_PENDING;
}
/* end fbe_ldo_send_io_packet_mock() */

/*!***************************************************************
 * @fn fbe_ldo_send_io_packet_mock_init(
 *           fbe_ldo_test_send_io_packet_mock_t *const mock_p,
 *           fbe_logical_drive_io_cmd_t *const io_cmd_p,
 *           fbe_status_t status,
 *           fbe_u32_t qualifier)
 ****************************************************************
 * @brief
 *  This function initializes our mock object for sending io packets.
 *
 * @param mock_p - Mock object pointer.
 * @param io_cmd_p - The io cmd to use for this mock. 
 *                   We use the io cmd in the callback in order
 *                   to get the io command's state.
 * @param status - Status to set on the io packet.
 * @param qualifier - Qualifier to set on this io packet.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_send_io_packet_mock_init(fbe_ldo_test_send_io_packet_mock_t * const mock_p,
                                      fbe_logical_drive_io_cmd_t *const io_cmd_p,
                                      fbe_status_t status, 
                                      fbe_u32_t qualifier)
{
    mock_p->packet_sent_count = 0;
    mock_p->packet_status_to_set = status;
    mock_p->packet_status_qualifier_to_set = qualifier;
    mock_p->io_cmd_p = io_cmd_p;
    return;
}
/* end fbe_ldo_send_io_packet_mock_init */

/*****************************************************
 * end fbe_logical_drive_test_send_io_packet_mock_object.c
 *****************************************************/
