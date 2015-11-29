
#include "sas_pmc_port_private.h"
#include "fbe/fbe_stat_api.h"
#include "fbe/fbe_enclosure.h"
//#include "fbe_pmc_shim.h"

/* Forward declaration */

static fbe_status_t sas_pmc_port_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_ssp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_status_t sas_pmc_port_discovery_transport_get_protocol_address(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_discovery_transport_get_port_object_id(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_discovery_transport_get_element_list(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_discovery_transport_record_fw_activation_status(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_discovery_transport_get_server_info(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_payload_discovery_element_type_t sas_pmc_port_conver_shim_device_type_to_discovery_element_type(fbe_cpd_shim_discovery_device_type_t device_type);
static fbe_payload_smp_element_type_t sas_pmc_port_conver_shim_device_type_to_smp_element_type(fbe_cpd_shim_discovery_device_type_t device_type);
static fbe_status_t sas_pmc_port_stp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_smp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_smp_transport_get_element_list(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_smp_transport_reset_end_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_smp_transport_set_element_status(fbe_payload_smp_element_extended_status_t * element_status, 
                                                    fbe_cpd_shim_device_login_reason_t device_login_reason);
static fbe_status_t sas_pmc_port_smp_transport_get_element_status(fbe_sas_pmc_port_t * sas_pmc_port, fbe_cpd_device_id_t	 cpd_device_id,
                                              fbe_payload_smp_element_extended_status_t * element_status);


static fbe_status_t sas_pmc_port_bypass_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t sas_pmc_port_ssp_transport_entry_async_start(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*!*************************************************************************
* @fn fbe_sas_pmc_port_io_entry(                  
*                    fbe_object_handle_t object_handle, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*			IO entry function.
*
* @param      object_handle - The handle to the port object.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
fbe_status_t 
fbe_sas_pmc_port_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_transport_id_t transport_id;
	fbe_status_t status = FBE_STATUS_OK;

	sas_pmc_port = (fbe_sas_pmc_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
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
			status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) sas_pmc_port,
																		(fbe_transport_entry_t)sas_pmc_port_discovery_transport_entry,
																		packet);
			break;
		case FBE_TRANSPORT_ID_SSP:
			/* The server part of ssp_transport is a member of sas_port class.
				Even more than that, we do not expect to receive ssp protocol packets
				for "non sas port" objects 
			*/

			status = fbe_sas_port_ssp_transport_entry((fbe_sas_port_t *) sas_pmc_port,
													(fbe_transport_entry_t)sas_pmc_port_ssp_transport_entry,
													packet);

			break;
		case FBE_TRANSPORT_ID_STP:
			/* The server part of ssp_transport is a member of sas_port class.
				Even more than that, we do not expect to receive ssp protocol packets
				for "non sas port" objects 
			*/

			status = fbe_sas_port_stp_transport_entry((fbe_sas_port_t *) sas_pmc_port,
													(fbe_transport_entry_t)sas_pmc_port_stp_transport_entry,
													packet);

			break;

		case FBE_TRANSPORT_ID_SMP:
			status = fbe_sas_port_smp_transport_entry((fbe_sas_port_t *) sas_pmc_port,
													(fbe_transport_entry_t)sas_pmc_port_smp_transport_entry,
													packet);
			break;
	}

	return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_transport_entry(                  
*                    fbe_base_object_t * base_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*		 Discovery transport entry.
*
* @param      base_object - - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_payload_discovery_opcode_t discovery_opcode;
	fbe_status_t status;

	sas_pmc_port = (fbe_sas_pmc_port_t *)base_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
	
	switch(discovery_opcode){
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID:
				status = sas_pmc_port_discovery_transport_get_port_object_id(sas_pmc_port, packet);
			break;
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS:
				status = sas_pmc_port_discovery_transport_get_protocol_address(sas_pmc_port, packet);
			break;
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_ELEMENT_LIST:
				status = sas_pmc_port_discovery_transport_get_element_list(sas_pmc_port, packet);
			break;
		case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SERVER_INFO:
			status = sas_pmc_port_discovery_transport_get_server_info(sas_pmc_port, packet);
			break;
		case FBE_PAYLOAD_DISCOVERY_OPCODE_NOTIFY_FW_ACTIVATION_STATUS:
			status = sas_pmc_port_discovery_transport_record_fw_activation_status(sas_pmc_port, packet);
			break;
		default:
			fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Unknown discovery_opcode %X\n", __FUNCTION__, discovery_opcode);

			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			status = FBE_STATUS_GENERIC_FAILURE;
			break;
	}

	return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_transport_get_protocol_address(                  
*                    fbe_sas_pmc_port_t * sas_pmc_port, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*			Returns the sas address of the direct attached enclosure.
*
* @param      sas_pmc_port - The pointer to the fbe_sas_pmc_port_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_discovery_transport_get_protocol_address(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_protocol_address_data_t * payload_discovery_get_protocol_address_data = NULL;	
	fbe_status_t status;
	fbe_edge_index_t server_index = 0;
	fbe_u32_t  table_index;
	fbe_sas_pmc_device_table_entry_t *this_entry = NULL;
    fbe_payload_discovery_status_t  payload_discovery_status;
	fbe_bool_t wait_for_kek_reestablish = FBE_FALSE;
	fbe_key_handle_t	kek_kek_handle;
	fbe_key_handle_t 	kek_handle;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_protocol_address_data = (fbe_payload_discovery_get_protocol_address_data_t *)sg_list[0].address;

	/* As a for now we have one client only with this particular id */
	status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) sas_pmc_port, 
														payload_discovery_operation->command.get_protocol_address_command.client_id,
														&server_index);
	if(status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_base_discovering_get_server_index_by_client_id fail\n");

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_port_get_kek_kek_handle((fbe_base_port_t *) sas_pmc_port, &kek_kek_handle);

	if(kek_kek_handle != FBE_INVALID_KEY_HANDLE) {
		/* This is encrypted system. Check to make sure the kek is also established */
		fbe_base_port_get_kek_handle((fbe_base_port_t *) sas_pmc_port, &kek_handle);
		if(kek_handle == FBE_INVALID_KEY_HANDLE) {
			fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								" Encrypted system wait for keys to be setup\n");
			wait_for_kek_reestablish = FBE_TRUE;
		}
	}
	fbe_spinlock_lock(&sas_pmc_port->list_lock);
	if (sas_pmc_port->da_child_index != FBE_SAS_PMC_INVALID_TABLE_INDEX){
		table_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(sas_pmc_port->da_child_index);	
		this_entry = &(sas_pmc_port->device_table_ptr[table_index]);
		if ((this_entry->device_logged_in) &&
			 FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(this_entry->parent_device_id) &&
			!wait_for_kek_reestablish)
		{
			payload_discovery_get_protocol_address_data->address.sas_address = this_entry->device_address;
			payload_discovery_get_protocol_address_data->generation_code = this_entry->generation_code; 
			payload_discovery_get_protocol_address_data->chain_depth = this_entry->device_locator.enclosure_chain_depth; 
            payload_discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_OK;			
		}
		else
		{
			payload_discovery_get_protocol_address_data->address.sas_address = FBE_SAS_ADDRESS_INVALID;
			payload_discovery_get_protocol_address_data->generation_code = 0;
			payload_discovery_get_protocol_address_data->chain_depth = 0;
            payload_discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE;			
		}
	}else{
		payload_discovery_get_protocol_address_data->address.sas_address = FBE_SAS_ADDRESS_INVALID;
		payload_discovery_get_protocol_address_data->generation_code = 0;
		payload_discovery_get_protocol_address_data->chain_depth = 0;
        payload_discovery_status = FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE;		
	}
	fbe_spinlock_unlock(&sas_pmc_port->list_lock);

    fbe_payload_discovery_set_status(payload_discovery_operation,payload_discovery_status);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_transport_get_port_object_id(                  
*                    fbe_sas_pmc_port_t * sas_pmc_port, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*			Returns the port's object ID.
*
* @param      sas_pmc_port - The pointer to the fbe_sas_pmc_port_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_discovery_transport_get_port_object_id(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_port_object_id_data_t * payload_discovery_get_port_object_id_data = NULL;
	fbe_object_id_t my_object_id;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_port_object_id_data = (fbe_payload_discovery_get_port_object_id_data_t *)sg_list[0].address;

	fbe_base_object_get_object_id((fbe_base_object_t *)sas_pmc_port, &my_object_id);

	payload_discovery_get_port_object_id_data->port_object_id = my_object_id;

	fbe_payload_discovery_set_status(payload_discovery_operation,FBE_PAYLOAD_DISCOVERY_STATUS_OK);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_transport_get_server_info(                  
*                    fbe_sas_pmc_port_t * sas_pmc_port, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*			Returns the port's sas address.
*
* @param      sas_pmc_port - The pointer to the fbe_sas_pmc_port_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_discovery_transport_get_server_info(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
	fbe_payload_discovery_get_server_info_data_t	* payload_discovery_get_server_info_data = NULL;	
	fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t be_port_number;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

	payload_discovery_get_server_info_data = (fbe_payload_discovery_get_server_info_data_t *)sg_list[0].address;

	status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &be_port_number);
	if (status == FBE_STATUS_OK){
		payload_discovery_get_server_info_data->address.sas_address = sas_pmc_port->port_address;
		payload_discovery_get_server_info_data->port_number = be_port_number;
		payload_discovery_get_server_info_data->position = 0;
        payload_discovery_get_server_info_data->component_id = 0;
		fbe_payload_discovery_set_status(payload_discovery_operation,FBE_PAYLOAD_DISCOVERY_STATUS_OK);
	}else{
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Failed to get port number, status: 0x%X",
								__FUNCTION__, status);
		fbe_payload_discovery_set_status(payload_discovery_operation,FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE);
	}

	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_transport_record_fw_activation_status(                  
*                    fbe_sas_pmc_port_t * sas_pmc_port, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*			recording da enclosure activation status.
*
* @param      sas_pmc_port - The pointer to the fbe_sas_pmc_port_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_discovery_transport_record_fw_activation_status(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_status_t status = FBE_STATUS_OK;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);

    fbe_spinlock_lock(&sas_pmc_port->list_lock);
    sas_pmc_port->da_enclosure_activating_fw = 
        payload_discovery_operation->command.notify_fw_status_command.activation_in_progress;
    fbe_spinlock_unlock(&sas_pmc_port->list_lock);

	fbe_payload_discovery_set_status(payload_discovery_operation,FBE_PAYLOAD_DISCOVERY_STATUS_OK);
	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}

/*!*************************************************************************
* @fn fbe_sas_pmc_port_event_entry(                  
*							fbe_object_handle_t object_handle, 
*							fbe_event_type_t event_type,
*							fbe_event_context_t event_context)
***************************************************************************
*
* @brief
*			  Event entry.
*
* @param      object_handle - The handle to the port object.
* @param      event_type - 
* @param      event_context - 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
fbe_status_t 
fbe_sas_pmc_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_status_t status;

	sas_pmc_port = (fbe_sas_pmc_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are not interested in any event at this time, so just forward it to the super class */
	status = fbe_sas_port_event_entry(object_handle, event_type, event_context);

	return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_ssp_transport_entry(                  
*							fbe_base_object_t * base_object, 
*							fbe_packet_t * packet)
***************************************************************************
*
* @brief
*			  SSP transport handling function. Internal function.
*
* @param      base_object - Base object pointer for the port object.
* @param      packet      - IO packet. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_ssp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_status_t status;
	fbe_port_number_t port_number;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
	fbe_cpd_device_id_t cpd_device_id;
	fbe_ssp_edge_t * ssp_edge = NULL;
	fbe_packet_attr_t packet_attr;
	fbe_bool_t async_io;

	sas_pmc_port = (fbe_sas_pmc_port_t *)base_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	fbe_cpd_shim_get_async_io(&async_io);
	if(async_io){
		fbe_queue_head_t tmp_queue;
		fbe_queue_init(&tmp_queue);

		fbe_queue_push(&tmp_queue, &packet->queue_element);
		fbe_transport_run_queue_push(&tmp_queue, sas_pmc_port_ssp_transport_entry_async_start, base_object);
		fbe_queue_destroy(&tmp_queue);
		return FBE_STATUS_PENDING;
	}

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

	/* Payload support */
	if (status == FBE_STATUS_OK){
		payload = fbe_transport_get_payload_ex(packet);
		payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
		if(payload_cdb_operation != NULL){ /* We have valid payload cdb operation */
			ssp_edge = (fbe_ssp_edge_t *)fbe_transport_get_edge(packet);
			fbe_ssp_transport_get_device_id(ssp_edge, &cpd_device_id);

			fbe_transport_set_status(packet, FBE_STATUS_PENDING, 0);
			fbe_transport_get_packet_attr(packet, &packet_attr);	
			if(packet_attr & (FBE_PACKET_FLAG_COMPLETION_BY_PORT | FBE_PACKET_FLAG_COMPLETION_BY_PORT_WITH_ZEROS)){
                fbe_queue_head_t tmp_queue;
#if 0
                if (packet_attr & FBE_PACKET_FLAG_COMPLETION_BY_PORT_WITH_ZEROS)
                {
                    fbe_sg_element_t *sg_p = payload->sg_list;
                    fbe_xor_zero_buffer_t zero_buffer;
                    zero_buffer.disks = 1;
                    zero_buffer.offset = 0;
                    zero_buffer.fru[0].count = payload_cdb_operation->transfer_count / FBE_BE_BYTES_PER_BLOCK;
                    zero_buffer.fru[0].offset = 0;
                    zero_buffer.fru[0].seed = FBE_LBA_INVALID;
                    zero_buffer.fru[0].sg_p = payload->sg_list;
                    fbe_xor_lib_zero_buffer(&zero_buffer);
                }
#endif
				fbe_queue_init(&tmp_queue);

				fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);
				fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

				fbe_queue_push(&tmp_queue, &packet->queue_element);
				fbe_transport_run_queue_push(&tmp_queue, sas_pmc_port_bypass_completion, NULL);
				fbe_queue_destroy(&tmp_queue);

				//fbe_transport_complete_packet(packet);
				return FBE_STATUS_OK;
			}


#if 0 
			/* Miniport takes care of cancelling the IOs on fleet. 
			   So skipping and additional lock per IO for now.*/

			/* Put the packet on the termination queue.	*/
			fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_pmc_port);
			fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_pmc_port, packet);
#endif
			status = fbe_cpd_shim_send_payload(port_number, cpd_device_id, payload);
			if (status == FBE_STATUS_OK){
				/* Return PENDING if the submission to Shim was successful.*/
				status = FBE_STATUS_PENDING;
			}else{
				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
										FBE_TRACE_LEVEL_ERROR,
										FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
										"%s Send payload returned error, status: 0x%X",
										__FUNCTION__, status);
			}
		} else {
			status = FBE_STATUS_GENERIC_FAILURE;
		}
	}
	return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_transport_get_element_list(                  
*							fbe_sas_pmc_port_t * sas_pmc_port, 
*							fbe_packet_t * packet)
***************************************************************************
*
* @brief
*			  Returns child list for the direct attached enclosure. 
*			  Internal function.
*
* @param      sas_pmc_port - port object pointer.
* @param      packet      - IO packet. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_discovery_transport_get_element_list(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
    fbe_u32_t i = 0,table_index = 0;
	fbe_u32_t number_of_elements = 0;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
	fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_discovery_get_element_list_command_t * payload_discovery_get_element_list_command = NULL;
	fbe_payload_discovery_get_element_list_data_t * payload_discovery_get_element_list_data = NULL;
    fbe_payload_discovery_element_t * element_list = NULL;
	fbe_sas_pmc_device_table_entry_t *this_entry = NULL;
	fbe_smp_edge_t	*smp_edge;
	fbe_cpd_device_id_t	 cpd_device_id;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	payload = fbe_transport_get_payload_ex(packet);
	payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
	fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    payload_discovery_get_element_list_command = &payload_discovery_operation->command.get_element_list_command;
	payload_discovery_get_element_list_data = (fbe_payload_discovery_get_element_list_data_t *)sg_list[0].address;
	element_list = payload_discovery_get_element_list_data->element_list;	
	smp_edge = (fbe_smp_edge_t *)fbe_transport_get_edge(packet);
	fbe_smp_transport_get_device_id(smp_edge,&cpd_device_id);

	number_of_elements = 0;

	table_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(cpd_device_id);
	fbe_spinlock_lock(&sas_pmc_port->list_lock);
	/* Confirm that the request is for the current device.*/
	this_entry = &(sas_pmc_port->device_table_ptr[table_index]);
	if (this_entry->device_id == cpd_device_id){
		/* Search the device list for children on this device.*/
		for(i = 0 ; i < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port); i ++){
			this_entry = &(sas_pmc_port->device_table_ptr[i]);
			if((this_entry->device_logged_in) &&
				(this_entry->parent_device_id == cpd_device_id)){

				element_list[number_of_elements].element_type = 
						sas_pmc_port_conver_shim_device_type_to_discovery_element_type(this_entry->device_type);
				element_list[number_of_elements].address.sas_address = this_entry->device_address;
				element_list[number_of_elements].phy_number = this_entry->device_locator.phy_number;
				element_list[number_of_elements].enclosure_chain_depth = this_entry->device_locator.enclosure_chain_depth;
				element_list[number_of_elements].generation_code = this_entry->generation_code;
				number_of_elements++;
			}
		}
	}
	fbe_spinlock_unlock(&sas_pmc_port->list_lock);

	payload_discovery_get_element_list_data->number_of_elements = number_of_elements; /* virtual phy and one SAS drive */
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}


fbe_status_t 
sas_pmc_port_send_payload_completion(fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context)
{
	fbe_packet_t * packet = NULL;
	fbe_base_edge_t * base_edge = NULL;
	fbe_transport_id_t transport_id;
	fbe_payload_cdb_operation_t * cdb_operation = NULL;
	fbe_payload_fis_operation_t * fis_operation = NULL;
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;

	sas_pmc_port = (fbe_sas_pmc_port_t *)context;

	packet = fbe_transport_payload_to_packet(payload);

	base_edge = fbe_transport_get_edge(packet);

	fbe_base_transport_get_transport_id(base_edge, &transport_id);
	if(transport_id == FBE_TRANSPORT_ID_SSP){
		cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
		if((cdb_operation->port_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS) && 
			(cdb_operation->port_request_status != FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN)){
				fbe_stat_io_fail(&sas_pmc_port->port_stat, sas_pmc_port->io_counter, &sas_pmc_port->io_error_tag, 0, FBE_STAT_WEIGHT_CHANGE_NONE);
				fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
				fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
				return FBE_STATUS_OK;
			}
	}else if(transport_id == FBE_TRANSPORT_ID_STP){
		fis_operation = fbe_payload_ex_get_fis_operation(payload);
		if((fis_operation->port_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS) && 
			(fis_operation->port_request_status != FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN)){
				fbe_stat_io_fail(&sas_pmc_port->port_stat, sas_pmc_port->io_counter,&sas_pmc_port->io_error_tag, 0, FBE_STAT_WEIGHT_CHANGE_NONE);
				fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
				fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
				return FBE_STATUS_OK;
			}
	} else {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid transport id %X \n", __FUNCTION__, transport_id);				

		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		//fbe_transport_complete_packet(packet);
		fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);		

		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

#if 0 
			/* Miniport takes care of cancelling the IOs on fleet. 
			   So skipping and additional lock per IO for now.*/

	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_pmc_port, packet);	
#endif

#if 0 /* This should be done in terminator, but I think it would be safe in production code as well */
	fbe_transport_get_cpu_id(packet, &cpu_id);
	current_cpu_id = fbe_get_cpu_id();

	if(cpu_id != FBE_CPU_ID_INVALID && current_cpu_id != cpu_id){
		fbe_queue_head_t tmp_queue;
		fbe_queue_init(&tmp_queue);
		fbe_transport_enqueue_packet(packet,&tmp_queue); 
		fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
		fbe_queue_destroy(&tmp_queue);
		return FBE_STATUS_OK;
	}
#endif
	//fbe_transport_complete_packet(packet);
	fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn sas_port_pmc_callback(                  
*							fbe_cpd_shim_callback_info_t * callback_info, 
*							fbe_cpd_shim_callback_context_t context)
***************************************************************************
*
* @brief
*			  Handles asynchronous event callback from shim.
*			  
*
* @param      callback_info - Callback information.
* @param      context      - Callback context that was provided during registration. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
fbe_status_t 
sas_port_pmc_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_port_number_t port_number;
    fbe_u32_t assigned_bus_number;
	fbe_packet_t *packet;
	fbe_key_handle_t	kek_kek_handle;

	sas_pmc_port = (fbe_sas_pmc_port_t *)context;

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

	switch(callback_info->callback_type)
    {
	case FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_TABLE_UPDATE:
        status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);
        if ((status==FBE_STATUS_OK) &&
            (assigned_bus_number != 0xFFFF))
        {
		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
								(fbe_base_object_t*)sas_pmc_port, 
								FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set direct attach enclosure login condition, status: 0x%X",
									__FUNCTION__, status);				
			}
			}
		break;
	case FBE_CPD_SHIM_CALLBACK_TYPE_DISCOVERY:
		switch(callback_info->info.discovery.discovery_event_type) {
			case FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_START:
				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
										FBE_TRACE_LEVEL_DEBUG_HIGH,
										FBE_TRACE_MESSAGE_ID_INFO,
										"%s Discovery start event port %X \n",
										__FUNCTION__,port_number);

				break;
			case FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_COMPLETE:
				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
										FBE_TRACE_LEVEL_DEBUG_HIGH,
										FBE_TRACE_MESSAGE_ID_INFO,
										"%s Discovery complete event port %X \n",
										__FUNCTION__,port_number);
				break;
		}
		/* TODO: Set condition to set or clear the DIP bit.*/
		break;
    
    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN:
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Link event. Port %d \n",
								__FUNCTION__,port_number);
        status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
                                (fbe_base_object_t*)sas_pmc_port, 
                                FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set get port data condition, status: 0x%X",
                                    __FUNCTION__, status);
        }        

        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS:

        fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Link event. Port %d \n",
                                __FUNCTION__,port_number);

        status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
                                (fbe_base_object_t*)sas_pmc_port, 
                                FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set get port data condition, status: 0x%X",
                                    __FUNCTION__, status);
        }        

        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP:

		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Link up event. Port %d \n",
								__FUNCTION__,port_number);

        
        status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
								(fbe_base_object_t*)sas_pmc_port, 
								FBE_SAS_PORT_LIFECYCLE_COND_LINK_UP);
	    if (status != FBE_STATUS_OK) {
		    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s can't set link up condition, status: 0x%X",
                                    __FUNCTION__, status);
        }        

        status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
                                (fbe_base_object_t*)sas_pmc_port, 
                                FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set get port data condition, status: 0x%X",
								    __FUNCTION__, status);
	    }        

        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_SFP:
		
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s SFP event. Port %d \n",
								__FUNCTION__,port_number);

        
        status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
								(fbe_base_object_t*)sas_pmc_port, 
								FBE_BASE_PORT_LIFECYCLE_COND_SFP_INFORMATION_CHANGE);
	    if (status != FBE_STATUS_OK) {
		    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s can't set SFP change condition, status: 0x%X",
								    __FUNCTION__, status);
	    }        

        break;

	case FBE_CPD_SHIM_CALLBACK_TYPE_DRIVER_RESET:
		
		if (callback_info->info.driver_reset.driver_reset_event_type == FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN)
		{
			
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
									FBE_TRACE_LEVEL_DEBUG_HIGH,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s Driver reset begin event. Port %d \n",
									__FUNCTION__,port_number);

			status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
									(fbe_base_object_t*)sas_pmc_port, 
									FBE_SAS_PORT_LIFECYCLE_COND_DRIVER_RESET_BEGIN_RECEIVED);
			if (status != FBE_STATUS_OK) {
				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
										FBE_TRACE_LEVEL_ERROR,
										FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
										"%s can't set direct attach enclosure login condition, status: 0x%X",
										__FUNCTION__, status);
			}


		}
		else if (callback_info->info.driver_reset.driver_reset_event_type == FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED)
		{
			sas_pmc_port->reset_complete_received = TRUE;

			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
									FBE_TRACE_LEVEL_DEBUG_HIGH,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s Driver reset complete event. Port %d \n",
									__FUNCTION__,port_number);
		}
		break;
	case FBE_CPD_SHIM_CALLBACK_TYPE_CTRL_RESET:
		
		if (callback_info->info.driver_reset.driver_reset_event_type == FBE_CPD_SHIM_CTRL_RESET_EVENT_TYPE_BEGIN)
		{
			
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
									FBE_TRACE_LEVEL_INFO,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s Controller reset begin event. Port %d \n",
									__FUNCTION__,port_number);

			fbe_base_port_get_kek_kek_handle((fbe_base_port_t *) sas_pmc_port, &kek_kek_handle);

			if(kek_kek_handle != FBE_INVALID_KEY_HANDLE) {

				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
										FBE_TRACE_LEVEL_INFO,
										FBE_TRACE_MESSAGE_ID_INFO,
										"%s Invalidate KEK handle. Port %d \n",
										__FUNCTION__,port_number);
				/* This is an encrypted system and so we need to clear the kek handle so that after the 
				 * controller reset is complete we will reestablish the KEKs */
				fbe_base_port_set_kek_handle((fbe_base_port_t *) sas_pmc_port, FBE_INVALID_KEY_HANDLE);
			}
		}
		else if (callback_info->info.driver_reset.driver_reset_event_type == FBE_CPD_SHIM_CTRL_RESET_EVENT_TYPE_COMPLETED)
		{
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
									FBE_TRACE_LEVEL_INFO,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s Controller reset complete event. Port %d \n",
									__FUNCTION__,port_number);

			fbe_base_port_get_kek_kek_handle((fbe_base_port_t *) sas_pmc_port, &kek_kek_handle);

			if(kek_kek_handle != FBE_INVALID_KEY_HANDLE) {
				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
										FBE_TRACE_LEVEL_INFO,
										FBE_TRACE_MESSAGE_ID_INFO,
										"%s Need to setup the KEKs. Port %d \n",
										__FUNCTION__,port_number);
				status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
												(fbe_base_object_t*)sas_pmc_port, 
												FBE_SAS_PMC_PORT_LIFECYCLE_COND_CTRL_RESET_COMPLETED);
			}

		}
		break;
	case FBE_CPD_SHIM_CALLBACK_TYPE_ENCRYPTION:
		packet = (fbe_packet_t *) callback_info->info.encrypt_context.context;
		status = callback_info->info.encrypt_context.status;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        
        break;
    default: /* ignore the rest */
		
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Unknown event. Event type: %X \n",
								__FUNCTION__,callback_info->callback_type);

        break;
    } /* end of switch(Info->Type) */

	return status;
}


