#include "fbe_base_discovered.h"
#include "sas_pmc_port_private.h"
#include "fbe_cpd_shim.h"
#include "fbe_pmc_shim.h"
#include "fbe_sas_port.h"

/* Forward declaration */
static fbe_status_t sas_pmc_port_attach_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t sas_pmc_port_detach_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_get_protocol_address(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_open_ssp_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_open_smp_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_get_device_id_from_table_entry(fbe_sas_pmc_port_t * sas_pmc_port, fbe_sas_address_t sas_address,
                                                fbe_generation_code_t generation_code,fbe_miniport_device_id_t *device_id,fbe_u32_t *index);
static fbe_status_t fbe_sas_pmc_port_table_entry_set_protocol_edge_validity(fbe_sas_pmc_port_t * sas_pmc_port, fbe_u32_t index,fbe_bool_t edge_valid);
static fbe_status_t fbe_sas_pmc_port_table_entry_set_stp_protocol_edge_validity(fbe_sas_pmc_port_t * sas_pmc_port, fbe_u32_t index,fbe_bool_t edge_valid);

static fbe_status_t fbe_sas_pmc_port_table_entry_set_smp_edge_validity(fbe_sas_pmc_port_t * sas_pmc_port, fbe_u32_t index,fbe_bool_t edge_valid);
static fbe_status_t fbe_sas_pmc_port_table_entry_mark_logged_out(fbe_sas_pmc_port_t * sas_pmc_port, fbe_u32_t index,fbe_cpd_device_id_t cpd_device_id);


static fbe_status_t fbe_sas_pmc_port_open_stp_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_reset_ssp_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_reset_stp_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
static fbe_status_t fbe_sas_pmc_port_smp_reset_end_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);



fbe_status_t 
fbe_sas_pmc_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	sas_pmc_port = (fbe_sas_pmc_port_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
			status = sas_pmc_port_attach_edge( sas_pmc_port, packet);
			break;
		case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
			status = sas_pmc_port_detach_edge( sas_pmc_port, packet);
			break;

        case FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
            status = fbe_sas_pmc_port_open_ssp_edge( sas_pmc_port, packet);
            break;

		case FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
            status = fbe_sas_pmc_port_open_stp_edge( sas_pmc_port, packet);
            break;

        case FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
            status = fbe_sas_pmc_port_open_smp_edge( sas_pmc_port, packet);
            break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE:
            status = fbe_sas_pmc_port_smp_reset_end_device(sas_pmc_port, packet);
            break;
		case FBE_SSP_TRANSPORT_CONTROL_CODE_RESET_DEVICE:
			status = fbe_sas_pmc_port_reset_ssp_device( sas_pmc_port, packet);
			break;
		case FBE_STP_TRANSPORT_CONTROL_CODE_RESET_DEVICE:
			status = fbe_sas_pmc_port_reset_stp_device( sas_pmc_port, packet);
			break;

        default:
			status = fbe_sas_port_control_entry(object_handle, packet);
			break;
	}
	return status;
}

static fbe_status_t 
sas_pmc_port_attach_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_u32_t number_of_clients = 0;
	fbe_discovery_transport_control_attach_edge_t * discovery_transport_control_attach_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
	fbe_edge_index_t server_index;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    
       
    /* TODO: Modify to accept ESES edges and SSP edges.*/
	fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_attach_edge);
	if(discovery_transport_control_attach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_discovery_transport_control_attach_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
								"%X len invalid\n", len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* sas_pmc_port may have only one edge, so if we already have one - reject the request */
	status = fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)sas_pmc_port, &number_of_clients);
	if(number_of_clients != 0) {
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s (Dell board support only one client)\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_base_discovering_attach_edge((fbe_base_discovering_t *) sas_pmc_port, discovery_transport_control_attach_edge->discovery_edge);
	fbe_discovery_transport_get_server_index(discovery_transport_control_attach_edge->discovery_edge,&server_index);
	fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) sas_pmc_port, server_index, 0 /* attr */);
	fbe_base_discovering_update_path_state((fbe_base_discovering_t *) sas_pmc_port);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
