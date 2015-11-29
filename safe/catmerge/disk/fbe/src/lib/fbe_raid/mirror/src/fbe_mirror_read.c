/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state code for a mirror read.
 *
 * @version
 *   5/15/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_mirror_io_private.h"
#include "fbe_raid_fruts.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!***************************************************************************
 *          fbe_mirror_read_state0()
 *****************************************************************************
 *
 * @brief   This state allocates all neccessary resources for a read request.
 *          The default state is FBE_RAID_STATE_STATUS_EXECUTING since the
 *          majority of the time the resources will be immediately available.
 *
 * @param   siots_p, [IO] - ptr to the fbe_raid_siots_t
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_WAITING if the resources
 *          aren't immediately available.
 *
 * @author
 *  05/20/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state0(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     read_info[FBE_MIRROR_MAX_WIDTH];

    /* Validate that this is properly configured mirror verify request.
     */
    if ( (status = fbe_mirror_read_validate(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read validate failed. status: 0x%0x\n",
                                            status);
        return(state_status);
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    if ((status = fbe_mirror_read_calculate_memory_size(siots_p, 
                                                        &read_info[0])) != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read calculate memory size failed. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now allocate all the pages required for this read request.  Note that
     * must transition to state1 since the callback can be invoked at any time. 
     * There are (3) possible status returned by allocate: 
     *  o FBE_STATUS_OK - Allocation completed immediately (never transition to state1)
     *  o FBE_STATUS_PENDING - Allocation didn't complete immediately
     *  o Other - Memory allocation error
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_read_state1);
    status = fbe_raid_siots_allocate_memory(siots_p);

    /* Check if the allocation completed immediately.  If so plant the resources
     * now.  Else we will transition to `state1' when the memory is available so 
     * return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately.  Plant the resources and then
         * transition to next state.
         */
        status = fbe_mirror_read_setup_resources(siots_p, 
                                                 &read_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read failed to setup resources. status: 0x%x\n",
                                            status);
            return(state_status);
        }

        /* To prevent confusion (i.e. we never went to state1) overwrite the
         * current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_mirror_read_state3);
    }
    else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING))
    {
        /* Else return waiting
         */
        return(FBE_RAID_STATE_STATUS_WAITING);
    }
    else
    {
        /* Change the state to unexpected error and return
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read memory request failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Always return the executing status.
     */
    return(state_status);
}   
/* end of fbe_mirror_read_state0() */

/*!***************************************************************************
 *          fbe_mirror_read_state1()
 *****************************************************************************
 *
 * @brief   This state is the callback for the memory allocation completion.
 *          We validate and plant the requested resources then transition to
 *          the issue state.  The only state status returned is
 *          FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @param   siots_p, [IO] - ptr to the fbe_raid_siots_t
 *
 * @author
 *  10/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state1(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     read_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t     write_info[FBE_MIRROR_MAX_WIDTH];
    fbe_u16_t               num_sgs[FBE_RAID_MAX_SG_TYPE];

    /* Need to clear num sgs array
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

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
                             "mirror: read memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return(state_status);
    }

    /* Now calculate the fruts information for each position including the
     * sg index if required.
     */
    status = fbe_mirror_read_get_fru_info(siots_p, 
                                          &read_info[0],
                                          &write_info[0],
                                          &num_sgs[0],
                                          FBE_TRUE /* Log a failure */);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read get fru info failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now plant the resources
     */
    status = fbe_mirror_read_setup_resources(siots_p, 
                                             &read_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read failed to setup resources. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Else the resources were setup properly, so transition to
     * the next read state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_read_state3);

    /* Always return the executing status.
     */
    return(state_status);
}   
/* end of fbe_mirror_read_state1() */

