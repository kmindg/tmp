/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_degraded_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the RAID 5 degraded 
 *  read algorithm.
 *  This algorithm is selected only when the read falls into the degraded 
 *  region of a disk. 
 *
 *
 * @notes
 *
 * @author
 *  11/08/1999 JJIN Created.
 *
 ***************************************************************************/

/*************************
 **   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"


/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS 
 *****************************************************/

/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/
/*!***************************************************************
 *  fbe_parity_degraded_read_transition()
 ****************************************************************
 * @brief
 *   This function is called when we degraded from normal read.
 *   It will set up info in the nest subiots and 
 *   go to either degraded or strip-mined degraded path.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *   11/01/99 - Created. JJ
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_degraded_read_transition(fbe_raid_siots_t * siots_p)
{
#if 0
    /* This is an error path, so we always increment stats.
     */
    rg_inc_transition_stats(fbe_raid_siots_get_iots(siots_p)->group,
                            siots_p->algorithm);
#endif
    fbe_parity_degraded_read_transition_setup(siots_p);

    fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state0);
    
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_degraded_read_transition() */

/*!***************************************************************
 * fbe_parity_degraded_read_state0()
 ****************************************************************
 * @brief
 *  Allocates all resources for this siots.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  fbe_raid_state_status_t
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1]; /* One extra for terminator. */
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];

    if ((siots_p->algorithm != R5_DEG_RD) &&
        (siots_p->algorithm != R5_RD) &&
        (siots_p->algorithm != R5_SM_RD))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd algorithm %d unexpected.\n", 
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if ((fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == FBE_RAID_INVALID_DEAD_POS))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd algorithm %d dead pos %d unexpected.\n", 
                                            siots_p->algorithm, siots_p->dead_pos);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) >= fbe_raid_siots_get_width(siots_p))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd algorithm %d dead pos %d unexpected.\n", 
                                            siots_p->algorithm, siots_p->dead_pos);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_degraded_read_calculate_memory_size(siots_p, &read_info[0], &write_info[0], &num_pages);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd calculate memory status: 0x%x unexpected. siots: %p\n", 
                                            status, siots_p);
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
    fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state1);
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
        status = fbe_parity_degraded_read_setup_resources(siots_p, &read_info[0], &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_degraded_read_state3);
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
                                            "parity: deg rd memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_degraded_read_state0() */

/****************************************************************
 * fbe_parity_degraded_read_state1()
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

fbe_raid_state_status_t fbe_parity_degraded_read_state1(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1]; /* One extra for terminator. */
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];
    fbe_u16_t dead_pos_1, dead_pos_2;
    fbe_bool_t b_is_degraded;

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
                             "parity: deg rd memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return(state_status);
    }

    /* If we are not degraded, then go back to normal read.
     * Some drive may have gone away or come back, we need to update dead position first.
     */
    b_is_degraded = fbe_parity_siots_is_degraded(siots_p);

    /* Either we are not degraded at all or the only dead position(s) are parity drives.
     */
    fbe_parity_degraded_read_get_dead_data_positions(siots_p, &dead_pos_1, &dead_pos_2);
    if (!b_is_degraded || (dead_pos_1 == FBE_RAID_INVALID_DEAD_POS))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "drd state1 no dead positions. lba: 0x%llx bl: 0x%llx nested: %s pos1: %d\n",
                                    siots_p->start_lba, siots_p->xfer_count,
                                    (fbe_raid_siots_is_nested(siots_p) ? "yes" : "no"), dead_pos_1);

        if (fbe_raid_siots_is_nested(siots_p))
        {
            /* Nested siots return a special error backup to the parent to cause the 
             * parent to start the read over again. 
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        }
        else
        {
            /* Not nested, just kick this back to read.
             */
            siots_p->algorithm = R5_RD;
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state0);
            fbe_raid_siots_reuse(siots_p);
        }
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate all the memory needed for this siots.
     */
    status = fbe_parity_degraded_read_calculate_memory_size(siots_p, &read_info[0], &write_info[0], &num_pages);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd calculate memory status: 0x%x unexpected. siots: %p\n", 
                                            status, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_degraded_read_setup_resources(siots_p, &read_info[0], &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: deg rd to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/****************************************
 * end fbe_parity_degraded_read_state1()
 ***************************************/

/*!***************************************************************
 * fbe_parity_degraded_read_state3()
 ****************************************************************
 * @brief
 *  Kick off the reads to the disk.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state3(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u32_t dead_pos2, dead_pos;

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (siots_p->algorithm == R5_DEG_RD)
    {
        fbe_status_t status;

        /* Determine if we need to generate another siots.
         * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
         * Typically this call is done after we have started the disk I/Os.
         */
        status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                                __FUNCTION__, siots_p, status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }
    }

    /* make sure the first or second dead position didn't change */
    dead_pos2 = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
    dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);

    /* If we are not degraded, then go back to normal read.
     * We need to update the dead position first.  Drive might have come back/gone away
     * while IO was quiesced.
     */
    if (fbe_parity_siots_is_degraded(siots_p))
    {
        /* If the original dead drive has changed, we need to set up the siot again. */
        if ((fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != dead_pos) ||
            (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != dead_pos2))
        {
            fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state0);
            fbe_raid_siots_reuse(siots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "drd state3 no dead positions. lba: 0x%llx bl: 0x%llx nested: %s\n",
                                    (unsigned long long)siots_p->start_lba,
				    (unsigned long long)siots_p->xfer_count,
                                    (fbe_raid_siots_is_nested(siots_p) ? "yes" : "no"));

        if (fbe_raid_siots_is_nested(siots_p))
        {
            /* Nested siots return a special error backup to the parent to cause the 
             * parent to start the read over again. 
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        }
        else
        {
            /* Not nested, just kick this back to read.
             */
            siots_p->algorithm = R5_RD;
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state0);
            fbe_raid_siots_reuse(siots_p);
        }
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state4);

    /* Issue reads to disks.
     */
    siots_p->wait_count = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_dead_pos_count(siots_p);

    if (RAID_COND(siots_p->wait_count != fbe_raid_fruts_get_chain_bitmap(read_fruts_ptr, NULL)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity drd wait count %lld != fruts chain count %d\n",
                                             siots_p->wait_count, 
                                            fbe_raid_fruts_get_chain_bitmap(read_fruts_ptr, NULL));
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts. Since we are not sure how many fruts
         * have been sent successfully, we can not move to completion process of siots 
         * immediately.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to start read fruts chain for "
                                            "siots = 0x%p, b_result = %d\n",
                                            __FUNCTION__, siots_p, b_result);
    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/* end of fbe_parity_degraded_read_state3() */

