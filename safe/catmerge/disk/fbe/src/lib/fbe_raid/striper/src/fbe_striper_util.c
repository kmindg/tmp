/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_striper_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains generic utility functions of the striper object.
 *
 * @version
 *   8/10/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_striper_get_fruts_error()
 ****************************************************************
 * @brief
 *  Determine the overall status of a fruts chain.
 *
 * @param fruts_p - This is the chain to get status on.
 * @param eboard_p - This is the structure to save the results in.
 *
 * @return  fbe_raid_fru_error_status_t
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_fru_error_status_t fbe_striper_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                        fbe_raid_fru_eboard_t * eboard_p)
{
    fbe_raid_fru_error_status_t fru_error_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    fbe_bool_t b_status;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Sum up the errors encountered. 
     */
    b_status = fbe_raid_fruts_chain_get_eboard(fruts_p, eboard_p);

    if (RAID_COND(b_status == FBE_FALSE))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: get fru eboard unexpected err siots_p = 0x%p\n", siots_p);
        fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
    }

    if (fbe_raid_siots_is_aborted_for_shutdown(siots_p))
    {
        /* Just let the monitor clean up this request.
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "striper: siots: %p Marked aborted for shutdown. Wait for monitor.\n", 
                                      siots_p);
        return FBE_RAID_FRU_ERROR_STATUS_WAITING;
    }
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        return FBE_RAID_FRU_ERROR_STATUS_ABORTED;
    }

    if (eboard_p->bad_crc_count > 0)
    {
        return FBE_RAID_FRU_ERROR_STATUS_BAD_CRC;
    }
    else if (eboard_p->not_preferred_count > 0)
    {
        return FBE_RAID_FRU_ERROR_STATUS_NOT_PREFERRED;
    }
    else if (eboard_p->reduce_qdepth_count > 0)
    {
        return FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_HARD;
    }
    else if ( eboard_p->soft_media_err_count > 0 ||
              (eboard_p->hard_media_err_count > 0 &&
               (eboard_p->menr_err_bitmap != eboard_p->hard_media_err_bitmap)))
    {
        siots_p->media_error_count++;

        /* If we took a media error, submit a request to remap.
         * For the case of a hard media error, only submit a request to remap
         * if there were some errors that weren't MENR.
         */
        if ( fbe_raid_geometry_is_raid10(raid_geometry_p) )
        {
            /* If we're at the RAID0 level in a striped mirror, we should be not
             * be sending a request to remap since it was done at the RAID1 level.
             */
        }
        /* If we are doing Read only Background verify 
         * we will not send it to Remap 
         */
        else if (!fbe_raid_siots_is_read_only_verify(siots_p))
        {
            /* The object will mark this chunk as needing a remap 
             * once we complete up to the object. 
             */
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
        }
    }

    if ((eboard_p->hard_media_err_bitmap > 0) ||
        (eboard_p->drop_err_bitmap > 0))
    {
        fru_error_status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
    }

    if (eboard_p->abort_err_count != 0)
    {
        /* If we are aborted, return this status immediately since
         * this entire request is finished.
         */
        fru_error_status = FBE_RAID_FRU_ERROR_STATUS_ABORTED;
    }
    else if ((eboard_p->dead_err_count == 0) && (eboard_p->retry_err_count == 0))
    {
        if ((fru_error_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS) &&
            (eboard_p->reduce_qdepth_soft_count > 0))
        {
            /* Only if we are otherwise successful do we report this status.
             */
            return FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_SOFT;
        }
        /* No frus are dead, just return current status.
         */
        return fru_error_status;
    }
    else if (eboard_p->dead_err_count > 0)
    {
        if (fbe_raid_siots_is_background_request(siots_p))
        {
            /* Trace some information and return `dead' status.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "striper fruts err bg op  iots opcode: 0x%x siots lba: 0x%llx blks: 0x%llx\n",
                                    fruts_p->opcode, (unsigned long long)siots_p->start_lba, siots_p->xfer_count);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                   "striper:  dead Bits: 0x%x\n",
                                   eboard_p->dead_err_bitmap);
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_DEAD;
        }
        else if (fbe_raid_siots_is_metadata_operation(siots_p))
        {
            /* If we are a metadata operation, we cannot wait. 
             * The monitor needs to quiesce and this operation needs to finish before we can quiesce. 
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "striper fruts err md op: iots opcode: 0x%x siots lba: 0x%llx blks: 0x%llx\n",
                                        fruts_p->opcode, (unsigned long long)siots_p->start_lba, siots_p->xfer_count);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                   "striper:  dead Bits: 0x%x\n",
                                   eboard_p->dead_err_bitmap);
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_DEAD;
        }
        else if (fbe_raid_siots_is_monitor_initiated_op(siots_p))
        {
            /* If we are a monitor operation, we cannot wait. 
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "striper fruts err mntr op: iots opcode: 0x%x siots lba: 0x%llx blks: 0x%x\n",
                                        fruts_p->opcode, siots_p->start_lba, (unsigned int)siots_p->xfer_count);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                   "striper:  dead Bits: 0x%x\n",
                                   eboard_p->dead_err_bitmap);
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_DEAD;
        }
        else
        {
            /* We wait for the monitor to tell us what to do.
             */
            fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                          FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                          "striper: non-retryable error count: %d mask: 0x%x\n",
                                          eboard_p->dead_err_count,
                                          eboard_p->dead_err_bitmap);
            
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_WAITING;
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);
        }
    }
    else if ((eboard_p->retry_err_count > 0) &&
             (fbe_raid_geometry_is_raid0(raid_geometry_p)))
    {
        /* We only expect retryable error from drives, not from a child raid group.
         */
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
        {
            /* If we have a retryable error and we are quiescing, then we should not 
             * execute the retry now.  Simply wait for the continue. 
             */
            fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                          FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                          "striper: retryable errors wait for quiesce retry errs: %d mask: 0x%x\n",
                                          eboard_p->retry_err_count,
                                          eboard_p->retry_err_bitmap);
            
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_WAITING;
        }
        else
        {
            /* Otherwise perform the retry.
             */
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_RETRY;
            fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                          FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                          "striper: retryable errors,retry err count: %d mask: 0x%x\n",
                                          eboard_p->retry_err_count,
                                          eboard_p->retry_err_bitmap);
        }
    }
    else if ((eboard_p->retry_err_count > 0) &&
             (fbe_raid_geometry_is_raid10(raid_geometry_p)))
    {
        /* We only expect retryable error from drives (i.e. not from a child
         * raid group).  This can happen when the mirror goes to activate.  Consider these as dead errors.
         */
        fru_error_status = FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN;
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "striper: retryable error count: %d mask: 0x%x\n",
                                      eboard_p->retry_err_count,
                                      eboard_p->retry_err_bitmap);
    }
    else
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "striper: err count unexpected,retry_c: %d mask: 0x%x dead_c: %d mask: 0x%x\n",
                                      eboard_p->retry_err_count,
                                      eboard_p->retry_err_bitmap,
                                      eboard_p->dead_err_count,
                                      eboard_p->dead_err_bitmap);

    }
    return fru_error_status;
}
/******************************************
 * end fbe_striper_get_fruts_error()
 ******************************************/

