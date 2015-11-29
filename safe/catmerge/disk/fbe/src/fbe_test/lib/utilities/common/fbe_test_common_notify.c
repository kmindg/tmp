/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_common_notify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains common notification functions.
 *  Created from the fbe_test_pp_util notification functions.
 *
 * @version
 *  09/25/2012  Ron Proulx  - Updated to create common notification testing
 *                            mechanism.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_test.h"
#include "mut.h"

/********************** 
 * LITERAL DEFINITIONS
 **********************/

/*!*******************************************************************
 * @def FBE_TEST_COMMON_BITS_PER_BYTE
 *********************************************************************
 * @brief Bits in a byte.
 *
 *********************************************************************/
#define FBE_TEST_COMMON_BITS_PER_BYTE                            8

/************************** 
 * STRUCTURE DEFINITIONS
 **************************/

/*!***************************************************************************
 * @struct  fbe_test_common_sp_notification_context_t
 *****************************************************************************
 * 
 * @brief   Per SP notification context.  Each SP will have a global
 *          notification context.  This structure is populated when ever an
 *          registered notification for the selected SP occurs.
 *
 *****************************************************************************/
typedef struct fbe_test_common_sp_notification_context_s
{
    fbe_semaphore_t                     sem;
    fbe_bool_t                          b_is_dualsp_test;
    fbe_bool_t                          b_is_peer_alive;
    fbe_notification_registration_id_t  notification_id;
    fbe_transport_connection_target_t   notification_sp;
    fbe_object_id_t                     object_id;
    fbe_notification_info_t             notification_info;

} fbe_test_common_sp_notification_context_t;

/*!***************************************************************************
 * @struct  fbe_test_common_sp_notification_found_t
 *****************************************************************************
 * 
 * @brief   Per SP notification found.  Each SP will have a global
 *          notification found.  It is more that a bool because in the dualsp
 *          environment will get (2) notifications for each SP.
 *
 * @note    This is required for the dualsp environment due to the fact that 
 *          `fbe_test' sees notifications from BOTH SPs.  Since the `fbe_test'
 *          executable includes the fbe notification API thread and `fbe_test'
 *          registers for notifications from BOTH SPs, each fbe_test_common_ client (one
 *          per SP) will see (2) notifications.
 *
 *          This will NOT occur on the actual hardware where there the fbe 
 *          notification API thread will ONLY see the notifications for its SP.
 *
 *****************************************************************************/
typedef struct fbe_test_common_sp_notification_found_s
{
    fbe_bool_t  b_found;                        /* FBE_TRUE - Required number of notifications detected.*/
    fbe_bool_t  b_allow_extra_notifications;    /* Some notifications like reconstruct don't have a fixed number. */
    fbe_u32_t   found_count;                    /* Number of matching notifications found.*/
} fbe_test_common_sp_notification_found_t;

/*!*******************************************************************
 * @var     fbe_test_common_sp_notification_array
 *********************************************************************
 * @brief   This is the array of per SP notification context.
 *
 *********************************************************************/
static fbe_test_common_sp_notification_context_t fbe_test_common_sp_notification_context_array[(FBE_TRANSPORT_SP_B + 1)] = { 0 };

/*!*******************************************************************
 * @var     fbe_test_common_notification_lock
 *********************************************************************
 * @brief   This is notification (update) to wait for (from both SPs).
 *
 *********************************************************************/
static fbe_spinlock_t fbe_test_common_notification_lock = { 0 };

/*!*******************************************************************
 * @var     fbe_test_common_notification_to_wait_for
 *********************************************************************
 * @brief   This is notification (update) to wait for (from both SPs).
 *
 *********************************************************************/
static update_object_msg_t fbe_test_common_notification_to_wait_for = { 0 };

/*!*******************************************************************
 * @var     fbe_test_common_sp_notification_found
 *********************************************************************
 * @brief   This is the array of per SP notification found array
 *
 * @note    Test assumes there will be only (1) notification per request.
 *          For dualsp there will be (2) notification callback per request.
 *********************************************************************/
static fbe_test_common_sp_notification_found_t fbe_test_common_sp_notification_found[(FBE_TRANSPORT_SP_B + 1)] = { 0 };

/*!*******************************************************************
 * @var     fbe_test_common_notification_object_type_mask
 *********************************************************************
 * @brief   This is the mask of object types that we will
 *          register for callback for.
 *
 *********************************************************************/
static fbe_topology_object_type_t fbe_test_common_notification_object_type_mask = (FBE_TOPOLOGY_OBJECT_TYPE_LUN               |
                                                                                   FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE     |
                                                                                   FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE   );

/*!*******************************************************************
 * @var     fbe_test_common_notification_type_mask
 *********************************************************************
 * @brief   This is the mask of notification types that we will
 *          register for callback for.
 *
 *********************************************************************/
static fbe_notification_type_t fbe_test_common_notification_type_mask = (FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED |
                                                                FBE_NOTIFICATION_TYPE_SWAP_INFO                |
                                                                FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION      |
                                                                FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED    |
                                                                FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY    |
                                                                FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL     |
                                                                FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED        );


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* the following is the utility functions for Object Lifecycle State Notification */
static void fbe_test_common_util_lifecycle_state_notification_callback_function(update_object_msg_t * update_object_msg, void * context);

/*!****************************************************************************
 * fbe_test_common_util_register_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function intializes the notification semaphore in the ns_context and register the notification. 
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    04/1/2010 - Created. guov
 *
 ******************************************************************************/
