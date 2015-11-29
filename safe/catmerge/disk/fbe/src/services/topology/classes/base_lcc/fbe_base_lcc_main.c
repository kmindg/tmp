#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_lcc.h"
#include "fbe_scheduler.h"
#include "fbe_diplex.h"
#include "fbe_base_lcc.h"

#include "base_object_private.h"
#include "base_lcc_private.h"

/* Class methods forward declaration */
fbe_status_t base_lcc_load(void);
fbe_status_t base_lcc_unload(void);
fbe_status_t fbe_base_lcc_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_lcc_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_lcc_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_lcc_event_entry(fbe_object_handle_t object_handle, fbe_event_t event);
static fbe_status_t base_lcc_get_mgmt_address(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet);

/* Export class methods  */
fbe_class_methods_t fbe_base_lcc_class_methods = {FBE_CLASS_ID_BASE_LCC,
													base_lcc_load,
													base_lcc_unload,
													fbe_base_lcc_create_object,
													fbe_base_lcc_destroy_object,
													fbe_base_lcc_control_entry,
													fbe_base_lcc_event_entry,
													NULL,
													NULL};


/* Forward declaration */
static fbe_status_t base_lcc_get_lcc_number(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet);
static fbe_status_t base_lcc_get_slot_number(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet);
static fbe_status_t base_lcc_get_lcc_info(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet);


