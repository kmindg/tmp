#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"
#include "fbe_ses.h"
#include "sas_bullet_lcc_private.h"


/* Export class methods  */
fbe_class_methods_t fbe_sas_bullet_lcc_class_methods = {FBE_CLASS_ID_SAS_BULLET_LCC,
												        fbe_sas_bullet_lcc_load,
												        fbe_sas_bullet_lcc_unload,
												        fbe_sas_bullet_lcc_create_object,
												        fbe_sas_bullet_lcc_destroy_object,
												        fbe_sas_bullet_lcc_control_entry,
												        fbe_sas_bullet_lcc_event_entry,
                                                        fbe_sas_bullet_lcc_io_entry,
														fbe_sas_bullet_lcc_monitor_entry};


/* Forward declaration */
static fbe_status_t fbe_sas_bullet_lcc_get_encl_device_details(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet);

static fbe_status_t fbe_sas_bullet_lcc_get_encl_device_details_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_bullet_lcc_get_status_page(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet);

static fbe_status_t sas_bullet_lcc_get_status_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t sas_bullet_lcc_process_status_page(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_u8_t *resp_buffer);

static fbe_status_t fbe_sas_bullet_lcc_get_device_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_sas_bullet_lcc_get_device_info(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet);

static fbe_status_t sas_bullet_lcc_slot_monitor(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_u8_t *resp_buffer);

static fbe_status_t fbe_sas_bullet_lcc_get_parent_address(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet);

static fbe_status_t fbe_sas_bullet_lcc_turn_fault_led_on(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet, fbe_u32_t slot_id);

static fbe_status_t fbe_sas_bullet_lcc_turn_fault_led_on_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_ses_build_encl_control_data(fbe_u8_t *cmd, fbe_u32_t cmd_size);

static fbe_status_t fbe_sas_bullet_lcc_set_drive_fault_led(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet);

static fbe_status_t sas_bullet_lcc_init(fbe_sas_bullet_lcc_t * sas_bullet_lcc);


/* Monitors */
static fbe_status_t fbe_sas_bullet_lcc_monitor_instantiated(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet);
static fbe_status_t fbe_sas_bullet_lcc_monitor_specialized(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet);
static fbe_status_t fbe_sas_bullet_lcc_monitor_ready(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet);

/* fbe_bool_t first_time = TRUE; */
fbe_status_t 
fbe_sas_bullet_lcc_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_lcc_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_lcc_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_bullet_lcc_t * sas_bullet_lcc;
	fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_lcc_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_bullet_lcc, FBE_CLASS_ID_SAS_BULLET_LCC);	

	sas_bullet_lcc_init(sas_bullet_lcc);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_lcc_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc;

    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	
	sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	/* Check parent edges */
	/* Cleanup */

	/* Call parent destructor */
	status = fbe_sas_lcc_destroy_object(object_handle);
	return status;
}

fbe_status_t 
fbe_sas_bullet_lcc_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_BASE_OBJECT_CONTROL_CODE_MONITOR:
			status = fbe_sas_bullet_lcc_monitor(sas_bullet_lcc, packet);
			break;

        case FBE_BASE_OBJECT_CONTROL_CODE_GET_PARENT_ADDRESS:
			status = fbe_sas_bullet_lcc_get_parent_address(sas_bullet_lcc, packet);
			break;
        case FBE_BASE_LCC_CONTROL_CODE_SET_DRIVE_FAULT_LED:
            status = fbe_sas_bullet_lcc_set_drive_fault_led(sas_bullet_lcc, packet);
            break;

        /* Let the base object handle these control codes as they are
         * generic functionality which can be serviced by base object */
        case FBE_BASE_OBJECT_CONTROL_CODE_CREATE_EDGE:
        case FBE_BASE_OBJECT_CONTROL_CODE_INSERT_EDGE:
		default:
			status = fbe_sas_lcc_control_entry(object_handle, packet);
			break;
	}

	return status;
}

