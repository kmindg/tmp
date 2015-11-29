#include "fbe_base_transport.h"
#include "fbe_lifecycle.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"


fbe_lifecycle_status_t
fbe_base_transport_server_pending(fbe_base_transport_server_t * base_transport_server,
									   fbe_lifecycle_const_t * p_class_const,
									   struct fbe_base_object_s * base_object)
{
	fbe_status_t status;
	fbe_lifecycle_state_t lifecycle_state;
	fbe_lifecycle_status_t lifecycle_status;
	fbe_u32_t number_of_clients;

	status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
	if(status != FBE_STATUS_OK){
		/* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
		/* We have no error code to return */
	}

	lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;

	fbe_base_transport_server_update_path_state(base_transport_server, p_class_const, base_object);

	if(lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY){
		/* we need to make shure that all edges are detached */
		fbe_base_transport_server_get_number_of_clients(base_transport_server, &number_of_clients);
		if(number_of_clients != 0){
			lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
		}
	}

	return lifecycle_status;
}

/* Caller must take the server lock */
fbe_status_t
fbe_base_transport_server_get_edge(fbe_base_transport_server_t * base_transport_server, 
								   fbe_edge_index_t server_index, 
fbe_base_edge_t ** client_edge)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge;

	*client_edge = NULL;

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->server_index == server_index){ /* We found it */
			*client_edge = base_edge;
			return FBE_STATUS_OK;			
		}
		i++;
		if(i > 0x0000FFFF){
			/* KvTrace("%s Critical error edge not found\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	return FBE_STATUS_GENERIC_FAILURE;
}

/* Caller must take the server lock */
fbe_status_t
fbe_base_transport_server_detach_edge(fbe_base_transport_server_t * base_transport_server, fbe_base_edge_t * client_edge)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	if(client_edge == base_transport_server->client_list){ /* The first edge on the list */
		base_transport_server->client_list = client_edge->next_edge;
		client_edge->next_edge = NULL;
		client_edge->path_state = FBE_PATH_STATE_INVALID;
		return FBE_STATUS_OK;
	}

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->next_edge == client_edge){ /* We found it */
			base_edge->next_edge = client_edge->next_edge;
			client_edge->path_state = FBE_PATH_STATE_INVALID;
			break;
		}
		i++;
		if(i > 0x0000FFFF){
			/* KvTrace("%s Critical error edge not found\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_server_get_number_of_clients(fbe_base_transport_server_t * base_transport_server, fbe_u32_t * number_of_clients)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	*number_of_clients = 0;

	fbe_base_transport_server_lock(base_transport_server);

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		i++;
		if(i > 0x0000FFFF){
			fbe_base_transport_server_unlock(base_transport_server);
			/*KvTrace("%s Critical error looped client list\n", __FUNCTION__);*/
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	fbe_base_transport_server_unlock(base_transport_server);

	*number_of_clients = i;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_server_set_path_attr(fbe_base_transport_server_t * base_transport_server,
											 fbe_edge_index_t  server_index, 
											 fbe_path_attr_t path_attr)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	fbe_base_transport_server_lock(base_transport_server);

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->server_index == server_index){
			base_edge->path_attr |= path_attr;
			/* We should send event here */
			fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, base_edge);
			break;
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_base_transport_server_unlock(base_transport_server);
			/* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	fbe_base_transport_server_unlock(base_transport_server);

	if(base_edge == NULL){ /*We did not found the edge */
		/* KvTrace("%s Critical error edge not found index = %d\n", __FUNCTION__, server_index); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_transport_server_clear_path_attr(fbe_base_transport_server_t * base_transport_server,
											fbe_edge_index_t  server_index, 
											fbe_path_attr_t path_attr)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	fbe_base_transport_server_lock(base_transport_server);

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
           (base_edge->server_index == server_index)){
			base_edge->path_attr &= ~path_attr;
			/* We should send event here */
            /* CGCG should we get rid of this?? not in armada_mcr_fb */
            fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, base_edge);
			break;
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_base_transport_server_unlock(base_transport_server);
			/* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	fbe_base_transport_server_unlock(base_transport_server);

	if(base_edge == NULL){ /*We did not found the edge */
		/* KvTrace("%s Critical error edge not found index %d\n", __FUNCTION__, server_index); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_server_set_edge_path_state(fbe_base_transport_server_t * base_transport_server,
											  fbe_edge_index_t  server_index,
											  fbe_path_state_t path_state)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	fbe_base_transport_server_lock(base_transport_server);

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->server_index == server_index){
			base_edge->path_state = path_state;

			/* We should send event here */
			fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, base_edge);
			break;
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_base_transport_server_unlock(base_transport_server);
			/* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	fbe_base_transport_server_unlock(base_transport_server);

	if(base_edge == NULL){ /*We did not found the edge */
		/* KvTrace("%s Critical error edge not found index = %d\n", __FUNCTION__, server_index); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_transport_server_set_path_state(fbe_base_transport_server_t * base_transport_server, fbe_path_state_t path_state)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	fbe_base_transport_server_lock(base_transport_server);

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->path_state != path_state){
			base_edge->path_state = path_state;
			/* We should send event here */
			fbe_topology_send_event(base_edge->client_id,FBE_EVENT_TYPE_EDGE_STATE_CHANGE, base_edge);
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_base_transport_server_unlock(base_transport_server);
			/* KvTrace("%s Critical error edge not found\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	fbe_base_transport_server_unlock(base_transport_server);

	return FBE_STATUS_OK;
}

/* Caller MUST hold the lock */
fbe_base_edge_t *
fbe_base_transport_server_get_client_edge_by_client_id(fbe_base_transport_server_t * base_transport_server, fbe_object_id_t object_id)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->client_id == object_id){
			break;
		}
		i++;
		if(i > 0x0000FFFF){
			/* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return NULL;
		}
		base_edge = base_edge->next_edge;
	}

	return base_edge;
}

fbe_base_edge_t *
fbe_base_transport_server_get_client_edge_by_server_index(fbe_base_transport_server_t * base_transport_server, fbe_edge_index_t server_index)
{
	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;

	base_edge = base_transport_server->client_list;
	while(base_edge != NULL) {
		if(base_edge->server_index == server_index){
			break;
		}
		i++;
		if(i > 0x0000FFFF){
			/* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

			return NULL;
		}
		base_edge = base_edge->next_edge;
	}

	return base_edge;
}

fbe_status_t
fbe_base_transport_server_lifecycle_to_path_state(fbe_lifecycle_state_t lifecycle_state, fbe_path_state_t *	path_state)
{
	switch(lifecycle_state){
		case FBE_LIFECYCLE_STATE_READY:
		case FBE_LIFECYCLE_STATE_PENDING_READY:
			*	path_state =  FBE_PATH_STATE_ENABLED;
			break;
		case FBE_LIFECYCLE_STATE_ACTIVATE:
		case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
			*	path_state =  FBE_PATH_STATE_DISABLED;
			break;
		case FBE_LIFECYCLE_STATE_SPECIALIZE:
			*	path_state =  FBE_PATH_STATE_DISABLED;
			break;
		case FBE_LIFECYCLE_STATE_HIBERNATE:
		case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
			*	path_state =  FBE_PATH_STATE_SLUMBER;
			break;
		case FBE_LIFECYCLE_STATE_OFFLINE:
		case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
			*	path_state =  FBE_PATH_STATE_DISABLED;
			break;
		case FBE_LIFECYCLE_STATE_FAIL:
		case FBE_LIFECYCLE_STATE_PENDING_FAIL:
			*	path_state =  FBE_PATH_STATE_BROKEN;
			break;
		case FBE_LIFECYCLE_STATE_DESTROY:
		case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
			*	path_state =  FBE_PATH_STATE_GONE;
			break;
		default :
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error Uknown lifecycle_state = %X \n", __FUNCTION__, lifecycle_state);

			return FBE_STATUS_GENERIC_FAILURE;
			break;
	}

	return FBE_STATUS_OK;
}

/* Caller must take the server lock */
fbe_status_t
fbe_base_transport_server_attach_edge(fbe_base_transport_server_t * base_transport_server, 
									  fbe_base_edge_t * base_edge,
									  fbe_lifecycle_const_t * p_class_const,
									  struct fbe_base_object_s * base_object)
{
	fbe_lifecycle_state_t lifecycle_state;
	fbe_status_t status;

	status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
	if(status != FBE_STATUS_OK){
		/* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);

		return FBE_STATUS_GENERIC_FAILURE;
	}

	base_edge->next_edge = base_transport_server->client_list;
	base_transport_server->client_list = base_edge;

	/* We need to set the valid path state */
	status = fbe_base_transport_server_lifecycle_to_path_state(lifecycle_state, &base_edge->path_state);

	return status;
}


fbe_status_t
fbe_base_transport_send_control_packet(fbe_base_edge_t * base_edge, fbe_packet_t * packet)
{
	fbe_object_id_t server_id;
	fbe_status_t status;

	/* Set packet address */
	fbe_base_transport_get_server_id(base_edge, &server_id);

	fbe_transport_set_address(	packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								server_id);

	/* We also should attach edge to the packet */
	packet->base_edge = (fbe_base_edge_t *)base_edge;

	/* Control packets should be sent via service manager */
	status = fbe_service_manager_send_control_packet(packet);
	return status;
}

fbe_status_t
fbe_base_transport_send_functional_packet(fbe_base_edge_t * base_edge, fbe_packet_t * packet, fbe_package_id_t package_id)
{
	fbe_object_id_t server_id;
	fbe_status_t status;
	fbe_path_state_t path_state;
	fbe_edge_hook_function_t hook_function;

	/* It would be nice to check edge state here(we allow sending packets to a sleeping edge, it will wake him up) */
	fbe_base_transport_get_path_state(base_edge, &path_state);
	if((path_state != FBE_PATH_STATE_ENABLED) && (path_state != FBE_PATH_STATE_SLUMBER)){
		fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, 0);
		fbe_transport_complete_packet_async(packet);
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

	/* Set packet address */
	fbe_base_transport_get_server_id(base_edge, &server_id);

	fbe_transport_set_address(	packet,
								package_id,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								server_id);

	/* We also should attach edge to the packet */
	packet->base_edge = (fbe_base_edge_t *)base_edge;

	/* Check if the hook is set. */ 
	status = fbe_base_transport_get_hook_function (base_edge, &hook_function);
	if(hook_function != NULL) {
		/* The hook is set, redirect the packet to hook. */
		/* It is up to the hook to decide on how to complete the packet*/
		/* TODO: change the trace level to debug, once we are done debugging*/
		fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s redirect packet with srv id %d to edge hook function.\n", __FUNCTION__, server_id);
        status = hook_function(packet);
		if(status != FBE_STATUS_CONTINUE){
			return status;
		}
	}

	/* No hook, continue with normal operation */
	status = fbe_topology_send_io_packet(packet);
	return status;
}

fbe_status_t
fbe_base_transport_server_update_path_state(fbe_base_transport_server_t * base_transport_server,
											fbe_lifecycle_const_t * p_class_const,
											struct fbe_base_object_s * base_object)
{
	fbe_status_t status;
	fbe_lifecycle_state_t lifecycle_state;

	status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
	if(status != FBE_STATUS_OK){
		/* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
		/* We have no error code to return */
	}

	switch(lifecycle_state){
		case FBE_LIFECYCLE_STATE_PENDING_READY:
			fbe_base_transport_server_set_path_state(base_transport_server, FBE_PATH_STATE_ENABLED);
			break;
		case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
			fbe_base_transport_server_set_path_state(base_transport_server, FBE_PATH_STATE_DISABLED);
			break;
		case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
			fbe_base_transport_server_set_path_state(base_transport_server, FBE_PATH_STATE_SLUMBER);
			break;
		case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
			fbe_base_transport_server_set_path_state(base_transport_server, FBE_PATH_STATE_DISABLED);
			break;
		case FBE_LIFECYCLE_STATE_PENDING_FAIL:
			fbe_base_transport_server_set_path_state(base_transport_server, FBE_PATH_STATE_BROKEN);
			break;
		case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
			fbe_base_transport_server_set_path_state(base_transport_server, FBE_PATH_STATE_GONE);
			break;
		default :
			/* KvTrace("%s Critical error Uknown lifecycle_state = %X \n", __FUNCTION__, lifecycle_state); */
			fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error Uknown lifecycle_state = %X \n", __FUNCTION__, lifecycle_state);

			break;
	}

	return FBE_STATUS_OK;
}


fbe_status_t
fbe_base_transport_send_functional_packet_directly(fbe_base_edge_t * base_edge, 
												   fbe_packet_t * packet, 
												   fbe_package_id_t package_id,
												   fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet),
												   fbe_object_handle_t object_handle)
{
	fbe_object_id_t server_id;
	fbe_status_t status;
	fbe_path_state_t path_state;
	fbe_edge_hook_function_t hook_function;

	/* It would be nice to check edge state here(we allow sending packets to a sleeping edge, it will wake him up) */
	fbe_base_transport_get_path_state(base_edge, &path_state);
	if((path_state != FBE_PATH_STATE_ENABLED) && (path_state != FBE_PATH_STATE_SLUMBER)){
		fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, 0);
		fbe_transport_complete_packet_async(packet);
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

	/* Set packet address */
	fbe_base_transport_get_server_id(base_edge, &server_id);

	fbe_transport_set_address(	packet,
								package_id,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								server_id);

	/* We also should attach edge to the packet */
	packet->base_edge = (fbe_base_edge_t *)base_edge;

	/* Check if the hook is set. */ 
	status = fbe_base_transport_get_hook_function (base_edge, &hook_function);
	if(hook_function != NULL) {
		/* The hook is set, redirect the packet to hook. */
		/* It is up to the hook to decide on how to complete the packet*/
		/* TODO: change the trace level to debug, once we are done debugging*/
		fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s redirect packet with srv id %d to edge hook function.\n", __FUNCTION__, server_id);
        status = hook_function(packet);
		if(status != FBE_STATUS_CONTINUE){
			return status;
		}
	}

	/* No hook, continue with normal operation */
	status = io_entry(object_handle, packet);
	return status;
}
