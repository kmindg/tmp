/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_create_extent_pool_lun.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for extent pool lun create used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   06/10/2014:  Created. Lili Chen
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
#include "fbe/fbe_bvd_interface.h"
#include "fbe_notification.h"
#include "fbe_database.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes
#include "fbe_private_space_layout.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

/* Lun create library request validation function. 
 */
static fbe_status_t fbe_create_extent_pool_lun_validation_function (fbe_packet_t *packet_p);
/* Lun create library update configuration in memory function.
 */
static fbe_status_t fbe_create_extent_pool_lun_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Lun create library persist configuration in db function.
 */
static fbe_status_t fbe_create_extent_pool_lun_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Lun create library rollback function.
 */
static fbe_status_t fbe_create_extent_pool_lun_rollback_function (fbe_packet_t *packet_p);

/* Lun create library commit function.
 */
static fbe_status_t fbe_create_extent_pool_lun_commit_function (fbe_packet_t *packet_p);

static fbe_status_t fbe_create_extent_pool_lun_create_lun(fbe_job_service_create_ext_pool_lun_t * create_lun_request_p);


/*************************
 *   DEFINITIONS
 *************************/

/* Job service lun create registration.
*/
fbe_job_service_operation_t fbe_create_extent_pool_lun_job_service_operation = 
{
    FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN,
    {
        /* validation function */
        fbe_create_extent_pool_lun_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_create_extent_pool_lun_update_configuration_in_memory_function,

        /* persist function */
        fbe_create_extent_pool_lun_persist_configuration_db_function,

        /* response/rollback function */
        fbe_create_extent_pool_lun_rollback_function,

        /* commit function */
        fbe_create_extent_pool_lun_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_create_extent_pool_lun_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the Lun create request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2010 - Created. Lili Chen
 ****************************************************************/

static fbe_status_t fbe_create_extent_pool_lun_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_ext_pool_lun_t *      lun_create_request_p = NULL;
    fbe_payload_ex_t                    *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_lifecycle_state_t               lifecycle_state;
    fbe_object_id_t                     pool_object_id;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "create_extent_pool_lun_valid_fun: job_queue_element is NULL.\n");
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "create_extent_pool_lun_valid_fun: fbe_job_queue_element wrong size\n");
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_create_request_p = (fbe_job_service_create_ext_pool_lun_t *) job_queue_element_p->command_data;

	job_queue_element_p->status = FBE_STATUS_OK;
	job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Get the ext pool object id from config service */
    status = fbe_create_lib_database_service_lookup_ext_pool_object_id(lun_create_request_p->pool_id, 
                                                                       &pool_object_id);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ID;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_get_object_state(pool_object_id, &lifecycle_state);
    if((status != FBE_STATUS_OK)||((lifecycle_state != FBE_LIFECYCLE_STATE_READY) && (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)))
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

#if 0
	/*validate we can create the lun as far as the system limits go*/
    status = fbe_create_extent_pool_lun_validate_system_limits(lun_create_request_p);
	if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SYSTEM_LIMITS_EXCEEDED;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_create_extent_pool_lun_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_create_extent_pool_lun_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the Lun create request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_create_extent_pool_lun_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_ext_pool_lun_t        *lun_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
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
    lun_create_request_p = (fbe_job_service_create_ext_pool_lun_t *) job_queue_element_p->command_data;
    lun_create_request_p->transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* open_transaction in configuration service.  should the type of transaction be given here? */
    status = fbe_create_lib_start_database_transaction(&lun_create_request_p->transaction_id, job_queue_element_p->job_number);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! @todo: call configuration_service to persist on peer */
    /* call configuration_service to create_lun */
    status = fbe_create_extent_pool_lun_create_lun(lun_create_request_p);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_create_extent_pool_lun_update_configuration_in_memory_function()
 **********************************************************/

