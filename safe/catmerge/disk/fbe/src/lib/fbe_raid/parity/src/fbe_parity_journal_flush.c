/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_parity_journal_flush.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for write journal flushing.
 * 
 * @author
 *   1/10/2011:  Created. Daniel Cummins
 *   2/28/2012: Vamsi V: Modified to stop using metadata service for journal_slot 
 *              info and instead rely on Header written to Journal slot as part of
 *              journal write operation.  
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_raid_library.h"
#include "fbe_raid_error.h"
/******************************************************************************
 * fbe_parity_write_log_flush_state0000()
 ******************************************************************************
 * @brief
 *  Start of write log flush state machine. Start by allocating iots memory for
 *  passing the common valid slot header from the header read to the data read phase.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *      to allocate memory immediately; otherwise, FBE_RAID_STATE_STATUS_EXECUTING 
 *      is returned.
 *
 * @author
 *  6/29/2012 - Created. Dave Agans
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state0000(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_RAID_STATE_STATUS_INVALID;

    /* Validate the algorithm.
     */
    if (!(siots_p->algorithm == RG_FLUSH_JOURNAL))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: %s unexpected algorithm: %d, expected %d, siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p->algorithm, RG_FLUSH_JOURNAL, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Validate the siots has been properly populated
     */
    if (fbe_parity_generate_validate_siots(siots_p) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: %s: validate failed, siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* This state machine assumes the eboard is valid.
     */
    fbe_xor_lib_eboard_init(&siots_p->misc_ts.cxts.eboard);

    /* Now allocate a single page for a copy of a valid header to span the destroy re-allocate
     * of the siots between the header read and the data read. Note that
     * must transition to state0001 since the callback can be invoked at any time. 
     * There are (3) possible status returned by allocate: 
     *  o FBE_STATUS_OK - Allocation completed immediately (never transition to state0001)
     *  o FBE_STATUS_PENDING - Allocation didn't complete immediately
     *  o Other - Memory allocation error
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state0001);

    /* now allocate all the memory we need for this flush operation
     */
    status = fbe_raid_siots_allocate_iots_buffer(siots_p);

    /* Check if the allocation completed immediately
     * If not we will transition to `state001' when the memory is available so 
     * return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately.
         * We don't plant this resource until we need it.
         * Transition to next state (allocate siots memory)
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state000);

        return FBE_RAID_STATE_STATUS_EXECUTING;
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
                                            "journal flush: iots header buffer memory request failed with status: 0x%x\n",
                                            status);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state0000()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state0001()
 ******************************************************************************
 * @brief
 *  Memory has been allocated. We now have to move on to allocate siots memory
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if siots need to quiesce;
 *      otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  6/29/2012 - Created. Dave Agans
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state0001(fbe_raid_siots_t * siots_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* First check if this request was aborted while we were waiting for
     * resources.  If so simply transition to the aborted handling state.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Now check for allocation failure
     */
    if (RAID_MEMORY_COND(!fbe_raid_iots_is_deferred_allocation_successful(iots_p)))
    {
        /* Trace some information and transition to allocation error
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "journal flush: iots buffer memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&iots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Deferred allocation completed immediately.
     * We don't plant this resource until we need it.
     * Transition to next state (allocate siots memory)
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state000);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state0001()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state000()
 ******************************************************************************
 * @brief
 *  Start of write log flush state machine. Start by allocating memory for read
 *  of slot headers from all the drives.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *      to allocate memory immediately; otherwise, FBE_RAID_STATE_STATUS_EXECUTING 
 *      is returned.
 *
 * @author
 *  2/28/2012 - Created. Vamsi V
 *  6/20/2012 - Change to allocate header read buffers dynamically. Dave Agans
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state000(fbe_raid_siots_t * siots_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_status_t status = FBE_RAID_STATE_STATUS_INVALID;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t journal_log_hdr_blocks;

    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    /* Validate the algorithm.
     */
    if (!((siots_p->algorithm == RG_FLUSH_JOURNAL) || (siots_p->algorithm == RG_WRITE_LOG_HDR_RD)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: %s unexpected algorithm: %d, expected %d, siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p->algorithm, RG_FLUSH_JOURNAL, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Validate the siots has been properly populated
     */
    if (fbe_parity_generate_validate_siots(siots_p) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: %s: validate failed, siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Figure out how many drive operations we will need.
     * We subtract one for each dead drive because we won't be reading them
     */
    siots_p->drive_operations = raid_geometry_p->width
        - ((siots_p->dead_pos != FBE_RAID_INVALID_DEAD_POS) ? 1 : 0)
        - ((siots_p->dead_pos_2 != FBE_RAID_INVALID_DEAD_POS) ? 1 : 0);

    /* Setup SG and buffer count. We need one SG element and one header buffer per drive.
     */
    num_sgs[FBE_RAID_SG_INDEX_SG_8] = siots_p->drive_operations; 
    siots_p->total_blocks_to_allocate = siots_p->drive_operations * journal_log_hdr_blocks;

    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         siots_p->drive_operations, 
                                         siots_p->total_blocks_to_allocate);

    status = fbe_raid_siots_calculate_num_pages(siots_p, siots_p->drive_operations,
                                                &num_sgs[0],
                                                FBE_FALSE);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal_flush: failed to calculate number of pages: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;

    }

    /* Now allocate all the pages required for this read request.  Note that
     * must transition to state1 since the callback can be invoked at any time. 
     * There are (3) possible status returned by allocate: 
     *  o FBE_STATUS_OK - Allocation completed immediately (never transition to state001)
     *  o FBE_STATUS_PENDING - Allocation didn't complete immediately
     *  o Other - Memory allocation error
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state001);

    /* now allocate all the memory we need for this flush operation
     */
    status = fbe_raid_siots_allocate_memory(siots_p);

    /* Check if the allocation completed immediately.  If so plant the resources
     * now.  Else we will transition to `state001' when the memory is available so 
     * return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately.  Plant the resources and then
         * transition to next state.
         */
        status = fbe_parity_journal_header_read_setup_resources(siots_p);

        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "journal flush: rebuild failed to setup resources. status: 0x%x\n",
                                                status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* issue the header read
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state002);

        return FBE_RAID_STATE_STATUS_EXECUTING;
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
                                            "journal flush: header read memory request failed with status: 0x%x\n",
                                            status);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state000()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state001()
 ******************************************************************************
 * @brief
 *  Memory has been allocated . We now have to setup resources to read in slot 
 * headers. 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if siots need to quiesce;
 *      otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  2/28/2012 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state001(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;

    /* First check if this request was aborted while we were waiting for
     * resources.  If so simply transition to the aborted handling state.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Now check for allocation failure
     */
    if (RAID_MEMORY_COND(!fbe_raid_siots_is_deferred_allocation_successful(siots_p)))
    {
        /* Trace some information and transition to allocation error
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "journal flush: memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Deferred allocation completed immediately.  Plant the resources and
     * transition to next state.
     */
    status = fbe_parity_journal_header_read_setup_resources(siots_p);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* issue slot header reads */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state002);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state001()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state002()
 ******************************************************************************
 * @brief
 *  In this state we start header reads from all non-degraded drives.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING if we are not going to do 
 *  header reads. FBE_RAID_STATE_STATUS_WAITING is returned if we are going to do
 *  the reads.
 *
 * @author
 *  2/248/2012 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state002(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_result = FBE_TRUE;
    fbe_raid_fruts_t * read_fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* do not send any operations to drives that are dead
     */
    fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

    /* Initialize the number of fruts we will be waiting for completion on */
    if ((siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p)) == 0)
    {
        /* No positions to read header from */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "write_log flush: No positions to read slot header.\n");

        /* Fail the write_log flush request */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* set the next state */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state003);

    /* Initiate the header reads */
    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);

    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts. 
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: %s failed to start header read fruts chain for "
                                            "siots_p 0x%p, b_result = 0x%x\n",
                                            __FUNCTION__,
                                            siots_p,
                                            b_result);
    }

    /* The reads were started; 
     * transition to the wait for header read state.
     */
    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state002()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state003()
 ******************************************************************************
 * @brief
 *  We completed the read of slot headers. Now validate the data in headers.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *      to allocate the required number of structures immediately;
 *      otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  2/28/2012 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state003(fbe_raid_siots_t * siots_p)
{   
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    fbe_raid_fru_eboard_init(&eboard);

    /* write_log header reads have completed. Evaluate the status of the reads.
     */
    status = fbe_parity_get_fruts_error(read_fruts_ptr, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All header reads are successful. Check lba stamps and checksums.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state004);
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             fbe_raid_siots_is_aborted(siots_p))
    {
        /* We check for aborted directly in siots since we are at the read phase and 
         * regardless of the operation we should abort the request. 
         */
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_ptr, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_status_t retry_status;

         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_ptr);

        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        retry_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, read_fruts_ptr);
        if (retry_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log flush: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        if ((eboard.dead_err_count > 0) || (eboard.hard_media_err_count > 0)) 
        {   
            /* Some Header reads failed due to dead drives or hard media errors.
             * Remove fruts that failed from read_fruts_chain and temporarily save 
             * them on read2_fruts_chain. For rest of the fruts in read_fruts_chain
             * check lba stamps and checksums.
             */
            fbe_parity_write_log_remove_failed_read_fruts(siots_p, &eboard);

            fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

            if(read_fruts_ptr != NULL)
            {
                /* At least one header read completed successfully. We can use it to generate 
                 * headers for failed positions because headers are all mirror copies.
                 * But first check if stamps are ok on successful header reads.
                 */
                fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state004);
            }
            else
            {
                /* We could not even read-in a single header successfully. We have to abandon 
                 * this flush operation. Since we are abandoning flush, Live stripe could be left
                 * with incomplete writes which would cause blocks on rebuilding drive  
                 * (for which write/time stamp does not match) to be invalidated.
                 */
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                     "write_log flush: Abandon flush due to header read errors: slot_id 0x%x, dead_bitmap 0x%x, me_bitmap 0x%x \n",
                                     siots_p->journal_slot_id, eboard.dead_err_bitmap, eboard.hard_media_err_bitmap);

                /* No need to send flush_abandoned event here, we will do that in state005 when we validate headers
                 *   with no good headers and many bad ones.
                 */
                fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state005);
            }
        }
        else if(eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                  "write_log flush: %s fru dropped error unexpected "
                                  "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                  __FUNCTION__, eboard.drop_err_count, eboard.drop_err_bitmap);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        }
        else
        {
            /* This status is not expected.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log flush: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);
        }
    }
    else
    {
        /* This status was not expected.  Instead of panicking, just
         * return the error. 
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: %s status unknown 0x%x\n", 
                                            __FUNCTION__, status);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state003()
 ******************************************************************************/

/****************************************************************
 * fbe_parity_write_log_flush_state004()
 ****************************************************************
 * @brief
 *  All reads are successful, check the checksums.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  2/28/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state004(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;

    /* Check xsums/lba stamps.
     */
    status = fbe_raid_xor_read_check_checksum(siots_p, FBE_TRUE, FBE_TRUE);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: xor check status unexpected 0x%x\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if ( xor_status == FBE_XOR_STATUS_NO_ERROR && !fbe_raid_siots_is_retry(siots_p) )
    {
        /* No checksum error, and we are done.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state005);
    }
    else
    {
        /* Error or retry in progress.
         * When retry is in progress we go down the
         * error path to log status of retry.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state008);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state004()
 ******************************************************************************/

/****************************************************************
 * fbe_parity_write_log_flush_state005()
 ****************************************************************
 * @brief
 *  CRC on slot headers is good. Validate write_log fields 
 *  within the headers.  
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  2/28/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state005(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_flush;
    fbe_bool_t b_inv;

    /* Sanity check the contents of each header and validate them across headers (one per disk).
     * Based on number of valid headers determine number of drives we need to flush. 
     */
    if (fbe_parity_write_log_validate_headers(siots_p, &siots_p->drive_operations, &b_flush, &b_inv) != FBE_STATUS_OK)
    {
        /* We hit an issue while starting fruts chain.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "%s failed to validate slot headers. siots_p 0x%p\n", 
                                            __FUNCTION__,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Set next state to transition.
     */
    if (b_flush)
    {
        /* Sanity check */
        if (siots_p->drive_operations == 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "%s write_log: Flush with zero drive ops. siots_p 0x%p\n", 
                                                __FUNCTION__,
                                                siots_p);
        }

        if(siots_p->algorithm == RG_WRITE_LOG_HDR_RD)
        {
            /* We completed header read. Complete back to Object which will read in chunk-info for the
             * live stripe and resubmit the flush request.
             */
            fbe_raid_iots_set_flag(fbe_raid_siots_get_iots(siots_p), FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED);
            fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_completed);
        }
        else
        {
            /* Transition to start flush process */
            fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state006);
        }
    }
    else if (b_inv)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO, 
                                    "%s Header read done. Flush not required. Invalidating 0x%x headers. slot_id 0x%x\n", 
                                    __FUNCTION__,
                                    siots_p->drive_operations,
                                    siots_p->journal_slot_id);

        /* Nothing to Flush but need to invalidate headers */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state18);

        /* Move fruts used for header reads to write_fruts chain to use them for header invalidations.  
         */
        fbe_parity_write_log_move_read_fruts_to_write_chain(siots_p);
    }
    else
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO, 
                                    "%s Header read done. Flush not required. Header Inv not required. slot_id 0x%x alg: 0x%x \n", 
                                    __FUNCTION__,
                                    siots_p->journal_slot_id,
                                    siots_p->algorithm);

        /* Nothing to do*/
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_completed);

        /* Release the slot (this is a synchronous call) */
        (void) fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state005()
 ******************************************************************************/