/*!***************************************************************************
 *          fbe_mirror_read_state3()
 *****************************************************************************
 *
 * @brief   This state issues one or more reads to the non-degraded
 *          positions (relative to this siots).  Before issuing any
 *          request (which this state does) we need to check that 
 *          the request hasn't been aborted.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_WAITING
 *      FBE_RAID_STATE_STATUS_EXECUTING if need to read from another drive.
 *
 * @author
 *  05/20/2009  Ron Proulx  - Updated from mirror_rd_state3
 *  11/2011 Susan Rundbaken - Updated for raw mirror reads
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state3(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_mirror_prefered_position_t prefered_mirror_pos_p;

    /* If this request has already been aborted transition to the abort 
     * handling.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted, treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* We need to re-evaluate our degraded positions before sending the request
     * since the raid group monitor may have updated our degraded status. 
     */  
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  FBE_FALSE, /* This is a read request */
                                                  FBE_FALSE  /* Don't write degraded positions*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* For raw mirror reads, transition to the verify state machine.
     * Raw mirror reads read from each mirror to compare sector data metadata and chose the appropriate mirror. 
     */
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_mirror_read_state7);

        return(FBE_RAID_STATE_STATUS_EXECUTING);    
    }

    /* The wait count is simply the number of items in the read chain
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);

    /* If the read count isn't (1) something is wrong.
     */
    if (siots_p->wait_count != 1)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: wait count unexpected 0x%llx \n", 
                                            siots_p->wait_count);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Determine if we need to generate another siots.
     * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
     * Typically this call is done after we have started the disk I/Os.
     */
    status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                            __FUNCTION__, siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
    /*mirror optimization */
    fbe_raid_geometry_get_mirror_prefered_position(raid_geometry_p, &prefered_mirror_pos_p);
    if (!fbe_raid_geometry_is_sparing_type(raid_geometry_p) &&
        (prefered_mirror_pos_p == FBE_MIRROR_PREFERED_POSITION_INVALID))
    {
        /* Check and apply read optimization
         */
        status = fbe_mirror_read_set_read_optimization(siots_p);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: %s failed in setting read optimization: siots_p = 0x%p, status = %dn",
                                                __FUNCTION__, siots_p, status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }
        /* mirror read optimization*/
        fbe_mirror_nway_mirror_optimize_fruts_start(siots_p, read_fruts_p);
    }

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    /* Always transitions BEFORE send the I/Os.  Now send the read fruts
     * chain.  The state status to waiting since we are waiting
     * for the I/Os to complete.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_read_state4);
    b_result = fbe_raid_fruts_send(read_fruts_p, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* Need to clear this so we do not wait for the generate which will never occur.
         */
        if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: %s failed to start read fruts chain 0x%p for siots_p 0x%p\n", 
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p);

        /* We might be here as we might have intentionally failed this 
         * condition while testing for error-path. In that case,
         * we will have to wait for completion of  fruts as it has
         * been already started.
         */
        return ((b_result) ? (FBE_RAID_STATE_STATUS_WAITING) : (FBE_RAID_STATE_STATUS_EXECUTING));
    }

    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return(state_status);
}                               
/* end of fbe_mirror_read_state3() */

/*!***************************************************************
 *          fbe_mirror_read_state4()
 ****************************************************************
 * @brief
 *      Reads have completed, check status.
 *
 * @param
 *      siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns 
 *      FBE_RAID_STATE_STATUS_WAITING if need to wait for cxts, dh read or disk dead;
 *      FBE_RAID_STATE_STATUS_EXECUTING otherwise.
 *
 * HISTORY:
 *      08/22/00  RG - Created.
 *
 * @author
 *  05/21/2009  Ron Proulx  - Updated based on mirror_rd_state4
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state4(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
   
    /* Since we haven't modified the media we need to check for aborted.
     * Get fruts error will check if this siots have been aborted.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Initialize out local eboard.
     */
    fbe_raid_fru_eboard_init(&fru_eboard);
    
    /* Validate that all fruts have completed.
     */
    if (siots_p->wait_count != 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: wait count unexpected 0x%llx \n", 
                                            siots_p->wait_count);
        return(state_status);
    }

    /*! @note We must cleanup the optimize BEFORE getting the fruts error 
     *        since we may change the read position.
     */
    fbe_mirror_optimize_fruts_finish(siots_p, read_fruts_p);

    /* Reads have completed. Evaluate the status of the reads.
     */
    fru_status = fbe_mirror_get_fruts_error(read_fruts_p, &fru_eboard); 
    
    /* Based on the fruts status we transition to the proper state.
     */
    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Success.  Transition to next state to check data.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_read_state5);
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_mirror_handle_fruts_error(siots_p, read_fruts_p, &fru_eboard, fru_status);
        return state_status;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_status_t   raid_status;

        /*! Invoke the method that will check and process the eboard situation
         *  for a read request.  If the status is success then we can transition
         *  to the next state which will evaluate the read data.
         *
         *  @note We purposefully don't transition in process fruts error so that
         *        all state transitions occur directly from the state machine.
         */
        raid_status = fbe_mirror_read_process_fruts_error(siots_p, &fru_eboard);

		/*let's see if we need to increment any error counts*/
		fbe_raid_fruts_increment_eboard_counters(&fru_eboard, read_fruts_p);

        switch(raid_status)
        {
            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* A retry is possible using the read data from a different
                 * position.  Goto state that will setup to continue.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_read_state6);

                /*! @note This is here for diagnostic purposes to see if this ever happens. 
                 */
                if ((fru_eboard.hard_media_err_count + fru_eboard.dead_err_count) == fbe_raid_siots_get_width(siots_p))
                {
                    fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: read unexpected raid_status: 0x%x from read process error\n",
                                                    raid_status);
                }
                break;

            case FBE_RAID_STATUS_MINING_REQUIRED:
                /* Enter read recovery verify.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_read_state7);
                break;

            case FBE_RAID_STATUS_TOO_MANY_DEAD:
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
                break;

            case FBE_RAID_STATUS_UNSUPPORTED_CONDITION:
            case FBE_RAID_STATUS_UNEXPECTED_CONDITION:
                /* Some unexpected or unsupported condition occurred.
                 * Transition to unexpected.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: read unexpected raid_status: 0x%x from read process error\n",
                                                    raid_status);
                break;

            case FBE_RAID_STATUS_OK:
                /* Since we only read from (1) position and an error was detected,
                 * OK isn't a valid status.
                 */
            default:
                /* Process fruts error reported an un-supported status.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: read unexpected raid_status: 0x%x from read process error\n",
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: rd state4 pos: %d lba: 0x%llx bl: 0x%llx media cnt: %d mask: 0x%x raid_status: %d\n",
                                    read_fruts_p->position, siots_p->start_lba, siots_p->xfer_count,
                                    siots_p->media_error_count, siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap, raid_status);

    } /* END else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR) */
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read siots lba: 0x%llx blks: 0x%llx unexpected fru status: 0x%x\n",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_read_state4() */

