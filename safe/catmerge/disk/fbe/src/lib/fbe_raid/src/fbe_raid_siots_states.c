/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_siots_states.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid siots state definitions.
 *
 * @version
 *  3/16/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"
#include "fbe_traffic_trace.h"

/*************************
 *   FORWARD FUNCTION DEFINITIONS
 *************************/
static fbe_raid_state_status_t fbe_raid_siots_finished_check_data(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_siots_finished_state1(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_siots_finished_state2(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_siots_nest_finished(fbe_raid_siots_t *siots_p);

/*!***************************************************************************
 *          fbe_raid_siots_finished()
 *****************************************************************************
 *
 * @brief   The rg_siots_finished function is defined as a function pointer 
 *          because the final state might need to be changed for diagnostic 
 *          purposes.
 * 
 * @param   siots_p - Pointer to siots to complete
 *
 * @return  state_status - The state status of the siots
 *
 * @note    Only checkin code that defines the finished method to:
 *              o fbe_raid_siots_finished_state2  
 *
 *****************************************************************************/
static fbe_raid_state_status_t(*fbe_raid_siots_finished) (fbe_raid_siots_t *siots_p) = fbe_raid_siots_finished_state2;

/*!**************************************************
 * fbe_raid_siots_transition_finished
 ***************************************************
 * @brief:
 *  This inline transitions a SIOTS to the final state
 *  based upon it being a nested siots or not.
 *
 * @param siots_p - A ptr to the current RAID_SUB_IOTS.
 *
 * @return fbe_raid_state_status_t
 *
 ***************************************************/

fbe_raid_state_status_t fbe_raid_siots_transition_finished( fbe_raid_siots_t *siots_p )
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Transition to the correct final state 
     * depending on the type of siots.
     */
    if (fbe_raid_siots_is_nested(siots_p))
    {
        fbe_raid_common_t *common_parent_p = fbe_raid_common_get_parent(&siots_p->common);

        /* The parent needs to be a siots, because we only nest SIOTS under other SIOTS.
         */
        if (RAID_FINISH_COND(!fbe_raid_common_is_flag_set(common_parent_p, FBE_RAID_COMMON_FLAG_TYPE_SIOTS)))
        {
              fbe_raid_siots_trace(siots_p,
                                   FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                   "raid: %s: unexpected error: flag 0x%x unexpected, siots_p = 0x%p\n",
                                   __FUNCTION__,
                                   common_parent_p->flags, siots_p);

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_nest_finished);
    }
    else
    {
        /* If the `check data' flag is set transition to the `check data' which
         * in turn will transition to fbe_raid_siots_finished_state2.
         */
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_PATTERN_DATA_CHECKING))
        {
            /* If pattern data checking is enabled transition to the data 
             * checking state.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished_check_data);
        }
        else if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_INVALID_DATA_CHECKING))
        {
            /* Else if checking for data that has been invalidated is enabled
             * transition to invalidated checking state.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished_state1);
        }
        else
        {
            /* Else transition immediately to finished
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished);
        }
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*****************************************
 * fbe_raid_siots_transition_finished()
 *****************************************/

/*!**************************************************************************
 *          fbe_raid_siots_get_error_precedence()
 ***************************************************************************
 * @brief
 *  Determine how to rate the error.
 *
 * @param
 *  error, [I] - error to rate
 *
 * @return
 *  A number with a relative precedence value.
 *  This number may be compared with other precedence values from this
 *  function.
 *
 * HISTORY:
 *  04/01/03:  RPF -- Created.
 *
 * @author
 *  05/28/2009  Ron Proulx - Updated from rg_get_error_precidence
 *
 **************************************************************************/

fbe_raid_common_error_precedence_t fbe_raid_siots_get_error_precedence(fbe_payload_block_operation_status_t error,
                                                                       fbe_payload_block_operation_qualifier_t qualifier)
{
    fbe_raid_common_error_precedence_t priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_NO_ERROR;
    
    /* Determine the priority of this error
     * Certain errors are more significant than others.
     */
    switch (error)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            /* The only way this could occur is if the I/O was never
             * executed.  Assumption is that it is retryable.
             */
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_INVALID_REQUEST;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            if (qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED)
            {
                priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_STILL_CONGESTED;
            }
            else
            {
                priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_NO_ERROR;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            /* May not want to retry this request.  Up to the client.
             */
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_TIMEOUT;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            /* Set precedence.
             */
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_MEDIA_ERROR;
            break;


        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            /* Device not usable therefore don't retry (at least until the
             * device is replaced).
             */
            if (qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED)
            {
                priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_CONGESTION;
            }
            else
            {
                priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_DEVICE_DEAD;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            /* The request was purposfully aborted.  It is up to the
             * originator to retry the request.
             */
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_ABORTED;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            /* With support of drive spin-down this may occur and depending
             * on the situation (i.e. if there is redundancy etc.) the I/O
             * may be retried.
             */
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_NOT_READY;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            /* Request was not properly formed, etc. Retryable.
             */
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_INVALID_REQUEST;
            break;

        default:
            /* We don't expect this so in check builds we assert.  Otherwise
             * use the highest priority error.
             */
            if(RAID_COND(error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "raid: %s :error 0x%x != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID\n",
                                       __FUNCTION__, error);
            }
            priority = FBE_RAID_COMMON_ERROR_PRECEDENCE_UNKNOWN;
            break;
    }

    return priority;
}
/* end fbe_raid_siots_get_error_precidence */

/*!**************************************************************************
 *              fbe_raid_siots_process_error()
 ***************************************************************************
 * @brief
 *  When a SIOTS finishes we need to either save it's error in the IOTS,
 *  or preserve the current IOTS error (if it takes precidence).
 * 
 *  If the IOTS does not have an error, then give the IOTS the SIOTS error.
 *  Otherwise, we need to compare the precidence of the existing IOTS error
 *  and the new SIOTS error and choose one to save in the IOTS.
 *
 * @param   siots_p, [I] - siots to determine error for.
 *
 * @return fbe_status_t
 *
 * @note    The iots status is saved in the block operation status of
 *          the packet.
 *
 * HISTORY:
 *  04/01/03:  RPF -- Created.
 *
 * @author
 *  05/28/2009  Ron Proulx  - Created from rg_siots_process_error
 *
 **************************************************************************/

static fbe_status_t fbe_raid_siots_process_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_status_t siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;
    fbe_payload_block_operation_status_t iots_status;
    fbe_payload_block_operation_qualifier_t iots_qualifier;
    fbe_status_t status = FBE_STATUS_OK;
    /* Get the iots and current siots block status and qualifier.
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);
    fbe_raid_iots_get_block_status(iots_p, &iots_status);
    fbe_raid_iots_get_block_qualifier(iots_p, &iots_qualifier);
            
    /* Assert that there was no status set in the siots.
     */
    if(RAID_COND((siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) ||
       (siots_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_status 0x%x == INVALID or siots_qualifier 0x%x == INVALID\n",
                             siots_status, siots_qualifier);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If this siots is in error, trace it.
     */
    if (siots_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        fbe_raid_siots_log_error(siots_p, "Errored siots");
    }

    /* Use block operation status to determine the current
     * iots status.
     */
    if (iots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) 
    {
        /* We have not yet saved any status in the iots, 
         * just set the current siots status in the iots
         * There is no precidence to determine.
         */ 
        fbe_raid_iots_set_block_status(iots_p, siots_status);
        fbe_raid_iots_set_block_qualifier(iots_p, siots_qualifier);
    }
    else
    {
        fbe_raid_common_error_precedence_t iots_error_precedence, siots_error_precedence;
        
        /* Else determine the precidence of errors.
         */
        iots_error_precedence = fbe_raid_siots_get_error_precedence(iots_status, iots_qualifier);
        siots_error_precedence = fbe_raid_siots_get_error_precedence(siots_status, siots_qualifier);
        
        /* Only if the siots status takes precedence do we update the iots status.
         */
        if ( siots_error_precedence > iots_error_precedence )
        {
            /* The SIOTS error clearly takes precidence,
             * over the IOTS error, so overwrite the iots error.
             */
            fbe_raid_iots_set_block_status(iots_p, siots_status);
            fbe_raid_iots_set_block_qualifier(iots_p, siots_qualifier);
        }
    }
    return status;
}
/* end fbe_raid_siots_process_error() */

/*!**************************************************************
 * fbe_raid_siots_freed()
 ****************************************************************
 * @brief
 *  When we free the siots, we put it into this state so that
 *  it cannot be executed again.
 *
 * @param siots_p - current I/O.               
 *
 * @return FBE_RAID_STATE_STATUS_DONE
 *
 * @author
 *  9/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_freed(fbe_raid_siots_t *siots_p)
{
    /* Panic - Something is wrong in freeing siots. CRITICAL ERROR 
     * takes care of panic. 
     */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                "raid: %s siots is freed: siots_p = 0x%p\n", 
                                __FUNCTION__, siots_p);
    /* return is meaningless here since we are taking a PANIC. 
     * Disable or Remove this return when the PANIC is enabled in
     * trace_to_ring().
     */
    return FBE_RAID_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_raid_siots_freed()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_finished_state1()
 ****************************************************************
 * @brief
 *  Optionally check the checksums on read requests.  
 *
 * @param siots_p - current I/O.
 *
 * @return fbe_raid_state_status_t.
 *
 * @author
 *  2/10/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_siots_finished_state1(fbe_raid_siots_t *siots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_status_t status = FBE_STATUS_OK;
#if 0
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(fbe_raid_siots_get_raid_geometry(siots_p));
#endif
    fbe_class_id_t  class_id = fbe_raid_geometry_get_class_id(fbe_raid_siots_get_raid_geometry(siots_p));

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished_state2);

    /* Check the checksums if it is a read operation and not a metadata operation.
     */
    if ( (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
        !fbe_raid_siots_is_metadata_operation(siots_p)          )
    {
        #if 0
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                               "read object: %d siots lba: %llx bl: 0x%x st: 0x%x iots lba: %llx bl: 0x%x st: 0x%x\n",
                               object_id, siots_p->start_lba, siots_p->xfer_count, siots_p->error,
                               iots_p->lba, iots_p->blocks, iots_p->error);
        #endif
        /* Only check if the operation was successful since the data will only be 
         * valid in this case. 
         */
        if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
            (iots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
        {
            if (iots_p->blocks_transferred + siots_p->xfer_count == iots_p->blocks)
            {
                fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;

                /*! @note Even if error injection is no longer enabled we cannot
                 *        check for invalidated sectors since there could be latent
                 *        invalidations caused by error injection.
                 */
                if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
                {
                    /* Do not check the checksum for a virtual drive since the 
                     * virtual does not execute verify and there could be latent
                     * invalidated sectors.
                     */
                }
                else
                {
                    /* Check the checksums of this operation.
                     */ 
                    status = fbe_raid_xor_check_iots_checksum(siots_p);
                    if (RAID_COND(status != FBE_STATUS_OK))
                    {
                        fbe_raid_siots_trace(siots_p,
                                             FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                             "raid: failed to check checksum: status = 0x%x, siots_p = %p\n",
                                             status, siots_p);
    
                        return FBE_RAID_STATE_STATUS_EXECUTING;
                    }
                    else
                    {
                        fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
                        if(RAID_COND(xor_status != FBE_XOR_STATUS_NO_ERROR))
                        {
                            /* XOR operation failed. Report is as critical error.
                             */
                            fbe_raid_siots_trace(siots_p,
                                                 FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                 "raid: unexpected xor status: xor_status = 0x%x, siots_p = %p\n",
                                                 xor_status, siots_p);
                            return FBE_RAID_STATE_STATUS_EXECUTING;
                        }
                    }
                }
            } /* end if (iots_p->blocks_transferred + siots_p->xfer_count == iots_p->blocks)*/
        }
        /* check if the operation has media error.
         * If so, make sure the checksum is not unknown. 
         */
        if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) &&
            (iots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR))
        {
            if (iots_p->blocks_transferred + siots_p->xfer_count == iots_p->blocks)
            {
                fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;
                fbe_xor_error_t * eboard_p = NULL;
                /* Check the checksums of this operation.
                 */ 
                status = fbe_raid_xor_check_iots_checksum(siots_p);
                if (RAID_COND(status != FBE_STATUS_OK))
                {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                         "raid: failed to check checksum: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);

                    return FBE_RAID_STATE_STATUS_EXECUTING;
                }
                else
                {
                    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
                    eboard_p = fbe_raid_siots_get_eboard(siots_p);
                    if(RAID_COND(xor_status != FBE_XOR_STATUS_CHECKSUM_ERROR)
                       ||RAID_COND(eboard_p->crc_unknown_bitmap != 0))
                    {
                        /* We have a checksum error, but the reason is unknow. Report is as critical error.
                         */
                        fbe_raid_siots_trace(siots_p,
                                             FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                             "raid: Unknown checksum: xor_status = 0x%x, crc_unknown_bitmap = 0x%x\n",
                                             xor_status, eboard_p->crc_unknown_bitmap);
                        return FBE_RAID_STATE_STATUS_EXECUTING;
                    }
                }
            } /* end if (iots_p->blocks_transferred + siots_p->xfer_count == iots_p->blocks)*/
        }        
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_raid_siots_finished_state1()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_modify_invalid_blocks()
 ****************************************************************
 * @brief
 *   Modify a blocks that were invalidated with a
 *   corrupt data reason to have a corrupt data invalidated reason.
 *
 *   We do this to notify the upper levels that these blocks
 *   cannot be fixed.
 *
 * @param iots_p -               
 *
 * @return fbe_status_t
 *
 * @author
 *  12/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_modify_invalid_blocks(fbe_raid_iots_t *iots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_block_count_t total_blocks;
    fbe_sg_element_t *sg_p = NULL;

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_current_op_blocks(iots_p, &total_blocks);

    if (fbe_payload_block_operation_opcode_is_media_read(opcode))
    {
        fbe_raid_iots_get_sg_ptr(iots_p, &sg_p);

        /* The sector was invalidated.  But there are cases where retrying with the
         * check checksum flag may be able to recover the data.  Currently sectors
         * invalidated for the following reasons are retryable:
         *  o The sector was purposefully invalidated to test sector reconstruct (corrupt data)
         *  o Copy invalidated due to an error on the source drive
         *  o Provision drive invalidated due to lost paged metadata
         *  o etc.
         *
         * Now we have attempted hte reconstruction but it did not succeed.
         * Therefore we must convert the reason from a retryable error to a
         * non-retryable error (reason).
         */
        status = fbe_xor_convert_retryable_reason_to_unretryable(sg_p, 
                                                                 total_blocks);
    }

    return status;
}
/******************************************
 * end fbe_raid_iots_modify_invalid_blocks()
 ******************************************/
/*!**************************************************************************
 *          fbe_raid_siots_finished_state2()
 ***************************************************************************
 * @brief
 *  A SUB_IOTS is complete, free the SUB_IOTS and any other
 *  resources the SUB_IOTS has allocated.  Fru tracking structures,
 *  buffers and SG lists will be freed here.
 *  This routine also kicks off any waiting requests on this or
 *  other groups.
 *
 * @params  siots_p:  [IO], A ptr to the current SUB_IOTS to deallocate.
 *
 * @return  state_status - FBE_RAID_STATE_STATUS_DONE
 *
 * @note    The original request information and iots status is now contained
 *          in the originating packet.
 *
 * HISTORY:
 *  07/20/99:  RPF -- Created. 
 *  01/30/99:  RPF -- Significant changes for scheduling.
 *  09/09/01:  RPF -- Renamed to rg_siots_finished_state2 from rg_siots_finished,
 *                    as rg_siots_finished was converted to a function ptr.
 *
 * @author
 *  05/21/2009  Ron Proulx  - Updated from rg_siots_finished_state2
 **************************************************************************/

static fbe_raid_state_status_t fbe_raid_siots_finished_state2(fbe_raid_siots_t *siots_p)
{
    fbe_raid_iots_t    *iots_p = NULL;
    fbe_bool_t iots_complete = FBE_FALSE;
    fbe_block_count_t iots_blocks;
    fbe_block_count_t blocks_transferred;
    fbe_payload_block_operation_status_t    iots_status;
    fbe_payload_block_operation_status_t    siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;
    fbe_u32_t siots_count;
    fbe_raid_geometry_t *geo_p = NULL;
    fbe_object_id_t object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *restart_siots_p = NULL;

    /* Validate siots.
     */
    if(RAID_FINISH_COND(siots_p == NULL))
    {
        /* We can not crawl further from here. So, just report it as
         * critical error and fake that we are done. There is no
         * point in calling fbe_raid_siots_set_unexpected_error() as 
         * we will never get a chance to run it.
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                               "raid: %s errored: : siots is null\n",
                               __FUNCTION__);
        return FBE_RAID_STATE_STATUS_DONE;
    }

    iots_p = fbe_raid_siots_get_iots(siots_p);
    geo_p = fbe_raid_iots_get_raid_geometry(iots_p);
    object_id = fbe_raid_geometry_get_object_id(geo_p);
    siots_count = iots_p->outstanding_requests;
    iots_status = iots_p->error;
    
    if (fbe_raid_siots_is_quiesced(siots_p) && !fbe_raid_siots_is_metadata_operation(siots_p))
    {
        /*! We do not expect to be here if we are quiesced.  This indicates an issue 
         * with not checking for quiesced prior to sending out the I/O. 
         * Metadata operations do not get quiesced. 
         */
        
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                    "raid: errored: wait flags (0x%x) are set, siots_p = 0x%p\n",
                                    siots_p->flags, siots_p);
    }
    /* Get the iots status and the block opertion.
     */
    if(RAID_FINISH_COND(iots_p == NULL))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                    "raid: %s failed: iots_p extracted from siots_p 0x%p is NULL\n",
                                    __FUNCTION__,
                                     siots_p);
        return FBE_RAID_STATE_STATUS_DONE;
    }
    fbe_raid_iots_get_block_status(iots_p, &iots_status);

    /* Use the payload for the original request information.
     */
    fbe_raid_siots_get_blocks(siots_p, &iots_blocks);

    /* We should have set our status to something.
     */
    if(RAID_COND(siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                              "raid:%s errored: siots_p->error 0x%x="
                              "PAYLOAD_BLK_OP_ST_INVALID>\n",
                              __FUNCTION__, siots_p->error);
		fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                              "raid:%s siots_p = 0x%p, iots_p = 0x%p<\n",
                              __FUNCTION__,siots_p, iots_p);
    }

    /* Log completion if enabled
     */
    fbe_raid_siots_log_traffic(siots_p, "finished");

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_FALSE);

    /***************************************************
     * Free our siots and all associated resources.
     ***************************************************/

    /* Dequeue the siots from the iots and restart waiters on the iots queue.
     * Note that we leave the outstanding_requests set and we do not increment blocks 
     * transferred to prevent the iots from getting completed until below. 
     */
    fbe_raid_iots_lock(iots_p);

    /* Free everything the siots allocated so that any siots we kick off can use these.
     */ 
    status = fbe_raid_siots_destroy_resources(siots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* We failed to free siots resources.
         */
        fbe_raid_siots_trace(siots_p,FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                             "siots:0x%p fail free siots res iots=0x%p st:0x%x\n",
                             siots_p, iots_p, status);
        fbe_raid_iots_unlock(iots_p);
        return FBE_RAID_STATE_STATUS_DONE;
    }

    /* Dequeue the siots from the iots.
     */
    if(RAID_FINISH_COND(fbe_queue_is_element_on_queue(&siots_p->common.queue_element) != FBE_TRUE) )
    {
        /* If we got here, we can no longer proceed. So, we will return 
         * saying we are done. However, we will release the lock so that 
         * others are not held here.
         */
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                             "raid:%s fail: siots->common.queue_elem 0x%p is "
                             "not on queue>\n",
                             __FUNCTION__,
                             &siots_p->common.queue_element);
		fbe_raid_siots_trace(siots_p,
                             FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                             "%s siots=0x%p,iots=0x%p<\n",
                             __FUNCTION__,
                             siots_p,
                             iots_p);
        fbe_raid_iots_unlock(iots_p);
        return FBE_RAID_STATE_STATUS_DONE;
    }
    fbe_queue_remove(&siots_p->common.queue_element);

    /* Restart any siots that were waiting for SIOTS locks.
     * Clear the owner flag since we are not the owner anymore. 
     */
    fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER);
    fbe_raid_siots_get_first_lock_waiter(siots_p, &restart_siots_p);

    /* Increment blocks transferred,
     * dequeue siots from the iots and decrement outstanding requests.
     */

    /* If this siots successfully completed update the transferred count.
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);
    if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* Update blocks transferred in the iots.
         */
        iots_p->blocks_transferred += siots_p->xfer_count;
    }

    /* If the siots completed successfully and the iots isn't in error
     * set the iots status and qualifier based on this siots.  Otherwise
     * invoke process error to determine the error precedence.
     */
    if ( (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        !(fbe_raid_iots_is_errored(iots_p))                              )
    {
        /* Update the iots status.
         */
        fbe_raid_iots_set_block_status(iots_p, siots_status);
        fbe_raid_iots_set_block_qualifier(iots_p, siots_qualifier);
    }
    else
    {        
        /* Determine if we should save this SIOTS error in the IOTS.
         */
        status = fbe_raid_siots_process_error(siots_p);
        if(RAID_COND(status != FBE_STATUS_OK))
        {
             fbe_raid_siots_trace(siots_p,
                                  FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                  "%s fail proc err:siots=0x%p,iots=0x%p,st=0x%x\n",
                                  __FUNCTION__, siots_p, iots_p, status);
        }
    }
            
    /*  Get the updated iots status and make sure it was set properly.
     */
    fbe_raid_iots_get_block_status(iots_p, &iots_status);
    if(RAID_COND(iots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID))
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s fail get block st:iots_st=0x%x,iots=0x%p\n",
                                    __FUNCTION__,
                                    iots_status,
                                    iots_p);
    }

    /* Clear this flag so that the embedded siots can be re-used.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS))
    {
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE);
    }

    /* Decrement the SIOTS count of this IOTS.
     * We do this only *after* we free up resources since the packet contains resources 
     * and another siots completing could return the packet.
     */
    iots_p->outstanding_requests--;

    /* If we are here, we should not have sent status yet. Also report it as a 
     * critical error.
     */
    if(RAID_FINISH_COND(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_STATUS_SENT)) )
    {
         fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                "%s:unexp err: iots=0x%p,iots->flags 0x%x set"
                                "to IOTS_FLAG_ST_SENT\n",
                                __FUNCTION__, 
                                iots_p,
                                iots_p->flags);

        /* Release the lock so that others are not stuck because of us.
         * Also, we no longer need lock to update the critical data.
         */
        fbe_raid_iots_unlock(iots_p);
        return FBE_RAID_STATE_STATUS_DONE;
    }
    fbe_raid_iots_get_blocks_transferred(iots_p, &blocks_transferred);

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ABORT) &&
        ((iots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) ||
         (iots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) &&
        (iots_p->outstanding_requests == 0) &&
        (blocks_transferred != iots_blocks) &&
        !fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS) &&
        !fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS))
    {
        /* This was aborted, and did not finish the request successfully.
         * When a request gets aborted, we do not set the status to bad since the request might finish succesfully. 
         * We need to turn the status into a bad status so that we do not return an invalid status.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
    }

    /* Decide if we can send status.
     */
    if (blocks_transferred == iots_blocks)
    {
        /* The iots is completed.
         */
        if(RAID_COND((iots_p->blocks_remaining != 0) ||
                     (iots_p->outstanding_requests != 0) ) )
        {
            /* if we are here, we can not do much here. 
             */
            /*Split trace to two lines*/
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                       "%s: unexp err: iots->blocks_remaining 0x%llx != 0 or \n",
                                       __FUNCTION__, 
                                       (unsigned long long)iots_p->blocks_remaining);

            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                       "%s: iots->outstanding_req 0x%x!=0:iots=0x%p\n",
                                       __FUNCTION__,iots_p->outstanding_requests,
                                       iots_p);
        }
            
        /* Make sure that we aren't trying to allocate another siots. 
         */
        if ( RAID_COND(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS)) ||
             RAID_COND(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS)) ||
             RAID_COND(!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATE_DONE)) )
        {
            /* if we are here, we can not do much here.
             */
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                       "%s:gen flag unexp:iots->flags:0x%x iots:0x%p\n",
                                       __FUNCTION__, iots_p->flags, iots_p);
        }
        iots_complete = FBE_TRUE;

        /* We should not have completed this already.
         */
        if(RAID_COND(iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE))
        {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                       "%s:unexp err:iots_st 0x%x=IOTS_STAT_COMPL:"
                                       "iots=0x%p\n",
                                       __FUNCTION__, 
                                       iots_status,
                                       iots_p);
        }
        fbe_raid_iots_mark_complete(iots_p);
    }
    else if ((iots_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
             (iots_p->outstanding_requests == 0))
    {
        /* Else in an error scenario, a timing situation exists,
         * where our outstanding count really is zero, 
         * but we are really still midstream, attempting to generate SIOTS. 
         * The RIOTS_ALLOCATING_SIOTS bit will still be set in this case and it will indicate that We should hold off 
         * before we free the IOTS. 
         * Similarly if we are simply in the process of kicking off the allocation then the generating bit 
         * will be set and we should hold off freeing the iots in this case also. 
         */
        if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS) &&
            !fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS))
        {
            /***************************************************
             * An ERROR was taken on this request.
             * The only possible action from here
             * is to return status, we will not
             * allow any further SIOTS requests to be generated.
             ***************************************************/
            iots_complete = FBE_TRUE;
            /* We should not have completed this already.
             */
            if(RAID_COND(iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE))
            {
                /* We should not be marked completed yet.
                 */
               
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "err:iots 0x%p already mark compl\n", iots_p);
                
            }
            fbe_raid_iots_mark_complete(iots_p);
        }
    }    /* end ERROR and outstanding_requests == 0 */
    
    /* We are done using the siots, so we can destroy it.
     */
    fbe_raid_siots_destroy(siots_p);

    fbe_raid_iots_unlock(iots_p);

    /*************************************************** 
     * IMPORTANT: 
     *  
     * After this point we cannot touch the siots. 
     * We already decremented the outstanding_requests 
     * and have released the iots lock. 
     * Releasing the lock could have allowed something else to 
     * run and complete the iots.  Thus our siots memory might 
     * not be available anymore. 
     ***************************************************/

    siots_p = NULL;

    /* If the iots is complete cleanup.
     */
    if (iots_complete)
    {
        fbe_payload_block_operation_status_t block_status;
        fbe_payload_block_operation_qualifier_t block_qualifier;
        fbe_raid_iots_get_block_status(iots_p, &block_status);
        fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);

        /* If we have a media error, check and switch any DAQ invalidated blocks
         * to a pattern that tells cache they cannot be fixed. 
         */
        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) &&
            (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST))
        {
            fbe_raid_iots_modify_invalid_blocks(iots_p);
        }
        /* Now complete the packet associated with the iots.
         * This must be done from the state machine after completing the
         * siots.
         */
        fbe_raid_common_state((fbe_raid_common_t *)iots_p);
    }    /* end if iots is complete. */

    if (restart_siots_p != NULL)
    {
        fbe_raid_common_state(((fbe_raid_common_t *) restart_siots_p));
    }
    /* Return to state machine to process next request.
     */
    return(FBE_RAID_STATE_STATUS_DONE);
}
/*****************************************
 * end fbe_raid_siots_finished_state2()
 *****************************************/