/****************************************************************
 * fbe_parity_write_log_flush_state006()
 ****************************************************************
 * @brief
 * Based on header info, allocate memory for Journal flush.  
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  2/28/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state006(fbe_raid_siots_t * siots_p)
{
    /* Add one for terminator. */
    fbe_raid_fru_info_t write_fru_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t read_fru_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_u16_t num_read_fruts_destroyed = 0;
    fbe_u16_t num_read2_fruts_destroyed = 0;
    fbe_status_t status;

    /* Generate the journal and preread fru info, determine the types of sgs we will need,
     * the optimal page size and the number of pages to allocate for this operation.
     */
    status = fbe_parity_journal_flush_calculate_memory_size(siots_p,
                                                            (fbe_u32_t) (FBE_RAID_MAX_DISK_ARRAY_WIDTH+1),
                                                            write_fru_info,
                                                            read_fru_info);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: %s: failed to calculate memory , siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* destroy the header read_fruts and read2_fruts
     */
    status = fbe_raid_fruts_chain_destroy(&siots_p->read_fruts_head, &num_read_fruts_destroyed);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: failed to destroy header read fruts chain status 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    status = fbe_raid_fruts_chain_destroy(&siots_p->read2_fruts_head, &num_read2_fruts_destroyed);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: failed to destroy header read2 fruts chain status 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* siots_p->total_fruts_initialized needs to be cleaned up before we can allocate more fruts to 
     * this siots.
     * Note, it is only incremented on creation of fruts if in debug mode, so don't do this in free builds!
     */
    if (RAID_DBG_ENABLED)
    {
        fbe_u32_t fruts_initialized = siots_p->total_fruts_initialized;
         
        siots_p->total_fruts_initialized -= num_read_fruts_destroyed;
        siots_p->total_fruts_initialized -= num_read2_fruts_destroyed;
        if (siots_p->total_fruts_initialized != 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log flush: number of fruts destroyed (%d) != fruts initialized (%d), siots_p 0x%p\n",
                                                num_read_fruts_destroyed + num_read2_fruts_destroyed,
                                                fruts_initialized,
                                                siots_p);

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }

    /* now free the memory - releasing the journal fruts
     */
    status = fbe_raid_siots_free_memory(siots_p);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: %s: failed to free journal fruts memory , siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Set next state that we need to transition to.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state007);

    /* now allocate all the memory we need for this flush operation
     */
    status = fbe_raid_siots_allocate_memory(siots_p);

    /* Check if the allocation completed immediately.  If so plant the resources
     * now.  Else we will transition to `state003' when the memory is available so 
     * return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately.  Plant the resources and then
         * transition to next state.
         */

        /* Assign memory and setup the fruts
         */
        status = fbe_parity_journal_flush_setup_resources(siots_p, write_fru_info, read_fru_info);

        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "journal flush: rebuild failed to setup resources. status: 0x%x\n",
                                                status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Issue the journal read */
        fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state11);
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
                                            "journal flush: read memory request failed with status: 0x%x\n",
                                            status);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state006()
 ******************************************************************************/