sas_pmc_port_detach_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;


	fbe_base_object_trace(	(fbe_base_object_t *)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_detach_edge);
	if(discovery_transport_control_detach_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"fbe_payload_control_get_buffer fail\n");
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}


	status = fbe_base_discovering_detach_edge((fbe_base_discovering_t *) sas_pmc_port, discovery_transport_control_detach_edge->discovery_edge);

	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}

static fbe_status_t 
fbe_sas_pmc_port_open_ssp_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_ssp_transport_control_open_edge_t * ssp_transport_control_open_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_ssp_edge_t * ssp_edge = NULL;
    fbe_miniport_device_id_t device_id = 0;
    fbe_status_t status;
    fbe_u32_t device_table_index = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u32_t assigned_bus_number = 0;

    status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry for BE %d\n", __FUNCTION__, assigned_bus_number);



    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &ssp_transport_control_open_edge);
	if(ssp_transport_control_open_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_ssp_transport_control_open_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s %X len invalid\n", __FUNCTION__, len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* We need to find the entry in the table with same address and generation code.
		If we were succesful we should update path state and attributes of the edge.
	*/

	ssp_edge = (fbe_ssp_edge_t *)fbe_transport_get_edge(packet);

	/* We should look in the SAS table find sas address and generation code 
		and set device_id to the edge 
	*/

    status = fbe_sas_pmc_port_get_device_id_from_table_entry(sas_pmc_port,  ssp_transport_control_open_edge->sas_address,
                                                            ssp_transport_control_open_edge->generation_code,
															&device_id,
                                                            &device_table_index);
    if (status != FBE_STATUS_OK)
    {
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device not found in device table.\n", __FUNCTION__);
    
        ssp_transport_control_open_edge->status = FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_NO_DEVICE;
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;

    }

    /* TODO: Resolve CPD reference in base sas port vs PMC port definitions.*/
    /* Typecasting for now.*/
    fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s setting new cpd_device_id 0x%x for entry %d\n", __FUNCTION__, (unsigned int)device_id, device_table_index);

	fbe_ssp_transport_set_device_id(ssp_edge, (fbe_cpd_device_id_t)device_id);
    fbe_ssp_transport_set_server_index(ssp_edge,device_table_index);

	fbe_sas_port_clear_ssp_path_attr((fbe_sas_port_t * )sas_pmc_port,device_table_index,FBE_SSP_PATH_ATTR_CLOSED);
    fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s BE %d Setting SSP edge to valid and open.\n", __FUNCTION__, assigned_bus_number);
	fbe_sas_pmc_port_table_entry_set_protocol_edge_validity(sas_pmc_port,device_table_index,TRUE);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_pmc_port_open_smp_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
    fbe_smp_transport_control_open_edge_t * smp_transport_control_open_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_smp_edge_t * smp_edge = NULL;
    fbe_miniport_device_id_t device_id = 0;
    fbe_status_t status;
    fbe_u32_t device_table_index = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u32_t assigned_bus_number = 0;

    status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);

    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry for BE %d\n", __FUNCTION__, assigned_bus_number);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &smp_transport_control_open_edge);
    if (smp_transport_control_open_edge == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_smp_transport_control_open_edge_t)){
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    smp_edge = (fbe_smp_edge_t *)fbe_transport_get_edge(packet);

    status = fbe_sas_pmc_port_get_device_id_from_table_entry(sas_pmc_port,
                                                             smp_transport_control_open_edge->sas_address,
                                                             smp_transport_control_open_edge->generation_code,
                                                             &device_id,
                                                             &device_table_index);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device not found in device table.\n", __FUNCTION__);
        smp_transport_control_open_edge->status = FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE_STATUS_NO_DEVICE;
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s setting new cpd_device_id 0x%x for entry %d\n", __FUNCTION__, (unsigned int)device_id, device_table_index);
    fbe_smp_transport_set_device_id(smp_edge, (fbe_cpd_device_id_t)device_id);
    fbe_smp_transport_set_server_index(smp_edge, device_table_index);

    fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s BE %d Setting SMP edge to Valid and Open.\n", __FUNCTION__, assigned_bus_number);

    fbe_sas_port_clear_smp_path_attr((fbe_sas_port_t * )sas_pmc_port,device_table_index, FBE_SMP_PATH_ATTR_CLOSED);
    fbe_sas_pmc_port_table_entry_set_smp_edge_validity(sas_pmc_port, device_table_index, TRUE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_pmc_port_get_device_id_from_table_entry(
                            fbe_sas_pmc_port_t * sas_pmc_port, 
                            fbe_sas_address_t sas_address,
                            fbe_generation_code_t generation_code,
                            fbe_miniport_device_id_t *device_id,
                            fbe_u32_t *index)
{
	fbe_status_t status = FBE_STATUS_NO_OBJECT;
    fbe_u32_t i = 0;

    if (sas_pmc_port->device_table_ptr != NULL)
    {
        fbe_spinlock_lock(&sas_pmc_port->list_lock);
        /* Find the entry.*/
        for (i = 0; i < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port); i++)
        {
            if ((sas_pmc_port->device_table_ptr[i].device_logged_in) &&
                (sas_pmc_port->device_table_ptr[i].device_address == sas_address) &&
                (sas_pmc_port->device_table_ptr[i].generation_code == generation_code))

            {
                *index = i;
                *device_id = sas_pmc_port->device_table_ptr[i].device_id;
                
                status = FBE_STATUS_OK;
                break;
            }
        }

        fbe_spinlock_unlock(&sas_pmc_port->list_lock);

        if (i == FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port))
        {
            /* We could not find the device entry in the table.*/
            status = FBE_STATUS_NO_OBJECT;
        }
    }
    return status;
}

