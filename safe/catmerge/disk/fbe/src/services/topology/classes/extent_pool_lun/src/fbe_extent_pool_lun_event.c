/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ext_pool_lun_event.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for processing events to the LUN object.
 * 
 * @ingroup lun_class_files
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_ext_pool_lun_private.h"
#include "VolumeAttributes.h"
#include "fbe_cmi.h"
#include "EmcPAL_Misc.h"


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_ext_pool_lun_state_change_event_entry(fbe_ext_pool_lun_t * const lun_p, fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_permit_request_event_entry(fbe_ext_pool_lun_t * lun_p, fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_update_verify_report_event_entry(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_power_saving_changed_event_entry(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_handle_data_request_event(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_handle_remap_event(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context);              
static fbe_status_t fbe_ext_pool_lun_process_non_paged_write(fbe_ext_pool_lun_t * lun_p);
static fbe_status_t fbe_ext_pool_lun_process_attribute_changed(fbe_ext_pool_lun_t * lun_p, fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_event_log_event_entry(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context);
static fbe_status_t fbe_ext_pool_lun_event_peer_contact_lost(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context);


/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * fbe_ext_pool_lun_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the lun class.
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
 *  02/23/2010 - add permit request handling. guov
 *
 ****************************************************************/
fbe_status_t 
fbe_ext_pool_lun_event_entry(fbe_object_handle_t object_handle, 
                    fbe_event_type_t event_type,
                    fbe_event_context_t event_context)
{
    fbe_ext_pool_lun_t * lun_p = NULL;
    fbe_status_t status;

    lun_p = (fbe_ext_pool_lun_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_ext_pool_lun_state_change_event_entry(lun_p, event_context);
            break;
        case FBE_EVENT_TYPE_PERMIT_REQUEST:
            status = fbe_ext_pool_lun_permit_request_event_entry(lun_p, event_context);
            break;
        case FBE_EVENT_TYPE_VERIFY_REPORT:
            status = fbe_ext_pool_lun_update_verify_report_event_entry(lun_p, event_context);
            break;
        case FBE_EVENT_TYPE_POWER_SAVING_CONFIG_CHANGED:
            status = fbe_ext_pool_lun_power_saving_changed_event_entry(lun_p, event_context);
            break;
        //  Data Request event
        case FBE_EVENT_TYPE_DATA_REQUEST:
            status = fbe_ext_pool_lun_handle_data_request_event(lun_p, event_context);
            break;
        case FBE_EVENT_TYPE_PEER_NONPAGED_WRITE:
            /*check if this is a change of the first write bit*/
            status = fbe_ext_pool_lun_process_non_paged_write(lun_p);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            /*and give it to base config to process as well*/
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            status = fbe_ext_pool_lun_process_attribute_changed(lun_p, event_context);
            break;
        case FBE_EVENT_TYPE_EVENT_LOG:
            status = fbe_ext_pool_lun_event_log_event_entry(lun_p, event_context);
            break;
        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            status = fbe_ext_pool_lun_event_peer_contact_lost(lun_p, event_context);
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;

        case FBE_EVENT_TYPE_PEER_MEMORY_UPDATE:
            status = lun_ext_pool_process_memory_update_message(lun_p);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
			break;
        default:
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;
    }
    return status;
}
/* end fbe_ext_pool_lun_event_entry() */


/*!**************************************************************
 * fbe_ext_pool_lun_state_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the LUN object handler for a change in the state of 
 *  the edge with downstream objects.
 *
 * @param lun_p - LUN object that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/

static fbe_status_t 
fbe_ext_pool_lun_state_change_event_entry(fbe_ext_pool_lun_t * const lun_p, 
                                 fbe_event_context_t event_context)
{
    fbe_status_t status;
    fbe_lifecycle_state_t   lifecycle_state;
    fbe_path_state_t path_state;
    fbe_block_edge_t *edge_p = NULL;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    /* Get the lifecycle state and associated edge for event.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)lun_p, &lifecycle_state);
    fbe_base_config_get_block_edge(&lun_p->base_config, &edge_p, 0);

    /*! Fetch the path state.  Note that while it returns a status, it currently
     * never fails. 
     * @todo We will need to iterate over the edges, but for now just look at the first
     * one. 
     */ 
    status = fbe_block_transport_get_path_state(edge_p, &path_state);

    if (status != FBE_STATUS_OK)
    {
        /* Not able to get the path state.
         */
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s cannot get block edge path state 0x%x!!\n",
                              __FUNCTION__, status);
        return status;
    }

    switch (path_state)
    {
        case FBE_PATH_STATE_INVALID:
            fbe_base_object_trace((fbe_base_object_t*)lun_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid configured edge path state: 0x%X",
                                  __FUNCTION__,
                                  path_state);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        case FBE_PATH_STATE_ENABLED:
        case FBE_PATH_STATE_DISABLED:
        case FBE_PATH_STATE_BROKEN:
        case FBE_PATH_STATE_GONE:
            /* Determine the health of the communication path with 
             * downstream objects.
             */
            downstream_health_state = fbe_ext_pool_lun_verify_downstream_health(lun_p);

            /* Set the specific lifecycle condition based on health of the 
             * communication path with downstream objects.
             */
            status =  fbe_ext_pool_lun_set_condition_based_on_downstream_health(lun_p,
                                                                       downstream_health_state);

            /* If the lifecycle is not Ready but the downstream health is now 
             * optimal reschedule immediately.
             */
            if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)             &&
                (downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL)    )
            {
                /* Reschedule immediately.
                 */
                fbe_lifecycle_reschedule(&fbe_ext_pool_lun_lifecycle_const,
                                         (fbe_base_object_t *) lun_p,
                                         (fbe_lifecycle_timer_msec_t) 0);
            }

            break;
        case FBE_PATH_STATE_SLUMBER:
            break;
        default:
            /* This is a path state we do not expect.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                                  "%s path state unexpected %d\n", __FUNCTION__, path_state);
            break;

    }
    return status;
}
/******************************************
 * end fbe_ext_pool_lun_state_change_event_entry()
 ******************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_permit_request_event_entry()
 ****************************************************************
 * @brief
 *  This is the handler function for permit request to the LUN
 *  object, Here the LUN can reject the request base on it's 
 *  setting and state.
 *
 * @param lun_p         - lun object which has received event.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  02/23/2010 - Created. guov
 *
 ****************************************************************/
static fbe_status_t fbe_ext_pool_lun_permit_request_event_entry(fbe_ext_pool_lun_t * lun_p,
                                                       fbe_event_context_t event_context)
{
    fbe_status_t status;
    fbe_event_t*                event_p = NULL;
    fbe_event_stack_t*          event_stack_p = NULL;
    fbe_event_permit_request_t  permit_request_info;
    fbe_lba_t                   event_start_lba;        //  starting LBA from the event 
    fbe_block_count_t           event_block_count;      //  block count from the event 
    fbe_lba_t                   event_end_lba;          //  calculated ending lba from the event 
    fbe_lba_t                   lun_capacity;           //  capacity of this LUN 
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_lba_t                   external_capacity;
    fbe_block_count_t           zero_bitmap_blocks;
    /* Trace function entry */
    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_ext_pool_lun_event: %s entry\n", __FUNCTION__);

    /* Set a pointer to the event, which is passed to us as "event context" */
    event_p = (fbe_event_t*) event_context;  

    /* If the LUN is not ready yet, then deny the permission. 
     * This prevents a LUN that is transitioning out of specialize, activate or fail to deny the request. 
     * LUN needs to deny since if it was just bound then it might not have sent zero or clear nz to the raid group. 
     * We don't want the raid group to do any background ops until the LUN has had a chance to initialize and 
     * send the zero or clear zero. 
     * Hibernate is treated special since a LUN is expected to hibernate before the raid group does.  If 
     * the RG or below is trying to finish up a bg operation then we want the RG to finish first, then hibernate. 
     */
    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *) lun_p, &lifecycle_state);
    if ((status != FBE_STATUS_OK) || 
        ((lifecycle_state != FBE_LIFECYCLE_STATE_READY) && (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun permit event: deny event type 0x%x lifecycle_state: 0x%x\n", 
                              event_p->event_type, lifecycle_state);
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }
    //  Get the permit request data from the event and set up the LUN's object id in it
    fbe_event_get_permit_request_data(event_p, &permit_request_info);  
    fbe_base_object_get_object_id((fbe_base_object_t*) lun_p, &permit_request_info.object_id);
    fbe_lun_get_object_index(lun_p, &permit_request_info.object_index);
    //  Get the event's starting and ending LBA
    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(event_stack_p, &event_start_lba, &event_block_count); 
    event_end_lba = event_start_lba + event_block_count - 1;

    //  Get the LUN capacity.  The LUN starts at LBA 0 and ends at its imported capacity - 1.  
    fbe_lun_get_imported_capacity(lun_p, &lun_capacity);

    //  Check if the event LBA is the start of the LUN and set the info in the permit data accordingly
    permit_request_info.is_start_b = FBE_FALSE; 
    if (event_start_lba == 0)
    {
        permit_request_info.is_start_b = FBE_TRUE;
    }

    //  Check if the event LBA is the end of the LUN and set the info in the permit data accordingly.  (If the block
    //  count is large enough, then one event can encompass both the start and the end of the LUN.)
    permit_request_info.is_end_b = FBE_FALSE; 
    if (event_end_lba >= (lun_capacity - 1))
    {
        permit_request_info.is_end_b = FBE_TRUE;
    }
    
    fbe_base_config_get_capacity((fbe_base_config_t*) lun_p, &external_capacity);

    /* We need to report it is beyond the user's capacity when we go beyond the zero bitmap.
     */
    fbe_ext_pool_lun_calculate_cache_zero_bit_map_size(external_capacity, &zero_bitmap_blocks);
    if (event_end_lba >= (external_capacity - zero_bitmap_blocks)) {
        permit_request_info.is_beyond_capacity_b = FBE_TRUE;
    }
    permit_request_info.top_lba = event_start_lba;

    /* Debug low traces
     */
    fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "lun: event stack current: 0x%llx prev: 0x%llx lba: 0x%llx blocks: 0x%llx \n",
                          (unsigned long long)event_stack_p->current_offset,
			  (unsigned long long)event_stack_p->previous_offset,
                          (unsigned long long)event_start_lba,
			  (unsigned long long)event_block_count);
    fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "lun: event type: %d status: 0x%x obj: 0x%x is_start: %d is_end: %d unconsumed: 0x%llx \n",
                          permit_request_info.permit_event_type, /*! @todo Status is always OK*/ FBE_EVENT_STATUS_OK,
                          permit_request_info.object_id,
			  permit_request_info.is_start_b,
                          permit_request_info.is_end_b,
			  (unsigned long long)permit_request_info.unconsumed_block_count);

    //  Write the permit info into the event   
    fbe_event_set_permit_request_data(event_p, &permit_request_info);

    //  Complete the event 
    fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);

    //  Return success 
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_ext_pool_lun_permit_request_event_entry()
 ****************************************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_update_verify_report_event_entry()
 ****************************************************************
 * @brief
 *  This function updates the LUN verify report with the error 
 *  counts received from RAID.
 *
 *
 * @param lun_p - lun object which has received event.
 * @param event_context - The context for the event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/31/2010 - Created Ashwin
 *
 ****************************************************************/
static fbe_status_t fbe_ext_pool_lun_update_verify_report_event_entry(fbe_ext_pool_lun_t * lun_p,
                                                             fbe_event_context_t event_context)
{
    fbe_event_t*                        event_p;
    fbe_event_verify_report_t           verify_event_data;
    fbe_lun_verify_report_t*            verify_report_p;


    /* Trace function entry */
    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_ext_pool_lun_event: %s entry\n", __FUNCTION__);

    /* Set a pointer to the event, which is passed to us as "event context" */
    event_p = (fbe_event_t*) event_context;  
    fbe_event_get_verify_report_data(event_p, &verify_event_data);

    //  Complete the event 
    fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);

    // Get a pointer to the verify report
    verify_report_p = fbe_lun_get_verify_report_ptr(lun_p);
    
    fbe_ext_pool_lun_lock(lun_p);
    // Add the error counts to the current pass data
    fbe_ext_pool_lun_usurper_add_verify_result_counts(&verify_event_data.error_counts, &verify_report_p->current);

    // Add the error counts to the historical totals
    fbe_ext_pool_lun_usurper_add_verify_result_counts(&verify_event_data.error_counts, &verify_report_p->totals);

    // Data disks is a field in the verify report event data that is just used for RAID 10
    // For all other raid types this field is not used and hence it will be zero
    if (verify_event_data.pass_completed_b == FBE_TRUE && verify_event_data.data_disks == 0)
    {
        // A pass completed. Increment the pass counter
        verify_report_p->pass_count++;
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "verify complete pass: %d \n",
                              verify_report_p->pass_count);
    }

    // For RAID 10 lun each time a verify report event data comes
    // we will increment the mirror objects count. When the number of mirror objects count
    // equals the number of data disks, we know we have received the verify report from all
    // the mirror objects attached to the striper and so we increment the pass count
    else if((verify_event_data.pass_completed_b == FBE_TRUE && verify_event_data.data_disks != 0))
    {
        // Its a RAID 10 lun. Increment the mirror objects count
        verify_report_p->mirror_objects_count++;
        if(verify_report_p->mirror_objects_count == verify_event_data.data_disks)
        {
            // A pass completed. Increment the pass counter
            verify_report_p->pass_count++;
        }
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "verify complete pass: %d mirror_objects_count: %d data_disks: %d\n",
                              verify_report_p->pass_count,
                              verify_report_p->mirror_objects_count,
                              verify_event_data.data_disks);
    }
    fbe_ext_pool_lun_unlock(lun_p);

    return FBE_STATUS_OK;
} 
/****************************************************
 * end fbe_ext_pool_lun_update_verify_report_event_entry()
 ****************************************************/

