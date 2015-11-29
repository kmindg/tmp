
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_control_system_bg_service.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for controlling the system background service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   02/29/2012:  Created. Vera Wang
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

static fbe_status_t fbe_control_system_bg_service_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_control_system_bg_service_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_control_system_bg_service_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_control_system_bg_service_commit_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_control_system_bg_service_database_service_update_peer_bgs(fbe_base_config_control_system_bg_service_t *system_bg_service);
static fbe_status_t fbe_control_system_bg_service_set_system_bg_service(fbe_base_config_control_system_bg_service_t *system_bg_service);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_control_system_bg_service_job_service_operation = 
{
    FBE_JOB_TYPE_CONTROL_SYSTEM_BG_SERVICE,
    {
        /* validation function */
        fbe_control_system_bg_service_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_control_system_bg_service_update_configuration_in_memory_function,

        /* persist function */
        NULL,

        /* response/rollback function */
        fbe_control_system_bg_service_rollback_function,

        /* commit function */
       fbe_control_system_bg_service_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_control_system_bg_service_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_service_control_system_bg_service_t         *control_system_bg_service_req_p = NULL;
    fbe_payload_ex_t                                    *payload_p = NULL;
    fbe_payload_control_operation_t                     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t                             *job_queue_element_p = NULL;


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

    control_system_bg_service_req_p = (fbe_job_service_control_system_bg_service_t *) job_queue_element_p->command_data;

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
	control_system_bg_service_req_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_control_system_bg_service_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_control_system_bg_service_t         *control_system_bg_service_req_p = NULL;
    fbe_payload_ex_t                                    *payload_p = NULL;
    fbe_payload_control_operation_t                     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 			    length = 0;
    fbe_job_queue_element_t                             *job_queue_element_p = NULL;


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
    control_system_bg_service_req_p = (fbe_job_service_control_system_bg_service_t *) job_queue_element_p->command_data;

    /* set control system_bg_service for basic_config object. */
    status = fbe_control_system_bg_service_set_system_bg_service(&control_system_bg_service_req_p->system_bg_service);
    if (status != FBE_STATUS_OK) {
		control_system_bg_service_req_p->error_code = FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED;
	}

    /*update control_system_bg_service in the peer sp.*/
    status = fbe_control_system_bg_service_database_service_update_peer_bgs(&control_system_bg_service_req_p->system_bg_service);
	if (status != FBE_STATUS_OK) {
		control_system_bg_service_req_p->error_code = FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED;
    }

    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
	control_system_bg_service_req_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_control_system_bg_service_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_control_system_bg_service_t         *control_system_bg_service_req_p = NULL;
    fbe_payload_ex_t                                    *payload_p = NULL;
    fbe_payload_control_operation_t                     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t                             *job_queue_element_p = NULL;
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
    control_system_bg_service_req_p = (fbe_job_service_control_system_bg_service_t *) job_queue_element_p->command_data;
	
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


static fbe_status_t fbe_control_system_bg_service_commit_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        	        status = FBE_STATUS_OK;
    fbe_job_service_control_system_bg_service_t     *control_system_bg_service_req_p = NULL;
    fbe_payload_ex_t                                *payload_p = NULL;
    fbe_payload_control_operation_t                 *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 	        length = 0;
    fbe_job_queue_element_t                         *job_queue_element_p = NULL;
	fbe_notification_info_t              	        notification_info;
    
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
    control_system_bg_service_req_p = (fbe_job_service_control_system_bg_service_t *) job_queue_element_p->command_data;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
	notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;

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
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
	return FBE_STATUS_OK;

}

static fbe_status_t fbe_control_system_bg_service_database_service_update_peer_bgs(fbe_base_config_control_system_bg_service_t *system_bg_service)
{
    fbe_status_t 					                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_peer_system_bg_service_t    db_update_peer_system_bgs;

    fbe_copy_memory(&db_update_peer_system_bgs.system_bg_service, system_bg_service, sizeof(fbe_base_config_control_system_bg_service_t));
    
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_SYSTEM_BG_SERVICE_FLAG,
                                                 &db_update_peer_system_bgs,
                                                 sizeof(fbe_database_control_update_peer_system_bg_service_t),
												 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
												 FBE_CLASS_ID_INVALID,
												 FBE_OBJECT_ID_INVALID,
												 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
        return status;
    }

    return status;
}

static fbe_status_t fbe_control_system_bg_service_set_system_bg_service(fbe_base_config_control_system_bg_service_t *system_bg_service){
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* update the control system background service */
    status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
                                                system_bg_service, 
                                                sizeof(fbe_base_config_control_system_bg_service_t),                                                 
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_BASE_CONFIG,
                                                FBE_OBJECT_ID_INVALID,
												FBE_PACKET_FLAG_NO_ATTRIB);

    if ( status != FBE_STATUS_OK )
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);      
    }

    return status;
}