/*!***************************************************************************
 *          fbe_mirror_read_state5()
 *****************************************************************************
 *
 * @brief   Read has completed without error.  Check the checksum/metadata.  
 *          Since the XOR is done in-line the default state status is:
 *          FBE_RAID_STATE_STATUS_EXECUTING.
 *          
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  09/10/2009  Ron Proulx - Created from mirror_rd_state5
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state5(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_xor_status_t            xor_status = FBE_XOR_STATUS_INVALID;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t hard_media_err_bitmask;
    fbe_bool_t                  validate;
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* First check and handle the case where we have switched positions due to
     * hard media errors.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    hard_media_err_bitmask = siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
    validate = fbe_raid_siots_is_validation_enabled(siots_p);
    if (hard_media_err_bitmask != 0)
    {
        /* Check if there are any hard errors (i.e. uncorrectable media errors) 
         * for a different position that should be changed to correctable errors.
         */
        if ( (status = fbe_raid_siots_convert_hard_media_errors(siots_p,
                                                     read_fruts_p->position))
                                                     != FBE_STATUS_OK )
        {
            /* This is unexpected.  It means that we completed a read to a hard
             * errored position.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: read hard error on read position: 0x%x - status: 0x%x\n", 
                                                read_fruts_p->position, status);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }

        /* Normally verify or rebuild would generate an XOR error region for
         * correctable errors.  But mirrors can generate recoverable crc
         * errors by using redundancy (i.e. reading from a different position).
         * Since the logical error injection code expects that a XOR error
         * region, create one if logical error injection is enabled.
         */
        if (validate == FBE_TRUE)
        {
            fbe_xor_lib_mirror_add_error_region(&siots_p->misc_ts.cxts.eboard,
                                            &siots_p->vcts_ptr->error_regions,
                                            siots_p->parity_start,
                                            siots_p->parity_count,
                                            width);
        }
    }
    
    /* If this is a sparing raid group we do not own the metadata.  Therefore
     * don't check the metadata (including the checksum) and simply complete 
     * the request. 
     */
    if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
    {
        /* Simply set the xor status to success and skip teh checksum.
         */
        xor_status = FBE_XOR_STATUS_NO_ERROR;
    }
    else
    {
        /* Else check the checksum.
         */
        status = fbe_raid_xor_read_check_checksum(siots_p, FBE_FALSE, FBE_TRUE);
        if (status != FBE_STATUS_OK )
        {
            /* If the request to check checksum fails it means that the request
             * wasn't properly formed.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: read check stamps request failed - status: 0x%x\n", 
                                                status);
            return(state_status);
        }
    
        /* Get the status of the XOR request.
         */
        fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
    }
    
    /* Either a crc error was detected and retried or we needed to switch read
     * positions. In either case we need to report errors.
     */
    if ((hard_media_err_bitmask != 0)                ||
        (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)    )
    {
        fbe_raid_position_bitmask_t valid_bitmask;
        fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);

        /* Even if there was no XOR error on this request we must report
         * the errors of the original request.  We report errors for all
         * positions (the siots `retried' flag will force us down this path).
         */
        status = fbe_raid_util_get_valid_bitmask(width, &valid_bitmask);
        if(RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "status 0x%x != FBE_STATUS_OK, width 0x%x valid_bitmask 0x%x \n",
                                                status, width, valid_bitmask);
            return(state_status);
        }
        if ( fbe_raid_siots_record_errors(siots_p,
                                          width,
                                          &siots_p->misc_ts.cxts.eboard,
                                          valid_bitmask,    /* Bits to record for. */
                                          FBE_TRUE,         /* Allow correctable errors.  */
                                          validate          /* Validate as required */ )
                                                != FBE_RAID_STATE_STATUS_DONE )
        {
            /* We have detected an inconsistency on the XOR error validation.
             * Wait for the raid group object to process the error.
             */
            return(FBE_RAID_STATE_STATUS_WAITING);
        }

        /* If we have converted a position swap to a correctable CRC (i.e.
         * corrected with redundancy) error clear the bits in the eboard.
         */
        fbe_raid_siots_clear_converted_hard_media_errors(siots_p, hard_media_err_bitmask);
    }

    /* Now process the XOR request status.
     */
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* We successfully read the data.
         * If any errors were encountered along the way they will be 
         * crc errors, so mark them correctable now in the vrts.
         */
        siots_p->vrts_ptr->eboard.c_crc_bitmap = siots_p->vrts_ptr->eboard.u_crc_bitmap;
        siots_p->vrts_ptr->eboard.u_crc_bitmap = 0;

        /* Retried request succeeded without error.  Set status and transition
         * to common state to complete request.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        
        /* Update the error counters return to the client object.
         */
        fbe_mirror_report_errors(siots_p, FBE_TRUE);

        fbe_raid_siots_transition(siots_p, fbe_mirror_read_state20);

        /* After reporting errors, just send Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
         * to PDO for single or multibit CRC errors found
         */
        if (fbe_raid_siots_pdo_usurper_needed(siots_p))
        {
            fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
            /*We are waiting only when usurper has been sent, else executing should be returned*/
            if (send_crc_status == FBE_STATUS_OK)
            {
                return FBE_RAID_STATE_STATUS_WAITING;
            }
       }
    }
    else if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The request was aborted, transition to aborted state to
         * return failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
       /* If there was a checksum error enter read recovery verify which
         * can may recover the data from another position or even if there
         * is only (1) readable position will process the checksum errors.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_read_state7);
    }
    else
    {
        /* Else unexpected/unsupported xor_status.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: %s: line: %d Unexpected xor_status: %d\n", 
                                    __FUNCTION__, __LINE__, xor_status);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
    }
    

    /* Always return the state status.
     */
    return(state_status);

}                               
/* end of fbe_mirror_read_state5() */

