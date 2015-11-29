/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_rebuild.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions of the
 *  RAID 5 rebuild (rb) state machine.
 *
 *  This state machine handles two types of algorithms.
 *
 *  ** R5_RB **
 *  A external rebuild has the R5_RB algorithm.
 *  This algorithm rebuilds an extent and writes the rebuilt data out
 *  to the rebuilding drive.
 *  This algorithm also updates the rebuild checkpoint after each pass.
 *  Note that this algorithm allows pipelining of SIOTS.
 *    If the SIOTS finish out of order, then we always allow the rebuild
 *    checkpoint to remain at the "highest" address.
 *
 *  ** Common Features **
 *  
 *  Strip mining mode.
 *    While we are strip mining a request, a single SIOTS will
 *     break up an extent into multiple pieces.  This occurs
 *     when a SIOTS encounters hard errors.
 *
 *    During the strip mine:
 *     xfer_count - how much is left in the area
 *       being rebuilt by this SIOTS. This is decremented after
 *       each successful strip mine pass.
 *
 *     parity_start - beginning of the current strip mine pass
 *
 *     parity_count - amount being operated on in the current pass
 *                    Typically, strip mine mode operates on
 *                    a fixed range throughout the rebuild.
 *                    Then at the end we rebuild the amount remaining.
 *
 *
 * @author
 *  10/99: Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 **    INCLUDE FILES    **
 *************************/

#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"


/**************************************** 
 * Local function prototypes.
 ****************************************/
