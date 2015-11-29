#include "fbe_persist_private.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_service_manager_interface.h"

/*local definitions*/
typedef enum fbe_persist_cmi_thread_flag_e{
    FBE_PERSIST_CMI_THREAD_RUN,
    FBE_PERSIST_CMI_THREAD_STOP,
    FBE_PERSIST_CMI_THREAD_DONE
}fbe_persist_cmi_thread_flag_t;

/*local arguments*/
static fbe_semaphore_t					fbe_persist_cmi_semaphore;
static fbe_persist_cmi_thread_flag_t	fbe_persist_cmi_thread_flag;
static fbe_thread_t                 	fbe_persist_cmi_thread_handle;

/*local function*/
static fbe_status_t fbe_persist_cmi_process_contact_lost(void);
static void fbe_persist_cmi_thread_func(void *context);
static void fbe_persist_cmi_process_peer_dead(void);
static fbe_status_t fbe_persist_cmi_set_lun_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*************************************************************************************************************************************************************/
static fbe_status_t fbe_persist_cmi_callback(fbe_cmi_event_t event, fbe_u32_t user_message_length, fbe_cmi_message_t user_message, fbe_cmi_event_callback_context_t context)
{
	fbe_status_t	status = FBE_STATUS_OK;

	/*for persistance service all we care about is if the peer died.*/
    switch (event) {
	case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
        break;
	case FBE_CMI_EVENT_SP_CONTACT_LOST:
		status = fbe_persist_cmi_process_contact_lost();
		break;
	case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        break;
	case FBE_CMI_EVENT_FATAL_ERROR:
        break;
	case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        break;
	case FBE_CMI_EVENT_PEER_BUSY:
		break;
	default:
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s, Invalid state (%d)for processing FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE\n", __FUNCTION__, event);

		status = FBE_STATUS_GENERIC_FAILURE;
	}
    
	return status;
}

