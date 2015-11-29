/**************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the R0 verify algorithm.
 *
 * @notes
 *  At initial entry, we assume that the fbe_raid_siots_t has the following fields
 *  initialized:
 *    start_lba       - logical address of first block of new data
 *    xfer_count      - total blocks of new data
 *    data_disks      - tatal data disks affected
 *    start_pos       - relative start disk position
 *              
 *    If a transfer of data from the host is required:
 *       current_sg_position     - copy of sg_desc (host_req_id)
 *       current_sg_position.offset      - updated with offset for this req
 *
 * @author
 *  02/2000 Created. Kevin Schleicher
 *
 ***************************************************************************/


/*************************
 **    INCLUDE FILES    **
 *************************/

#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"


/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
/* static fbe_raid_state_status_t fbe_striper_recovery_verify_state1(fbe_raid_siots_t * siots_p); Not in Use */
static fbe_raid_state_status_t fbe_striper_recovery_verify_state2(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_recovery_verify_state3(fbe_raid_siots_t * siots_p);
/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/

/*!*************************************************************** 
 * fbe_striper_recovery_verify_state0()
 ****************************************************************
 * @brief   We have encountered a media error on an unaligned
 *          RAID group.  We will generate an aligned verify request.
 *          We FIRST check for overlapping requests (with the new 
 *          possibly expanded verify).  Then we upgrade the iots
 *          lock to the possibly expanded range and add a write.
 *
 * @param   siots_p, [IO] - ptr to the fbe_raid_siots_t
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *          to allocate the required number of structures immediately;
 *          otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @note    We FIRST check and wait for overlapping siots to complete
 *          so that we don't wait when we issue the lock upgrade.
 *
 * @author
 *  12/03/2008  Ron Proulx  - Created
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_recovery_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_t *parent_siots_p;
    fbe_status_t status = FBE_STATUS_OK;
    /* The nested siots allocation already set the state to
     * this routine.
     */

    /* Get the parent siots
     */
    parent_siots_p = fbe_raid_siots_get_parent(siots_p);
   
    /* Mark error handling as going on.
     */
    fbe_raid_iots_inc_errors(fbe_raid_siots_get_iots(siots_p));

    /* Invoke the routine that will update the request range to the entire
     * stripe where the error was detected.  We need to verify the entire
     * stripe since verify uses a physical request and each fruts must be of
     * the same size.  For recovery verify will break these requests up into
     * `block' sized sub-requests.
     */
    status = fbe_striper_generate_expanded_request(siots_p);
    if(RAID_COND(status == FBE_STATUS_GENERIC_FAILURE))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: failed to set the request range"
                                            "status %d, siots_p %p\n",
                                            status, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    
    /* Now transition to the next state which will expand the iots
     * lock range.
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_recovery_verify_state2);

    return(FBE_RAID_STATE_STATUS_EXECUTING);
} /* end of fbe_striper_recovery_verify_state0() */

#if 0 /* Not in Use */
/*!***************************************************************
 * fbe_striper_recovery_verify_state1()
 ****************************************************************
 * @brief Upgrade the lock for this siots. 
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return  This function always returns FBE_RAID_STATE_STATUS_DONE.
 *
 * @author
 *  5/10/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_recovery_verify_state1(fbe_raid_siots_t * siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    fbe_raid_siots_transition(siots_p, fbe_striper_recovery_verify_state2);

    /* Try to upgrade the lock.  We will upgrade to be aligned to 
     * the optimal block size. 
     */
    fbe_raid_siots_upgrade_lock(siots_p, raid_geometry_p->optimal_block_size);
    return FBE_RAID_STATE_STATUS_WAITING;
}
/**************************************
 * end fbe_striper_recovery_verify_state1()
 **************************************/
#endif 