/*!**************************************************************
 * fbe_ext_pool_lun_power_saving_changed_event_entry()
 ****************************************************************
 * @brief
 *  This function will ask RAID what are the power saving configuration changed to
 *  In theory, LU was suposed to be updated with FBE_LUN_CONTROL_CODE_SET_POWER_SAVING_POLICY
 *  but it seems layerd drivers do not support that yet so we have to get it from
 *  RAID which gets it from NAVI
 *
 * @param lun_p - lun object which has received event.
 * @param event_context - The context for the event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/03/2010 - Created Shay
 *
 ****************************************************************/
static fbe_status_t fbe_ext_pool_lun_power_saving_changed_event_entry(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context)
{
    fbe_status_t    status;

    /*all we do is set a condition and let the monitor take care of that.
    It will send a packet to RAID to ask for the information*/
    status = fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                                    (fbe_base_object_t*)lun_p,
                                     FBE_LUN_LIFECYCLE_COND_LUN_GET_POWER_SAVE_INFO_FROM_RAID);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, failed to set condition\n", __FUNCTION__);
    }

     return status;
}
/****************************************************
 * end fbe_ext_pool_lun_power_saving_changed_event_entry()
 ****************************************************/


/*!****************************************************************************
 * fbe_ext_pool_lun_handle_data_request_event()
 ******************************************************************************
 * @brief
 *   This function will handle a "data request" event. This function
 *   determines the type of data event and initiates action accordingly.
 *
 * @param lun_p - lun object which has received event.
 * @param event_context - The context for the event. 
 *
 * @return fbe_status_t   
 *
 * @author
 *   9/1/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
static fbe_status_t fbe_ext_pool_lun_handle_data_request_event(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context)
{

    fbe_event_t*                event_p;
    fbe_event_data_request_t    data_request; 
    fbe_data_event_type_t       data_event_type;


    /* Trace function entry */
    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_ext_pool_lun_event: %s entry\n", __FUNCTION__);


    event_p  =(fbe_event_t*)event_context;
    fbe_event_get_data_request_data(event_p, &data_request);
    data_event_type = data_request.data_event_type;

    switch (data_event_type)
    {
        case FBE_DATA_EVENT_TYPE_REMAP:
            fbe_ext_pool_lun_handle_remap_event(lun_p, event_context);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t*)lun_p,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "%s unsupported data event type\n", __FUNCTION__);
    }

    return FBE_STATUS_OK;

} // End fbe_ext_pool_lun_handle_data_request_event()




