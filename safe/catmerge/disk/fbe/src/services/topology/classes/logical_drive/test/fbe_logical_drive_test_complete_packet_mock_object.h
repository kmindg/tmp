#ifndef FBE_LOGICAL_DRIVE_TEST_COMPLETE_PACKET_MOCK_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_COMPLETE_PACKET_MOCK_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_complete_packet_mock_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's complete
 *  packet mock object.
 * 
 *  This object allows our testing code to "mock" the completion and thus test
 *  code that has calls to our completion function.
 *
 * HISTORY
 *   10/17/2008:  Created. RPF
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

/*! @struct fbe_ldo_test_complete_packet_mock_t 
 *  
 *  @brief This object is used for testing the complete packet interface.
 *         This structure will store the results of the mock operation.
 */
typedef struct fbe_ldo_test_complete_packet_mock_s
{
    fbe_u32_t packet_completed_count;
    fbe_status_t packet_status;
    fbe_u32_t packet_status_qualifier;
}
fbe_ldo_test_complete_packet_mock_t;

/*************************
 *   API FUNCTIONS
 *************************/
fbe_ldo_test_complete_packet_mock_t * fbe_ldo_test_get_complete_packet_mock(void);

fbe_status_t fbe_ldo_complete_packet_mock(fbe_packet_t *io_packet_p);

void fbe_ldo_complete_packet_mock_init(fbe_ldo_test_complete_packet_mock_t *const mock_p);

#endif /* FBE_LOGICAL_DRIVE_TEST_complete_packet_MOCK_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_complete_packet_mock_object.h
 *****************************************************/
