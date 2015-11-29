#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_multi_pvds_pool_id_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_multi_pvds_pool_id_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_multi_pvds_pool_id_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_update_multi_pvds_pool_id_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_multi_pvds_pool_id_commit_function(fbe_packet_t *packet_p);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_multi_pvds_pool_id_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID,
    {
        /* validation function */
        fbe_update_multi_pvds_pool_id_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_multi_pvds_pool_id_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_multi_pvds_pool_id_persist_function,

        /* response/rollback function */
        fbe_update_multi_pvds_pool_id_rollback_function,

        /* commit function */
        fbe_update_multi_pvds_pool_id_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_update_multi_pvds_pool_id_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_service_update_multi_pvds_pool_id_t * 		update_multi_pvds_pool_id_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_u32_t                                           index = 0;
    fbe_topology_mgmt_get_object_type_t                 topology_mgmt_get_object_type;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    
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

    update_multi_pvds_pool_id_request_p = (fbe_job_service_update_multi_pvds_pool_id_t*) job_queue_element_p->command_data;

    if (update_multi_pvds_pool_id_request_p->pvd_pool_data_list.total_elements > MAX_UPDATE_PVD_POOL_ID)
    {
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: the total element number(%d) of pvd pool id is not in right arrange\n",
                          __FUNCTION__, update_multi_pvds_pool_id_request_p->pvd_pool_data_list.total_elements);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check all the pvd object ids are valid */
    for (index = 0; index < update_multi_pvds_pool_id_request_p->pvd_pool_data_list.total_elements; index++)
    {
        /* Get and check the object type */
        topology_mgmt_get_object_type.object_id = update_multi_pvds_pool_id_request_p->pvd_pool_data_list.pvd_pool_data[index].pvd_object_id;

        status = fbe_create_lib_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE,
                                                    &topology_mgmt_get_object_type,
                                                    sizeof(topology_mgmt_get_object_type),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_OBJECT_ID_INVALID,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);

        if (status != FBE_STATUS_OK || topology_mgmt_get_object_type.topology_object_type != FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE) 
        {
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION;

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
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

static fbe_status_t fbe_update_multi_pvds_pool_id_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_multi_pvds_pool_id_t * 		update_multi_pvds_pool_id_request_p = NULL;
    fbe_payload_ex_t                       *			payload_p = NULL;
    fbe_payload_control_operation_t     *				control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 				length = 0;
    fbe_job_queue_element_t             *				job_queue_element_p = NULL;
    fbe_u32_t                                           index = 0;
    fbe_database_control_update_pvd_t                   update_pvd;

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
    update_multi_pvds_pool_id_request_p = (fbe_job_service_update_multi_pvds_pool_id_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_start_database_transaction(&update_multi_pvds_pool_id_request_p->transaction_id, job_queue_element_p->job_number);
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
    
    /* update the pvds pool id */
    for (index = 0; index < update_multi_pvds_pool_id_request_p->pvd_pool_data_list.total_elements; index++)
    {
        fbe_zero_memory(&update_pvd, sizeof(fbe_database_control_update_pvd_t));
        update_pvd.transaction_id = update_multi_pvds_pool_id_request_p->transaction_id;
        update_pvd.object_id = update_multi_pvds_pool_id_request_p->pvd_pool_data_list.pvd_pool_data[index].pvd_object_id;
        update_pvd.update_type = FBE_UPDATE_PVD_POOL_ID;
        update_pvd.update_type_bitmask = 0;
        update_pvd.pool_id = update_multi_pvds_pool_id_request_p->pvd_pool_data_list.pvd_pool_data[index].pvd_pool_id;

        /* Send the update provision drive configuration to database service */
        status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                    &update_pvd,
                                                    sizeof(fbe_database_control_update_pvd_t),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_DATABASE,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_OBJECT_ID_INVALID,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: FBE_DATABASE_CONTROL_CODE_UPDATE_PVD failed\n", __FUNCTION__);
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: index = %d, total_elements = 0x%x, pvd obj_id = 0x%x, pool_id = 0x%x\n",
                                   __FUNCTION__, index,  update_multi_pvds_pool_id_request_p->pvd_pool_data_list.total_elements, 
                                   update_multi_pvds_pool_id_request_p->pvd_pool_data_list.pvd_pool_data[index].pvd_object_id,
                                   update_multi_pvds_pool_id_request_p->pvd_pool_data_list.pvd_pool_data[index].pvd_pool_id);
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
    }

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_update_multi_pvds_pool_id_rollback_function(fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    fbe_job_service_update_multi_pvds_pool_id_t * 		update_multi_pvds_pool_id_request_p = NULL;
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
    update_multi_pvds_pool_id_request_p = (fbe_job_service_update_multi_pvds_pool_id_t *)job_queue_element_p->command_data;
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
                __FUNCTION__, (int)update_multi_pvds_pool_id_request_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(update_multi_pvds_pool_id_request_p->transaction_id);
        if(status != FBE_STATUS_OK){
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Could not abort transaction id %llu with configuration service\n", 
                    __FUNCTION__,
		    (unsigned long long)update_multi_pvds_pool_id_request_p->transaction_id);

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
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID;
    /*We update several pvds, so we can't provide a unique object id. What we care in this notification is job_number and job type */ 
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID; 

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
    
    fbe_payload_control_set_status(control_operation, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
    
}

static fbe_status_t fbe_update_multi_pvds_pool_id_persist_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        				status = FBE_STATUS_OK;
    fbe_job_service_update_multi_pvds_pool_id_t * 		update_multi_pvds_pool_id_request_p = NULL;
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
    update_multi_pvds_pool_id_request_p = (fbe_job_service_update_multi_pvds_pool_id_t *) job_queue_element_p->command_data;

	status = fbe_create_lib_commit_database_transaction(update_multi_pvds_pool_id_request_p->transaction_id);
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

static fbe_status_t fbe_update_multi_pvds_pool_id_commit_function(fbe_packet_t *packet_p)
{
	fbe_status_t                        	status = FBE_STATUS_OK;
    fbe_job_service_update_multi_pvds_pool_id_t * 		update_multi_pvds_pool_id_request_p = NULL;
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
    update_multi_pvds_pool_id_request_p = (fbe_job_service_update_multi_pvds_pool_id_t *) job_queue_element_p->command_data;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type =  FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
	notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
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


