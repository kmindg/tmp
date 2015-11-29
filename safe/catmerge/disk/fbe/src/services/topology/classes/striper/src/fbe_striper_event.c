/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_striper_event.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the event entry for the striper object.
 *
 * @version
 *   6/23/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_striper.h"
#include "base_object_private.h"
#include "fbe_striper_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_group_object.h"

/*************************
 * LOCAL PROTOTYPES
 *************************/
static fbe_status_t fbe_striper_main_handle_sparing_request_event(fbe_striper_t *in_striper_p,
                                                                  fbe_event_context_t in_event_context);
static fbe_status_t fbe_striper_main_handle_copy_request_event(fbe_striper_t *in_striper_p,
                                                               fbe_event_context_t in_event_context);
static fbe_status_t fbe_striper_main_handle_abort_copy_request_event(fbe_striper_t *in_striper_p,
                                                               fbe_event_context_t in_event_context);
static fbe_status_t fbe_striper_main_handle_permit_request_event(fbe_striper_t * striper_p,
                                                                 fbe_event_context_t in_event_context);
static fbe_status_t fbe_striper_main_drop_stripe_locks_and_monitor_ops(fbe_striper_t * striper_p);
static fbe_status_t fbe_striper_attribute_change_event_entry(fbe_striper_t * const striper_p,
                                                             fbe_event_context_t event_context);
static fbe_status_t fbe_striper_main_handle_event_log_request(fbe_striper_t * in_striper_p, fbe_event_context_t in_event_context);


/*!***************************************************************
 * fbe_striper_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the striper class.
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
fbe_striper_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type,
                              fbe_event_context_t event_context)
{
    fbe_striper_t * striper_p = NULL;
    fbe_status_t status;

    striper_p = (fbe_striper_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)striper_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            status = fbe_striper_attribute_change_event_entry(striper_p, event_context);
            break;

        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_striper_state_change_event_entry (striper_p, event_context);
            break;

        case FBE_EVENT_TYPE_SPARING_REQUEST:
            status = fbe_striper_main_handle_sparing_request_event(striper_p, event_context);
            break;

        case FBE_EVENT_TYPE_COPY_REQUEST:
            status = fbe_striper_main_handle_copy_request_event(striper_p, event_context);
            break;

        case FBE_EVENT_TYPE_ABORT_COPY_REQUEST:
            status = fbe_striper_main_handle_abort_copy_request_event(striper_p, event_context);
            break;

        case FBE_EVENT_TYPE_EVENT_LOG:
            status = fbe_striper_main_handle_event_log_request(striper_p, event_context);
            break;    

        case FBE_EVENT_TYPE_PERMIT_REQUEST:
            status = fbe_striper_main_handle_permit_request_event(striper_p, event_context);
            if(status != FBE_STATUS_TRAVERSE)
            {
                break;
            } 
            // else continue to go through default case.

        default:
            status = fbe_raid_group_event_entry(object_handle, event_type, event_context);
            break;

    }
    return status;
}
/* end fbe_striper_event_entry() */


/*!**************************************************************
 * fbe_striper_state_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the striper handler for a change in the state of the 
 *  edge with downstream objects.
 *
 * @param striper_p -        Striper RAID type that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  8/18/2009 - Created. Dhaval Patel
 *
 ****************************************************************/

