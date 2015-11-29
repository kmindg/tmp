/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_common.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the raid common object.
 *
 * @version
 *   5/15/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_raid_common.h"
#include "fbe_raid_library_proto.h"

/*!**************************************************************
 * fbe_raid_common_init()
 ****************************************************************
 * @brief
 *  Initialize the common raid object.
 *
 * @param  common_p - The common raid object to initialize               
 *
 * @return None.
 *
 * @author
 *  5/15/2009 - Created. rfoley
 *
 ****************************************************************/
void fbe_raid_common_init(fbe_raid_common_t *common_p)
{
    common_p->flags = 0;
    fbe_queue_element_init(&common_p->queue_element);
    fbe_queue_element_init(&common_p->wait_queue_element);
    common_p->magic_number = FBE_MAGIC_NUMBER_INVALID;
    common_p->parent_p = NULL;
    common_p->queue_p = NULL;
    common_p->state = NULL;
    return;
}
/******************************************
 * end fbe_raid_common_init()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_common_restart_memory_request()
 *****************************************************************************
 *
 * @brief   Determine the ts type and invoke the proper method to restart a
 *          memory request.
 *
 * @param   common_p - Pointer to common to determine request to restart
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_common_restart_memory_request(fbe_raid_common_t *common_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_common_flags_t common_flags = fbe_raid_common_get_flags(common_p);

    /* First clear any flags associated with the previous memory request
     */
    fbe_raid_common_clear_flag(common_p, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE);
    fbe_raid_common_clear_flag(common_p, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION);
    fbe_raid_common_clear_flag(common_p, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED);
    fbe_raid_common_clear_flag(common_p, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR);

    /* Determine the ts type so that we can invoke the proper method
     */
    common_flags &= FBE_RAID_COMMON_FLAG_STRUCT_TYPE_MASK;
    switch(common_flags)
    {
        case FBE_RAID_COMMON_FLAG_TYPE_IOTS:
            status = fbe_raid_iots_restart_allocate_siots((fbe_raid_iots_t *)common_p);
            break;

        case FBE_RAID_COMMON_FLAG_TYPE_SIOTS:
        case FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS:
            status = fbe_raid_siots_restart_allocate_memory((fbe_raid_siots_t *)common_p);
            break;

        default:
            /* Unsupported structure type
             */
            fbe_raid_library_trace(FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                   "raid: common_p: 0x%p failed to restart flags: 0x%x invalid type: 0x%x\n",
                                   common_p, fbe_raid_common_get_flags(common_p), common_flags);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    
    /* Return the execution status
     */
    return status;
}
/*************************************************
 * end of fbe_raid_common_restart_memory_request()
 *************************************************/

/*!**************************************************************
 * fbe_raid_restart_common_queue()
 ****************************************************************
 * @brief
 *  Execute raid group state on a given queue of raid common objects.
 *
 * @param  raid_group_p - raid group to execute on.
 * @param  queue_p - queue contains a list of raid common objects.
 *
 * @return fbe_u32_t count of objects restarted.   
 *
 * @author
 *  10/30/2009 - Created. Rob Foley
 *  09/28/2012 - Modified. Arun S - To run the iots/siots on the
 *                  same core when restarted.. 
 *
 ****************************************************************/