static fbe_raid_state_status_t fbe_parity_rebuild_state1(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state3(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state4(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state5(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state6(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state7(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state9(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state10(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rebuild_state12(fbe_raid_siots_t * siots_p);

/*!***************************************************************
 *  fbe_parity_rebuild_state0()
 ****************************************************************
 * @brief
 *  This function allocates vcts structures.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING if we
 *  were unable to allocate the required number of structures immediately;
 *  otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  9/9/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_rebuild_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_u32_t num_pages = 0;
    fbe_raid_position_bitmask_t rebuild_bitmask;
    fbe_raid_siots_get_rebuild_bitmask(siots_p, &rebuild_bitmask);
    
    /* Validate the algorithm and rebuild pos.
     */
    if (RAID_COND(siots_p->algorithm != R5_RB))
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: wrong algorithm: 0x%x\n",
                                            siots_p->algorithm);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (RAID_COND(rebuild_bitmask == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: rebuild positions 0x%x\n", rebuild_bitmask);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_rebuild_get_fru_info(siots_p, &read_info[0], &write_info[0]);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: status of 0x%x from get rebuild fru info.\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_rebuild_calculate_memory_size(siots_p, &read_info[0], &write_info[0], &num_pages);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: status of 0x%x from calculate rebuild memory size.\n",
                                            status);
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
    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state1);
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
        status = fbe_parity_rebuild_setup_resources(siots_p, &read_info[0], &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: rebuild failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_rebuild_state3);
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
                                            "parity: rebuild memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_rebuild_state0()
 *******************************/

/****************************************************************
 * fbe_parity_rebuild_state1()
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

fbe_raid_state_status_t fbe_parity_rebuild_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

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
                             "parity: rebuild memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_rebuild_get_fru_info(siots_p, &read_info[0], &write_info[0]);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: status of 0x%x from get rebuild fru info.\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_rebuild_calculate_memory_size(siots_p, &read_info[0], &write_info[0], &num_pages);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: status of 0x%x from calculate rebuild memory size.\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_rebuild_setup_resources(siots_p, &read_info[0], &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: rebuild failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/********************************
 * end fbe_parity_rebuild_state1()
 ********************************/

/*!***************************************************************
 *      fbe_parity_rebuild_state3()
 ****************************************************************
 * @brief
 *   This function initializes the S/G lists,
 *   and starts the reads.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *   9/09/99 - Created. Rob Foley
 *   1/29/00 - Modified to pipeline SIOTS.
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);
    
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* R6 continues degraded, so we need to check if we lost 
     * another drive while waiting.
     */
    if ( fbe_raid_siots_parity_num_positions(siots_p) == 2 )
    {
        fbe_status_t status;
        /* See if we are degraded on this LUN.
         */
        status = fbe_parity_check_degraded_phys(siots_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to check degraded status: 0x%x\n", 
                                                __FUNCTION__, status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }

    /* If non-nest rebuild and we are not in ss mode,
     * start the pipeline.
     */
    if (!fbe_raid_siots_is_nested(siots_p) && 
        !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
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
    
    /* We are submitting a read to all but the
     * rebuilding drive (array_width - dead drives)
     */
    siots_p->wait_count = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_dead_pos_count(siots_p);
        
    /* Before we send out any read request,
     * take the dead disk to be NOP.
     * Since we always set dead_pos before dead_pos_2,
     * just checking dead_pos is good enough to tell us 
     * if we have a dead disk or not.
     */
    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
    }
    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state4);
        
    /* Send off the reads.
     */
    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to start read fruts chian for "
                                            "siots 0x%p, b_result = %d\n",
                                            __FUNCTION__,
                                            siots_p,
                                            b_result);        
    }

    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************
 * end fbe_parity_rebuild_state3()
 *******************************/

/*!***************************************************************
 * fbe_parity_rebuild_state4()
 ****************************************************************
 * @brief
 *  Interrogate status of the reads.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  returns FBE_RAID_STATE_STATUS_EXECUTING if retries are not needed and
 *  FBE_RAID_STATE_STATUS_WAITING otherwise.
 *
 * @author
 *  9/9/99 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state4(fbe_raid_siots_t * siots_p)
{
    /* Get status of reads.
     */
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(read_fruts_p, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All reads successful, continue with rebuild.
         */
        if (RAID_COND((eboard.dead_err_count != 0) || (eboard.hard_media_err_count != 0)))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: error count unexpected : dead_err_count = %d, hard_err_count = %d\n",
                                                eboard.dead_err_count,
                                                eboard.hard_media_err_count);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state5);
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_raid_state_status_t state_status;

         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* Only on raid 6 we might be able to see another drive go away. 
         * Check now since retries might occur below. 
         */
        if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            fbe_status_t status;
            /* See if we are degraded on this LUN.
             */
            status = fbe_parity_check_degraded_phys(siots_p);
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: %s failed to check degraded status: 0x%x\n", 
                                                    __FUNCTION__, status);
                fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* Make the opcode of any degraded positions be nop.
             */
            fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
        } /* end if raid6 */

        if (eboard.drop_err_count != 0)
        {
            /* We should not get dropped here.  The only other 
             * errors could get are hard error and dead error.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "%s dropped errors unexpected status: 0x%x count: 0x%x bitmap: 0x%x",  
                                                __FUNCTION__, status, eboard.drop_err_count, eboard.drop_err_count);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else if (eboard.dead_err_count != 0)
        {
            /* We should not get dead here.  Only FBE_RAID_FRU_ERROR_STATUS_DEAD.
             */ 
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "%s dead errors unexpected status: 0x%x count: 0x%x bitmap: 0x%x",  
                                                __FUNCTION__, status, eboard.dead_err_count, eboard.dead_err_bitmap);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else if (eboard.hard_media_err_count > 0)
        {
            /* Transition to state to handle media errors.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state10);
        } /* end if hard errors */
        else
        {
            /* We should not get dead here.  Only FBE_RAID_FRU_ERROR_STATUS_DEAD.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "%s error type unexpected for fru eboard status: 0x%x\n",  
                                                __FUNCTION__, status );
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }/* end if FBE_RAID_FRU_ERROR_STATUS_ERROR */
    else
    {
        /* No other possibilities.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "%s unexpected fru eboard status: 0x%x",  __FUNCTION__, status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_rebuild_state4()
 *******************************/
/*!***************************************************************
 * fbe_parity_rebuild_state5()
 ****************************************************************
 * @brief
 *  Reads have completed successfully.
 *  Continue with normal rebuild by rebuiding the extent.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns  FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  9/9/99 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Only on raid 6 we might be able to see another drive go away. 
     * Check now since retries might occur below. 
     */
    if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        fbe_status_t status;
        /* See if we are degraded on this LUN.
         */
        status = fbe_parity_check_degraded_phys(siots_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to check degraded status: 0x%x\n", 
                                                __FUNCTION__, status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    
        /* Make the opcode of any degraded positions be nop.
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
    } /* end if raid6 */

    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state6);

    (void) fbe_raid_fruts_get_media_err_bitmap(read_fruts_p,
                                               &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

    /* If there are hard errors then we are in single strip mode or
     * we are a raid group with an optimal size > 1. 
     */
    if (RAID_COND((siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap != 0) &&
                  (siots_p->parity_count > fbe_raid_siots_get_region_size(siots_p)) ))
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected hard error : hard media error bitmap 0x%x,"
                                            "parity count = 0x%llx, region size = 0x%x,"
                                            "optimal block size is not 1\n",
                                            siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap,
                                            (unsigned long long)siots_p->parity_count,
                                            fbe_raid_siots_get_region_size(siots_p));

        return FBE_RAID_STATE_STATUS_EXECUTING;
     }

    /* Do a stripwise rebuild. This function will
     * submit any error messages.
     */
    status = fbe_parity_rebuild_extent(siots_p);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected status 0x%x\n",
                                            status);

        return FBE_RAID_STATE_STATUS_EXECUTING;
     }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_rebuild_state5()
 *******************************/
/*!***************************************************************
 * fbe_parity_rebuild_state6()
 ****************************************************************
 * @brief
 *  Rebuild completed.  Start writes of rebuilding and any 
 *  errored positions.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns 
 *  FBE_RAID_STATE_STATUS_EXECUTING and FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *  7/7/2010 - Created. Rob Foley
 *  
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state6(fbe_raid_siots_t * siots_p)
{
    fbe_u16_t w_bitmap = siots_p->misc_ts.cxts.eboard.w_bitmap;
    fbe_u16_t u_bitmap = siots_p->misc_ts.cxts.eboard.u_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t rebuild_bitmask;
    fbe_raid_siots_get_rebuild_bitmask(siots_p, &rebuild_bitmask);

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    if ((w_bitmap & siots_p->misc_ts.cxts.eboard.m_bitmap) !=
        siots_p->misc_ts.cxts.eboard.m_bitmap)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: write bitmap: 0x%x != m_bitmap: 0x%x rb_bitmask: 0x%x",
                                            w_bitmap,  siots_p->misc_ts.cxts.eboard.m_bitmap, rebuild_bitmask);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* We must be at least writing out the rebuilt drive.
     * If not something went wrong above.
     */
    if (RAID_COND((w_bitmap & rebuild_bitmask) != rebuild_bitmask))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected rebuild behaviour."
                                            "write bitmap = 0x%x, rebuild bitmask = 0x%x\n",
                                            w_bitmap, rebuild_bitmask);

        return FBE_RAID_STATE_STATUS_EXECUTING;
     }

    if (RAID_COND(((w_bitmap == 0) ||
                 ((w_bitmap & rebuild_bitmask) == 0))))
    {
        /* We have not selected any drives, or
         * have selected all but the drive to rebuild.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected write bitmap: 0x%x rb_bitmask: 0x%x",  
                                             w_bitmap, rebuild_bitmask);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (!fbe_raid_csum_handle_csum_error( siots_p, read_fruts_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                          fbe_parity_rebuild_state4 ))
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Record errors detected by the XOR operation.
     * We expect either uncorrectables, or retried crc errors.
     * Always call fbe_raid_siots_record_errors() for R6 since it can also
     * take correctable errors here.
     */
    if ( u_bitmap || siots_p->vrts_ptr->retried_crc_bitmask ||
         fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) )
    {
        if ( fbe_raid_siots_record_errors(siots_p,
                              fbe_raid_siots_get_width(siots_p),
                              (fbe_xor_error_t *) & siots_p->misc_ts.cxts.eboard,
                              (fbe_u16_t) - 1,
                              /* On R5 we don't allow correctables,
                               * on R6 it could be correctable or uncorrectable.
                               */
                              fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)),
                              FBE_TRUE /* Yes also validate. */)
             != FBE_RAID_STATE_STATUS_DONE )
        {
            /* Retries were performed in rg_record errors.
             */
            return FBE_RAID_STATE_STATUS_WAITING;
        }
    }
    
    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state7);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_rebuild_state6()
 *******************************/

