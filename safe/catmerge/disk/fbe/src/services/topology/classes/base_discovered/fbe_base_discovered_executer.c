#include "base_discovered_private.h"
#include "fbe_transport_memory.h"

/* Forward declaration */
static fbe_status_t base_discovered_get_port_object_id_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_discovered_handle_edge_attr_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_discovered_edge_state_change_event_entry(fbe_base_discovered_t * base_discovered, fbe_event_context_t event_context);

static fbe_status_t base_discovered_discovery_transport_get_port_object_id(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);
static fbe_status_t base_discovered_check_attributes(fbe_base_discovered_t * base_discovered);

fbe_status_t 
fbe_base_discovered_send_get_port_object_id_command(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_packet_t * new_packet = NULL;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_u8_t * buffer = NULL;
	fbe_sg_element_t  * sg_list = NULL; 

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Allocate packet */
	new_packet = fbe_transport_allocate_packet();
	if(new_packet == NULL) {
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_transport_allocate_packet fail", __FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	buffer = fbe_transport_allocate_buffer();
	if(buffer == NULL) {
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_transport_allocate_buffer fail", __FUNCTION__);

		fbe_transport_release_packet(new_packet);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_initialize_packet(new_packet);

	payload = fbe_transport_get_payload_ex(new_packet);
	payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

	fbe_payload_discovery_build_get_port_object_id(payload_discovery_operation);

	/* Provide memory for the responce */
	sg_list = (fbe_sg_element_t  *)buffer;
	sg_list[0].count = sizeof(fbe_payload_discovery_get_port_object_id_data_t);
	sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

	sg_list[1].count = 0;
	sg_list[1].address = NULL;

	fbe_payload_ex_set_sg_list(payload, sg_list, 1);

	/* We need to assosiate newly allocated packet with original one */
	fbe_transport_add_subpacket(packet, new_packet);

	fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)base_discovered);
	fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)base_discovered, packet);

	status = fbe_transport_set_completion_function(new_packet, base_discovered_get_port_object_id_completion, base_discovered);

	/* Note we can not send the packet right away.
		It is possible that blocking is set or queue depth is exeded.
		So, we should "submit" "send" the packet to the bouncer for evaluation.
		Right now we have no bouncer implemented, but as we will have it we will use it.
	*/
	/* Another interesting thing - the discovery bouncer introdused in discoverING object */

	/* We are sending discovery packet via discovery edge */
	status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_discovered, new_packet);

	return status;
}