fbe_status_t
fbe_sas_bullet_lcc_monitor(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet)
{
	fbe_object_state_t mgmt_state;
	fbe_status_t status;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entered\n", __FUNCTION__);

	/* We need to think about locking here */
	fbe_base_object_get_mgmt_state((fbe_base_object_t *) sas_bullet_lcc, &mgmt_state);

	switch(mgmt_state) {
		case FBE_OBJECT_STATE_INSTANTIATED:
			status = fbe_sas_bullet_lcc_monitor_instantiated(sas_bullet_lcc, packet);
			break;
        case FBE_OBJECT_STATE_SPECIALIZED:
            status = fbe_sas_bullet_lcc_monitor_specialized(sas_bullet_lcc, packet);
            break;

        case FBE_OBJECT_STATE_READY:
            status = fbe_sas_bullet_lcc_monitor_ready(sas_bullet_lcc, packet);
            break;

		case FBE_OBJECT_STATE_DESTROY:
		case FBE_OBJECT_STATE_DESTROY_PENDING:
        default:
       		status = fbe_sas_lcc_monitor((fbe_sas_lcc_t *) sas_bullet_lcc, packet);
			break;
	}

	return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_monitor_instantiated(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_edge_state_t edge_state;
	fbe_physical_object_level_t physical_object_level;

	/* Check our edge state */
	fbe_base_object_get_edge_state((fbe_base_object_t *) sas_bullet_lcc, 0, &edge_state);
	if(edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

	/* Initialize physical_object_level. ( This functionality belong to base_object_monitor */
	status = fbe_base_object_get_physical_object_level((fbe_base_object_t *) sas_bullet_lcc, &physical_object_level);
	if(physical_object_level == FBE_PHYSICAL_OBJECT_LEVEL_INVALID){
		status = fbe_base_object_init_physical_object_level((fbe_base_object_t *) sas_bullet_lcc, packet);
		return status;
	}

	/* We are done with instantiated state - got to specialized */
	fbe_base_object_set_mgmt_state((fbe_base_object_t *)sas_bullet_lcc, FBE_OBJECT_STATE_SPECIALIZED);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sas_bullet_lcc_event_entry(fbe_object_handle_t object_handle, fbe_event_t event)
{
	return fbe_sas_lcc_event_entry(object_handle, event);
}

static fbe_status_t
fbe_sas_bullet_lcc_monitor_ready(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_edge_t   * edge = NULL; 

	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_bullet_lcc, 0, &edge);

	if(edge->edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_bullet_lcc, FBE_OBJECT_STATE_DESTROY_PENDING); 
		/* Our state is changed. Ask scheduler to pay attention to us */
		fbe_base_object_run_request((fbe_base_object_t *) sas_bullet_lcc);
     	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);	
		fbe_transport_complete_packet(packet);	
		return FBE_STATUS_OK;
	}

	if(sas_bullet_lcc->status_page_requiered == TRUE) {
		status = sas_bullet_lcc_get_status_page(sas_bullet_lcc, packet); 
		return status;
	}

    /* status = fbe_sas_bullet_lcc_turn_fault_led_on(sas_bullet_lcc, packet); */
    
	/* Pass control to the super */
	status = fbe_sas_lcc_monitor((fbe_sas_lcc_t *) sas_bullet_lcc, packet);
	return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_monitor_specialized(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_edge_t   * edge = NULL; 
	fbe_object_mgmt_attributes_t mgmt_attributes;

	status = fbe_base_object_get_edge((fbe_base_object_t *) sas_bullet_lcc, 0, &edge);

	/* Check if our edge is exist */
	if(edge->edge_state & FBE_EDGE_STATE_FLAG_NOT_EXIST){
		/* We lost our edge, so it is time to die */
		fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_bullet_lcc, FBE_OBJECT_STATE_DESTROY_PENDING); 
		fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    /*status = fbe_sas_bullet_lcc_get_encl_device_details(sas_bullet_lcc, packet);*/

	/* If the object was specialized by the a base class the constructor was not called, do the initialization here. */
	fbe_base_object_lock((fbe_base_object_t *)sas_bullet_lcc);
	status = fbe_base_object_get_mgmt_attributes((fbe_base_object_t *) sas_bullet_lcc, &mgmt_attributes);
	fbe_base_object_unlock((fbe_base_object_t *)sas_bullet_lcc);
	if (mgmt_attributes & FBE_BASE_OBJECT_FLAG_INIT_REQUIRED) {
		status = sas_bullet_lcc_init(sas_bullet_lcc);
		if (status != FBE_STATUS_OK) {
			fbe_transport_set_status(packet, status, 0);
		    fbe_transport_complete_packet(packet);
			return status;
		}
	}

	fbe_base_object_set_mgmt_state((fbe_base_object_t *) sas_bullet_lcc, FBE_OBJECT_STATE_READY);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

	return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_get_encl_device_details(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet)
{
	fbe_packet_t * sub_packet = NULL;
	fbe_sas_port_mgmt_ses_command_t *fbe_sas_port_mgmt_ses_command = NULL;
	fbe_status_t status;
    fbe_u32_t bus_id, target_id, lun_id;

	fbe_base_object_trace((fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	/* Allocate packet */
	sub_packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(sub_packet);
	/* Allocate buffer */
	fbe_sas_port_mgmt_ses_command = (fbe_sas_port_mgmt_ses_command_t *)fbe_transport_allocate_buffer();
    
    fbe_zero_memory(fbe_sas_port_mgmt_ses_command, sizeof(fbe_sas_port_mgmt_ses_command_t));

    status = fbe_sas_lcc_get_bus_id((fbe_sas_lcc_t *)sas_bullet_lcc, &bus_id);
    status = fbe_sas_lcc_get_target_id((fbe_sas_lcc_t *)sas_bullet_lcc, &target_id);
    status = fbe_sas_lcc_get_lun_id((fbe_sas_lcc_t *)sas_bullet_lcc, &lun_id);

    fbe_sas_port_mgmt_ses_command->bus_id = bus_id;
    fbe_sas_port_mgmt_ses_command->target_id = target_id;
    fbe_sas_port_mgmt_ses_command->lun_id = lun_id;
    status =  fbe_ses_build_encl_config_page(&fbe_sas_port_mgmt_ses_command->cdb[0], 
                                             SAS_PORT_MAX_SES_CDB_SIZE, 
                                             FBE_SES_ENCLOSURE_CONFIGURATION_PAGE_SIZE);

    fbe_sas_port_mgmt_ses_command->cdb_size = FBE_SES_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE;
    fbe_sas_port_mgmt_ses_command->resp_buffer_size = FBE_SES_ENCLOSURE_CONFIGURATION_PAGE_SIZE;

	fbe_transport_build_control_packet(sub_packet, 
								FBE_SAS_PORT_CONTROL_CODE_SEND_SCSI_COMMAND,
								fbe_sas_port_mgmt_ses_command,
								sizeof(fbe_sas_port_mgmt_ses_command_t),
								sizeof(fbe_sas_port_mgmt_ses_command_t),
								fbe_sas_bullet_lcc_get_encl_device_details_completion, 
								sas_bullet_lcc);

    /* Put our subpacket on the mgmt packet's queue of subpackets. */
	
	fbe_transport_add_subpacket(packet, sub_packet);

	/* Put our mgmt packet on the termination queue while we wait for the subpacket to complete. */
	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_bullet_lcc);
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_bullet_lcc, packet);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_bullet_lcc, sub_packet, 0); /* for now hard coded control edge */

    return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_get_encl_device_details_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context)
{
	fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)context;
	fbe_sas_port_mgmt_ses_command_t *fbe_sas_port_mgmt_ses_command = NULL;
	fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);  

	master_packet = fbe_transport_get_master_packet(sub_packet); 

    payload = fbe_transport_get_payload_ex(sub_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  
	fbe_payload_control_get_buffer(control_operation, &fbe_sas_port_mgmt_ses_command); 
    
    /* Remove the subpacket from the mgmt packet's subpacket queue. */
	fbe_transport_remove_subpacket(sub_packet);
    
    /* Remove the mgmt packet from the termination queue */
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_bullet_lcc, master_packet);

    /* Release the resources of mgmt packet's subpacket queue. */
	

	status = fbe_transport_get_status_code(sub_packet);

	if(status != FBE_STATUS_OK){ /* Something bad happen to child, just release packet */
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Failed to send scsi\n\n", __FUNCTION__);
	}
    else {
	    /* Ask scheduler to pay attention to us */
	    fbe_base_object_run_request((fbe_base_object_t *) sas_bullet_lcc);
    }

	fbe_transport_release_buffer(fbe_sas_port_mgmt_ses_command);

    /* Release the subpacket itself. */
	fbe_transport_release_packet(sub_packet);

	/* We HAVE to check if subpacket queue is empty */
	fbe_transport_set_status(master_packet, status, 0);
	fbe_transport_complete_packet(master_packet);
	return status;
}