/*!***************************************************************
 * fbe_ext_pool_lun_handle_remap_event
 *****************************************************************
 * @brief
 *    This function handles processing remap events. This event is sent
 *    by the RAID group to determine whether the specified lba  is
 *    part of bound LUN.
 *
 * @param lun_p - lun object which has received event.
 * @param event_context - The context for the event. 
 *
 * @return  fbe_status_t      -  remap event processing status
 *
 * @author
 *    09/01/2010 - Created. Maya Dagon
 *
 ****************************************************************/

fbe_status_t
fbe_ext_pool_lun_handle_remap_event(fbe_ext_pool_lun_t * lun_p,fbe_event_context_t event_context) 
{

    fbe_event_t*                    event_p;            // pointer to event

    /* Trace function entry */
    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_ext_pool_lun_event: %s entry\n", __FUNCTION__);
 
    // cast context to event object
    event_p = (fbe_event_t *)event_context;

    // if packet reached Lun , it means that the specified lba is within the Lun extent. 
    // We don't handle differently signature and user space
    // so we can just return STATUS_OK and let RAID mark verify

    // set status in event and complete event
    fbe_event_set_status( event_p, FBE_EVENT_STATUS_OK );
    fbe_event_complete( event_p );

    return FBE_STATUS_OK;
    
}   // end fbe_ext_pool_lun_handle_remap_event()