/*!**************************************************************
 * fbe_striper_handle_fruts_error()
 ****************************************************************
 * @brief
 *  Handle a common fruts error status.
 *
 * @param siots_p - current I/O.
 * @param fruts_p - This is the chain to get status on.
 * @param eboard_p - This is the structure to save the results in.
 * @param fru_error_status - This is the status we need to handle.
 *
 * @return fbe_raid_state_status_t FBE_RAID_STATE_STATUS_EXECUTING or
 *                                 FBE_RAID_STATE_STATUS_WAITING
 * @author
 *  2/10/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_handle_fruts_error(fbe_raid_siots_t *siots_p,
                                                       fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_fru_eboard_t * eboard_p,
                                                       fbe_raid_fru_error_status_t fruts_error_status)
{
    if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Metadata operation hit a dead error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* Some number of drives are not available.  Wait for monitor to make a decision.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_status_t status;
        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        status = fbe_raid_siots_retry_fruts_chain(siots_p, eboard_p, fruts_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard_p->retry_err_count, eboard_p->retry_err_bitmap, status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED)
    {
        /* The siots has been aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_BAD_CRC)
    {
        /* The siots has been aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_NOT_PREFERRED)
    {
        /* The siots has been aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_not_preferred_error);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_HARD)
    {
        /* The siots has been aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_reduce_qdepth_hard);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_SOFT)
    {
        /* The siots has been aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_reduce_qdepth_soft);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "%s fru unexpected status: 0x%x\n", 
                                            __FUNCTION__, fruts_error_status);
        fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_ERROR);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_striper_handle_fruts_error()
 **************************************/
/*!**************************************************************
 * fbe_striper_invalidate_hard_err()
 ****************************************************************
 * @brief
 *  For any drive that took a hard error, invalidate its associated
 *  buffers.
 *
 * @param siots_p - The siots that took the hard errors
 * @param eboard_p - The eboard that indicates which fru positions
 *                   took the errors
 *
 * @return  fbe_status_t
 *
 * @author
 *  3/25/2010 - Created. kls
 *
 ****************************************************************/
