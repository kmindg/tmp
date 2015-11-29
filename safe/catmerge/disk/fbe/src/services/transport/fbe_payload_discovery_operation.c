#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"

fbe_status_t 
fbe_payload_discovery_get_status(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_t * status)
{
	*status = discovery_operation->status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_set_status(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_t status)
{
	discovery_operation->status = status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_get_status_qualifier(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_qualifier_t * status_qualifier)
{
	*status_qualifier = discovery_operation->status_qualifier;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_set_status_qualifier(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_qualifier_t status_qualifier)
{
	discovery_operation->status_qualifier = status_qualifier;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_get_protocol_address(fbe_payload_discovery_operation_t * discovery_operation, fbe_object_id_t client_id)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS;
	discovery_operation->command.get_protocol_address_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_get_port_object_id(fbe_payload_discovery_operation_t * discovery_operation)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID;
	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_discovery_build_get_element_list(fbe_payload_discovery_operation_t * discovery_operation, fbe_address_t address, fbe_generation_code_t generation_code)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_GET_ELEMENT_LIST;
	discovery_operation->command.get_element_list_command.address = address;
	discovery_operation->command.get_element_list_command.generation_code = generation_code;

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_discovery_build_get_slot_info(fbe_payload_discovery_operation_t * discovery_operation, fbe_object_id_t client_id)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SLOT_INFO;
	discovery_operation->command.get_slot_info_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_power_on(fbe_payload_discovery_operation_t * discovery_operation, fbe_object_id_t client_id)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON;
	discovery_operation->command.power_on_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_unbypass(fbe_payload_discovery_operation_t * discovery_operation, fbe_object_id_t client_id)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_UNBYPASS;
	discovery_operation->command.unbypass_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_spin_up(fbe_payload_discovery_operation_t * discovery_operation, fbe_object_id_t client_id)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_SPIN_UP;
	discovery_operation->command.spin_up_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_power_cycle(fbe_payload_discovery_operation_t * discovery_operation, 
										fbe_object_id_t client_id,
										fbe_bool_t completed,
										fbe_u32_t duration /*in 500ms increments*/)
{
	
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_CYCLE;
	discovery_operation->command.power_cycle_command.client_id = client_id;
	discovery_operation->command.power_cycle_command.completed = completed;
	discovery_operation->command.power_cycle_command.duration = duration;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_common_command(fbe_payload_discovery_operation_t * discovery_operation, 
										  fbe_payload_discovery_opcode_t opcode,
										  fbe_object_id_t client_id)
{
	
	discovery_operation->discovery_opcode = opcode;
	discovery_operation->command.common_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_get_opcode(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_opcode_t * discovery_opcode)
{
	* discovery_opcode = discovery_operation->discovery_opcode;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_get_children_list_command_sas_address(fbe_payload_discovery_operation_t * discovery_operation,
															fbe_sas_address_t * sas_address)
{
	* sas_address = discovery_operation->command.get_element_list_command.address.sas_address;

	return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_discovery_build_get_server_info(fbe_payload_discovery_operation_t * discovery_operation,
                                            fbe_object_id_t client_id)
{
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SERVER_INFO;
    discovery_operation->command.get_server_info_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_discovery_build_notify_fw_activation_status(fbe_payload_discovery_operation_t * discovery_operation,
                                                        fbe_bool_t in_progress,
                                                        fbe_object_id_t client_id)
{
	discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_NOTIFY_FW_ACTIVATION_STATUS;
    discovery_operation->command.notify_fw_status_command.activation_in_progress = in_progress;
    discovery_operation->command.notify_fw_status_command.client_id = client_id;
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_discovery_build_encl_SN(fbe_payload_discovery_operation_t * discovery_operation,  
                                    fbe_payload_discovery_check_dup_encl_SN_data_t *op_data,
                                    fbe_u8_t *encl_SN,
                                    fbe_u32_t length)
{
    if (op_data == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (length > FBE_PAYLOAD_DISCOVERY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_CHECK_DUP_ENCL_SN;
    op_data->serial_number_length = length;
    fbe_copy_memory(op_data->encl_SN, encl_SN, length);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_discovery_build_get_drive_location_info(fbe_payload_discovery_operation_t * discovery_operation, fbe_object_id_t client_id)
{
    discovery_operation->discovery_opcode = FBE_PAYLOAD_DISCOVERY_OPCODE_GET_DRIVE_LOCATION_INFO;
    discovery_operation->command.common_command.client_id = client_id;
    return FBE_STATUS_OK;
}
