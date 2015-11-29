/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_set_encryption_paused.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for pausing or resuming background encryption
 * 
 * @ingroup job_lib_files
 * 
 * @version
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

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_set_encryption_paused_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_encryption_paused_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_encryption_paused_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_encryption_paused_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_pause_encryption_database_pause(fbe_database_transaction_id_t transaction_id, 
                                                        fbe_bool_t encryption_paused);
/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_set_encryption_paused_job_service_operation = 
{
    FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED,
    {
        /* validation function */
        fbe_set_encryption_paused_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_set_encryption_paused_update_configuration_in_memory_function,

        /* persist function */
        fbe_set_encryption_paused_persist_function,

        /* response/rollback function */
        fbe_set_encryption_paused_rollback_function,

        /* commit function */
        NULL,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_set_encryption_paused_validation_function
 ****************************************************************
 * @brief
 * This function validates the encryption request to make sure that it is valid.
 * 
 * @param packet_p - packet paused
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_set_encryption_paused_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_service_pause_encryption_t   *pause_encryption_request_p = NULL;
    fbe_payload_ex_t                     *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t   length = 0;
    fbe_job_queue_element_t              *job_queue_element_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s Entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pause_encryption_request_p = (fbe_job_service_pause_encryption_t *) job_queue_element_p->command_data;

    if ((pause_encryption_request_p->encryption_paused == FBE_TRUE) ||
        (pause_encryption_request_p->encryption_paused == FBE_FALSE))
    {
        /* Set the payload status */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

        /* update job_queue_element status */
        job_queue_element_p->status = FBE_STATUS_OK;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

        /* Complete the packet so that the job service thread can continue */
        pause_encryption_request_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_OK;
    }

    /* Not valid state */
    pause_encryption_request_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

    job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_GENERIC_FAILURE;

} /* end of fbe_set_encryption_paused_validation_function() */

/*!**************************************************************
 * fbe_set_encryption_paused_update_configuration_in_memory_function
 ****************************************************************
 * @brief
 * This function updates the encryption request in memory.
 * 
 * @param packet_p - packet paused
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_set_encryption_paused_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_pause_encryption_t * pause_encryption_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s Entry\n", __FUNCTION__);


    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    pause_encryption_request_p = (fbe_job_service_pause_encryption_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_start_database_transaction(&pause_encryption_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to start set power save transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*store it in the configuration which will also updte memory and both SPs*/
    status = fbe_pause_encryption_database_pause(pause_encryption_request_p->transaction_id, 
                                                 pause_encryption_request_p->encryption_paused);
    if (status != FBE_STATUS_OK) {
        pause_encryption_request_p->error_code = FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED;
    }

    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    pause_encryption_request_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

} /* end of fbe_set_encryption_paused_update_configuration_in_memory_function() */

/*!**************************************************************
 * fbe_set_encryption_paused_rollback_function
 ****************************************************************
 * @brief
 * This function rollbacks the encryption request.
 * 
 * @param packet_p - packet paused
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_set_encryption_paused_rollback_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_pause_encryption_t     *pause_encryption_request_p = NULL;
    fbe_payload_ex_t                                    *payload_p = NULL;
    fbe_payload_control_operation_t                     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;
    fbe_job_queue_element_t                             *job_queue_element_p = NULL;
    fbe_notification_info_t                             notification_info;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s Entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    pause_encryption_request_p = (fbe_job_service_pause_encryption_t *) job_queue_element_p->command_data;
    
    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_GENERIC_FAILURE;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, action state: %d\n",
                          __FUNCTION__, status);
    }


    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

} /* end of fbe_set_encryption_paused_rollback_function() */

/*!**************************************************************
 * fbe_set_encryption_paused_database_service_set_paused
 ****************************************************************
 * @brief
 * This function set paused in the database the encryption request.
 * 
 * @param
 *    transaction_id - transaction id
 *    encryption_paused_p - encryption paused
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_pause_encryption_database_pause(fbe_database_transaction_id_t transaction_id, 
                                                        fbe_bool_t encryption_paused)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_encryption_t   database_encryption;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s Entry\n", __FUNCTION__);

    fbe_zero_memory(&database_encryption, sizeof(database_encryption));
    database_encryption.transaction_id = transaction_id;
    database_encryption.system_encryption_info.encryption_paused = encryption_paused;
    
    
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_SET_ENCRYPTION_PAUSED,
                                                 &database_encryption,
                                                 sizeof(fbe_database_encryption_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
        return status;
    }

    return status;

} /* end of fbe_set_encryption_paused_database_service_set_paused() */


/*!**************************************************************
 * fbe_set_encryption_paused_persist_function
 ****************************************************************
 * @brief
 * This function persists the encryption paused.
 * 
 * @param
 *    packet_p - packet paused
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_set_encryption_paused_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_pause_encryption_t    *pause_encryption_request_p = NULL;
    fbe_payload_ex_t                                   *payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t                            *job_queue_element_p = NULL;
    fbe_notification_info_t             				notification_info;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s Entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    pause_encryption_request_p = (fbe_job_service_pause_encryption_t *) job_queue_element_p->command_data;

    /*update the config service which will persist it*/
    status = fbe_create_lib_commit_database_transaction(pause_encryption_request_p->transaction_id);

    if (status == FBE_STATUS_OK) {
        /* Send a notification for this commit*/
        notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
        notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
        notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
        notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
        notification_info.notification_data.job_service_error_info.error_code = pause_encryption_request_p->error_code;

        notification_info.class_id = FBE_CLASS_ID_INVALID;
        notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
        status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
        if (status != FBE_STATUS_OK) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: job status notification failed, status: %d\n",
                              __FUNCTION__, status);
        }
    }
    else
    {
        pause_encryption_request_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    }

    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

} /* end of fbe_set_encryption_paused_persist_function()*/


