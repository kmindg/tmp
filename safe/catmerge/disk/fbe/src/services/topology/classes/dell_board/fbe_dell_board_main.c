#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"

#include "fbe/fbe_board.h"
#include "dell_board_private.h"
#include "fbe_cpd_shim.h"

/* Class methods forward declaration */
fbe_status_t dell_board_load(void);
fbe_status_t dell_board_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_dell_board_class_methods = {FBE_CLASS_ID_DELL_BOARD,
													dell_board_load,
													dell_board_unload,
													fbe_dell_board_create_object,
													fbe_dell_board_destroy_object,
													fbe_dell_board_control_entry,
													fbe_dell_board_event_entry,
                                                    NULL,
													fbe_dell_board_monitor_entry};


/* Forward declaration */
static fbe_status_t dell_board_init(fbe_dell_board_t * dell_board);

/*
static fbe_status_t fbe_dell_board_port_monitor(fbe_dell_board_t * dell_board, fbe_packet_t * packet);
static fbe_status_t fbe_dell_board_get_io_port_number(fbe_dell_board_t * dell_board, fbe_packet_t * packet);
static fbe_status_t fbe_dell_board_get_port_driver(fbe_dell_board_t * dell_board, fbe_packet_t * packet);
*/


fbe_status_t dell_board_load(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	return fbe_dell_board_monitor_load_verify();
}

fbe_status_t dell_board_unload(void)
{
	KvTrace("%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_dell_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board;

	KvTrace("%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_board_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	dell_board = (fbe_dell_board_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) dell_board, FBE_CLASS_ID_DELL_BOARD);	

	dell_board_init(dell_board);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_dell_board_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board;

	KvTrace("%s entry\n", __FUNCTION__);
	
	dell_board = (fbe_dell_board_t *)fbe_base_handle_to_pointer(object_handle);

	/* Check parent edges ??? */
	/* Cleanup */

	/* Call parent destructor */
	status = fbe_base_board_destroy_object(object_handle);
	return status;
}


fbe_status_t 
fbe_dell_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_dell_board_update_hardware_ports(fbe_dell_board_t * dell_board, fbe_packet_t * packet)
{
#if 0
	fbe_status_t status;
	fbe_u32_t i;
	fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports;
	fbe_port_type_t port_type;
    
    KvTrace("%s entry\n", __FUNCTION__);

	status = fbe_cpd_shim_enumerate_backend_ports(&cpd_shim_enumerate_backend_ports);
	if(status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s failed", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return status;
	}

	fbe_base_object_trace((fbe_base_object_t *) dell_board, 
							FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%X hardware ports found: \n", cpd_shim_enumerate_backend_ports.number_of_io_ports);

	fbe_base_board_set_number_of_io_ports((fbe_base_board_t *) dell_board, cpd_shim_enumerate_backend_ports.number_of_io_ports);

	for(i = 0; i < cpd_shim_enumerate_backend_ports.number_of_io_ports; i++) {
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%X hardware port \n", cpd_shim_enumerate_backend_ports.io_port_array[i]);

		/*dell_board->io_port_array[i] = cpd_shim_enumerate_backend_ports.io_port_array[i];*/
		dell_board->io_port_array[i] = (fbe_u32_t)cpd_shim_enumerate_backend_ports.io_port_array[i].port_number;
		fbe_cpd_shim_get_port_type(i, &port_type);
		dell_board->io_port_type[i] = port_type;
		
		


	}

	status = fbe_lifecycle_set_cond(&fbe_dell_board_lifecycle_const, 
									(fbe_base_object_t*)dell_board, 
									FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't set discovery update condition, status: 0x%X",
								__FUNCTION__, status);
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
#endif
	return FBE_STATUS_OK;
}

#if 0
static fbe_status_t 
fbe_dell_board_get_port_driver(fbe_dell_board_t * dell_board, fbe_packet_t * packet)
{
	fbe_base_board_mgmt_get_port_driver_t * base_board_mgmt_get_port_driver = NULL;
	fbe_payload_control_buffer_length_t out_len = 0;
	fbe_base_edge_t * parent_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer(control_operation, &base_board_mgmt_get_port_driver); 
	if(base_board_mgmt_get_port_driver == NULL){
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &out_len); 
	if(out_len != sizeof(fbe_base_board_mgmt_get_port_driver_t)){
		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X out_len invalid\n", out_len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_object_lock_parent_list((fbe_base_object_t *)dell_board);
	parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) dell_board, base_board_mgmt_get_port_driver->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)dell_board);

		fbe_base_object_trace((fbe_base_object_t *) dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	base_board_mgmt_get_port_driver->port_driver = FBE_PORT_DRIVER_SAS_CPD;

	fbe_base_object_unlock_parent_list((fbe_base_object_t *)dell_board);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}
#endif

static fbe_status_t 
dell_board_init(fbe_dell_board_t * dell_board)
{
	fbe_status_t status;
	fbe_object_mgmt_attributes_t mgmt_attributes;
	fbe_u32_t i;

    KvTrace("%s entry\n", __FUNCTION__);

	/* Set first io_port index and number of io_ports */
	fbe_base_board_set_number_of_io_ports((fbe_base_board_t *)dell_board, FBE_DELL_BOARD_NUMBER_OF_IO_PORTS); 
	/* fbe_base_board_set_first_io_port_index((fbe_base_board_t *)dell_board, FBE_DELL_BOARD_FIRST_IO_PORT_INDEX); */

	/* Mark all our ports as invalid */
	for (i = 0; i < FBE_DELL_BOARD_NUMBER_OF_IO_PORTS; i++){
		dell_board->io_port_array[i] = FBE_BASE_BOARD_IO_PORT_INVALID;
	}

	/* Set initial flags */
	fbe_base_object_lock((fbe_base_object_t *)dell_board);
	dell_board->dell_board_state |= FBE_DELL_BOARD_UPDATE_HARDWARE_PORTS;

	status = fbe_base_object_get_mgmt_attributes((fbe_base_object_t *) dell_board, &mgmt_attributes);
	
	status = fbe_base_object_set_mgmt_attributes((fbe_base_object_t *) dell_board, mgmt_attributes);

	fbe_base_object_unlock((fbe_base_object_t *)dell_board);

	return FBE_STATUS_OK;
}



