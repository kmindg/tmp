#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"

#include "fbe_base_discovered.h"
#include "base_discovered_private.h"

/* Class methods forward declaration */
static fbe_status_t base_discovered_load(void);
static fbe_status_t base_discovered_unload(void);
 
/* Export class methods  */
fbe_class_methods_t fbe_base_discovered_class_methods = {FBE_CLASS_ID_BASE_DISCOVERED,
													base_discovered_load,
													base_discovered_unload,
													fbe_base_discovered_create_object,
													fbe_base_discovered_destroy_object,
													fbe_base_discovered_control_entry,
													fbe_base_discovered_event_entry,
													NULL,
													NULL};

/* Forward declaration */
static fbe_status_t base_discovered_init(fbe_base_discovered_t * base_discovered);
static fbe_status_t base_discovered_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t 
base_discovered_load(void)
{
	KvTrace("%s entry\n", __FUNCTION__);

	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_base_discovered_t) < FBE_MEMORY_CHUNK_SIZE);

	return fbe_base_discovered_monitor_load_verify();
}

static fbe_status_t 
base_discovered_unload(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovered_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_base_discovered_t * base_discovered = NULL;
	fbe_status_t status;

	status = fbe_base_object_create_object(packet, object_handle);
	if (status != FBE_STATUS_OK) {
		return status;
	}

	base_discovered = (fbe_base_discovered_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) base_discovered, FBE_CLASS_ID_BASE_DISCOVERED);	

	fbe_base_object_trace(	(fbe_base_object_t*)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry.\n", __FUNCTION__);

	base_discovered_init(base_discovered);

	return status;
}

/* This function may be called from the constractor or as a result of some sort of init condition */
static fbe_status_t 
base_discovered_init(fbe_base_discovered_t * base_discovered)
{	
	base_discovered->death_reason = FBE_DEATH_REASON_INVALID;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovered_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;

	/* Check edges */

	/* Cleanup */

	status = fbe_base_object_destroy_object(object_handle);
	return status;
}


#if 0
fbe_status_t 
fbe_base_discovered_send_packet(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_status_t status;

	status = fbe_discovery_transport_send_packet(&base_discovered->discovery_edge, packet);

	return status;
}
#endif 

fbe_status_t 
fbe_base_discovered_send_control_packet(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_status_t status;

	status = fbe_discovery_transport_send_control_packet(&base_discovered->discovery_edge, packet);

	return status;
}

fbe_status_t 
fbe_base_discovered_send_functional_packet(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_status_t status;

	status = fbe_discovery_transport_send_functional_packet(&base_discovered->discovery_edge, packet);

	return status;
}

fbe_status_t
fbe_base_discovered_get_port_object_id(fbe_base_discovered_t * base_discovered, fbe_object_id_t * port_object_id)
{

	*port_object_id = base_discovered->port_object_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovered_detach_edge(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_discovery_transport_control_detach_edge_t discovered_control_detach_edge; /* sent to the server object */	
	fbe_payload_ex_t * payload = NULL;
	fbe_path_state_t path_state;
	fbe_payload_control_operation_t * control_operation = NULL;

	fbe_status_t status;
	fbe_semaphore_t sem;

	fbe_base_object_trace(	(fbe_base_object_t *)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	status = fbe_base_discovered_get_path_state(base_discovered, &path_state);
	/* If we don't have an edge to detach, complete the packet and return Good status. */
	if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_INVALID)
	{
		fbe_base_object_trace((fbe_base_object_t*)base_discovered,
							FBE_TRACE_LEVEL_DEBUG_LOW,
							FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
							"%s Discovery edge is Invalid. No need to detach it.\n", __FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	discovered_control_detach_edge.discovery_edge = &base_discovered->discovery_edge;

	/* Insert the edge. */
	fbe_semaphore_init(&sem, 0, 1);

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_allocate_control_operation(payload); 
	if(control_operation == NULL) {    
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to allocate control operation\n",__FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;    
    }


    fbe_payload_control_build_operation(control_operation,  // control operation
                                    FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE,  // opcode
                                    &discovered_control_detach_edge, // buffer
                                    sizeof(fbe_discovery_transport_control_detach_edge_t));   // buffer_length 

	status = fbe_transport_set_completion_function(packet, base_discovered_detach_edge_completion, &sem);

	fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_DESTROY_ENABLED);
	
	fbe_base_discovered_send_control_packet(base_discovered, packet);

	fbe_semaphore_wait(&sem, NULL); /* We have to wait because discovered_control_detach_edge is on the stack */
    fbe_semaphore_destroy(&sem);

	return FBE_STATUS_OK;
}

static fbe_status_t 
base_discovered_detach_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload); 
	fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_semaphore_release(sem, 0, 1, FALSE);
	
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_discovered_get_path_attributes(fbe_base_discovered_t * base_discovered, fbe_path_attr_t * path_attr)
{
	fbe_path_state_t path_state;
	fbe_status_t status;

	status = fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &path_state);

	if (status != FBE_STATUS_OK)
	{
		return status;
	}

	if(path_state != FBE_PATH_STATE_ENABLED){
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

	status = fbe_discovery_transport_get_path_attributes(&base_discovered->discovery_edge, path_attr);
	return status;
}

fbe_status_t
fbe_base_discovered_get_path_state(fbe_base_discovered_t * base_discovered, fbe_path_state_t *path_state)
{
	fbe_status_t status;

	status = fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, path_state);
	return status;
}

fbe_status_t
fbe_base_discovered_set_death_reason (fbe_base_discovered_t *base_discovered, fbe_object_death_reason_t death_reason)
{
	base_discovered->death_reason = death_reason;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_discovered_get_death_reason (fbe_base_discovered_t *base_discovered, fbe_object_death_reason_t *death_reason)
{
	*death_reason = base_discovered->death_reason;
	return FBE_STATUS_OK;
}