static fbe_status_t 
sas_bullet_lcc_get_status_page(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet)
{
	fbe_packet_t * new_packet = NULL;
	fbe_u8_t * buffer = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_io_block_t * io_block = NULL;
	fbe_u8_t * cdb = NULL;
	fbe_status_t status;
	fbe_cpd_device_id_t cpd_device_id;
	fbe_io_block_t *	in_io_block = fbe_transport_get_io_block (packet);

	fbe_base_object_trace((fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);
    
	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if (new_packet == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_transport_allocate_packet failed\n",
								__FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	buffer = fbe_transport_allocate_buffer();
	if (buffer == NULL){
		fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, fbe_transport_allocate_buffer failed\n",
								__FUNCTION__);

		fbe_transport_release_packet(new_packet);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_list = (fbe_sg_element_t  *)buffer;
	sg_list[0].count = FBE_SES_ENCLOSURE_STATUS_PAGE_SIZE;
	sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

	sg_list[1].count = 0;
	sg_list[1].address = NULL;
    
	fbe_transport_initialize_packet(new_packet);
	fbe_transport_set_completion_function(new_packet, sas_bullet_lcc_get_status_page_completion, sas_bullet_lcc);
	
	io_block = fbe_transport_get_io_block(new_packet);
	io_block_set_sg_list(io_block, sg_list);
	
	fbe_sas_lcc_get_cpd_device_id((fbe_sas_lcc_t *) sas_bullet_lcc, &cpd_device_id);

    io_block_convert_to_execute_cdb (in_io_block, cpd_device_id, io_block);

	cdb = fbe_io_block_get_cdb(io_block);

	fbe_ses_build_encl_status_page(cdb, SAS_PORT_MAX_SES_CDB_SIZE, FBE_SES_ENCLOSURE_STATUS_PAGE_SIZE);

    /* Put our new packet on the packet's queue of subpackets. */	
	fbe_transport_add_subpacket(packet, new_packet);

	/* Put our packet on the termination queue while we wait for the subpacket
	 * with the unbypass command to complete.
	 */
	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_bullet_lcc);
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_bullet_lcc, packet);

	status = fbe_base_object_send_io_packet((fbe_base_object_t *) sas_bullet_lcc, new_packet, 0); /* for now hard coded control edge */

	return status;
}