void fbe_test_common_util_register_lifecycle_state_notification(fbe_test_common_util_lifecycle_state_ns_context_t *ns_context,
                                                                fbe_package_notification_id_mask_t package_mask,
                                                                fbe_topology_object_type_t object_type,
                                                                fbe_object_id_t object_id,
                                                                fbe_notification_type_t expected_state_mask)
{
    fbe_status_t                        status;

    ns_context->object_id = object_id;
    ns_context->object_type = object_type;
    ns_context->actual_lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    ns_context->expected_lifecycle_state = expected_state_mask;

    fbe_semaphore_init(&(ns_context->sem), 0, 1);
    status = fbe_api_notification_interface_register_notification(expected_state_mask,
                                                                  package_mask,
                                                                  object_type,
                                                                  fbe_test_common_util_lifecycle_state_notification_callback_function,
                                                                  ns_context,
																  &ns_context->user_registration_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "=== fbe_test_common_util register Lifecycle State notification successfully! ===");
    return;
}

/*!****************************************************************************
 * fbe_test_common_util_unregister_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function unregister the notification and destroy the semaphore in the ns_context.
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    04/1/2010 - Created. guov
 *
 ******************************************************************************/
void fbe_test_common_util_unregister_lifecycle_state_notification(fbe_test_common_util_lifecycle_state_ns_context_t *ns_context)
{
    fbe_status_t                        status;

    status = fbe_api_notification_interface_unregister_notification(fbe_test_common_util_lifecycle_state_notification_callback_function, 
                                                                    ns_context->user_registration_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "=== fbe_test_common_util unregister Lifecycle State notification successfully! ===");

    fbe_semaphore_destroy(&(ns_context->sem));
    return;
}

/*!****************************************************************************
 * fbe_test_common_util_wait_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function waits and checkes for timeout.
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    03/15/2010 - Created. guov
 *
 ******************************************************************************/
void fbe_test_common_util_wait_lifecycle_state_notification(fbe_test_common_util_lifecycle_state_ns_context_t *ns_context,
                                                            fbe_time_t timeout_in_ms)  
{
    fbe_status_t status;

    status = fbe_semaphore_wait_ms(&(ns_context->sem), (fbe_u32_t)timeout_in_ms);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Wait for lifecycle state notification timed out!");
    return;
}

/*!**************************************************************
 * fbe_test_common_util_lifecycle_state_notification_callback_function()
 ****************************************************************
 * @brief
 *  The the callback function, where the sem is release when a match found.
 *  The test that waits for this notification will be blocked until 
 *  either a match found or timeout reaches.
 *
 * @param update_object_msg  - message fron the notification.   
 * @param context  - notification context.
 *
 * @return None.
 *
 ****************************************************************/
static void fbe_test_common_util_lifecycle_state_notification_callback_function(update_object_msg_t * update_object_msg, void * context)
{
    fbe_test_common_util_lifecycle_state_ns_context_t * ns_context_p = (fbe_test_common_util_lifecycle_state_ns_context_t * )context;

	if ((update_object_msg->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) &&
        (update_object_msg->notification_info.object_type == ns_context_p->object_type) &&
        ((ns_context_p->object_id == FBE_OBJECT_ID_INVALID) || (ns_context_p->object_id == update_object_msg->object_id)))
    {
        /* update the actual state, for debugging purpose */
		ns_context_p->actual_lifecycle_state = update_object_msg->notification_info.notification_type;

       /* Release the semaphore */
        mut_printf(MUT_LOG_LOW, "%s: received a matching notification from object type %llu object_id: 0x%x, state is 0x%llX.",
                   __FUNCTION__, 
                   (unsigned long long)update_object_msg->notification_info.object_type,
		   update_object_msg->object_id, 
                   (unsigned long long)ns_context_p->actual_lifecycle_state);
        fbe_semaphore_release(&(ns_context_p->sem), 0, 1, FALSE);
    }
    return;
}



/*!****************************************************************************
 *          fbe_test_common_notification_callback_function()
 ******************************************************************************
 *
 * @brief   Callback function for the spare library notification.
 *
 * @param   update_object_msg - Pointer to notification message received.
 *
 * @return  status
 *
 * @author
 *  08/15/2012  Ron Proulx  - Updated.
 ******************************************************************************/
