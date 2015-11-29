#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe_private_space_layout_generated.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_raid_group_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_raid_group_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_raid_group_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_raid_group_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_raid_group_commit_function(fbe_packet_t *packet_p);
static fbe_status_t unquiesce_rg_if_quiesced(fbe_object_id_t rg_id);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_raid_group_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_RAID_GROUP,
    {
        /* validation function */
        fbe_update_raid_group_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_raid_group_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_raid_group_persist_function,

        /* response/rollback function */
        fbe_update_raid_group_rollback_function,

        /* commit function */
        fbe_update_raid_group_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_update_raid_group_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_service_update_raid_group_t * 				update_raid_group_request_p = NULL;
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

    update_raid_group_request_p = (fbe_job_service_update_raid_group_t *) job_queue_element_p->command_data;

	if ((update_raid_group_request_p->power_save_idle_time_in_sec == 0) && 
		update_raid_group_request_p->update_type == FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME) {
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: power saving idle time can't be 0\n",
                          __FUNCTION__);

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
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_update_raid_group_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_raid_group_t * 				update_raid_group_request_p = NULL;
    fbe_payload_ex_t                       *				payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_database_control_update_raid_t		        config_update;

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
    update_raid_group_request_p = (fbe_job_service_update_raid_group_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_start_database_transaction(&update_raid_group_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start transaction %d\n",
                __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	config_update.object_id = update_raid_group_request_p->object_id;
	config_update.transaction_id = update_raid_group_request_p->transaction_id;
	config_update.power_save_idle_time_in_sec = update_raid_group_request_p->power_save_idle_time_in_sec;
	config_update.update_type = update_raid_group_request_p->update_type;
	config_update.new_rg_size = update_raid_group_request_p->new_rg_size;/*used for expansion*/
	config_update.new_edge_capacity = update_raid_group_request_p->new_edge_size;/*used for expansion*/
    
	status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_UPDATE_RAID,
												 &config_update,
												 sizeof(fbe_database_control_update_raid_t),
												 FBE_PACKAGE_ID_SEP_0,
												 FBE_SERVICE_ID_DATABASE,
												 FBE_CLASS_ID_INVALID,
												 FBE_OBJECT_ID_INVALID,
												 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to send update to DB service %d\n", __FUNCTION__, status);
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

static fbe_status_t fbe_update_raid_group_rollback_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    fbe_job_service_update_raid_group_t * update_raid_group_request_p = NULL;
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
    update_raid_group_request_p = (fbe_job_service_update_raid_group_t *)job_queue_element_p->command_data;
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
                __FUNCTION__, (int)update_raid_group_request_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(update_raid_group_request_p->transaction_id);
        if(status != FBE_STATUS_OK){
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Could not abort transaction id %llu with configuration service\n", 
                    __FUNCTION__,
		    (unsigned long long)update_raid_group_request_p->transaction_id);

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
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_UPDATE_RAID_GROUP;
    notification_info.notification_data.job_service_error_info.object_id = update_raid_group_request_p->object_id;


    status = fbe_notification_send(update_raid_group_request_p->object_id, notification_info);
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
	job_queue_element_p->object_id = update_raid_group_request_p->object_id;
    
    fbe_payload_control_set_status(control_operation, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
    
}

static fbe_status_t fbe_update_raid_group_persist_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_raid_group_t * 				update_raid_group_request_p = NULL;
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
    update_raid_group_request_p = (fbe_job_service_update_raid_group_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(update_raid_group_request_p->transaction_id);
    fbe_payload_control_set_status(control_operation_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

	if (status != FBE_STATUS_OK) {
		job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: failed tp commit transaction, status:%d\n",
                __FUNCTION__, status);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;

	}

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

}

static fbe_status_t fbe_update_raid_group_commit_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        	status = FBE_STATUS_OK;
    fbe_job_service_update_raid_group_t * 	update_raid_group_request_p = NULL;
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
    update_raid_group_request_p = (fbe_job_service_update_raid_group_t *) job_queue_element_p->command_data;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_UPDATE_RAID_GROUP;
    notification_info.notification_data.job_service_error_info.object_id = update_raid_group_request_p->object_id;
	notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
    status = fbe_notification_send(update_raid_group_request_p->object_id, notification_info);
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
    job_queue_element_p->object_id = update_raid_group_request_p->object_id;
    
    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
	return FBE_STATUS_OK;

}

static fbe_status_t unquiesce_rg_if_quiesced(fbe_object_id_t rg_id)
{
	fbe_status_t 					status = FBE_STATUS_GENERIC_FAILURE; 
	fbe_raid_group_get_io_info_t      raid_group_info;

    status = fbe_create_lib_send_control_packet (
                                FBE_RAID_GROUP_CONTROL_CODE_GET_IO_INFO,
                                &raid_group_info,
                                sizeof(fbe_raid_group_get_io_info_t),
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                rg_id,
                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, failed to get rg info to know if quiesced\n", __FUNCTION__);

		return status;
    }


	/*were we quieced ?*/
	if (raid_group_info.b_is_quiesced) {
		/*if so, let's unquiesce*/
		status = fbe_create_lib_send_control_packet (
									FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE,
									NULL,
									0,
									FBE_PACKAGE_ID_SEP_0,
									FBE_SERVICE_ID_TOPOLOGY,
									FBE_CLASS_ID_INVALID,
									rg_id,
									FBE_PACKET_FLAG_NO_ATTRIB);

		if (status == FBE_STATUS_GENERIC_FAILURE){
			job_service_trace(FBE_TRACE_LEVEL_ERROR, 
					FBE_TRACE_MESSAGE_ID_INFO,
					"%s, failed to unquiesce\n", __FUNCTION__);

			return status;
		}


	}

	return FBE_STATUS_OK;

}