/****************************************************************
 * fbe_parity_write_log_flush_state007()
 ****************************************************************
 * @brief
 * Memory has been allocated . We now have to setup resources to  
 * read in write_log data.  
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  3/01/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state007(fbe_raid_siots_t * siots_p)
{
    /* Add one for terminator. */
    fbe_raid_fru_info_t write_fru_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];
    fbe_raid_fru_info_t read_fru_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];
    fbe_status_t status;

    /* First check if this request was aborted while we were waiting for
     * resources.  If so simply transition to the aborted handling state.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Now check for allocation failure
     */
    if (RAID_MEMORY_COND(!fbe_raid_siots_is_deferred_allocation_successful(siots_p)))
    {
        /* Trace some information and transition to allocation error
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "journal flush: memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Generate the journal and preread fru info, determine the types of sgs we will need,
     * the optimal page size and the number of pages to allocate for this operation.
     */
    status = fbe_parity_journal_flush_calculate_memory_size(siots_p,
                                                            (fbe_u32_t) (FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1),
                                                            write_fru_info,
                                                            read_fru_info);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: %s: failed to calculate memory , siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Assign memory and setup the fruts
     */
    status = fbe_parity_journal_flush_setup_resources(siots_p, write_fru_info, read_fru_info);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "journal flush: rebuild failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Issue the journal read
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state11);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state007()
 ******************************************************************************/