fbe_status_t 
fbe_striper_state_change_event_entry(fbe_striper_t * const striper_p,
                                     fbe_event_context_t event_context)
{
    fbe_path_state_t path_state;
    fbe_status_t status;
    fbe_block_edge_t *edge_p = NULL;
    fbe_base_config_downstream_health_state_t downstream_health_state;


    /*! Fetch the path state.  Note that while it returns a status, it currently
     * never fails. 
     */ 
    edge_p = (fbe_block_edge_t*) event_context;
    status = fbe_block_transport_get_path_state(edge_p, &path_state);

    if (status != FBE_STATUS_OK)
    {
        /* Not able to get the path state.
         */
        fbe_base_object_trace((fbe_base_object_t*)striper_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s cannot get block edge path state 0x%x!!\n",
                              __FUNCTION__, status);
        return status;
    }

    switch (path_state)
    {
        case FBE_PATH_STATE_INVALID:
            fbe_base_object_trace((fbe_base_object_t*)striper_p,
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
            downstream_health_state = fbe_striper_verify_downstream_health(striper_p);

            /* Set the specific lifecycle condition based on health of the 
             * communication path with downstream objects.
             */
            status =  fbe_striper_set_condition_based_on_downstream_health (striper_p,
                                                                            downstream_health_state);
             break;
        case FBE_PATH_STATE_SLUMBER:
            break;
        default:
            /* This is a path state we do not expect.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)striper_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                                  "%s path state unexpected %d\n", __FUNCTION__, path_state);
            break;

    }
    return status;
}
/*****************************************
 * end fbe_striper_state_change_event_entry()
 *****************************************/


/*!**************************************************************
 * fbe_striper_verify_downstream_health()
 ****************************************************************
 * @brief
 *  This function is used to verify the health of the downstream
 *  objects, it will return any of the three return type based on
 *  state of the edge with its downstream objects and specific 
 *  RAID type.
 *
 * @param fbe_striper_t - striper object.
 *
 * @return fbe_status_t
 *  It returns any of the three return type based on RAID type and
 *  state of the edges with downstream objects.
 * 
 *  1) FBE_DOWNSTREAM_HEALTH_OPTIMAL: Communication with downstream
 *                                    object is fully optimal.
 *  2) FBE_DOWNSTREAM_HEALTH_DEGRADED:Communication with downstream
 *                                    object is degraded.
 *  3) FBE_DOWNSTREAM_HEALTH_BROKEN:  Communication with downstream
 *                                    object is broken.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_base_config_downstream_health_state_t
fbe_striper_verify_downstream_health (fbe_striper_t * striper_p)
{
    fbe_base_config_downstream_health_state_t downstream_health_state;
    fbe_base_config_path_state_counters_t path_state_counters;
    fbe_u32_t width;

    fbe_base_object_trace((fbe_base_object_t*)striper_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_base_config_get_width((fbe_base_config_t *)striper_p, &width);
    fbe_base_config_get_path_state_counters((fbe_base_config_t *) striper_p, &path_state_counters);

    if(path_state_counters.enabled_counter == width){ 
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_OPTIMAL;
    } else if(path_state_counters.broken_counter > 0){ /* The entire thing is broken */
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
    } else if(path_state_counters.disabled_counter > 0){ /* For striper it is enough to have one disabled edge */
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_DISABLED;
    } else {
        downstream_health_state = FBE_DOWNSTREAM_HEALTH_BROKEN;
    }

    return downstream_health_state;
}
/**********************************************
 * end fbe_striper_verify_downstream_health()
 **********************************************/


/*!**************************************************************
 * fbe_striper_set_condition_based_on_downstream_health()
 ****************************************************************
 * @brief
 *  This function will set the different condition based on the health of the 
 * communication path with its downstream obhects.
 *
 * @param fbe_striper_t - striper object.
 * @param downstream_health_state - Health state of the dowstream objects
 *                                                     communication path.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t
fbe_striper_set_condition_based_on_downstream_health (fbe_striper_t *striper_p, 
                                                      fbe_base_config_downstream_health_state_t downstream_health_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
        case FBE_DOWNSTREAM_HEALTH_DEGRADED:
            break;
        case FBE_DOWNSTREAM_HEALTH_DISABLED:

            fbe_striper_main_drop_stripe_locks_and_monitor_ops(striper_p);

            status = fbe_lifecycle_set_cond(&fbe_striper_lifecycle_const,
                                            (fbe_base_object_t*)striper_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:

            fbe_striper_main_drop_stripe_locks_and_monitor_ops(striper_p);
            /* Downstream health broken condition will be set.
             */
            status = fbe_lifecycle_set_cond(&fbe_striper_lifecycle_const,
                                            (fbe_base_object_t*)striper_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)striper_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid downstream health state:: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);

            break;
    }

    return status;
}
/**************************************************************
 * end fbe_striper_set_condition_based_on_downstream_health()
 **************************************************************/


/*!****************************************************************************
 * fbe_striper_main_handle_sparing_request_event()
 ******************************************************************************
 * @brief
 *   This function will handle a sparing request event.  This event is sent by
 *   the VD when it wants to swap in a Hot Spare or Proactive Spare.   The 
 *   function always deny the request since sparing is not supported on Striper
 *   raid groups. 
 *
 * @param in_striper_p          - pointer to a striper object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/18/2009 - Created. JMM
 *
 ******************************************************************************/