static fbe_status_t fbe_test_common_notification_callback_function(update_object_msg_t *update_object_msg,
                                                          void *callback_context_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_common_sp_notification_context_t   *notification_context_p = (fbe_test_common_sp_notification_context_t *)callback_context_p;
    fbe_bool_t                          b_is_dualsp = notification_context_p->b_is_dualsp_test;
    fbe_bool_t                          b_is_peer_alive = notification_context_p->b_is_peer_alive;
    fbe_transport_connection_target_t   source_sp = FBE_TRANSPORT_INVALID_SERVER;
    fbe_u32_t                           num_required_before_found = 1;

    /* Get the SP that the notification came from.
     */
    source_sp = notification_context_p->notification_sp;
    if ((source_sp != FBE_TRANSPORT_SP_A) &&
        (source_sp != FBE_TRANSPORT_SP_B)    )
    {
        /* Invalid source SP.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s Invalid source_sp: %d",
                   __FUNCTION__, source_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* We will drop any notifications if we are rebooting the SP.
     */
    if (!fbe_test_is_sp_up(fbe_test_conn_target_to_sp(source_sp))) {
        mut_printf(MUT_LOG_TEST_STATUS, "%s drop notification for SP: %s",
                   __FUNCTION__, (source_sp == FBE_TRANSPORT_SP_A) ? "SPA" : "SPB"); 
        return FBE_STATUS_OK;
    }

    /* If we are in dualsp mode we must see (2) before declaring found.
     */
    if ((b_is_dualsp == FBE_TRUE)      &&
        (b_is_peer_alive != FBE_FALSE)    )
    {
        num_required_before_found = 2;
    }

    /* If we have unregistered just return.
     */
    if (notification_context_p->notification_id == 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
               "=== test common notification: %s: obj: 0x%x object_type: 0x%llx type: 0x%llx unregistered===",
               (source_sp == FBE_TRANSPORT_SP_A) ? "SPA" : "SPB",
               update_object_msg->object_id, update_object_msg->notification_info.object_type, update_object_msg->notification_info.notification_type);
        return FBE_STATUS_OK;
    }

    /* Synchronize with the test thread.
     */
	fbe_spinlock_lock(&fbe_test_common_notification_lock);

    /* If we are not waiting for a notification just exit.  We signify that we
     * are not waiting for a notification by setting the wait for object id to 0.
     */
    if (fbe_test_common_notification_to_wait_for.object_id == 0)
    {
        /* Just return.
         */
        fbe_spinlock_unlock(&fbe_test_common_notification_lock);
        return FBE_STATUS_OK;
    }


    /* Trace all notifications.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "=== test common notification: %s: obj: 0x%x object_type: 0x%llx type: 0x%llx ===",
               (source_sp == FBE_TRANSPORT_SP_A) ? "SPA" : "SPB",
               update_object_msg->object_id, update_object_msg->notification_info.object_type, update_object_msg->notification_info.notification_type);

    /*! @note Validate the notification.  Skip any notification where the object
     *        type is FBE_TOPOLOGY_OBJECT_TYPE_ALL.  There are update types like
     *       `set spare trigger timer' that have not object associated with them.
     *
     *  @note Do not use these `obsolete' fields:
     *          FBE30 temporary *
     *              fbe_topology_object_type_t  object_type;            
     *              fbe_s32_t                   port;
     *              fbe_s32_t                   encl;
     *              fbe_s32_t                   encl_addr;
     *              fbe_s32_t                   drive;
     *          FBE30 temporary *
     */
    if (update_object_msg->notification_info.object_type != FBE_TOPOLOGY_OBJECT_TYPE_ALL)
    {
        /* The object and notification type need to be valid.
         */
        if ((update_object_msg->object_id == 0)                           ||
            (update_object_msg->object_id == FBE_OBJECT_ID_INVALID)       ||
            (update_object_msg->notification_info.object_type == 0)                         ||
            (update_object_msg->notification_info.notification_type == 0)    )
        {
            fbe_spinlock_unlock(&fbe_test_common_notification_lock);
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== test common notification: obj: 0x%x object_type: 0x%llx type: 0x%llx invalid notification! ===",
                   update_object_msg->object_id, update_object_msg->notification_info.object_type, update_object_msg->notification_info.notification_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Validate the `notification to wait for'.
     */
    if ((fbe_test_common_notification_to_wait_for.object_id == 0)                           ||
        (fbe_test_common_notification_to_wait_for.object_id == FBE_OBJECT_ID_INVALID)       ||
        (fbe_test_common_notification_to_wait_for.notification_info.object_type == 0)       ||
        (fbe_test_common_notification_to_wait_for.notification_info.notification_type == 0)    )
    {
        fbe_spinlock_unlock(&fbe_test_common_notification_lock);
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== test common notification: obj: 0x%x object_type: 0x%llx type: 0x%llx invalid wait for notification! ===",
                   fbe_test_common_notification_to_wait_for.object_id, fbe_test_common_notification_to_wait_for.notification_info.object_type, 
                   fbe_test_common_notification_to_wait_for.notification_info.notification_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /*! @note Check if the notification received matches the one expected.
     *
     *  @note We currently assume that the notification is for exactly (1)
     *        object type and exactly (1) notification type.
     */
    if ((update_object_msg->object_id == fbe_test_common_notification_to_wait_for.object_id)                                                     &&
        (update_object_msg->notification_info.object_type == fbe_test_common_notification_to_wait_for.notification_info.object_type)                                                 &&
        (update_object_msg->notification_info.notification_type == fbe_test_common_notification_to_wait_for.notification_info.notification_type)    )
    {
        /* Print a message about the matching notification.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== test common notification: %s: obj: 0x%x object_type: 0x%llx type: 0x%llx match found ===",
                   (source_sp == FBE_TRANSPORT_SP_A) ? "SPA" : "SPB",
                   update_object_msg->object_id, update_object_msg->notification_info.object_type, update_object_msg->notification_info.notification_type);
        
        /* For job service notifications validate the expected status.
         */
        if (update_object_msg->notification_info.notification_type == FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED)
        {
            /* Only if the status or error code doesn't match print an error trace.
             */
            if ((update_object_msg->notification_info.notification_data.job_service_error_info.status !=
                    fbe_test_common_notification_to_wait_for.notification_info.notification_data.job_service_error_info.status)    ||
                (update_object_msg->notification_info.notification_data.job_service_error_info.error_code !=
                    fbe_test_common_notification_to_wait_for.notification_info.notification_data.job_service_error_info.error_code)   )
            {
                /* Fail the match
                 */
                fbe_spinlock_unlock(&fbe_test_common_notification_lock);
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== test common notification: obj: 0x%x type: 0x%llx status: 0x%x error: %d doesn't match expected status: 0x%x error: %d===",
                   fbe_test_common_notification_to_wait_for.object_id,fbe_test_common_notification_to_wait_for.notification_info.notification_type,
                   update_object_msg->notification_info.notification_data.job_service_error_info.status,
                   update_object_msg->notification_info.notification_data.job_service_error_info.error_code,
                   fbe_test_common_notification_to_wait_for.notification_info.notification_data.job_service_error_info.status,
                   fbe_test_common_notification_to_wait_for.notification_info.notification_data.job_service_error_info.error_code);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
        }

        /*! @note We expect exactly (1) matching notification.  Therefore if we
         *        have already recorded a match it is an error.
         */
        if (fbe_test_common_sp_notification_found[source_sp].b_found == FBE_TRUE)
        {
            /* There are cases like reconstruct where the number of notifications
             * is not fixed.  If that is the case then `allow extra notifications'
             * will be set which indicates that we should simply ignore the
             * notification.
             */
            if (fbe_test_common_sp_notification_found[source_sp].b_allow_extra_notifications == FBE_FALSE)
            {
                /* Match already found. Report a failure.
                */
                fbe_spinlock_unlock(&fbe_test_common_notification_lock);
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s %s: obj: 0x%x object_type: 0x%llx type: 0x%llx match already found!",
                       __FUNCTION__, (source_sp == FBE_TRANSPORT_SP_A) ? "SPA" : "SPB",
                       update_object_msg->object_id, update_object_msg->notification_info.object_type, update_object_msg->notification_info.notification_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
        }
        else
        {
            /* Else simply set the associated match found, copy the notification
             * into the per SP array (so that we can perform additional validation
             * like checking status etc.) and then signal the semaphore.
             */
            fbe_test_common_sp_notification_found[source_sp].found_count++;
            if (fbe_test_common_sp_notification_found[source_sp].found_count >= num_required_before_found)
            {
                fbe_test_common_sp_notification_found[source_sp].b_found = FBE_TRUE;
                notification_context_p->notification_info = update_object_msg->notification_info;
                fbe_semaphore_release(&notification_context_p->sem, 0, 1, 0);
            }
        }
    }

    /* Return the execution status.
     */
    fbe_spinlock_unlock(&fbe_test_common_notification_lock);
    return status;
}
/******************************************************************************
 * end fbe_test_common_notification_callback_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_test_common_register_notifications()
 *****************************************************************************
 *
 * @brief   Register for all notification types that are used by this test.
 *
 * @param   None
 *
 * @return  status
 *
 * @author
 *  08/15/2012  Ron Proulx  - Updated
 *****************************************************************************/
static fbe_status_t fbe_test_common_register_notifications(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_sp;
    fbe_test_common_sp_notification_context_t   *notification_context_p = NULL;

    /* Get the current target.
     */
    current_sp = fbe_api_transport_get_target_server();

    /* Always register for SP A. Validate that the context has been initialized.
     */
    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
    notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_A];
    MUT_ASSERT_TRUE(notification_context_p->notification_sp == FBE_TRANSPORT_SP_A);
    MUT_ASSERT_TRUE(notification_context_p->object_id == FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_TRUE(notification_context_p->notification_id == 0);
    status = fbe_api_notification_interface_register_notification(fbe_test_common_notification_type_mask,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                  fbe_test_common_notification_object_type_mask,
                                                                  (fbe_api_notification_callback_function_t) fbe_test_common_notification_callback_function, /* SAFEBUG - temporary adding cast for now to supress warning but this function should not return status */ 
                                                                  notification_context_p,
                                                                  &notification_context_p->notification_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If dualsp mode, register on SP B also.
     */
    if (notification_context_p->b_is_dualsp_test == FBE_TRUE)
    {
        /* Set the target server to SP B.
         */
        fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_B];
        MUT_ASSERT_TRUE(notification_context_p->notification_sp == FBE_TRANSPORT_SP_B);
        MUT_ASSERT_TRUE(notification_context_p->object_id == FBE_OBJECT_ID_INVALID);
        MUT_ASSERT_TRUE(notification_context_p->notification_id == 0);
        status = fbe_api_notification_interface_register_notification(fbe_test_common_notification_type_mask,
                                                                      FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                      fbe_test_common_notification_object_type_mask,
                                                                      (fbe_api_notification_callback_function_t)fbe_test_common_notification_callback_function, /* SAFEBUG - temporary adding cast for now to supress warning but this function should not return status */
                                                                      notification_context_p,
                                                                      &notification_context_p->notification_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the target to the original.
     */
    fbe_api_transport_set_target_server(current_sp);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_test_common_register_notifications()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_test_common_setup_notifications()
 ******************************************************************************
 * @brief   Initialize notification structures.
 *
 * @param   b_is_dualsp_test - FBE_TRUE - This is a dualsp test.
 *
 * @return status 
 *
 * @author
 *  09/25/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_test_common_setup_notifications(fbe_bool_t b_is_dualsp_test)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_common_sp_notification_context_t   *notification_context_p = NULL;

    /* Initialize the spinlock protecting the `wait for message'
     */
    fbe_spinlock_init(&fbe_test_common_notification_lock);

    /* Always register for SP A.  Initialize the context.
     */
    notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_A];
    notification_context_p->notification_sp = FBE_TRANSPORT_SP_A;
    notification_context_p->object_id = FBE_OBJECT_ID_INVALID;
    notification_context_p->b_is_dualsp_test = b_is_dualsp_test;
    notification_context_p->b_is_peer_alive = b_is_dualsp_test;
    /* Initialize the semaphore as Empty with a limit of 2.*/
    fbe_semaphore_init(&notification_context_p->sem, 0, 2);
       
    /* If this is a dualsp test initialize the context for SP B.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_B];
        notification_context_p->notification_sp = FBE_TRANSPORT_SP_B;
        notification_context_p->object_id = FBE_OBJECT_ID_INVALID;
        notification_context_p->b_is_dualsp_test = b_is_dualsp_test;
        notification_context_p->b_is_peer_alive = b_is_dualsp_test;
        /* Initialize the semaphore as Empty with a limit of 2.*/
        fbe_semaphore_init(&notification_context_p->sem, 0, 2);
    }
    
    /* Now register notifications 
     */
    status = fbe_test_common_register_notifications();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/********************************************** 
 * end fbe_test_common_setup_notifications()
 ********************************************/

/*!***************************************************************************
 *          fbe_test_common_unregister_notifications()
 *****************************************************************************
 *
 * @brief   Unregister for the previously registered notificaitons.
 *
 * @param   None
 *
 * @return  status
 *
 * @author
 *  08/15/2012  Ron Proulx  - Updated
 *****************************************************************************/
fbe_status_t fbe_test_common_unregister_notifications(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   current_sp;
    fbe_test_common_sp_notification_context_t   *notification_context_p = NULL;

    /* Get the current target.
     */
    current_sp = fbe_api_transport_get_target_server();

    /* Always register for SP A. Validate that the context has been initialized.
     */
    fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
    notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_A];
    MUT_ASSERT_TRUE(notification_context_p->notification_sp == FBE_TRANSPORT_SP_A);
    MUT_ASSERT_TRUE(notification_context_p->object_id == FBE_OBJECT_ID_INVALID);
    MUT_ASSERT_TRUE(notification_context_p->notification_id != 0);
    status = fbe_api_notification_interface_unregister_notification((fbe_api_notification_callback_function_t)fbe_test_common_notification_callback_function,/* SAFEBUG - temporary adding cast for now to supress warning but this function should not return status */
                                                                    notification_context_p->notification_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    notification_context_p->notification_id = 0;

    /* If dualsp mode, register on SP B also.
     */
    if (notification_context_p->b_is_dualsp_test == FBE_TRUE)
    {
        /* Set the target server to SP B.
         */
        fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_B];
        MUT_ASSERT_TRUE(notification_context_p->notification_sp == FBE_TRANSPORT_SP_B);
        MUT_ASSERT_TRUE(notification_context_p->object_id == FBE_OBJECT_ID_INVALID);
        MUT_ASSERT_TRUE(notification_context_p->notification_id != 0);
        status = fbe_api_notification_interface_unregister_notification((fbe_api_notification_callback_function_t)fbe_test_common_notification_callback_function,/* SAFEBUG - temporary adding cast for now to supress warning but this function should not return status */
                                                                        notification_context_p->notification_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        notification_context_p->notification_id = 0;
    }

    /* Set the target to the original.
     */
    fbe_api_transport_set_target_server(current_sp);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_test_common_unregister_notifications()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_test_common_count_bits
 ******************************************************************************
 *
 * @brief   Return the number of bits set in a u64.
 *
 * @param   value_to_check - Value to count
 *
 * @return  count of bits set
 *
 * @author
 *  08/15/2012  Ron Proulx  - Updated.
 ******************************************************************************/
fbe_u32_t fbe_test_common_count_bits(fbe_u64_t value_to_check)
{
    fbe_u32_t   bits_set = 0;
    fbe_u64_t   bit_index;

    for (bit_index = 0; 
         bit_index < (sizeof(fbe_u64_t) * FBE_TEST_COMMON_BITS_PER_BYTE), value_to_check != 0; 
         bit_index++)
    {
        if ((1ULL << bit_index) & value_to_check)
        {
            bits_set++;
            value_to_check &= ~(1ULL << bit_index);
        }
    }
    return bits_set;
}
/******************************************************************************
 * end fbe_test_common_count_bits()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_test_common_set_notification_to_wait_for()
 ******************************************************************************
 *
 * @brief   Populate the `notification to wait for' structure with the passed
 *          information.
 *
 * @param   object_id - The object id of the notificaion to wait for
 * @param   object_type - The object type of the notification to wait for
 * @param   notification_type - The notification type to wait for
 *
 * @return  status
 *
 * @author
 *  08/15/2012  Ron Proulx  - Updated.
 ******************************************************************************/
fbe_status_t fbe_test_common_set_notification_to_wait_for(fbe_object_id_t object_id,
                                                         fbe_topology_object_type_t object_type,
                                                         fbe_notification_type_t notification_type,
                                                         fbe_status_t expected_job_status,
                                                         fbe_job_service_error_type_t expected_job_error_code)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   object_type_bit_count = fbe_test_common_count_bits((fbe_u64_t)object_type);
    fbe_u32_t                   notification_type_bit_count = fbe_test_common_count_bits((fbe_u64_t)object_type);

    /* Trace all notifications.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "=== %s obj: 0x%x object_type: 0x%llx type: 0x%llx ===",
               __FUNCTION__, object_id, object_type, notification_type);

    /* Validate the input parameters.
     */
    if ((object_id == 0)                                            ||
        (object_id == FBE_OBJECT_ID_INVALID)                        ||
        ((object_type & fbe_test_common_notification_object_type_mask) == 0) ||
        ((notification_type & fbe_test_common_notification_type_mask) == 0)  ||
        (object_type_bit_count != 1)                                ||
        (notification_type_bit_count != 1)                             )
    {
        /* Invalid set notification request.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s obj: 0x%x object_type: 0x%llx type: 0x%llx invalid ===",
                   __FUNCTION__, object_id, object_type, notification_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Take the spinlock protecting the notification.
     */
    fbe_spinlock_lock(&fbe_test_common_notification_lock);

    /* Validate that the `notification to wait for' is not in use.
     */
    if (fbe_test_common_notification_to_wait_for.object_id != 0)
    {
        /* Notification to wait for is in-use
         */
        fbe_spinlock_unlock(&fbe_test_common_notification_lock);
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s obj: 0x%x object_type: 0x%llx type: 0x%llx wait for in-use! ===",
                   __FUNCTION__, fbe_test_common_notification_to_wait_for.object_id,
                   fbe_test_common_notification_to_wait_for.notification_info.object_type, fbe_test_common_notification_to_wait_for.notification_info.notification_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate that the notifications wasn't found on either SP.
     */
    if ((fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_A].b_found != FBE_FALSE) ||
        (fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_B].b_found != FBE_FALSE)    )
    {
        /* Already reported found
         */
        fbe_spinlock_unlock(&fbe_test_common_notification_lock);
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s obj: 0x%x object_type: 0x%llx type: 0x%llx already found SPA: %d SPB: %d===",
                   __FUNCTION__, object_id, object_type, notification_type,
                   fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_A].b_found, fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_B].b_found);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Initialize the found structures.
     */
    fbe_zero_memory(&fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_A], sizeof(fbe_test_common_sp_notification_found_t));
    fbe_zero_memory(&fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_B], sizeof(fbe_test_common_sp_notification_found_t));

    /* Now populate the `notification to wait for'
     */
    fbe_test_common_notification_to_wait_for.object_id = object_id;
    fbe_test_common_notification_to_wait_for.notification_info.object_type = object_type;
    fbe_test_common_notification_to_wait_for.notification_info.notification_type = notification_type;
    if (notification_type == FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED)
    {

        fbe_test_common_notification_to_wait_for.notification_info.notification_data.job_service_error_info.status = expected_job_status;
        fbe_test_common_notification_to_wait_for.notification_info.notification_data.job_service_error_info.error_code = expected_job_error_code;
    }
    else if (notification_type == FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION)
    {
        /* The number of reconstruct notifications is not fixed.
         */
        fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_A].b_allow_extra_notifications = FBE_TRUE;
        fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_B].b_allow_extra_notifications = FBE_TRUE;
    }
    else if (notification_type == FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED)
    {
        /*! @todo For some reason the database commits the provision drive
         *        configuration change (2) times.  Therfore allow it if the
         *        notification type is FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED.
         */
        fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_A].b_allow_extra_notifications = FBE_TRUE;
        fbe_test_common_sp_notification_found[FBE_TRANSPORT_SP_B].b_allow_extra_notifications = FBE_TRUE;
    }

    /* Return the execution status.
     */
    fbe_spinlock_unlock(&fbe_test_common_notification_lock);
    return status;
}
/******************************************************************************
 * end fbe_test_common_set_notification_to_wait_for()
 ******************************************************************************/

