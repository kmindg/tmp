#include "fbe_iscsi_port.h"
#include "iscsi_port_private.h"



/* Forward declaration */

fbe_status_t 
fbe_iscsi_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_iscsi_port_t * iscsi_port = NULL;
	fbe_payload_control_operation_opcode_t control_code;

	iscsi_port = (fbe_iscsi_port_t *)fbe_base_handle_to_pointer(object_handle);
	fbe_base_object_trace(	(fbe_base_object_t*)iscsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	control_code = fbe_transport_get_control_code(packet);
	switch(control_code) {

        default:
			status = fbe_base_port_control_entry(object_handle, packet);
			break;
	}

	return status;
}