/*!***************************************************************************
 *      fbe_mirror_read_state6()
 *****************************************************************************
 *
 * @brief   An error was detected either during the read of the XOR on the 
 *          primary position.  By the time this method is invoked we should
 *          have selected a new (i.e. non-degraded) position to read from.
 *          Re-initialize the fruts (this is the same fruts that was used to
 *          read from the primary position) and send the read.  Since this
 *          state issues I/O the default state state is 
 *          FBE_RAID_STATE_STATUS_WAITING.
 *
 * @param   siots_p, [IO] - ptr to the SIOTS
 *
 * @return
 *  This function returns 
 *  FBE_RAID_STATE_STATUS_WAITING if we retry from the alternate drive;
 *  FBE_RAID_STATE_STATUS_EXECUTING if we exit with hard error.
 * 
 * @note
 *  08/22/00  RG - Created.
 *  10/03/00  RFoley - Fixed bug so we can correctly
 *                     determine if it's OK to retry read.
 *  03/20/01 MZ  - LU Centric support (multiple partitions).
 *  03/27/01 BJP - Added N-way mirror support
 *
 * @author
 *  09/10/2009  Ron Proulx  - Created from mirror_rd_state6
 * 
 ****************************************************************/

fbe_raid_state_status_t fbe_mirror_read_state6(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_u32_t                   read_fruts_count;
    fbe_bool_t b_result = FBE_TRUE;

    /* We just checked for the I/O being aborted so there is not need to check
     * here.
     */
    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* We need to re-evaluate our degraded positions before sending the request
     * since the raid group monitor may have updated our degraded status. 
     */  
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  FBE_FALSE, /* This is a read request */
                                                  FBE_FALSE  /* Don't write degraded positions */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* We shouldn't be here if we haven't setup the read fruts or
     * there is more than (1) fruts on the read chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    read_fruts_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);
    if ((read_fruts_p == NULL) ||
        (read_fruts_count > 1)    )
    {
        /* Shouldn't be here if we haven't setup the read position.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: unexpected read fruts count: 0x%x\n",
                                            read_fruts_count);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* It is possible that we got a media error on one position, and then went degraded on anther.
     * We need to fail the request now. 
     */
    if (read_fruts_count == 0)
    {
        fbe_raid_position_bitmask_t degraded_bitmask;
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "%s: 0x%x read_fruts_p : %p hme: 0x%x dead: 0x%x\n",
                                    __FUNCTION__, fbe_raid_memory_pointer_to_u32(siots_p), read_fruts_p,
                                    siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap, degraded_bitmask);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We shouldn't be in this state if this position is degraded, detected
     * a media error or an uncorrectable CRC error.  Report an unexpected 
     * error if this is the case.
     */
    if (fbe_raid_siots_is_position_degraded(siots_p, read_fruts_p->position)           ||
        ((1 << read_fruts_p->position) & siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap) ||
        ((1 << read_fruts_p->position) & siots_p->misc_ts.cxts.eboard.u_crc_bitmap)       )

    {
        /* We shouldn't have entered this state if the position isn't usable.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: position: 0x%x hard error: 0x%x uncorrectable crc: 0x%x unexpected \n",
                                            read_fruts_p->position, 
                                            siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap,
                                            siots_p->misc_ts.cxts.eboard.u_crc_bitmap);
        return(FBE_RAID_STATE_STATUS_EXECUTING);

    }

    /* Re-initialize the read fruts and send it.
     */
    status = fbe_raid_fruts_init_read_request(read_fruts_p,
                                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                siots_p->parity_start,
                                siots_p->parity_count,
                                read_fruts_p->position);
    if( RAID_COND(status != FBE_STATUS_OK) )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Fruts init request failed, fruts %p, status 0x%x\n",
                                            read_fruts_p, status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* The wait count is simply the number of items in the read chain
     */
    siots_p->wait_count = read_fruts_count;

    /* Transition to the state that will process the read completion.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_read_state4);

    /* Issue the read.
     */

    b_result = fbe_raid_fruts_send(read_fruts_p, fbe_raid_fruts_evaluate);
    if  (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: %s failed to start read fruts chain 0x%p for siots_p 0x%p\n", 
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p);
        
        /* We might be here as we might have intentionally failed this 
         * condition while testing for error-path. In that case,
         * we will have to wait for completion of  fruts as it has
         * been already started.
         */
        return ((b_result) ? (FBE_RAID_STATE_STATUS_WAITING) : (FBE_RAID_STATE_STATUS_EXECUTING));
    }

    /* Always return the state status.
     */
    return state_status;

}
/* end of fbe_mirror_read_state6() */