static fbe_status_t fbe_ext_pool_lun_process_non_paged_write(fbe_ext_pool_lun_t * lun_p)
{
	fbe_status_t						status;
    fbe_lun_nonpaged_metadata_t * 		lun_nonpaged_metadata = NULL;
    
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) lun_p, (void **)&lun_nonpaged_metadata);
	if (lun_nonpaged_metadata == NULL) {
		fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to get metadata ptr", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    /*if we got a the first write but it was on the peer, we make sure we are up to dade*/
    if (FBE_IS_TRUE(lun_nonpaged_metadata->has_been_written) && (!fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_HAS_BEEN_WRITTEN))) {

		fbe_lun_set_flag(lun_p, FBE_LUN_FLAGS_HAS_BEEN_WRITTEN);

		status = fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
										(fbe_base_object_t*)lun_p,
										 FBE_LUN_LIFECYCLE_COND_LUN_SIGNAL_FIRST_WRITE);


		 if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)lun_p, 
							  FBE_TRACE_LEVEL_ERROR, 
							  FBE_TRACE_MESSAGE_ID_INFO,
							  "%s, failed to set condition\n", __FUNCTION__);
		 }

        
	}

	return FBE_STATUS_OK;
}

static fbe_status_t fbe_ext_pool_lun_process_attribute_changed(fbe_ext_pool_lun_t * lun_p, fbe_event_context_t event_context)
{
	fbe_status_t 						status;
	fbe_block_edge_t*      				edge_p;
	fbe_path_attr_t						attr;


	/* Get the edge from the event context */
	edge_p = (fbe_block_edge_t *)event_context;

	/* Get the attribute from the edge */
	status = fbe_block_transport_get_path_attributes(edge_p, &attr);
	if (status != FBE_STATUS_OK) {
		return status;
	}

    /* we only need to propogate path attribute change, nothing else (medic) */
    if (attr != lun_p->prev_path_attr) {
        lun_p->prev_path_attr = attr;

        /* Propogate the attribute upstream.
	    We mask out FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN so it won't get overwritten since LUN may set it*/
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&((fbe_base_config_t *)lun_p)->block_transport_server, 
																		     attr,
																		     ~FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN);
    }

	return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_ext_pool_lun_event_log_event_entry()
 ****************************************************************
 * @brief
 *  This function will log an event.
 *
 *
 * @param lun_p - lun object which has received event.
 * @param event_context - The context for the event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/30/2011 - Created Vishnu Sharma
 *
 ****************************************************************/
