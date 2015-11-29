#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"

fbe_status_t 
fbe_payload_control_build_operation(fbe_payload_control_operation_t * control_operation,
									fbe_payload_control_operation_opcode_t	opcode,	
									fbe_payload_control_buffer_t			buffer,
									fbe_payload_control_buffer_length_t		buffer_length)
{
	control_operation->opcode = opcode;
	control_operation->buffer = buffer;
	control_operation->buffer_length = buffer_length;
	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_control_get_opcode(fbe_payload_control_operation_t * control_operation, fbe_payload_control_operation_opcode_t * opcode)
{
	* opcode = 0;

	* opcode = control_operation->opcode;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_control_get_buffer_impl(fbe_payload_control_operation_t * control_operation, fbe_payload_control_buffer_t * buffer)
{
	* buffer = NULL;

	* buffer = control_operation->buffer;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_control_get_buffer_length(fbe_payload_control_operation_t * control_operation, fbe_payload_control_buffer_length_t * buffer_length)
{
	* buffer_length = 0;

	* buffer_length = control_operation->buffer_length;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_control_get_status(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_t * status)
{
	*status = control_operation->status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_control_set_status(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_t status)
{
	control_operation->status = status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_control_get_status_qualifier(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_qualifier_t * status_qualifier)
{
	*status_qualifier = control_operation->status_qualifier;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_control_set_status_qualifier(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_qualifier_t status_qualifier)
{
	control_operation->status_qualifier = status_qualifier;
	return FBE_STATUS_OK;
}