static fbe_status_t 
fbe_sas_pmc_port_table_entry_set_protocol_edge_validity(
                            fbe_sas_pmc_port_t * sas_pmc_port, 
                            fbe_u32_t index,
							fbe_bool_t edge_valid)
{
    fbe_u32_t assigned_bus_number = 0;
    fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);
	fbe_spinlock_lock(&sas_pmc_port->list_lock);
	if (index < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port)){
		if(edge_valid){
            fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s BE %d setting SSP edge valid for index %u\n", __FUNCTION__, assigned_bus_number, index);
			SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_SSP_PROTOCOL_EDGE_VALID(&sas_pmc_port->device_table_ptr[index]);
		}else{
            fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s BE %d clearing SSP edge valid for index %u\n", __FUNCTION__, assigned_bus_number, index);
			SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SSP_PROTOCOL_EDGE_VALID(&sas_pmc_port->device_table_ptr[index]);
		}
	}
	fbe_spinlock_unlock(&sas_pmc_port->list_lock);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_pmc_port_table_entry_set_stp_protocol_edge_validity(
                            fbe_sas_pmc_port_t * sas_pmc_port, 
                            fbe_u32_t index,
							fbe_bool_t edge_valid)
{
	fbe_spinlock_lock(&sas_pmc_port->list_lock);
	if (index < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port))	{
		if(edge_valid){
			SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_STP_PROTOCOL_EDGE_VALID(&sas_pmc_port->device_table_ptr[index]);
		}else{
			SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_STP_PROTOCOL_EDGE_VALID(&sas_pmc_port->device_table_ptr[index]);
		}
	}
	fbe_spinlock_unlock(&sas_pmc_port->list_lock);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_pmc_port_table_entry_set_smp_edge_validity(
                            fbe_sas_pmc_port_t * sas_pmc_port, 
                            fbe_u32_t index,
							fbe_bool_t edge_valid)
{
	fbe_spinlock_lock(&sas_pmc_port->list_lock);
	if (index < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port))	{
		if(edge_valid){
			SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_SMP_PROTOCOL_EDGE_VALID(&sas_pmc_port->device_table_ptr[index]);
		}else{
			SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SMP_PROTOCOL_EDGE_VALID(&sas_pmc_port->device_table_ptr[index]);
		}
	}
	fbe_spinlock_unlock(&sas_pmc_port->list_lock);

	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_pmc_port_table_entry_mark_logged_out(
                            fbe_sas_pmc_port_t * sas_pmc_port, 
                            fbe_u32_t index,
							fbe_cpd_device_id_t cpd_device_id)
{
	fbe_status_t	status = FBE_STATUS_OK;

	fbe_spinlock_lock(&sas_pmc_port->list_lock);
	if (index < FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port))	{
		if ((sas_pmc_port->device_table_ptr[index].device_logged_in) && (sas_pmc_port->device_table_ptr[index].device_id == cpd_device_id)){
			sas_pmc_port->device_table_ptr[index].device_logged_in = FBE_FALSE;
		}else{
			status = FBE_STATUS_NO_DEVICE;
		}
	}else{
		status = FBE_STATUS_GENERIC_FAILURE;
	}
	fbe_spinlock_unlock(&sas_pmc_port->list_lock);

	return status;
}