static fbe_status_t
sas_bullet_lcc_get_status_page_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)context;
	fbe_packet_t * master_packet = NULL;
	fbe_sg_element_t  * sg_list = NULL; 
	fbe_io_block_t * io_block;
    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	master_packet = fbe_transport_get_master_packet(packet); 

	/* We HAVE to check status of packet before we continue */
	io_block = fbe_transport_get_io_block(packet);
	sg_list = io_block_get_sg_list(io_block);
    
    /* Remove the packet from the  master_packet subpacket queue. */
	fbe_transport_remove_subpacket(packet);
    
    /* Remove the master_packet from the termination queue */
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_bullet_lcc, master_packet);	

	status = fbe_transport_get_status_code(packet);

	if(status != FBE_STATUS_OK){ /* Something bad happen to child, just release packet */
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Failed\n", __FUNCTION__);
	    fbe_transport_release_buffer(sg_list);
		fbe_transport_release_packet(packet);

        /* We HAVE to check if subpacket queue is empty */
        fbe_transport_set_status(master_packet, status, 0);
        fbe_transport_complete_packet(master_packet);
		return FBE_STATUS_OK;
	}

	sas_bullet_lcc_process_status_page(sas_bullet_lcc, sg_list[0].address);

	sas_bullet_lcc->status_page_requiered = FALSE;
	/* Ask scheduler to pay attention to us */
	fbe_base_object_run_request((fbe_base_object_t *) sas_bullet_lcc);

	fbe_transport_release_buffer(sg_list);
	fbe_transport_release_packet(packet);

    /* We HAVE to check if subpacket queue is empty */
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_bullet_lcc_process_status_page(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_u8_t *resp_buffer)
{
 	fbe_u32_t number_of_slots = 0;
	fbe_u32_t i = 0;
	fbe_u32_t sas_connector_index = 0;
	fbe_u32_t sas_connector_count = 0;
	fbe_ses_status_element_t sas_connector_status_element;

    fbe_base_lcc_get_number_of_slots((fbe_base_lcc_t *)sas_bullet_lcc, &number_of_slots);
	

	for(i = 1; i < number_of_slots +1 ; i++){/* "+ 1" to count Overall status element */
		fbe_ses_get_status_element(resp_buffer, i, (fbe_ses_status_element_t *)&sas_bullet_lcc->drive_info[i].slot_status_element);
	}

	fbe_sas_lcc_get_sas_connector_descriptor((fbe_sas_lcc_t *) sas_bullet_lcc, &sas_connector_index, &sas_connector_count);
    fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: sas_connector_index %d,  sas_connector_count %d\n",
							 __FUNCTION__,sas_connector_index, sas_connector_count);

	for(i = 0; i < sas_connector_count + 1; i++){ /* "+ 1" to count Overall status element */
		fbe_ses_get_status_element(resp_buffer, sas_connector_index + i, &sas_connector_status_element);
	}

	return FBE_STATUS_OK;
}

