#include "fbe/fbe_memory.h"
/*#include "fbe_base_discovering.h"*/
#include "fbe/fbe_winddk.h"
#include "fbe_scheduler.h"
#include "fbe_topology.h" 
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_notification.h"

#include "fbe_transport_memory.h"
#include "fbe_base_discovered.h"
#include "base_discovering_private.h"

/* Class methods forward declaration */
static fbe_status_t base_discovering_load(void);
static fbe_status_t base_discovering_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_base_discovering_class_methods = {FBE_CLASS_ID_BASE_DISCOVERING,
															base_discovering_load,
															base_discovering_unload,
															fbe_base_discovering_create_object,
															fbe_base_discovering_destroy_object,
															fbe_base_discovering_control_entry,
															fbe_base_discovering_event_entry,
															NULL,
															NULL};

/* Forward declaration */
static fbe_status_t base_discovering_instantiate_client_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t base_discovering_init(fbe_base_discovering_t * base_discovered);

static fbe_status_t 
base_discovering_load(void)
{
	KvTrace("%s entry\n", __FUNCTION__);

	return fbe_base_discovering_monitor_load_verify();
}

static fbe_status_t 
base_discovering_unload(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovering_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_base_discovering_t * base_discovering = NULL;
	fbe_status_t status;

	status = fbe_base_discovered_create_object(packet, object_handle);
	if (status != FBE_STATUS_OK) {
		return status;
	}

	base_discovering = (fbe_base_discovering_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) base_discovering, FBE_CLASS_ID_BASE_DISCOVERING);	

	fbe_base_object_trace(	(fbe_base_object_t*)base_discovering,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry.", __FUNCTION__);

	base_discovering_init(base_discovering);

	return status;
}

/* This function may be called from the constractor or as a result of some sort of init condition */
static fbe_status_t 
base_discovering_init(fbe_base_discovering_t * base_discovering)
{
	/* Discovery transport server initialization */
	fbe_discovery_transport_server_init(&base_discovering->discovery_transport_server);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovering_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_base_discovering_t * base_discovering = NULL;
	fbe_status_t status;
	base_discovering = (fbe_base_discovering_t *)fbe_base_handle_to_pointer(object_handle);

	/* Check edges */

	/* Cleanup */
	fbe_discovery_transport_server_destroy(&base_discovering->discovery_transport_server);

	status = fbe_base_discovered_destroy_object(object_handle);
	return status;
}

