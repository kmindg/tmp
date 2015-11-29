#ifndef FBE_LOGICAL_DRIVE_TEST_SET_CONDITION_MOCK_OBJECT_H
#define FBE_LOGICAL_DRIVE_TEST_SET_CONDITION_MOCK_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_set_condition_mock_object.h
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
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"

/*! @struct fbe_ldo_test_set_condition_mock_t 
 *  
 *  @brief This object is used for testing the set condition interface.
 *         This structure will store the results of the mock operation.
 */
typedef struct fbe_ldo_test_set_condition_mock_s
{
    fbe_u32_t called_count; /*!< Number of times we were called. */
    struct fbe_base_object_s * object_p; /*!< ptr to object sent in on last call. */
    fbe_lifecycle_cond_id_t cond_id; /*!< last cond_id we were called with. */
    fbe_lifecycle_const_t * class_const_p; /*!< last class const we were called with. */
}
fbe_ldo_test_set_condition_mock_t;

/*************************
 *   API FUNCTIONS
 *************************/
fbe_ldo_test_set_condition_mock_t * fbe_ldo_test_get_set_condition_mock(void);
fbe_status_t fbe_ldo_set_condition_mock(fbe_lifecycle_const_t * p_class_const,
                                        struct fbe_base_object_s * p_object,
                                        fbe_lifecycle_cond_id_t cond_id);
void fbe_ldo_set_condition_mock_init(fbe_ldo_test_set_condition_mock_t *const mock_p);

#endif /* FBE_LOGICAL_DRIVE_TEST_SET_CONDITION_MOCK_OBJECT_H */
/*****************************************************
 * end fbe_logical_drive_test_set_condition_mock_object.h
 *****************************************************/