/****************************************************************
 * fbe_parity_write_log_flush_state008()
 ****************************************************************
 * @brief
 *  Got CRC errors. Perform retries.  
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  2/28/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state008(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if ( (xor_status != FBE_XOR_STATUS_NO_ERROR || fbe_raid_siots_is_retry( siots_p )) &&
         !fbe_raid_csum_handle_csum_error( siots_p, read_fruts_p, 
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           fbe_parity_write_log_flush_state004 ))
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 004 to evaluate retries.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* No checksum error, continue with write_log flush. */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state005);
    }
    else if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* An aborted SIOTS is treated like a hard error.
         * Bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
		/* Some Header reads failed due to csum/lba stamp errors.
         * Remove fruts that failed from read_fruts_chain and temporarily save 
         * them on read2_fruts_chain.
         */
        fbe_parity_write_log_remove_failed_read_fruts(siots_p, NULL);

        /* Transition to next state which will reconstruct headers that had csum errors from headers   
         * with no errors. If headers with no errors are not available Flush operation will be abandoned.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state005);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected xor status %d", xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state008()
 ******************************************************************************/

/*!*****************************************************************************
 * fbe_parity_journal_flush_state4()
 *******************************************************************************
 * @brief
 *  Interrogate the status of the prereads.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  2/4/2011 - Created. Daniel Cummins
 ******************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state4(fbe_raid_siots_t * siots_p)
{
	fbe_raid_fru_eboard_t eboard;
	fbe_raid_fru_error_status_t status;
	fbe_raid_fruts_t *read_fruts_ptr = NULL;

	fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

	fbe_raid_fru_eboard_init(&eboard);

	/* Get totals for errored or dead drives.
	 */
	status = fbe_parity_get_fruts_error(read_fruts_ptr, &eboard);

	if (status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
	{
		/* More than one read failed  
		 * or the unit is shutdown,
		 */
		fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
	}
	else if ((status == FBE_RAID_FRU_ERROR_STATUS_DEAD) || (status == FBE_RAID_FRU_ERROR_STATUS_RETRY))
	{
		/* Metadata operation hit a dead error.
		 */
		fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
	}
	else if (fbe_raid_siots_is_aborted(siots_p))
	{
		/* the request has been aborted,
		 * go to the state where we'll exit with failed status.
		 */
		fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
	}
	else if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
	{
		/* All prereads are successful. Check lba stamps and checksums.
		 */
		fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state5);
	}
	else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
	{
		if (eboard.hard_media_err_count > 0)
		{
			/* Reads failed due to hard errors,
			 * exit to verify.
			 */
			fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state8);
		}
		else
		{
			/* This status is not expected.
			 */
			fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
												"journal flush: %s fru error status unknown 0x%x\n", 
												__FUNCTION__, status);
			return FBE_RAID_STATE_STATUS_EXECUTING;
		}
	}
	else
	{
		/* This status was not expected.  Instead of panicking, just
		 * return the error. 
		 */
		fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
											"journal flush: %s status unknown 0x%x\n", 
											__FUNCTION__, status);
		return FBE_RAID_STATE_STATUS_EXECUTING;
	}
	return FBE_RAID_STATE_STATUS_EXECUTING;
}

/*!*****************************************************************************
 * end fbe_parity_journal_flush_state4()
 ******************************************************************************/

/*******************************************************************************
 * fbe_parity_journal_flush_state5()
 *******************************************************************************
 * @brief
 *  All prereads are successful, check the checksums.
 *
 * @param siots_p - current I/O.
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  02/07/11 Created. Daniel Cummins
 *
 ******************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state5(fbe_raid_siots_t * siots_p)
{
	fbe_status_t status;
	fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;
	fbe_raid_fruts_t *read_fruts_ptr = NULL;

	fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

	/* Check xsums/lba stamps. routine will determine if both need
	 * checked or if only lba stamps need checked.
	 */
	status = fbe_raid_xor_fruts_check_checksum(read_fruts_ptr, FBE_TRUE);

	if (RAID_COND(status != FBE_STATUS_OK))
	{
		fbe_raid_siots_set_unexpected_error(siots_p, 
											FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
											"Journal flush: Invalid XOR status %d",
											status);
		return FBE_RAID_STATE_STATUS_EXECUTING;
	}

	fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

	if ( xor_status == FBE_XOR_STATUS_NO_ERROR && !fbe_raid_siots_is_retry(siots_p) )
	{
		/* No checksum error, continue to journal read
		 */
		fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state11);
	}
	else
	{
		/* Checksum error, start recovery verify on preread
		 */
		fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state8);
	}

	return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_journal_flush_state5()
 ******************************************************************************/

