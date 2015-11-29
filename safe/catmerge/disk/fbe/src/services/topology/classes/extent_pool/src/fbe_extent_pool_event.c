#include "fbe_extent_pool_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_database.h"

/* Forward declarations */

/*!***************************************************************
 * fbe_extent_pool_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the extent_pool class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
 * @param event_context - Context that is associated with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  06/06/2014 - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_event_entry(fbe_object_handle_t object_handle, 
                                fbe_event_type_t event_type,
                                fbe_event_context_t event_context)
{
    fbe_extent_pool_t * extent_pool_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    
    extent_pool_p = (fbe_extent_pool_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            //status = fbe_extent_pool_state_change_event_entry(extent_pool_p, event_context);
            break;        

        case FBE_EVENT_TYPE_PEER_NONPAGED_WRITE:
            //status = fbe_extent_pool_peer_nonpaged_write_event_entry(object_handle, event_type, event_context);
            break;

        case FBE_EVENT_TYPE_PEER_NONPAGED_CHKPT_UPDATE:
            //status = fbe_extent_pool_send_checkpoint_notification(extent_pool_p);
            break;

        case FBE_EVENT_TYPE_PEER_MEMORY_UPDATE:
            //fbe_extent_pool_handle_peer_memory_update(extent_pool_p);
            /* We need to call both memory update handlers since both PVD and base config might have work to do.
             */
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;

        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            //status = fbe_extent_pool_event_peer_contact_lost(extent_pool_p, event_context);  
            /* We need to call both event handlers since both PVD and base config might have work to do.
             */
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;            

        default:
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}
/******************************************
 * end fbe_extent_pool_event_entry()
 ******************************************/

/*************************************
 * end fbe_extent_pool_event.c
 *************************************/
