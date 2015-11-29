/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_parity_event.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the event entry for the parity object.
 *
 * @version
 *   6/23/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_parity.h"
#include "base_object_private.h"
#include "fbe_parity_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_needs_rebuild.h"

/*************************
 * LOCAL PROTOTYPES
 *************************/
static fbe_status_t fbe_parity_peer_contact_lost_eval_write_log_flush(fbe_parity_t * parity_p);

/*!***************************************************************
 * fbe_parity_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the parity class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
 * @param event_context - Context that is associated with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_parity_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type,
                              fbe_event_context_t event_context)
{
    fbe_parity_t * parity_p = NULL;
    fbe_block_edge_t *edge_p = NULL;
    fbe_status_t status;

    parity_p = (fbe_parity_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)parity_p,
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
                    fbe_raid_group_is_in_download((fbe_raid_group_t*)parity_p))
                {
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) parity_p,
                                           FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST);
                }

                if (((fbe_base_edge_t *)edge_p)->path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
                {
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) parity_p,
                                           FBE_RAID_GROUP_LIFECYCLE_COND_KEYS_REQUESTED);
                }
            }
            fbe_raid_group_attribute_changed((fbe_raid_group_t*)parity_p, event_context);
            status = fbe_raid_group_edge_state_change_event_entry ((fbe_raid_group_t*)parity_p, event_context);
            break;

        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_raid_group_edge_state_change_event_entry ((fbe_raid_group_t*)parity_p, event_context);
            break;

        case FBE_EVENT_TYPE_SPARING_REQUEST:
            status = fbe_raid_group_handle_sparing_request_event((fbe_raid_group_t*)parity_p, event_context);
            break;

        case FBE_EVENT_TYPE_COPY_REQUEST:
            status = fbe_raid_group_handle_copy_request_event((fbe_raid_group_t*)parity_p, event_context);
            break;  

        case FBE_EVENT_TYPE_ABORT_COPY_REQUEST:
            status = fbe_raid_group_handle_abort_copy_request_event((fbe_raid_group_t*)parity_p, event_context);
            break;

        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            status = fbe_parity_peer_contact_lost_eval_write_log_flush(parity_p); 
            /* We also need to call raid_group event handler since it will have more work to do. */
            status = fbe_raid_group_event_entry(object_handle, event_type, event_context);
            break; 
                    
        default:
            status = fbe_raid_group_event_entry(object_handle, event_type, event_context);
            break;
    }
    return status;
}
/* end fbe_parity_event_entry() */

/*!**************************************************************
 * fbe_parity_peer_contact_lost_eval_write_log_flush()
 ****************************************************************
 * @brief
 *  This function handles clustered memory update for slf.
 *
 * @param parity_p - parity object that needs to handle peer_contact lost.
 *
 * @return - Status of the handling.
 *
 * @author
 *  5/14/2012 - Created. Vamsi V.
 *
 ****************************************************************/
fbe_status_t fbe_parity_peer_contact_lost_eval_write_log_flush(fbe_parity_t * parity_p)
{
	fbe_status_t status;
    fbe_bool_t is_flush_req;
    fbe_lifecycle_state_t lifecycle_state;

    /* Get state of the object */
    status = fbe_lifecycle_get_state(&fbe_parity_lifecycle_const, (fbe_base_object_t*)parity_p, &lifecycle_state);

    /* Check if Flush is required */
    is_flush_req = fbe_raid_group_is_write_log_flush_required((fbe_raid_group_t *)parity_p);

	/* If state is in Specialize/Activate, Flushes run as preset cond. Nothing to do here. */
    if ((is_flush_req == FBE_TRUE) &&
        (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
    {
        /* Set write_log slots of peerSP as valid as this SP will do the Flushes (see function description) */
        fbe_parity_write_log_set_all_slots_valid(parity_p->write_log_info_p, FBE_FALSE);

        /* set the do_journal_flush condition in the raid group object */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               (fbe_base_object_t *) parity_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_FLUSH);

        fbe_base_object_trace((fbe_base_object_t *) parity_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_peer_lost: JOURNAL FLUSH condition set on rg: 0x%x\n",
                              parity_p->raid_group.base_config.base_object.object_id);
    }
    
    return FBE_STATUS_OK;
}
/********************************************************
 * end fbe_parity_peer_contact_lost_eval_write_log_flush()
 ********************************************************/

/*************************
 * end file fbe_parity_event.c
 *************************/