static fbe_status_t fbe_striper_main_handle_sparing_request_event(
                                                fbe_striper_t*          in_striper_p,
                                                fbe_event_context_t     in_event_context)
{

    fbe_event_t*                                event_p;                //  pointer to the event


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_striper_p);

    //  Set a pointer to the event, which is passed to us as "event context" 
    event_p = (fbe_event_t*) in_event_context;  

    //  The striper class will never allow a spare to swap in.  It does not have redundancy, so it can not Hot 
    //  Spare, and it can not Proactive Spare either. 
    fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);

    //  Complete the event 
    fbe_event_set_status (event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_striper_main_handle_sparing_request_event()

/*!****************************************************************************
 * fbe_striper_main_handle_copy_request_event()
 ******************************************************************************
 * @brief
 *   This function will handle a copy request event.  This event is sent by
 *   the VD when it wants to swap in drive to start a copy operation.   The 
 *   function always deny the request since copy is not allowed due to the
 *   additional risk of data loss.
 *
 * @param in_striper_p          - pointer to a striper object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/18/2009 - Created. JMM
 *
 ******************************************************************************/
static fbe_status_t fbe_striper_main_handle_copy_request_event(
                                                fbe_striper_t*          in_striper_p,
                                                fbe_event_context_t     in_event_context)
{

    fbe_event_t*                                event_p;                //  pointer to the event


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_striper_p);

    //  Set a pointer to the event, which is passed to us as "event context" 
    event_p = (fbe_event_t*) in_event_context;  

    // The striper class will never allow a copy operation due to the fact that
    // a copy operation increases the risk of data loss.
    fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);

    //  Complete the event 
    fbe_event_set_status (event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_striper_main_handle_copy_request_event()

/*!****************************************************************************
 * fbe_striper_main_handle_abort_copy_request_event()
 ******************************************************************************
 * @brief
 *   This function will handle a copy request event.  This event is sent by
 *   the VD when it wants to swap in drive to start a copy operation.   The 
 *   function always deny the request since copy is not allowed due to the
 *   additional risk of data loss.
 *
 * @param in_striper_p          - pointer to a striper object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/18/2009 - Created. JMM
 *
 ******************************************************************************/
static fbe_status_t fbe_striper_main_handle_abort_copy_request_event(
                                                fbe_striper_t*          in_striper_p,
                                                fbe_event_context_t     in_event_context)
{

    fbe_event_t*                                event_p;                //  pointer to the event


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_striper_p);

    //  Set a pointer to the event, which is passed to us as "event context" 
    event_p = (fbe_event_t*) in_event_context;  

    // The striper class will never allow a copy operation due to the fact that
    // a copy operation increases the risk of data loss.
    fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);

    //  Complete the event 
    fbe_event_set_status (event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_striper_main_handle_abort_copy_request_event()

/*!****************************************************************************
 * fbe_striper_main_handle_permit_request_event()
 ******************************************************************************
 * @brief
 *  This function will handle the permit request event associated with striper
 *  objects, if it does not handle any permit event then it will return the
 *  traverse status which allows caller to send request to its base class.
 * 
 * @param in_striper_p           - pointer to a striper object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   10/29/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_main_handle_permit_request_event(fbe_striper_t * striper_p,
                                             fbe_event_context_t event_context)
{
    fbe_event_t *               event_p = NULL;
    fbe_status_t                status;
    fbe_event_permit_request_t  permit_request; 
    fbe_permit_event_type_t     permit_event_type;

    /* ge the event data to find the permiy request type. */
    event_p  =(fbe_event_t *) event_context;
    fbe_event_get_permit_request_data(event_p, &permit_request);
    permit_event_type = permit_request.permit_event_type;

    switch (permit_event_type)
    {        
        case FBE_PERMIT_EVENT_TYPE_IS_CONSUMED_REQUEST:
        default:
            /* if striper does not handle the event then send it its base (raid) class. */
            status = FBE_STATUS_TRAVERSE;
            break;
    }

    return status;
} // End fbe_striper_main_handle_permit_request_event()

