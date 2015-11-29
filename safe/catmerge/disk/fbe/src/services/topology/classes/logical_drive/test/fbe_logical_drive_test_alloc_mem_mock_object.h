#ifndef FBE_LOGICAL_DRIVE_TEST_ALLOC_MEM_MOCK_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_ALLOC_MEM_MOCK_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_test_alloc_mem_mock_object.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the definition for the logical drive object's
 *  allocate memory mock object.
 *
 * HISTORY
 *   11/28/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_trace.h"

/* This macro tests a condition and displays/returns a failure
 * if the condition is false.
 */
#define TEST_CONDITION_WITH_MSG(m_condition, m_msg)\
if (!(m_condition)) \
{\
    mut_printf(MUT_LOG_HIGH, "ASSERT FAILURE: line: %d function: %s Condition: %s\n", __LINE__, __FUNCTION__, m_msg);\
    return(FBE_STATUS_GENERIC_FAILURE);\
}\

/* This object is used for testing the allocate io packet interface.
 */
typedef struct fbe_ldo_test_alloc_io_packet_mock_s
{
    fbe_u32_t allocation_callback_count;
    fbe_memory_request_t *memory_request_p;
    fbe_memory_completion_function_t completion_function;
    fbe_memory_completion_context_t completion_context;
}
fbe_ldo_test_alloc_io_packet_mock_t;

/*************************
 *   API FUNCTIONS
 *************************/

void fbe_ldo_alloc_mem_mock_init(fbe_ldo_test_alloc_io_packet_mock_t * const mock_p);

fbe_status_t fbe_ldo_alloc_mem_mock_fn(fbe_packet_t *io_packet_p,
                                       fbe_memory_completion_function_t completion_function,
                                       fbe_memory_completion_context_t completion_context,
                                       fbe_u32_t number_to_allocate);

fbe_status_t fbe_ldo_alloc_mem_mock_validate(fbe_ldo_test_alloc_io_packet_mock_t *const mock_p);
fbe_status_t fbe_ldo_alloc_mem_mock_validate_not_called(fbe_ldo_test_alloc_io_packet_mock_t *const mock_p);
fbe_ldo_test_alloc_io_packet_mock_t * fbe_ldo_alloc_mem_mock_get_object(void);

#endif /* FBE_LOGICAL_DRIVE_TEST_ALLOC_MEM_MOCK_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_alloc_mem_mock_object.h
 *****************************************************/