/*!***************************************************************
 * fbe_parity_rebuild_state7()
 ****************************************************************
 * @brief
 *  Rebuild completed.  Start writes of rebuilding and any 
 *  errored positions.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns 
 *  FBE_RAID_STATE_STATUS_EXECUTING and FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *  7/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state7(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_u16_t w_bitmap = siots_p->misc_ts.cxts.eboard.w_bitmap;
    fbe_u16_t u_bitmap = siots_p->misc_ts.cxts.eboard.u_bitmap;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t rebuild_bitmask;
    fbe_raid_siots_get_rebuild_bitmask(siots_p, &rebuild_bitmask);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* No need to write into dead disks.
     */
    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_raid_position_bitmask_t rl_bitmask;
        fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rl_bitmask);

        /* If there are two dead positions, then it
         * means we are rebuilding one position with another dead.
         * In this case we should clear out the drive that is dead (the
         * non rebuilding drive, since we shouldn't be writing it.
         */
        w_bitmap &= ~(rl_bitmask);
        u_bitmap &= ~(rl_bitmask);

        if (u_bitmap == rebuild_bitmask )
        {
            u_bitmap = 0;
        }
    }
    
    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state9);

    /* Start the fruts chain for needed writes.
     */
    if ((u_bitmap == 0) && ((w_bitmap & (~rebuild_bitmask)) == 0))
    {
        fbe_raid_position_bitmask_t rebuild_logging_bitmask;
        fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rebuild_logging_bitmask);

        /* Normal case. Write rebuilding drive only.
         */
        siots_p->wait_count = fbe_raid_get_bitcount(rebuild_bitmask);

        if (rebuild_logging_bitmask)
        {
            /* These positions are not present, do not write out for these.
             */
            status = fbe_raid_fruts_for_each_only(rebuild_logging_bitmask,
                                                  write_fruts_p,
                                                  fbe_raid_fruts_set_opcode,
                                                  (fbe_u64_t)FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, (fbe_u64_t)0);
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "unexpected status 0x%x from fbe_raid_fruts_for_each_only\n",
                                                    status);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
        }

        if (RAID_COND(siots_p->wait_count == 0))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: wait_count is zero\n");
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
        if (RAID_FRUTS_COND(b_result != FBE_TRUE))
        {
            /* We hit an issue while starting chain of fruts.
            */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to start write fruts chian for "
                                                "siots 0x%p, b_result = %d\n",
                                                __FUNCTION__,
                                                siots_p,
                                                b_result);
        }
    }
    else
    {
        /* Since we no longer invalidate parity, we could recreate a sector that
         * was previously invalidated without the need to update parity.  Thus
         * the minimum number of write positions is 1.
         */
        if (RAID_COND(fbe_raid_get_bitcount(w_bitmap) < 1))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: unexpected write bitmap 0x%x\n",
                                                fbe_raid_get_bitcount(w_bitmap));

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        if (RAID_COND(fbe_raid_get_bitcount(w_bitmap) < fbe_raid_get_bitcount(u_bitmap)))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: write count 0x%x < uncorrectable count 0x%x\n",
                                                fbe_raid_get_bitcount(w_bitmap),
                                                fbe_raid_get_bitcount(u_bitmap));

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Write all the positions from the write bitmap.
         */
        status = fbe_parity_rebuild_multiple_writes(siots_p, w_bitmap);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "unexpected status 0x%x from fbe_parity_rebuild_multiple_writes\n",
                                                status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************
 * end fbe_parity_rebuild_state7()
 *******************************/

