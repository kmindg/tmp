/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_connect.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for connecting PVD to downstream LDO used by job service.
 * 
 * @ingroup job_lib_files
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
#include "fbe_database.h"
#include "fbe_private_space_layout.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

/* drive connect library request validation function.
 */
static fbe_status_t fbe_drive_connect_validation_function (fbe_packet_t *packet_p);
/* drive connect library update configuration in memory function.
 */
static fbe_status_t fbe_drive_connect_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* drive connect library persist configuration in db function.
 */
static fbe_status_t fbe_drive_connect_persist_configuration_db_function (fbe_packet_t *packet_p);

/* drive connect library rollback function.
 */
static fbe_status_t fbe_drive_connect_rollback_function (fbe_packet_t *packet_p);

/* drive connect library commit function.
 */
static fbe_status_t fbe_drive_connect_commit_function (fbe_packet_t *packet_p);


/*************************
 *   DEFINITIONS
 *************************/

/* Job service drive connect registration.
*/
fbe_job_service_operation_t fbe_drive_connect_job_service_operation = 
{
    FBE_JOB_TYPE_CONNECT_DRIVE,
    {
        /* validation function */
        fbe_drive_connect_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_drive_connect_update_configuration_in_memory_function,

        /* persist function */
        fbe_drive_connect_persist_configuration_db_function,

        /* response/rollback function */
        fbe_drive_connect_rollback_function,

        /* commit function */
        fbe_drive_connect_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_drive_connect_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the drive connect request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 ****************************************************************/

static fbe_status_t fbe_drive_connect_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s: entry.\n", 
                      __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: job_queue_element_p is NULL.\n", 
                          __FUNCTION__);
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
                          "%s: fbe_job_queue_element wrong size.\n", 
                          __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*nothing needs to be validated*/

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_drive_connect_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_drive_connect_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the drive connect request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_connect_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_connect_drive_t        *drive_connect_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_database_control_drive_connection_t database_drive_connection;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s: entry.\n", 
                      __FUNCTION__);

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
    drive_connect_request_p = (fbe_job_service_connect_drive_t *) job_queue_element_p->command_data;

    /* no need to open database transaction here because we would not change db entries in job thread*/
    database_drive_connection.request_size = drive_connect_request_p->request_size;
    fbe_copy_memory(database_drive_connection.PVD_list, drive_connect_request_p->PVD_list, drive_connect_request_p->request_size*sizeof(fbe_object_id_t));

    status = fbe_create_lib_send_control_packet (
                              FBE_DATABASE_CONTROL_CODE_CONNECT_DRIVES,
                              &database_drive_connection,
                              sizeof(database_drive_connection),
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID,
                              FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s connect drives in database failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }    
   
    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR; 
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_drive_connect_update_configuration_in_memory_function()
 **********************************************************/

/*!**************************************************************
 * fbe_drive_connect_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the drive connect request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - drive connect request.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_connect_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s: entry.\n", 
                      __FUNCTION__);

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

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR; 

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_drive_connect_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_drive_connect_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the drive connect request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - drive connect request.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_connect_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s: entry.\n", 
                      __FUNCTION__);

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
    notification_info.notification_data.job_service_error_info.status = job_queue_element_p->status;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_CONNECT_DRIVE;
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
 * end fbe_drive_connect_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_drive_connect_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the drive connect request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - drive connect request.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_connect_commit_function (fbe_packet_t *packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_control_operation_t     *control_operation_p = NULL;    
    fbe_job_service_connect_drive_t        *drive_connect_request_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s: entry.\n", 
                      __FUNCTION__);
    
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
    drive_connect_request_p = (fbe_job_service_connect_drive_t *) job_queue_element_p->command_data;

    /* send the notification*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = job_queue_element_p->status;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_CONNECT_DRIVE;
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
 * end fbe_drive_connect_commit_function()
 **********************************************************/
