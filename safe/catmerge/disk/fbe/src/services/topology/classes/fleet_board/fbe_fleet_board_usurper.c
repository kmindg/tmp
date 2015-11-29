#include "fbe/fbe_winddk.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_transport.h"
#include "fleet_board_private.h"

/* Forward declarations */
static fbe_status_t fleet_board_attach_edge(fbe_fleet_board_t * fleet_board, fbe_packet_t * packet);
static fbe_status_t fleet_board_detach_edge(fbe_fleet_board_t * fleet_board, fbe_packet_t * packet);
static fbe_status_t fleet_board_get_port_info(fbe_fleet_board_t * fleet_board, fbe_packet_t * packet);

fbe_status_t 
fbe_fleet_board_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_fleet_board_t * fleet_board = NULL;

    fleet_board = (fbe_fleet_board_t*)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)fleet_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {        
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = fleet_board_attach_edge(fleet_board, packet);
            break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = fleet_board_detach_edge(fleet_board, packet);
            break;
        case FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_INFO:
            status = fleet_board_get_port_info(fleet_board, packet);
            break;
        default:
            status = fbe_base_board_control_entry(object_handle, packet);
    }
    return status;
}

static fbe_status_t 
fleet_board_attach_edge(fbe_fleet_board_t * fleet_board, fbe_packet_t * packet)
{
    fbe_edge_index_t server_index;
    fbe_discovery_transport_control_attach_edge_t * discovery_transport_control_attach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_attach_edge);
    if (discovery_transport_control_attach_edge == NULL){
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_discovery_transport_control_attach_edge_t)){
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_discovery_transport_get_server_index(discovery_transport_control_attach_edge->discovery_edge, &server_index);
    if (server_index >= FBE_FLEET_BOARD_NUMBER_OF_IO_PORTS) {
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: server_index out of range, server_index: 0x%X\n", __FUNCTION__, server_index);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (fleet_board->io_port_array[(fbe_u32_t)server_index].io_port_number == FBE_BASE_BOARD_IO_PORT_INVALID) {
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: server_index maps to an invalid port, server_index: 0x%X\n",
                              __FUNCTION__, server_index);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_discovering_attach_edge((fbe_base_discovering_t*)fleet_board, discovery_transport_control_attach_edge->discovery_edge);
    fbe_base_discovering_set_path_attr((fbe_base_discovering_t*)fleet_board, server_index, 0 /* attr */);
    fbe_base_discovering_update_path_state((fbe_base_discovering_t*)fleet_board);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
fleet_board_detach_edge(fbe_fleet_board_t * fleet_board, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;    

    fbe_base_object_trace((fbe_base_object_t*)fleet_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_detach_edge);
    if (discovery_transport_control_detach_edge == NULL){
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_discovering_detach_edge((fbe_base_discovering_t*)fleet_board,
                                              discovery_transport_control_detach_edge->discovery_edge);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t
fleet_board_get_port_info(fbe_fleet_board_t * fleet_board, fbe_packet_t * packet)
{
    fbe_base_board_get_port_info_t * port_info;
    fbe_edge_index_t server_index;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &port_info); 
    if (port_info == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: null buffer pointer in get_port_info request\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    server_index = FBE_FLEET_BOARD_NUMBER_OF_IO_PORTS;
    fbe_transport_get_server_index(packet, &server_index);

    if (server_index >= FBE_FLEET_BOARD_NUMBER_OF_IO_PORTS) {
        fbe_base_object_trace((fbe_base_object_t*)fleet_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: server_index out of range, server_index: 0x%X\n",
                              __FUNCTION__, server_index);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_info->server_index = server_index;
    port_info->io_portal_number = (fbe_u32_t)fleet_board->io_port_array[server_index].io_portal_number;
    port_info->io_port_number = (fbe_u32_t)fleet_board->io_port_array[server_index].io_port_number;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
