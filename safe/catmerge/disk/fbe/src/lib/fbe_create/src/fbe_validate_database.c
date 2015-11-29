/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_validate_database.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for validating the in-memory database against
 *  the on-disk database.  If any descrepencies are found the `failure action'
 *  is taken.
 * 
 * @ingroup job_lib_files
 * 
 * @author
 *  07/01/2014  Ron Proulx  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_base_config.h"
#include "fbe_database.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_validate_database_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_validate_database_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_validate_database_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_validate_database_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_validate_database_commit_function(fbe_packet_t *packet_p);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_validate_database_job_service_operation = 
{
    FBE_JOB_TYPE_VALIDATE_DATABASE,
    {
        /* validation function */
        fbe_validate_database_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_validate_database_update_configuration_in_memory_function,

        /* persist function */
        fbe_validate_database_persist_function,

        /* response/rollback function */
        fbe_validate_database_rollback_function,

        /* commit function */
        fbe_validate_database_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 *          fbe_validate_database_validation_function()
 ****************************************************************************** 
 * 
 * @brief   Validate the `validate database' job request.
 * 
 * @param   packet_p - Packet with validate database request.
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/01/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_validate_database_validation_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;
    fbe_job_service_validate_database_t *validate_database_p = NULL;
    fbe_database_control_is_validation_allowed_t is_validation_allowed;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t)) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the request.
     */
    validate_database_p = (fbe_job_service_validate_database_t *)job_queue_element_p->command_data;

    /* Validate that either this is a user request or the request is allowed.
     */
    fbe_zero_memory(&is_validation_allowed, sizeof(fbe_database_control_is_validation_allowed_t));
    is_validation_allowed.validate_caller = validate_database_p->validate_caller;
    if (validate_database_p->validate_caller != FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER) {
        /* Check if validation is allowed.
         */
        status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_IS_VALIDATION_ALLOWED,
                                                     &is_validation_allowed,
                                                     sizeof(fbe_database_control_is_validation_allowed_t),
                                                     FBE_PACKAGE_ID_SEP_0,
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: is allowed failed - status: %d\n",
                              __FUNCTION__, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If not allowed fail the request.
         */
        if (!is_validation_allowed.b_allowed) {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: caller: %d not allowed reason: %d\n",
                              __FUNCTION__, is_validation_allowed.validate_caller,
                              is_validation_allowed.not_allowed_reason);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    } /* end if not a user request*/

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_validate_database_validation_function()
 *************************************************/

/*!****************************************************************************
 *          fbe_validate_database_update_configuration_in_memory_function()
 ****************************************************************************** 
 * 
 * @brief   Issue the `validate database' job request.
 * 
 * @param   packet_p - Packet with validate database request.
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/01/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_validate_database_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;
    fbe_job_service_validate_database_t *validate_database_p = NULL;
    fbe_database_control_validate_database_t validate_request;
    fbe_database_control_enter_degraded_mode_t  enter_degraded_mode;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t)) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the validate database usurper.
     */
    validate_database_p = (fbe_job_service_validate_database_t *)job_queue_element_p->command_data;
    validate_request.validate_caller = validate_database_p->validate_caller;
    validate_request.failure_action = validate_database_p->failure_action;

    /* Start the transaction.
     */
    status = fbe_create_lib_start_database_transaction(&validate_database_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start transaction - status: 0x%x\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send the request to the database service.
     */
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_VALIDATE_DATABASE,
                                                 &validate_request,
                                                 sizeof(fbe_database_control_validate_database_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s validation failed - status: 0x%x\n", 
                          __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        validate_database_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    } else {  
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        if (validate_request.validation_status == FBE_STATUS_OK) {
            /* Validation was successful.
             */
            validate_database_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s validation successful.\n", 
                              __FUNCTION__);
        } else if (validate_request.validation_status == FBE_STATUS_SERVICE_MODE) {
            /* Validation completed but failed.
             */
            validate_database_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "validate_database: %d validation of database failed.  Take action: %d \n", 
                              validate_request.validate_caller, validate_request.failure_action);

            /* Always generate an event log.
             */
            fbe_event_log_write(SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED,
                                NULL, 0, NULL);

            /* Take the requested action.
             */
            switch(validate_request.failure_action) {
                case FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE:
                    /* We have already generated an error trace. */
                    break;
                case FBE_DATABASE_VALIDATE_FAILURE_ACTION_ENTER_DEGRADED_MODE:
                case FBE_DATABASE_VALIDATE_FAILURE_ACTION_PANIC_SP:
                    /* Enter degraded mode.
                     */
                    enter_degraded_mode.degraded_mode_reason = FBE_DATABASE_SERVICE_MODE_REASON_DB_VALIDATION_FAILED;
                    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_ENTER_DEGRADED_MODE,
                                                 &enter_degraded_mode,
                                                 sizeof(fbe_database_control_enter_degraded_mode_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK) {
                        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                          "%s enter degraded mode failed - status: 0x%x \n", 
                                          __FUNCTION__, status);
                    }

                    /* If requested PANIC this SP.
                     */
                    if (validate_request.failure_action == FBE_DATABASE_VALIDATE_FAILURE_ACTION_PANIC_SP) {
                        job_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                          "%s caller: %d requested PANIC on database validation failure \n", 
                                          __FUNCTION__, validate_request.validate_caller);
                    }
                    break;
                case FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY:
                default:
                    job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                      "%s caller: %d validation failed but unsupported action: %d\n", 
                                      __FUNCTION__, validate_request.validate_caller,
                                      validate_request.failure_action);
            }
        } else {
            /* Validation failed.
             */
            validate_database_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s validation of database failed - status: 0x%x\n", 
                              __FUNCTION__, validate_request.validation_status);

        }
    }

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/*********************************************************************
 * end fbe_validate_database_update_configuration_in_memory_function()
 *********************************************************************/

