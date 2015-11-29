/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_set_system_pvd_removed_time_threshold.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for setting the system time threshold for pvd garbage collection
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   05/31/2012:  Created. zhangy
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

static fbe_status_t fbe_set_system_time_threshold_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_time_threshold_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_time_threshold_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_time_threshold_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_time_threshold_database_service_set_time_threshold(fbe_database_transaction_id_t transaction_id, 
                                                                          fbe_system_time_threshold_info_t *time_threshold_info);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_set_system_time_threshold_job_service_operation = 
{
    FBE_JOB_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO,
    {
        /* validation function */
        fbe_set_system_time_threshold_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_set_system_time_threshold_update_configuration_in_memory_function,

        /* persist function */
        fbe_set_system_time_threshold_persist_function,

        /* response/rollback function */
        fbe_set_system_time_threshold_rollback_function,

        /* commit function */
        NULL,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 * fbe_set_system_time_threshold_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the pvd removed time threshold setting request
 *  job service command. It checks the request. If the request is NULL, we finished the
 *  job with error
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/06/2012 - Created. Yang Zhang
 ******************************************************************************/

static fbe_status_t fbe_set_system_time_threshold_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_service_set_system_time_threshold_info_t * set_system_timethreshold_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;


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

    set_system_timethreshold_request_p = (fbe_job_service_set_system_time_threshold_info_t *) job_queue_element_p->command_data;

	if (set_system_timethreshold_request_p == NULL) {
		set_system_timethreshold_request_p->error_code = FBE_JOB_SERVICE_ERROR_NULL_COMMAND;

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NULL_COMMAND;

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
	set_system_timethreshold_request_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_set_system_time_threshold_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 *  This function is used to update the pvd removed time threshold in database global entry in memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/06/2012 - Created. Yang Zhang
 ******************************************************************************/
static fbe_status_t fbe_set_system_time_threshold_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_set_system_time_threshold_info_t *  set_system_timethreshold_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;


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
    set_system_timethreshold_request_p = (fbe_job_service_set_system_time_threshold_info_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_start_database_transaction(&set_system_timethreshold_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to start set system time threshold transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*store it in the configuration which will also updte memory and both SPs*/
    status = fbe_set_system_time_threshold_database_service_set_time_threshold(set_system_timethreshold_request_p->transaction_id, 
                                                                   &set_system_timethreshold_request_p->system_time_threshold_info);
	if (status != FBE_STATUS_OK) {
		set_system_timethreshold_request_p->error_code = FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED;
	}

    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
	set_system_timethreshold_request_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

/*!****************************************************************************
 * fbe_set_system_time_threshold_rollback_function()
 ******************************************************************************
 * @brief
 *  This function is used to rollback to set pvd remved time threshold
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/06/2012 - Created. Yang Zhang
 ******************************************************************************/
static fbe_status_t fbe_set_system_time_threshold_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_set_system_time_threshold_info_t *  set_system_timethreshold_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_payload_control_status_t        				payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             				notification_info;

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
    set_system_timethreshold_request_p = (fbe_job_service_set_system_time_threshold_info_t *) job_queue_element_p->command_data;
	
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)
            && (set_system_timethreshold_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID))
    {
        status = fbe_create_lib_abort_database_transaction(set_system_timethreshold_request_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
    }
    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_GENERIC_FAILURE;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
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
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*!****************************************************************************
 * fbe_set_system_time_threshold_database_service_set_time_threshold()
 ******************************************************************************
 * @brief
 *  This function is used to send control packet to ask database service to update
 *   pvd removed time threshold in global entry in memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/06/2012 - Created. Yang Zhang
 ******************************************************************************/
static fbe_status_t fbe_set_system_time_threshold_database_service_set_time_threshold(fbe_database_transaction_id_t transaction_id, 
                                                                          fbe_system_time_threshold_info_t *time_threshold_info)
{
    fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_time_threshold_t	database_time_threshold_info;

    database_time_threshold_info.transaction_id = transaction_id;
    fbe_copy_memory(&database_time_threshold_info.system_time_threshold_info, time_threshold_info, sizeof(fbe_system_time_threshold_info_t));
    
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_SET_PVD_DESTROY_TIMETHRESHOLD,
                                                 &database_time_threshold_info,
                                                 sizeof(fbe_database_time_threshold_t),
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
}

/*!****************************************************************************
 * fbe_set_system_time_threshold_persist_function()
 ******************************************************************************
 * @brief
 *  This function is used to persist the pvd removed time threshold
 *  in configuration database and send notification for this job.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/06/2012 - Created. Yang Zhang
 ******************************************************************************/
static fbe_status_t fbe_set_system_time_threshold_persist_function(fbe_packet_t *packet_p)
{
	 fbe_status_t                        				status = FBE_STATUS_OK;
	 fbe_job_service_set_system_time_threshold_info_t *  set_system_timethreshold_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
	fbe_notification_info_t             				notification_info;

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
    set_system_timethreshold_request_p = (fbe_job_service_set_system_time_threshold_info_t *) job_queue_element_p->command_data;

    /*update the config service which will persist it*/
	status = fbe_create_lib_commit_database_transaction(set_system_timethreshold_request_p->transaction_id);

	if (status == FBE_STATUS_OK) {
		/* Send a notification for this commit*/
		notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
		notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
		notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
		notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
		notification_info.notification_data.job_service_error_info.error_code = set_system_timethreshold_request_p->error_code;

		notification_info.class_id = FBE_CLASS_ID_INVALID;
		notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
	
		status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
		if (status != FBE_STATUS_OK) {
			job_service_trace(FBE_TRACE_LEVEL_ERROR,
							  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							  "%s: job status notification failed, status: %d\n",
							  __FUNCTION__, status);
		}
		
	}else{
		set_system_timethreshold_request_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
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

}



