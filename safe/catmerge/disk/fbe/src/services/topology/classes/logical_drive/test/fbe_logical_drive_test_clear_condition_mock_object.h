#ifndef FBE_LOGICAL_DRIVE_TEST_CLEAR_CONDITION_MOCK_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_CLEAR_CONDITION_MOCK_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_clear_condition_mock_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's clear
 *  condition, mock object.
 * 
 *  This object allows our testing code to "mock" the clearing of a lifecycle
 *  condition, and thus test code that has calls to the lifecycle clear.
 *
 * HISTORY
 *   10/21/2008:  Created. RPF
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

/*! @struct fbe_ldo_test_clear_condition_mock_t 
 *  
 *  @brief This object is used for testing the clear condition interface.
 *         This structure will store the results of the mock operation.
 */
typedef struct fbe_ldo_test_clear_condition_mock_s
{
    fbe_u32_t called_count; /*!< Number of times we were called. */
    struct fbe_base_object_s * object_p; /*!< ptr to object sent in on last call. */
}
fbe_ldo_test_clear_condition_mock_t;

/*************************
 *   API FUNCTIONS
 *************************/
fbe_ldo_test_clear_condition_mock_t * fbe_ldo_test_get_clear_condition_mock(void);
fbe_status_t fbe_ldo_clear_condition_mock(struct fbe_base_object_s * p_object);
void fbe_ldo_clear_condition_mock_init(fbe_ldo_test_clear_condition_mock_t *const mock_p);

#endif /* FBE_LOGICAL_DRIVE_TEST_CLEAR_CONDITION_MOCK_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_clear_condition_mock_object.h
 *****************************************************/