fbe_u32_t fbe_raid_restart_common_queue(fbe_queue_head_t *queue_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       restart_count = 0;
    fbe_raid_iots_t *iots_p = NULL;

    while (!fbe_queue_is_empty(queue_p))
    {
        fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;
        fbe_raid_common_t *common_p = NULL;

        common_p = fbe_raid_common_wait_queue_pop(queue_p);

        /* If the memory request was aborted we need to re-start the allocation */
        if (fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED) &&
            !fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_RESTART))
        {
            /* Invoke method that restarts the memory allocation sequence */
            status = fbe_raid_common_restart_memory_request(common_p);
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                /* Trace out the error and goto next item */
                fbe_raid_library_trace(FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                       "raid: common_p: 0x%p failed to restart flags: 0x%x status: 0x%x \n",
                                       common_p, fbe_raid_common_get_flags(common_p), status);
                continue;
            }
        }
        else
        {
            fbe_raid_common_clear_flag(common_p, FBE_RAID_COMMON_FLAG_RESTART);

            /* Else restart state machine for this ts */
            if (fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_TYPE_SIOTS) ||
                fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS))
            {
                fbe_raid_siots_t *siots_p = (fbe_raid_siots_t *)common_p;
                iots_p = fbe_raid_siots_get_iots(siots_p);
            }
            else if(fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_TYPE_IOTS))
            {
                iots_p = (fbe_raid_iots_t *)common_p;
            }

            if (iots_p == NULL)
            {
                fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "raid: iots is NULL. Flags: 0x%x, state: %x",
                                     common_p->flags, (unsigned int)common_p->state);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            fbe_raid_iots_get_cpu_id(iots_p, &cpu_id);

            fbe_transport_record_callback_with_action(fbe_raid_iots_get_packet(iots_p), 
                                                      (fbe_packet_completion_function_t) fbe_raid_restart_common_queue, 
                                                      PACKET_RUN_QUEUE_PUSH);

            fbe_transport_run_queue_push_raid_request(&common_p->wait_queue_element, cpu_id);
        }

        restart_count++;
    }
    return restart_count;
}
/******************************************
 * end fbe_raid_restart_common_queue()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_common_set_memory_allocation_error()
 *****************************************************************************
 *
 * @brief   This method simply flags the associated ts as having a memory
 *          allocation error.  If the flag is already set it doesn't set it 
 *          again.
 *
 * @param   common_p - Pointer to common to flag in error
 * 
 * @note    Basically an unexpected allocation error occurred.  We mark the
 *          siots in error and let the state machine transition to the proper
 *          state.
 *
 * @return  None
 * 
 *****************************************************************************/
void fbe_raid_common_set_memory_allocation_error(fbe_raid_common_t *common_p) 
{
     /* Only set the flag an increment the allocation error counter if it has
     * not been reported yet.  This method can be invoked multiple times for 
     * the same iots. 
     */
    if (!fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR))
    {
        /* Set the flag and increment the associated counter
         */
        fbe_raid_common_set_flag(common_p, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR);
        fbe_raid_memory_api_allocation_error();
    }

    return;
}
/* end of fbe_raid_common_set_memory_allocation_error() */

/*!***************************************************************************
 *          fbe_raid_common_set_memory_free_error()
 *****************************************************************************
 *
 * @brief   This method simply flags the associated ts as having a memory
 *          free error.  If the flag is already set it doesn't set it 
 *          again.
 *
 * @param   common_p - Pointer to ts to flag in error
 * 
 * @note    Basically an unexpected free error occurred.  We mark the
 *          siots in error and let the state machine transition to the proper
 *          state.
 *
 * @return  None
 * 
 *****************************************************************************/
void fbe_raid_common_set_memory_free_error(fbe_raid_common_t *common_p)
{
     /* Only set the flag an increment the free error counter if it has
     * not been reported yet.  This method can be invoked multiple times for 
     * the same siots. 
     */
    if (!fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_MEMORY_FREE_ERROR))
    {
        /* Set the flag and increment the associated counter
         */
        fbe_raid_common_set_flag(common_p, FBE_RAID_COMMON_FLAG_MEMORY_FREE_ERROR);
        fbe_raid_memory_api_free_error();
    }

    return;
}
/* end of fbe_raid_common_set_memory_free_error() */

/*!***************************************************************************
 *          fbe_raid_common_set_memory_request_fields_from_parent()
 *****************************************************************************
 *
 * @brief   Determine the ts type to determine the parent memory request and
 *          based on the structure type.  Then populate any memory request
 *          fields that come from the parent.  Currently they are:
 *              o Memory request priority
 *              o Memory request I/O stamp (for debugging)
 *
 * @param   common_p - Pointer to common to determine request to restart
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 *
 *****************************************************************************/