/*******************************************************************************
 * fbe_parity_journal_flush_state8()
 *******************************************************************************
 * @brief
 *  Start recovery verify on a preread.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *     02/07/11 - Daniel Cummins
 *
 *****************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state8(fbe_raid_siots_t * siots_p)
{
	fbe_status_t fbe_status;

	/* When we get back from verify, continue with read state machine.
	 */
	fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state9);

	/* Transition to recovery verify to attempt to fix errors.
	 */
	fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_recovery_verify_state0);
	if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
	{
		fbe_raid_siots_set_unexpected_error(siots_p, 
											FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
											"journal flush: failed to start nested siots  0x%p\n",
											siots_p);
		return FBE_RAID_STATE_STATUS_EXECUTING;
	}

	return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************************************************************
 * end fbe_parity_journal_flush_state8()
 ******************************************************************************/

/*!*****************************************************************************
 * fbe_parity_journal_flush_state9()
 ******************************************************************************* 
 * @brief 
 *  Handle status from degraded recovery verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  02/07/11 - Created. Daniel Cummins
 *
 ******************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state9(fbe_raid_siots_t * siots_p)
{
	/* Return from degraded recovery verify.
	 */
	if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
		fbe_raid_siots_is_aborted(siots_p))
	{
		fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
	}
	else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
	{
		/* The data has been verified,
		 * continue with journal read.
		 */
		fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state11);
	}
	else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
	{
		/* There is a shutdown condition, go shutdown error.
		 */
		fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
	}
	else
	{
		/* We do not expect this error.  Display the error and return bad status.
		 */
		fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
					"journal flush: %s unexpected error for siots_p = 0x%p, "
					"siots_p->error = 0x%x, lba: 0x%llx\n", 
						__FUNCTION__,
						siots_p,
						siots_p->error, 
						(unsigned long long)siots_p->start_lba);
	}
	return FBE_RAID_STATE_STATUS_EXECUTING; 
}
/*!*****************************************************************************
 * fbe_parity_journal_flush_state9()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_state11()
 ******************************************************************************
 * @brief
 *  Prereads are complete. Read the journal data.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  returns FBE_RAID_STATE_STATUS_EXECUTING if retries are not needed and
 *  FBE_RAID_STATE_STATUS_WAITING otherwise.
 *
 * @author
 * 1/24/2011 - Created. Daniel Cummins
 *
 ******************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state11(fbe_raid_siots_t * siots_p)
{    
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_bool_t b_result = FBE_TRUE;

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* The journal fruts are on the write fruts chain.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

    /* Initialize the number of fruts we will be waiting for completion on
     */
    if ((siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p)) == 0)
    {
        /* No positions to flush the write data to because they are dead!  The only thing
         * we can do here is complete the flush. The drives will have been marked for
         * needs rebuild before we discover they are dead.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                             "journal flush: journal flush needed to read %d drives but they are dead.\n",
                             siots_p->drive_operations);

        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* set the next state
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state12);

    /* Initiate the journal reads.
     */
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);

    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts. */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: %s failed to read write_log slot data "
                                            "siots_p 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_parity_journal_flush_state11()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_parity_journal_flush_state12()
 ******************************************************************************
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
 * 1/24/2011 - Created. Daniel Cummins
 * 04/05/2012 - Vamsi V. Changes to Error handling.
 ******************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state12(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_status_t status;

    /* The write fruts chain contains the write_log data we just read 
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    fru_status = fbe_parity_get_fruts_error(write_fruts_p, &eboard);

    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Success. Continue
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state13);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             fbe_raid_siots_is_aborted(siots_p))
    {
        /* We check for aborted directly in siots since we are at the read phase and 
         * regardless of the operation we should abort the request. 
         */
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, write_fruts_p, &eboard, fru_status);
        return state_status;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_status_t retry_status;

        /* We might have gone degraded and have a retry.
         * In this case it is necessary to mark the degraded positions as nops since 
         * 1) we already marked the paged for these 
         * 2) the next time we run through this state we do not want to consider these failures 
         *    (and wait for continue) since we already got the continue. 
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        retry_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_p);
        if (retry_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        if (eboard.hard_media_err_count > 0)
        {
            /* Some write_log data reads failed due to hard media errors.
             * Its too many faults: 
             * 1. RG is degraded. 
             * 2. RG is broken and so has to do a Flush.
             * 3. We get a media error on write_log data read.
             *
             * Lets not do any heroics. Abandon Flush and run a Verify on live-stripe. 
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                 "write_log flush: Abandon flush due to media error: lba 0x%llx, slot_id 0x%x, me_bitmap 0x%x \n",
                                 siots_p->start_lba, siots_p->journal_slot_id, eboard.hard_media_err_bitmap);

            status = fbe_parity_write_log_send_flush_abandoned_event(siots_p,
                                                                     eboard.hard_media_err_bitmap,
                                                                     0, /* This field is for CRC errors, not media errors */ 
                                                                     RG_FLUSH_JOURNAL);

            fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state16);
        }
        else if (eboard.dead_err_count > 0) 
        {
            /* We continue with Flushing non-degraded positions.
             * Make the opcode of any degraded positions be nop.  
             */
            fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

            /* Next state to execute */
            fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state13);
        }
        else if(eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                  "write_log flush: %s fru dropped error unexpected "
                                  "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                  __FUNCTION__, eboard.drop_err_count, eboard.drop_err_bitmap);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        }
        else
        {
            /* This status is not expected.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log flush: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, fru_status);
        }
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Write_log flush: unexpected status 0x%x\n", fru_status);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_journal_flush_state12()
 ******************************************************************************/

/*!*****************************************************************************
 * fbe_parity_journal_flush_state13()
 *******************************************************************************
 * @brief
 *  Write_log reads completed successfully. Check stamps and validate data. 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns  FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  4/11/2012 - Vamsi V. Ported and modified from state12. 
 *
 ******************************************************************************/
fbe_raid_state_status_t fbe_parity_journal_flush_state13(fbe_raid_siots_t * siots_p)
{   
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_status_t status;
    fbe_xor_status_t xor_status;
    fbe_u32_t slot_id;
    fbe_u16_t chksum_err_bitmap; 

    /* The write fruts chain contains the write_log data we just read 
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);

    /* The reads are successful - we now need to check the lba stamps and checksums.
     * Before we can do that we need to ensure the write fruts chain has the proper
     * seed.  To do this we need to modify the lba in each write fruts to point to
     * the live stripe area where we intend to write.  The live stripe lbas were the
     * seeds used to generate the lba stamps and checksums.
     */
    status = fbe_parity_journal_flush_restore_dest_lba(siots_p);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Write_log flush: setup of write portion of journal flush operation failed, status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Now that the lbas (seeds) were adjusted we can now validate the lba stamps and 
     * checksums for the data we just read in from the log.
     */
    status = fbe_raid_xor_fruts_check_checksum(write_fruts_p, FBE_TRUE);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Write_log flush: Invalid XOR status %d",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* check the status of the checksum/lba stamp check
     */
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if ((xor_status != FBE_XOR_STATUS_NO_ERROR) &&
        (xor_status != FBE_XOR_STATUS_CHECKSUM_ERROR))
    {
        /* Unexpected xor status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Write_log flush: XOR status 0x%x unexpected.\n", xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    } 

    /* check for bad checksum or lba stamp errors
     */
    chksum_err_bitmap =   siots_p->misc_ts.cxts.eboard.crc_lba_stamp_bitmap
                        | siots_p->misc_ts.cxts.eboard.crc_single_bitmap
                        | siots_p->misc_ts.cxts.eboard.crc_multi_bitmap;
    if (chksum_err_bitmap)
    {
        /* Process error bits into the error_info for the event
         */
        fbe_u32_t uc_err_bits = fbe_raid_get_uncorrectable_status_bits(&siots_p->misc_ts.cxts.eboard, FBE_RAID_POSITION_BITMASK_ALL);

        //  fbe_raid_write_journal_trace(siots_p,FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
        fbe_raid_write_journal_trace(siots_p,FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                     "Write_log read of slot %d found bad checksum or LBA stamp in positions 0x%x\n",
                                     slot_id, chksum_err_bitmap);

        /* Some write_log data reads failed due to stamp errors.
         * Its too many faults: 
         * 1. RG is degraded. 
         * 2. RG is broken and so has to do a Flush.
         * 3. We get a media error on write_log data read.
         *
         * Lets not do any heroics. Abandon Flush and run a Verify on live-stripe. 
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                             "write_log flush: Abandon flush due to crc error: lba 0x%llx, slot_id 0x%x, chksum_err_bitmap 0x%x \n",
                             siots_p->start_lba, siots_p->journal_slot_id, chksum_err_bitmap);

        status = fbe_parity_write_log_send_flush_abandoned_event(siots_p,
                                                                 chksum_err_bitmap,
                                                                 uc_err_bits, 
                                                                 RG_FLUSH_JOURNAL);

        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state16);
    }
    else
    {
        /* Csum/lba stamps are good on all the blocks. Now Verify checksum_of_checksums 
         * field in the slot header 
         */
        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
        status = fbe_parity_write_log_calc_csum_of_csums(siots_p, write_fruts_p, FBE_FALSE, &chksum_err_bitmap);

        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "Write_log flush: Invalid calc_csum_of_csums status %d",
                                                status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* csum_of_csums error on any position implies that write to write_log slot was 
         * incomplete which implies that write to live-stripe was not initiated. So, 
         * live-stripe is consistent with old-data (which is ok since we did not acknowledge 
         * the client.) So, this is the same as an invalid slot and we should not flush.
         */
        if (chksum_err_bitmap != 0)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO, 
                                        "%s Data read complete. Found csum_of_csum error, slot write incomplete, no flush required. slot_id: 0x%x csum_bitmap: 0x%x \n", 
                                        __FUNCTION__,
                                        siots_p->journal_slot_id, chksum_err_bitmap);

            /* Nothing to Flush. Run verify on live stripe. */
            fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state16);
        }
        else
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO, 
                                        "%s Data read complete. Flushing to live-stripe. slot_id: 0x%x\n", 
                                        __FUNCTION__, siots_p->journal_slot_id);

            /* Flush to live-stripe */
            fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state14);
        }
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************************************************
 * end fbe_parity_journal_flush_state13()
 *****************************************************************************/