static fbe_status_t 
base_discovered_get_port_object_id_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_base_discovered_t * base_discovered = NULL;
	fbe_packet_t * master_packet = NULL;
	fbe_sg_element_t  * sg_list = NULL; 
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_payload_discovery_get_port_object_id_data_t * payload_discovery_get_port_object_id_data = NULL;

	base_discovered = (fbe_base_discovered_t *)context;

	master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_port_object_id_data = (fbe_payload_discovery_get_port_object_id_data_t *)sg_list[0].address;
	base_discovered->port_object_id = payload_discovery_get_port_object_id_data->port_object_id;

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s port_object_id = %X\n", __FUNCTION__, base_discovered->port_object_id);

	/* We need to remove packet from master queue */
	fbe_transport_remove_subpacket(packet);

	fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
	fbe_transport_release_buffer(sg_list);
	fbe_transport_release_packet(packet);

	/* Remove master packet from the termination queue. */
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)base_discovered, master_packet);

	fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(master_packet);

	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_base_discovered_send_handle_edge_attr_command(
*                           fbe_base_discovered_t * base_discovered, 
*                           fbe_packet_t * packet)                  
***************************************************************************
* @brief
*       This function sends down the corresponding command based on the discovery edge attributes. 
*       The FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST attribute take priority over the 
*       FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST attribute. 
*       It means if the edge is powered off and bypassed,
*       the power on command will be sent down first.
*
* @param   base_discovered - The pointer to the base discovered object.
* @param   packet - The pointer to the fbe_packet_t.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK - no error is found.
*        otherwise - the error is found.
*
* NOTES
*       
* HISTORY
*   12-Jan-2009 PHE - Created.
***************************************************************************/
fbe_status_t 
fbe_base_discovered_send_handle_edge_attr_command(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_control_operation_t * payload_control_operation = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_u8_t * buffer = NULL;
	fbe_sg_element_t  * sg_list = NULL; 
	fbe_path_attr_t path_attr;
	fbe_object_id_t my_object_id;

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);		

	payload = fbe_transport_get_payload_ex(packet);
	if(payload == NULL)
	{
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s NULL payload.\n", __FUNCTION__); 

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE; 
	}
     

	payload_control_operation = fbe_payload_ex_get_control_operation(payload);
	if(payload_control_operation == NULL)
	{
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s NULL payload_control_operation.\n", __FUNCTION__); 

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;		
	}
	

	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	/* The sg_list should not be used yet. Check it here. */
	if(sg_list != NULL) {
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s sg_list has already been used.\n", __FUNCTION__);

		fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	buffer = fbe_transport_allocate_buffer();
	if(buffer == NULL) {
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_transport_allocate_buffer fail.\n", __FUNCTION__);

		fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Provide memory for the response */
	sg_list = (fbe_sg_element_t  *)buffer;

	/* Allocate the discovery operation. */
	payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);
	
	status = fbe_base_discovered_get_path_attributes(base_discovered, &path_attr);

	fbe_base_object_get_object_id((fbe_base_object_t *)base_discovered, &my_object_id);

	if((status == FBE_STATUS_OK) && 
		(path_attr & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST | 
					  FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON))) {
		fbe_payload_discovery_build_power_on(payload_discovery_operation, my_object_id);
		sg_list[0].count = sizeof(fbe_payload_discovery_common_command_t);
	} 
	else if((status == FBE_STATUS_OK) && 
		(path_attr & FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST)) {
		fbe_payload_discovery_build_unbypass(payload_discovery_operation, my_object_id);
		sg_list[0].count = sizeof(fbe_payload_discovery_common_command_t);
	}
	else
	{
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, it should not happen, status: 0x%x, path_attr: 0x%x.\n",  __FUNCTION__, status, path_attr);
		
		fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
		fbe_transport_release_buffer(buffer);
		fbe_payload_ex_set_sg_list(payload, NULL, 0);
		fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

	sg_list[1].count = 0;
	sg_list[1].address = NULL;

	fbe_payload_ex_set_sg_list(payload, sg_list, 1);	

	status = fbe_transport_set_completion_function(packet, base_discovered_handle_edge_attr_completion, base_discovered);

	/* Note we can not send the packet right away.
		It is possible that blocking is set or queue depth is exeded.
		So, we should "submit" "send" the packet to the bouncer for evaluation.
		Right now we have no bouncer implemented, but as we will have it we will use it.
	*/
	/* Another interesting thing - the discovery bouncer introdused in discoverING object */

	/* We are sending discovery packet via discovery edge */
	status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_discovered, packet);

	return status;
}