static fbe_status_t fbe_sas_bullet_lcc_get_device_info(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet)
{
    fbe_packet_t * sub_packet = NULL;
	fbe_sas_port_mgmt_ses_command_t *fbe_sas_port_mgmt_ses_command = NULL;
	fbe_status_t status;
   	fbe_semaphore_t sem;
    fbe_u32_t bus_id, target_id, lun_id;

    fbe_semaphore_init(&sem, 0, 1);

	fbe_base_object_trace((fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

     
	/* Allocate packet */
	sub_packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(sub_packet);

    
	/* Allocate buffer */
	fbe_sas_port_mgmt_ses_command = (fbe_sas_port_mgmt_ses_command_t *)fbe_transport_allocate_buffer();
    
    fbe_zero_memory(fbe_sas_port_mgmt_ses_command,sizeof(fbe_sas_port_mgmt_ses_command_t));

    status = fbe_sas_lcc_get_bus_id((fbe_sas_lcc_t *)sas_bullet_lcc, &bus_id);
    status = fbe_sas_lcc_get_target_id((fbe_sas_lcc_t *)sas_bullet_lcc, &target_id);
    status = fbe_sas_lcc_get_lun_id((fbe_sas_lcc_t *)sas_bullet_lcc, &lun_id);

    fbe_sas_port_mgmt_ses_command->bus_id = bus_id;
    fbe_sas_port_mgmt_ses_command->target_id = target_id;
    fbe_sas_port_mgmt_ses_command->lun_id = lun_id;
    status =  fbe_ses_build_additional_status_page(&fbe_sas_port_mgmt_ses_command->cdb[0], 
                                                   SAS_PORT_MAX_SES_CDB_SIZE, 
                                                   FBE_SES_ADDITIONAL_STATUS_PAGE_SIZE);

    fbe_sas_port_mgmt_ses_command->cdb_size = FBE_SES_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE;
    fbe_sas_port_mgmt_ses_command->resp_buffer_size = FBE_SES_ADDITIONAL_STATUS_PAGE_SIZE;

	fbe_transport_build_control_packet(sub_packet, 
								FBE_SAS_PORT_CONTROL_CODE_SEND_SCSI_COMMAND,
								fbe_sas_port_mgmt_ses_command,
								sizeof(fbe_sas_port_mgmt_ses_command_t),
								sizeof(fbe_sas_port_mgmt_ses_command_t),
								fbe_sas_bullet_lcc_get_device_info_completion, 
								sas_bullet_lcc);

    
    /* Put our subpacket on the mgmt packet's queue of subpackets. */
	
    
	fbe_transport_add_subpacket(packet, sub_packet);

	/* Put our mgmt packet on the termination queue while we wait for the subpacket
	 * with the unbypass command to complete.
	 */
	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_bullet_lcc);
    
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_bullet_lcc, packet);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_bullet_lcc, sub_packet, 0); /* for now hard coded control edge */
	return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_get_device_info_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)context;
	fbe_sas_port_mgmt_ses_command_t *fbe_sas_port_mgmt_ses_command = NULL;
	fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	master_packet = fbe_transport_get_master_packet(sub_packet); 
    payload = fbe_transport_get_payload_ex(sub_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
	fbe_payload_control_get_buffer(control_operation, &fbe_sas_port_mgmt_ses_command);
    
    /* Remove the subpacket from the mgmt packet's subpacket queue. */
	fbe_transport_remove_subpacket(sub_packet);
    
    /* Remove the mgmt packet from the termination queue */
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_bullet_lcc, master_packet);

    /* Release the resources of mgmt packet's subpacket queue. */
	

	status = fbe_transport_get_status_code(sub_packet);

	if(status != FBE_STATUS_OK){ /* Something bad happen to child, just release packet */
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Failed to send scsi\n", __FUNCTION__);
	}
    else {
        status = sas_bullet_lcc_slot_monitor(sas_bullet_lcc, fbe_sas_port_mgmt_ses_command->resp_buffer);
	    /* Ask scheduler to pay attention to us */
	   fbe_base_object_run_request((fbe_base_object_t *) sas_bullet_lcc);
    }

 	fbe_transport_release_buffer(fbe_sas_port_mgmt_ses_command);
    /* Release the subpacket itself. */
	fbe_transport_release_packet(sub_packet);

    /* We HAVE to check if subpacket queue is empty */
	fbe_transport_set_status(master_packet, status, 0);
	fbe_transport_complete_packet(master_packet);
	return status;
}

