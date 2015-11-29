#include "dell_board_private.h"

/* Forward declarations */
static fbe_status_t dell_board_attach_edge(fbe_dell_board_t * dell_board, fbe_packet_t * packet);
static fbe_status_t dell_board_detach_edge(fbe_dell_board_t * dell_board, fbe_packet_t * packet);


fbe_status_t 
fbe_dell_board_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_control_operation_opcode_t control_code;
	fbe_dell_board_t * dell_board = NULL;

	dell_board = (fbe_dell_board_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace((fbe_base_object_t*)dell_board, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {		
/*
		case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_NUMBER:
			status = fbe_dell_board_get_io_port_number(dell_board, packet);
			break;
        case FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_DRIVER:
            status = fbe_dell_board_get_port_driver(dell_board, packet);
            break;
*/
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = dell_board_attach_edge( dell_board, packet);
			break;
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = dell_board_detach_edge( dell_board, packet);
			break;

        default:
			status = fbe_base_board_control_entry(object_handle, packet);
	}
	return status;
}

static fbe_status_t 
dell_board_attach_edge(fbe_dell_board_t * dell_board, fbe_packet_t * packet)
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
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len);
	if(len != sizeof(fbe_discovery_transport_control_attach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
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

	/* Dell board may have only one edge, so if we already have one - reject the request */
	status = fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)dell_board, &number_of_clients);
	if(number_of_clients != 0) {
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s (Dell board support only one client)\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_discovering_attach_edge((fbe_base_discovering_t *) dell_board, discovery_transport_control_attach_edge->discovery_edge);
	fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) dell_board, 0 /* server_index */, 0 /* attr */);
	fbe_base_discovering_update_path_state((fbe_base_discovering_t *) dell_board);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
dell_board_detach_edge(fbe_dell_board_t * dell_board, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 

	fbe_base_object_trace(	(fbe_base_object_t *)dell_board,
							FBE_TRACE_LEVEL_INFO /*FBE_TRACE_LEVEL_DEBUG_HIGH*/,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

	fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_detach_edge);
	if(discovery_transport_control_detach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}


	status = fbe_base_discovering_detach_edge((fbe_base_discovering_t *) dell_board, discovery_transport_control_detach_edge->discovery_edge);

	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}
