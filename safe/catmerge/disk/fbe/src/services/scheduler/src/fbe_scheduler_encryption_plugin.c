#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

#include "fbe_testability.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_scheduler_private.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe_job_service.h"
#include "fbe_cmi.h"

/* Encryption plugin */
fbe_status_t fbe_scheduler_encryption_plugin_init(void);
fbe_status_t fbe_scheduler_encryption_plugin_destroy(void);
fbe_status_t fbe_scheduler_encryption_plugin_entry(fbe_packet_t * packet);


/* This will be used by sep_init to register plugin */
fbe_scheduler_plugin_t encryption_plugin = {fbe_scheduler_encryption_plugin_init, 
											fbe_scheduler_encryption_plugin_destroy,
											fbe_scheduler_encryption_plugin_entry};




static fbe_spinlock_t		encryption_plugin_lock; 
static fbe_u32_t			encryption_plugin_job_in_progress = 0;
/*
static fbe_u64_t            encryption_plugin_job_number = 0;
*/
static fbe_job_service_update_encryption_mode_t job_service_update_encryption_mode;

static void job_callback_function(fbe_u64_t job_number, fbe_status_t status);
static fbe_status_t fbe_scheduler_encryption_plugin_start_job_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t 
fbe_scheduler_encryption_plugin_init(void)
{
    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);


    /* Initialize scheduler lock */
    fbe_spinlock_init(&encryption_plugin_lock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_scheduler_encryption_plugin_destroy(void)
{

    scheduler_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);


    fbe_spinlock_destroy(&encryption_plugin_lock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_scheduler_encryption_plugin_entry(fbe_packet_t * packet)
{
	fbe_object_id_t object_id;
    fbe_payload_ex_t * payload;
	fbe_payload_control_operation_t * control_operation;
	fbe_status_t status;

	fbe_transport_get_object_id(packet, &object_id);

	/* Job will be started on ACTIVE SIDE ONLY */
	if(!fbe_cmi_is_active_sp()){
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	fbe_spinlock_lock(&encryption_plugin_lock);
	if(encryption_plugin_job_in_progress > 0){
		fbe_spinlock_unlock(&encryption_plugin_lock);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_present_control_operation(payload);

	switch(control_operation->status_qualifier){
		case FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTED_REQUEST:
			job_service_update_encryption_mode.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED;
			break;

		case FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS_REQUEST:
			job_service_update_encryption_mode.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS;
			break;

		case FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_REKEY_IN_PROGRESS_REQUEST:
			job_service_update_encryption_mode.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS;
			break;

		default:
			fbe_spinlock_unlock(&encryption_plugin_lock);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_OK;
	}

	job_service_update_encryption_mode.object_id = object_id;	
    job_service_update_encryption_mode.job_number = 0;
	job_service_update_encryption_mode.job_callback = job_callback_function;

	fbe_payload_ex_release_control_operation(payload, control_operation);
	control_operation = fbe_payload_ex_allocate_control_operation(payload);
	/* Send packet to the job service */
	fbe_payload_control_build_operation(control_operation,
										FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE,	
										&job_service_update_encryption_mode,
										sizeof(fbe_job_service_update_encryption_mode_t));

	/* Set packet address */
	fbe_transport_set_address(packet,
								FBE_PACKAGE_ID_SEP_0,
								FBE_SERVICE_ID_JOB_SERVICE,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID);

	fbe_transport_set_completion_function(packet, fbe_scheduler_encryption_plugin_start_job_completion, NULL);

	encryption_plugin_job_in_progress++;	
	fbe_spinlock_unlock(&encryption_plugin_lock);

	/* Control packets should be sent via service manager */
	status = fbe_service_manager_send_control_packet(packet);
	return FBE_STATUS_OK;
}


static void 
job_callback_function(fbe_u64_t job_number, fbe_status_t status)
{
    scheduler_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: job_number = %lld, status = %d\n",
                    __FUNCTION__, job_number, status);

	fbe_spinlock_lock(&encryption_plugin_lock);
	if(encryption_plugin_job_in_progress > 0){
		encryption_plugin_job_in_progress--;
	}
	fbe_spinlock_unlock(&encryption_plugin_lock);
}

static fbe_status_t 
fbe_scheduler_encryption_plugin_start_job_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload;
	fbe_payload_control_operation_t * control_operation;
	fbe_job_service_update_encryption_mode_t * job;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload); /* Job control operation */	

	fbe_payload_control_get_buffer(control_operation, &job);

	if(job){
		scheduler_trace(FBE_TRACE_LEVEL_INFO,
						FBE_TRACE_MESSAGE_ID_INFO,
						"%s: job_number %lld\n", __FUNCTION__, job->job_number);
	}

	return FBE_STATUS_OK;
}
