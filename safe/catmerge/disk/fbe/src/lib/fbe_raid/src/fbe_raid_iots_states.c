/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_iots_states.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions related to the raid iots structure.
 *
 * @version
 *   5/14/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_queue.h"
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"
#include "fbe_raid_library.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_raid_state_status_t fbe_raid_iots_freed(fbe_raid_iots_t *iots_p);

/*!**************************************************************
 * fbe_raid_iots_freed()
 ****************************************************************
 * @brief
 *  When we free the iots, we put it into this state so
 *  that it cannot be executed again.
 *
 * @param iots_p - current I/O.               
 *
 * @return FBE_RAID_STATE_STATUS_DONE
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_iots_freed(fbe_raid_iots_t *iots_p)
{
    /* Panic - Something is wrong in freeing iots. CRITICAL ERROR 
     * takes care of panic. 
     */
    fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                             FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                             "raid: %s iots is freed 0x%p\n", 
                             __FUNCTION__, iots_p);

    /* return is meaningless here since we are taking a PANIC. 
     * Disable or Remove this return when the PANIC is enabled in
     * trace_to_ring().
     */
    return FBE_RAID_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_raid_iots_freed()
 ******************************************/

/*!**************************************************************
 *          fbe_raid_iots_memory_allocation_error()
 ****************************************************************
 * @brief
 *  The memory service returned an error to an allocation request
 *
 * @param  iots_p - current I/O.
 *
 * @return None.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_iots_memory_allocation_error(fbe_raid_iots_t *iots_p)
{
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    /*! @note There might be other siots outstanding.  Therefore don't
     *        transition iots to another state.
     */
    fbe_raid_common_set_memory_allocation_error(&iots_p->common);

    /* We panic in checked builds since we want to catch these errors. 
     * In free builds we will allow the system to continue without panicking. 
     */
    fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                               "raid: memory allocation error for iots: 0x%p\n",
                               iots_p);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**********************************************
 * end fbe_raid_iots_memory_allocation_error()
 **********************************************/