/*!**************************************************************
 *  fbe_raid_siots_finished_check_data()
 ****************************************************************
 * @brief
 *  The siots is done, first check the data pattern for 
 *  correctness before completing this siots.
 *
 * @param  siots - Current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  1/20/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_finished_check_data(fbe_raid_siots_t *siots_p)
{
    fbe_raid_iots_t    *iots_p = NULL;
    fbe_raid_geometry_t *geo_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_status_t siots_status;

    /* Initialize hte values we need to determine if we should check the data
     * or not.
     */
    iots_p = fbe_raid_siots_get_iots(siots_p);
    geo_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Only check the data if the operation was successful and it is a read or
     * write operation.
     */
    if (fbe_raid_is_option_flag_set(geo_p, FBE_RAID_OPTION_FLAG_DATA_CHECKING) &&
        (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)           &&
        ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)   )                 )
    {
        if (iots_p->blocks_transferred + siots_p->xfer_count == iots_p->blocks)
        {
            /* For cached operations, we
             * only check data when the transfer is complete.
             * We must wait for the whole
             * transfer to be done, so we may check the 
             * entire SG list that was presented to 
             * the raid driver.
             */
            status = fbe_raid_siots_check_data_pattern(siots_p);
        }
    }
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished_state2);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************
 * end  fbe_raid_siots_finished_check_data()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_nested_process_error()
 ****************************************************************
 * @brief
 *  Nested siots finished in error.  Trace information if enabled
 *
 * @param  siots_p - current I/O.               
 *
 * @return status
 *
 ****************************************************************/