fbe_status_t fbe_striper_invalidate_hard_err(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_eboard_t * eboard_p)
{
    fbe_raid_fruts_t *read_fruts_p;
    fbe_xor_invalidate_sector_t xor_invalidate;
    fbe_u32_t current_position = 0;
   
	if (fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p)) == FBE_TRUE) {
		return FBE_STATUS_OK;
	}

    /* Initialize the number of disks to invalidate to 0.
     */
    xor_invalidate.data_disks = 0;

    for (fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
         read_fruts_p != NULL;
         read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p))
    {
        if ((1 << read_fruts_p->position) & eboard_p->hard_media_err_bitmap)
        {
            /* This position took a media error. Invalidate its buffer by
             * inserting it into the xor interface array to invalidate.
             */
            xor_invalidate.fru[current_position].sg_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p);
            xor_invalidate.fru[current_position].seed = read_fruts_p->lba;
            xor_invalidate.fru[current_position].count = read_fruts_p->blocks;
            xor_invalidate.fru[current_position].offset = 0;
            
            /* Increment the number of disks to invalidate, and move to the next position.
             */
            xor_invalidate.data_disks++;
            current_position++;
        }
    } 

    /* We should have found at least one position to invalidate.
     */
    if (xor_invalidate.data_disks == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                "striper: expected at least one drive to invalidate.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the invalidate reason and who.  The reason is `data lost' the
     * who is RAID.
     */
    xor_invalidate.invalidated_reason = XORLIB_SECTOR_INVALID_REASON_DATA_LOST;
    xor_invalidate.invalidated_by = XORLIB_SECTOR_INVALID_WHO_RAID;

    /* Invalidate the blocks.
     */
    if (fbe_xor_lib_execute_invalidate_sector(&xor_invalidate) != FBE_STATUS_OK)
    {
        /* We don't expect any errors invalidating.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                "striper: expected error invalidatint.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_striper_invalidate_hard_err()
 ******************************************/

/*!**************************************************************
 * fbe_striper_add_error_region_and_validate()
 ****************************************************************
 * @brief
 *  Populate the error region and record the errors.
 *
 * @param siots_p - The siots that took the hard errors
 *
 * @return  fbe_status_t
 *
 * @author
 *  4/6/2012 - Created.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_striper_add_media_error_region_and_validate(fbe_raid_siots_t *siots_p,
                                                                  fbe_raid_fruts_t*  read_fruts_p,
                                                                  fbe_u16_t      media_err_pos_bitmask)
                                             
{
    fbe_status_t              status;
    fbe_raid_position_bitmask_t valid_bitmask;
    fbe_raid_fruts_t*              current_fruts_ptr;
    fbe_u32_t                      position;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);


    status = fbe_raid_util_get_valid_bitmask(width, &valid_bitmask);
    if(RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "status 0x%x != FBE_STATUS_OK, width 0x%x valid_bitmask 0x%x \n",
                                            status, width, valid_bitmask);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_xor_lib_eboard_init(&siots_p->misc_ts.cxts.eboard);

    for (current_fruts_ptr = read_fruts_p;
         current_fruts_ptr != NULL;
         current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr))
    {
        position = current_fruts_ptr->position;
        if(media_err_pos_bitmask & (1 << position))
        {
            siots_p->misc_ts.cxts.eboard.media_err_bitmap |= (1 << position);
            siots_p->misc_ts.cxts.eboard.u_crc_bitmap |= (1 << position);

            fbe_xor_lib_mirror_add_error_region(&siots_p->misc_ts.cxts.eboard,
                                        &siots_p->vcts_ptr->error_regions,
                                        current_fruts_ptr->lba,
                                        current_fruts_ptr->blocks,
                                        width);
        }
        
    }

    if ( fbe_raid_siots_record_errors(siots_p,
                                      width,
                                      &siots_p->misc_ts.cxts.eboard,
                                      valid_bitmask,    /* Bits to record for. */
                                      FBE_TRUE,         /* Allow correctable errors.  */
                                      FBE_TRUE          /* Validate as required */ )
                                            != FBE_RAID_STATE_STATUS_DONE )
    {
        /* We have detected an inconsistency on the XOR error validation.
         * Wait for the raid group object to process the error.
         */
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    


    return FBE_RAID_STATE_STATUS_DONE;

}
/****************************************************
 * end fbe_striper_add_error_region_and_validate()
 ****************************************************/

/*************************
 * end file fbe_striper_util.c
 *************************/