/****************************************************************
 * fbe_raid_iots_generate_siots()
 ****************************************************************
 *  DESCRIPTION:
 *    This is the entry state for new sub requests.
 *    First try to get a SUB_IOTS for this driver.
 *
 *  PARAMETERS:
 *    iots_ptr, [I] - ptr to new request.
 *
 *  RETURN VALUE:
 *    FBE_RAID_STATE_STATUS_WAITING
 *
 *  HISTORY:
 *    7/13/99 - Created. RPF
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_iots_generate_siots(fbe_raid_iots_t * iots_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_memory_request_t       *memory_request_p = NULL;
    fbe_cpu_id_t cpu_id;

    fbe_raid_iots_get_cpu_id(iots_p, &cpu_id);

    /* We should not already be allocating.
     */
    fbe_raid_iots_lock(iots_p);

    if (RAID_GENERATE_COND(!fbe_raid_iots_is_any_flag_set(iots_p, (FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS|FBE_RAID_IOTS_FLAG_GENERATING_SIOTS))))
    {
        /* It is very-unexpected event. Just trace the error as a 
         * critical error. Also, release the acquired lock so that others
         * are not halted because of us.
         */       
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL, 
                                   "status: raid: iots flag 0x%x  not set to FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS or FBE_RAID_IOTS_FLAG_GENERATING_SIOTS\n",
                                   iots_p->flags);
        fbe_raid_iots_unlock(iots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* If we are aborted, just reinit the memory request and clean up the flags.
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED))
    {
        memory_request_p = fbe_raid_iots_get_memory_request(iots_p);
        if (memory_request_p!=NULL)
        {
            fbe_memory_request_init(memory_request_p);
        }
        fbe_raid_common_clear_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED);
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_INFO, 
                                   "status: iots_p: 0x%p iots flag 0x%x  memory allocation aborted, reinit memory request 0x%p\n",
                                   iots_p, iots_p->flags, memory_request_p);
    }

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS))
    {
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
    }

    /* Check if we can use an embedded or if we need to allocate from a pool.
     */
    if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE))
    {
        /* Find the embedded siots.
         */
        siots_p = fbe_raid_iots_get_embedded_siots(iots_p);
        if(RAID_GENERATE_COND(siots_p == NULL))
        {
            /* Embedded siots is not expected to be null and is 
             * indication of critical failure. Just clean-up
             * the current iots.
             */
            fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
            fbe_raid_iots_unlock(iots_p);
            fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                               "raid : %s failed to get embedded siots which is NULL: iots_p = 0x%p\n",
                                               __FUNCTION__,
                                               iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Initialize the embedded siots
         */
        status = fbe_raid_siots_embedded_init(siots_p, iots_p);
        if(RAID_GENERATE_COND(status != FBE_STATUS_OK)) 
        {
            /* We have not kicked off siots state machine. So, we can do 
             * clean up at iots level only.
             */
            fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
            fbe_raid_iots_unlock(iots_p);
            fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,  
                                               "raid: failed to initialize embedded siots 0x%p of "
                                               "iots 0x%p, status = 0x%x\n",
                                               siots_p,
                                               iots_p,
                                               status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        fbe_raid_common_enqueue_tail(&iots_p->siots_queue, &siots_p->common);
        iots_p->outstanding_requests++;
        fbe_raid_iots_unlock(iots_p);

        /* Initialize and kick off the siots.
         */
        fbe_raid_common_state((fbe_raid_common_t *)siots_p);
    }
    else if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR))
    {
        /*! It is very-unexpected event. Just trace the error as a 
         *  critical error. Also, release the acquired lock so that others
         *  are not halted because of us.
         */
        fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                           "raid: iots_p: 0x%p generate siots attempt to generate for a failed allocaiton\n",
                                           iots_p);
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
        fbe_raid_iots_unlock(iots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE))
    {
        /* The deferred allocation completed.  Process the allocated memory.
         */
        status = fbe_raid_iots_process_allocated_siots(iots_p, &siots_p);
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            /* Trace some information, flag the error and continue.
             */
            fbe_raid_iots_memory_allocation_error(iots_p);
            fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                "raid: iots_p: 0x%p process allocated siots failed with status: 0x%x\n",
                                iots_p, status);
            fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* Else release the iots lock and activate the siots*/
            fbe_raid_iots_unlock(iots_p);
            /* Push the siots onto the run queue instead of recursing into common state
             * since we now split into many siots in degraded write mode */
            fbe_transport_run_queue_push_raid_request(&((fbe_raid_common_t *)siots_p)->wait_queue_element, cpu_id);
        }
    }
    else
    {
        /* Else we need to allocate resources
         */
        status = fbe_raid_iots_request_siots_allocation(iots_p);
        if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
        {
            /* The memory was immediately available.  Now process the allocated
             * siots.
             */
            status = fbe_raid_iots_process_allocated_siots(iots_p, &siots_p);
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                /* Trace some information, flag the error and continue.
                 */
                fbe_raid_iots_memory_allocation_error(iots_p);
                fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                          "raid: iots_p: 0x%p process allocated siots failed with status: 0x%x\n",
                                          iots_p, status);
                fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
                fbe_raid_iots_unlock(iots_p);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            else
            {
                /* Else release the iots lock and activate the siots*/
                fbe_raid_iots_unlock(iots_p);
                /* Push the siots onto the run queue instead of recursing into common state
                 * since we now split into many siots in degraded write mode */
                fbe_transport_run_queue_push_raid_request(&((fbe_raid_common_t *)siots_p)->wait_queue_element, cpu_id);
            }
        }
        else if (status == FBE_STATUS_PENDING)
        {
            /* Else we have marked `deferred' allocation, just release the lock
             */
            fbe_raid_iots_unlock(iots_p);
        }
        else
        {
            /* Trace some information, flag the error and continue.
             */
            fbe_raid_iots_memory_allocation_error(iots_p);
            fbe_raid_iots_set_unexpected_error(iots_p, FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                "raid: iots_p: 0x%p generate siots allocation failed with status: 0x%x\n",
                                iots_p, status);
            fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }

    /* Always return waiting since the siots completion will wake us up
     */
    return FBE_RAID_STATE_STATUS_WAITING;
}
/************************************
 * end fbe_raid_iots_generate_siots()
 ************************************/