static fbe_status_t fbe_raid_siots_nested_process_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Generate an error trace if enabled.
     */
    if ( fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_ERROR_TRACE_SIOTS_ERROR) &&
        !fbe_raid_siots_is_metadata_operation(siots_p)                                                                )
    {
        fbe_trace_level_t   trace_level = FBE_TRACE_LEVEL_ERROR;
        fbe_payload_block_operation_status_t siots_status;
        fbe_payload_block_operation_qualifier_t siots_qualifier;

        fbe_raid_siots_get_block_status(siots_p, &siots_status);
        fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);

        /*! @note For a virtual drive generate a warning.
         */
        if (fbe_raid_geometry_get_class_id(raid_geometry_p) == FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            trace_level = FBE_TRACE_LEVEL_WARNING;
        }
        fbe_raid_siots_object_trace(siots_p, trace_level, __LINE__, __FUNCTION__,
                               "nest siots: %x lba:0x%llX bl:0x%llx alg:0x%x sts:0x%x qual:0x%x\n",
                               fbe_raid_memory_pointer_to_u32(siots_p), (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, 
                               siots_p->algorithm, siots_status, siots_qualifier);
    }
    return FBE_STATUS_OK;
}
/********************************************
 * end  fbe_raid_siots_nested_process_error()
 ********************************************/