/*!***************************************************************
 *  fbe_parity_rebuild_state9()
 ****************************************************************
 * @brief
 *  The writes completed, check the status of the writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t      
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  9/20/99 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state9(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_state_status_t e_return_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *write_fruts_p = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    
    fbe_raid_fru_eboard_init(&eboard);
    
    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(write_fruts_p, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Write successful, send good status.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state12);
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "parity rebuild state 9 failed with retry error\n");
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, write_fruts_p, &eboard, status);
        return state_status;
    }
    else
    {
        /* Unexpected error
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "%s unexpected status: 0x%x", __FUNCTION__, status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return e_return_status;
}
/*********************
 * end fbe_parity_rebuild_state9()
 *********************/

/*!***************************************************************
 *  fbe_parity_rebuild_state10()
 ****************************************************************
 * @brief
 *  Encountered a hard error.
 *  Decide if retries are necessary or if single sector mode
 *  should be entererd.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t      
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  02/09/00 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state10(fbe_raid_siots_t * siots_p)
{
    /* [At least] one read failed due to hard error.
     */
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;

    if ( !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE) )
    {
        /* If we cross a region boundary, then we need to drop into a mining mode
         * where we mine the request on the optimal block size (region size). 
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

        /* Set the parity count to the correct value for mining a region.
         */
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

        /* Re-init all our fruts with the new count.
         */
        fbe_parity_rebuild_reinit(siots_p);

        /* Transition back to the issue reads state to
         * reinit our sgs and start reads.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state3);
    }
    else
    {
        /* We are performing the smallest size allowed, either 
         * our block mode size or our strip mine size. 
         * Transition to the next state and invalidate.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state5);
    }

    return status;
}
/*********************
 * end fbe_parity_rebuild_state10()
 *********************/
/*!***************************************************************
 *  fbe_parity_rebuild_state12()
 ****************************************************************
 * @brief
 *  The writes were successfull, return good status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_DONE.
 *
 * @author
 *  9/20/99 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rebuild_state12(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status;
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else
    {
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

        if (siots_p->xfer_count > siots_p->parity_count)
        {
            /* We are in SINGLE STRIP mode, and we are not done.
             */
            if (RAID_COND(siots_p->xfer_count == 0))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity:xfer count == 0\n");

                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* Since this is single strip and external,
             * advance blocks transferred (the xfer_count is subtracted below
             * to insure blocks transferred isn't incremented twice). 
             * Expansion rebuilds shouldn't have their blocks transferred 
             * increased, since the xfer_count decrement below does not 
             * affect that siots's count.
             */
            if (siots_p->algorithm == R5_RB)
            {
                fbe_raid_iots_lock(iots_p);
                iots_p->blocks_transferred += siots_p->parity_count;
                fbe_raid_iots_unlock(iots_p);
            }

            /* This strip mine operation has completed.
             * Generate and start the next strip mine operation.
             */
            siots_p->xfer_count -= siots_p->parity_count;
            siots_p->parity_start += siots_p->parity_count;

            if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
            {
                /* Set the parity count to the correct value for mining a region.
                 */
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
            }

            if (RAID_COND(siots_p->parity_count == 0))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: siots_p->parity_count == 0\n");

                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            if (RAID_COND(siots_p->parity_count > siots_p->xfer_count))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: parity count  0x%llx > xfer count 0x%llx\n",
                                                    (unsigned long long)siots_p->parity_count,
                                                    (unsigned long long)siots_p->xfer_count);

                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* We are strip mining this rebuild.
             * Re-initialize the tracking structures to
             * allow the next piece of this siots to be started.
             */
            status = fbe_parity_rebuild_reinit(siots_p);
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: status %d from rebuild reinit\n", status);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state3);
        }
        else
        {
            /* xfer count is always less than parity count
             */
            if (RAID_COND(siots_p->xfer_count > siots_p->parity_count))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: xfer count  0x%llx > parity count 0x%llx\n",
                                                    (unsigned long long)siots_p->xfer_count,
                                                    (unsigned long long)siots_p->parity_count);

                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            fbe_parity_report_errors(siots_p, FBE_TRUE);

            /* This rebuild is complete, ack the caller.
             */
            if (fbe_raid_siots_is_nested(siots_p))
            {
                /* This was an internal rebuild/shed
                 * Free siot and ack caller.
                 */
            }
            else
            {
                /* We need to ack the upper driver. */
 
            }                   /* Else not nested rebuild */
            fbe_raid_siots_transition_finished(siots_p);
            /* After reporting errors, just send Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
             * to PDO for single or multibit CRC errors found 
             */
            if(fbe_raid_siots_pdo_usurper_needed(siots_p))
            {
                fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
                /*We are waiting only when usurper has been sent, else executing should be returned*/
                if (send_crc_status == FBE_STATUS_OK)
                {
                    return FBE_RAID_STATE_STATUS_WAITING;
                }
            }
        }                       /* Else not strip minining */

    }                           /* Not aborted */

    return FBE_RAID_STATE_STATUS_EXECUTING;

}
/*******************************
 * end fbe_parity_rebuild_state12()
 *******************************/
/***********
 * End fbe_parity_rebuild.c
 ***********/