/*!****************************************************************************
 *          fbe_validate_database_rollback_function()
 ****************************************************************************** 
 * 
 * @brief   Validate database failed.  Simply generate an error trace.
 * 
 * @param   packet_p - Packet with validate database request.
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/01/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_validate_database_rollback_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;
    fbe_job_service_validate_database_t *validate_database_p = NULL;
    fbe_notification_info_t             notification_info;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t)) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the validation request.
     */
    validate_database_p = (fbe_job_service_validate_database_t *)job_queue_element_p->command_data;

    /* Generate an error trace.
     */
    job_service_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "validate database - job: 0x%llx caller: %d action: %d failed with error: 0x%x\n",
                      validate_database_p->job_number,
                      validate_database_p->validate_caller,
                      validate_database_p->failure_action, 
                      validate_database_p->error_code);

    /* Must `terminate' the transaction.
     */
    if (((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)           ||
         (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)    ) &&    
        (validate_database_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID)             ) {
        status = fbe_create_lib_abort_database_transaction(validate_database_p->transaction_id);
    }

    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = job_queue_element_p->status;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK)  {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "validate_database: job status notification failed- status: 0x%x\n",
                          status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_validate_database_rollback_function()
 ************************************************/

/*!****************************************************************************
 *          fbe_validate_database_persist_function()
 ****************************************************************************** 
 * 
 * @brief   Persist any changes that the validation made.
 * 
 * @param   packet_p - Packet with validate database request.
 * 
 * @return  status - status of the operation.
 * 
 * @note    Currently the action `correct' is not supported so this operation
 *          is a noop.
 *
 * @author
 *  07/01/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_validate_database_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;
    fbe_job_service_validate_database_t *validate_database_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t)) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the validation request.
     */
    validate_database_p = (fbe_job_service_validate_database_t *)job_queue_element_p->command_data;

    /*! @note In the future if the action was `correct' we would persist any changes. 
     */
    if ((validate_database_p->failure_action == FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY) &&
        (validate_database_p->error_code == FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED)                ) {
        /*! @note Perform corrective action here. */
        //status = fbe_create_lib_commit_database_transaction(validate_database_p->transaction_id);
    } else {
        /* Must `terminate' the transaction.
        */
        if (validate_database_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID) {
            status = fbe_create_lib_abort_database_transaction(validate_database_p->transaction_id);
        }
    }
        
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************
 * end fbe_validate_database_persist_function()
 **********************************************/

/*!****************************************************************************
 *          fbe_validate_database_rollback_function()
 ****************************************************************************** 
 * 
 * @brief   Validate database commit.  If `correct' was selected (and is supported)
 *          commit the modifed objects.
 * 
 * @param   packet_p - Packet with validate database request.
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/06/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_validate_database_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;
    fbe_job_service_validate_database_t *validate_database_p = NULL;
    fbe_notification_info_t             notification_info;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t)) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the validation request.
     */
    validate_database_p = (fbe_job_service_validate_database_t *)job_queue_element_p->command_data;

    /* Generate an info trace
     */
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "validate database - job: 0x%llx caller: %d action: %d status: 0x%x code: %d\n",
                      validate_database_p->job_number,
                      validate_database_p->validate_caller,
                      validate_database_p->failure_action,
                      job_queue_element_p->status, 
                      validate_database_p->error_code);

    /*! @note  If `correct' is supported commit the corrected objects here
     */

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = job_queue_element_p->status;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK)  {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "validate_database: job status notification failed- status: 0x%x\n",
                          status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_validate_database_commit_function()
 ************************************************/

/**************************************
 * end of file fbe_validate_database.c
 **************************************/

