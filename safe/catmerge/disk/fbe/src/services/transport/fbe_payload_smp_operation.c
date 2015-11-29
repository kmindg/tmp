#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"

fbe_status_t 
fbe_payload_smp_get_status(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_t * status)
{
	*status = smp_operation->status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_smp_set_status(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_t status)
{
	smp_operation->status = status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_smp_get_status_qualifier(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_qualifier_t * status_qualifier)
{
	*status_qualifier = smp_operation->status_qualifier;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_smp_set_status_qualifier(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_qualifier_t status_qualifier)
{
	smp_operation->status_qualifier = status_qualifier;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_smp_get_opcode(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_opcode_t * smp_opcode)
{
    *smp_opcode = smp_operation->smp_opcode;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_smp_build_get_element_list(fbe_payload_smp_operation_t * smp_operation,
                                       fbe_address_t address,
                                       fbe_generation_code_t generation_code)
{
    smp_operation->smp_opcode = FBE_PAYLOAD_SMP_OPCODE_GET_ELEMENT_LIST;
    smp_operation->command.get_element_list_command.address = address;
    smp_operation->command.get_element_list_command.generation_code = generation_code;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_smp_get_children_list_command_sas_address(fbe_payload_smp_operation_t * smp_operation,
                                                      fbe_sas_address_t * sas_address)
{
    *sas_address = smp_operation->command.get_element_list_command.address.sas_address;
    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_payload_smp_build_reset_end_device(fbe_payload_smp_operation_t * smp_operation,
                                     fbe_u8_t phy_id)
{
    smp_operation->smp_opcode = FBE_PAYLOAD_SMP_OPCODE_RESET_END_DEVICE;
    smp_operation->command.reset_end_device_command.phy_id = phy_id;
    return FBE_STATUS_OK;
}
