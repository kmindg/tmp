#include "base_discovering_private.h"

fbe_status_t 
fbe_base_discovering_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	fbe_base_discovering_t * base_discovering = NULL;
	fbe_status_t status;

	base_discovering = (fbe_base_discovering_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)base_discovering,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are not interested in any event at this time, so just forward it to the super class */
	status = fbe_base_discovered_event_entry(object_handle, event_type, event_context);

	return status;
}

fbe_status_t 
fbe_base_discovering_discovery_transport_entry(fbe_base_discovering_t * base_discovering, fbe_packet_t * packet)
{
	fbe_status_t status;

	fbe_base_object_trace((fbe_base_object_t*)base_discovering,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are running in executer context, so we are good to extract and fill the data from the protocol */
	status = fbe_base_discovered_discovery_transport_entry((fbe_base_discovered_t *) base_discovering, packet);

	return status;
}