/*!***************************************************************
 * fbe_striper_recovery_verify_state2()
 ****************************************************************
 *
 * @brief   Update the IOTS lock associated with this verify
 *          request.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return  This function always returns FBE_RAID_STATE_STATUS_DONE.
 *
 * @author
 *  12/02/2008  Ron Proulx - Created.
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_recovery_verify_state2(fbe_raid_siots_t * siots_p)
{
    /* Our upgrade is here, so clear our upgrade flag.
     */
    fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE_LOCK);

    /* Flag the fact that we are the lock owner so that we
     * don't start any other siots that are waiting for an upgrade.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: upgrade owner already set\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER);
    /* Continue the recovery verify state machine.
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_recovery_verify_state3);

    /* Before we enter the verify state machine we must tranlate the siots 
     * request range to a physical range.
     */
    siots_p->start_lba = siots_p->parity_start;
    siots_p->xfer_count = siots_p->parity_count;

    return(FBE_RAID_STATE_STATUS_EXECUTING);

} /* end fbe_striper_recovery_verify_state2 */

/*!***************************************************************
 * fbe_striper_recovery_verify_state3()
 ****************************************************************
 *
 * @brief   The iots lock assoicated with this siots has been upgraded.
 *          Now check for an overlap with the newly expanded siots and
 *          the other active siots.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return  This function always returns FBE_RAID_STATE_STATUS_DONE.
 *
 * @author
 *  12/02/2008  Ron Proulx - Created.
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_recovery_verify_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status;

    /* Transition to start our verify.
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state0);

    /* Before we kick off this siots, let's make sure that it can run. 
     * Since we might have changed the parity range, we need to check if 
     * it conflicts with anything else that is running. 
     */
    state_status = fbe_raid_siots_generate_get_lock(siots_p);

    return state_status;
} /* end fbe_striper_recovery_verify_state3 */

/****************************************************************
 * fbe_striper_bva_vr_state0()
 ****************************************************************
 * @brief
 *      This is starting point of BVA verify for R0 type and this
 *      function sets up the algorithm for the BVA verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *      09/28/08  HEW - Created.
 *      11/02/08  MA - added new algorithm for BVA
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_bva_vr_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status;

    if (RAID_COND(parent_siots_p->algorithm != RAID0_WR))
    {
        fbe_raid_siots_set_unexpected_error(parent_siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: not RAID0 WRITE algorithm - unexpected\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

   /* Set up some fields of this nested sub iots.
    */
   fbe_status = fbe_striper_bva_setup_nsiots(siots_p);
   if(RAID_COND(fbe_status != FBE_STATUS_OK))
   {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to set the request range"
                                            "status 0x%x, siots_p 0x%p\n",
                                            __FUNCTION__,
                                            fbe_status, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
   }

   siots_p->algorithm = RAID0_BVA_VR;

   fbe_raid_siots_transition(siots_p, fbe_striper_verify_state0);

    /* Before we kick off this siots, let's make sure that it can run. 
     * We might write out changes and we might overlap with something else, 
     * so we need to make sure we can get a lock. 
     */
    state_status = fbe_raid_siots_generate_get_lock(siots_p);

    return state_status;
}
/********************************************************
 * end of fbe_striper_bva_vr_state0()
 ********************************************************/
