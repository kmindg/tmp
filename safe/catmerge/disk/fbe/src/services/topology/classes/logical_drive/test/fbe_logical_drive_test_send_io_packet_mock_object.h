#ifndef FBE_LOGICAL_DRIVE_TEST_SEND_IO_PACKET_MOCK_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_SEND_IO_PACKET_MOCK_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_send_io_packet_mock_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definition for the logical drive object's
 *  allocate memory mock object.
 *
 * HISTORY
 *   12/03/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"

/*! @struct fbe_ldo_test_send_io_packet_mock_t 
 *  @brief This object is used for testing the send io packet interface.
 *         This object contains useful information that is received when the
 *         send packet function is called.
 *  
 *         The client later will validate this information to make sure that
 *         when the send packet mock was called the appropriate values were
 *         received.
 */
typedef struct fbe_ldo_test_send_io_packet_mock_s
{
    fbe_u32_t packet_sent_count;
    fbe_status_t packet_status_to_set;
    fbe_u32_t packet_status_qualifier_to_set;
    fbe_u32_t sg_entries;
    fbe_u32_t sg_list_bytes;
    fbe_u32_t io_packet_bytes;
    fbe_logical_drive_io_cmd_t *io_cmd_p;

    /* State of the io cmd at the time the mock was called.
     */
    fbe_ldo_io_cmd_state_t io_cmd_state;
}
fbe_ldo_test_send_io_packet_mock_t;

/*************************
 *   API FUNCTIONS
 *************************/
fbe_ldo_test_send_io_packet_mock_t * fbe_ldo_test_get_send_packet_mock(void);

fbe_status_t fbe_ldo_send_io_packet_mock(fbe_logical_drive_t * const logical_drive_p,
                                         fbe_packet_t *io_packet_p);

void fbe_ldo_send_io_packet_mock_init(fbe_ldo_test_send_io_packet_mock_t *const mock_p,
                                      fbe_logical_drive_io_cmd_t *const io_cmd_p,
                                      fbe_status_t status, 
                                      fbe_u32_t qualifier);

#endif /* FBE_LOGICAL_DRIVE_TEST_SEND_IO_PACKET_MOCK_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_send_io_packet_mock_object.h
 *****************************************************/