/*!**************************************************************
 * fbe_test_common_get_active_sp
 ****************************************************************
 * @brief
 *  This function identifies the SP that is currently defined
 *  as Active. If neither SP is currently defined as active,
 *  the output parameter is set to "invalid server".
 * 
 * @param out_active_sp_p - pointer to the active SP identifier               
 *
 * @return  void
 * 
 * @author
 *  07/2010 - Created. Susan Rundbaken (rundbs)
 * 
 ****************************************************************/
static void fbe_test_common_get_active_sp(fbe_sim_transport_connection_target_t* out_active_sp_p, fbe_bool_t b_is_dualsp_test)
{
    fbe_status_t                            spa_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_status_t                            spb_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t   target_invoked_on = FBE_SIM_SP_A;
    fbe_cmi_service_get_info_t              spa_cmi_info;
    fbe_cmi_service_get_info_t              spb_cmi_info;

    /* Get CMI info for each target */
    target_invoked_on = fbe_api_sim_transport_get_target_server();
    if (b_is_dualsp_test)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        spa_status = fbe_api_cmi_service_get_info(&spa_cmi_info);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        spb_status = fbe_api_cmi_service_get_info(&spb_cmi_info);
        fbe_api_sim_transport_set_target_server(target_invoked_on);
    }
    else
    {
        if (target_invoked_on == FBE_SIM_SP_A)
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            spa_status = fbe_api_cmi_service_get_info(&spa_cmi_info);
        }
        else
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            spb_status = fbe_api_cmi_service_get_info(&spb_cmi_info);
        }
    }

    /* Check for the active state and set the output parameter accordingly */
    if ((spa_status == FBE_STATUS_OK)                   &&
        (spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE)    )
    {
        *out_active_sp_p = FBE_SIM_SP_A;
        return;
    }

    if ((spb_status == FBE_STATUS_OK)                   &&
        (spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE)    )
    {
        *out_active_sp_p = FBE_SIM_SP_B;
        return;
    }

    /* If got this far, the active SP is not set */
    *out_active_sp_p = FBE_SIM_INVALID_SERVER;

    return;

}
/************************************** 
 * end fbe_test_common_get_active_sp()
 **************************************/