static fbe_payload_smp_element_type_t
sas_pmc_port_conver_shim_device_type_to_smp_element_type(fbe_cpd_shim_discovery_device_type_t device_type)
{
    fbe_payload_smp_element_type_t element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID;

    switch (device_type)
    {
    case FBE_CPD_SHIM_DEVICE_TYPE_SSP:
        element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_SSP;
        break;
    case FBE_CPD_SHIM_DEVICE_TYPE_STP:
        element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_STP;
        break;
    case FBE_CPD_SHIM_DEVICE_TYPE_ENCLOSURE:
        element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_EXPANDER;
        break;
    case FBE_CPD_SHIM_DEVICE_TYPE_VIRTUAL:
        element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_VIRTUAL;
        break;
    default:
        element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID;
        break;

    }

    return element_type;
}

static fbe_payload_discovery_element_type_t
sas_pmc_port_conver_shim_device_type_to_discovery_element_type(fbe_cpd_shim_discovery_device_type_t device_type)
{
    fbe_payload_discovery_element_type_t element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_INVALID;

    switch (device_type)
    {
    case FBE_CPD_SHIM_DEVICE_TYPE_SSP:
        element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP;
        break;
    case FBE_CPD_SHIM_DEVICE_TYPE_STP:
        element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_STP;
        break;
    case FBE_CPD_SHIM_DEVICE_TYPE_ENCLOSURE:
        element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_EXPANDER;
        break;
    case FBE_CPD_SHIM_DEVICE_TYPE_VIRTUAL:
        element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_VIRTUAL;
        break;
    default:
        element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_INVALID;
        break;

    }

    return element_type;
}
/*!*************************************************************************
* @fn sas_pmc_port_stp_transport_entry(                  
*							fbe_base_object_t * base_object, 
*							fbe_packet_t * packet)
***************************************************************************
*
* @brief
*			  STP transport handling function. Internal function.
*
* @param      base_object - Base object pointer for the port object.
* @param      packet      - IO packet. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_stp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_status_t status;
	fbe_port_number_t port_number;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_fis_operation_t * payload_fis_operation = NULL;
	fbe_cpd_device_id_t cpd_device_id;
	fbe_stp_edge_t * stp_edge = NULL;

	sas_pmc_port = (fbe_sas_pmc_port_t *)base_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

	/* Payload support */
	payload = fbe_transport_get_payload_ex(packet);
	payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
	if(payload_fis_operation != NULL){ /* We have valid payload fis operation */
		stp_edge = (fbe_stp_edge_t *)fbe_transport_get_edge(packet);
		fbe_stp_transport_get_device_id(stp_edge, &cpd_device_id);

		fbe_transport_set_status(packet, FBE_STATUS_PENDING, 0);
		status = FBE_STATUS_PENDING;

#if 0 
			/* Miniport takes care of cancelling the IOs on fleet. 
			   So skipping and additional lock per IO for now.*/

		/* Put the packet on the termination queue.	*/
		fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) sas_pmc_port);
		fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) sas_pmc_port, packet);