/****************************************************************
 * fbe_striper_verify_state0()
 ****************************************************************
 * @brief
 *  This function allocates a RAID_VC_TS tracking structure.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *  to allocate the required number of structures immediately;
 *  otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  02/08/06 - Created. Rob Foley
 *  11/02/08 - modified for BVA. MA
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    /* Make sure the verify is sane.
     */
    status = fbe_striper_verify_validate(siots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed validate status: %d \n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
       
    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_striper_verify_calculate_memory_size(siots_p, &read_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed to calculate mem size status: 0x%x algorithm: 0x%x\n",
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Now allocate all the pages required for this request.  Note that must 
     * transition to `deferred allocation complete' state since the callback 
     * can be invoked at any time. 
     * There are (3) possible status returned by the allocate method: 
     *  o FBE_STATUS_OK - Allocation completed immediately (skip deferred state)
     *  o FBE_STATUS_PENDING - Allocation didn't complete immediately
     *  o Other - Memory allocation error
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state1);
    status = fbe_raid_siots_allocate_memory(siots_p);

    /* Check if the allocation completed immediately.  If so plant the resources
     * now.  Else we will transition to `deferred allocation complete' state when 
     * the memory is available so return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately.  Plant the resources and then
         * transition to next state.
         */
        status = fbe_striper_verify_setup_resources(siots_p, &read_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: verify failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_striper_verify_state3);
    }
    else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING))
    {
        /* Else return waiting
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* Change the state to unexpected error and return
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: verify memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_verify_state0() */

/****************************************************************
 * fbe_striper_verify_state1()
 ****************************************************************
 * @brief
 *  Resources weren't immediately available.  We were called back
 *  by the memory service.
 *
 * @param siots_p - current sub request
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  7/30/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    /* First check if this request was aborted while we were waiting for
     * resources.  If so simply transition to the aborted handling state.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Now check for allocation failure
     */
    if (RAID_MEMORY_COND(!fbe_raid_siots_is_deferred_allocation_successful(siots_p)))
    {
        /* Trace some information and transition to allocation error
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: verify memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */  
    status = fbe_striper_verify_calculate_memory_size(siots_p, &read_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed to calculate mem size status: 0x%x algorithm: 0x%x\n",
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }      

    /* Allocation completed. Plant the resources
     */
    status = fbe_striper_verify_setup_resources(siots_p, &read_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: verify failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***********************************
 * end fbe_striper_verify_state1()
 **********************************/

/****************************************************************
 * fbe_striper_verify_state3()
 ****************************************************************
 * @brief
 *      This function initializes the S/G lists with the
 *      buffer addresses, releasing any buffers that remain
 *      unused.  Then it sends read requests to disks.
 *
 *      siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *      1/20/00 - Created. Kevin Schleicher
 *     11/02/08 - modified for BVA. MA
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state3(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_result = FBE_TRUE;
    fbe_queue_head_t *read_head_p = NULL;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    fbe_raid_siots_get_read_fruts_head(siots_p, &read_head_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Set up the sgs.  We use a different routine for
     * recovery verify.
     */
    if (siots_p->algorithm == RAID0_VR)
    {
        /* Because we have finished allocating resources, we 
         * may start the next the sub-request in the pipeline.
         */
        if (!(fbe_raid_siots_is_any_flag_set(siots_p, (FBE_RAID_SIOTS_FLAG_SINGLE_STRIP_MODE|FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))))
        {
            fbe_status_t status;
            /* Determine if we need to generate another siots.
             * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
             * Typically this call is done after we have started the disk I/Os.
             */
            status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
            if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                    "parity: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                                    __FUNCTION__, siots_p, status);
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
        }
    }

    /* Issue reads to the disks.
     */
    siots_p->wait_count = siots_p->data_disks;

    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state4);

    b_result = fbe_raid_fruts_send_chain(read_head_p, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to start read fruts chain for "
                                            "siots = 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);
        
    }

    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;

} /* end of fbe_striper_verify_state3() */

/*!**************************************************************
 * fbe_striper_verify_handle_read_error()
 ****************************************************************
 * @brief
 *  Determine what to do when we get a FBE_RAID_FRU_ERROR_STATUS_ERROR,
 *  on a read for verify.
 *
 * @param siots_p - current I/O.
 * @param eboard_p - Eboard filled out from a read chain.  
 *
 * @return FBE_RAID_STATE_STATUS_DONE - Just continue with verify.
 *         Otherwise - return this status from the state machine.
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_handle_read_error(fbe_raid_siots_t * siots_p,
                                                            fbe_raid_fru_eboard_t *eboard_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    if (eboard_p->hard_media_err_count)
    {
        if (eboard_p->hard_media_err_count > fbe_raid_siots_get_width(siots_p))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: error count more than width 0x%x 0x%x", 
                                                eboard_p->hard_media_err_count, fbe_raid_siots_get_width(siots_p));
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* If we are here we are doing error recovery. Set the single error recovery flag
         * since we already have an error and do not want to short ciruit any of the error 
         * handling for efficiency. 
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);

        if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
        {
            /* If we cross a region boundary, then we need to drop into a mining mode
             * where we mine the request on the optimal block size (region size). 
             */
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

            siots_p->parity_count = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);
            if(siots_p->parity_count == 0)
            { 
                /* we must have crossed the 32bit limit while calculating the region size
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: siots_p->parity_count == 0 \n");
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
            /* For each fruts, reset it to a single sector,
             * and make minor reset changes.
             */
            status = fbe_raid_fruts_for_each(0,
                                           read_fruts_p,
                                           fbe_raid_fruts_reset,
                                           (fbe_u64_t) siots_p->parity_count,
                                           (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
            
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "striper: %s failed for fruts chain. read_fruts_p = 0x%p, siots_p 0x%p, status = 0x%x\n",
                                                    __FUNCTION__,
                                                    read_fruts_p,
                                                    siots_p, 
                                                    status);
                 return FBE_RAID_STATE_STATUS_EXECUTING;
            }


            status = fbe_striper_verify_setup_sgs_for_next_pass(siots_p);

            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "striper: status from setup sgs for next pass: %d\n", status);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            /* Transition back up to state3.
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state3);
        }
        else
        {
            fbe_status_t verify_status;
            fbe_raid_fruts_t *read_fruts_p;

            /* Already in region mode, Invalidate the sector(s)
             */
            fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
            (void) fbe_raid_fruts_get_media_err_bitmap(read_fruts_p,
                                                       &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

            /* Call strip_mine. This will invalidate the sector giving us
             * the problem.
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state8);

            verify_status = fbe_striper_verify_strip(siots_p);
            if (verify_status != FBE_STATUS_OK)
            {
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "striper: %s unexpected verify error 0x%x\n", __FUNCTION__, verify_status);
            }
        }
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (eboard_p->drop_err_count > 0)
    {
        /* At least one read failed due to a DROPPED. 
         * This is not valid for verifies, they are not optional.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: dropped error unexpected \n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /* Unknown error on this raid group.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: no hard or dropped error\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_striper_verify_handle_read_error()
 ******************************************/
/****************************************************************
 * fbe_striper_verify_state4()
 ****************************************************************
 * @brief
 *  Reads have completed, check status.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *      1/25/00 - Created. Kevin Schleicher 
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_striper_get_fruts_error(read_fruts_p, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All reads are successful.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_verify_state5);
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_ABORTED) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_striper_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_state_status_t state_status;

        state_status = fbe_striper_verify_handle_read_error(siots_p, &eboard);
        
        if (state_status == FBE_RAID_STATE_STATUS_DONE)
        {
            /* Done status will do the verify.
             */ 
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state5);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* Other status set by above call.  State also set by above call.
             */
            return state_status;
        }
    }
    else
    {
        /* Unexpected status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: status unknown 0x%x\n", status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;

}
/* end of fbe_striper_verify_state4() */

/****************************************************************
 * fbe_striper_verify_state5()
 ****************************************************************
 * @brief
 *  Check the checksums.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  2/09/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap = 0;

    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state6);

    status = fbe_striper_verify_strip(siots_p);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to verify to strip with status 0x%x\n", 
                                            __FUNCTION__,
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_verify_state5() */

/**************************************************************** 
 *  fbe_striper_verify_state6() 
 **************************************************************** 
 * @brief
 *  Evaluate status of checksum check.
 *
 *  siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  5/16/00 - Created. MPG
 *  6/28/00 - Added use of rg_record_errors.  Rob Foley
 *
 ****************************************************************/


fbe_raid_state_status_t fbe_striper_verify_state6(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_u16_t w_bitmap;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    w_bitmap = fbe_striper_verify_get_write_bitmap(siots_p);

    if (!fbe_raid_csum_handle_csum_error( siots_p, 
                                          read_fruts_p, 
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                          fbe_striper_verify_state4))
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    if ( fbe_raid_siots_record_errors(siots_p,
                          fbe_raid_siots_get_width(siots_p),
                          (fbe_xor_error_t *) & siots_p->misc_ts.cxts.eboard,
                          FBE_RAID_FRU_POSITION_BITMASK,
                          FBE_FALSE,
                          FBE_TRUE /* Yes also validate. */)
         != FBE_RAID_STATE_STATUS_DONE )
    {
        /* Retries were performed in rg_record errors.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* We will skip the writes here if we are doing Read Only Background Verify
     */
    if (w_bitmap && (! fbe_raid_siots_is_read_only_verify(siots_p)))
    {
        fbe_u32_t write_opcode;

        /* Checksum error is detected in the read data,
         * We must now call the library routine to check 
         * checksum's and invalidate the region.
         */
        siots_p->wait_count = fbe_raid_get_bitcount(w_bitmap);

        if (RAID_COND(siots_p->wait_count == 0))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: wait_count unexpected %lld, w_bitmap: 0x%x\n", 
                                                siots_p->wait_count, w_bitmap);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        if (siots_p->algorithm == RAID0_VR)
        {
            /* A read remap does a write verify which can fail 
             * with a media error. 
             */
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
        }
        else
        {
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
        }
        /* Set all fruts to WRITE opcode
         */
        fbe_status =  fbe_raid_fruts_for_each(0,
                                           read_fruts_p,
                                           fbe_raid_fruts_set_opcode_crc_check,
                                           (fbe_u64_t) write_opcode,
                                           (fbe_u64_t) NULL);
        
        if (RAID_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: %s failed for fruts chain. read_fruts_p = 0x%p, siots_p 0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                read_fruts_p,
                                                siots_p, 
                                                fbe_status);
             return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Start writes to all frus for our write bitmap.
         */
        fbe_status =  fbe_raid_fruts_for_each(w_bitmap,
                                           read_fruts_p,
                                           fbe_raid_fruts_retry,
                                           (fbe_u64_t) write_opcode,
                                           (fbe_u64_t) fbe_raid_fruts_evaluate);
        
        if (RAID_FRUTS_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to retry fruts chain. read_fruts_p = 0x%p, siots_p 0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                read_fruts_p,
                                                siots_p, 
                                                fbe_status);
             return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        status = FBE_RAID_STATE_STATUS_WAITING;
        fbe_raid_siots_transition(siots_p, fbe_striper_verify_state9);
    }
    else if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
    {
        /* We are in single region mode, but did not need to invalidate.
         * Return back to state7 to continue region mining.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_verify_state7);
    }
    else
    {
        /* We are not in single region mode, and did not invalidate.
         * This request is complete, clean up and exit.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_verify_state20);
    }
    return status;

}
/* end of fbe_striper_verify_state6() */

/****************************************************************
 * fbe_striper_verify_state7()
 ****************************************************************
 * @brief
 *  Errored sector search state. 
 *  Set up for next region and send out the request.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  1/26/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state7(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_block_count_t last_request_size;
    fbe_u32_t region_size = fbe_raid_siots_get_region_size(siots_p);
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Should only enter this function if region mining.
     */    
    if(RAID_COND(!(fbe_raid_siots_is_any_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: siots_p->flags 0x%x & "
                                            "FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE 0x%x) == 0\n",
                                            siots_p->flags,
                                            FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Increment the lba in the fruts to the next sector or region
     */
    siots_p->parity_start += siots_p->parity_count;

    /* The size of the last request is fbe_raid_siots_get_region_size.
     */
    last_request_size = siots_p->parity_count;

    /* Check to see if we are done.
     */
    if (siots_p->parity_start >= (siots_p->start_lba +
                                  siots_p->xfer_count))
    {
        /* We have reached the end of this verify request. Exit
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_verify_state20);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Make sure each region will
     * line up on a region boundary.
     */
    if (siots_p->parity_start % region_size)
    {
        /* Align our count so that we end on a region boundary.
         */
        siots_p->parity_count = (fbe_u32_t)(region_size - (siots_p->parity_start % region_size));
    }
    else
    {
        /* Start aligned to optimal size, just do the next piece.
         */ 
        siots_p->parity_count = region_size;
    }

    /* Set the count to the min of the current count and what is left.
     */
    siots_p->parity_count = FBE_MIN(siots_p->parity_count,
                            (fbe_u32_t)((siots_p->start_lba + siots_p->xfer_count) -
                            siots_p->parity_start));

    /* Re-init the fruts so we can use it for a region read Use the lba that is
     * already found in parity_start . It will be incremented at the bottom of the 
     * loop. 
     */
    status = fbe_raid_fruts_for_each(0,
                                  read_fruts_p,
                                  fbe_raid_fruts_reset_vr,
                                  last_request_size,
                                  siots_p->parity_count);
    
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s: unexpected error for fruts chain read_fruts_p = 0x%p, "
                                            "siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p, 
                                            status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Return to state 3 to continue verify state machine
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state3);
    
    status = fbe_striper_verify_setup_sgs_for_next_pass(siots_p);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: status from setup sgs for next pass: %d\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_verify_state7() */

/****************************************************************
 * fbe_striper_verify_state8()
 ****************************************************************
 * @brief
 *  Errored Sector search state.
 *  Have identified an errored sector. Invalidate it.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  5/15/00 - Created. MPG 
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state8(fbe_raid_siots_t * siots_p)
{
    fbe_u16_t u_bitmap = siots_p->misc_ts.cxts.eboard.u_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_u16_t w_bitmap = siots_p->misc_ts.cxts.eboard.w_bitmap;
    fbe_u32_t write_opcode;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    if ( fbe_raid_siots_record_errors(siots_p,
                          fbe_raid_siots_get_width(siots_p),
                          (fbe_xor_error_t *) & siots_p->misc_ts.cxts.eboard,
                          FBE_RAID_FRU_POSITION_BITMASK,
                          FBE_FALSE,
                          FBE_TRUE /* Yes also validate. */)
         != FBE_RAID_STATE_STATUS_DONE )
    {
        /* Retries were performed in rg_record errors.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Uncorrectable bitmap and the write bitmap should be exactly
     * The same on a LUD and Raid0.
     */
    if (RAID_COND(u_bitmap != w_bitmap))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: u_bitmap 0x%x != w_bitmap 0x%x\n", 
                                            u_bitmap, w_bitmap);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    siots_p->wait_count = fbe_raid_get_bitcount(w_bitmap);

    if (RAID_COND(siots_p->wait_count == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: wait_count unexpected %lld, w_bitmap: 0x%x\n", 
                                            siots_p->wait_count, w_bitmap);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (siots_p->algorithm == RAID0_VR)
    {
        /* A read remap does a write verify which can fail 
         * with a media error. 
         */
        write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
    }
    else
    {
        write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
    }

    /* Set all fruts to WRITE opcode
     */
    status = fbe_raid_fruts_for_each(0,
                                   read_fruts_p,
                                   fbe_raid_fruts_set_opcode,
                                   (fbe_u64_t) write_opcode,
                                   (fbe_u64_t) NULL);

    
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to set opcode: read_fruts_p = 0x%p, "
                                            "siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p, 
                                            status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Start writes to all frus for our write bitmap.
     */
    status = fbe_raid_fruts_for_each(w_bitmap,
                                   read_fruts_p,
                                   fbe_raid_fruts_retry,
                                   (fbe_u64_t) write_opcode,
                                   (fbe_u64_t) fbe_raid_fruts_evaluate);
    
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s unexpected error for fruts chain read_fruts_p = 0x%p, "
                                            "siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p, 
                                            status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }


    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state9);
    return FBE_RAID_STATE_STATUS_WAITING;

}                               /* end of fbe_striper_verify_state8() */

/****************************************************************
 * fbe_striper_verify_state9()
 ****************************************************************
 * @brief
 *      Errored sector search state.
 *      Write is done for an invalidation of a sector. Check
 *      status and continue single sector mode.
 *
 *      siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *      1/25/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state9(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &write_fruts_p);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_striper_get_fruts_error(write_fruts_p, &eboard);
    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        if ((eboard.soft_media_err_count > 0) && (siots_p->algorithm == RAID0_VR))
        {
            /* Verifies that get a soft media error on the write will fail 
             * back to verify so that verify will retry this.  This limits 
             * the number of media errors we try to fix at once. 
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state10);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* All writes are successful.
         */
        /* If we are in single sector mode continue to next sector.
         * Otherwise, we are done.
         */
        if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
        {
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state7);
        }
        else
        {
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state20);
        }
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_ABORTED) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_striper_handle_fruts_error(siots_p, write_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        if (eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED. 
             * This is not valid for verifies, they are not optional.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: dropped error unexpected 0x%x\n", status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else if ((siots_p->algorithm == RAID0_VR) &&
                 eboard.hard_media_err_count > 0)
        {
            /* Read remaps do write verifies, so they can get media errors.
             * Simply transition to return status. 
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_verify_state10);
        }
        else
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: error unexpected 0x%x\n", status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    } /* end FBE_RAID_FRU_ERROR_STATUS_ERROR */
    else
    {
        /* Unexpected status.  We do not expect waiting.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: error unexpected 0x%x\n", status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_verify_state9() */

/*!**************************************************************
 * @fn fbe_striper_verify_state10(fbe_raid_siots_t * siots_p)
 ****************************************************************
 * @brief
 *  Process a media error on a write/verify request.
 *  
 * @param siots_p - The current request.
 * 
 * @return fbe_raid_state_status_t - Always FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  02/05/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state10(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_block_count_t min_blocks = siots_p->parity_count;
    fbe_u16_t errored_bitmask = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    /* The writes actually stay on the read chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Transition to next state regardless.
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state21);

    /* Determine how much we transferred.
     */
    status = fbe_raid_fruts_get_min_blocks_transferred(read_fruts_ptr, 
                                                       read_fruts_ptr->lba, &min_blocks, &errored_bitmask);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: get min blocks transferred unexpected status %d\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We need to set media_error lba. 
     * Note that if we block or strip mined, the iots->blocks_transferred 
     * would have been incremented as part of the strip mine. 
     * Thus we only need to worry about setting media error lba to indicate the number of 
     * blocks successfully transferred in this block/strip pass. 
     */
    fbe_raid_siots_set_media_error_lba(siots_p, siots_p->start_lba + min_blocks); 
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
   
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                "striper: wr media err lba:0x%llX blks:0x%llx m_lba: 0x%llx err_bitmask:0x%x\n",
                                (unsigned long long)siots_p->start_lba,
			        (unsigned long long)siots_p->xfer_count,
                                (unsigned long long)siots_p->media_error_lba,
			        errored_bitmask);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_striper_verify_state10()
 **************************************/
/*!***************************************************************
 * fbe_striper_verify_state20()
 ****************************************************************
 * @brief
 *   The verify is completing with good status.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *      1/25/00 - Created. Kevin Schleicher
 *     11/02/08 - modified for BVA. MA
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state20(fbe_raid_siots_t * siots_p)
{

    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state21);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_verify_state20() */

/*!**************************************************************
 * fbe_striper_verify_state21()
 ****************************************************************
 * @brief
 *  This is the terminal state for verifies, regardless of the status.
 *  
 * @param siots_p - The current request.
 * 
 * @return fbe_raid_state_status_t - Always FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  02/06/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_verify_state21(fbe_raid_siots_t * siots_p)
{
    if (fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p)) != FBE_TRUE)
    {
        /* In the case of R0/ID, we want to log something to the
         * event log to indicate what happened here. For stripped
         * mirror raid group (RAID 10), all logging will be done
         * at mirror level.
         */
        fbe_striper_report_errors(siots_p, FBE_TRUE);
    }

    fbe_raid_siots_transition_finished(siots_p);
    /* After reporting errors, just send Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
     * to PDO for single or multibit CRC errors found 
     */
    if ( (fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p)) != FBE_TRUE) 
        && (fbe_raid_siots_pdo_usurper_needed(siots_p)))
    {
       fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
       /*We are waiting only when usurper has been sent, else executing should be returned*/
       if (send_crc_status == FBE_STATUS_OK)
       {
           return FBE_RAID_STATE_STATUS_WAITING;
       }
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_verify_state21() */

/*************************
 * end file fbe_striper_verify.c
 *************************/
