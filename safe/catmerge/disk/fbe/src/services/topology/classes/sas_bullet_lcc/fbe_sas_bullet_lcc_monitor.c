#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_ses.h"
#include "sas_bullet_lcc_private.h"

fbe_status_t 
fbe_sas_bullet_lcc_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_bullet_lcc_t * sas_bullet_lcc = NULL;

	sas_bullet_lcc = (fbe_sas_bullet_lcc_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_bullet_lcc,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry .\n", __FUNCTION__);

	status = fbe_sas_bullet_lcc_monitor(sas_bullet_lcc, packet);
	return status;
}
