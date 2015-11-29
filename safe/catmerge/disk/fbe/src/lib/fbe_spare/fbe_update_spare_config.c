/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_set_system_system_spare_config_info.c
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
#include "fbe_spare.h"
#include "fbe_spare_lib_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_base_config.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_spare_config_validation_function(fbe_packet_t * packet_p);
static fbe_status_t fbe_update_spare_config_update_configuration_in_memory_function(fbe_packet_t * packet_p);
static fbe_status_t fbe_update_spare_config_rollback_function (fbe_packet_t * packet_p);
static fbe_status_t fbe_update_spare_config_persist_function(fbe_packet_t * packet_p);
static fbe_status_t fbe_update_spare_configuration(fbe_database_transaction_id_t transaction_id,
                                                   fbe_system_spare_config_info_t * system_spare_config_info,
                                                   fbe_database_update_spare_config_type_t update_type);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_spare_config_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_SPARE_CONFIG,
    {
        /* validation function */
        fbe_update_spare_config_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_spare_config_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_spare_config_persist_function,

        /* response/rollback function */
        fbe_update_spare_config_rollback_function,

        /* commit function */
        NULL,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_update_spare_config_validation_function(fbe_packet_t * packet_p)
{
    fbe_job_service_update_spare_config_t *		update_spare_config_p = NULL;
    fbe_payload_ex_t                       *		payload_p = NULL;
    fbe_payload_control_operation_t     *		control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_job_queue_element_t             *		job_queue_element_p = NULL;


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
    if (length != sizeof(fbe_job_queue_element_t))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid job element size\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the update spare configiration request.
     */
    update_spare_config_p = (fbe_job_service_update_spare_config_t *) job_queue_element_p->command_data;

    /* Validate that the command type is supported.
     */
    if (update_spare_config_p->update_type != FBE_DATABASE_UPDATE_PERMANENT_SPARE_TIMER)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s invalid update type: %d \n", __FUNCTION__,update_spare_config_p->update_type);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the spare time value (0 is not supported)
     */
    if (update_spare_config_p->system_spare_info.permanent_spare_trigger_time == 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s invalid spare trigger timer: 0x%llx\n", __FUNCTION__, (unsigned long long)update_spare_config_p->system_spare_info.permanent_spare_trigger_time);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_update_spare_config_update_configuration_in_memory_function(fbe_packet_t * packet_p)
{
    fbe_status_t                        		status = FBE_STATUS_OK;
    fbe_job_service_update_spare_config_t *     update_spare_config_p = NULL;
    fbe_payload_ex_t                       *		payload_p = NULL;
    fbe_payload_control_operation_t     *		control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_job_queue_element_t             *		job_queue_element_p = NULL;


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
    update_spare_config_p = (fbe_job_service_update_spare_config_t *) job_queue_element_p->command_data;

    status =  fbe_spare_lib_utils_start_database_transaction(&update_spare_config_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s failed to start set power save transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*store it in the DATABASE which will also updte memory and both SPs*/
    status = fbe_update_spare_configuration(update_spare_config_p->transaction_id,
                                            &update_spare_config_p->system_spare_info,
                                            update_spare_config_p->update_type);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s failed to update the spare DATABASE, status:%d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    /* set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_update_spare_config_rollback_function(fbe_packet_t * packet_p)
{
    fbe_status_t                        			status = FBE_STATUS_OK;
    fbe_job_service_update_spare_config_t *         update_spare_config_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *			control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 			length = 0;
    fbe_job_queue_element_t             *			job_queue_element_p = NULL;
    fbe_notification_info_t             	        notification_info;

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
    update_spare_config_p = (fbe_job_service_update_spare_config_t *) job_queue_element_p->command_data;
    
    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_GENERIC_FAILURE;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: job status notification failed, action state: %d\n", __FUNCTION__, status);
    }


    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_update_spare_configuration(fbe_database_transaction_id_t transaction_id,
                                                   fbe_system_spare_config_info_t * system_spare_config_info,
                                                   fbe_database_update_spare_config_type_t update_type)
{
    fbe_status_t 					                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_system_spare_config_t   update_spare_configuration;

    update_spare_configuration.transaction_id = transaction_id;
    update_spare_configuration.update_type = update_type;
    fbe_copy_memory(&update_spare_configuration.system_spare_info, system_spare_config_info, sizeof(fbe_system_spare_config_info_t));
    
    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_SYSTEM_SPARE_CONFIG,
                                                     &update_spare_configuration,
                                                     sizeof(fbe_database_control_update_system_spare_config_t),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_SPARE_CONFIG failed\n", __FUNCTION__);
    }
    return status;
}


static fbe_status_t fbe_update_spare_config_persist_function(fbe_packet_t *packet_p)
{
     fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_spare_config_t * update_spare_config_p = NULL;
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
    update_spare_config_p = (fbe_job_service_update_spare_config_t *) job_queue_element_p->command_data;

    /*update the config service which will persist it*/
    status = fbe_spare_lib_utils_commit_database_transaction(update_spare_config_p->transaction_id);

    if(status == FBE_STATUS_OK)
    {
        /* Send a notification for this commit*/
        notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
        notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
        notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
        notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
        notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;

        notification_info.class_id = FBE_CLASS_ID_INVALID;
        notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
        status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: job status notification failed, status: %d\n",
                                   __FUNCTION__, status);
        }
        
    }

    job_queue_element_p->status = status;
    job_queue_element_p->error_code = (status == FBE_STATUS_OK) ? 
                                                  FBE_JOB_SERVICE_ERROR_NO_ERROR : FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    fbe_payload_control_set_status(control_operation_p,
                                   status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

