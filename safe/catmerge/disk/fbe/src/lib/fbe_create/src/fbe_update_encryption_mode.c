/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_update_encryption_mode.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the job for changing the encryption mode.
 *
 * @version
 *   10/31/2013:  Created. Rob Foley
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
//#include "fbe_private_space_layout_generated.h"
#include "fbe_private_space_layout.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_encryption_mode_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_encryption_mode_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_encryption_mode_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_encryption_mode_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_encryption_mode_commit_function(fbe_packet_t *packet_p);
static fbe_status_t unquiesce_rg_if_quiesced(fbe_object_id_t rg_id);
static fbe_status_t fbe_update_encryption_mode_validate_lun_state(fbe_object_id_t rg_id);
static fbe_status_t fbe_update_encryption_mode_validate_vd_state(fbe_object_id_t rg_id);
static fbe_status_t fbe_update_encryption_mode_validate_pvd_state(fbe_object_id_t rg_id);
static fbe_status_t fbe_update_encryption_mode_check_pvd_state(fbe_object_id_t rg_id);
static fbe_status_t fbe_update_encryption_mode_restore_unmap_ability(fbe_object_id_t rg_id);
static fbe_status_t fbe_update_encryption_mode_restore_unmap_ability_pvd(fbe_object_id_t rg_id);


/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_encryption_mode_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE,
    {
        /* validation function */
        fbe_update_encryption_mode_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_encryption_mode_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_encryption_mode_persist_function,

        /* response/rollback function */
        fbe_update_encryption_mode_rollback_function,

        /* commit function */
        fbe_update_encryption_mode_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_update_encryption_mode_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_service_update_encryption_mode_t * 				update_encryption_mode_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
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

    update_encryption_mode_request_p = (fbe_job_service_update_encryption_mode_t *) job_queue_element_p->command_data;

	if ((update_encryption_mode_request_p->encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID) ||
        (update_encryption_mode_request_p->encryption_mode >= FBE_BASE_CONFIG_ENCRYPTION_MODE_LAST)) {
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: encryption mode is invalid 0x%x\n",
                          __FUNCTION__, update_encryption_mode_request_p->encryption_mode);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

		if(update_encryption_mode_request_p->job_callback != NULL){
			update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}

		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    if (update_encryption_mode_request_p->encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) {
        /* Check whether all the upstream objects (LUNs) are in ready state */
        status = fbe_update_encryption_mode_validate_lun_state(update_encryption_mode_request_p->object_id);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: LUNs are not ready\n",
                              __FUNCTION__);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY;

            if(update_encryption_mode_request_p->job_callback != NULL){
                update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            }

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Check whether any of the VDs are in PACO state */
        status = fbe_update_encryption_mode_validate_vd_state(update_encryption_mode_request_p->object_id);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: one of the VD in PACO\n",
                              __FUNCTION__);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY;

            if(update_encryption_mode_request_p->job_callback != NULL){
                update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            }

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Check whether any of the PVDs are in Zeroing state */
        status = fbe_update_encryption_mode_validate_pvd_state(update_encryption_mode_request_p->object_id);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: one of the PVD in not in a state for encryption \n",
                              __FUNCTION__);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY;

            if (update_encryption_mode_request_p->job_callback != NULL)
            {
                update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            }

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            status = fbe_update_encryption_mode_restore_unmap_ability(update_encryption_mode_request_p->object_id);
            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: one of the PVD in not in a state for encryption \n",
                                  __FUNCTION__);

                job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY;

                if (update_encryption_mode_request_p->job_callback != NULL)
                {
                    update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                }

                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }


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


