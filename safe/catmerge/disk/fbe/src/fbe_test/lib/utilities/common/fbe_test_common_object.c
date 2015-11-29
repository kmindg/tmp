/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_common_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains common object functions.
 *
 * @version
 *  03/25/2011  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe_test_common_utils.h"
#include "mut.h"
#include "sep_utils.h"


/**********************
 *  LOCAL DEFINITIONS
 **********************/
#define FBE_TEST_OBJECT_POLLING_INTERVAL    100  /*! This is puposefully short, 100ms */

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************************
 *          fbe_test_common_util_wait_for_object_lifecycle_state_on_sp()
 *****************************************************************************
 * 
 * @brief   Wait for the object on the sp specified.  Use this method when the
 *          object may not reside on the calling sp.
 *
 * @param   b_wait_for_both_sps - FBE_TRUE Need to wait for both sps
 * @param   object_id - object id to wait for
 * @param   expected_lifecycle_state - Lifecycle state to wait for
 * @param   timeout_ms - Timeout in ms
 * @param   package_id - package id of object to wait for
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(fbe_object_id_t object_id, 
                                                                        fbe_transport_connection_target_t target_sp_of_object,
                                                                        fbe_lifecycle_state_t expected_lifecycle_state, 
                                                                        fbe_u32_t timeout_ms, 
                                                                        fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_OK; 
    fbe_status_t                        target_status;
    fbe_u32_t                           current_time = 0;
    fbe_lifecycle_state_t               current_state = 0;
    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t   peer_target = FBE_TRANSPORT_SP_B;

    /* First get the sp id this was invoked on and out peer.
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    peer_target = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    
    while (current_time < timeout_ms)
    {
        fbe_transport_connection_target_t   current_target = target_invoked_on;

        /* Configure the target before sending the control command
         */
        if (target_sp_of_object != target_invoked_on)
        {
             status = fbe_api_transport_set_target_server(target_sp_of_object);
             MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
             current_target = target_sp_of_object;
        }

        /* Issue the request
         */
        status = fbe_api_get_object_lifecycle_state(object_id, &current_state, package_id);

        /* Set the target back to the sp it was invoked on (if required)
         */
        if (target_sp_of_object != target_invoked_on)
        {
             target_status = fbe_api_transport_set_target_server(target_invoked_on);
             MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, target_status);
             current_target = target_invoked_on;
        }

        /* Now process the status
         */
        if (status == FBE_STATUS_GENERIC_FAILURE){
            mut_printf(MUT_LOG_TEST_STATUS,  
                          "wait_for_lifecycle_state_on_sp: obj: 0x%x sp of object: %d this sp: %d failed with status: 0x%x",
                          object_id, target_sp_of_object, target_sp_of_object, status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }

		if ((status == FBE_STATUS_NO_OBJECT) && (expected_lifecycle_state == FBE_LIFECYCLE_STATE_NOT_EXIST)) {
            mut_printf(MUT_LOG_TEST_STATUS,
                          "wait_for_lifecycle_state_on_sp: obj: 0x%x sp of object: %d this sp: %d no such object status: 0x%x",
                          object_id, target_sp_of_object, target_sp_of_object, status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
		}
            
        if (current_state == expected_lifecycle_state){
            current_target = fbe_api_transport_get_target_server();
            mut_printf(MUT_LOG_TEST_STATUS,
                       "wait_for_lifecycle_state_on_sp: obj: 0x%x sp of object: %d this sp: %d",
                       object_id, target_sp_of_object, target_sp_of_object);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(target_invoked_on, current_target);
            return status;
        }

        current_time = current_time + FBE_TEST_OBJECT_POLLING_INTERVAL;
        fbe_api_sleep(FBE_TEST_OBJECT_POLLING_INTERVAL);
    }

    /* We timed out
     */
    status = FBE_STATUS_TIMEOUT;
    mut_printf(MUT_LOG_TEST_STATUS,
               "wait_for_lifecycle_state_on_sp: obj: 0x%x expected state %d != current state %d in %d ms!", 
               object_id, expected_lifecycle_state, current_state, timeout_ms);
    mut_printf(MUT_LOG_TEST_STATUS,
               "wait_for_lifecycle_state_on_sp: this sp: %d object sp: %d",
               target_invoked_on, target_sp_of_object);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*****************************************************************
 * end fbe_test_common_util_wait_for_object_lifecycle_state_on_sp()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_common_util_wait_for_object_lifecycle_state_all_sps()
 *****************************************************************************
 * 
 * @brief   Wait for the object on (1) or both sps
 *
 * @param   b_wait_for_both_sps - FBE_TRUE Need to wait for both sps
 * @param   object_id - object id to wait for
 * @param   expected_lifecycle_state - Lifecycle state to wait for
 * @param   timeout_ms - Timeout in ms
 * @param   package_id - package id of object to wait for
 *
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_bool_t b_wait_for_both_sps,
                                                              fbe_object_id_t object_id, 
                                                              fbe_lifecycle_state_t expected_lifecycle_state, 
                                                              fbe_u32_t timeout_ms, 
                                                              fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_target;

    if (b_wait_for_both_sps)
    {
    
        /* For all targets wait for lifecycle state for the object specified
         */
        for (current_target = FBE_TRANSPORT_SP_A; current_target <= FBE_TRANSPORT_SP_B; current_target++)
        {
            /* Invoke the `wait for object' specifying the target server
             */
            status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(object_id, 
                                                                                current_target,
                                                                                expected_lifecycle_state, 
                                                                                timeout_ms, 
                                                                                package_id);
            if (status != FBE_STATUS_OK)
            {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for lifecycle both sps: obj: 0x%x exp state: %d timeout(ms): %d target: %d status: 0x%x", 
                           object_id, expected_lifecycle_state, timeout_ms, current_target, status);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
    } /* end b_wait_for_both_sps */
    else
    {
        current_target = fbe_api_transport_get_target_server();

        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(object_id, current_target,
                                                                            expected_lifecycle_state, timeout_ms, package_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Success
     */
    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_test_common_util_wait_for_object_lifecycle_state_all_sps()
 *******************************************************************/

/***********************************
 * end file fbe_test_common_object.c
 ***********************************/

fbe_status_t fbe_test_sep_utils_wait_for_attribute(fbe_volume_attributes_flags attr, 
                                      fbe_u32_t timeout_in_sec, 
                                      fbe_object_id_t lun_object_id, 
									  fbe_bool_t attribute_existence) 
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_volume_attributes_flags         attribute;
	fbe_object_id_t                     bvd_object_id;
	fbe_u32_t                           current_time = 0;
	fbe_u32_t							condition_met = 0;
	
	status = fbe_test_sep_util_lookup_bvd(&bvd_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, bvd_object_id);
	
	while (current_time < timeout_in_sec) 
    {
        /* Get event from BVD object */
        status = fbe_api_bvd_interface_get_downstream_attr(bvd_object_id, &attribute, lun_object_id);

        if (attribute & attr)
        {
            if (attribute_existence)
            {
                condition_met = 1;
                break;
            }
        }
        else
        {
            if (!attribute_existence)
            {
                condition_met = 1;
                break;
            }
        }
        current_time = current_time + FBE_TEST_OBJECT_POLLING_INTERVAL;
        fbe_api_sleep(FBE_TEST_OBJECT_POLLING_INTERVAL);
    }
	if(condition_met == 0) {
        status = FBE_STATUS_GENERIC_FAILURE;
		if(attribute_existence) {
			mut_printf(MUT_LOG_TEST_STATUS, "=== Failure! Attribute Mismatch! Looking for attribute 0x%x to be set but the current arribute is 0x%x ===\n", attr, attribute);
		}
		else {
			mut_printf(MUT_LOG_TEST_STATUS, "=== Failure! Attribute Mismatch! Looking for attribute 0x%x to be cleared but the current arribute is 0x%x ===\n", attr, attribute);
		}
	}
	return status;
}

const char *fbe_test_common_get_sp_name(fbe_sim_transport_connection_target_t sp)
{
    switch (sp) {
    case FBE_SIM_SP_A:
        return "SPA";
    case FBE_SIM_SP_B:
        return "SPB";
    default:
        return "<UNKNOWN>";
    }
}
