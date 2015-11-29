#include "fbe/fbe_types.h"
#include "sas_bullet_enclosure_private.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"

/* Forward declarations */
static fbe_status_t fbe_sas_bullet_enclosure_send_get_element_list_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_bullet_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_bullet_enclosure_discovery_transport_get_protocol_address(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

fbe_status_t 
fbe_sas_bullet_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = NULL;
	fbe_transport_id_t transport_id;
	fbe_status_t status;

	sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry\n", __FUNCTION__);

	/* Fisrt we need to figure out to what transport this packet belong */
	fbe_transport_get_transport_id(packet, &transport_id);
	switch(transport_id) {
		case FBE_TRANSPORT_ID_DISCOVERY:
			/* The server part of fbe_discovery transport is a member of discovering class.
				Even more than that, we do not expect to receive discovery protocol packets
				for "non discovering" objects 
			*/
			status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) sas_bullet_enclosure,
																		sas_bullet_enclosure_discovery_transport_entry,
																		packet);
			break;
	}

	return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t 
fbe_sas_bullet_enclosure_send_get_element_list_command(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_packet_t * new_packet = NULL;

	new_packet = fbe_sas_enclosure_build_get_element_list_packet((fbe_sas_enclosure_t *)sas_bullet_enclosure, packet);

	/* Push additional completion context on the packet stack */
	status = fbe_transport_set_completion_function(new_packet, fbe_sas_bullet_enclosure_send_get_element_list_completion, sas_bullet_enclosure);

	/* Note we can not send the packet right away.
		It is possible that blocking is set or queue depth is exeded.
		So, we should "submit" "send" the packet to the bouncer for evaluation.
		Right now we have no bouncer implemented, but as we will have it we will use it.
	*/
	/* We are sending discovery packet via discovery edge */
	status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) sas_bullet_enclosure, new_packet);

	return status;
}

static fbe_status_t 
fbe_sas_bullet_enclosure_send_get_element_list_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = NULL;
	fbe_sg_element_t  * sg_list = NULL; 
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_payload_discovery_get_element_list_data_t * payload_discovery_get_element_list_data = NULL;
	fbe_u32_t i;
	fbe_status_t status;
	fbe_u32_t phy_number;

	sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)context;

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_element_list_data = (fbe_payload_discovery_get_element_list_data_t *)sg_list[0].address;
	
	for(i = 0 ; i < payload_discovery_get_element_list_data->number_of_elements; i++){
		/* Let's look if we got a SAS drive */
		if(payload_discovery_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP) {
			/*
			KvTrace("%s. SSP element %d sas_address %llX \n",__FUNCTION__,i,
				payload_discovery_get_element_list_data->element_list[i].address.sas_address);
			*/
			/* For now we will assume the direct mapping between phy_number and slot number */
			/* We need extruct locator information in order to understand where to put the drive */
			phy_number = payload_discovery_get_element_list_data->element_list[i].phy_number;
			if(phy_number >= FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS){
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                                   "%s Invalid slot number %d \n",
                                   __FUNCTION__, phy_number);
				continue;
			}
			sas_bullet_enclosure->drive_info[phy_number].sas_address = payload_discovery_get_element_list_data->element_list[i].address.sas_address;
			sas_bullet_enclosure->drive_info[phy_number].generation_code = payload_discovery_get_element_list_data->element_list[i].generation_code; 
			sas_bullet_enclosure->drive_info[phy_number].element_type = payload_discovery_get_element_list_data->element_list[i].element_type; 
		}else if(payload_discovery_get_element_list_data->element_list[i].element_type == FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_EXPANDER){
			/*
			KvTrace("%s. EXPANDER element %d sas_address %llX \n",__FUNCTION__,i,
				payload_discovery_get_element_list_data->element_list[i].address.sas_address);
			*/
			sas_bullet_enclosure->expansion_port_info.sas_address = payload_discovery_get_element_list_data->element_list[i].address.sas_address;
			sas_bullet_enclosure->expansion_port_info.generation_code = payload_discovery_get_element_list_data->element_list[i].generation_code; 
			sas_bullet_enclosure->expansion_port_info.element_type = payload_discovery_get_element_list_data->element_list[i].element_type; 

		}


	}

	/* We will assume that we found new element or lost some old element.
		In this case we should triger discovery update condition to take care of discovery edges.
	*/

	status = fbe_lifecycle_set_cond(&fbe_sas_bullet_enclosure_lifecycle_const, 
					(fbe_base_object_t*)sas_bullet_enclosure, 
					FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

	if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                                "%s can't set discovery update condition, status: 0x%X",
                                __FUNCTION__, status);
	}

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	return fbe_sas_enclosure_event_entry(object_handle, event_type, event_context);
}


static fbe_status_t 
sas_bullet_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = NULL;
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_payload_discovery_opcode_t discovery_opcode;

	sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)base_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
	
	switch(discovery_opcode){
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS:
				status = sas_bullet_enclosure_discovery_transport_get_protocol_address(sas_bullet_enclosure, packet);
			break;

		default:
			status = fbe_sas_enclosure_discovery_transport_entry((fbe_sas_enclosure_t *)sas_bullet_enclosure, packet);
			break;
	}

	return status;
}

static fbe_status_t 
sas_bullet_enclosure_discovery_transport_get_protocol_address(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_protocol_address_data_t * payload_discovery_get_protocol_address_data = NULL;
	fbe_edge_index_t server_index;
	fbe_status_t status;
    fbe_enclosure_number_t encl_number = FBE_ENCLOSURE_VALUE_INVALID;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_protocol_address_data = (fbe_payload_discovery_get_protocol_address_data_t *)sg_list[0].address;

	/* As a for now we have one client only with this particular id */
	status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) sas_bullet_enclosure, 
																payload_discovery_operation->command.get_protocol_address_command.client_id,
																&server_index);

	if(status != FBE_STATUS_OK) {
         fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                               "fbe_base_discovering_get_server_index_by_client_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	/* Check if server_index is an index of the drive */
	if(server_index < FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS) {
		payload_discovery_get_protocol_address_data->address.sas_address = sas_bullet_enclosure->drive_info[server_index].sas_address;
		payload_discovery_get_protocol_address_data->generation_code = sas_bullet_enclosure->drive_info[server_index].generation_code; 
		payload_discovery_get_protocol_address_data->element_type = sas_bullet_enclosure->drive_info[server_index].element_type; 

		fbe_base_enclosure_get_enclosure_number ((fbe_base_enclosure_t *)sas_bullet_enclosure, &encl_number);
        payload_discovery_get_protocol_address_data->chain_depth = encl_number; 
		status = FBE_STATUS_OK;
	} else if(server_index == FBE_SAS_BULLET_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX) {
		payload_discovery_get_protocol_address_data->address.sas_address = sas_bullet_enclosure->expansion_port_info.sas_address;
		payload_discovery_get_protocol_address_data->generation_code = sas_bullet_enclosure->expansion_port_info.generation_code; 
		payload_discovery_get_protocol_address_data->element_type = sas_bullet_enclosure->expansion_port_info.element_type; 
		payload_discovery_get_protocol_address_data->chain_depth = sas_bullet_enclosure->expansion_port_info.chain_depth; 
		status = FBE_STATUS_OK;
	} else {
         fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                               "%s Invalid server_index 0x%X",
                                __FUNCTION__, server_index);

		status = FBE_STATUS_GENERIC_FAILURE;
	}


	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}




