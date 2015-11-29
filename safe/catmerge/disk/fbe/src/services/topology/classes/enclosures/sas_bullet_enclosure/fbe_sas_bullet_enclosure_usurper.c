#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "sas_bullet_enclosure_private.h"

/* Forward declaration */
static fbe_status_t sas_bullet_enclosure_attach_edge(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);
static fbe_status_t sas_bullet_enclosure_detach_edge(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);
static fbe_status_t sas_bullet_enclosure_get_slot_number(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

fbe_status_t 
fbe_sas_bullet_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_bullet_enclosure_t * sas_bullet_enclosure = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_bullet_enclosure = (fbe_sas_bullet_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry \n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = sas_bullet_enclosure_attach_edge( sas_bullet_enclosure, packet);
			break;
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = sas_bullet_enclosure_detach_edge( sas_bullet_enclosure, packet);
			break;
		case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER:
			status = sas_bullet_enclosure_get_slot_number( sas_bullet_enclosure, packet);
			break;

		default:
			status = fbe_sas_enclosure_control_entry(object_handle, packet);
			break;
	}

	return status;
}

static fbe_status_t 
sas_bullet_enclosure_attach_edge(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet)
{
	fbe_u32_t number_of_clients = 0;
	fbe_discovery_transport_control_attach_edge_t * discovery_transport_control_attach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
	fbe_edge_index_t server_index;
	fbe_status_t  status;

     len = sizeof(fbe_discovery_transport_control_attach_edge_t);
     status = fbe_base_enclosure_get_packet_payload_control_data(packet,(fbe_base_enclosure_t*)sas_bullet_enclosure,TRUE,&discovery_transport_control_attach_edge,len);
      if(status == FBE_STATUS_OK)
      {
      

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	fbe_discovery_transport_get_server_index(discovery_transport_control_attach_edge->discovery_edge, &server_index);
	/* Edge validatein need to be done here */

	fbe_base_discovering_attach_edge((fbe_base_discovering_t *) sas_bullet_enclosure, discovery_transport_control_attach_edge->discovery_edge);
	fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) sas_bullet_enclosure, server_index, 0 /* attr */);
	fbe_base_discovering_update_path_state((fbe_base_discovering_t *) sas_bullet_enclosure);
      	}
	fbe_transport_set_status(packet, status, 0);
       fbe_transport_complete_packet(packet);
	return status;
}

static fbe_status_t 
sas_bullet_enclosure_detach_edge(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                       "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_detach_edge); 
	if(discovery_transport_control_detach_edge == NULL){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bullet_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bullet_enclosure),
                           "fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}


	status = fbe_base_discovering_detach_edge((fbe_base_discovering_t *) sas_bullet_enclosure, discovery_transport_control_detach_edge->discovery_edge);

	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}


static fbe_status_t sas_bullet_enclosure_get_slot_number(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet)
{
	fbe_base_enclosure_mgmt_get_slot_number_t * base_enclosure_mgmt_get_slot_number = NULL;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
	fbe_edge_index_t server_index;
	fbe_status_t status;

      out_len = sizeof(fbe_base_enclosure_mgmt_get_slot_number_t);
      status = fbe_base_enclosure_get_packet_payload_control_data(packet,(fbe_base_enclosure_t*)sas_bullet_enclosure,TRUE,&base_enclosure_mgmt_get_slot_number,out_len);
      if(status == FBE_STATUS_OK)
      {

	status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) sas_bullet_enclosure,
																base_enclosure_mgmt_get_slot_number->client_id,
																&server_index);
	
	/* Translate server_index to slot_number */
	/* One to one translation */
	base_enclosure_mgmt_get_slot_number->enclosure_slot_number = server_index;
      	}
	fbe_transport_set_status(packet, status, 0);
	fbe_transport_complete_packet(packet);
	return status;

}