fbe_status_t 
base_lcc_load(void)
{
	KvTrace("fbe_base_lcc_main: %s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
base_lcc_unload(void)
{
	KvTrace("fbe_base_lcc_main: %s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_base_lcc_t * base_lcc;
	fbe_status_t status;

	KvTrace("fbe_base_lcc_main: %s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_object_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	base_lcc = (fbe_base_lcc_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) base_lcc, FBE_CLASS_ID_BASE_LCC);	

	/* Clear number of slots */
	fbe_base_lcc_set_number_of_slots(base_lcc, 0);
	fbe_base_lcc_set_first_slot_index(base_lcc, 0);

	fbe_base_lcc_set_number_of_expansion_ports(base_lcc, 0);
	fbe_base_lcc_set_first_expansion_port_index(base_lcc, 0);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_base_lcc_t * base_lcc;

	KvTrace("fbe_base_lcc_main: %s entry\n", __FUNCTION__);
	
	base_lcc = (fbe_base_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	/* Check parent edges */
	/* Cleanup */

	/* Call parent destructor */
	status = fbe_base_object_destroy_object(object_handle);
	return status;
}

fbe_status_t 
fbe_base_lcc_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_lcc_t * base_lcc = NULL;

	fbe_payload_control_operation_opcode_t control_code;

	/* KvTrace("fbe_base_lcc_main: %s entry\n", __FUNCTION__); */
	base_lcc = (fbe_base_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_BASE_OBJECT_CONTROL_CODE_MONITOR:
			status = fbe_base_lcc_monitor(base_lcc, packet);
			break;
		case FBE_BASE_LCC_CONTROL_CODE_GET_LCC_NUMBER:
			status = base_lcc_get_lcc_number(base_lcc, packet);
			break;
		case FBE_BASE_LCC_CONTROL_CODE_GET_SLOT_NUMBER:
			status = base_lcc_get_slot_number(base_lcc, packet);
			break;
		case FBE_BASE_LCC_CONTROL_CODE_GET_LCC_INFO:
			status = base_lcc_get_lcc_info(base_lcc, packet);
			break;
		case FBE_BASE_LCC_CONTROL_CODE_GET_MGMT_ADDRESS:
			status = base_lcc_get_mgmt_address(base_lcc, packet);
			break;
		default:
			/*
			KvTrace("fbe_base_lcc_main: %s Uknown control code %X\n", __FUNCTION__, control_code);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			*/
			return fbe_base_object_control_entry(object_handle, packet);
			break;
	}

	status = fbe_transport_get_status_code(packet);
	if(status != FBE_STATUS_PENDING) {
		fbe_transport_complete_packet(packet);	
	}
	return status;
}

fbe_status_t
fbe_base_lcc_monitor(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_object_state_t mgmt_state;

	KvTrace("fbe_base_lcc_main: %s entry\n", __FUNCTION__);

	/* We need to think about the lock */
	fbe_base_object_get_mgmt_state((fbe_base_object_t *) base_lcc, &mgmt_state);

	status = FBE_STATUS_GENERIC_FAILURE;
	switch(mgmt_state) {
		default:
			KvTrace("fbe_base_lcc_main: %s Invalid object state %X\n", __FUNCTION__, mgmt_state);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_GENERIC_FAILURE;
			break;
	}

	return status;
}


fbe_status_t 
fbe_base_lcc_get_number_of_slots(fbe_base_lcc_t * base_lcc, fbe_u32_t * number_of_slots)
{
	*number_of_slots = base_lcc->number_of_slots;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_set_number_of_slots(fbe_base_lcc_t * base_lcc, fbe_u32_t number_of_slots)
{
	base_lcc->number_of_slots = number_of_slots;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_get_first_slot_index(fbe_base_lcc_t * base_lcc, fbe_edge_index_t * first_slot_index)
{
	*first_slot_index = base_lcc->first_slot_index;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_set_first_slot_index(fbe_base_lcc_t * base_lcc, fbe_edge_index_t first_slot_index)
{
	base_lcc->first_slot_index = first_slot_index;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_get_number_of_expansion_ports(fbe_base_lcc_t * base_lcc, fbe_u32_t * number_of_expansion_ports)
{
	*number_of_expansion_ports = base_lcc->number_of_expansion_ports;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_set_number_of_expansion_ports(fbe_base_lcc_t * base_lcc, fbe_u32_t number_of_expansion_ports)
{
	base_lcc->number_of_expansion_ports = number_of_expansion_ports;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_get_first_expansion_port_index(fbe_base_lcc_t * base_lcc, fbe_edge_index_t * first_expansion_port_index)
{
	*first_expansion_port_index = base_lcc->first_expansion_port_index;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_set_first_expansion_port_index(fbe_base_lcc_t * base_lcc, fbe_edge_index_t first_expansion_port_index)
{
	base_lcc->first_expansion_port_index = first_expansion_port_index;
	return FBE_STATUS_OK;
}

static fbe_status_t
base_lcc_get_lcc_number(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet)
{
	fbe_base_lcc_mgmt_get_lcc_number_t * base_lcc_mgmt_get_lcc_number = NULL;
	fbe_status_t status;
	fbe_payload_control_buffer_length_t out_len = 0;
	fbe_physical_object_level_t physical_object_level;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload(packet);
    control_operation = fbe_payload_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &base_lcc_mgmt_get_lcc_number); 
	if(base_lcc_mgmt_get_lcc_number == NULL){
		KvTrace("fbe_base_lcc_main: %s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &out_len);
	if(out_len != sizeof(fbe_base_lcc_mgmt_get_lcc_number_t)){
		KvTrace("fbe_base_lcc_main: %s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_base_object_get_physical_object_level((fbe_base_object_t *)base_lcc, &physical_object_level);

	/* lcc_number = 0 for first lcc */
	base_lcc_mgmt_get_lcc_number->lcc_number = physical_object_level - FBE_PHYSICAL_OBJECT_LEVEL_FIRST_LCC;
	status = FBE_STATUS_OK;
	fbe_transport_set_status(packet, status, 0);
	return status;
}

static fbe_status_t 
base_lcc_get_slot_number(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet)
{
	fbe_base_lcc_mgmt_get_slot_number_t * base_lcc_mgmt_get_slot_number = NULL;
	fbe_payload_control_buffer_length_t out_len = 0;
	fbe_base_edge_t * parent_edge = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload(packet);
    control_operation = fbe_payload_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &base_lcc_mgmt_get_slot_number);
	if(base_lcc_mgmt_get_slot_number == NULL){
		KvTrace("fbe_base_lcc_main: %s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &out_len);
	if(out_len != sizeof(fbe_base_lcc_mgmt_get_slot_number_t)){
		KvTrace("fbe_base_lcc_main: %s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_object_lock_parent_list((fbe_base_object_t *)base_lcc);
	parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) base_lcc, base_lcc_mgmt_get_slot_number->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)base_lcc);
		KvTrace("fbe_base_lcc_main: %s fbe_base_object_get_parent_by_id fail\n", __FUNCTION__);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	/* Translate parent_edge->child_index to slot_number */
	if((parent_edge->child_index >= base_lcc->first_slot_index) && 
		(parent_edge->child_index < (base_lcc->first_slot_index + base_lcc->number_of_slots))){
			base_lcc_mgmt_get_slot_number->lcc_slot_number = parent_edge->child_index - base_lcc->first_slot_index;
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)base_lcc);
	} else {
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)base_lcc);
		KvTrace("fbe_base_lcc_main: %s Invalid child_index %X\n", __FUNCTION__, parent_edge->child_index);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	return FBE_STATUS_OK;
}

static fbe_status_t 
base_lcc_get_lcc_info(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet)
{
	fbe_base_object_mgmt_get_lcc_info_t * base_object_mgmt_get_lcc_info = NULL;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload(packet);
    control_operation = fbe_payload_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_get_lcc_info);
	if(base_object_mgmt_get_lcc_info == NULL){
		KvTrace("fbe_base_lcc_main: %s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &out_len);
	if(out_len != sizeof(fbe_base_object_mgmt_get_lcc_info_t)){
		KvTrace("fbe_base_lcc_main: %s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	base_object_mgmt_get_lcc_info->number_of_slots = base_lcc->number_of_slots;

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_lcc_event_entry(fbe_object_handle_t object_handle, fbe_event_t event)
{
	return FBE_STATUS_OK;
}

static fbe_status_t 
base_lcc_get_mgmt_address(fbe_base_lcc_t * base_lcc, fbe_packet_t * packet)
{
	fbe_base_lcc_mgmt_get_mgmt_address_t * base_lcc_mgmt_get_mgmt_address = NULL;
	fbe_payload_control_operation_t out_len = 0;
	fbe_physical_object_level_t physical_object_level;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload(packet);
    control_operation = fbe_payload_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &base_lcc_mgmt_get_mgmt_address);
	if(base_lcc_mgmt_get_mgmt_address == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)base_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &out_len);
	if(out_len != sizeof(fbe_base_lcc_mgmt_get_mgmt_address_t)){
		fbe_base_object_trace(	(fbe_base_object_t*)base_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, invalid out_buffer_length: %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_object_get_physical_object_level((fbe_base_object_t *)base_lcc, &physical_object_level);

	base_lcc_mgmt_get_mgmt_address->address.diplex_address = physical_object_level - FBE_PHYSICAL_OBJECT_LEVEL_FIRST_LCC;

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
