#include "ktrace.h"

#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"

#include "fbe_transport_memory.h"
#include "sas_lsi_port_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_sas_lsi_port_load(void);
fbe_status_t fbe_sas_lsi_port_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sas_lsi_port_class_methods = {FBE_CLASS_ID_SAS_LSI_PORT,
							        			      fbe_sas_lsi_port_load,
												      fbe_sas_lsi_port_unload,
												      fbe_sas_lsi_port_create_object,
												      fbe_sas_lsi_port_destroy_object,
												      fbe_sas_lsi_port_control_entry,
												      fbe_sas_lsi_port_event_entry,
                                                      NULL};


/* Forward declaration */
static fbe_status_t fbe_sas_lsi_port_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_sas_lsi_port_get_parent_address(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_lsi_port_get_product_type(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_lsi_port_get_device_ids(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_lsi_port_send_scsi_command(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);

/* Monitor functions */
static fbe_status_t fbe_sas_lsi_port_monitor_state_specialized(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_lsi_port_monitor_state_ready(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_lsi_port_monitor_destroy_pending(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_lsi_port_monitor_destroy(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet);

fbe_bool_t first_time_port = TRUE;
//fbe_bool_t first_time = TRUE;
fbe_status_t fbe_sas_lsi_port_load(void)
{
	KvTrace("fbe_sas_lsi_port_main: %s entry\n", __FUNCTION__);
	fbe_sas_lsi_port_init();
	return FBE_STATUS_OK;
}

fbe_status_t fbe_sas_lsi_port_unload(void)
{
	KvTrace("fbe_sas_lsi_port_main: %s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lsi_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_lsi_port_t * sas_lsi_port;
	fbe_status_t status;
	

	KvTrace("fbe_sas_lsi_port_main: %s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_port_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_lsi_port = (fbe_sas_lsi_port_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_lsi_port, FBE_CLASS_ID_SAS_LSI_PORT);	


    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_lsi_port_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_sas_lsi_port_t *sas_lsi_port;
    fbe_u32_t port_number;

	KvTrace("fbe_sas_lsi_port_main: %s entry\n", __FUNCTION__);
    sas_lsi_port = (fbe_sas_lsi_port_t *)fbe_base_handle_to_pointer(object_handle);
  	/* Destroy sas port */
	status = fbe_sas_port_get_io_port_number((fbe_sas_port_t *)sas_lsi_port, &port_number);
	
    fbe_sas_lsi_port_destroy(port_number);

    /* For now we just call our sas port destry function */	
	/* Call parent destructor */
	status = fbe_sas_port_destroy_object(object_handle);
	return status;
}

fbe_status_t 
fbe_sas_lsi_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_lsi_port_t * sas_lsi_port = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_lsi_port = (fbe_sas_lsi_port_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_BASE_OBJECT_CONTROL_CODE_MONITOR:
			status = fbe_sas_lsi_port_monitor(object_handle, packet);
			break;
		case FBE_BASE_OBJECT_CONTROL_CODE_CREATE_EDGE:
			status = fbe_sas_lsi_port_create_edge(sas_lsi_port, packet);
			break;
        case FBE_BASE_OBJECT_CONTROL_CODE_GET_PARENT_ADDRESS:
			status = fbe_sas_lsi_port_get_parent_address(sas_lsi_port, packet);
			break;

        case FBE_SAS_PORT_CONTROL_CODE_GET_PRODUCT_TYPE:
            status = fbe_sas_lsi_port_get_product_type(sas_lsi_port, packet);
            break;
    
        case FBE_SAS_PORT_CONTROL_CODE_GET_PARENT_SAS_ELEMENT:
            status = fbe_sas_lsi_port_get_device_ids(sas_lsi_port, packet);
            break;
        
        case FBE_SAS_PORT_CONTROL_CODE_SEND_SCSI_COMMAND:
            status = fbe_sas_lsi_port_send_scsi_command(sas_lsi_port, packet);
            break;
        
        default:
			return fbe_base_port_control_entry(object_handle, packet);
			break;
	}
	return status;
}

fbe_status_t
fbe_sas_lsi_port_monitor(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_sas_lsi_port_t * sas_lsi_port;
	fbe_object_state_t mgmt_state;
	fbe_status_t status;
    
	sas_lsi_port = (fbe_sas_lsi_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	/* We need to think about lock here */
	fbe_base_object_get_mgmt_state((fbe_base_object_t *) sas_lsi_port, &mgmt_state);

	switch(mgmt_state) {
		case FBE_OBJECT_STATE_INSTANTIATED:
			/* Get specific port type */

			/* Change class ID */
			/* Note: The next monitor will arrive to the identified (specific) object */
			fbe_base_object_set_class_id((fbe_base_object_t *) sas_lsi_port, FBE_CLASS_ID_SAS_LSI_PORT);

			/* Change state to FBE_OBJECT_STATE_SPECIALIZED */
			fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lsi_port, FBE_OBJECT_STATE_SPECIALIZED);

			/* Our state is changed. Ask scheduler to pay attention to us */
			fbe_base_object_run_request((fbe_base_object_t *) sas_lsi_port);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
			status = FBE_STATUS_OK;
			break;
		case FBE_OBJECT_STATE_SPECIALIZED:
			status = fbe_sas_lsi_port_monitor_state_specialized(sas_lsi_port, packet);
			break;

		case FBE_OBJECT_STATE_READY:
            status = fbe_sas_lsi_port_monitor_state_ready(sas_lsi_port, packet);
            break;
        case FBE_OBJECT_STATE_DESTROY_PENDING:
            status = fbe_sas_lsi_port_monitor_destroy_pending(sas_lsi_port, packet);
            break;

        case FBE_OBJECT_STATE_DESTROY:
            status = fbe_sas_lsi_port_monitor_destroy(sas_lsi_port, packet);
            break;

        default:
			KvTrace("fbe_sas_lsi_port_main: %s Invalid object state %X\n", __FUNCTION__, mgmt_state);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			break;
	}

	return status;
}

fbe_status_t 
fbe_sas_lsi_port_create_edge(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	fbe_base_object_mgmt_create_edge_t * base_object_mgmt_create_edge;
	fbe_packet_t * new_packet = NULL;
	fbe_base_edge_t   * edge = NULL;
	fbe_status_t status;
	fbe_semaphore_t sem;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_create_edge);

	/* Build the edge. */ 
	status = fbe_base_object_build_edge((fbe_base_object_t *) sas_lsi_port,
										base_object_mgmt_create_edge->parent_index,
										base_object_mgmt_create_edge->child_id,
										base_object_mgmt_create_edge->child_index);

	/* Insert the edge. */
	fbe_semaphore_init(&sem, 0, 1);
	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if(new_packet == NULL) {
		KvTrace("fbe_sas_lsi_lcc_main: %s fbe_transport_allocate_packet fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    	fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_initialize_packet(new_packet);
	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_lsi_port, 
										base_object_mgmt_create_edge->parent_index,
										&edge);

	fbe_transport_build_control_packet(new_packet, 
								FBE_BASE_OBJECT_CONTROL_CODE_INSERT_EDGE,
								edge,
								sizeof(fbe_base_edge_t),
								0,
								fbe_sas_lsi_port_create_edge_completion,
								&sem);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_lsi_port, new_packet, base_object_mgmt_create_edge->parent_index);

	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	status = fbe_transport_get_status_code(new_packet);
	if(status == FBE_STATUS_OK){ 
		/* We succesfully created edge, so our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lsi_port);
	} else {
		KvTrace("\n\n fbe_sas_lsi_lcc_main: %s Faild to insert edge\n\n", __FUNCTION__);
	}

    fbe_transport_copy_packet_status(new_packet, packet);
	fbe_transport_release_packet(new_packet);
	/* We have to move this code to the monitor */
	fbe_base_port_set_port_number((fbe_base_port_t *) sas_lsi_port, base_object_mgmt_create_edge->child_index);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_lsi_port_create_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

	/*KvTrace("fbe_sas_lsi_lcc_main: %s entry\n", __FUNCTION__);*/

    fbe_semaphore_release(sem, 0, 1, FALSE);

	return FBE_STATUS_OK;
}

static fbe_status_t
fbe_sas_lsi_port_get_parent_address(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	fbe_base_object_mgmt_get_parent_address_t * base_object_mgmt_get_parent_address = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer_length(control_operation, &in_len); 
	fbe_payload_control_get_buffer_length(control_operation, &out_len); 

	if(in_len != sizeof(fbe_base_object_mgmt_get_parent_address_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	if(out_len != sizeof(fbe_base_object_mgmt_get_parent_address_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_get_parent_address); 
	if(base_object_mgmt_get_parent_address == NULL){
		KvTrace("fbe_sas_lsi_port_main: %s Ifbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	/* As a sas_lsi port we supposed to have one parent only with this particular id */
	/* Since we are dealing with the parent_list we have to grub the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_lsi_port, base_object_mgmt_get_parent_address->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);

		fbe_base_object_trace((fbe_base_object_t *) sas_lsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    /* Translate parent_edge->child_index to SAS Address */
	base_object_mgmt_get_parent_address->address.sas_address = sas_lsi_port->port_info.device_info[parent_edge->child_index].sas_address;

	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return fbe_transport_get_status_code(packet);
}

fbe_status_t 
fbe_sas_lsi_port_event_entry(fbe_object_handle_t object_handle, fbe_event_t event)
{
	return FBE_STATUS_OK;
}


static fbe_status_t
fbe_sas_lsi_port_monitor_state_specialized(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{	
	fbe_base_edge_t * edge = NULL; 
	fbe_status_t status;

	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_lsi_port, 0, &edge);
	if(edge->edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		/* We lost our edge, so it is time to die */
		fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lsi_port, FBE_OBJECT_STATE_DESTROY_PENDING); 
		/* Our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lsi_port);
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}
	
    /* We done with specialized state. Let's go to ready */
	/* Change state to FBE_OBJECT_STATE_SPECIALIZED */
	fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lsi_port, FBE_OBJECT_STATE_READY);

	/* Our state is changed. Ask scheduler to pay attention to us */
	fbe_base_object_run_request((fbe_base_object_t *) sas_lsi_port);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t
fbe_sas_lsi_port_get_device_ids(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	fbe_sas_port_mgmt_get_parent_sas_element_t * fbe_sas_port_mgmt_get_device_ids = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
        
	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer_length(control_operation, &in_len);
	fbe_payload_control_get_buffer_length(control_operation, &out_len);

	if(in_len != sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	if(out_len != sizeof(fbe_sas_port_mgmt_get_parent_sas_element_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	fbe_payload_control_get_buffer(control_operation, &fbe_sas_port_mgmt_get_device_ids); 
	if(fbe_sas_port_mgmt_get_device_ids == NULL){
		KvTrace("fbe_sas_lsi_port_main: %s Ifbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	/* As a sas_lsi port we supposed to have one parent only with this particular id */
	/* Since we are dealing with the parent_list we have to grub the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_lsi_port, fbe_sas_port_mgmt_get_device_ids->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);

		fbe_base_object_trace((fbe_base_object_t *) sas_lsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*
	fbe_sas_port_mgmt_get_device_ids->bus_id = sas_lsi_port->port_info.device_info[parent_edge->child_index].bus_id;
    fbe_sas_port_mgmt_get_device_ids->target_id = sas_lsi_port->port_info.device_info[parent_edge->child_index].target_id;
    fbe_sas_port_mgmt_get_device_ids->lun_id = sas_lsi_port->port_info.device_info[parent_edge->child_index].lun_id;
	*/

    
	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return fbe_transport_get_status_code(packet);
}

static fbe_status_t
fbe_sas_lsi_port_get_product_type(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	fbe_sas_port_mgmt_get_product_type_t * fbe_sas_port_mgmt_get_product_type = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer_length(control_operation, &in_len); 
	fbe_payload_control_get_buffer_length(control_operation, &out_len);

	if(in_len != sizeof(fbe_sas_port_mgmt_get_product_type_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	if(out_len != sizeof(fbe_sas_port_mgmt_get_product_type_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	fbe_payload_control_get_buffer(control_operation, &fbe_sas_port_mgmt_get_product_type);
	if(fbe_sas_port_mgmt_get_product_type == NULL){
		KvTrace("fbe_sas_lsi_port_main: %s Ifbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	/* As a sas_lsi port we supposed to have one parent only with this particular id */
	/* Since we are dealing with the parent_list we have to grub the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_lsi_port, fbe_sas_port_mgmt_get_product_type->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);

		fbe_base_object_trace((fbe_base_object_t *) sas_lsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
	fbe_sas_port_mgmt_get_product_type->product_type = sas_lsi_port->port_info.device_info[parent_edge->child_index].product_type;

	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return fbe_transport_get_status_code(packet);
}

static fbe_status_t
fbe_sas_lsi_port_send_scsi_command(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	fbe_sas_port_mgmt_ses_command_t * fbe_sas_port_mgmt_ses_command = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_lsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer_length(control_operation, &in_len); 
	fbe_payload_control_get_buffer_length(control_operation, &out_len); 

	if(in_len != sizeof(fbe_sas_port_mgmt_ses_command_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	if(out_len != sizeof(fbe_sas_port_mgmt_ses_command_t)){
		KvTrace("fbe_sas_lsi_port_main: %s Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

	fbe_payload_control_get_buffer(control_operation, &fbe_sas_port_mgmt_ses_command); 
	if(fbe_sas_port_mgmt_ses_command == NULL){
		KvTrace("fbe_sas_lsi_port_main: %s Ifbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return fbe_transport_get_status_code(packet);
	}

    status = fbe_sas_port_get_io_port_number((fbe_sas_port_t *)sas_lsi_port, &port_number);
    status = fbe_sas_lsi_shim_send_scsi_command(port_number, fbe_sas_port_mgmt_ses_command->bus_id,
                                                fbe_sas_port_mgmt_ses_command->target_id,
                                                fbe_sas_port_mgmt_ses_command->lun_id,
                                                &fbe_sas_port_mgmt_ses_command->cdb[0],
                                                fbe_sas_port_mgmt_ses_command->cdb_size,
                                                &fbe_sas_port_mgmt_ses_command->resp_buffer[0],
                                                fbe_sas_port_mgmt_ses_command->resp_buffer_size, 
                                                &fbe_sas_port_mgmt_ses_command->cmd[0],
                                                fbe_sas_port_mgmt_ses_command->cmd_size);

	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return fbe_transport_get_status_code(packet);
}

static fbe_status_t
fbe_sas_lsi_port_monitor_state_ready(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
    fbe_u32_t i;
    fbe_port_number_t port_number;
	fbe_base_edge_t * parent_edge = NULL;
    fbe_u32_t device_count = 0;
    fbe_status_t status;
    fbe_base_edge_t   * edge = NULL; 

	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_lsi_port, 0, &edge);

	if(edge->edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lsi_port, FBE_OBJECT_STATE_DESTROY_PENDING); 
		/* Our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_lsi_port);
     	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);	
		fbe_transport_complete_packet(packet);	
		return FBE_STATUS_OK;
	}

    if(first_time_port)
    {
    status = fbe_sas_port_get_io_port_number((fbe_sas_port_t *)sas_lsi_port, &port_number);
    status = fbe_sas_lsi_get_all_attached_devices(port_number, 
                                                  &sas_lsi_port->port_info.device_info[0], 
                                                  FBE_SAS_LSI_SHIM_MAX_DEVICES_PER_PORT, 
                                                  &device_count);
    for (i = 0; i < device_count; i++){
        if(sas_lsi_port->port_info.device_info[i].device_type != FBE_DEVICE_TYPE_INVALID){ /* We have valid device */
            /* Lock parent list */
            fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
            parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lsi_port, i);
            if(parent_edge == NULL){ /* This port have no parent */
                fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
                if(sas_lsi_port->port_info.device_info[i].device_type == FBE_DEVICE_TYPE_EXPANDER){
                    status = fbe_base_object_instantiate_parent((fbe_base_object_t *) sas_lsi_port,
                                                                 FBE_CLASS_ID_SAS_LCC,
                                                                 i);				
                }
            } else { /* Just update the edge state */
                /* update the edge state */
                fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
            }
        } else { /* We have no valid port */
            /* Lock parent list */
            fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
            parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lsi_port, i);
            if(parent_edge == NULL){ /* This port have no parent */
                fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
            } else { /* We have parent, but port is invalid */
                fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
                /* Destroy the edge */
                KvTrace("\n\n fbe_sas_lsi_port_main: %s A Parent exists; destroy it\n\n", __FUNCTION__);
                fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lsi_port, parent_edge->parent_id);
            }
        }
    }/* for (i = 0; i < FBE_SAS_LSI_SHIM_MAX_DEVICES_PER_PORT; i++) */
    if(device_count == 0)
    {
        for (i = 0; i < FBE_SAS_LSI_SHIM_MAX_DEVICES_PER_PORT; i++){
            fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
            parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lsi_port, i);
            if(parent_edge == NULL){ /* This port have no parent */
                fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
            } else { /* We have parent, but port is invalid */
                fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
                /* Destroy the edge */
                KvTrace("\n\n fbe_sas_lsi_port_main: %s A Parent exists but no devices on this port\n\n", __FUNCTION__);
                fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lsi_port, parent_edge->parent_id);
            }
        }
    }
    first_time_port = FALSE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return(FBE_STATUS_OK);
}

static fbe_status_t 
fbe_sas_lsi_port_monitor_destroy_pending(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	fbe_base_edge_t * parent_edge = NULL; 
	fbe_object_id_t parent_id;

	/* The first thing to do is to destroy all parent edges */
	/* It is make sense to support this functionality in base_object, but for now it is OK to do it here */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_lsi_port);
	parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_lsi_port, 0);
	if(parent_edge != NULL){
		parent_id = parent_edge->parent_id;
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
		fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_lsi_port, parent_id);			
	} else{
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_lsi_port);
	}

	/* check termination and pending queues */
	if(!fbe_base_object_is_terminator_queue_empty((fbe_base_object_t *)sas_lsi_port)){
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* switch to destroy state */
	fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_lsi_port, FBE_OBJECT_STATE_DESTROY); 

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_lsi_port_monitor_destroy(fbe_sas_lsi_port_t * sas_lsi_port, fbe_packet_t * packet)
{
	/* Tell scheduler that object is no longer exist */
	fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
	fbe_transport_complete_packet(packet);
    
	fbe_base_object_destroy_request((fbe_base_object_t *) sas_lsi_port);
	return FBE_STATUS_NO_DEVICE;
}
