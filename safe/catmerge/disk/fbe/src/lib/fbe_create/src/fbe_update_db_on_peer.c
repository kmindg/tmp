/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_update_db_on_peer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for enabling the database to update the configuration on the peer
 * 
 * @ingroup job_lib_files
 * 
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
#include "fbe_cmi.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_db_on_peer_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_db_on_peer_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_db_on_peer_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_db_on_peer_persist_function(fbe_packet_t *packet_p);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_db_on_peer_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_DB_ON_PEER,
    {
        /* validation function */
        fbe_update_db_on_peer_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_db_on_peer_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_db_on_peer_persist_function,

        /* response/rollback function */
        fbe_update_db_on_peer_rollback_function,

        /* commit function */
        NULL,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_update_db_on_peer_validation_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
	fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
	fbe_job_service_update_db_on_peer_t *				update_on_peer_p = NULL;

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

    update_on_peer_p = (fbe_job_service_update_db_on_peer_t *) job_queue_element_p->command_data;

	/* fix AR 698996. make sure peer SP is alive when update db to peer */
    if (!fbe_cmi_is_peer_alive()) 
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: peer SP is not alive\n",
                          __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_update_db_on_peer_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_db_on_peer_t *				update_on_peer_p = NULL;
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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    update_on_peer_p = (fbe_job_service_update_db_on_peer_t *) job_queue_element_p->command_data;

	/* fix AR 698996. make sure peer SP is alive when update db to peer */
    if (!fbe_cmi_is_peer_alive()) 
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: peer SP is not alive\n",
                          __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*all we do here is give job CPU context and protection against other jobs to the DB service
	to be able to execute it peer update*/
	status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_CONFIG,
                                                 NULL,
                                                 0,
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    
    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_update_db_on_peer_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

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

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_update_db_on_peer_persist_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_notification_info_t             				notification_info;
	fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
	fbe_job_service_update_db_on_peer_t *				update_on_peer_p = NULL;

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

	 /* get the user data from job command*/
    update_on_peer_p = (fbe_job_service_update_db_on_peer_t *) job_queue_element_p->command_data;

	/* Send a notification for letting users know we are done*/
	notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
	notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
	notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
	notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
	notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;

	notification_info.class_id = FBE_CLASS_ID_INVALID;
	notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

	status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
	if (status != FBE_STATUS_OK) {
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s: job status notification failed, status: %d\n",
						  __FUNCTION__, status);
	}
		
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

}