/*!*****************************************************************************
 * fbe_parity_journal_flush_state14()
 *******************************************************************************
 * @brief
 *  Prereads, Journal Reads, and lba/checksum validation has occured.  Now its
 * time to flush the data.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns  FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  1/24/2011 - Created. Daniel Cummins
 *
 ******************************************************************************/
fbe_raid_state_status_t 
fbe_parity_journal_flush_state14(fbe_raid_siots_t * siots_p)
{   
	fbe_status_t status;
	fbe_raid_fruts_t *write_fruts_p = NULL;
	fbe_u16_t bitmask = 0;
	fbe_bool_t b_result = FBE_TRUE;
	fbe_u32_t slot_id;

	fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);
	fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

	/* Turn the write fruts chain into WRITE operation.  
	 */
	status = fbe_parity_journal_flush_generate_write_fruts(siots_p);

	if (RAID_COND(status != FBE_STATUS_OK))
	{
		fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
											"journal flush: setup of write portion of journal flush operation failed, status: 0x%x\n",
											status);
		return FBE_RAID_STATE_STATUS_EXECUTING;
	}

	/* invalidate fruts that had bad stamps or checksums
	 */
	bitmask =  siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap
             | siots_p->misc_ts.cxts.eboard.crc_lba_stamp_bitmap
             | siots_p->misc_ts.cxts.eboard.crc_single_bitmap
             | siots_p->misc_ts.cxts.eboard.crc_multi_bitmap;
    if (bitmask)
	{
        /* Unexpected xor status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Write_log flush: Proceeding with Flush with media/csum errors at positions 0x%x .\n", bitmask);

        return FBE_RAID_STATE_STATUS_EXECUTING;
	}

    /* Initialize the number of fruts we will be waiting for completion on
     */
    if ((siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p)) == 0)
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                             "write_log flush: slot %d Needed to flush %d drives but could not due to errors, bitmask 0x%x.\n",
                             slot_id, siots_p->drive_operations, bitmask);

        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_completed);

         /* Abandon the flush and release the slot (this is a synchronous call) */
        (void) fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

	/* set the next state
	 */
	fbe_raid_siots_transition(siots_p, fbe_parity_journal_flush_state15);

	/* Initiate the flush
	 */
	b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
	if (RAID_FRUTS_COND(b_result != FBE_TRUE))
	{
		/* We hit an issue while starting chain of fruts.
		 */
		fbe_raid_siots_set_unexpected_error(siots_p, 
											FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
											"write_log flush: %s failed to start write fruts chain for "
											"siots_p 0x%p\n",
											__FUNCTION__,
											siots_p);
	}

	return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_parity_journal_flush_state14()
 *****************************************************************************/

