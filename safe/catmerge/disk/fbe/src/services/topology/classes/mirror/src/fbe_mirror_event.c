/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_event.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the mirror event entry.
 *
 * @version
 *   6/23/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_mirror.h"
#include "base_object_private.h"
#include "fbe_mirror_private.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_group_needs_rebuild.h"

/*************************
 * LOCAL PROTOTYPES
 *************************/

/*!***************************************************************
 * fbe_mirror_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the mirror class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
 * @param event_context - Context that is associated with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type,
                              fbe_event_context_t event_context)
{
    fbe_mirror_t * mirror_p = NULL;
    fbe_status_t status;
    fbe_block_edge_t *edge_p;

    mirror_p = (fbe_mirror_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)mirror_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            /* If the request is related to download request viablity check, we need to handle it differently.
             */
            edge_p = (fbe_block_edge_t *)event_context;
            if (edge_p != NULL) 
            {
                if (fbe_block_transport_is_download_attr(edge_p) || 
                    fbe_raid_group_is_in_download((fbe_raid_group_t*)mirror_p))
                {
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) mirror_p,
                                           FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST);
                }

                if (((fbe_base_edge_t *)edge_p)->path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
                {
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) mirror_p,
                                           FBE_RAID_GROUP_LIFECYCLE_COND_KEYS_REQUESTED);
                }
            }

            fbe_raid_group_attribute_changed((fbe_raid_group_t*)mirror_p, event_context);
            status = fbe_raid_group_edge_state_change_event_entry ((fbe_raid_group_t*)mirror_p, event_context);
			break;

        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_raid_group_edge_state_change_event_entry ((fbe_raid_group_t*)mirror_p, event_context);
			break;

        case FBE_EVENT_TYPE_SPARING_REQUEST:
            status = fbe_raid_group_handle_sparing_request_event((fbe_raid_group_t*)mirror_p, event_context);
            break;

        case FBE_EVENT_TYPE_COPY_REQUEST:
            status = fbe_raid_group_handle_copy_request_event((fbe_raid_group_t*)mirror_p, event_context);
            break;
              
        case FBE_EVENT_TYPE_ABORT_COPY_REQUEST:
            status = fbe_raid_group_handle_abort_copy_request_event((fbe_raid_group_t*)mirror_p, event_context);
            break; 

        default:
			status = fbe_raid_group_event_entry(object_handle, event_type, event_context);
			break;

    }
    /* We also need to let the base class know about the event.
     */
    return status;
}
/* end fbe_mirror_event_entry() */

/*************************
 * end file fbe_mirror_event.c
 *************************/