/*!
 * \ingroup FBE_SAS_BULLET_LCC_INTERNAL_FUNCTION
 *
 * \fn static fbe_status_t fibre_lcc_slot_monitor(fbe_fibre_lcc_t * fibre_lcc, fbe_packet_t * packet)
 *
 * This is a monitor function called out of the READY execution flow.  It correlates the existence of edges
 * with the bits in the LCC's shadow registers to decide if there have been any drive slot state changes.
 *
 * \param fibre_lcc
 * 		A pointer to the Fibre LCC object.
 *
 * \param packet
 * 		A pointer to the management packet used to drive the LCC monitor.
 *
 * \return
 * 		FBE_STATUS_OK if no slot operations are needed, FBE_STATUS_PENDING if a slot operation
 * 		is pending, otherwise an error status code is returned.
 */
static fbe_status_t 
sas_bullet_lcc_slot_monitor(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_u8_t *resp_buffer)
{
    fbe_u32_t i = 0;
	fbe_edge_index_t first_slot_index = 0;
	fbe_base_edge_t * parent_edge = NULL;
    /*fbe_sas_address_t sas_address;*/
	/*fbe_status_t status;*/
	fbe_u32_t number_of_slots = 0;

	fbe_base_lcc_get_number_of_slots((fbe_base_lcc_t *)sas_bullet_lcc, &number_of_slots);
	fbe_base_lcc_get_first_slot_index((fbe_base_lcc_t *) sas_bullet_lcc, &first_slot_index);

#if 0
    for(i = 0; i < number_of_slots; i++){

        status = fbe_ses_get_drive_sas_address(resp_buffer, i, &sas_address);
        sas_bullet_lcc->drive_info[i].sas_address = sas_address;

		/* This function called in monitor context, so we do not need to abtain the lock */
    	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_bullet_lcc);
		parent_edge = fbe_base_object_get_parent_by_index((fbe_base_object_t *)sas_bullet_lcc, first_slot_index + i);
		if(parent_edge == NULL){ /* This slot have no parent */
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_bullet_lcc);
			if (sas_bullet_lcc->drive_info[i].drive_status == 0x01) { /* The drive is present */
				status = fbe_base_object_instantiate_parent((fbe_base_object_t *) sas_bullet_lcc, FBE_CLASS_ID_SAS_DRIVE, first_slot_index + i);
			}
		} else { /* Just update the edge state */
			/* update the edge state */
			fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_bullet_lcc);
			if (sas_bullet_lcc->drive_info[i].drive_status == 0x05) { /* The drive NOT present, but we have parent - let's destroy edge  */
				fbe_base_object_destroy_parent_edge((fbe_base_object_t *)sas_bullet_lcc, parent_edge->parent_id);
			}
		}
	}
#endif

	return FBE_STATUS_OK;
}