fbe_status_t 
fbe_base_discovering_attach_edge(fbe_base_discovering_t * base_discovering, fbe_discovery_edge_t * client_edge)
{
	fbe_discovery_transport_server_lock(&base_discovering->discovery_transport_server);

	fbe_discovery_transport_server_attach_edge(&base_discovering->discovery_transport_server, 
												client_edge,
												&fbe_base_discovering_lifecycle_const,
												(fbe_base_object_t *)base_discovering);

	fbe_discovery_transport_server_unlock(&base_discovering->discovery_transport_server);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovering_detach_edge(fbe_base_discovering_t * base_discovering, fbe_discovery_edge_t * client_edge)
{
	fbe_status_t status;
	fbe_u32_t number_of_clients;

	fbe_base_object_trace(	(fbe_base_object_t *)base_discovering,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	fbe_discovery_transport_server_lock(&base_discovering->discovery_transport_server);
	fbe_discovery_transport_server_detach_edge(&base_discovering->discovery_transport_server, client_edge);
	fbe_discovery_transport_server_unlock(&base_discovering->discovery_transport_server);

	fbe_base_discovering_get_number_of_clients(base_discovering, &number_of_clients);

	/* It is possible that we waiting for discovery server to become empty.
		It whould be nice if we can reschedule monitor when we have no clients any more 
	*/
	fbe_base_object_trace(	(fbe_base_object_t *)base_discovering,
							FBE_TRACE_LEVEL_DEBUG_HIGH /*FBE_TRACE_LEVEL_DEBUG_HIGH*/,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

	if(number_of_clients == 0){
		status = fbe_lifecycle_reschedule(&fbe_base_discovering_lifecycle_const,
										(fbe_base_object_t *) base_discovering,
										(fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */
	}
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovering_instantiate_client(fbe_base_discovering_t * base_discovering, 
										fbe_class_id_t parent_class_id, 
										fbe_edge_index_t  server_index)
{
	fbe_packet_t * packet;
	fbe_base_object_mgmt_create_object_t  base_object_mgmt_create_object;
	fbe_discovery_transport_control_create_edge_t discovery_transport_control_create_edge;
	fbe_status_t status;
	fbe_semaphore_t sem;
	fbe_object_id_t my_object_id;

    fbe_semaphore_init(&sem, 0, 1);

	/* This is temporary approach */
	fbe_base_object_trace(	(fbe_base_object_t*)base_discovering,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	/* Allocate packet */
	packet = fbe_transport_allocate_packet();
	if(packet == NULL) {
		KvTrace("%s fbe_transport_allocate_packet fail\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_initialize_packet(packet);
	fbe_base_object_mgmt_create_object_init(&base_object_mgmt_create_object, parent_class_id, FBE_OBJECT_ID_INVALID);

	fbe_transport_build_control_packet(packet, 
								FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT,
								&base_object_mgmt_create_object,
								sizeof(fbe_base_object_mgmt_create_object_t),
								sizeof(fbe_base_object_mgmt_create_object_t),
								base_discovering_instantiate_client_completion,
								&sem);
	/* Set packet address */
	fbe_transport_set_address(	packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID);

	status = fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

	/* Reuse packet */
	fbe_transport_reuse_packet(packet);

	fbe_base_object_get_object_id((fbe_base_object_t *)base_discovering, &my_object_id);

	discovery_transport_control_create_edge.server_id = my_object_id;
	discovery_transport_control_create_edge.server_index = server_index;

	fbe_transport_build_control_packet(packet, 
								FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE,
								&discovery_transport_control_create_edge,
								sizeof(fbe_discovery_transport_control_create_edge_t),
								sizeof(fbe_discovery_transport_control_create_edge_t),
								base_discovering_instantiate_client_completion,
								&sem);

	/* Set packet address */
	fbe_transport_set_address(	packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								base_object_mgmt_create_object.object_id);

	/* fbe_transport_send_packet(base_object_mgmt_create_object->object_id, packet); */
	fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	fbe_transport_release_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
base_discovering_instantiate_client_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
	fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovering_get_number_of_clients(fbe_base_discovering_t * base_discovering, fbe_u32_t * number_of_clients)
{
	fbe_status_t status;
	status = fbe_discovery_transport_server_get_number_of_clients(&base_discovering->discovery_transport_server, number_of_clients);
	return status;
}

fbe_status_t
fbe_base_discovering_get_path_attr(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_attr_t * path_attr)
{
	fbe_status_t status;
	fbe_path_state_t path_state;
	fbe_discovery_edge_t  * discovery_edge = NULL;
	
	fbe_base_discovering_transport_server_lock(base_discovering);
	discovery_edge = fbe_base_discovering_get_client_edge_by_server_index(base_discovering, server_index);

	if (discovery_edge == NULL)
	{
        fbe_base_discovering_transport_server_unlock(base_discovering);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_discovery_transport_get_path_state(discovery_edge, &path_state);

	if(path_state != FBE_PATH_STATE_ENABLED){
        fbe_base_discovering_transport_server_unlock(base_discovering);
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}
	
	status = fbe_discovery_transport_get_path_attributes(discovery_edge, path_attr);
	fbe_base_discovering_transport_server_unlock(base_discovering);
	return status;
}

fbe_status_t
fbe_base_discovering_set_path_attr(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_discovery_transport_server_set_path_attr(&base_discovering->discovery_transport_server, server_index, path_attr);
	return status;
}

fbe_status_t
fbe_base_discovering_clear_path_attr(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_attr_t path_attr)
{
	fbe_status_t status;
	status = fbe_discovery_transport_server_clear_path_attr(&base_discovering->discovery_transport_server, server_index, path_attr);
	return status;
}

/* This function is deprecated */
#if 0
fbe_status_t
fbe_base_discovering_set_path_state(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index, fbe_path_state_t path_state)
{
	fbe_status_t status;
	status = fbe_discovery_transport_server_set_edge_path_state(&base_discovering->discovery_transport_server, server_index, path_state);
	return status;
}
#endif 

fbe_status_t
fbe_base_discovering_update_path_state(fbe_base_discovering_t * base_discovering)
{
	fbe_status_t status;
	fbe_path_state_t path_state;
	/* path state should be calculated based on lifecycle state and some other staff*/
	path_state = FBE_PATH_STATE_ENABLED;
	status = fbe_discovery_transport_server_set_path_state(&base_discovering->discovery_transport_server, path_state);
	return status;
}

fbe_status_t 
fbe_base_discovering_transport_server_lock(fbe_base_discovering_t * base_discovering)
{
	fbe_discovery_transport_server_lock(&base_discovering->discovery_transport_server);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovering_transport_server_unlock(fbe_base_discovering_t * base_discovering)
{
	fbe_discovery_transport_server_unlock(&base_discovering->discovery_transport_server);
	return FBE_STATUS_OK;
}

fbe_discovery_edge_t *
fbe_base_discovering_get_client_edge_by_client_id(fbe_base_discovering_t * base_discovering, fbe_object_id_t client_id)
{
	return fbe_discovery_transport_server_get_client_edge_by_client_id(&base_discovering->discovery_transport_server, client_id);	
}

fbe_discovery_edge_t *
fbe_base_discovering_get_client_edge_by_server_index(fbe_base_discovering_t * base_discovering, fbe_edge_index_t server_index)
{
	return fbe_discovery_transport_server_get_client_edge_by_server_index(&base_discovering->discovery_transport_server, server_index);	
}

fbe_status_t 
fbe_base_discovering_get_server_index_by_client_id(fbe_base_discovering_t * base_discovering,
												   fbe_object_id_t client_id,
												   fbe_edge_index_t * server_index)
{
	fbe_status_t status;
	fbe_discovery_edge_t * discovery_edge;

	fbe_base_discovering_transport_server_lock(base_discovering);
	discovery_edge = fbe_base_discovering_get_client_edge_by_client_id(base_discovering, client_id);
	if(discovery_edge == NULL){
		fbe_base_discovering_transport_server_unlock(base_discovering);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_discovery_transport_get_server_index(discovery_edge, server_index);
	fbe_base_discovering_transport_server_unlock(base_discovering);
	return status;
}

fbe_status_t
fbe_base_discovering_discovery_bouncer_entry(fbe_base_discovering_t * base_discovering,
												fbe_transport_entry_t transport_entry,
												fbe_packet_t * packet)
{
	/* We have to evaluate our state */
	/* We have to evaluate the state of the edge */	
	/* We have to evaluate the number of outstanding I/O's to maintain the queue depth */	

	/* We have to increment the number of outstanding I/O's */
	/* Finally, we can send the packet to the executer of the specific object */
	transport_entry(base_discovering, packet);
	return FBE_STATUS_OK;
}
