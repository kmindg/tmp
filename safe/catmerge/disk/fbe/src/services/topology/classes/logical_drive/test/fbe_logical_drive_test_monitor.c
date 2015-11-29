/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  monitor.  We test each of the lifecycle states for normal case and error
 *  cases.
 * 
 * @ingroup logical_drive_class_tests
 *
 * @version
 *   10/21/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_state_object.h"
#include "fbe_logical_drive_test_clear_condition_mock_object.h"
#include "fbe_logical_drive_test_set_condition_mock_object.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_test_state_setup_lifecycle(
 *                   fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function sets up for a read state machine test.
 *
 * @param test_state_p - The test state object to use in the test.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_state_setup_lifecycle(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_packet_t *io_packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;

    /* Init the state object.
     */
    fbe_ldo_test_state_init(test_state_p);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");

    /* Initialize our logical drive block size and optimum block size.
     */
    fbe_ldo_setup_block_sizes( logical_drive_p, 520);

    /* Setup io packet sg.
     */

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_state_setup_lifecycle() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_identity_unknown_completion(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  getting the identity for the first time.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_identity_unknown_completion(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_u32_t index;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    /* Zero out the identity info in the logical drive. 
     * Set the identity info returned to all 1s.  
     */
    fbe_zero_memory(&logical_drive_p->identity, sizeof(fbe_block_transport_identify_t));

    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Init every byte in the identify info. 
         * This is the value that gets returned by the identify command.
         */
        logical_drive_p->last_identity.identify_info[index] = index;
    }

    /* Run the state
     */
    status = logical_drive_identity_unknown_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was called with the expected
     * values. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 1,
                             "LDO: clear condition mock not called");
    MUT_ASSERT_TRUE(clear_condition_mock_p->object_p == (struct fbe_base_object_s*)logical_drive_p);

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_OK,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the identify info was copied correctly.
     */
    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Make sure the identify info was copied OK.
         * This is the value that gets returned by the identify command.
         */
        MUT_ASSERT_INT_EQUAL(logical_drive_p->identity.identify_info[index], index);
    }
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_identity_unknown_completion() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_identity_unknown_completion_error(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  getting the identity for the first time.  In this case the
 *  packet was completed with error, so we are testing the
 *  error path for this completion function.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_identity_unknown_completion_error(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_u32_t index;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

    /* Zero out the identity info in the logical drive. 
     * Set the identity info returned to all 1s.  
     */
    fbe_zero_memory(&logical_drive_p->identity, sizeof(fbe_block_transport_identify_t));

    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Init every byte in the identify info. 
         * This is the value that gets returned by the identify command.
         */
        logical_drive_p->last_identity.identify_info[index] = index;
    }

    /* Run the state
     */
    status = logical_drive_identity_unknown_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was not called.
     * We hit an error so we do not expect the condition to be cleared. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 0,
                             "LDO: clear condition mock should not be called");

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the identify info was copied correctly.
     */
    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Make sure the identify info was not touched.
         * We should not have copied the identity info since there was an error.
         */
        MUT_ASSERT_INT_EQUAL(logical_drive_p->identity.identify_info[index], 0);
    }
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_identity_unknown_completion_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_identity_not_validated_completion(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  getting the identity for the first time.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_identity_not_validated_completion(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_u32_t index;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Init every byte in the identify info. 
         * We setup the values so that the most recent fetch of the identity 
         * in logical_drive_p->last_identity, matches the saved value of the identity in 
         * the logical_drive_p->identity.
         */
        logical_drive_p->last_identity.identify_info[index] = index;
        logical_drive_p->identity.identify_info[index] = index;
    }

    /* Run the state
     */
    status = logical_drive_identity_not_validated_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was called with the expected
     * values. There were no errors, so we expect the condition to be cleared.
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 1,
                             "LDO: clear condition mock not called");
    MUT_ASSERT_TRUE(clear_condition_mock_p->object_p == (struct fbe_base_object_s*)logical_drive_p);

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_OK,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Make sure the identify info was not changed.
         */
        MUT_ASSERT_INT_EQUAL(logical_drive_p->identity.identify_info[index], index);
        MUT_ASSERT_INT_EQUAL(logical_drive_p->last_identity.identify_info[index], index);
    }
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_identity_not_validated_completion() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_identity_not_validated_completion_changed(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  validating the identity.
 * 
 *  In this function we are testing for the case where the identity information
 *  has changed.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_identity_not_validated_completion_changed(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_ldo_test_set_condition_mock_t *set_condition_mock_p = NULL;
    fbe_u32_t index;
    
    /* Setup our mock CLEAR condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);
    
    /* Setup our mock SET condition function.
     */
    fbe_ldo_set_condition_set_fn(fbe_ldo_set_condition_mock);
    set_condition_mock_p = fbe_ldo_test_get_set_condition_mock();
    fbe_ldo_set_condition_mock_init(set_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    /* Zero out the identity info in the logical drive. 
     * Set the identity info returned to the byte index. 
     * We are intentionally making it look like the stored value for identity 
     * is different than the value being returned (in last_identity). 
     *  
     * This would normally cause the object to get destroyed. 
     */
    fbe_zero_memory(&logical_drive_p->identity, sizeof(fbe_block_transport_identify_t));

    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Init every byte in the identify info. 
         * This is the value that gets returned by the identify command.
         */
        logical_drive_p->last_identity.identify_info[index] = index;
    }

    /* Run the state
     */
    status = logical_drive_identity_not_validated_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to set the condition was called.
     * Since the identity changed, we expect this mock to be called 
     * in order to destroy the object. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(set_condition_mock_p->called_count, 1,
                             "LDO: set condition mock should be called");
    MUT_ASSERT_INT_EQUAL_MSG(set_condition_mock_p->cond_id, FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY,
                             "LDO: set condition mock condition unexpected");
    MUT_ASSERT_TRUE_MSG(set_condition_mock_p->class_const_p == &fbe_logical_drive_lifecycle_const,
                             "LDO: set condition mock class const unexpected");
    MUT_ASSERT_TRUE_MSG(set_condition_mock_p->object_p == (struct fbe_base_object_s *)logical_drive_p,
                             "LDO: set condition mock base object unexpected");

    /* Make sure the mock to clear the condition was not called.
     * We hit an error so we do not expect the condition to be cleared. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 0,
                             "LDO: clear condition mock should not be called");

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_OK,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the identify info was copied correctly.
     */
    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Make sure the identify info was not changed.
         * This is the value that gets returned by the identify command.
         */
        MUT_ASSERT_INT_EQUAL(logical_drive_p->identity.identify_info[index], 0);
        MUT_ASSERT_INT_EQUAL(logical_drive_p->last_identity.identify_info[index], index);
    }
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_identity_not_validated_completion_changed() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_identity_not_validated_completion_error(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  getting the identity for the first time.  In this case the
 *  packet was completed with error, so we are testing the
 *  error path for this completion function.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_identity_not_validated_completion_error(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_u32_t index;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

    /* Zero out the identity info in the logical drive. 
     * Set the identity info returned to all 1s.  
     */
    fbe_zero_memory(&logical_drive_p->identity, sizeof(fbe_block_transport_identify_t));

    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Init every byte in the identify info. 
         * This is the value that gets returned by the identify command.
         */
        logical_drive_p->last_identity.identify_info[index] = index;
    }

    /* Run the state
     */
    status = logical_drive_identity_not_validated_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was not called.
     * We hit an error so we do not expect the condition to be cleared. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 0,
                             "LDO: clear condition mock should not be called");

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the identify info was copied correctly.
     */
    for (index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
    {
        /* Make sure the identify info was not touched.
         * We should not have copied the identity info since there was an error.
         */
        MUT_ASSERT_INT_EQUAL(logical_drive_p->identity.identify_info[index], 0);
        MUT_ASSERT_INT_EQUAL(logical_drive_p->last_identity.identify_info[index], index);
    }
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_identity_not_validated_completion_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_edge_detached_completion(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  trying to attach the edge.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_edge_detached_completion(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    /* Run the state
     */
    status = logical_drive_block_edge_detached_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was called with the expected
     * values. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 1,
                             "LDO: clear condition mock not called");
    MUT_ASSERT_TRUE(clear_condition_mock_p->object_p == (struct fbe_base_object_s*)logical_drive_p);

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_OK,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_edge_detached_completion() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_edge_detached_completion_error(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  getting the identity for the first time.  In this case the
 *  packet was completed with error, so we are testing the
 *  error path for this completion function.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_edge_detached_completion_error(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

    /* Run the state
     */
    status = logical_drive_block_edge_detached_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was not called.
     * We hit an error so we do not expect the condition to be cleared. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 0,
                             "LDO: clear condition mock should not be called");

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_edge_detached_completion_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_edge_disabled_completion(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  trying to enable the edge.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_edge_disabled_completion(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    /* Run the state
     */
    status = logical_drive_block_edge_disabled_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was called with the expected
     * values. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 1,
                             "LDO: clear condition mock not called");
    MUT_ASSERT_TRUE(clear_condition_mock_p->object_p == (struct fbe_base_object_s*)logical_drive_p);

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_OK,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_edge_disabled_completion() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_edge_disabled_completion_error(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  trying to enable the edge.  In this case the
 *  packet was completed with error, so we are testing the
 *  error path for this completion function.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_edge_disabled_completion_error(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

    /* Run the state
     */
    status = logical_drive_block_edge_disabled_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was not called.
     * We hit an error so we do not expect the condition to be cleared. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 0,
                             "LDO: clear condition mock should not be called");

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_edge_disabled_completion_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_edge_attached_completion(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  trying to detach the edge.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_edge_attached_completion(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    /* Run the state
     */
    status = logical_drive_block_edge_attached_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was called with the expected
     * values. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 1,
                             "LDO: clear condition mock not called");
    MUT_ASSERT_TRUE(clear_condition_mock_p->object_p == (struct fbe_base_object_s*)logical_drive_p);

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_OK,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_edge_attached_completion() */

/*!***************************************************************
 * @fn fbe_ldo_test_monitor_edge_attached_completion_error(void)
 ****************************************************************
 * @brief
 *  This function tests the monitor state where we handle the completion for
 *  trying to detach the edge.  In this case the
 *  packet was completed with error, so we are testing the
 *  error path for this completion function.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_monitor_edge_attached_completion_error(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_clear_condition_mock_t *clear_condition_mock_p = NULL;
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_status_t status;
    
    /* Setup our mock clear condition function.
     */
    fbe_ldo_set_clear_condition_fn(fbe_ldo_clear_condition_mock);
    clear_condition_mock_p = fbe_ldo_test_get_clear_condition_mock();
    fbe_ldo_clear_condition_mock_init(clear_condition_mock_p);

    /* Init the state object.
     */
    status = fbe_ldo_test_state_setup_lifecycle(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");

    /* Initialize the packet status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

    /* Run the state
     */
    status = logical_drive_block_edge_attached_cond_completion(packet_p, logical_drive_p);

    /* Status should be OK.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure the mock to clear the condition was not called.
     * We hit an error so we do not expect the condition to be cleared. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(clear_condition_mock_p->called_count, 0,
                             "LDO: clear condition mock should not be called");

    /* Make sure the packet status values are set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(packet_p), 
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(packet_p), 
                             0,
                             "LDO: packet qualifier not set as expected");
    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_monitor_edge_attached_completion_error() */

/*!**************************************************************
 * @fn fbe_ldo_test_add_monitor_tests(mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  Add the monitor unit tests tests to the input suite.
 *
 * @param suite_p - suite to add tests to.               
 *
 * @return none.
 *
 * @author
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_monitor_tests(mut_testsuite_t * const suite_p)
{
    /* Tests for testing the monitor functions.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_identity_unknown_completion, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_identity_unknown_completion_error, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_identity_not_validated_completion, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_identity_not_validated_completion_changed, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_identity_not_validated_completion_error, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_edge_detached_completion, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_edge_detached_completion_error, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_edge_disabled_completion, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_edge_disabled_completion_error, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_edge_attached_completion, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_monitor_edge_attached_completion_error, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/******************************************
 * end fbe_ldo_test_add_monitor_tests()
 ******************************************/
/*****************************************
 * end fbe_logical_drive_test_monitor.c
 *****************************************/
