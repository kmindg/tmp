/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_create_extent_pool.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the job for creating an extent pool.
 *
 * @version
 *   06/06/2014:  Created. Lili Chen
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
#include "fbe_private_space_layout.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_create_extent_pool_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_create_extent_pool_selection_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_create_extent_pool_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_create_extent_pool_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_create_extent_pool_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_create_extent_pool_commit_function(fbe_packet_t *packet_p);

static fbe_status_t fbe_create_extent_pool_get_pvd_object_list(fbe_job_service_create_extent_pool_t * create_extent_pool_request_p);
static fbe_status_t fbe_create_extent_pool_create_extent_pool(fbe_job_service_create_extent_pool_t * create_extent_pool_request_p);
static fbe_status_t fbe_create_extent_pool_create_extent_pool_metadata_lun(fbe_job_service_create_extent_pool_t * create_extent_pool_request_p);


/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_create_extent_pool_job_service_operation = 
{
    FBE_JOB_TYPE_CREATE_EXTENT_POOL,
    {
        /* validation function */
        fbe_create_extent_pool_validation_function,

        /* selection function */
        fbe_create_extent_pool_selection_function,

        /* update in memory function */
        fbe_create_extent_pool_update_configuration_in_memory_function,

        /* persist function */
        fbe_create_extent_pool_persist_function,

        /* response/rollback function */
        fbe_create_extent_pool_rollback_function,

        /* commit function */
        fbe_create_extent_pool_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_create_extent_pool_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_service_create_extent_pool_t * 				create_extent_pool_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    //fbe_status_t                                        status;
    
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

    create_extent_pool_request_p = (fbe_job_service_create_extent_pool_t *) job_queue_element_p->command_data;
    if(create_extent_pool_request_p == NULL){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that pool id doesn't exist */
    /* Validate the system limit doesn't exceeded */

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_create_extent_pool_selection_function(fbe_packet_t *packet_p)
{
    fbe_job_service_create_extent_pool_t * 				create_extent_pool_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_status_t                                        status;
    
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

    create_extent_pool_request_p = (fbe_job_service_create_extent_pool_t *) job_queue_element_p->command_data;
    if(create_extent_pool_request_p == NULL){

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the pvd object list */
    status = fbe_create_extent_pool_get_pvd_object_list(create_extent_pool_request_p);
    if (status != FBE_STATUS_OK){
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

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
}

static fbe_status_t fbe_create_extent_pool_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_create_extent_pool_t * 				create_extent_pool_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_u32_t                                           pvd_index;
    fbe_object_id_t *                                   pvd_object_id;
    fbe_u32_t                                           pool_id;

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
    create_extent_pool_request_p = (fbe_job_service_create_extent_pool_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_start_database_transaction(&create_extent_pool_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start transaction %d\n",
                __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        if(create_extent_pool_request_p->job_callback != NULL){
            create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Update PVD configuration. */
    pvd_object_id = (fbe_object_id_t *)(create_extent_pool_request_p + 1);
    for (pvd_index = 0; pvd_index < create_extent_pool_request_p->drive_count;  pvd_index++) {
        /* update the configuration type of the pvd object. */
        status = fbe_create_lib_database_service_update_pvd_config_type(create_extent_pool_request_p->transaction_id, 
                                                                        *pvd_object_id,
                                                                        FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL);
        if (status != FBE_STATUS_OK) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to update config type pvd 0x%x status %d\n",
                __FUNCTION__, *pvd_object_id, status);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            if(create_extent_pool_request_p->job_callback != NULL){
                create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            }

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "     updated config type to EXT_POOL for PVD[%d] object.\n",
                *pvd_object_id);

        pool_id = create_extent_pool_request_p->pool_id | pvd_index << 16;
        /* update the pool of the pvd object. */
        status = fbe_create_lib_database_service_update_pvd_pool_id(create_extent_pool_request_p->transaction_id, 
                                                                    *pvd_object_id,
                                                                    pool_id);
        if (status != FBE_STATUS_OK) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to update pool id pvd 0x%x status %d\n",
                __FUNCTION__, *pvd_object_id, status);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            if(create_extent_pool_request_p->job_callback != NULL){
                create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            }

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "     updated pool_id to %d for PVD[%d] object.\n",
                pool_id, *pvd_object_id);

        pvd_object_id++;
    }

    status = fbe_create_extent_pool_create_extent_pool(create_extent_pool_request_p);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to create extent pool status %d\n",
                __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

       if(create_extent_pool_request_p->job_callback != NULL){
            create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       }

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_extent_pool_create_extent_pool_metadata_lun(create_extent_pool_request_p);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to create extent pool status %d\n",
                __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

       if(create_extent_pool_request_p->job_callback != NULL){
            create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
       }

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_create_extent_pool_rollback_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                     *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    fbe_job_service_create_extent_pool_t * create_extent_pool_request_p = NULL;
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
    create_extent_pool_request_p = (fbe_job_service_create_extent_pool_t *)job_queue_element_p->command_data;
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
                __FUNCTION__, (int)create_extent_pool_request_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(create_extent_pool_request_p->transaction_id);
        if(status != FBE_STATUS_OK){
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Could not abort transaction id %llu with configuration service\n", 
                    __FUNCTION__,
		    (unsigned long long)create_extent_pool_request_p->transaction_id);

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
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_CREATE_EXTENT_POOL;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

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
	job_queue_element_p->object_id = FBE_OBJECT_ID_INVALID;
    
    fbe_payload_control_set_status(control_operation, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
    
}

static fbe_status_t fbe_create_extent_pool_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_create_extent_pool_t * 				create_extent_pool_request_p = NULL;
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

    /* get the user data from job command*/
    create_extent_pool_request_p = (fbe_job_service_create_extent_pool_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(create_extent_pool_request_p->transaction_id);
    if (status != FBE_STATUS_OK) {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: failed tp commit transaction, status:%d\n",
                __FUNCTION__, status);

        if(create_extent_pool_request_p->job_callback != NULL){
            create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;

}

static fbe_status_t fbe_create_extent_pool_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        	status = FBE_STATUS_OK;
    fbe_job_service_create_extent_pool_t * 	create_extent_pool_request_p = NULL;
    fbe_payload_ex_t                     *	payload_p = NULL;
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
    create_extent_pool_request_p = (fbe_job_service_create_extent_pool_t *) job_queue_element_p->command_data;

    /* wait object to be ready */
    if (1) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(create_extent_pool_request_p->object_id, 
                                                                  FBE_LIFECYCLE_STATE_READY, 
                                                                  60000);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting rg object id %d be ready. status %d\n", 
                              __FUNCTION__, create_extent_pool_request_p->object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    //notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_CREATE_EXTENT_POOL;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_RAID_GROUP_CREATE;
    notification_info.notification_data.job_service_error_info.object_id = create_extent_pool_request_p->object_id;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
    status = fbe_notification_send(create_extent_pool_request_p->object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: status change notification failed, status %d\n",
                __FUNCTION__, status);
        
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = FBE_OBJECT_ID_INVALID;
    
    if(create_extent_pool_request_p->job_callback != NULL){
        create_extent_pool_request_p->job_callback(job_queue_element_p->job_number , FBE_STATUS_OK);
    }

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
	return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_create_extent_pool_get_pvd_object_list(fbe_job_service_create_extent_pool_t * create_extent_pool_request_p)
{
    fbe_database_control_get_pvd_list_for_ext_pool_t get_pvd_list;
    fbe_status_t status;

    get_pvd_list.drive_count = create_extent_pool_request_p->drive_count;
    get_pvd_list.drive_type = create_extent_pool_request_p->drive_type;
    //get_pvd_list.pvd_list = (fbe_object_id_t *)((fbe_u64_t)create_extent_pool_request_p + sizeof(fbe_job_service_create_extent_pool_t));
    get_pvd_list.pvd_list = (fbe_object_id_t *)(create_extent_pool_request_p + 1);

    /* update the proision drive configuration. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_GET_PVD_LIST_FOR_EXT_POOL,
                                                &get_pvd_list,
                                                sizeof(fbe_database_control_get_pvd_list_for_ext_pool_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}

static fbe_status_t fbe_create_extent_pool_create_extent_pool(
        fbe_job_service_create_extent_pool_t * create_extent_pool_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_database_control_create_ext_pool_t create_ext_pool;

    create_ext_pool.pool_id                                = create_extent_pool_request_p->pool_id;
    create_ext_pool.drive_count                            = create_extent_pool_request_p->drive_count;
    create_ext_pool.object_id                              = FBE_OBJECT_ID_INVALID;
    create_ext_pool.class_id                               = FBE_CLASS_ID_EXTENT_POOL;
    create_ext_pool.transaction_id                         = create_extent_pool_request_p->transaction_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL,
                                                 &create_ext_pool,
                                                 sizeof(fbe_database_control_create_ext_pool_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, extent pool id %d\n", 
                __FUNCTION__, 
                create_ext_pool.object_id);

        create_extent_pool_request_p->object_id = create_ext_pool.object_id;
    }
     return status;
}


static fbe_status_t fbe_create_extent_pool_create_extent_pool_metadata_lun(
        fbe_job_service_create_extent_pool_t * create_extent_pool_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_database_control_create_ext_pool_lun_t create_ext_pool_lun;

    create_ext_pool_lun.pool_id                                = create_extent_pool_request_p->pool_id;
    create_ext_pool_lun.lun_id                                 = 0;
    create_ext_pool_lun.capacity                               = 0x10000;
    create_ext_pool_lun.object_id                              = FBE_OBJECT_ID_INVALID;
    create_ext_pool_lun.class_id                               = FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN;
    create_ext_pool_lun.transaction_id                         = create_extent_pool_request_p->transaction_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL_LUN,
                                                 &create_ext_pool_lun,
                                                 sizeof(fbe_database_control_create_ext_pool_lun_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
				"%s: extent pool metadata lun id %d\n", 
                __FUNCTION__, 
                create_ext_pool_lun.object_id);

        create_extent_pool_request_p->metadata_lun_object_id = create_ext_pool_lun.object_id;
    }
     return status;
}

/*************************
 * end file fbe_create_extent_pool.c
 *************************/
