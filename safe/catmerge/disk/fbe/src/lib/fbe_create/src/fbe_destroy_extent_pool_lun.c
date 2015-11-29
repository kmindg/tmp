/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_destroy_ext_pool_lun.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for lun destroy used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   07/02/2014:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_notification.h"
#include "fbe/fbe_event_log_api.h"   // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h" // for message codes

/*************************
 *   FORWARD DECLARATIONS
 *************************/

/* Lun destroy library request validation function.
 */
static fbe_status_t fbe_destroy_ext_pool_lun_validation_function (fbe_packet_t *packet_p);
/* Lun destroy library update configuration in memory function.
 */
static fbe_status_t fbe_destroy_ext_pool_lun_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Lun destroy library persist configuration in db function.
 */
static fbe_status_t fbe_destroy_ext_pool_lun_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Lun destroy library rollback function.
 */
static fbe_status_t fbe_destroy_ext_pool_lun_rollback_function (fbe_packet_t *packet_p);

/* Lun destroy library commit function.
 */
static fbe_status_t fbe_destroy_ext_pool_lun_commit_function (fbe_packet_t *packet_p);

/* Utility function to destroy the LUN by sending the appropriate control packet to the Config Service */
static fbe_status_t fbe_destroy_ext_pool_lun_destroy_lun(fbe_object_id_t object_id, fbe_database_transaction_id_t transaction_id);

/* Utility function to get a count of the LUN Object's upstream edges from the Topology Service */
static fbe_status_t fbe_destroy_ext_pool_lun_get_upstream_edge_count(fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_edges);

/* Utility function to send a "prepare for destroy" usurper command to the LUN object */
static fbe_status_t fbe_destroy_ext_pool_lun_send_prepare_to_destroy_usurper_cmd_to_lun(fbe_object_id_t   object_id);


/*************************
 *   DEFINITIONS
 *************************/

/* Job service lun destroy registration.
*/
fbe_job_service_operation_t fbe_destroy_extent_pool_lun_job_service_operation = 
{
    FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN,
    {
        /* validation function */
        fbe_destroy_ext_pool_lun_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_destroy_ext_pool_lun_update_configuration_in_memory_function,

        /* persist function */
        fbe_destroy_ext_pool_lun_persist_configuration_db_function,

        /* response/rollback function */
        fbe_destroy_ext_pool_lun_rollback_function,

        /* commit function */
        fbe_destroy_ext_pool_lun_commit_function,
    }
};



/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_destroy_ext_pool_lun_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/02/2010 - Created. Lili Chen
 ****************************************************************/