static fbe_status_t 
fbe_sas_pmc_port_open_stp_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_stp_transport_control_open_edge_t * stp_transport_control_open_edge = NULL;
	fbe_payload_control_buffer_length_t len = 0;
	fbe_stp_edge_t * stp_edge = NULL;
    fbe_miniport_device_id_t device_id = 0;
    fbe_status_t status;
    fbe_u32_t device_table_index = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &stp_transport_control_open_edge);
	if(stp_transport_control_open_edge == NULL){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
    
	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_stp_transport_control_open_edge_t)){
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s %X len invalid\n", __FUNCTION__, len);

	    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
	    return FBE_STATUS_GENERIC_FAILURE;
	}

	/* Right now we do not know all the path state and path attributes rules,
	   so we will do something straight forward and may be not exactly correct.
	 */

	/* We need to find the entry in the table with same address and generation code.
		If we were succesful we should update path state and attributes of the edge.
	*/

	stp_edge = (fbe_stp_edge_t *)fbe_transport_get_edge(packet);

	/* We should look in the SAS table find sas address and generation code 
		and set device_id to the edge 
	*/

    status = fbe_sas_pmc_port_get_device_id_from_table_entry(sas_pmc_port,  stp_transport_control_open_edge->sas_address,
                                                            stp_transport_control_open_edge->generation_code,
															&device_id,
                                                            &device_table_index);
    if (status != FBE_STATUS_OK)
    {
		fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device not found in device table.\n", __FUNCTION__);
            
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;

    }

    /* TODO: Resolve CPD reference in base sas port vs PMC port definitions.*/
    /* Typecasting for now.*/
	fbe_stp_transport_set_device_id(stp_edge, (fbe_cpd_device_id_t)device_id);
    fbe_stp_transport_set_server_index(stp_edge,device_table_index);

	fbe_sas_port_clear_stp_path_attr((fbe_sas_port_t * )sas_pmc_port,device_table_index,FBE_STP_PATH_ATTR_CLOSED);
	fbe_sas_pmc_port_table_entry_set_stp_protocol_edge_validity(sas_pmc_port,device_table_index,TRUE);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_sas_pmc_port_reset_ssp_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
	fbe_ssp_edge_t * ssp_edge = NULL;
    fbe_cpd_device_id_t cpd_device_id = 0;    
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
	fbe_edge_index_t server_index;
	fbe_port_number_t port_number;
	fbe_status_t	  status = FBE_STATUS_OK;
	fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	ssp_edge = (fbe_ssp_edge_t *)fbe_transport_get_edge(packet);
	fbe_ssp_transport_get_device_id(ssp_edge,&cpd_device_id);
	fbe_ssp_transport_get_server_index(ssp_edge,&server_index);	
	/* TODO: Additonal flag for reset complete notification rcvd and retry mechanism.*/
	fbe_sas_pmc_port_table_entry_mark_logged_out(sas_pmc_port, server_index,cpd_device_id);
	fbe_sas_port_set_ssp_path_attr((fbe_sas_port_t *) sas_pmc_port, server_index, FBE_SSP_PATH_ATTR_CLOSED);
	fbe_ssp_transport_set_server_index(ssp_edge, FBE_EDGE_INDEX_INVALID);
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);
	if (status == FBE_STATUS_OK){
		control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
		status = fbe_cpd_shim_reset_device(port_number,cpd_device_id);
		if (status != FBE_STATUS_OK){
			control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Error sending RESET request, status: 0x%X \n",
									__FUNCTION__, status);
		}
	}else{
		control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Failed to get port number, status: 0x%X",
								__FUNCTION__, status);
	}
	fbe_payload_control_set_status(control_operation, control_status);
	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}

