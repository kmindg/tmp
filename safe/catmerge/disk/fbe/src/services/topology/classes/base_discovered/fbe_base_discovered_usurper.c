#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"

#include "fbe_base_discovered.h"
#include "base_discovered_private.h"

/* Forward declaration */
static fbe_status_t base_discovered_create_edge(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);
static fbe_status_t base_discovered_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_discovered_get_edge_info(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);
static fbe_status_t base_discovered_get_death_reason (fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);

fbe_status_t 
fbe_base_discovered_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_control_operation_opcode_t control_code;
	fbe_base_discovered_t * base_discovered = NULL;

	base_discovered = (fbe_base_discovered_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE:
			status = base_discovered_create_edge( base_discovered, packet);		
			break;
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
			status = base_discovered_get_edge_info( base_discovered, packet);
			break;
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DEATH_REASON:
			status = base_discovered_get_death_reason( base_discovered, packet);
			break;
		default:
			status = fbe_base_object_control_entry(object_handle, packet);
			if(status == FBE_STATUS_TRAVERSE) {
				status = fbe_base_discovered_send_control_packet(base_discovered, packet);
			}
			break;
	}

	return status;
}

/* Sends attach command to the server object */
static fbe_status_t 
base_discovered_create_edge(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_discovery_transport_control_create_edge_t * discovered_control_create_edge = NULL; /* INPUT */
	fbe_discovery_transport_control_attach_edge_t discovered_control_attach_edge; /* sent to the server object */
	fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * new_payload = NULL;
    fbe_payload_control_operation_t * new_control_operation = NULL;

	fbe_path_state_t path_state;
	fbe_object_id_t my_object_id;

	fbe_status_t status;
	fbe_semaphore_t sem;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &discovered_control_create_edge);
	if(discovered_control_create_edge == NULL) {
		fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Build the edge. ( We have to perform some sanity checking here) */ 
	fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &path_state);
	if(path_state != FBE_PATH_STATE_INVALID) { /* discovery edge may be connected only once */
		fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
								"%s: path_state %X\n",__FUNCTION__, path_state);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_discovery_transport_set_server_id(&base_discovered->discovery_edge, discovered_control_create_edge->server_id);
	fbe_discovery_transport_set_server_index(&base_discovered->discovery_edge, discovered_control_create_edge->server_index);
	fbe_discovery_transport_set_transport_id(&base_discovered->discovery_edge);

	fbe_base_object_get_object_id((fbe_base_object_t *)base_discovered, &my_object_id);

	fbe_discovery_transport_set_client_id(&base_discovered->discovery_edge, my_object_id);
	fbe_discovery_transport_set_client_index(&base_discovered->discovery_edge, 0); /* We have only one discovery edge */

	discovered_control_attach_edge.discovery_edge = &base_discovered->discovery_edge;

	/* Insert the edge. */
	fbe_semaphore_init(&sem, 0, 1);
	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if(new_packet == NULL) {
		fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: fbe_transport_allocate_packet failed\n",__FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_initialize_packet(new_packet);

    new_payload = fbe_transport_get_payload_ex(new_packet);
    new_control_operation = fbe_payload_ex_allocate_control_operation(new_payload);

    fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,  // opcode
                                    &discovered_control_attach_edge, // buffer
                                    sizeof(fbe_discovery_transport_control_attach_edge_t));   // buffer_length 

	status = fbe_transport_set_completion_function(new_packet, base_discovered_create_edge_completion, &sem);

	/* We are sending control packet, so we have to form address manually */
	/* Set packet address */
	fbe_transport_set_address(	new_packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								discovered_control_create_edge->server_id);

	/* Control packets should be sent via service manager */
	status = fbe_service_manager_send_control_packet(new_packet);

	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);


	status = fbe_transport_get_status_code(new_packet);
	if(status != FBE_STATUS_OK){ 
		fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: base_discovered_create_edge failed status %X\n",__FUNCTION__, status);
	}

    fbe_transport_copy_packet_status(new_packet, packet);
	fbe_payload_ex_release_control_operation(new_payload, new_control_operation);
	fbe_transport_release_packet(new_packet);

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s calls fbe_lifecycle_reschedule\n", __FUNCTION__);
	
	status = fbe_lifecycle_reschedule(&fbe_base_discovered_lifecycle_const,
                                      (fbe_base_object_t *) base_discovered,
                                      (fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
base_discovered_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

static fbe_status_t 
base_discovered_get_edge_info(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_discovery_transport_control_get_edge_info_t * get_edge_info = NULL; /* INPUT */
	fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &get_edge_info);
	if(get_edge_info == NULL) {
		fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer\n");
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_discovery_transport_get_client_id(&base_discovered->discovery_edge, &get_edge_info->base_edge_info.client_id);
	status = fbe_discovery_transport_get_server_id(&base_discovered->discovery_edge, &get_edge_info->base_edge_info.server_id);

	status = fbe_discovery_transport_get_client_index(&base_discovered->discovery_edge, &get_edge_info->base_edge_info.client_index);
	status = fbe_discovery_transport_get_server_index(&base_discovered->discovery_edge, &get_edge_info->base_edge_info.server_index);

	status = fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &get_edge_info->base_edge_info.path_state);
	status = fbe_discovery_transport_get_path_attributes(&base_discovered->discovery_edge, &get_edge_info->base_edge_info.path_attr);

	get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_DISCOVERY;

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}


static fbe_status_t base_discovered_get_death_reason (fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_discovery_transport_control_get_death_reason_t * get_death_reason = NULL; /* INPUT */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &get_death_reason);
	if(get_death_reason == NULL) {
		fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer\n");
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	get_death_reason->death_reason = base_discovered->death_reason;

	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
