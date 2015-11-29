/**************************************************************************
*  
*  *****************************************************************
*  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
*  *****************************************************************
*      SATA drive support was added as black content to R30
*      but SATA drives were never supported for performance reasons.
*      The code has not been removed in the event that we wish to
*      re-address the use of SATA drives at some point in the future.
*
***************************************************************************/

#include "fbe/fbe_types.h"
#include "sata_physical_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "base_physical_drive_private.h"

/* Forward declarations */
static fbe_status_t sata_physical_drive_edge_state_change_event_entry(fbe_sata_physical_drive_t * sata_physical_drive,
																	  fbe_event_context_t event_context);

static fbe_status_t sata_physical_drive_check_attributes(fbe_sata_physical_drive_t * sata_physical_drive);

fbe_status_t 
fbe_sata_physical_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_sata_physical_drive_t * sata_physical_drive = NULL;
    fbe_status_t status;

    sata_physical_drive = (fbe_sata_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* Handle the event we have received. */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = sata_physical_drive_edge_state_change_event_entry(sata_physical_drive, event_context);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
 
static fbe_status_t 
sata_physical_drive_edge_state_change_event_entry(fbe_sata_physical_drive_t * sata_physical_drive, fbe_event_context_t event_context)
{
    fbe_path_state_t path_state;
    fbe_status_t status;
    fbe_transport_id_t transport_id;
	fbe_object_handle_t object_handle;

    /* Get the transport type from the edge pointer. */
    fbe_base_transport_get_transport_id((fbe_base_edge_t *) event_context, &transport_id);

    /* Process STP edge state change only */
    if (transport_id != FBE_TRANSPORT_ID_STP) {
		object_handle = fbe_base_pointer_to_handle((fbe_base_t *) sata_physical_drive);
		status = fbe_base_physical_drive_event_entry(object_handle, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, event_context);
		return status;
	}

	/* Get stp edge state */
	status = fbe_stp_transport_get_path_state(&sata_physical_drive->stp_edge, &path_state);

	switch(path_state){
		case FBE_PATH_STATE_ENABLED:
			status = sata_physical_drive_check_attributes(sata_physical_drive);
			break;
		case FBE_PATH_STATE_GONE:
			status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const,
											(fbe_base_object_t*)sata_physical_drive,
											FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
			break;
		case FBE_PATH_STATE_DISABLED:
			status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const,
											(fbe_base_object_t*)sata_physical_drive,
											FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY);
			break;
		case FBE_PATH_STATE_INVALID:
		default:
			status = FBE_STATUS_GENERIC_FAILURE;
			break;

	}
    return status;
}

static fbe_status_t 
sata_physical_drive_check_attributes(fbe_sata_physical_drive_t * sata_physical_drive)
{
    fbe_status_t status;
    fbe_path_attr_t path_attr;

    status = fbe_stp_transport_get_path_attributes(&sata_physical_drive->stp_edge, &path_attr);
    if(path_attr & FBE_STP_PATH_ATTR_CLOSED) {
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const,
                                        (fbe_base_object_t*)sata_physical_drive,
                                        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY);
    }

    return FBE_STATUS_OK;
}