static fbe_status_t fbe_update_encryption_mode_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_encryption_mode_t * 				update_encryption_mode_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_database_ds_object_list_t ds_list;
    fbe_u32_t index;

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
    update_encryption_mode_request_p = (fbe_job_service_update_encryption_mode_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_start_database_transaction(&update_encryption_mode_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start transaction %d\n",
                __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

		if(update_encryption_mode_request_p->job_callback != NULL){
			update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_rg_get_vd_pvd_list(update_encryption_mode_request_p->object_id, &ds_list);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to get ds list %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

		if(update_encryption_mode_request_p->job_callback != NULL){
			update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for ( index = 0; index < ds_list.number_of_downstream_objects; index++) {
        status = fbe_create_lib_database_service_update_encryption_mode(update_encryption_mode_request_p->transaction_id,
                                                                        ds_list.downstream_object_list[index],
                                                                        update_encryption_mode_request_p->encryption_mode);

        if (status == FBE_STATUS_GENERIC_FAILURE) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to send disk 0x%x update to DB service %d\n", 
                              __FUNCTION__, ds_list.downstream_object_list[index], status);
            break;
        }
    }
    if (status == FBE_STATUS_OK) {
        status = fbe_create_lib_database_service_update_encryption_mode(update_encryption_mode_request_p->transaction_id,
                                                                        update_encryption_mode_request_p->object_id,
                                                                        update_encryption_mode_request_p->encryption_mode);

        if (status == FBE_STATUS_GENERIC_FAILURE) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to send update to DB service %d\n", __FUNCTION__, status);
        }
    }


	if(status != FBE_STATUS_OK){
		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

		if(update_encryption_mode_request_p->job_callback != NULL){
			update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		}
	    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

	} else {
		job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

	}

    /* update job_queue_element status */
    job_queue_element_p->status = status;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_update_encryption_mode_rollback_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    fbe_job_service_update_encryption_mode_t * update_encryption_mode_request_p = NULL;
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
    update_encryption_mode_request_p = (fbe_job_service_update_encryption_mode_t *)job_queue_element_p->command_data;
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
                __FUNCTION__, (int)update_encryption_mode_request_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(update_encryption_mode_request_p->transaction_id);
        if(status != FBE_STATUS_OK){
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Could not abort transaction id %llu with configuration service\n", 
                    __FUNCTION__,
		    (unsigned long long)update_encryption_mode_request_p->transaction_id);

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
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE;
    notification_info.notification_data.job_service_error_info.object_id = update_encryption_mode_request_p->object_id;


    status = fbe_notification_send(update_encryption_mode_request_p->object_id, notification_info);
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
	job_queue_element_p->object_id = update_encryption_mode_request_p->object_id;
    
    fbe_payload_control_set_status(control_operation, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
    
}

static fbe_status_t fbe_update_encryption_mode_persist_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_encryption_mode_t * 				update_encryption_mode_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
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
    update_encryption_mode_request_p = (fbe_job_service_update_encryption_mode_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(update_encryption_mode_request_p->transaction_id);
    
	if (status != FBE_STATUS_OK) {
		job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: failed tp commit transaction, status:%d\n",
                __FUNCTION__, status);

		if(update_encryption_mode_request_p->job_callback != NULL){
			update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
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

static fbe_status_t fbe_update_encryption_mode_commit_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        	status = FBE_STATUS_OK;
    fbe_job_service_update_encryption_mode_t * 	update_encryption_mode_request_p = NULL;
    fbe_payload_ex_t                       *	payload_p = NULL;
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
    update_encryption_mode_request_p = (fbe_job_service_update_encryption_mode_t *) job_queue_element_p->command_data;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE;
    notification_info.notification_data.job_service_error_info.object_id = update_encryption_mode_request_p->object_id;
	notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
    status = fbe_notification_send(update_encryption_mode_request_p->object_id, notification_info);
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
    job_queue_element_p->object_id = update_encryption_mode_request_p->object_id;
    
	if(update_encryption_mode_request_p->job_callback != NULL){
		update_encryption_mode_request_p->job_callback(job_queue_element_p->job_number , FBE_STATUS_OK);
	}

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
	return FBE_STATUS_OK;
}

static fbe_status_t fbe_update_encryption_mode_validate_lun_state(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_u32_t         number_of_upstream_objects = 0;
    fbe_object_id_t   upstream_object_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_raid_group_type_t raid_type;
    fbe_u32_t         i;
    fbe_lifecycle_state_t lifecycle_state;

    status =  fbe_create_lib_get_raid_group_type(rg_id, &raid_type); 
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "fbe_create_lib_get_raid_group_type call failed, status %d\n", status);
        return status;
    }

    if (raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        status = fbe_create_lib_get_upstream_edge_list(rg_id, 
                                                       &number_of_upstream_objects,
                                                       upstream_object_list);
        if ((status != FBE_STATUS_OK) || (number_of_upstream_objects != 1)) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					"fbe_create_lib_get_upstream_edge_list call failed, status %d, obj %d\n", status, number_of_upstream_objects);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        rg_id = upstream_object_list[0];
    }

    status = fbe_create_lib_get_upstream_edge_list(rg_id, 
                                                   &number_of_upstream_objects,
                                                   upstream_object_list);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "fbe_create_lib_get_upstream_edge_list call failed, status %d\n", status);
        return status;
    }

    for (i = 0; i < number_of_upstream_objects; i++)
    {
        status = fbe_create_lib_get_object_state(upstream_object_list[i], &lifecycle_state);
        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "fbe_create_lib_get_object_state failed, status %d\n", status);
            return status;
        }
        if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY && lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)) 
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "object 0x%x not in ready state 0x%x\n", upstream_object_list[i], lifecycle_state);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_update_encryption_mode_check_vd_config_mode(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_base_config_downstream_object_list_t ds_object_list;
    fbe_virtual_drive_configuration_t get_configuration;
    fbe_u32_t object_index;

    status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                &ds_object_list,
                                                sizeof(fbe_base_config_downstream_object_list_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                rg_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (object_index = 0; object_index < ds_object_list.number_of_downstream_objects; object_index++)
    {
        
        status = fbe_create_lib_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                    &get_configuration,
                                                    sizeof(fbe_virtual_drive_configuration_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    ds_object_list.downstream_object_list[object_index],
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
            (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "fbe_update_encryption_mode_validate_vd_state VD 0x%x in copy\n", 
                              ds_object_list.downstream_object_list[object_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_update_encryption_mode_validate_vd_state(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_base_config_downstream_object_list_t mirror_object_list;
    fbe_u32_t object_index;
    fbe_raid_group_type_t raid_type;

    if (fbe_private_space_layout_object_id_is_system_raid_group(rg_id))
    {
        return FBE_STATUS_OK;
    }

    status =  fbe_create_lib_get_raid_group_type(rg_id, &raid_type); 
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "fbe_create_lib_get_raid_group_type call failed, status %d\n", status);
        return status;
    }

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                    &mirror_object_list,
                                                    sizeof(fbe_base_config_downstream_object_list_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    rg_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        for (object_index = 0; object_index < mirror_object_list.number_of_downstream_objects; object_index++)
        {
            status = fbe_update_encryption_mode_check_vd_config_mode(mirror_object_list.downstream_object_list[object_index]);
            if (status != FBE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    else
    {
        status = fbe_update_encryption_mode_check_vd_config_mode(rg_id);
    }

    return status;
}

static fbe_status_t fbe_update_encryption_mode_validate_pvd_state(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_base_config_downstream_object_list_t mirror_object_list;
    fbe_u32_t object_index;
    fbe_raid_group_type_t raid_type;

    if (fbe_private_space_layout_object_id_is_system_raid_group(rg_id))
    {
        return FBE_STATUS_OK;
    }

    status =  fbe_create_lib_get_raid_group_type(rg_id, &raid_type); 
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "fbe_create_lib_get_raid_group_type call failed, status %d\n", status);
        return status;
    }

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                    &mirror_object_list,
                                                    sizeof(fbe_base_config_downstream_object_list_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    rg_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        for (object_index = 0; object_index < mirror_object_list.number_of_downstream_objects; object_index++)
        {
            status = fbe_update_encryption_mode_check_pvd_state(mirror_object_list.downstream_object_list[object_index]);
            if (status != FBE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    else
    {
        status = fbe_update_encryption_mode_check_pvd_state(rg_id);
    }

    return status;
}

static fbe_status_t fbe_update_encryption_mode_check_pvd_state(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_base_config_downstream_object_list_t ds_vd_object_list;
    fbe_base_config_downstream_object_list_t ds_pvd_object_list;
    fbe_provision_drive_info_t pvd_info;
    fbe_u32_t vd_object_index;
    fbe_u32_t pvd_object_index;

    /* First get the downstream VDs Object ID */
    status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                &ds_vd_object_list,
                                                sizeof(fbe_base_config_downstream_object_list_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                rg_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "fbe_update_encryption_mode_validate_pvd_state failed to get VD List. status %d\n", status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (vd_object_index = 0; vd_object_index < ds_vd_object_list.number_of_downstream_objects; vd_object_index++)
    {
        /* For each of the VDs, get the downstream PVDs */
        status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                    &ds_pvd_object_list,
                                                    sizeof(fbe_base_config_downstream_object_list_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    ds_vd_object_list.downstream_object_list[vd_object_index],
                                                    FBE_PACKET_FLAG_NO_ATTRIB);

         if (status != FBE_STATUS_OK)
         {
             job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
             return FBE_STATUS_GENERIC_FAILURE;
         }

         /* For each of the PVDs, get the Zeroing information */
         for (pvd_object_index = 0; pvd_object_index < ds_pvd_object_list.number_of_downstream_objects; pvd_object_index++)
         {
            status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                        &pvd_info,
                                                        sizeof(fbe_provision_drive_info_t),
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_INVALID,
                                                        ds_pvd_object_list.downstream_object_list[pvd_object_index],
                                                        FBE_PACKET_FLAG_NO_ATTRIB);
            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Get PVD Info failed, status %d\n", status);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (((pvd_info.flags & FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED) != 0) &&
                ((pvd_info.flags & FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED) != 0))
            {
                fbe_provision_drive_control_set_zero_checkpoint_t set_zero_checkpoint;
                fbe_provision_drive_control_set_unmap_enabled_disabled_t set_unmap_enabled_disabled;
                set_zero_checkpoint.background_zero_checkpoint = pvd_info.default_offset;
                set_unmap_enabled_disabled.is_enabled = FBE_FALSE;

                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "UNMAP clear unmap support 0x%x\n", 
                                  ds_pvd_object_list.downstream_object_list[pvd_object_index]);
                /* Do not issue unmap write sames for background zeroing requests so that
                 * we can write the paged as nz:0 and cu:1 when encryption is enabled
                 */
                status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_SET_UNMAP_ENABLED_DISABLED,
                                                            &set_unmap_enabled_disabled,
                                                            sizeof(fbe_provision_drive_control_set_unmap_enabled_disabled_t),
                                                            FBE_PACKAGE_ID_SEP_0,
                                                            FBE_SERVICE_ID_TOPOLOGY,
                                                            FBE_CLASS_ID_INVALID,
                                                            ds_pvd_object_list.downstream_object_list[pvd_object_index],
                                                            FBE_PACKET_FLAG_NO_ATTRIB);

                if (status != FBE_STATUS_OK)
                {
                    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "UNMAP unable to clear unmap enabled flag, status %d\n", status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "UNMAP setting the zero checkpoint to 0x%llx 0x%x\n", 
                                  pvd_info.default_offset,
                                  ds_pvd_object_list.downstream_object_list[pvd_object_index]);

                /* send usurper to move checkpoint back so that we zero out unmapped areas
                 */
                status = fbe_create_lib_send_control_packet_with_cpu_id(FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT,
                                                            &set_zero_checkpoint,
                                                            sizeof(fbe_provision_drive_control_set_zero_checkpoint_t),
                                                            FBE_PACKAGE_ID_SEP_0,
                                                            FBE_SERVICE_ID_TOPOLOGY,
                                                            FBE_CLASS_ID_INVALID,
                                                            ds_pvd_object_list.downstream_object_list[pvd_object_index],
                                                            FBE_PACKET_FLAG_NO_ATTRIB);

                if (status != FBE_STATUS_OK)
                {
                    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "UNMAP unable to set zero checkpoint for encryption, status %d\n", status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else if (pvd_info.zero_checkpoint != FBE_LBA_INVALID)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "fbe_update_encryption_mode_validate_pvd_state Zero chkpt:0x%llx PVD 0x%x\n", 
                                  pvd_info.zero_checkpoint,
                                  ds_pvd_object_list.downstream_object_list[pvd_object_index]);
                return FBE_STATUS_GENERIC_FAILURE;
            }
         }
    }


    return status;
}

static fbe_status_t fbe_update_encryption_mode_restore_unmap_ability(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_base_config_downstream_object_list_t mirror_object_list;
    fbe_u32_t object_index;
    fbe_raid_group_type_t raid_type;

    if (fbe_private_space_layout_object_id_is_system_raid_group(rg_id))
    {
        return FBE_STATUS_OK;
    }

    status =  fbe_create_lib_get_raid_group_type(rg_id, &raid_type); 
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "fbe_create_lib_get_raid_group_type call failed, status %d\n", status);
        return status;
    }

    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                    &mirror_object_list,
                                                    sizeof(fbe_base_config_downstream_object_list_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    rg_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        for (object_index = 0; object_index < mirror_object_list.number_of_downstream_objects; object_index++)
        {
            status = fbe_update_encryption_mode_restore_unmap_ability_pvd(mirror_object_list.downstream_object_list[object_index]);
            if (status != FBE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    else
    {
        status = fbe_update_encryption_mode_restore_unmap_ability_pvd(rg_id);
    }

    return status;
}

static fbe_status_t fbe_update_encryption_mode_restore_unmap_ability_pvd(fbe_object_id_t rg_id)
{
    fbe_status_t      status;
    fbe_base_config_downstream_object_list_t ds_vd_object_list;
    fbe_base_config_downstream_object_list_t ds_pvd_object_list;
    fbe_provision_drive_info_t pvd_info;
    fbe_u32_t vd_object_index;
    fbe_u32_t pvd_object_index;

    /* First get the downstream VDs Object ID */
    status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                &ds_vd_object_list,
                                                sizeof(fbe_base_config_downstream_object_list_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                rg_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "fbe_update_encryption_mode_validate_pvd_state failed to get VD List. status %d\n", status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Reenable unmap support on the drive
     */
    for (vd_object_index = 0; vd_object_index < ds_vd_object_list.number_of_downstream_objects; vd_object_index++)
    {
        /* For each of the VDs, get the downstream PVDs */
        status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                    &ds_pvd_object_list,
                                                    sizeof(fbe_base_config_downstream_object_list_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    ds_vd_object_list.downstream_object_list[vd_object_index],
                                                    FBE_PACKET_FLAG_NO_ATTRIB);

         if (status != FBE_STATUS_OK)
         {
             job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "fbe_update_encryption_mode_validate_vd_state failed, status %d\n", status);
             return FBE_STATUS_GENERIC_FAILURE;
         }

         /* For each of the PVDs, get the Zeroing information */
         for (pvd_object_index = 0; pvd_object_index < ds_pvd_object_list.number_of_downstream_objects; pvd_object_index++)
         {
            status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                        &pvd_info,
                                                        sizeof(fbe_provision_drive_info_t),
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_INVALID,
                                                        ds_pvd_object_list.downstream_object_list[pvd_object_index],
                                                        FBE_PACKET_FLAG_NO_ATTRIB);
            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Get PVD Info failed, status %d\n", status);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (((pvd_info.flags & FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED) != 0) &&
                ((pvd_info.flags & FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED) == 0) &&
                pvd_info.zero_checkpoint == FBE_LBA_INVALID)
            {
                fbe_provision_drive_control_set_unmap_enabled_disabled_t set_unmap_enabled_disabled;
                set_unmap_enabled_disabled.is_enabled = FBE_TRUE;
                
                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "UNMAP reset unmap support 0x%x\n", 
                                  ds_pvd_object_list.downstream_object_list[pvd_object_index]);

                /* Once encryption is enabled we can do unmaps again if needed
                 */
                status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_SET_UNMAP_ENABLED_DISABLED,
                                                            &set_unmap_enabled_disabled,
                                                            sizeof(fbe_provision_drive_control_set_unmap_enabled_disabled_t),
                                                            FBE_PACKAGE_ID_SEP_0,
                                                            FBE_SERVICE_ID_TOPOLOGY,
                                                            FBE_CLASS_ID_INVALID,
                                                            ds_pvd_object_list.downstream_object_list[pvd_object_index],
                                                            FBE_PACKET_FLAG_NO_ATTRIB);

                if (status != FBE_STATUS_OK)
                {
                    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "UNMAP unable to set unmap enabled flag, status %d\n", status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
         }
    }

    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_update_encryption_mode.c
 *************************/
