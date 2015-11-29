/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_update_virtual_drive.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines to update the virtual drive configuration 
 *  using job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   06/01/2010:  Created. Dhaval Patel
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
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_base_config.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_virtual_drive_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_virtual_drive_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_virtual_drive_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_virtual_drive_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_virtual_drive_commit_function(fbe_packet_t *packet_p);

fbe_status_t fbe_update_virtual_drive_configuration(fbe_database_transaction_id_t transaction_id, 
                                                    fbe_object_id_t vd_object_id,
                                                    fbe_virtual_drive_configuration_mode_t configuration_mode,
                                                    fbe_update_vd_type_t update_type);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_virtual_drive_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE,
    {
        /* validation function */
        fbe_update_virtual_drive_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_virtual_drive_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_virtual_drive_persist_function,

        /* response/rollback function */
        fbe_update_virtual_drive_rollback_function,

        /* commit function */
        fbe_update_virtual_drive_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 * fbe_update_virtual_drive_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the update virtual drive configuration 
 *  job service command.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/01/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_virtual_drive_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_service_update_virtual_drive_t *    update_virtual_drive_p = NULL;
    fbe_payload_ex_t                       *       payload_p = NULL;
    fbe_payload_control_operation_t     *       control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t             *       job_queue_element_p = NULL;
    
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

    update_virtual_drive_p = (fbe_job_service_update_virtual_drive_t *) job_queue_element_p->command_data;

    if (update_virtual_drive_p->configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: configuration mode cannot be unknown\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_virtual_drive_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_virtual_drive_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 *  This function is used to update the virtual drive configuration changes in
 *  memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/01/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_virtual_drive_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_virtual_drive_t *    update_virtual_drive_p = NULL;
    fbe_payload_ex_t                       *       payload_p = NULL;
    fbe_payload_control_operation_t     *       control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t             *       job_queue_element_p = NULL;

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
    update_virtual_drive_p = (fbe_job_service_update_virtual_drive_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_start_database_transaction(&update_virtual_drive_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start virtual drive update transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_update_virtual_drive_configuration(update_virtual_drive_p->transaction_id,
                                                    update_virtual_drive_p->object_id,
                                                    update_virtual_drive_p->configuration_mode,
                                                    update_virtual_drive_p->update_type);
    
    fbe_payload_control_set_status(control_operation_p,
                                   status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_update_virtual_drive_update_configuration_in_memory_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_virtual_drive_rollback_function()
 ******************************************************************************
 * @brief
 *  This function is used to rollback to configuration update for the virtual
 *  drive object.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/01/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_virtual_drive_rollback_function(fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;    
    fbe_job_service_update_virtual_drive_t *    update_virtual_drive_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_notification_info_t                     notification_info;
    fbe_status_t                                status = FBE_STATUS_OK;

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
    update_virtual_drive_p = (fbe_job_service_update_virtual_drive_t *)job_queue_element_p->command_data;
    if (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_VALIDATE){
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s entry, rollback not required, previous state %d, current state %d\n", __FUNCTION__, 
                job_queue_element_p->previous_state,
                job_queue_element_p->current_state);
    }

    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)){
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: aborting transaction id %d\n",
                __FUNCTION__, (int)update_virtual_drive_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(update_virtual_drive_p->transaction_id);
        if(status != FBE_STATUS_OK){
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Could not abort transaction id %llu with configuration service\n", 
                    __FUNCTION__,
		    (unsigned long long)update_virtual_drive_p->transaction_id);

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
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE;
    notification_info.notification_data.job_service_error_info.object_id = update_virtual_drive_p->object_id;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    

    status = fbe_notification_send(update_virtual_drive_p->object_id, notification_info);
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

    fbe_payload_control_set_status(control_operation,
                                   status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
    
}
/******************************************************************************
 * end fbe_update_virtual_drive_rollback_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_virtual_drive_persist_function()
 ******************************************************************************
 * @brief
 *  This function is used to persist the updated virtual drive configuration
 *  in configuration database.
 * object.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/01/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_virtual_drive_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_virtual_drive_t *    update_virtual_drive_p = NULL;
    fbe_payload_ex_t                       *       payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    
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
    update_virtual_drive_p = (fbe_job_service_update_virtual_drive_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(update_virtual_drive_p->transaction_id);
    
    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p,
                                   status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/******************************************************************************
 * end fbe_update_virtual_drive_persist_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_virtual_drive_commit_function()
 ******************************************************************************
 * @brief
 *  This function is used to commit the configuration update to the virtual
 *  drive object. It sends notification to the notification service that job
 *  is completed.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/01/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_virtual_drive_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_virtual_drive_t *    update_virtual_drive_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_notification_info_t                     notification_info;
    
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
    update_virtual_drive_p = (fbe_job_service_update_virtual_drive_t *) job_queue_element_p->command_data;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE;
    notification_info.notification_data.job_service_error_info.object_id = update_virtual_drive_p->object_id;

    status = fbe_notification_send(update_virtual_drive_p->object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: status change notification failed, status %d\n",
                __FUNCTION__, status);
        
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    job_queue_element_p->object_id = update_virtual_drive_p->object_id;

    fbe_payload_control_set_status(control_operation_p,
                                   status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_update_virtual_drive_commit_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_virtual_drive_configuration()
 ******************************************************************************
 * @brief
 * This function is used to update the configuration of the virtual drive
 * object.
 *
 * @param transaction_id          - Transaction id.
 * @param virtual_drive_object    - Virtual drive object.
 * @param configuration_mode      - Configuration mode which we want to update.
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  04/27/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_update_virtual_drive_configuration(fbe_database_transaction_id_t transaction_id, 
                                       fbe_object_id_t vd_object_id,
                                       fbe_virtual_drive_configuration_mode_t configuration_mode,
                                       fbe_update_vd_type_t update_type)
{
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_vd_t    update_virtual_drive;

    /* Update the transaction details to update the virtual drive configuration. */
    update_virtual_drive.transaction_id = transaction_id;
    update_virtual_drive.object_id = vd_object_id;
    update_virtual_drive.update_type = update_type;
    update_virtual_drive.configuration_mode = configuration_mode;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: vd obj: 0x%x update_type: %d config_mode: %d\n",
                           __FUNCTION__, vd_object_id, update_type, configuration_mode);

    /* Send the update virtual drive configuration to configuration service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_VD,
                                                &update_virtual_drive,
                                                sizeof(fbe_database_control_update_vd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE failed\n", __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_update_virtual_drive_configuration()
 ******************************************************************************/