/*!**************************************************************
 * fbe_create_extent_pool_lun_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Lun create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - lun create request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_create_extent_pool_lun_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_ext_pool_lun_t        *lun_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;


    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
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
    lun_create_request_p = (fbe_job_service_create_ext_pool_lun_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(lun_create_request_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_create_extent_pool_lun_persist_configuration_db_function()
 **********************************************************/

/*!**************************************************************
 * fbe_create_extent_pool_lun_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Lun create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - lun create request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_create_extent_pool_lun_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_ext_pool_lun_t        *lun_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
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
    lun_create_request_p = (fbe_job_service_create_ext_pool_lun_t *) job_queue_element_p->command_data;

    /* see if transaction should be aborted */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY) &&
            (lun_create_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID))
    {
        status = fbe_create_lib_abort_database_transaction(lun_create_request_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
    }

    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    job_queue_element_p->object_id = FBE_OBJECT_ID_INVALID;
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
                __FUNCTION__, 
                status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_create_extent_pool_lun_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_create_extent_pool_lun_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the Lun create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - lun create request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_create_extent_pool_lun_commit_function (fbe_packet_t *packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_job_service_create_ext_pool_lun_t        *lun_create_request_p = NULL;
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
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
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
    lun_create_request_p = (fbe_job_service_create_ext_pool_lun_t *) job_queue_element_p->command_data;

	job_service_trace(FBE_TRACE_LEVEL_INFO, 
		FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
		"%s entry, job number 0x%llx, status 0x%x, error_code %d\n",
		__FUNCTION__,
		(unsigned long long)job_queue_element_p->job_number, 
		job_queue_element_p->status,
		job_queue_element_p->error_code);

	job_queue_element_p->status = FBE_STATUS_OK;
	//job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = lun_create_request_p->object_id;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = lun_create_request_p->object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    //notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_LUN_CREATE;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(1) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(lun_create_request_p->object_id, 
                                                                  FBE_LIFECYCLE_STATE_READY, 
                                                                  30000);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting lun object id %d be ready. status %d\n", 
                              __FUNCTION__, lun_create_request_p->object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    status = fbe_notification_send(lun_create_request_p->object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, 
                status);
    }
  
    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);
    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_create_extent_pool_lun_commit_function()
 **********************************************************/


/*
 * The following functions are utility functions used by the LUN create Job Service code
 */

/*!**************************************************************
 * fbe_create_extent_pool_lun_create_lun()
 ****************************************************************
 * @brief
 * This function asks configuration service to create a lun object.  
 *
 * @param create_lun_request_p - Pointer to the request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2010 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_create_extent_pool_lun_create_lun(fbe_job_service_create_ext_pool_lun_t * create_lun_request_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_create_ext_pool_lun_t create_lun;

    fbe_zero_memory(&create_lun, sizeof(fbe_database_control_create_ext_pool_lun_t));

    create_lun.pool_id                                = create_lun_request_p->pool_id;
    create_lun.lun_id                                 = create_lun_request_p->lun_id;
    create_lun.capacity                               = create_lun_request_p->capacity;
    create_lun.object_id                              = FBE_OBJECT_ID_INVALID;
    create_lun.class_id                               = FBE_CLASS_ID_EXTENT_POOL_LUN;
    create_lun.transaction_id                         = create_lun_request_p->transaction_id;
    create_lun.world_wide_name                        = create_lun_request_p->world_wide_name;
    create_lun.user_defined_name                      = create_lun_request_p->user_defined_name;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL_LUN,
                                                 &create_lun,
												 sizeof(fbe_database_control_create_ext_pool_lun_t),
												 FBE_PACKAGE_ID_SEP_0,                                             
												 FBE_SERVICE_ID_DATABASE,
												 FBE_CLASS_ID_INVALID,
												 FBE_OBJECT_ID_INVALID,
												 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: failed.  Error 0x%x\n", 
                          __FUNCTION__, 
                          status);
        return status;
    }

    create_lun_request_p->object_id = create_lun.object_id;
    
    return status;
}
/**********************************************************
 * end fbe_create_extent_pool_lun_create_lun()
 **********************************************************/