#endif

		fbe_cpd_shim_send_fis(port_number, cpd_device_id, payload);
		return status;
	}
    
	return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_smp_transport_entry(                  
*							fbe_base_object_t * base_object, 
*							fbe_packet_t * packet)
***************************************************************************
*
* @brief
*			  SMP transport handling function. Internal function.
*
* @param      base_object - Base object pointer for the port object.
* @param      packet      - IO packet. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_smp_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_sas_pmc_port_t * sas_pmc_port = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_smp_operation_t * payload_smp_operation = NULL;
    fbe_payload_smp_opcode_t smp_opcode;
    fbe_status_t status = FBE_STATUS_OK;

    sas_pmc_port = (fbe_sas_pmc_port_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_smp_operation = fbe_payload_ex_get_smp_operation(payload);
    fbe_payload_smp_get_opcode(payload_smp_operation, &smp_opcode);
    
    switch(smp_opcode){
        case FBE_PAYLOAD_SMP_OPCODE_GET_ELEMENT_LIST:
                status = sas_pmc_port_smp_transport_get_element_list(sas_pmc_port, packet);
            break;
        case FBE_PAYLOAD_SMP_OPCODE_RESET_END_DEVICE:
                status = sas_pmc_port_smp_transport_reset_end_device(sas_pmc_port, packet);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unknown smp_opcode %X\n", __FUNCTION__, smp_opcode);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    
    return status;
}

/*!*************************************************************************
* @fn sas_pmc_port_smp_transport_get_element_list(                  
*							fbe_sas_pmc_port_t * sas_pmc_port, 
*							fbe_packet_t * packet)
***************************************************************************
*
* @brief
*			  Returns child list for the direct attached enclosure. 
*			  Internal function.
*
* @param      sas_pmc_port - port object pointer.
* @param      packet      - IO packet. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_smp_transport_get_element_list(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
    fbe_u32_t i;
    fbe_u32_t number_of_elements;
    fbe_payload_ex_t * payload;
    fbe_payload_smp_operation_t * payload_smp_operation;
    fbe_edge_index_t server_index;

    fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sg_element_t  * sg_list;
    fbe_payload_smp_get_element_list_command_t * payload_smp_get_element_list_command;
    fbe_payload_smp_get_element_list_data_t * payload_smp_get_element_list_data;
    fbe_payload_smp_element_t * element_list = NULL;
	fbe_smp_edge_t	*smp_edge;
	fbe_cpd_device_id_t	 cpd_device_id;
    fbe_payload_smp_element_extended_status_t element_status;

    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_smp_operation = fbe_payload_ex_get_smp_operation(payload);
    payload_smp_get_element_list_command = &payload_smp_operation->command.get_element_list_command;
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    payload_smp_get_element_list_data = (fbe_payload_smp_get_element_list_data_t *)sg_list[0].address;
    if (payload_smp_get_element_list_data == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s: got NULL element list pointer\n", __FUNCTION__);
    }
    else {
		smp_edge = (fbe_smp_edge_t *)fbe_transport_get_edge(packet);
		fbe_smp_transport_get_device_id(smp_edge,&cpd_device_id);

        fbe_status = sas_pmc_port_smp_transport_get_element_status(sas_pmc_port,cpd_device_id,&element_status);
        /* Check whether this is an invalid enclosure logged in by miniport driver as faulted.*/
        if ((fbe_status == FBE_STATUS_OK) && (element_status == FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NORMAL))
		{
			number_of_elements = 0;
			element_list = payload_smp_get_element_list_data->element_list;
			fbe_spinlock_lock(&sas_pmc_port->list_lock);
			for(i = 0 ; i < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port); i ++)
			{
				if((sas_pmc_port->device_table_ptr[i].device_logged_in) &&
					(sas_pmc_port->device_table_ptr[i].parent_device_id == cpd_device_id))
				{
					element_list[number_of_elements].element_type = 
							sas_pmc_port_conver_shim_device_type_to_smp_element_type(sas_pmc_port->device_table_ptr[i].device_type);
					element_list[number_of_elements].address.sas_address = sas_pmc_port->device_table_ptr[i].device_address;
					element_list[number_of_elements].phy_number = sas_pmc_port->device_table_ptr[i].device_locator.phy_number;
					element_list[number_of_elements].enclosure_chain_depth = sas_pmc_port->device_table_ptr[i].device_locator.enclosure_chain_depth;
					element_list[number_of_elements].generation_code = sas_pmc_port->device_table_ptr[i].generation_code;
                    sas_pmc_port_smp_transport_set_element_status(&(element_list[number_of_elements].element_status),
                                                                    sas_pmc_port->device_table_ptr[i].device_login_reason);
					number_of_elements++;
					if (number_of_elements >= FBE_PAYLOAD_SMP_ELEMENT_LIST_SIZE)
					{
						break;
					}
				}
			}
			fbe_spinlock_unlock(&sas_pmc_port->list_lock);
			if (fbe_transport_get_server_index(packet, &server_index) == FBE_STATUS_OK) 
			{
				fbe_sas_port_clear_smp_path_attr((fbe_sas_port_t*)sas_pmc_port, server_index, FBE_SMP_PATH_ATTR_ELEMENT_CHANGE);
			}
	        
			payload_smp_get_element_list_data->number_of_elements = number_of_elements;
			fbe_payload_smp_set_status(payload_smp_operation,FBE_PAYLOAD_SMP_STATUS_OK);
        }
		else
		{
            fbe_payload_smp_set_status(payload_smp_operation,FBE_PAYLOAD_SMP_STATUS_FAILURE);
            if (fbe_status == FBE_STATUS_OK)
			{
                /* Device was logged with special status indicating that this should not be part
                 * of a normal topology.
                 */                
                fbe_payload_smp_set_status_qualifier(payload_smp_operation,element_status);
            }
			else
			{
                /* Could not locate entry for the device in the device table.*/
                fbe_payload_smp_set_status_qualifier(payload_smp_operation,FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NO_DEVICE);
            }
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*!*************************************************************************
* @fn sas_pmc_port_smp_transport_reset_end_device(                  
*							fbe_sas_pmc_port_t * sas_pmc_port, 
*							fbe_packet_t * packet)
***************************************************************************
*
* @brief
*			  Resets end device phy for the given enclosure. 
*			  Internal function.
*
* @param      sas_pmc_port - port object pointer.
* @param      packet      - IO packet. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_status_t 
sas_pmc_port_smp_transport_reset_end_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{    
    fbe_payload_ex_t * payload;
    fbe_payload_smp_operation_t * payload_smp_operation;
    fbe_edge_index_t server_index;

    fbe_status_t fbe_status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_smp_reset_end_device_command_t * payload_smp_reset_end_device_command;
    fbe_smp_edge_t	*smp_edge;
    fbe_cpd_device_id_t	 cpd_device_id;
    fbe_payload_smp_element_extended_status_t element_status;
    fbe_port_number_t port_number;

    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_smp_operation = fbe_payload_ex_get_smp_operation(payload);
    payload_smp_reset_end_device_command = &payload_smp_operation->command.reset_end_device_command;

    smp_edge = (fbe_smp_edge_t *)fbe_transport_get_edge(packet);
    fbe_smp_transport_get_device_id(smp_edge,&cpd_device_id);

    fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

    fbe_status = sas_pmc_port_smp_transport_get_element_status(sas_pmc_port,cpd_device_id,&element_status);
    /* Check whether this is an invalid enclosure logged in by miniport driver as faulted.*/
    if ((fbe_status == FBE_STATUS_OK) && (element_status == FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NORMAL))
    {
        fbe_status = fbe_cpd_shim_reset_expander_phy(port_number,cpd_device_id,
                                                 payload_smp_reset_end_device_command->phy_id);
        if (fbe_status != FBE_STATUS_OK){		    
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Error sending RESET request, status: 0x%X \n",
                                    __FUNCTION__, fbe_status);
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "  ---> Error RESET port_num: 0x%X, SMP dev id 0x%X, phy_id 0x%X \n",
                                    port_number,cpd_device_id,
                                    payload_smp_reset_end_device_command->phy_id);

        }

        fbe_payload_smp_set_status(payload_smp_operation,FBE_PAYLOAD_SMP_STATUS_OK);
    }
    else
    {
        fbe_payload_smp_set_status(payload_smp_operation,FBE_PAYLOAD_SMP_STATUS_FAILURE);
        if (fbe_status == FBE_STATUS_OK)
        {
            /* Device was logged with special status indicating that this should not be part
             * of a normal topology.
             */                
            fbe_payload_smp_set_status_qualifier(payload_smp_operation,element_status);
        }
        else
        {
            /* Could not locate entry for the device in the device table.*/
            fbe_payload_smp_set_status_qualifier(payload_smp_operation,0xFFFF);
            if (fbe_transport_get_server_index(packet, &server_index) == FBE_STATUS_OK){ 
                fbe_sas_port_set_smp_path_attr((fbe_sas_port_t *) sas_pmc_port, server_index, FBE_SMP_PATH_ATTR_CLOSED);
            }
		}
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/* NOTE: Spin lock might be held when this function is invoked by the caller.*/
static fbe_status_t 
sas_pmc_port_smp_transport_set_element_status(fbe_payload_smp_element_extended_status_t * element_status, 
                                              fbe_cpd_shim_device_login_reason_t device_login_reason)
{
    switch (device_login_reason){
        case CPD_SHIM_DEVICE_LOGIN_REASON_NORMAL:
            *element_status = FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NORMAL;
            break;
        case CPD_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_EXPANDER:
            *element_status = FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_UNSUPPORTED_ENCL;
            break;

        case CPD_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_TOPOLOGY:
            *element_status = FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_UNSUPPORTED_TOPOLOGY;
            break;
        case CPD_SHIM_DEVICE_LOGIN_REASON_EXPANDER_MIXED_COMPLIANCE:
            *element_status = FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_MIXED_COMPLIANCE;
            break;
        case CPD_SHIM_DEVICE_LOGIN_REASON_TOO_MANY_END_DEVICES:
            *element_status = FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_TOO_MANY_END_DEVICES;
            break;
    };		


    return FBE_STATUS_OK;
}

static fbe_status_t 
sas_pmc_port_smp_transport_get_element_status(fbe_sas_pmc_port_t * sas_pmc_port, fbe_cpd_device_id_t	 cpd_device_id,
                                              fbe_payload_smp_element_extended_status_t * element_status)
{
    fbe_u32_t ii;
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_bool_t  entry_located = FBE_FALSE;
    fbe_sas_pmc_device_table_entry_t *this_entry = NULL;

    fbe_spinlock_lock(&sas_pmc_port->list_lock);
    for(ii = 0 ; ii < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port); ii++){

        this_entry = &(sas_pmc_port->device_table_ptr[ii]);
        if((this_entry->device_logged_in) &&(this_entry->device_id == cpd_device_id)){
             entry_located = FBE_TRUE;
             break;
        }
    }
    fbe_spinlock_unlock(&sas_pmc_port->list_lock);

    if (entry_located == FBE_TRUE){
        fbe_status = sas_pmc_port_smp_transport_set_element_status(element_status, this_entry->device_login_reason);
    }else{
        fbe_status = FBE_STATUS_NO_OBJECT;
    }
    return fbe_status;
}


