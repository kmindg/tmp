#ifndef FBE_LOGICAL_DRIVE_TEST_ALLOC_IO_CMD_MOCK_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_ALLOC_IO_CMD_MOCK_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_test_alloc_io_cmd_mock_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definition for the logical drive object's
 *  allocate io cmd mock object.
 *
 * HISTORY
 *   11/15/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_trace.h"

/*! @struct fbe_ldo_test_alloc_io_cmd_mock_t
 *  @brief This object is used for testing the allocate io cmd interface.
 */
typedef struct fbe_ldo_test_alloc_io_cmd_mock_s
{
    /*! Number of times the mock was called
     */
    fbe_u32_t allocate_count;
    fbe_memory_request_t *memory_request_p;
    fbe_memory_completion_function_t completion_function;
    fbe_memory_completion_context_t completion_context;
}
fbe_ldo_test_alloc_io_cmd_mock_t;

/*************************
 *   API FUNCTIONS
 *************************/

void fbe_ldo_alloc_io_cmd_mock_init(fbe_ldo_test_alloc_io_cmd_mock_t * const mock_p);

fbe_status_t fbe_ldo_test_alloc_io_cmd_mock(fbe_packet_t * const packet_p,
                                            fbe_memory_completion_function_t completion_function,
                                            fbe_memory_completion_context_t completion_context);

fbe_status_t fbe_ldo_alloc_io_cmd_mock_validate(fbe_ldo_test_alloc_io_cmd_mock_t *const mock_p);
fbe_status_t fbe_ldo_alloc_io_cmd_mock_validate_not_called(fbe_ldo_test_alloc_io_cmd_mock_t *const mock_p);
fbe_ldo_test_alloc_io_cmd_mock_t * fbe_ldo_test_get_alloc_io_cmd_mock(void);

#endif /* FBE_LOGICAL_DRIVE_TEST_ALLOC_IO_CMD_MOCK_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_alloc_io_cmd_mock_object.h
 *****************************************************/