/*!****************************************************************************
 *          fbe_test_common_wait_for_notification()
 ******************************************************************************
 *
 * @brief   Wait for the already populatd notification.
 *
 * @param   function_p - Function called from 
 * @param   line - The line this is called from
 * @param   timeout_msecs - Time to wait on semaphore
 * @param   notification_info_p - Pointer to notification structure to populate
 *
 * @return  status
 *
 * @author
 *  08/15/2012  Ron Proulx  - Updated.
 ******************************************************************************/
fbe_status_t fbe_test_common_wait_for_notification(const char *function_p, fbe_u32_t line,
                                                 fbe_u32_t timeout_msecs,
                                                 fbe_notification_info_t *notification_info_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_transport_connection_target_t   this_sp;
    fbe_char_t                         *this_sp_string; 
    fbe_transport_connection_target_t   other_sp; 
    fbe_char_t                         *other_sp_string; 
    fbe_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t active_sp_sim;
    fbe_bool_t                          b_is_dualsp = FBE_FALSE;
    fbe_test_common_sp_notification_context_t   *notification_context_p = NULL;
    fbe_notification_info_t            *spb_notification_info_p = NULL;
    fbe_bool_t                          b_is_peer_alive;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Determine if peer is alive
     */
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    b_is_peer_alive = cmi_info.peer_alive;

    /* Get `this' and `other' and `active' SP.
     */
    this_sp = fbe_api_transport_get_target_server();
    other_sp = (this_sp == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;
    notification_context_p = &fbe_test_common_sp_notification_context_array[this_sp];
    fbe_test_common_get_active_sp(&active_sp_sim, notification_context_p->b_is_dualsp_test);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    switch (active_sp_sim) {
        case FBE_SIM_SP_A:
            active_sp = FBE_TRANSPORT_SP_A;
            break;
        case FBE_SIM_SP_B:
            active_sp = FBE_TRANSPORT_SP_B;
            break;
        case FBE_SIM_ADMIN_INTERFACE_PACKAGE:
        case FBE_SIM_INVALID_SERVER:
        case FBE_SIM_LAST_CONNECTION_TARGET:
        default:
            MUT_FAIL();
            break;
    }

    if (this_sp == FBE_TRANSPORT_SP_A)
    {
        this_sp_string = "SPA";
        other_sp_string = "SPB";
    }
    else
    {
        this_sp_string = "SPB";
        other_sp_string = "SPA";
    }

    /* Display some information.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "=== %s: active SP: %d Called from %s line %d timeout ms: %d ===",
               __FUNCTION__, active_sp, function_p, line, timeout_msecs);

    /* Validate that the `notification to wait for' is in use.
     */
    if (fbe_test_common_notification_to_wait_for.object_id == 0)
    {
        /* Notification to wait for is in-use
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s Called from %s line %d object_type: 0x%llx type: 0x%llx wait for not in-use! ===",
                   __FUNCTION__, function_p, line,
                   fbe_test_common_notification_to_wait_for.notification_info.object_type, fbe_test_common_notification_to_wait_for.notification_info.notification_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Get the notification information for `this' SP.
     */
    fbe_api_transport_set_target_server(this_sp);
    notification_context_p = &fbe_test_common_sp_notification_context_array[this_sp];
    b_is_dualsp = notification_context_p->b_is_dualsp_test;
    status = fbe_semaphore_wait_ms(&notification_context_p->sem, timeout_msecs);
    if ((status != FBE_STATUS_OK)                                             ||
        (fbe_test_common_sp_notification_found[this_sp].b_found == FBE_FALSE)    )
    {
        status = (status == FBE_STATUS_OK) ? FBE_STATUS_NOT_INITIALIZED : status;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s Called from %s: from %s line %d obj: 0x%x object_type: 0x%llx type: 0x%llx wait timed out !===",
                   this_sp_string, __FUNCTION__, function_p, line, 
                   fbe_test_common_notification_to_wait_for.object_id,
                   fbe_test_common_notification_to_wait_for.notification_info.object_type, fbe_test_common_notification_to_wait_for.notification_info.notification_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Copy the notification data into the structure passed.
     */
    status = FBE_STATUS_OK;
    *notification_info_p = notification_context_p->notification_info;

    /* If dualsp and the second SP is `alive' wait for the notification on the
     * `other' SP.
     */
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_transport_set_target_server(other_sp);
        notification_context_p = &fbe_test_common_sp_notification_context_array[other_sp];

        /* There is a use case where the peer is not alive.  Only check if the peer
         * is alive.
         */
        if ((notification_context_p->b_is_peer_alive == FBE_TRUE) &&
            (b_is_peer_alive == FBE_TRUE)                            )
        {
            status = fbe_semaphore_wait_ms(&notification_context_p->sem, timeout_msecs);
            if ((status != FBE_STATUS_OK)                                               ||
                (fbe_test_common_sp_notification_found[other_sp].b_found == FBE_FALSE)    )
            {
                status = (status == FBE_STATUS_OK) ? FBE_STATUS_NOT_INITIALIZED : status;
                mut_printf(MUT_LOG_TEST_STATUS, 
                       "=== %s Called from %s: from %s line %d obj: 0x%x object_type: 0x%llx type: 0x%llx wait timed out !===",
                       other_sp_string, __FUNCTION__, function_p, line, 
                       fbe_test_common_notification_to_wait_for.object_id,
                       fbe_test_common_notification_to_wait_for.notification_info.object_type, fbe_test_common_notification_to_wait_for.notification_info.notification_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }

            /* Validate that the header of SPA and SPB are the same.
             */
            spb_notification_info_p = &notification_context_p->notification_info;
            if ((notification_info_p->notification_type != spb_notification_info_p->notification_type) ||
                (notification_info_p->source_package    != spb_notification_info_p->source_package)    ||
                (notification_info_p->class_id          != spb_notification_info_p->class_id)          ||
                (notification_info_p->object_type       != spb_notification_info_p->object_type)          )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s Called from SPB: from %s line %d obj: 0x%x object_type: 0x%llx type: 0x%llx doesnt match SPA !===",
                   __FUNCTION__, function_p, line, 
                   fbe_test_common_notification_to_wait_for.object_id,
                   fbe_test_common_notification_to_wait_for.notification_info.object_type, fbe_test_common_notification_to_wait_for.notification_info.notification_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }

            /* Mark success.
             */
            status = FBE_STATUS_OK;

            /* Use the notification data from the passive side.
             */
            if (active_sp == this_sp)
            {
                *notification_info_p = notification_context_p->notification_info;
            }
        } /* end if peer is alive */

    } /* end if dualsp test*/

    /* Set the transport back to original.
     */
    fbe_api_transport_set_target_server(this_sp);

    /* Wait for a short period to see if we see `extra' notifications.
     */
    fbe_api_sleep(1000);

    /* Free the `wait for' notification buffer(s).
     */
	fbe_spinlock_lock(&fbe_test_common_notification_lock);
    fbe_zero_memory(&fbe_test_common_sp_notification_found[this_sp], sizeof(fbe_test_common_sp_notification_found_t));
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_zero_memory(&fbe_test_common_sp_notification_found[other_sp], sizeof(fbe_test_common_sp_notification_found_t));
    }
    fbe_test_common_notification_to_wait_for.object_id = 0;
    fbe_spinlock_unlock(&fbe_test_common_notification_lock);

    /* Return success.
     */
    return status;
}
/******************************************************************************
 * end fbe_test_common_wait_for_notification()
 ******************************************************************************/

/*!**************************************************************
 *          fbe_test_common_cleanup_notifications()
 **************************************************************** 
 * 
 * @brief   Cleanup notificaitons.
 *
 * @param   b_is_dualsp_test - FBE_TRUE - This is a dualsp test           
 *
 * @return  status
 *
 * @author
 *  09/25/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_test_common_cleanup_notifications(fbe_bool_t b_is_dualsp_test)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_common_sp_notification_context_t   *notification_context_p = NULL;

    /* First unregistger for notifications.
     */
    status = fbe_test_common_unregister_notifications();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Destroy semaphore
     */
    notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_A];
    notification_context_p->b_is_dualsp_test = FBE_FALSE;
    fbe_semaphore_destroy(&notification_context_p->sem);
       
    /* Destroy semaphore for SP B.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_B];
        notification_context_p->b_is_dualsp_test = FBE_FALSE;
        fbe_semaphore_destroy(&notification_context_p->sem);
    }

    /* Destroy the spinlock.
     */
    fbe_spinlock_destroy(&fbe_test_common_notification_lock);

    return status;
}
/***********************************************
 * end fbe_test_common_cleanup_notifications()
 ***********************************************/