/*!***************************************************************
 * fbe_parity_degraded_read_state4()
 ****************************************************************
 * @brief
 *  This function will
 *      1. evaluate the reads from disks;
 *      2. reconstruct degraded regions for host data;
 *      3. check checksums of extra host data, if necessary.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  10/28/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    fbe_raid_fru_eboard_init(&eboard);

    /* We have read all the data;
     * evaluate them first.
     */
    status = fbe_parity_get_fruts_error(read_fruts_ptr, &eboard);
    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* No errors, continue
         */
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
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_ptr);
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_ptr, &eboard, status);
        return state_status;
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_ABORTED))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_ptr, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* See if we have any hard errors and we need to retry.
         */
        if (eboard.hard_media_err_count > 0)
        {
            /* Validate hard media error count.
             */
            if (eboard.hard_media_err_count > fbe_raid_siots_get_width(siots_p))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: hard err count %d unexpected\n", 
                                                    eboard.hard_media_err_count);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* For RAID-6, if we get any media errors, we need to transition
             * directly to recovery verify to possibly perform strip mining.
             * For RAID-5 if this was a degraded read (i.e. when I/O was started 
             * the raid group was already degraded and therefore the request is 
             * not nested) also transition to verify so that we populate the 
             * read data. 
             */
            if ((parity_drives > 1)               ||
                (siots_p->algorithm == R5_DEG_RD)    )
            {
                /* Even though we cannot recover the data for RAID-5, we must go 
                 * into verify so that we return the correct, invalidated data. 
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state13);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
            else
        {
                /* Else if we transitioned to degraded read we will issue the
                 * verify after the degraded read returns.  Report the errors 
                 * and return error status.
             */
             fbe_parity_report_media_errors(siots_p, read_fruts_ptr, FBE_FALSE /* Not correctable. */);
             fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
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
               return FBE_RAID_STATE_STATUS_EXECUTING;
            }
        }
        else if (eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dropped_error);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else if (eboard.dead_err_count > 0)
        {
            /* When we start a degraded read it is because we know at least
             * one drive is dead.
             * On R5 it's not possible to get another dead error, since
             * we should go shutdown.
             * On R6 we can get another dead error and still continue.
             */
            if ((parity_drives > 1) && (eboard.dead_err_count < parity_drives))
            {
                /* Try to recover from this dead error on R6.
                 * We started with 1 degraded and saw another drive die.
                 * Since the degraded range might be different for the 2nd
                 * drive than the first drive, we need to use degraded verify
                 * to do the reconstruct, since we might not have
                 * allocated sufficient buffers as part of the reconstruct
                 * for the first dead drive.
                 */
                fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state13);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            else
            {
                /* This isn't possible since we should have become shutdown.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: %s parity drives: %d dead errors: %d\n", 
                                                    __FUNCTION__, parity_drives, eboard.dead_err_count);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
        } /* End else dead error count > 0. */
        else
        {
            /* This status is not expected.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                  "parity: %s fru error status unknown 0x%x\n", 
                                  __FUNCTION__, status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else
    {
        /* If the status was WAITING and this is a raid 5,
         * then this isn't possible since we should have received a shutdown.
         *
         * If the status is something else it is completely unexpected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru unexpected status: 0x%x\n", 
                                            __FUNCTION__, status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* All the reads are successful,
     * begin reconstruct.
     */
    {
        fbe_raid_state_status_t state_status;

        /* Refresh the degraded state of this raid group.
         */
        fbe_parity_siots_is_degraded(siots_p);

        /* We always continue when we are degraded.
         * Make the opcode of any degraded positions be nop. 
         * This is needed since we might be retrying fruts below. 
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_ptr);

        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state5);

        /* Initialize the hard error bitmap now.
         */
        (void) fbe_raid_fruts_get_media_err_bitmap(
            read_fruts_ptr,
            &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

        state_status = fbe_parity_degraded_read_reconstruct(siots_p);
        return (state_status);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;

}
/* end of fbe_parity_degraded_read_state4() */

/*!***************************************************************
 * fbe_parity_degraded_read_state5()
 ****************************************************************
 * @brief
 *  This function will process the reconstruction
 *  when the XOR driver has responded with a completion. 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  04/25/00 - Created. MPG
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state5(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_u16_t degraded_bitmask;

    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Validate the write bitmap.
     */
    if ((siots_p->misc_ts.cxts.eboard.w_bitmap == 0) ||
        ((siots_p->misc_ts.cxts.eboard.w_bitmap & degraded_bitmask) != degraded_bitmask))
    {
        /* The reconstruct did not modify (rebuild)
         * the dead position.
         */
        /* XOR detected bad stamps or bad checksum on host data.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "parity: %s dead pos not modified: w_bitmap 0x%x d_bitmap:0x%x\n", 
                              __FUNCTION__, siots_p->misc_ts.cxts.eboard.w_bitmap,
                              degraded_bitmask);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if ( !fbe_raid_csum_handle_csum_error( siots_p, 
                                           read_fruts_p, 
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           fbe_parity_degraded_read_state4 ) )
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    /* Record errors detected by the XOR operation.
     * For R5/R3 we expect either uncorrectables, or retried crc errors.
     * For R6, we could have correctables too.
     */
    if ( siots_p->misc_ts.cxts.eboard.u_bitmap || siots_p->vrts_ptr->retried_crc_bitmask ||
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

    if (siots_p->misc_ts.cxts.eboard.u_bitmap || siots_p->misc_ts.cxts.eboard.m_bitmap)
    {
        /* If any errors were found, mark that we need to verify this strip outside the I/O path.
         * Reads cannot write out corrections since they do not have a write lock. 
         */
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
    }

    /* Transition to the next state regardless.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state9);
    
    return (status);
}
/* end of fbe_parity_degraded_read_state5() */

/*!***************************************************************
 * fbe_parity_degraded_read_state9()
 ****************************************************************
 * @brief
 *  This function checks the status of the reconstruct.            
 *
 * @param siots_p - ptr to the current fbe_raid_siots_t.
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING - if the xor of host data was started
 *  FBE_RAID_STATE_STATUS_EXECUTING - if no errors.
 *
 * @author
 *  04/14/00 - Created. MPG
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state9(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t raid_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t degraded_bitmask;

    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

    /* Check status of rebuild.
     * We encountered no unexpected errors if there were no uncorrectables or
     * modification to anything other than parity or degraded positions.
     */
    if ((siots_p->misc_ts.cxts.eboard.u_bitmap == 0) &&
        ((siots_p->misc_ts.cxts.eboard.w_bitmap &
          ~( degraded_bitmask | fbe_raid_siots_parity_bitmask(siots_p)) ) == 0) )
    {
        /* No uncorrectable errors, and no
         * modification to anything other than parity or deg positions.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state20);
    }
    else
    {
        /* We took an error in something other than parity or data.
         * The reconstruct cannot determine if the error is in the
         * data being requested, so we will checksum the data now
         * in an effort to detect uncorrectables.
         * If there is an uncorrectable in the data being requested
         * we will return an error to the caller.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state12);

        status = fbe_raid_xor_check_siots_checksum(siots_p);

        if (status != FBE_STATUS_OK) 
        {  
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s unexpected fbe_raid_xor_check_iots_checksum  status: 0x%x\n", 
                                                __FUNCTION__, status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }

    return raid_status;
}
/* end fbe_parity_degraded_read_state9() */

/*!***************************************************************
 * fbe_parity_degraded_read_state12()
 ****************************************************************
 * @brief
 *  This function will process the status of the checksum 
 *  operation on host/cache data.
 *
 * @param siots_p - Ptr to the fbe_raid_siots_t.
 *
 * @return
 *  FBE_RAID_STATE_STATUS_EXECUTING - We always go to the next state.
 *
 * @author
 *  05/19/00 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state12(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_xor_status_t xor_status;

    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* The host/cache data is good,
         * ignore the invalid backfill data,
         * continue with good status.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state20);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Nothing we can do to fix the error.
         * We already made our best attempt at fixing the error
         * during the reconstruct.
         * Report the errors and return status.
         */
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
        fbe_parity_report_media_errors(siots_p, read_fruts_p, FBE_FALSE /* Not correctable. */);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
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
    }
    else
    {
        /* XOR detected bad stamps or bad checksum on host data.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s unexpected xor status: 0x%x\n", 
                                            __FUNCTION__, xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_degraded_read_state12() */

/*!***************************************************************
 * fbe_parity_degraded_read_state13()
 ****************************************************************
 * @brief
 *  This function will transition us to degraded verify.
 *
 *  Before we can transition to degraded verify, we must
 *  get a write lock if we don't already have one.
 *  A write lock will allow the verify state machine to 
 *  write out any corrections that are due to stamp/crc errs.
 *
 * @param siots_p - Ptr to the fbe_raid_siots_t.
 *
 * @return
 *
 * @author
 *  04/27/06 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_degraded_read_state13(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_INVALID; 

    fbe_packet_priority_t priority; 
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

    /* If there is a another SIOTS in flight, we will wait for it to complete.
     * We are intentionally putting this thread into single thread mode,
     * since the other siots might be handling errors just like us.
     * To avoid two verifies running, we just wait until the peer 
     * is complete.  Mark ourselves as waiting for a SIOTS lock.
     */
    status = fbe_raid_siots_wait_previous(siots_p);
    if (status == FBE_RAID_STATE_STATUS_EXECUTING)
    {
        /* When we get back from verify, continue with drd read state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state16);

        /* Just set that we have a read error to
         * force siots pipelining to single threaded.
         */
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_recovery_verify_state0);
        if (RAID_GENERATE_COND(fbe_status  != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to start nested siots 0x%p, "
                                                "status = 0x%x\n",
                                                __FUNCTION__,
                                                siots_p,
                                                fbe_status);
            return (FBE_RAID_STATE_STATUS_EXECUTING); 
        }
        status = FBE_RAID_STATE_STATUS_WAITING;
    }
    return status;
}
/* end fbe_parity_degraded_read_state13() */

/*!***************************************************************
 * fbe_parity_degraded_read_state16()
 ****************************************************************
 * @brief
 *  This function will process the status from degraded verify.
 *  If degraded verify is finished then we should have the
 *  reconstructed data already.
 *
 * @param siots_p - Ptr to the fbe_raid_siots_t.
 *
 * @return
 *
 * @author
 *  04/27/06 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_raid_state_status_t fbe_parity_degraded_read_state16(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;

    if (fbe_queue_is_empty(&siots_p->nsiots_q) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: nested siots found unexpectedly\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Return from verify and recovery.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* This status means that the verify state machine
         * already checked the checksums of the host read data.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state20);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* Verify has checksumed data and found
         * invalid sectors.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY ||
             siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED )
    {
        /* LUN is shutdown.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else
    {
        /* Not able to retry since this is not a crc error.
         * Return error status to host.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s unexpected error for siots_p = 0x%p, siots_p->error = 0x%x\n", 
                                            __FUNCTION__, 
                                            siots_p,
                                            siots_p->error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/* end fbe_parity_degraded_read_state16() */
/*!***************************************************************
 * fbe_parity_degraded_read_state20()
 ****************************************************************
 * @brief
 *  This function will return good status.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  10/28/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_state20(fbe_raid_siots_t * siots_p)
{
#if 0
    r5_vr_report_errors(siots_p);
#endif

    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_degraded_read_state20() */

/*****************************************
 * fbe_parity_degraded_read.c
 *****************************************/