/*!**************************************************************
 * fbe_raid_siots_nest_finished()
 ****************************************************************
 * @brief
 *  Finish a nested siots by freeing resources and
 *  waking up the parent if needed.
 *
 * @param  siots_p - current I/O.               
 *
 * @return FBE_RAID_STATE_STATUS_DONE
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_siots_nest_finished(fbe_raid_siots_t *siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_bool_t b_wakeup_parent = FBE_TRUE;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_status_t siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;
    fbe_raid_siots_t *restart_siots_p = NULL;

    iots_p = fbe_raid_siots_get_iots(siots_p);
    
    fbe_raid_siots_log_traffic(siots_p, "nest_siots_finished"); 

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_FALSE);

    /* If we are degraded, then set the parent's dead_pos
     * accordingly.  If nest siots is not degraded,
     * don't overwrite the parent's dead_pos.
     */
    if ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS )
    {
        fbe_raid_siots_dead_pos_set(parent_siots_p, FBE_RAID_FIRST_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
        fbe_raid_siots_dead_pos_set(parent_siots_p, FBE_RAID_SECOND_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);

        fbe_raid_siots_dead_pos_set( parent_siots_p, FBE_RAID_FIRST_DEAD_POS,
                               fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) );
        fbe_raid_siots_dead_pos_set( parent_siots_p, FBE_RAID_SECOND_DEAD_POS,
                               fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) );
    }

    if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED)
    {
        if(RAID_COND(!fbe_raid_siots_is_aborted(siots_p)))
        {
            /* siots is not aborted as expected as expected. We do have to free resources so, we will 
             * not stop here.
             */
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                 "raid: %s got unexpected error as siots_p 0x%p failed to abort.\n",
                                 __FUNCTION__,
                                 siots_p);
        }
    }

    /*! @note We must lock at the iots since the iots destroy will also attempt
     *        to destroy nested siots.
     */    
    fbe_raid_iots_lock(iots_p);

    /* Validate that we havce completed the siots
     */
    if(RAID_FINISH_COND(siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID))
    {
         /* siots has not executed as expected. Report it as critical error 
          * and reutrn completion status from here.
          */
         fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "raid: %s got unexpected error as siots_p 0x%p had invalid block operation.\n",
                              __FUNCTION__, siots_p);
         fbe_raid_iots_unlock(iots_p);
         return FBE_RAID_STATE_STATUS_DONE;
    }

    /* Validate that this siots is on the parent list, then remove it
     */
    if(RAID_FINISH_COND(fbe_queue_is_element_on_queue(&siots_p->common.queue_element) != FBE_TRUE))
    {
        /* If we are here then there is a possibility that current 
         * siots is already completed. Report critical error and 
         * say that we are done.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                             "raid: %s got unexpected error. siots_p->common.queue_element 0x%p, no element on queue.\n",
                             __FUNCTION__, siots_p);
        fbe_raid_iots_unlock(iots_p);
        return FBE_RAID_STATE_STATUS_DONE;
    }
    fbe_queue_remove(&siots_p->common.queue_element);

    /* Set the parent's error and qualifier from the child's error.
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);
    fbe_raid_siots_set_block_status(parent_siots_p, siots_status);
    fbe_raid_siots_set_block_qualifier(parent_siots_p, siots_qualifier);

    /* Handle any nested siots error processing.
     */
    if (siots_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        fbe_raid_siots_nested_process_error(siots_p);
    }

    /* For degraded parity verify need to decrement wait count
     */
    if ( siots_p->algorithm == R5_DEG_VR ||
         siots_p->algorithm == R5_DEG_RVR )
    {
		/* We under the iots lock. see Ln 969 */
		/* Lockless siots */
        //fbe_raid_siots_lock(parent_siots_p);

        /* The parent is assuming we will decrement the wait_count
         * before waking them up.
         */
        if(RAID_FINISH_COND(parent_siots_p->wait_count == 0 ))
        {
            /* if we are here, it means that we have been already 
             * processed. Report it as critical error and say 
             * that we are done.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                 "raid: %s got unexpected error for sitos_p = 0x%p. "
                                 "parent_siots_p->wait_count 0x%llx == 0.\n",
                                 __FUNCTION__, 
                                 siots_p,
                                 parent_siots_p->wait_count);
			/* We under the iots lock. see Ln 969 */
			/* Lockless siots */
            //fbe_raid_siots_unlock(parent_siots_p);
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_DONE;
        }
        
        if ( parent_siots_p->wait_count > 0 )
        {
            /* This is defensive code for free builds.
             */
            parent_siots_p->wait_count--;
        }
        if (parent_siots_p->wait_count > 0)
        {
            /* The parent is still waiting for something,
             * so it does not need for us to wake them up yet.
             */
            b_wakeup_parent = FBE_FALSE;
        }

		/* We under the iots lock. see Ln 969 */
		/* Lockless siots */
        //fbe_raid_siots_unlock(parent_siots_p);
    }

    /* Destroy any resources used by this siots
     */
    fbe_status = fbe_raid_siots_destroy_resources(siots_p);
    if (RAID_MEMORY_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                             "raid: siots_p: 0x%p failed to free resources with status: 0x%x\n",
                             siots_p, fbe_status);
         fbe_raid_iots_unlock(iots_p);
         return FBE_RAID_STATE_STATUS_DONE;
    }

    /* Now we can destroy
     */
    fbe_raid_siots_destroy(siots_p);

    /* Since other SIOTS might be waiting because of this SIOTS, try to wake them now. 
     * We put this here because by this point we have been dequeued. 
     */
    fbe_raid_siots_get_first_lock_waiter(parent_siots_p, &restart_siots_p);

    /* Now unlock the iots
     */
    fbe_raid_iots_unlock(iots_p);

    if (restart_siots_p != NULL)
    { 
        /* Restart any waiter.
         */ 
        fbe_raid_common_state(((fbe_raid_common_t *) restart_siots_p));
    }
    if (b_wakeup_parent)
    {
        /* We only wakeup the parent if the parent is ready.
         */
        fbe_raid_common_state(((fbe_raid_common_t *) parent_siots_p));
    }

    siots_p = NULL;
    
    return FBE_RAID_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_raid_siots_nest_finished()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_set_unexpected_error()
 ****************************************************************
 * @brief
 *  For a given siots transition it to an unexpected error state.
 * 
 * @param siots_p - siots to transition to failed.
 * @param line - The line number in source file where failure occurred.
 * @param function_p - Function that detected error
 * @param fail_string_p - This is the string describing the failure.
 *
 * @return - None.
 *
 * @author
 *  10/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_set_unexpected_error(fbe_raid_siots_t *const siots_p,
                                         fbe_u32_t line,
                                         const char * function_p,
                                         fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RAID_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RAID_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[127] = '\0';
        va_end(argList);
    }
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: siots: 0x%p Unexpected error\n", siots_p);

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: siots: 0x%x line: %d function: %s\n", 
                                fbe_raid_memory_pointer_to_u32(siots_p), line, function_p);
    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,buffer);
    }
    /* Transition this siots to a failure state.
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
    return;
}
/**************************************
 * end fbe_raid_siots_set_unexpected_error()
 **************************************/