static fbe_status_t fbe_ext_pool_lun_event_log_event_entry(fbe_ext_pool_lun_t * lun_p,
                                                  fbe_event_context_t event_context)
{
    fbe_event_t*                        event_p;
    fbe_event_event_log_request_t       event_log_data;
    


    /* Trace function entry */
    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_ext_pool_lun_event: %s entry\n", __FUNCTION__);

    /* Set a pointer to the event, which is passed to us as "event context" */
    event_p = (fbe_event_t*) event_context;  
    fbe_event_get_event_log_request_data(event_p, &event_log_data);

    //  Complete the event 
    fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);

    fbe_ext_pool_lun_event_log_event(lun_p,&event_log_data);
    
    return FBE_STATUS_OK;
} 



/*!**************************************************************
 * fbe_ext_pool_lun_event_peer_contact_lost()
 ****************************************************************
 * @brief
 *  This function will log an event.
 *
 *
 * @param lun_p - lun object which has received event.
 * @param event_context - The context for the event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/29/2012 - Created. MFerson
 *
 ****************************************************************/
static fbe_status_t fbe_ext_pool_lun_event_peer_contact_lost(fbe_ext_pool_lun_t * lun_p,
                                                    fbe_event_context_t event_context)
{
    fbe_lifecycle_state_t   lifecycle_state;
    fbe_status_t            status;
    fbe_base_config_t *base_config_p = (fbe_base_config_t *)lun_p;

    status = fbe_lifecycle_get_state(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t*)lun_p, &lifecycle_state);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "LUN peer lost statel/p:%u/%u mde_st: %x %c lcf:%x/%x bcf:%x/%x\n", 
                          lifecycle_state,
                          lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state,
                          base_config_p->metadata_element.metadata_element_state,
                          ((base_config_p->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE) ? 'A' : 'P'),
                          (unsigned int)lun_p->lun_metadata_memory.flags, 
                          (unsigned int)lun_p->lun_metadata_memory_peer.flags,
                          (unsigned int)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                          (unsigned int)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags);
    fbe_ext_pool_lun_lock(lun_p);
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
    fbe_ext_pool_lun_unlock(lun_p);

    if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY)) {
        status = fbe_lifecycle_set_cond(
            &fbe_ext_pool_lun_lifecycle_const,
            (fbe_base_object_t*)lun_p,
            FBE_LUN_LIFECYCLE_COND_CHECK_PEER_DIRTY_FLAG);
    }
    
    return FBE_STATUS_OK;
} 