fbe_status_t fbe_raid_common_set_memory_request_fields_from_parent(fbe_raid_common_t *common_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_common_flags_t common_flags = fbe_raid_common_get_flags(common_p);
    fbe_raid_iots_t    *iots_p = (fbe_raid_iots_t *)common_p;
    fbe_raid_siots_t   *parent_siots_p = NULL;
    fbe_packet_resource_priority_t resource_priority = 0;
    fbe_memory_request_priority_t memory_request_priority = 0;
    fbe_packet_io_stamp_t io_stamp = FBE_PACKET_IO_STAMP_INVALID;

    /* Determine the ts type so that we can invoke the proper method
     */
    common_flags &= FBE_RAID_COMMON_FLAG_STRUCT_TYPE_MASK;
    switch(common_flags)
    {
        case FBE_RAID_COMMON_FLAG_TYPE_IOTS:
            /* Give packet resource priority a boost to level where MDS 
             * requests need to be made.  
             */
            fbe_raid_iots_adjust_packet_resource_priority(iots_p);

            fbe_raid_iots_get_packet_resource_priority(iots_p, 
                                                       &resource_priority);

            /* Relative resource priorites need to be as follows:
             * Iots = 0
             * Siots = 1
             * nSiots = 2
             * MDS = 3 (i.e. FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_BOOST)
             * Packet's reosurce priroity is set to level of MDS; so adjust
             * Iots priority level down.  
             */
            memory_request_priority = (fbe_memory_request_priority_t)resource_priority;// - FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_BOOST;

            fbe_raid_iots_set_memory_request_priority(iots_p, 
                                                      memory_request_priority);
            /* Set the I/O stamp to that of the packet
             */
            fbe_raid_iots_get_packet_io_stamp(iots_p, &io_stamp);
            fbe_raid_iots_set_memory_request_io_stamp(iots_p, io_stamp);
            break;

        case FBE_RAID_COMMON_FLAG_TYPE_SIOTS:
            iots_p = fbe_raid_siots_get_iots((fbe_raid_siots_t *)common_p);
            fbe_raid_iots_get_memory_request_priority(iots_p,
                                                      &memory_request_priority);
            fbe_raid_siots_set_memory_request_priority((fbe_raid_siots_t *)common_p,
                                                       memory_request_priority);
            /* Now bump the resource priority
             */
            fbe_raid_memory_request_increment_priority(fbe_raid_siots_get_memory_request((fbe_raid_siots_t *)common_p));
            /* Set the io stamp from the parent iots
             */
            fbe_raid_iots_get_memory_request_io_stamp(iots_p, &io_stamp);
            fbe_raid_siots_set_memory_request_io_stamp((fbe_raid_siots_t *)common_p, io_stamp);
            break;

        case FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS:
            parent_siots_p = fbe_raid_siots_get_parent((fbe_raid_siots_t *)common_p);
            fbe_raid_siots_get_memory_request_priority(parent_siots_p,
                                                      &memory_request_priority);
            fbe_raid_siots_set_memory_request_priority((fbe_raid_siots_t *)common_p,
                                                       memory_request_priority);
            /* Now bump the resource priority
             */
            fbe_raid_memory_request_increment_priority(fbe_raid_siots_get_memory_request((fbe_raid_siots_t *)common_p));
            /* Set the io stamp from the parent siots
             */
            fbe_raid_siots_get_memory_request_io_stamp(parent_siots_p, &io_stamp);
            fbe_raid_siots_set_memory_request_io_stamp((fbe_raid_siots_t *)common_p, io_stamp);
            break;

        case FBE_RAID_COMMON_FLAG_TYPE_FRU_TS:
            parent_siots_p = fbe_raid_fruts_get_siots((fbe_raid_fruts_t *)common_p);

            /* Get the parent priority and bump it since all downstream memory requests
             * should have a higher priority than the upstream request.
            */
            fbe_raid_siots_get_memory_request_priority(parent_siots_p,
                                                       &memory_request_priority);
            fbe_raid_fruts_set_memory_request_priority((fbe_raid_fruts_t *)common_p, 
                                                       memory_request_priority);
            fbe_raid_memory_request_increment_priority(fbe_raid_fruts_get_memory_request((fbe_raid_fruts_t *)common_p));

            /* Now set the packet resource priority from the memory request
             * priority.
             */
            fbe_raid_fruts_get_memory_request_priority((fbe_raid_fruts_t *)common_p, 
                                                       &memory_request_priority);
            fbe_transport_set_resource_priority(&((fbe_raid_fruts_t *)common_p)->io_packet,
                                                (fbe_packet_resource_priority_t)memory_request_priority);
            /* Set the io stamp from the parent siots
             */
            fbe_raid_siots_get_memory_request_io_stamp(parent_siots_p, &io_stamp);
            fbe_raid_fruts_set_memory_request_io_stamp((fbe_raid_fruts_t *)common_p, io_stamp);
            fbe_transport_set_io_stamp(&((fbe_raid_fruts_t *)common_p)->io_packet,
                                       (fbe_packet_io_stamp_t)io_stamp);
            break;

        default:
            /* Unsupported structure type
             */
            fbe_raid_library_trace(FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                   "raid: common_p: 0x%p failed to set resource priority flags: 0x%x invalid type: 0x%x\n",
                                   common_p, fbe_raid_common_get_flags(common_p), common_flags);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    
    /* Return the execution status
     */
    return status;
}
/****************************************************************
 * end of fbe_raid_common_set_memory_request_fields_from_parent()
 ****************************************************************/

/*!****************************************************************************
 * fbe_raid_common_set_resource_priority()
 ******************************************************************************
 * @brief
 * This function sets memory allocation priority for the packet.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/20/2011 - Created. Vamsi V
 *
 ******************************************************************************/
fbe_status_t  
fbe_raid_common_set_resource_priority(fbe_packet_t * packet_p, fbe_raid_geometry_t * geo)
{
    if((packet_p->resource_priority_boost & FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_RG) == 
       FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_RG)
    {
        /* Flag is set */
        /* Packet is being resent/reused without resetting the resource priroity. */ 
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                               "raid: resource priority is not reset for packet 0x%p raid_type %d \n",
                               packet_p, geo->raid_type);
    }
    
    if (geo->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        if (packet_p->resource_priority >= FBE_MEMORY_DPS_STRIPER_ABOVE_MIRROR_BASE_PRIORITY)
        {
            /* Generally in IO path packets flow down the stack from LUN to RG to VD to PVD.
             * In this case, resource priority will be less than Objects base priority and so
             * we bump it up. 
             * But in some cases (such as monitor, control, event operations, etc.) packets can some 
             * time move up the stack. In these cases packets's resource priority will already be greater
             * than Object's base priority. But since we should never reduce priority, we dont
             * update it.    
             */
        }
        else
        {
            packet_p->resource_priority = FBE_MEMORY_DPS_STRIPER_ABOVE_MIRROR_BASE_PRIORITY;
        }       
    }
    else
    {
        if (packet_p->resource_priority >= FBE_MEMORY_DPS_RAID_BASE_PRIORITY)
        {
            /* Generally in IO path packets flow down the stack from LUN to RG to VD to PVD.
             * In this case, resource priority will be less than Objects base priority and so
             * we bump it up. 
             * But in some cases (such as monitor, control, event operations, etc.) packets can some 
             * time move up the stack. In these cases packets's resource priority will already be greater
             * than Object's base priority. But since we should never reduce priority, we dont
             * update it.    
             */
        }
        else
        {
            packet_p->resource_priority = FBE_MEMORY_DPS_RAID_BASE_PRIORITY;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_common_set_resource_priority()
 ******************************************************************************/

/*************************
 * end file fbe_raid_common.c
 *************************/
