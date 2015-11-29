/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_process_encryption_keys.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for processing the encryption keys
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   10/03/2013
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_encryption.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_process_encryption_keys_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_process_encryption_keys_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_process_encryption_keys_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_process_encryption_keys_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_process_encryption_keys_commit_function(fbe_packet_t *packet_p);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_process_encryption_keys_job_service_operation = 
{
    FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS,
    {
        /* validation function */
        fbe_process_encryption_keys_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_process_encryption_keys_update_configuration_in_memory_function,

        /* persist function */
        fbe_process_encryption_keys_persist_function,

        /* response/rollback function */
        fbe_process_encryption_keys_rollback_function,

        /* commit function */
        fbe_process_encryption_keys_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/


/*!**************************************************************
 * fbe_process_encryption_keys_validation_function
 ****************************************************************
 * @brief
 * This function validates the encryption request to make sure that it is valid.
 * 
 * @param packet_p - packet info
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_process_encryption_keys_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_service_process_encryption_keys_t * 		process_encryption_keys_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_encryption_key_table_t                         *keys_p = NULL;
    
    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL){

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t)){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((job_queue_element_p->local_job_data_valid != FBE_TRUE) || 
        (job_queue_element_p->additional_job_data == NULL))
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    keys_p = (fbe_encryption_key_table_t *)job_queue_element_p->additional_job_data;

    process_encryption_keys_request_p = (fbe_job_service_process_encryption_keys_t*) job_queue_element_p->command_data;

    if ((process_encryption_keys_request_p->operation == FBE_JOB_SERVICE_ENCRYPTION_KEY_OPERATION_NULL) ||
        (process_encryption_keys_request_p->operation == FBE_JOB_SERVICE_ENCRYPTION_KEY_OPERATION_INVALID) ||
        (keys_p->num_of_entries > FBE_ENCRYPTION_KEYS_BATCH_PROCESSING_SIZE))
    {
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Failed.  Check Operation: %d or number of entries: %d\n",
                          __FUNCTION__, process_encryption_keys_request_p->operation, 
                          keys_p->num_of_entries);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

} // end of fbe_process_encryption_keys_validation_function


/*!**************************************************************
 * fbe_process_encryption_keys_update_configuration_in_memory_function
 ****************************************************************
 * @brief
 * This function updates the encryption request in memory.
 * 
 * @param packet_p - packet info
 *
 * @return status - status of init call
 *
 * @author
 *    14-Nov-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_process_encryption_keys_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_process_encryption_keys_t * 		process_encryption_keys_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL){

		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t)){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (job_queue_element_p->local_job_data_valid != FBE_TRUE)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    process_encryption_keys_request_p = (fbe_job_service_process_encryption_keys_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_start_database_transaction(&process_encryption_keys_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to start process encryption transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (process_encryption_keys_request_p->operation)
    {
        case FBE_JOB_SERVICE_SETUP_ENCRYPTION_KEYS:
            /* Send the update provision drive configuration to database service */
            status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_KEY,
                                                        job_queue_element_p->additional_job_data,
                                                        sizeof(fbe_database_control_setup_encryption_key_t),
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_DATABASE,
                                                        FBE_CLASS_ID_INVALID,
                                                        FBE_OBJECT_ID_INVALID,
                                                        FBE_PACKET_FLAG_NO_ATTRIB);
            if (status != FBE_STATUS_OK) {
                job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return status;
            }
            break;

        case FBE_JOB_SERVICE_DELETE_ENCRYPTION_KEYS:
            break;
            
        case FBE_JOB_SERVICE_UPDATE_ENCRYPTION_KEYS:
            break;

        default:
            break;
        
    }

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

} // end of fbe_process_encryption_keys_update_configuration_in_memory_function


/*!**************************************************************
 * fbe_process_encryption_keys_rollback_function
 ****************************************************************
 * @brief
 * This function rollbacks the encryption request.
 * 
 * @param packet_p - packet info
 *
 * @return status - status of init call
 *
 * @author
 *    04-Oct-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_process_encryption_keys_rollback_function(fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    fbe_job_service_process_encryption_keys_t * 		process_encryption_keys_request_p = NULL;
    fbe_payload_control_buffer_length_t  length = 0;
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
	fbe_notification_info_t              notification_info;
    fbe_status_t                         status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL){

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t)){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    process_encryption_keys_request_p = (fbe_job_service_process_encryption_keys_t *)job_queue_element_p->command_data;

    if (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_VALIDATE){
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s entry, rollback not required, previous state %d, current state %d\n", __FUNCTION__, 
                job_queue_element_p->previous_state,
                job_queue_element_p->current_state);
    }

    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)){
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: aborting transaction id %llu\n",
                __FUNCTION__, (unsigned long long)process_encryption_keys_request_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(process_encryption_keys_request_p->transaction_id);
        if(status != FBE_STATUS_OK){
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Could not abort transaction id %llu with configuration service\n", 
                    __FUNCTION__,
            (unsigned long long)process_encryption_keys_request_p->transaction_id);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_GENERIC_FAILURE;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS;
    /*We update several pvds, so we can't provide a unique object id. What we care in this notification is job_number and job type */ 
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID; 

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: status change notification failed, status: %d\n",
                __FUNCTION__, status);
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
    fbe_payload_control_set_status(control_operation, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
    
} // end of fbe_process_encryption_keys_rollback_function

/*!**************************************************************
 * fbe_process_encryption_keys_persist_function
 ****************************************************************
 * @brief
 * This function persists the encryption info.
 * 
 * @param
 *    packet_p - packet info
 *
 * @return status - status of init call
 *
 * @author
 *    14-Nov-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_process_encryption_keys_persist_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_process_encryption_keys_t * 		process_encryption_keys_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    
    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL){

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t)){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    process_encryption_keys_request_p = (fbe_job_service_process_encryption_keys_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_commit_database_transaction(process_encryption_keys_request_p->transaction_id);
    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

} // end of fbe_process_encryption_keys_persist_function

/*!**************************************************************
 * fbe_process_encryption_keys_commit_function
 ****************************************************************
 * @brief
 * This function commits the encryption info.
 * 
 * @param
 *    packet_p - packet info
 *
 * @return status - status of init call
 *
 * @author
 *    14-Nov-2013 - Created
 * 
 ****************************************************************/
static fbe_status_t fbe_process_encryption_keys_commit_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        	status = FBE_STATUS_OK;
    fbe_job_service_process_encryption_keys_t * 		process_encryption_keys_request_p = NULL;
    fbe_payload_ex_t                       *	payload_p = NULL;
    fbe_payload_control_operation_t     *	control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 	length = 0;
    fbe_job_queue_element_t             *	job_queue_element_p = NULL;
	fbe_notification_info_t              	notification_info;
    
    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL){

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t)){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    process_encryption_keys_request_p = (fbe_job_service_process_encryption_keys_t *) job_queue_element_p->command_data;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
	notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: status change notification failed, status %d\n",
                __FUNCTION__, status);
        
    }

    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

	return FBE_STATUS_OK;

} // end of fbe_process_encryption_keys_commit_function