/*!***************************************************************************
 *          fbe_mirror_read_state7()
 *****************************************************************************
 *
 * @brief   We got a hard error or CSUM error, shift to read recovery verify.
 *          The default state status is FBE_RAID_STATE_STATUS_WAITING since
 *          we will enter the verify state machine.
 *          Note that this routine is also called for raw mirror reads which
 *          read from each mirror to compare sector data metadata.  We transition
 *          to the verify state machine accordingly.
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns
 *      FBE_RAID_STATE_STATUS_WAITING if we come back from recovery verify.
 *      FBE_RAID_STATE_STATUS_EXECUTING if retry from the other drive.
 * 
 * @note
 *      08/22/00  RG - Created.
 *
 * @author
 *  09/11/2009  Ron Proulx  - Created from mirror_rd_state7
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state7(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               full_access_count = fbe_raid_siots_get_full_access_count(siots_p);
    fbe_packet_priority_t   priority;
    fbe_bool_t              b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t        *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
 
    /* Need at least (1) position that is readable (i.e. not degraded) if
     * we can use recovery verify to get the data.
     */
    if (full_access_count < 1)
    {
        /* Too many degraded positions to attempt recovery verify.
         * This is unexpected.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* For errors that occur for a sparing object (i.e. errors that occur
     * when the virtual drive is in copy mode) we let the parent raid group 
     * recover (i.e. execute a verify and reconstruct).  Fail the request 
     * with `unrecoverable' error. 
     */
    if (fbe_raid_geometry_is_sparing_type(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
        return(FBE_RAID_STATE_STATUS_EXECUTING);    
    }

    fbe_raid_siots_get_priority(siots_p, &priority);
    if (priority == FBE_PACKET_PRIORITY_OPTIONAL)
    {
        /* Just in case this read is optional, 
         * set the priority to normal to avoid
         * sending down an optional request.
         * Since we are doing error handling, this can no longer
         * be optional for entrance to degraded verify.
         */        
        fbe_raid_siots_set_priority(siots_p, FBE_PACKET_PRIORITY_NORMAL);
    }

    /* Drop into single threaded mode for recovery verify
     */
    state_status = fbe_raid_siots_wait_previous(siots_p);
    if (state_status == FBE_RAID_STATE_STATUS_WAITING)
    {       
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* Determine if we need to generate another siots.
     */
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                                __FUNCTION__, siots_p, status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }
    }

    /* Enter the read recovery verify state machine.  When we get back from 
     * verify, continue with read state machine.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_read_state9);

    /* Transition to recovery verify to attempt to fix errors.
     */
    status = fbe_raid_siots_start_nested_siots(siots_p, fbe_mirror_recovery_verify_state0);

    /* If we weren't able to start the nested siots process the error.
     */
    switch(status)
    {
        case FBE_STATUS_OK:
            /* Simply wait for the nested siots to complete.
             */
            state_status = FBE_RAID_STATE_STATUS_WAITING;
            break;

        case FBE_STATUS_INSUFFICIENT_RESOURCES:
            /* Transition to this state and wait for allocation to
             * complete.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_read_state7);
            state_status = FBE_RAID_STATE_STATUS_WAITING;
            break;

        default:
            /* Unexpected status.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: unexpected status: 0x%x \n",
                                                status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
            break;
    }

    /* Kick off the next siots in the chain if necessary.
     */
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    /* Always return the state status.
     */
    return(state_status);

}                               
/* end of fbe_mirror_read_state7() */

/*!***************************************************************************
 *          fbe_mirror_read_state9()
 *****************************************************************************
 *
 * @brief   We are back from read recovery verify. We should have already 
 *          populated the siots status with either success for failure.  Thus
 *          simply complete the request.
 *
 * @param   siots_p, [IO] - ptr to the siots
 *
 * @return  state_status - This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @note    08/22/00  RG - Created.
 *
 * @author
 *  09/11/2009  Ron Proulx - Created from mirror_rd_state9
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state9(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;

    /* Validate that there are no nested siots remaining.
     */
    if (!fbe_queue_is_empty(&siots_p->nsiots_q))
    {
        /* Transition to unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: read with recovery - Nested siots still on queue. \n");
        return(state_status);
    }
    
    /* We should have populated the siots status and qualifier.  Transition
     * to commmon complete state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_read_state20);

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_read_state9() */

/*!***************************************************************
 *          fbe_mirror_read_state20()
 ****************************************************************
 *
 * @brief   The read has completed.  The siots status should be
 *          populated by this time.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  02/08/2010  Ron Proulx  - Created from fbe_mirror_verify_state20
 *  11/2011 Susan Rundbaken - Updated for raw mirror reads
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_read_state20(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_payload_block_operation_status_t    siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Validate that the status and qualifier have been populated
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);
    if ((siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)       ||
        (siots_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID)    )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots lba: 0x%llx blks: 0x%llx unexpected status: 0x%x or qualifier: 0x%x",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    siots_status, siots_qualifier);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Notice the order of the checks.  Although it isn't necessary to
     * transition to all the states below, it allows for debugging or 
     * special code for specific errors. 
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted, treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* For raw mirror reads, complete the siots.  Any correctable errors will be returned to the client so the
         * client can take corrective action.
         */
        if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
        {
            fbe_raid_siots_transition_finished(siots_p);
            /* Always return the state status.
             */
            return (state_status);
        }

        /* If success transition to success (yes this resets the status)
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* shutdown means too many drives are down to continue.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* media error, means we took an uncorrectable error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else
    {
        /* Else the status reported isn't supported
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: siots lba: 0x%llx blks: 0x%llx status: 0x%x not supported\n", 
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    siots_status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_read_state20() */


/******************************
 * end fbe_mirror_read.c
 ******************************/