static fbe_status_t fbe_destroy_ext_pool_lun_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_destroy_ext_pool_lun_t           *destroy_ext_pool_lun_request_p = NULL;
    fbe_payload_control_operation_t         *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_job_queue_element_t                 *job_queue_element_p = NULL;
    fbe_payload_ex_t                        *payload_p = NULL;

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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    destroy_ext_pool_lun_request_p = (fbe_job_service_destroy_ext_pool_lun_t*)job_queue_element_p->command_data;

    /* Get the LUN object id from Config Service */
    status = fbe_create_lib_database_service_lookup_ext_pool_lun_object_id(FBE_POOL_ID_INVALID, 
                                                                           destroy_ext_pool_lun_request_p->lun_id, 
                                                                           &destroy_ext_pool_lun_request_p->object_id);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s failed to lookup lun %d's object id; status %d\n",
                          __FUNCTION__, destroy_ext_pool_lun_request_p->lun_id, status);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ID; 
 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "%s: lookup lun %d has object id 0x%x\n",
                      __FUNCTION__, 
                      destroy_ext_pool_lun_request_p->lun_id, 
                      destroy_ext_pool_lun_request_p->object_id);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_destroy_ext_pool_lun_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_ext_pool_lun_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/02/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_ext_pool_lun_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_destroy_ext_pool_lun_t               *destroy_ext_pool_lun_request_p = NULL;
    fbe_payload_control_operation_t             *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t                     *job_queue_element_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_status_t                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;


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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    destroy_ext_pool_lun_request_p = (fbe_job_service_destroy_ext_pool_lun_t*)job_queue_element_p->command_data;

    destroy_ext_pool_lun_request_p->transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* Open a transaction with the Configuration Service */
    status = fbe_create_lib_start_database_transaction(&destroy_ext_pool_lun_request_p->transaction_id, job_queue_element_p->job_number);

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s cannot start destroy transaction with Config Service; LUN %d; status %d\n",
                          __FUNCTION__, destroy_ext_pool_lun_request_p->lun_id, status);
		
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                      "%s: destroy LUN %d - obj0x%x\n",
                      __FUNCTION__, destroy_ext_pool_lun_request_p->lun_id, destroy_ext_pool_lun_request_p->object_id);

    /* Use Configuration Service to destroy LUN object */
    status = fbe_destroy_ext_pool_lun_destroy_lun(destroy_ext_pool_lun_request_p->object_id, destroy_ext_pool_lun_request_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s cannot destroy LUN %d; status %d\n",
                          __FUNCTION__, destroy_ext_pool_lun_request_p->lun_id, status);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_destroy_ext_pool_lun_update_configuration_in_memory_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_ext_pool_lun_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/02/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_ext_pool_lun_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_destroy_ext_pool_lun_t       *destroy_ext_pool_lun_request_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;


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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    destroy_ext_pool_lun_request_p = (fbe_job_service_destroy_ext_pool_lun_t*)job_queue_element_p->command_data;

    /* Commit the transaction */
    status = fbe_create_lib_commit_database_transaction(destroy_ext_pool_lun_request_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s commit transaction failed; status %d\n",
                          __FUNCTION__, status);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;  
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    
    return status;
}
/**********************************************************
 * end fbe_destroy_ext_pool_lun_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_ext_pool_lun_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/02/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_ext_pool_lun_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_destroy_ext_pool_lun_t       *destroy_ext_pool_lun_request_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;

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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    destroy_ext_pool_lun_request_p = (fbe_job_service_destroy_ext_pool_lun_t*)job_queue_element_p->command_data;

    /*!TODO: how will rollback know what to rollback? */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)
            && (destroy_ext_pool_lun_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID))
    {
        status = fbe_create_lib_abort_database_transaction(destroy_ext_pool_lun_request_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
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
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, status: 0x%X\n",
                          __FUNCTION__, status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_destroy_ext_pool_lun_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_ext_pool_lun_commit_function()
 ****************************************************************
 * @brief
 * This function is used to commit the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/02/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_ext_pool_lun_commit_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status= FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;
    fbe_job_service_destroy_ext_pool_lun_t       *destroy_ext_pool_lun_request_p = NULL;

    /*! Get the request payload for validation */
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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    destroy_ext_pool_lun_request_p = (fbe_job_service_destroy_ext_pool_lun_t*)job_queue_element_p->command_data;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
		FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
		"%s entry, job number 0x%llx, status 0x%x, error_code %d\n",
		__FUNCTION__,
		(unsigned long long)job_queue_element_p->job_number, 
		job_queue_element_p->status,
		job_queue_element_p->error_code);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = destroy_ext_pool_lun_request_p->object_id;
    //job_queue_element_p->need_to_wait = destroy_ext_pool_lun_request_p->wait_destroy;
    //job_queue_element_p->timeout_msec = destroy_ext_pool_lun_request_p->destroy_timeout_msec;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = destroy_ext_pool_lun_request_p->object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    //notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_LUN_DESTROY;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(1) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(destroy_ext_pool_lun_request_p->object_id, 
                                                                  FBE_LIFECYCLE_STATE_NOT_EXIST, 
                                                                  30000);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting lun object id %d destroy. status %d\n", 
                              __FUNCTION__,  destroy_ext_pool_lun_request_p->object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    status = fbe_notification_send(destroy_ext_pool_lun_request_p->object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);


    return status;
}
/**********************************************************
 * end fbe_destroy_ext_pool_lun_commit_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_ext_pool_lun_database_service_destroy_lun()
 ****************************************************************
 * @brief
 * This function sends a control packet to the Database Service 
 * to destroy a lun object.  
 *
 * !TODO consider moving this function to the appropriate utility file
 *
 * @param object_id         - object edge connected to (IN)
 * @param transaction ID    - Config Service transaction id (IN)
 *
 * @return status - status of the operation
 *
 * @author
 *  07/02/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_ext_pool_lun_destroy_lun(fbe_object_id_t object_id, fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_destroy_object_t  destroy_lun;

    /* Set the ID of the LUN to destroy */
    destroy_lun.object_id       = object_id;
    destroy_lun.transaction_id  = transaction_id;

    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL_LUN,
												&destroy_lun,
												sizeof(fbe_database_control_destroy_object_t),
												FBE_PACKAGE_ID_SEP_0,
												FBE_SERVICE_ID_DATABASE,
												FBE_CLASS_ID_LUN,
												destroy_lun.object_id,
												FBE_PACKET_FLAG_NO_ATTRIB);

    /* If failure status returned, log an error and return immediately */
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
        return status;
    }

    return status;
}
/******************************************************************
 * end fbe_destroy_ext_pool_lun_destroy_lun()
 *****************************************************************/