/*!***************************************************************
 *      fbe_raid_iots_complete_iots()
 ****************************************************************
 * 
 * @brief   We are done with all the siots associated with this
 *          iots.  Populate the parent packet with the iots completion
 *          status and then complete it.
 *
 * @param   iots_pt, [I] - ptr to completed request.
 *
 * @return  FBE_RAID_STATE_STATUS_DONE
 *
 * @note    Must be invoked from state machine so that we clean up
 *
 * @author
 *  05/27/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_iots_complete_iots(fbe_raid_iots_t * iots_p)
{
    fbe_status_t transport_status;
    fbe_raid_iots_completion_function_t callback;
    fbe_raid_iots_completion_context_t context;

    /* We are on the verge of completing iots and we do not whom to
     * report status.
     */
    fbe_raid_iots_get_callback(iots_p, &callback, &context);
    if (RAID_ERROR_COND(callback == NULL))
    {
        fbe_object_id_t object_id;
        fbe_raid_geometry_t *raid_geometry_p = NULL;

        raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
        object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "iots: 0x%p callback is null context: 0x%p status: 0x%x\n",
                                      iots_p, context, iots_p->status);

        return(FBE_RAID_STATE_STATUS_DONE);
    }

    /* We intentionally clear the completion so this iots cannot be completed twice.
     */
    fbe_raid_iots_set_callback(iots_p, NULL, NULL);

    transport_status = (callback)(iots_p->packet_p, context);
    return(FBE_RAID_STATE_STATUS_DONE);
}
/***********************************
 * end fbe_raid_iots_complete_iots()
 ***********************************/

/*!***************************************************************
 *      fbe_raid_iots_callback()
 ****************************************************************
 * 
 * @brief
 *  Call back on this iots leaving the callback ptr intact.
 *
 * @param   iots_pt, [I] - ptr to completed request.
 *
 * @return  FBE_RAID_STATE_STATUS_DONE
 * 
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_iots_callback(fbe_raid_iots_t * iots_p)
{
    fbe_status_t transport_status;
    fbe_raid_iots_completion_function_t callback;
    fbe_raid_iots_completion_context_t context;

    /* We are on the verge of completing iots and we do not whom to
     * report status.
     */
    fbe_raid_iots_get_callback(iots_p, &callback, &context);
    if (RAID_ERROR_COND(callback == NULL))
    {
        fbe_object_id_t object_id;
        fbe_raid_geometry_t *raid_geometry_p = NULL;

        raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
        object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "iots: 0x%p callback is null context: 0x%p status: 0x%x\n",
                                      iots_p, context, iots_p->status);

        return(FBE_RAID_STATE_STATUS_DONE);
    }

    transport_status = (callback)(iots_p->packet_p, context);
    return(FBE_RAID_STATE_STATUS_DONE);
}
/***********************************
 * end fbe_raid_iots_callback()
 ***********************************/

/*!**************************************************************
 * fbe_raid_iots_unexpected_error()
 ****************************************************************
 * @brief
 *  Set an aborted status into the iots.
 *
 * @param  iots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/16/2010 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_iots_unexpected_error(fbe_raid_iots_t *iots_p)
{
    fbe_raid_state_status_t return_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    fbe_raid_iots_transition(iots_p, fbe_raid_iots_complete_iots);

    /* We panic in checked builds since we want to catch these errors. 
     * In free builds we will allow the system to continue without panicking. 
     */
    fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                               "Invalid iots or unexpected error occured with iots : 0x%p\n",
                               iots_p);
    fbe_raid_iots_lock(iots_p);
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS);
    if (iots_p->outstanding_requests != 0)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                     "%s outstanding count is %d, iots %p waiting\n", 
                                     __FUNCTION__, iots_p->outstanding_requests, iots_p);
        return_status = FBE_RAID_STATE_STATUS_WAITING;
    }
    fbe_raid_iots_unlock(iots_p);
    return return_status;
}
/**************************************
 * end fbe_raid_iots_unexpected_error()
 **************************************/


/*********************************
 * end file fbe_raid_iots_states.c
 *********************************/