/*!*************************************************************************
* @fn base_discovered_handle_edge_attr_completion(fbe_packet_t * packet, 
*                             fbe_packet_completion_context_t context)            
***************************************************************************
* @brief
*       This is the completion function for handling discovery edge attributes.
*       It sets up the control status based on the discovery status and 
*       releases the discovery operation and the buffer attached to the sg list.
*
* @param   packet - The pointer to the fbe_packet_t.
* @param   context - The pointer to the completion context.
*
* @return   fbe_status_t.
*        Always returns FBE_STATUS_OK. 
*
* NOTES
*       
* HISTORY
*   12-Jan-2009 PHE - Created.
***************************************************************************/
static fbe_status_t 
base_discovered_handle_edge_attr_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_base_discovered_t * base_discovered = NULL;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_payload_discovery_status_t discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_OK;
	fbe_payload_control_operation_t * payload_control_operation = NULL;
	fbe_payload_control_status_t  control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_sg_element_t  * sg_list = NULL;

	base_discovered = (fbe_base_discovered_t *)context;

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry.\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	if(payload != NULL)
	{
		payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
		if(payload_discovery_operation != NULL)
		{
			fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
			payload_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
			if(payload_control_operation != NULL) {
				control_status = (discovery_status == FBE_PAYLOAD_DISCOVERY_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
				fbe_payload_control_set_status(payload_control_operation, control_status);
			}
			fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
		}

		fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
		if(sg_list != NULL)
		{
			fbe_transport_release_buffer(sg_list);
			fbe_payload_ex_set_sg_list(payload, NULL, 0);
		}
	}
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_discovered_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	fbe_base_discovered_t * base_discovered = NULL;
	fbe_status_t status = FBE_STATUS_OK;
	fbe_path_state_t path_state;

	base_discovered = (fbe_base_discovered_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Let's see what kind of event we have */
	switch(event_type) {
		case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
				status = base_discovered_edge_state_change_event_entry(base_discovered, event_context);
			break;
		case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
			fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &path_state);
			if (FBE_PATH_STATE_ENABLED == path_state) {
				status = base_discovered_check_attributes(base_discovered);
			}
			break;
		case FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE:
			break;
		default:
				fbe_base_object_trace((fbe_base_object_t*)base_discovered,
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Uknown event_type %X\n", __FUNCTION__, event_type);

				status = FBE_STATUS_GENERIC_FAILURE;
				
			break;

	}

	return status;
}


static fbe_status_t 
base_discovered_edge_state_change_event_entry(fbe_base_discovered_t * base_discovered, fbe_event_context_t event_context)
{
	fbe_path_state_t path_state;
	fbe_status_t status;

	/* If discovery_edge state is invalid we should wait for the next monitor */
	status = fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &path_state);

	switch(path_state){
		case FBE_PATH_STATE_ENABLED:
			status = base_discovered_check_attributes(base_discovered);
			break;
		case FBE_PATH_STATE_DISABLED:
		case FBE_PATH_STATE_BROKEN:
		case FBE_PATH_STATE_SLUMBER:

			status = fbe_lifecycle_set_cond(&fbe_base_discovered_lifecycle_const,
								(fbe_base_object_t*)base_discovered,
								FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY);

			break;
		case FBE_PATH_STATE_GONE:
			fbe_base_object_trace((fbe_base_object_t*)base_discovered,
								FBE_TRACE_LEVEL_DEBUG_LOW,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Setting the lifecycle condition DESTROY.\n", __FUNCTION__);

			status = fbe_lifecycle_set_cond(&fbe_base_discovered_lifecycle_const,
											(fbe_base_object_t*)base_discovered,
											FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
			break;
		default:
			fbe_base_object_trace((fbe_base_object_t*)base_discovered,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Uknown path_state %X\n", __FUNCTION__, path_state);

			status = FBE_STATUS_GENERIC_FAILURE;
			break;

	}
	return status;
}

fbe_status_t 
fbe_base_discovered_discovery_transport_entry(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	/* fbe_io_block_t * io_block = NULL; */
	fbe_status_t status;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;

	fbe_payload_discovery_opcode_t discovery_opcode;

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are running in executer context, so we are good to extract and fill the data from the protocol */
	/* io_block = fbe_transport_get_io_block (packet); */
	payload = fbe_transport_get_payload_ex(packet);

	/* We are inside discovery transport entry - the operation has to be discovery operation */
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

	if(payload_discovery_operation == NULL){
		fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid operation",
								__FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);

	switch(discovery_opcode){
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID:
			status = base_discovered_discovery_transport_get_port_object_id(base_discovered, packet);
			break;

		case FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON:
		case FBE_PAYLOAD_DISCOVERY_OPCODE_UNBYPASS:
			fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s, opcode 0x%x should be handled in leaf class.\n",
								__FUNCTION__, discovery_opcode);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			status = FBE_STATUS_GENERIC_FAILURE;
			break;

		default:
			/* The base board object should handle all discovery packets */
			status = fbe_base_discovered_send_functional_packet(base_discovered, packet);
			break;
	}

	return status;
}

static fbe_status_t 
base_discovered_discovery_transport_get_port_object_id(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_port_object_id_data_t * payload_discovery_get_port_object_id_data = NULL;
	fbe_object_id_t port_object_id;

	fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are running in executer context, so we are good to extract and fill the data from the protocol */
	payload = fbe_transport_get_payload_ex(packet);

	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_port_object_id_data = (fbe_payload_discovery_get_port_object_id_data_t *)sg_list[0].address;

	fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *) base_discovered, &port_object_id);

	payload_discovery_get_port_object_id_data->port_object_id = port_object_id;

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn base_discovered_check_attributes(fbe_base_discovered_t * base_discovered)             
***************************************************************************
* @brief
*       This function checks the discovery edge attributes to set the corresponding conditions.
*
* @param   base_discovered - The pointer to the base discovered object.
*
* @return   fbe_status_t.
*        FBE_STATUS_OK - no error is found.
*        otherwise - the error is found.
*
* NOTES
*       
* HISTORY
*   12-Jan-2009 PHE - Added the check for other attributes.
***************************************************************************/
static fbe_status_t 
base_discovered_check_attributes(fbe_base_discovered_t * base_discovered)
{  
	fbe_status_t status;
	fbe_path_attr_t path_attr;

	status = fbe_base_discovered_get_path_attributes(base_discovered, &path_attr);    

	if(status == FBE_STATUS_OK) {
		if(((path_attr & FBE_DISCOVERY_PATH_ATTR_REMOVED) && 
	   		 (path_attr & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT)) ||
           (path_attr & FBE_DISCOVERY_PATH_ATTR_POWERED_ON_NEED_DESTROY))
        {
		   /* The discovery_edge path_attribute is REMOVED(physically not present)
			* and NOT_RPESENT(logged out), we should set destroy condition.
			* The reason to check the NOT PRESENT attribute is to prevent
			* from destroying the object while the inserted bit is flaky.
			*/
			fbe_base_object_trace((fbe_base_object_t*)base_discovered,
									FBE_TRACE_LEVEL_DEBUG_LOW,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s Set to destroy condition, path_attr: 0x%x.\n", __FUNCTION__, path_attr);	                      

			status = fbe_lifecycle_set_cond(&fbe_base_discovered_lifecycle_const,
											(fbe_base_object_t*)base_discovered,
											FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);

			if(status != FBE_STATUS_OK)
			{
				fbe_base_object_trace((fbe_base_object_t*)base_discovered,
	                      FBE_TRACE_LEVEL_ERROR,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
	                      "%s Can't set destroy condition, status: 0x%x.\n",
                          __FUNCTION__, status);
			}

		}
		else if(path_attr & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST |
							 FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST |
							 FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST |
							 FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST |
							 FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON)) {
			fbe_base_object_trace((fbe_base_object_t*)base_discovered,
									FBE_TRACE_LEVEL_DEBUG_LOW,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s Set to disc edge not ready condition, path_attr: 0x%x.\n", __FUNCTION__, path_attr);
	                      

			status = fbe_lifecycle_set_cond(&fbe_base_discovered_lifecycle_const,
											(fbe_base_object_t*)base_discovered,
											FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY);

			if(status != FBE_STATUS_OK)
			{
				fbe_base_object_trace((fbe_base_object_t*)base_discovered,
										FBE_TRACE_LEVEL_ERROR,
										FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
										"%s Can't set edge not ready condition, status: 0x%x.\n",
										__FUNCTION__, status);
			}
		}
		else { 
			/* Let's wake up the monitor thread */
			status = fbe_lifecycle_reschedule(&fbe_base_discovered_lifecycle_const,
                                      (fbe_base_object_t*)base_discovered,
                                      0);
		}
	}

	return status;
}

