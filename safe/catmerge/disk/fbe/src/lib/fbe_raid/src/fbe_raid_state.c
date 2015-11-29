 /***************************************************************************
  * Copyright (C) EMC Corporation 2009
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!*************************************************************************
  * @file fbe_raid_state.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the raid state machine framework.
  *
  * @version
  *   5/13/2009:  Created. rfoley
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe_raid_library_proto.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/****************************************************************
 * fbe_raid_common_state()
 ****************************************************************
 * @brief
 *  This is the entry point for raid group state machines.
 *
 * @param common_request_p - request to start executing
 *
 * @return
 *  fbe_raid_state_status_t RAID_STATE_WAITING or RAID_STATE_DONE
 *
 ****************************************************************/

fbe_status_t fbe_raid_common_state(fbe_raid_common_t * common_request_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (common_request_p == NULL)
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "raid: common_request_p is null\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ( RAID_DBG_ENA(!(common_request_p->flags & (FBE_RAID_COMMON_FLAG_TYPE_IOTS       | 
                                                   FBE_RAID_COMMON_FLAG_TYPE_SIOTS      | 
                                                   FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS |
                                                   FBE_RAID_COMMON_FLAG_TYPE_FRU_TS       ))) )
    {
        fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "raid: common_flags unexpected 0x%x 0x%p\n",
                             common_request_p->flags, common_request_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_ENA(common_request_p->flags & (FBE_RAID_COMMON_FLAG_TYPE_SIOTS | 
                                                FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS)))
    {
        fbe_raid_siots_t * siots_p = (fbe_raid_siots_t *)common_request_p;
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        if (iots_p == NULL)
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, "raid: iots is null for siots %p common flags: 0x%x state: %p",
                                 common_request_p, common_request_p->flags, common_request_p->state);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (fbe_queue_is_empty(&iots_p->siots_queue))
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS, 
                                 "raid: iots %p siots queue is empty for siots %p common flags: 0x%x state: %p",
                                 iots_p, common_request_p, common_request_p->flags, common_request_p->state);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    status = fbe_raid_common_start(common_request_p);

    return status;
}/* END fbe_raid_common_state() */

/****************************************************************
 * fbe_raid_siots_start_nested_siots()
 ****************************************************************
 * @brief
 *  Allocate and start execution of a nested siots.
 *
 * @param siots_p - ptr to the parent siots
 * @param first_state - first state for this nested siots.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_start_nested_siots(fbe_raid_siots_t * siots_p,
                                               void *(first_state))
{
    fbe_raid_siots_t   *new_siots_p = NULL;
    fbe_u32_t           nested_queue_length = 0;
    fbe_status_t        status = FBE_STATUS_OK;

    /* In the raid driver we only one level of SIOTS nesting.
     * We can only support this many since we only have
     * three levels of priorities for allocating buffers.
     *
     * IOTS               priority = 0
     *  \
     *   SIOTS            priority = 1
     *    \
     *    Nested SIOTS    priority = 2
     *
     * Check this is not a nested siots, since we are 
     * guaranteed to run into further issues with 3 levels of nesting.
     */   
    if ( fbe_raid_common_is_flag_set((fbe_raid_common_t*)siots_p, 
                                     FBE_RAID_COMMON_FLAG_SIOTS_NEST_MASK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: start nested siots_p: 0x%p nested flag set in parent. flags: 0x%x\n",
                             siots_p, fbe_raid_siots_get_flags(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the previously allocated nested siots
     */
    new_siots_p = fbe_raid_siots_get_nested_siots(siots_p);
    if (RAID_COND(new_siots_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: start nested siots_p: 0x%p nested siots ptr is NULL flags: 0x%x\n",
                             siots_p, fbe_raid_siots_get_flags(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that there is nothing on the queue
     */
    nested_queue_length = fbe_queue_length(&siots_p->nsiots_q);
    if (RAID_COND(nested_queue_length != 0))
    {
        fbe_raid_siots_t   *already_nested_siots_p;

        fbe_raid_siots_get_nest_queue_head(siots_p, &already_nested_siots_p);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: start nested: %d nested siots head: 0x%p unexpected\n",
                             nested_queue_length, already_nested_siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Successfully allocated the siots.  No longer a need to lock the iots
     * since the resource has already been allocated.
     */
    status = fbe_raid_nest_siots_init(new_siots_p, siots_p);
    if(RAID_MEMORY_COND(status != FBE_STATUS_OK)) 
    {
        /* If we fail to initialize the nested siots we log an error, but
         * there is no need to free memory now since it was allocated with 
         * the parent siots memory allocation. 
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: unexpected error, initialization of new_siots_p = 0x%p failed. "
                             "original siots_p = 0x%p, status 0x%x\n",
                             new_siots_p, siots_p, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Add nested siots to parent's nested queue.  Transition it to the
     * specified state and start it.
     */
    fbe_raid_common_enqueue_tail(&siots_p->nsiots_q, (fbe_raid_common_t *)new_siots_p);
    fbe_raid_siots_transition(new_siots_p, first_state);
    fbe_raid_common_state((fbe_raid_common_t *)new_siots_p);
    
    return status;
}
/*******************************
 * end of fbe_raid_siots_start_nested_siots()
 *******************************/

/*************************
  * end file raid_state.c
  *************************/
 
 