static fbe_status_t
fbe_parity_journal_flush_done_msg(fbe_raid_siots_t *siots_p, fbe_raid_fru_error_status_t status)
{	
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_u32_t slot_id;
    fbe_object_id_t object_id;
    fbe_raid_fruts_t *fruts_p;
    const fbe_u8_t *error_str = (status==FBE_RAID_FRU_ERROR_STATUS_SUCCESS ? "SUCCESS" : "FAIL");

    fbe_raid_siots_get_journal_slot_id(siots_p,&slot_id);
    fbe_transport_get_object_id(fbe_raid_iots_get_packet(iots_p), &object_id);
    fbe_raid_siots_get_write_fruts(siots_p, &fruts_p);
    while (fruts_p)
    {

        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "FLUSHED ,slot 0x%x, fru %d, lba 0x%llx, blocks 0x%llx, preread lba 0x%llx, preread blocks %llu\n",
                                    slot_id,
                                    fruts_p->position,
                                    fruts_p->lba,
                                    fruts_p->blocks,
                                    fruts_p->write_preread_desc.lba,
                                    fruts_p->write_preread_desc.block_count);


        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "FLUSH COMPL(%s 0x%x) siots0x%llx,siots->flag0x%x,slot0x%x,total frus%d\n",
                                error_str,
                                status,
                                (unsigned long long)siots_p,
                                siots_p->flags,
                                slot_id,
                                siots_p->drive_operations);

	return FBE_STATUS_OK;
}
/*!****************************************************************************
 *  fbe_parity_journal_flush_state15()
 ******************************************************************************
 * @brief
 *  The writes completed, check the status of the writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t      
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  1/25/2011 - Created. Daniel Cummins
 *  3/20/2012 - Vamsi V. Modified error handling. 
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_journal_flush_state15(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status = FBE_RAID_FRU_ERROR_STATUS_INVALID;
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(write_fruts_ptr, &eboard);

    fbe_parity_journal_flush_done_msg(siots_p, status);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Flush to live stripe completed. Run Verify to make sure there are no stamp errors. */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state16);   

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        /* Multiple errors, we are going shutdown.
         * Return bad status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Metadata operation hit a dead error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        /* We might have gone degraded and have a retry.
         * In this case it is necessary to mark the degraded positions as nops since 
         * 1) we already marked the paged for these 
         * 2) the next time we run through this state we do not want to consider these failures 
         *    (and wait for continue) since we already got the continue. 
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_ptr);
        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        fbe_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_ptr);
        if (fbe_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, fbe_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);

            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* One drive is dead, we need to mark this request
         * as waiting for a shutdown or continue.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* No hard errors on writes.
         */
        if (eboard.hard_media_err_count != 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: hard error count unexpected: %d \n",
                                                eboard.hard_media_err_count);
        }
        if (eboard.drop_err_count > 0)
        {
            /* DH Dropped the request,
             * we should not have submitted writes as optional.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru dropped error unexpected status: 0x%x "
                                                "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                                __FUNCTION__, status, eboard.drop_err_count, eboard.drop_err_bitmap);
        }
        else if ((eboard.dead_err_count <= fbe_raid_siots_parity_num_positions(siots_p)) &&
                 (eboard.dead_err_count > 0))
        {
            /* Nothing to do if some write positions are dead. They will get rebuilt when drive comes back.
             * Run a Verify to make sure Flush operation completed successfully and there are no stamp errors.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state16);
        }
        else
        {
            /* Uknown error
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);
        }
    }    /* end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/
    else
    {
        /* This case should have been handled above.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru status unknown 0x%x\n", 
                                            __FUNCTION__, status);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************************************************
 * end fbe_parity_journal_flush_state15()
 *****************************************************************************/

/*******************************************************************************
 * fbe_parity_write_log_flush_state16()
 *******************************************************************************
 * @brief
 *  Run Verify on live stripe and fix any inconsistencies.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  04/17/12 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state16(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status;

    /* When we get back from Verify, continue with finishing up the Flush operation.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state17);

    /* Remove read fruts to prevent Verify from using read memory buffers.
     * Read buffers are allocated for pre-reads due to block virtualization.
     * This should be dead-code once support for BV is removed.
     */
    {       
        fbe_raid_fruts_t * read_fruts_p = NULL;
        fbe_raid_fruts_t * next_read_fruts_p = NULL;
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

        while (read_fruts_p)
        {
            /* get next fruts in the chain */
            next_read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

            /* Remove from write chain and place on the freed_chain
             */
            fbe_status = fbe_raid_siots_move_fruts(siots_p, 
                                                   read_fruts_p,
                                                   &siots_p->freed_fruts_head);
            if (RAID_COND_STATUS((fbe_status != FBE_STATUS_OK), fbe_status))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "flush: failed to remove read fruts: siots_p = 0x%p, "
                                                    "status = 0x%x, read_fruts_p = 0x%p\n",
                                                    siots_p, fbe_status, read_fruts_p);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* move to next fruts in the chain */
            read_fruts_p = next_read_fruts_p;
        }
    }

    /* Transition to recovery verify to attempt to fix errors.
     */
    fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_recovery_verify_state0);
    if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: failed to start nested siots  0x%p\n",
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state16()
 ******************************************************************************/