fbe_status_t fbe_persist_cmi_init(void)
{
	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
	EMCPAL_STATUS       	nt_status;

	status =  fbe_cmi_register(FBE_CMI_CLIENT_ID_PERSIST, fbe_persist_cmi_callback, NULL);
	if (status != FBE_STATUS_OK) {
        
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s failed to init CMI connection\n", __FUNCTION__);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_semaphore_init(&fbe_persist_cmi_semaphore, 0, 10);

	fbe_persist_cmi_thread_flag = FBE_PERSIST_CMI_THREAD_RUN;
	nt_status = fbe_thread_init(&fbe_persist_cmi_thread_handle, "fbe_persist_cmi", fbe_persist_cmi_thread_func, NULL);
	if (nt_status != EMCPAL_STATUS_SUCCESS) {
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s: can't start message process thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 

		return FBE_STATUS_GENERIC_FAILURE;
	}


	return FBE_STATUS_OK;
}

fbe_status_t fbe_persist_cmi_destroy(void)
{
	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

    status =  fbe_cmi_unregister(FBE_CMI_CLIENT_ID_PERSIST);
	if (status != FBE_STATUS_OK) {
        
		fbe_persist_trace(FBE_TRACE_LEVEL_ERROR,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						  "%s failed to unregister CMI connection\n", __FUNCTION__);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_persist_cmi_thread_flag = FBE_PERSIST_CMI_THREAD_STOP;

	fbe_semaphore_release(&fbe_persist_cmi_semaphore, 0, 1, FALSE);
        fbe_thread_wait(&fbe_persist_cmi_thread_handle);
	fbe_thread_destroy(&fbe_persist_cmi_thread_handle);

	fbe_semaphore_destroy(&fbe_persist_cmi_semaphore);

	return FBE_STATUS_OK;
}


static fbe_status_t fbe_persist_cmi_process_contact_lost(void)
{
    fbe_cmi_sp_state_t  state  = FBE_CMI_STATE_INVALID;

    /* If current SP is service mode, don't wake up the thread*/
    fbe_cmi_get_current_sp_state(&state);
    if (state == FBE_CMI_STATE_SERVICE_MODE) {
        fbe_persist_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: current SP is degraded mode\n", 
                          __FUNCTION__); 
        return FBE_STATUS_OK;
    }

    /*wake up the thread*/
	fbe_semaphore_release(&fbe_persist_cmi_semaphore, 0, 1, FALSE);

	return FBE_STATUS_OK;
}

static void fbe_persist_cmi_thread_func(void *context)
{
	FBE_UNREFERENCED_PARAMETER(context);

	while (1) {
		fbe_semaphore_wait(&fbe_persist_cmi_semaphore, NULL);
		if (fbe_persist_cmi_thread_flag == FBE_PERSIST_CMI_THREAD_RUN) {
			fbe_persist_cmi_process_peer_dead();
		}else{
			break;
		}
	}

	fbe_persist_cmi_thread_flag = FBE_PERSIST_CMI_THREAD_STOP;
	fbe_persist_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
					FBE_TRACE_MESSAGE_ID_INFO, "%s: done\n", __FUNCTION__); 
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void fbe_persist_cmi_process_peer_dead(void)
{
	fbe_packet_t *						packet;
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
	fbe_persist_control_set_lun_t *     set_lun = NULL;

	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "Persist CMI - peer is dead, start to take ownership...\n");

	/*when the peer dies, we need to take over by repeating the sequance of setting the LUN.
	This will:
	1) Rebuild the bitmap of available entries.
	2) Look for a journal and if it exist it will execute the journal entries
	*/

	set_lun = (fbe_persist_control_set_lun_t *)fbe_memory_ex_allocate(sizeof(fbe_persist_control_set_lun_t));
	if (set_lun == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: Failed to allocate structure\n", __FUNCTION__);
        return ;
    }

    packet = fbe_transport_allocate_packet();
	if (packet == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: Failed to allocate packet\n", __FUNCTION__);
        fbe_memory_ex_release(set_lun);
        return ;
    }

	status = fbe_transport_initialize_sep_packet(packet);
	if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: Failed to init packet\n", __FUNCTION__);
        fbe_memory_ex_release(set_lun);
        fbe_transport_release_packet(packet);
        return ;
    }

	sep_payload = fbe_transport_get_payload_ex (packet);
    if (sep_payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: Failed to get payload\n", __FUNCTION__);
        fbe_memory_ex_release(set_lun);
        fbe_transport_release_packet(packet);
        return ;
    }
    
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: Failed to allocate control operation\n", __FUNCTION__);
        fbe_memory_ex_release(set_lun);
        fbe_transport_release_packet(packet);
        return ;
    }

    fbe_payload_control_build_operation (control_operation,
                                         FBE_PERSIST_CONTROL_CODE_SET_LUN,
                                         set_lun,
                                         sizeof(fbe_persist_control_set_lun_t));


    fbe_transport_set_completion_function(packet, fbe_persist_cmi_set_lun_completion, set_lun);

	/*since we bypass the service manager and go directly to the service, we need to do that manually*/
	fbe_payload_ex_increment_control_operation_level(sep_payload);
	fbe_transport_increment_stack_level(packet);

	/*the LU is already set so we skip over to the important stuff*/
	fbe_persist_transaction_read_sectors_and_update_bitmap(packet);
}

static fbe_status_t fbe_persist_cmi_set_lun_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_persist_control_set_lun_t *     	set_lun = (fbe_persist_control_set_lun_t *)context;
    fbe_status_t 							status;
    fbe_payload_ex_t *						payload;
    fbe_payload_control_operation_t * 		control_operation;
    fbe_payload_control_status_t 			control_status;

    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s taking ownership over persistence failed\n", __FUNCTION__);
        
		fbe_memory_ex_release(set_lun);
		fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    }

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get payload\n", __FUNCTION__);
        fbe_memory_ex_release(set_lun);
		fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Verify that the operation completed ok. */
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Can't get control\n", __FUNCTION__);
        fbe_memory_ex_release(set_lun);
		fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_persist_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s taking ownership over persistence failed,status 0x%X\n", __FUNCTION__, control_status);
        fbe_payload_ex_release_control_operation(payload, control_operation);
		fbe_memory_ex_release(set_lun);
		fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    }

    /* We are done with the entry read operation. */
    fbe_payload_ex_release_control_operation(payload, control_operation);
	fbe_memory_ex_release(set_lun);
	fbe_transport_release_packet(packet);

	fbe_persist_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Took over ownership of persistence from failed peer\n", __FUNCTION__);

	return FBE_STATUS_OK;
}