static fbe_status_t
fbe_sas_bullet_lcc_get_parent_address(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet)
{
	fbe_base_object_mgmt_get_parent_address_t * base_object_mgmt_get_parent_address = NULL;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer_length(control_operation, &in_len); 
	fbe_payload_control_get_buffer_length(control_operation, &out_len); 

	if(in_len != sizeof(fbe_base_object_mgmt_get_parent_address_t)){
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(out_len != sizeof(fbe_base_object_mgmt_get_parent_address_t)){
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_get_parent_address); 
	if(base_object_mgmt_get_parent_address == NULL){
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Ifbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* As a sas_lsi port we supposed to have one parent only with this particular id */
	/* Since we are dealing with the parent_list we have to grab the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_bullet_lcc);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_bullet_lcc, base_object_mgmt_get_parent_address->parent_id);
	if(parent_edge == NULL){
		fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_bullet_lcc);

		fbe_base_object_trace((fbe_base_object_t *) sas_bullet_lcc, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_object_get_parent_by_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    /* Translate parent_edge->child_index to SAS Address */
	base_object_mgmt_get_parent_address->address.sas_address = sas_bullet_lcc->drive_info[parent_edge->child_index].sas_address;

	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_bullet_lcc);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t fbe_sas_bullet_lcc_turn_fault_led_on(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t *packet, fbe_u32_t slot_id)
{
    fbe_packet_t * sub_packet = NULL;
	fbe_sas_port_mgmt_ses_command_t *fbe_sas_port_mgmt_ses_command = NULL;
	fbe_status_t status;
   	fbe_semaphore_t sem;
    fbe_u32_t bus_id, target_id, lun_id;

    fbe_semaphore_init(&sem, 0, 1);

	fbe_base_object_trace((fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

     
	/* Allocate packet */
	sub_packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(sub_packet);

    
	/* Allocate buffer */
	fbe_sas_port_mgmt_ses_command = (fbe_sas_port_mgmt_ses_command_t *)fbe_transport_allocate_buffer();
    
    fbe_zero_memory(fbe_sas_port_mgmt_ses_command,sizeof(fbe_sas_port_mgmt_ses_command_t));

    status = fbe_sas_lcc_get_bus_id((fbe_sas_lcc_t *)sas_bullet_lcc, &bus_id);
    status = fbe_sas_lcc_get_target_id((fbe_sas_lcc_t *)sas_bullet_lcc, &target_id);
    status = fbe_sas_lcc_get_lun_id((fbe_sas_lcc_t *)sas_bullet_lcc, &lun_id);

    fbe_sas_port_mgmt_ses_command->bus_id = bus_id;
    fbe_sas_port_mgmt_ses_command->target_id = target_id;
    fbe_sas_port_mgmt_ses_command->lun_id = lun_id;
    status =  fbe_ses_build_encl_control_page(&fbe_sas_port_mgmt_ses_command->cdb[0], 
                                              SAS_PORT_MAX_SES_CDB_SIZE, 
                                              FBE_SES_ENCLOSURE_CONTROL_PAGE_SIZE);

    status = fbe_ses_build_encl_control_cmd(&fbe_sas_port_mgmt_ses_command->cmd[0],
                                             FBE_SES_ENCLOSURE_CONTROL_DATA_SIZE);

    status = fbe_ses_drive_control_fault_led_flash_onoff(slot_id,
                                                         &fbe_sas_port_mgmt_ses_command->cmd[0],
                                                         FBE_SES_ENCLOSURE_CONTROL_DATA_SIZE,
                                                         TRUE);

    fbe_sas_port_mgmt_ses_command->cdb_size = FBE_SES_SEND_DIAGNOSTIC_RESULTS_CDB_SIZE;
    fbe_sas_port_mgmt_ses_command->resp_buffer_size = FBE_SES_ENCLOSURE_CONTROL_PAGE_SIZE;
    fbe_sas_port_mgmt_ses_command->cmd_size = FBE_SES_ENCLOSURE_CONTROL_PAGE_SIZE;

	fbe_transport_build_control_packet(sub_packet, 
								FBE_SAS_PORT_CONTROL_CODE_SEND_SCSI_COMMAND,
								fbe_sas_port_mgmt_ses_command,
								sizeof(fbe_sas_port_mgmt_ses_command_t),
								sizeof(fbe_sas_port_mgmt_ses_command_t),
								fbe_sas_bullet_lcc_turn_fault_led_on_completion, 
								sas_bullet_lcc);

    
    /* Put our subpacket on the mgmt packet's queue of subpackets. */
	
    
	fbe_transport_add_subpacket(packet, sub_packet);

	/* Put our mgmt packet on the termination queue while we wait for the subpacket
	 * with the unbypass command to complete.
	 */
	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_bullet_lcc);
    
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_bullet_lcc, packet);

	status = fbe_base_object_send_packet((fbe_base_object_t *) sas_bullet_lcc, sub_packet, 0); /* for now hard coded control edge */
	return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_turn_fault_led_on_completion(fbe_packet_t * sub_packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)context;
	fbe_sas_port_mgmt_ses_command_t *fbe_sas_port_mgmt_ses_command = NULL;
	fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    
	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);  

	master_packet = fbe_transport_get_master_packet(sub_packet); 
    
    payload = fbe_transport_get_payload_ex(sub_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  
	fbe_payload_control_get_buffer(control_operation, &fbe_sas_port_mgmt_ses_command); 
    
    /* Remove the subpacket from the mgmt packet's subpacket queue. */
	fbe_transport_remove_subpacket(sub_packet);
    
    /* Remove the mgmt packet from the termination queue */
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_bullet_lcc, master_packet);

    /* Release the resources of mgmt packet's subpacket queue. */
	

	status = fbe_transport_get_status_code(sub_packet);

	fbe_transport_release_buffer(fbe_sas_port_mgmt_ses_command);
    /* Release the subpacket itself. */
	fbe_transport_release_packet(sub_packet);

	/* We HAVE to check if subpacket queue is empty */
	fbe_transport_set_status(master_packet, status, 0);
	fbe_transport_complete_packet(master_packet);
	return status;
}

static fbe_status_t
fbe_sas_bullet_lcc_set_drive_fault_led(fbe_sas_bullet_lcc_t * sas_bullet_lcc, fbe_packet_t * packet)
{
	fbe_base_lcc_mgmt_set_drive_fault_led_t *fbe_base_lcc_mgmt_set_drive_fault_led;
	fbe_base_edge_t * parent_edge = NULL;
	fbe_payload_control_buffer_length_t in_len = 0;
	fbe_payload_control_buffer_length_t out_len = 0;
    fbe_u32_t slot_id;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s function entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

	fbe_payload_control_get_buffer_length(control_operation, &in_len); 
	fbe_payload_control_get_buffer_length(control_operation, &out_len); 

	if(in_len != sizeof(fbe_base_lcc_mgmt_set_drive_fault_led_t)){
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid in_len %X\n", __FUNCTION__, in_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if(out_len != sizeof(fbe_base_lcc_mgmt_set_drive_fault_led_t)){
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: Invalid out_len %X\n", __FUNCTION__, out_len);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &fbe_base_lcc_mgmt_set_drive_fault_led); 
	if(fbe_base_lcc_mgmt_set_drive_fault_led == NULL){
		fbe_topology_class_trace(FBE_CLASS_ID_SAS_BULLET_LCC,
		                         FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: fbe_transport_get_buffer failed \n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* As a sas_lsi port we supposed to have one parent only with this particular id */
	/* Since we are dealing with the parent_list we have to grub the lock */
	fbe_base_object_lock_parent_list((fbe_base_object_t *)sas_bullet_lcc);
    parent_edge = fbe_base_object_get_parent_by_id((fbe_base_object_t *) sas_bullet_lcc, fbe_base_lcc_mgmt_set_drive_fault_led->parent_id);
	fbe_base_object_unlock_parent_list((fbe_base_object_t *)sas_bullet_lcc);
    if(parent_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_bullet_lcc, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: fbe_base_object_get_parent_by_id failed, parent id: %d\n",
								__FUNCTION__, fbe_base_lcc_mgmt_set_drive_fault_led->parent_id);
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    slot_id = parent_edge->child_index;
    status = fbe_sas_bullet_lcc_turn_fault_led_on(sas_bullet_lcc, packet, slot_id);
    return status;
}


static fbe_status_t 
sas_bullet_lcc_init(fbe_sas_bullet_lcc_t * sas_bullet_lcc)
{
	fbe_object_mgmt_attributes_t mgmt_attributes;
	fbe_status_t status;

	/* Set number of slots */
	fbe_base_lcc_set_number_of_slots((fbe_base_lcc_t *)sas_bullet_lcc, FBE_SAS_BULLET_LCC_NUMBER_OF_SLOTS);
	fbe_base_lcc_set_first_slot_index((fbe_base_lcc_t *)sas_bullet_lcc, FBE_SAS_BULLET_LCC_FIRST_SLOT_INDEX);

	fbe_base_lcc_set_number_of_expansion_ports((fbe_base_lcc_t *)sas_bullet_lcc, FBE_SAS_BULLET_LCC_NUMBER_OF_EXPANSION_PORTS);
	fbe_base_lcc_set_first_expansion_port_index((fbe_base_lcc_t *)sas_bullet_lcc, FBE_SAS_BULLET_LCC_FIRST_EXPANSION_PORT_INDEX);

	sas_bullet_lcc->status_page_requiered = TRUE;

	fbe_base_object_lock((fbe_base_object_t *)sas_bullet_lcc);
	status = fbe_base_object_get_mgmt_attributes((fbe_base_object_t *)sas_bullet_lcc, &mgmt_attributes);
	if (status == FBE_STATUS_OK) {
		mgmt_attributes &= ~FBE_BASE_OBJECT_FLAG_INIT_REQUIRED;
		status = fbe_base_object_set_mgmt_attributes((fbe_base_object_t *)sas_bullet_lcc, mgmt_attributes);
	}
	fbe_base_object_unlock((fbe_base_object_t *)sas_bullet_lcc);
	
	return status;
}

static fbe_status_t 
fbe_sas_bullet_lcc_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = NULL;

	sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	return fbe_base_object_send_io_packet((fbe_base_object_t *) sas_bullet_lcc, packet, 0); /* for now hard coded control edge */
}