/*!**************************************************************
 * fbe_raid_siots_condition_failure()
 ****************************************************************
 * @brief
 *  Trace out failure information for this siots.
 * 
 * @param siots_p - siots to transition to failed.
 * @param line - The line number where failure occurred.
 * @param file_p - File of failure.
 * @param fmt - This is the string describing the failure.
 *
 * @return - None.
 *
 * @author
 *  10/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_condition_failure(fbe_raid_siots_t *siots_p,
                                                         const char *const condition_p,
                                                         fbe_u32_t line,
                                                         const char *const file_p)
{
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: raid siots 0x%p error\n", siots_p);
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: error: line: %d function: %s\n", line, file_p);
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
    return FBE_RAID_STATE_STATUS_EXECUTING; 
}
/**************************************
 * end fbe_raid_siots_condition_failure()
 **************************************/
/*!**************************************************************
 * fbe_raid_siots_success()
 ****************************************************************
 * @brief
 *  Set a successful status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/7/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_success(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_success()
 **************************************/
/*!**************************************************************
 * fbe_raid_siots_zeroed()
 ****************************************************************
 * @brief
 *  Set a successful status into the siots with a zeroed qualifier.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  9/21/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_zeroed(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_zeroed()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_aborted()
 ****************************************************************
 * @brief
 *  Set an aborted status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_aborted(fbe_raid_siots_t *siots_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_status_t iots_status;
    fbe_raid_iots_get_block_status(iots_p, &iots_status);

    /* If we are aborted, and there is an iots status (other than success), we must
     * preserve the iots status, since the siots is aborted check 
     * checks for more than just that the client aborted this request, 
     * it also checks to see if this iots has taken an error. 
     * In the case where the iots has taken an error, we do not want to 
     * finish this siots with the request aborted status. 
     * In the case where the iots status is success (because another siots already finished), 
     * then we need to set the status to aborted. 
     */
    if (fbe_raid_siots_is_aborted(siots_p) &&
        (iots_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) &&
        (iots_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        (iots_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR))
    {
        fbe_payload_block_operation_qualifier_t qualifier;

        fbe_raid_iots_get_block_status(iots_p, &iots_status);
        fbe_raid_iots_get_block_qualifier(iots_p, &qualifier);

        fbe_raid_siots_set_block_status(siots_p, iots_status);
        fbe_raid_siots_set_block_qualifier(siots_p, qualifier);
    }
    else
    {
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_siots_get_opcode(siots_p, &opcode);
        /* Otherwise set the aborted status in the siots.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
        if (fbe_payload_block_operation_opcode_is_media_modify(opcode) &&
            !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED))
        {
            /* For write type operations once the write is started, we will not abort the request.
             */
            fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED);
        }
        else
        {
            fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
        }
    }

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_aborted()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_expired()
 ****************************************************************
 * @brief
 *  Set an aborted status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  3/16/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_expired(fbe_raid_siots_t *siots_p)
{
    /* There is no qualifier for this error.
     */
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_expired()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_shutdown_error()
 ****************************************************************
 * @brief
 *  Set an aborted status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_shutdown_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_shutdown_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_media_error()
 ****************************************************************
 * @brief
 *  Set a media errored status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_media_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_media_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_dropped_error()
 ****************************************************************
 * @brief
 *  Set a dropped status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_dropped_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_dropped_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_dead_error()
 ****************************************************************
 * @brief
 *  Set an failed status (insufficient devices to complete request)
 *
 * @param  siots_p - current I/O.
 *
 * @return state status
 *
 * @note    This method returns `retry possible' to indicate to the
 *          raid group object that it must re-evaluate the downstream
 *          health to determine if it should retry the request or not.
 *
 * @author
 *  9/10/2009 - Created. Ron Proulx
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_dead_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_dead_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_unexpected_error()
 ****************************************************************
 * @brief
 *  Set an aborted status into the siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_unexpected_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    fbe_raid_siots_transition_finished(siots_p);

    /* We panic in checked builds since we want to catch these errors. 
     * In free builds we will allow the system to continue without panicking. 
     */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: invalid siots: 0x%p. \n", siots_p);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_unexpected_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_invalid_opcode()
 ****************************************************************
 * @brief
 *  We have encountered an invalid operation on a siots, return
 *  status now.
 *
 * @param siots_p - Our sub request.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  5/19/2009 - Created. rfoley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_invalid_opcode(fbe_raid_siots_t * siots_p)
{
    /* Setup the return status here. 
     */
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE);
    
    /* Transition to siots finished.
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_raid_siots_invalid_opcode()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_invalid_parameter()
 ****************************************************************
 * @brief
 *  We have encountered an invalid parameter on a siots, return
 *  status now.
 *
 * @param siots_p - Our sub request.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  7/29/2009 - Created. rfoley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_invalid_parameter(fbe_raid_siots_t * siots_p)
{
    /* Setup the return status here. 
     */

    /* Transition to siots finished.
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_raid_siots_invalid_parameter()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_write_crc_error()
 ****************************************************************
 * @brief
 *  Set a crc error status into the siots. This error only happens on writes.
 *
 * @param  siots_p - current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  5/11/2010 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_write_crc_error(fbe_raid_siots_t *siots_p)
{
    /* Report the fact that the client sent data with an unexpected CRC error.
     * We need to let the user know that there could be a memory issue on the SP.
     */
    fbe_raid_record_bad_crc_on_write(siots_p);

    /* CRC error is spec'd as an IO_FAILED status with a qualifier of CRC_ERROR.
     */
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_write_crc_error()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_not_preferred_error()
 ****************************************************************
 * @brief
 *  Set a not preferred error into siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_not_preferred_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_record_bad_crc_on_write(siots_p);
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_not_preferred_error()
 **************************************/


/*!**************************************************************
 * fbe_raid_siots_reduce_qdepth_hard()
 ****************************************************************
 * @brief
 *  Set a failed status of reduce qdepth into siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_reduce_qdepth_hard(fbe_raid_siots_t *siots_p)
{
    fbe_raid_record_bad_crc_on_write(siots_p);
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_reduce_qdepth_hard()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_reduce_qdepth_soft()
 ****************************************************************
 * @brief
 *  Set a success status of reduce qdepth into siots.
 *
 * @param  siots_p - current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_reduce_qdepth_soft(fbe_raid_siots_t *siots_p)
{
    fbe_raid_record_bad_crc_on_write(siots_p);
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_reduce_qdepth_soft()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_write_log_aborted()
 ****************************************************************
 * @brief
 *  The attempt to get a write log slot was aborted while the
 *  request was waiting for a slot.
 *
 * @param  siots_p - current I/O.
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  9/2/2010 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_write_log_aborted(fbe_raid_siots_t *siots_p)
{
    /* write log aborted is spec'd as an IO_FAILED status with a qualifier of WRITE_LOG_ABORTED.
     */
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WRITE_LOG_ABORTED);

    fbe_raid_siots_transition_finished(siots_p);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_siots_write_log_aborted()
 **************************************/

/*!**************************************************************
 *          fbe_raid_siots_memory_allocation_error()
 ****************************************************************
 * @brief
 *  The memory service returned an error to an allocation request
 *
 * @param  siots_p - current I/O.
 *
 * @return None.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_raid_siots_memory_allocation_error(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    fbe_raid_siots_transition_finished(siots_p);

    /* We panic in checked builds since we want to catch these errors. 
     * In free builds we will allow the system to continue without panicking. 
     */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: memory allocation error for siots: 0x%p. \n",
                                siots_p);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**********************************************
 * end fbe_raid_siots_memory_allocation_error()
 **********************************************/

/*!**************************************************************
 * fbe_raid_siots_degraded()
 ****************************************************************
 * @brief
 * Siots is aborted because request started as non-degraded but went 
 * degraded while in progress. Request when retried will be setup
 * as degraded IO. 
 *
 * Note: This is used in Write SMs to bailout if RG goes degraded
 * before it issues write to live stripe (for ex: during pre-reads)  
 *
 * @param siots_p - Our sub request.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  3/16/2012 - Created. Vamsi V
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_degraded(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAIDLIB_ABORTED);

    fbe_raid_siots_transition_finished(siots_p);

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "raid: Retry Iots as degraded IO : 0x%p. \n",
                                siots_p);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_raid_siots_degraded()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_too_many_dead_positions
 ****************************************************************
 * @brief
 *  We have encountered a case where the degraded information (from
 *  the paged metadata) indicated that there were too many degraded
 *  position to continue.  This should never occur since the raid
 *  group should not have sent the `continue / I/O' if that was the
 *  case.  Therefore this indicates a logical error that may have
 *  resulted in the paged metadata being corrupted.  We need to fix
 *  it by reporting this error.
 *
 * @param siots_p - Our sub request.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  03/12/2013  Ron Proulx  - Created.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_too_many_dead_positions(fbe_raid_siots_t * siots_p)
{
    /* Setup the return status here. 
     */
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS);
    
    /* Transition to siots finished.
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_finished);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**********************************************
 * end fbe_raid_siots_too_many_dead_positions()
 **********************************************/


/**********************************
 * end file fbe_raid_siots_states.c
 **********************************/