/*!****************************************************************************
 * fbe_striper_main_drop_stripe_locks_and_monitor_ops()
 ******************************************************************************
 * @brief
 *   This function will abort the stripe locks and the monitor operations.
 *
 * @param in_striper_p           - pointer to a striper object
 *
 * @return fbe_status_t   
 *
 * @author
 *   04/26/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_main_drop_stripe_locks_and_monitor_ops(fbe_striper_t * striper_p)
{
    if (fbe_raid_group_is_flag_set((fbe_raid_group_t*)striper_p, FBE_RAID_GROUP_FLAG_INITIALIZED))
    {
        //  The edge went away for the first time.  We need to abort any outstanding stripe locks that the monitor is 
        //  waiting for.  Otherwise the monitor might never get a chance to run.
        fbe_base_object_trace((fbe_base_object_t*) striper_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
            "%s: aborting MD for MD element 0x%p\n", __FUNCTION__,
            &((fbe_raid_group_t*)striper_p)->base_config.metadata_element);
        fbe_metadata_stripe_lock_element_abort(&((fbe_raid_group_t*)striper_p)->base_config.metadata_element);
    
        /* Monitor operations might be waiting for memory.  Make sure these get aborted.
         */
        fbe_base_config_abort_monitor_ops((fbe_base_config_t*)striper_p);

        /* Abort paged to allow any monitor operations to get kick started. 
         * Otherwise the monitor might never run again if we're stuck behind a paged operation that 
         * saw a drive die and waited. 
         */
        fbe_metadata_element_abort_paged(&striper_p->raid_group.base_config.metadata_element);
    }

    return FBE_STATUS_OK;
} // End fbe_striper_main_drop_stripe_locks_and_monitor_ops

/*!**************************************************************
 * fbe_striper_attribute_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the striper handler for a change in the attribute
 *  of the edge with downstream objects.
 *
 * @param striper_p - Striper RAID type that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  8/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_striper_attribute_change_event_entry(fbe_striper_t * const striper_p,
                                         fbe_event_context_t event_context)
{
    fbe_block_edge_t * edge_p = NULL;
    fbe_bool_t         is_degraded = FBE_FALSE;

    /* Process edge attribute changes for firmware download */
    edge_p = (fbe_block_edge_t *)event_context;

    if(edge_p != NULL)
    {
        if (fbe_block_transport_is_download_attr(edge_p) ||
            fbe_raid_group_is_in_download((fbe_raid_group_t*)striper_p)) 
        {
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) striper_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST);
        }

        if(((fbe_base_edge_t *)edge_p)->path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
        {
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) striper_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_KEYS_REQUESTED);
        }

        /* Check for degraded raid group */
        is_degraded = fbe_raid_group_is_degraded((fbe_raid_group_t*)striper_p);

        /* Propagate degraded attribute up to LUN for Raid 10 */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                        &((fbe_base_config_t *)striper_p)->block_transport_server, 
                        (is_degraded ? FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED : 0), 
                        FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);
    }

    fbe_raid_group_attribute_changed((fbe_raid_group_t*)striper_p, event_context);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_striper_attribute_change_event_entry()
 *****************************************/

/*!**************************************************************
 * fbe_striper_main_handle_event_log_request()
 ****************************************************************
 * @brief
 *  This function sends the Event Log request to the LUN level.
 *
 * @param in_striper_p           - pointer to a striper object
 * @param in_event_context       - event context  
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/30/2011 - Created Vishnu Sharma
 *
 ****************************************************************/
static fbe_status_t fbe_striper_main_handle_event_log_request(fbe_striper_t * in_striper_p, fbe_event_context_t in_event_context)
{
    fbe_event_t*          event_p;
    fbe_status_t          status;
    fbe_event_stack_t*    event_stack_p;          //  event stack pointer
    fbe_lba_t             start_lba;              //  start LBA (a RAID-based LBA)
    fbe_block_count_t     unused_block_count;     //  block count (unused) 
    fbe_lba_t             exported_disk_capacity; //  capacity of the RG available for data 

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_striper_p);

    // Set a pointer to the event, which is passed to us as "event context"
    event_p = (fbe_event_t*) in_event_context;  

    /* allocate the new event stack with raid based lba and block count. */
    status = fbe_raid_group_allocate_event_stack_with_raid_lba((fbe_raid_group_t*)in_striper_p, in_event_context);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_striper_p,
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s event stack allocation failed, status:%d, context:0x%p\n",
            __FUNCTION__, status, in_event_context);
    }

    //  Get the start lba and block count out of the event 
    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &unused_block_count);

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*) in_striper_p, &exported_disk_capacity);

    //  If the LBA range of the event is greater than the exported capacity, then the event does not 
    //  need to be send upstream, because we know that it is not seen by an upstream object.  Complete 
    //  the event with status "okay" because it is consumed by this object. 
    if (start_lba >= exported_disk_capacity) {
        fbe_event_set_status(event_p, FBE_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    //  Send the event to the upstream object for it to process it further 
    fbe_base_config_send_event((fbe_base_config_t*) in_striper_p, FBE_EVENT_TYPE_EVENT_LOG, event_p);

    return FBE_STATUS_OK;
} // end of  fbe_striper_main_handle_event_log_request()



/*************************
 * end file fbe_striper_event.c
 *************************/