static fbe_status_t 
sas_pmc_port_bypass_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_block_operation_t * block_operation = NULL;
	fbe_payload_block_operation_opcode_t block_opcode;

	payload = fbe_transport_get_payload_ex(packet);
	block_operation = fbe_payload_ex_get_prev_block_operation(payload);
	fbe_payload_block_get_opcode(block_operation, &block_opcode);

	switch(block_opcode){
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:

		break;
	case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:

		break;
	default:

		break;
	}

	return FBE_STATUS_OK;
}


static fbe_status_t 
sas_pmc_port_ssp_transport_entry_async_start(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_status_t status;
	fbe_port_number_t port_number;
	fbe_payload_ex_t * payload = NULL;
	fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
	fbe_cpd_device_id_t cpd_device_id;
	fbe_ssp_edge_t * ssp_edge = NULL;	

	sas_pmc_port = (fbe_sas_pmc_port_t *)context;

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

	/* Payload support */
	if (status == FBE_STATUS_OK){
		payload = fbe_transport_get_payload_ex(packet);
		payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
		if(payload_cdb_operation != NULL){ /* We have valid payload cdb operation */
			ssp_edge = (fbe_ssp_edge_t *)fbe_transport_get_edge(packet);
			fbe_ssp_transport_get_device_id(ssp_edge, &cpd_device_id);

			fbe_transport_set_status(packet, FBE_STATUS_PENDING, 0);

			status = fbe_cpd_shim_send_payload(port_number, cpd_device_id, payload);
		} else {
			return FBE_STATUS_GENERIC_FAILURE;
		}
	}
	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