static fbe_status_t 
fbe_sas_pmc_port_reset_stp_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{	
	fbe_stp_edge_t * stp_edge = NULL;
    fbe_cpd_device_id_t cpd_device_id = 0;    
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
	fbe_edge_index_t server_index;
	fbe_status_t	status = FBE_STATUS_OK;
	fbe_port_number_t port_number;	
	fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	stp_edge = (fbe_stp_edge_t *)fbe_transport_get_edge(packet);
	fbe_stp_transport_get_device_id(stp_edge,&cpd_device_id);


	fbe_stp_transport_get_server_index(stp_edge,&server_index);	
	/* TODO: Additonal flag for reset complete notification rcvd and retry mechanism.*/
	fbe_sas_pmc_port_table_entry_mark_logged_out(sas_pmc_port, server_index,cpd_device_id);
	fbe_sas_port_set_stp_path_attr((fbe_sas_port_t *) sas_pmc_port, server_index, FBE_STP_PATH_ATTR_CLOSED);
	fbe_stp_transport_set_server_index(stp_edge, FBE_EDGE_INDEX_INVALID);
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);
	if (status == FBE_STATUS_OK){
		control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
		status = fbe_cpd_shim_reset_device(port_number,cpd_device_id);
		if (status != FBE_STATUS_OK){
			control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Error sending RESET request, status: 0x%X",
									__FUNCTION__, status);			
		}
	}else{
		control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Failed to get port number, status: 0x%X",
								__FUNCTION__, status);		
	}
	fbe_payload_control_set_status(control_operation, control_status);
	fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return status;
}
static fbe_status_t 
fbe_sas_pmc_port_smp_reset_end_device(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet)
{
    fbe_smp_transport_control_reset_end_device_t * smp_transport_control_reset_end_device = NULL;
    fbe_payload_control_buffer_length_t len = 0;    
    fbe_miniport_device_id_t device_id = 0;
    fbe_status_t status;
    fbe_u32_t device_table_index = 0;    
    fbe_port_number_t port_number;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &smp_transport_control_reset_end_device);
    if (smp_transport_control_reset_end_device == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_smp_transport_control_reset_end_device_t)){
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }    

   	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);
    if (status != FBE_STATUS_OK){
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s Invalid port number.\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, control_status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }    

    status = fbe_sas_pmc_port_get_device_id_from_table_entry(sas_pmc_port,
                                                             smp_transport_control_reset_end_device->sas_address,
                                                             smp_transport_control_reset_end_device->generation_code,
                                                             &device_id,
                                                             &device_table_index);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s device not found in device table.\n", __FUNCTION__);
        smp_transport_control_reset_end_device->status = FBE_SMP_TRANSPORT_CONTROL_CODE_RESET_END_DEVICE_STATUS_NO_DEVICE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_cpd_shim_reset_expander_phy(port_number,(fbe_cpd_device_id_t)device_id,
                                             smp_transport_control_reset_end_device->phy_id);
	if (status != FBE_STATUS_OK){
		control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error sending RESET request, status: 0x%X \n",
								__FUNCTION__, status);
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"  ---> Error RESET port_num: 0x%X, SMP dev id 0x%llX, phy_id 0x%X \n",
								port_number, (unsigned long long)device_id,
                                smp_transport_control_reset_end_device->phy_id);

	}

    fbe_payload_control_set_status(control_operation, control_status);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
