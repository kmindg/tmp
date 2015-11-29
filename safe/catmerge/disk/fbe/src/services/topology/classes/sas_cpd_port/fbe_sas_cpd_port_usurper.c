#include "fbe_base_discovered.h"
#include "sas_cpd_port_private.h"

/* Forward declaration */
static fbe_status_t sas_cpd_port_attach_edge(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t sas_cpd_port_detach_edge(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);

fbe_status_t 
fbe_sas_cpd_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_cpd_port = (fbe_sas_cpd_port_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace(	(fbe_base_object_t*)sas_cpd_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = sas_cpd_port_attach_edge( sas_cpd_port, packet);
			break;
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = sas_cpd_port_detach_edge( sas_cpd_port, packet);
			break;

        default:
			status = fbe_sas_port_control_entry(object_handle, packet);
			break;
	}
	return status;
}

static fbe_status_t 
sas_cpd_port_attach_edge(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_u32_t number_of_clients = 0;
	fbe_discovery_transport_control_attach_edge_t * discovery_transport_control_attach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_attach_edge);
	if(discovery_transport_control_attach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_discovery_transport_control_attach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* sas_cpd_port may have only one edge, so if we already have one - reject the request */
	status = fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)sas_cpd_port, &number_of_clients);
	if(number_of_clients != 0) {
		fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s (sas port suppors only one discovery edge)\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_discovering_attach_edge((fbe_base_discovering_t *) sas_cpd_port, discovery_transport_control_attach_edge->discovery_edge);
	fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) sas_cpd_port, 0 /* server_index */, 0 /* attr */);
	fbe_base_discovering_update_path_state((fbe_base_discovering_t *) sas_cpd_port);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_cpd_port_detach_edge(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t *)sas_cpd_port,
							FBE_TRACE_LEVEL_INFO /*FBE_TRACE_LEVEL_DEBUG_HIGH*/,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_detach_edge); 
	if(discovery_transport_control_detach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}


	status = fbe_base_discovering_detach_edge((fbe_base_discovering_t *) sas_cpd_port, discovery_transport_control_detach_edge->discovery_edge);

	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}

