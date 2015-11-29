
#include "sas_cpd_port_private.h"

/* Forward declaration */
static fbe_status_t sas_cpd_port_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_cpd_port_ssp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_status_t sas_cpd_port_discovery_transport_get_protocol_address(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t sas_cpd_port_discovery_transport_get_port_object_id(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t sas_cpd_port_discovery_transport_get_element_list(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);

fbe_status_t 
fbe_sas_cpd_port_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;
	fbe_transport_id_t transport_id;
	fbe_status_t status;

	sas_cpd_port = (fbe_sas_cpd_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_cpd_port,
						FBE_TRACE_LEVEL_DEBUG_HIGH,
						FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
						"%s entry .\n", __FUNCTION__);

	/* Fisrt we need to figure out to what transport this packet belong */
	fbe_transport_get_transport_id(packet, &transport_id);
	switch(transport_id) {
		case FBE_TRANSPORT_ID_DISCOVERY:
			/* The server part of fbe_discovery transport is a member of discovering class.
				Even more than that, we do not expect to receive discovery protocol packets
				for "non discovering" objects 
			*/
			status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) sas_cpd_port,
																		sas_cpd_port_discovery_transport_entry,
																		packet);
			break;
		case FBE_TRANSPORT_ID_SSP:
			/* The server part of ssp_transport is a member of sas_port class.
				Even more than that, we do not expect to receive ssp protocol packets
				for "non sas port" objects 
			*/

			status = fbe_sas_port_ssp_transport_entry((fbe_sas_port_t *) sas_cpd_port,
													sas_cpd_port_ssp_transport_entry,
													packet);

			break;

	}

	return status;
}

static fbe_status_t 
sas_cpd_port_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_payload_discovery_opcode_t discovery_opcode;

	fbe_status_t status;

	sas_cpd_port = (fbe_sas_cpd_port_t *)base_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
	
	switch(discovery_opcode){
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID:
				status = sas_cpd_port_discovery_transport_get_port_object_id(sas_cpd_port, packet);
			break;
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS:
				status = sas_cpd_port_discovery_transport_get_protocol_address(sas_cpd_port, packet);
			break;
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_ELEMENT_LIST:
				status = sas_cpd_port_discovery_transport_get_element_list(sas_cpd_port, packet);
			break;

		default:
			fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Uknown discovery_opcode %X\n", __FUNCTION__, discovery_opcode);

			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			status = FBE_STATUS_GENERIC_FAILURE;
			break;
	}

	return status;
}

static fbe_status_t 
sas_cpd_port_discovery_transport_get_protocol_address(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_protocol_address_data_t * payload_discovery_get_protocol_address_data = NULL;
	fbe_edge_index_t server_index;
	fbe_status_t status;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_protocol_address_data = (fbe_payload_discovery_get_protocol_address_data_t *)sg_list[0].address;

	/* As a for now we have one client only with this particular id */
	status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) sas_cpd_port, 
																payload_discovery_operation->command.get_protocol_address_command.client_id,
																&server_index);
	if(status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t *) sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_discovering_get_server_index_by_client_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	payload_discovery_get_protocol_address_data->address.sas_address = sas_cpd_port->sas_table[server_index].sas_address;
	payload_discovery_get_protocol_address_data->generation_code = sas_cpd_port->sas_table[server_index].generation_code;
	payload_discovery_get_protocol_address_data->chain_depth = sas_cpd_port->sas_table[server_index].enclosure_chain_depth; 

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_cpd_port_discovery_transport_get_port_object_id(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_port_object_id_data_t * payload_discovery_get_port_object_id_data = NULL;
	fbe_object_id_t my_object_id;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_port_object_id_data = (fbe_payload_discovery_get_port_object_id_data_t *)sg_list[0].address;

	fbe_base_object_get_object_id((fbe_base_object_t *)sas_cpd_port, &my_object_id);

	payload_discovery_get_port_object_id_data->port_object_id = my_object_id;

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_cpd_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;
	fbe_status_t status;

	sas_cpd_port = (fbe_sas_cpd_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are not interested in any event at this time, so just forward it to the super class */
	status = fbe_sas_port_event_entry(object_handle, event_type, event_context);

	return status;
}

static fbe_status_t 
sas_cpd_port_ssp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;
	fbe_io_block_t * io_block = NULL;
	fbe_status_t status;
	fbe_port_number_t port_number;

	sas_cpd_port = (fbe_sas_cpd_port_t *)base_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	io_block = fbe_transport_get_io_block (packet);

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);

	status = fbe_cpd_shim_send_io(port_number, io_block);

	return status;
}


static fbe_status_t 
sas_cpd_port_discovery_transport_get_element_list(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_element_list_command_t * get_element_list_command = NULL;
	fbe_payload_discovery_get_element_list_data_t * payload_discovery_get_element_list_data = NULL;
	fbe_u32_t number_of_elements;
	fbe_u32_t i;
	fbe_payload_discovery_element_t * element_list = NULL;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	get_element_list_command = &payload_discovery_operation->command.get_element_list_command;

	payload_discovery_get_element_list_data = (fbe_payload_discovery_get_element_list_data_t *)sg_list[0].address;

	element_list = payload_discovery_get_element_list_data->element_list;
	number_of_elements = 0;
	/* Lock sas_table */
	for(i = 0 ; i < FBE_SAS_CPD_PORT_MAX_PHY_NUMBER; i ++){
		if(sas_cpd_port->sas_table[i].parent_sas_address == get_element_list_command->address.sas_address){			
			element_list[number_of_elements].element_type = sas_cpd_port->sas_table[i].element_type;
			element_list[number_of_elements].address.sas_address = sas_cpd_port->sas_table[i].sas_address;
			element_list[number_of_elements].phy_number = sas_cpd_port->sas_table[i].phy_number;
			element_list[number_of_elements].enclosure_chain_depth = sas_cpd_port->sas_table[i].enclosure_chain_depth;

			element_list[number_of_elements].generation_code = sas_cpd_port->sas_table[i].generation_code;
			number_of_elements++;
		}
	}
	/* Unlock sas_table */
	payload_discovery_get_element_list_data->number_of_elements = number_of_elements; /* virtual phy and one SAS drive */

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