/*!***************************************************************************
 *          fbe_test_common_notify_mark_peer_dead()
 *****************************************************************************
 * 
 * @brief   If this is a dualsp test mark the peer as `dead' so that we don't 
 *          wait for the notifications from the peer.
 *
 * @param   b_is_dualsp_test - FBE_TRUE - This is a dualsp test           
 *
 * @return  status
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_common_notify_mark_peer_dead(fbe_bool_t b_is_dualsp_test)
{
    fbe_test_common_sp_notification_context_t *notification_context_p = NULL;

    /* Only need to do this if it is a dualsp test.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        /* Set the `peer alive' flag in both notification context to `False'.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s Mark peer dead ===",
                   __FUNCTION__);
        fbe_spinlock_lock(&fbe_test_common_notification_lock);
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_A];
        notification_context_p->b_is_peer_alive = FBE_FALSE;
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_B];
        notification_context_p->b_is_peer_alive = FBE_FALSE;
        fbe_spinlock_unlock(&fbe_test_common_notification_lock);
    }

    return FBE_STATUS_OK;
}
/*********************************************
 * end fbe_test_common_notify_mark_peer_dead()
 *********************************************/

/*!***************************************************************************
 *          fbe_test_common_notify_mark_peer_alive()
 *****************************************************************************
 * 
 * @brief   If this is a dualsp test mark the peer as `alive' so that we 
 *          wait for the notifications from the peer.
 *
 * @param   b_is_dualsp_test - FBE_TRUE - This is a dualsp test           
 *
 * @return  status
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_common_notify_mark_peer_alive(fbe_bool_t b_is_dualsp_test)
{
    fbe_test_common_sp_notification_context_t *notification_context_p = NULL;

    /* Only need to do this if it is a dualsp test.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        /* Set the `peer alive' flag in both notification context to `False'.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "=== %s Mark peer alive ===",
                   __FUNCTION__);
        fbe_spinlock_lock(&fbe_test_common_notification_lock);
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_A];
        notification_context_p->b_is_peer_alive = FBE_TRUE;
        notification_context_p = &fbe_test_common_sp_notification_context_array[FBE_TRANSPORT_SP_B];
        notification_context_p->b_is_peer_alive = FBE_TRUE;
        fbe_spinlock_unlock(&fbe_test_common_notification_lock);
    }

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_test_common_notify_mark_peer_alive()
 ***********************************************/


/************************************
 * end file fbe_test_common_notify.c
 ************************************/