/*!*****************************************************************************
 * fbe_parity_write_log_flush_state17()
 ******************************************************************************* 
 * @brief 
 *  Handle status from verify of live stripe.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  04/17/12 - Created. Vamsi V
 *
 ******************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state17(fbe_raid_siots_t * siots_p)
{
    /* Return from degraded verify.
     */
    if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
        fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* The data has been verified. Proceed to next state.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state18);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* There is a shutdown condition, go shutdown error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else
    {
        /* We do not expect this error.  Display the error and return bad status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: %s unexpected error for siots_p = 0x%p, "
                                            "siots_p->error = 0x%x, lba: 0x%llx\n", 
                                            __FUNCTION__,
                                            siots_p,
                                            siots_p->error, 
                                            siots_p->start_lba);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING; 
}
/*!*****************************************************************************
 * fbe_parity_write_log_flush_state17()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state18()
 ******************************************************************************
 * @brief
 *  Flush of user data from write_log slot to live stripe is completed. 
 *  Invalidate the slot header to indicate that write_log slot no longer
 *  has any valid user data and the slot is free. 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  3/06/2012 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state18(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_bool_t b_result = FBE_TRUE;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Re-initalize write fruts to do slot header invalidation */
    status = fbe_parity_write_log_generate_header_invalidate_fruts(siots_p);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: setup of header_invalidation portion of write_log flush operation failed, status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Initialize the number of fruts we will be waiting for completion on */
    if ((siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p)) == 0)
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                             "write_log flush: Needed to flush %d drives but could not due to errors.\n",
                             siots_p->drive_operations);

        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_completed);

        /* Abandon the flush and release the slot (this is a synchronous call) */
        (void) fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* set the next state */
    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state19);

    /* Initiate the flush */
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "write_log flush: %s failed to start write fruts chain for "
                                            "siots_p 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state18()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_state19()
 ******************************************************************************
 * @brief
 *  Check the status of Header Invalidation.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  3/06/2012 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_state19(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status = FBE_RAID_FRU_ERROR_STATUS_INVALID;
    fbe_status_t fbe_status = FBE_STATUS_INVALID;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u32_t slot_id;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fru_eboard_init(&eboard);
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_id);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(write_fruts_ptr, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Success on slot header invalidation - now release the slot */
        fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_completed);  
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "INVAL COMPL siots:0x%lx,slot0x%x,total frus%d\n",
                                    (unsigned long)siots_p,
                                    slot_id,
                                    siots_p->drive_operations);
        (void) fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        /* Multiple errors, we are going shutdown.
         * Return bad status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Metadata operation hit a dead error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error); 

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        /* We might have gone degraded and have a retry.
         * In this case it is necessary to mark the degraded positions as nops since 
         * 1) we already marked the paged for these 
         * 2) the next time we run through this state we do not want to consider these failures 
         *    (and wait for continue) since we already got the continue. 
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_ptr);
        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        fbe_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_ptr);
        if (fbe_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, fbe_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);

            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* One drive is dead, we need to mark this request
         * as waiting for a shutdown or continue.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* No hard errors on writes.
         */
        if (eboard.hard_media_err_count != 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: hard error count unexpected: %d \n",
                                                eboard.hard_media_err_count);

            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        if (eboard.drop_err_count > 0)
        {
            /* DH Dropped the request,
             * we should not have submitted writes as optional.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru dropped error unexpected status: 0x%x "
                                                "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                                __FUNCTION__, status, eboard.drop_err_count, eboard.drop_err_bitmap);

            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        else if ((eboard.dead_err_count <= fbe_raid_siots_parity_num_positions(siots_p)) &&
                 (eboard.dead_err_count > 0))
        {
            /* For RAID5/RAID3, if one error then return good status.
             * For RAID 6, if one or two error(s) return good status.
             * Release the write_log slot.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_completed);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "DEGR INVAL COMPL siots:0x%lx,slot0x%x,total frus%d\n",
                                        (unsigned long)siots_p,
                                        slot_id,
                                        siots_p->drive_operations);
            (void) fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);

            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        else
        {
            /* Uknown error
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);

            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
    }    /* end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/
    else
    {
        /* This case should have been handled above.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru status unknown 0x%x\n", 
                                            __FUNCTION__, status);

        return FBE_RAID_STATE_STATUS_EXECUTING; 
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_state19()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_completed()
 ******************************************************************************
 * @brief
 *  Check the status of the release slot.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  2/24/2011 - Created. Daniel Cummins
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_completed(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_completed()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_write_log_flush_slot_dataloss()
 ******************************************************************************
 * @brief
 *  This state handles loss of journal slot data.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  03/06/2011 - Created. Vamsi V
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_write_log_flush_slot_dataloss(fbe_raid_siots_t * siots_p)
{   
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_write_log_flush_slot_dataloss()
 ******************************************************************************/
