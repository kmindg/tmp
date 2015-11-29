/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_set_system_power_save_info.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for changing the system power save information
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   04/28/2010:  Created. sharel
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
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe_database.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_set_system_ps_info_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_ps_info_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_ps_info_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_ps_info_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_set_system_ps_info_database_service_set_info(fbe_database_transaction_id_t transaction_id, 
                                                                          fbe_system_power_saving_info_t *power_save_info);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_set_system_ps_info_job_service_operation = 
{
    FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO,
    {
        /* validation function */
        fbe_set_system_ps_info_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_set_system_ps_info_update_configuration_in_memory_function,

        /* persist function */
        fbe_set_system_ps_info_persist_function,

        /* response/rollback function */
        fbe_set_system_ps_info_rollback_function,

        /* commit function */
        NULL,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_set_system_ps_info_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_service_change_system_power_saving_info_t * change_system_ps_request_p = NULL;
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

    change_system_ps_request_p = (fbe_job_service_change_system_power_saving_info_t *) job_queue_element_p->command_data;

	if (change_system_ps_request_p->system_power_save_info.hibernation_wake_up_time_in_minutes == 0) {
		change_system_ps_request_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

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
	change_system_ps_request_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_set_system_ps_info_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_change_system_power_saving_info_t * change_system_ps_request_p = NULL;
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
    change_system_ps_request_p = (fbe_job_service_change_system_power_saving_info_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_start_database_transaction(&change_system_ps_request_p->transaction_id, job_queue_element_p->job_number);
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
    status = fbe_set_system_ps_info_database_service_set_info(change_system_ps_request_p->transaction_id, 
                                                                   &change_system_ps_request_p->system_power_save_info);
	if (status != FBE_STATUS_OK) {
		change_system_ps_request_p->error_code = FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED;
	}

    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
	change_system_ps_request_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_set_system_ps_info_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_change_system_power_saving_info_t * change_system_ps_request_p = NULL;
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
    change_system_ps_request_p = (fbe_job_service_change_system_power_saving_info_t *) job_queue_element_p->command_data;
	
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
}

static fbe_status_t fbe_set_system_ps_info_database_service_set_info(fbe_database_transaction_id_t transaction_id, 
                                                                          fbe_system_power_saving_info_t *power_save_info)
{
    fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_power_save_t	database_ps_info;

    database_ps_info.transaction_id = transaction_id;
    fbe_copy_memory(&database_ps_info.system_power_save_info, power_save_info, sizeof(fbe_system_power_saving_info_t));
    
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_SET_POWER_SAVE,
                                                 &database_ps_info,
                                                 sizeof(fbe_database_power_save_t),
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


static fbe_status_t fbe_set_system_ps_info_persist_function(fbe_packet_t *packet_p)
{
	 fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_change_system_power_saving_info_t * change_system_ps_request_p = NULL;
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
    change_system_ps_request_p = (fbe_job_service_change_system_power_saving_info_t *) job_queue_element_p->command_data;

    /*update the config service which will persist it*/
	status = fbe_create_lib_commit_database_transaction(change_system_ps_request_p->transaction_id);

	if (status == FBE_STATUS_OK) {
		/* Send a notification for this commit*/
		notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
		notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
		notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
		notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
		notification_info.notification_data.job_service_error_info.error_code = change_system_ps_request_p->error_code;

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
		change_system_ps_request_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
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

