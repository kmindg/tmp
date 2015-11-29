#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"

#include "fbe_service_manager.h"
#include "fbe_base_service.h"

/* Forward declaration */
static fbe_status_t fbe_base_service_send_init_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_service_send_destroy_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_service_control_set_trace_level(fbe_base_service_t * base_service, fbe_packet_t * packet);
static fbe_status_t fbe_base_service_control_get_trace_level(fbe_base_service_t * base_service, fbe_packet_t * packet);

fbe_status_t 
fbe_base_service_send_init_command(fbe_service_id_t service_id)
{
    fbe_packet_t * packet = NULL;
    fbe_semaphore_t sem;
    fbe_package_id_t package_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    
    /* The memory service is not available yet */
    packet = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');
    if(packet == NULL){
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    status = fbe_transport_initialize_packet(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&sem);
        fbe_transport_destroy_packet(packet);
        fbe_release_nonpaged_pool(packet);
        return status;
    }
    status = fbe_transport_build_control_packet(packet, 
                                                FBE_BASE_SERVICE_CONTROL_CODE_INIT,
                                                NULL,
                                                0,
                                                0,
                                                fbe_base_service_send_init_command_completion,
                                                &sem);
    

    if(status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&sem);
        fbe_transport_destroy_packet(packet);
        fbe_release_nonpaged_pool(packet);
        return status;
    }

    /* Set packet address */
    status = fbe_get_package_id(&package_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&sem);
        fbe_transport_destroy_packet(packet);
        fbe_release_nonpaged_pool(packet);
        return status;
    }
    status = fbe_transport_set_address( packet,
                                        package_id,
                                        service_id,
                                        FBE_CLASS_ID_INVALID,
                                        FBE_OBJECT_ID_INVALID);
    if(status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&sem);
        fbe_transport_destroy_packet(packet);
        fbe_release_nonpaged_pool(packet);
        return status;
    }

    status = fbe_service_manager_send_control_packet(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(status != FBE_STATUS_OK)
    {
        fbe_semaphore_destroy(&sem);
        fbe_payload_ex_release_control_operation(payload, control_operation);
        fbe_transport_destroy_packet(packet);
        fbe_release_nonpaged_pool(packet);
        return status;
    }

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    status = fbe_payload_ex_release_control_operation(payload, control_operation);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_destroy_packet(packet);
        fbe_release_nonpaged_pool(packet);
        return status;
    }

    status = fbe_transport_destroy_packet(packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_release_nonpaged_pool(packet);
        return status;
    }

    fbe_release_nonpaged_pool(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_base_service_send_init_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_base_service_send_destroy_command(fbe_service_id_t service_id)
{
    fbe_status_t status;
	fbe_packet_t * packet = NULL;
	fbe_semaphore_t sem;
	fbe_package_id_t package_id;

	/* The memory service is not available yet */
	packet = fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'pfbe');
	if(packet == NULL){
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

    fbe_semaphore_init(&sem, 0, 1);

	/* Allocate packet */
	fbe_transport_initialize_packet(packet);

	fbe_transport_build_control_packet(packet, 
								FBE_BASE_SERVICE_CONTROL_CODE_DESTROY,
								NULL,
								0,
								0,
								fbe_base_service_send_destroy_command_completion,
								&sem);

		/* Set packet address */
	fbe_get_package_id(&package_id);
	fbe_transport_set_address(	packet,
								package_id,
								service_id,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID); 

	status = fbe_service_manager_send_control_packet(packet);
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	fbe_transport_destroy_packet(packet);
	fbe_release_nonpaged_pool(packet);

	return status;
}

static fbe_status_t 
fbe_base_service_send_destroy_command_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_service_set_service_id(fbe_base_service_t * base_service, fbe_service_id_t service_id)
{
	base_service->service_id = service_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_service_set_trace_level(fbe_base_service_t * base_service, fbe_trace_level_t trace_level)
{
	base_service->trace_level = trace_level;
	return FBE_STATUS_OK;
}

fbe_trace_level_t
fbe_base_service_get_trace_level(fbe_base_service_t * base_service)
{
	return base_service->trace_level;
}

void 
fbe_base_service_trace(fbe_base_service_t * base_service, 
		       fbe_trace_level_t trace_level,
		       fbe_trace_message_id_t message_id,
		       const fbe_char_t * fmt, ...)
{
	va_list argList;
	if(trace_level <= base_service->trace_level)	{
		va_start(argList, fmt);
		fbe_trace_report(	FBE_COMPONENT_TYPE_SERVICE,
							base_service->service_id,
							trace_level,
							message_id,
							fmt, 
							argList);
		va_end(argList);
	}
}

fbe_status_t 
fbe_base_service_init(fbe_base_service_t * base_service)
{
	if(fbe_base_service_is_initialized(base_service)){
		fbe_base_service_trace(base_service,
							   FBE_TRACE_LEVEL_CRITICAL_ERROR,
							   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							   "%s: Service 0x%X is initialized! Should not be initialized again.\n",
							   __FUNCTION__, base_service->service_id);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	base_service->initialized = TRUE;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_service_destroy(fbe_base_service_t * base_service)
{
	base_service->initialized = FALSE;
	return FBE_STATUS_OK;
}

fbe_bool_t 
fbe_base_service_is_initialized(fbe_base_service_t * base_service)
{
	return base_service->initialized;
}

fbe_status_t 
fbe_base_service_control_entry(fbe_base_service_t * base_service, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_control_operation_opcode_t control_code;

	control_code = fbe_transport_get_control_code(packet);

	switch(control_code) {
		case FBE_BASE_SERVICE_CONTROL_CODE_SET_TRACE_LEVEL:
			status = fbe_base_service_control_set_trace_level(base_service, packet);
			break;
		case FBE_BASE_SERVICE_CONTROL_CODE_GET_TRACE_LEVEL:
			status = fbe_base_service_control_get_trace_level(base_service, packet);
			break;

		default:
			fbe_base_service_trace(base_service,
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s: Unknown control code 0x%X\n", __FUNCTION__, control_code);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			status = FBE_STATUS_GENERIC_FAILURE;
			fbe_transport_complete_packet(packet);
			break;
	}

	return status;
}

static fbe_status_t 
fbe_base_service_control_set_trace_level(fbe_base_service_t * base_service, fbe_packet_t * packet)
{
	fbe_base_service_control_set_trace_level_t * base_service_control_set_trace_level = NULL;
    fbe_payload_control_buffer_length_t trace_length;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_service_trace(base_service,
							FBE_TRACE_LEVEL_DEBUG_HIGH, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s: entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &base_service_control_set_trace_level);
	if(base_service_control_set_trace_level == NULL){
		fbe_base_service_trace(base_service, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s base_service_set_trace_level failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer_length(control_operation, &trace_length);
    if (trace_length != sizeof(fbe_base_service_control_set_trace_level_t)) {
		fbe_base_service_trace(base_service, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s base_service_set_trace_level length mismatch", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_base_service_set_trace_level(base_service, base_service_control_set_trace_level->trace_level );
	fbe_base_service_trace(base_service,
							FBE_TRACE_LEVEL_INFO, 
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: trace level = %d \n", __FUNCTION__, base_service_control_set_trace_level->trace_level);

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
static fbe_status_t 
fbe_base_service_control_get_trace_level(fbe_base_service_t * base_service, fbe_packet_t * packet)
{
	fbe_trace_level_t * trace_level = NULL;
    fbe_payload_control_buffer_length_t trace_length;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_service_trace(base_service,
							FBE_TRACE_LEVEL_DEBUG_HIGH, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s: entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &trace_level);
	if(trace_level == NULL){
		fbe_base_service_trace(base_service, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s base_service_get_trace_level failed", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer_length(control_operation, &trace_length);
    if (trace_length != sizeof(fbe_trace_level_t)) {
		fbe_base_service_trace(base_service,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s base_service_get_trace_level length mismatch", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	*trace_level = fbe_base_service_get_trace_level(base_service);
    if (*trace_level == FBE_TRACE_LEVEL_INVALID)
    {
        *trace_level = fbe_trace_get_default_trace_level();
    }
	fbe_base_service_trace(base_service,
							FBE_TRACE_LEVEL_INFO, 
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s: trace level = %d \n", __FUNCTION__, *trace_level);

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
	
